# exptToHv - Get an experiment topology via xmlrpc, and write a HyperViewer .hyp file.

import sets
import string

from sshxmlrpc import *
from emulabclient import *

## Server constants.
PACKAGE_VERSION = 0.1                   # The package version number
XMLRPC_SERVER   = "boss.emulab.net"     # Default server
xmlrpc_server   = XMLRPC_SERVER         # User supplied server name.
login_id        = os.environ["USER"]    # User supplied login ID to use.
module          = "experiment"          # The default RPC module to invoke.
path            = None

## initServer - Get a handle on the server.
#
server = None
def initServer():
    global server
    uri = "ssh://" + login_id + "@" + xmlrpc_server + "/xmlrpc/" + module
    ##print uri
    server = SSHServerProxy(uri, path=path)
    pass

## intfcHost - An interface name is host:port .  Get just the host part.
#
def intfcHost(intfcName):
    return intfcName[0:string.index(intfcName,':')]

## AddConnect - Represent a topology graph as a dictionary of nodes with sets
## of their connected nodes.
#
def addConnection(graph, h1, h2):

    # Connect the first host in the link to the host at the other end.
    if h1 in graph:
        graph[h1].add(h2)
    else:
        graph[h1] = sets.Set([h2])

    # Make back-connections as well for an undirected (bi-directed) graph.
    if h2 in graph:
        graph[h2].add(h1)
    else:
        graph[h2] = sets.Set([h1])

## exptToHv - Make the request from the server.  Reconstitute a topology graph
## from the host interface list, and traverse it to write HyperViewer .hyp file.
#
# Args are the project and experiment names, and optionally the root of the topology.
# If no root is given, the first lan or the first host is the default root node.
#
# An experiment.hyp file is written in /tmp.
# Return is the .hyp file name, or None in case of failure.
#
def getExperiment(project, experiment, root=""):
    if not server:
        initServer()
    meth_args = [PACKAGE_VERSION, {'proj':project, 'exp':experiment, 'aspect':'links'}]
    response = None
    try:
        meth = getattr(server, "info")
        response = apply(meth, meth_args)
        pass
    except xmlrpclib.Fault, e:
        print e.faultString
        return None

    if response["code"] != RESPONSE_SUCCESS:
        print "XMLRPC failure, code", response["code"]
        pass
    links = response["value"]

    # Figure out the nodes from the experiment links (interfaces) from the virt_lans table.
    # These are "link ends", which are members of either inter-host links or lans.
    # Lan nodes are are implicitly represented as links with over two interfaces as members.
    linksByName = {}	# Regroup by link name into sets of members, to find lans.
    for member, link in links.items():	# The links dict is keyed by interface (member) name.
        linkName = link['name']
        if not linksByName.has_key(linkName):
            linksByName[linkName] = sets.Set()
        linksByName[linkName].add(member)	# Each link/lan connects a set of interfaces.

    # Build the connection graph as a dictionary of nodes with sets of connected nodes.
    hosts = sets.Set()	# Collect unique node names (hosts and lans.)
    lans = sets.Set()
    graph = {}
    for link, intfcs in linksByName.items():
        if len(intfcs) == 2:
            # Two interfaces connected indicates a host-to-host link.
            h1, h2 = [intfcHost(intfc) for intfc in intfcs]
            hosts.add(h1)
            hosts.add(h2)
            addConnection(graph, h1, h2)
        else:
            # Lan nodes are are links with more than two interfaces as members.
            lans.add(link)
            for intfc in intfcs:
                addConnection(graph, link, intfcHost(intfc))

    # Use the first lan or the first host as the default root node.
    if root == "":
        if len(lans) > 0:
            root = lans.copy().pop()
        else:
            root = hosts.copy().pop()

    # Walk the graph structure in depth-first order to generate the .hyp file.
    # Make a second copy of the graph as we go to avoid repeating nodes due to back-links.
    def walkNodes(graph, graph2, node, level, outfile):
        ##print level, node, 1, ['host','lan'][node in lans]
        outfile.write(str(level)+" "+node+" 1 "+['host','lan'][node in lans]+'\n')

        # Recursively traverse the nodes connected to this one.
        for conn in graph[node]:
            if not (node in graph2 and conn in graph2[node]):
                addConnection(graph2, node, conn)
                walkNodes(graph, graph2, conn, level+1, outfile)
                pass
            pass
        pass
    hypfile = "/tmp/"+experiment+'.hyp'
    outfile = file(hypfile,'w')
    graph2 = {}
    walkNodes(graph, graph2, root, 0, outfile)
    outfile.close()

    return hypfile
