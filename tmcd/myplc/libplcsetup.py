#!/usr/bin/python

import sys
import os
import os.path
import socket
import random
import traceback
import xmlrpclib
# grab plc_config
sys.path.append('/plc/root/usr/lib/python2.4/site-packages')
from plc_config import PLCConfiguration


debug = 1
cachedRootUID = None
cachedRootPasswd = None

DEF_ROOT_UID = 'root@localhost.localdomain'
DEF_ROOT_ACCOUNT_INFO_FILE = '/etc/planetlab/rootacctinfo'
DEF_PLC_CONFIG_FILE = '/plc/data/etc/planetlab/plc_config.xml'
DEF_PLC_URL = 'https://localhost/PLCAPI/'
DEF_SITE_ID = 1

def plcReadConfigVar(catID,varID,configFile=DEF_PLC_CONFIG_FILE):
    """
    Returns the value of the specified variable.
    """
    plcc = PLCConfiguration(configFile)
    ret = plcc.get(catID,varID)
    if ret == None:
        return None

    for d in ret:
        if d['id'] == varID:
            ret = d['value']
            break
        pass

    return ret

def plcUpdateConfig(variables,configFile=DEF_PLC_CONFIG_FILE):
    """
    Update the default planetlab config file.
    Arguments:
      variables = list of dicts or lists
      configFile = alternate planetlab config file

      Example: to set plc_www/host (i.e., PLC_WWW_HOST in myplc docs), do

      variables[0] = { 'category' : 'plc_www',
                       'variable' : 'host',
                       'value'    : <value> }
        or
      variables[0] = [ 'plc_www','host',<value> ]
    """
    
    plcc = PLCConfiguration(configFile)
    
    for obj in variables:
        catDict = dict()
        varDict = dict()
        
        if type(obj) == list:
            catDict['id'] = obj[0]
            varDict['id'] = obj[1]
            varDict['value'] = obj[2]
            pass
        elif type(obj) == dict:
            catDict['id'] = obj['category']
            varDict['id'] = obj['variable']
            varDict['value'] = obj['value']
            pass
        else:
            raise "Unsupported variable object!"
        
        plcc.set(catDict,varDict)
        pass
    
    plcc.save()
    pass

def getXMLRPCAuthInfo():
    readRootAcctInfo()
    
    auth = { 'Username'   : cachedRootUID,
             'AuthString' : cachedRootPasswd,
             'AuthMethod' : 'password',
             'Role'       : 'admin' }

    return auth

def getXMLRPCServer(url=DEF_PLC_URL):
    return xmlrpclib.Server(url,allow_none=True)

def plcAddUser(realname,email,passwd,keys=[],root=False):
    auth = getXMLRPCAuthInfo()
    server = getXMLRPCServer()

    (fn,ln) = (None,None)
    sn = realname.split(' ')
    if len(sn) >= 2:
        (fn,ln) = (sn[0],sn[-1])
        pass
    else:
        (fn,ln) = (sn[0],'EmulabFamily')
        pass

    pid = server.AddPerson(auth,{ 'first_name' : fn,
                                  'last_name'  : ln,
                                  'url'        : 'http://www.emulab.net',
                                  # "saw your name and number on the wall..."
                                  'phone'      : '867-5309',
                                  'password'   : passwd,
                                  'email'      : email })

    server.UpdatePerson(auth,pid,{ 'enabled' : True })

    for key in keys:
        # have to catch exceptions here cause plc only allows ssh2 keys,
        # and users could have *anything* in their authorized_keys
        try:
            server.AddPersonKey(auth,pid,{ 'key_type' : 'ssh',
                                           'key'      : key })
        except:
            pass
        pass

    if root:
        server.AddRoleToPerson(auth,'admin',pid)
        pass
    
    return pid

def plcDeleteUser(email):
    auth = getXMLRPCAuthInfo()
    server = getXMLRPCServer()

    return server.DeletePerson(auth,email)

def plcUpdateUser(realname,email,passwd,keys=[],root=False):
    auth = getXMLRPCAuthInfo()
    server = getXMLRPCServer()

    ulist = server.GetPersons(auth,
                              { 'email' : email },
                              [ 'first_name','last_name','roles','key_ids' ])

    if len(ulist) == 0:
        print "WARNING: could not find match for user %s!" % email
        return 0
    elif len(ulist) > 1:
        print "WARNING: more than one match for user %s, using first" % email
        pass
    
    (fn,ln) = (None,None)
    sn = realname.split(' ')
    if len(sn) >= 2:
        (fn,ln) = (sn[0],sn[-1])
        pass
    else:
        (fn,ln) = (sn[0],'EmulabFamily')
        pass

    # update name
    if fn != ulist[0]['first_name'] or ln != ulist[0]['last_name']:
        server.UpdatePerson(auth,email,{ 'first_name' : fn,
                                         'last_name'  : ln })
        pass

    # update roles
    if 'admin' in ulist[0]['roles'] and not root:
        server.UpdatePerson(auth,email,{ 'roles' : [ 'user' ] })
        pass

    # update keys
    retlist = server.GetKeys(auth,ulist[0]['key_ids'],[ 'key_id','key' ])
    keylist = map(lambda(x): x['key'],retlist)
    # add:
    for rk in keys:
        if not rk in keylist:
            # have to catch exceptions here cause plc only allows ssh2 keys,
            # and users could have *anything* in their authorized_keys
            try:
                server.AddPersonKey(auth,email,{ 'key_type' : 'ssh',
                                                 'key'      : rk })
            except:
                pass
            pass
        pass
    # delete:
    for ret in retlist:
        if not ret['key'] in keylist:
            server.DeletePersonKey(auth,ret['key_id'])
            pass
        pass

    # always update the password, it's easier than doing AuthCheck and all...
    server.UpdatePerson(auth,email,{ 'password' : passwd })
    
    return 1

def plcUpdateUsers(uplist=[]):
    """
    Takes a list of (realname,email,passwd,keys=[],root=False) account tuples
    and adds, deletes, and updates as needed.
    """
    auth = getXMLRPCAuthInfo()
    server = getXMLRPCServer()

    userlist = server.GetPersons(auth,{},[ 'email' ])

    # go through the uplist and ulist and figure out what needs
    # adding, deleting, or updating:
    alist = list() # list of tuples from uplist
    dlist = list() # list of hostnames to be deleted
    ulist = list() # list of tuples from uplist

    for ntuple in uplist:
        found = False
        for user in userlist:
            if ntuple[1] == user['email']:
                found = True
                break
            pass
        if found:
            ulist.append(ntuple)
            pass
        else:
            alist.append(ntuple)
            pass
        pass

    for user in userlist:
        found = False
        for ntuple in uplist:
            if ntuple[1] == user['email']:
                found = True
                break
            pass
        if not found and user['email'].endswith('emulab.net'):
            dlist.append(user['email'])
            pass
        pass

    #print "alist = %s\n\nulist = %s\n\ndlist = %s\n" % (alist,ulist,dlist)
    
    for als in alist:
        print "Adding user %s" % als[1]
        plcAddUser(*als)
        pass

    for uls in ulist:
        print "Updating user %s" % uls[1]
        plcUpdateUser(*uls)
        pass

    for dls in dlist:
        print "Deleting user %s" % dls
        plcDeleteUser(dls)
        pass

    return None

def addMACDelim(mac,delim=':'):
    if mac.count(delim) == 0:
        return mac[0:2] + delim + mac[2:4] + delim + mac[4:6] + delim + \
               mac[6:8] + delim + mac[8:10] + delim + mac[10:12]
    else:
        return mac
    pass

def plcAddNode(hostname,ip,mac):
    auth = getXMLRPCAuthInfo()
    server = getXMLRPCServer()

    fmac = addMACDelim(mac,':')
    
    nid = server.AddNode(auth,DEF_SITE_ID,{ 'hostname' : hostname })

    nnid = server.AddNodeNetwork(auth,nid,{ 'is_primary' : True,
                                            'hostname'   : hostname,
                                            'ip'         : ip,
                                            'mac'        : fmac,
                                            'method'     : 'dhcp',
                                            'type'       : 'ipv4' })

    # ugh, have to set a node key manually
    astr = "abcdefghijklmnopqrstuvwxyz"
    rstr = astr + astr.upper() + '0123456789'
    tpasslist = random.sample(rstr,32)
    tkey = ''
    for char in tpasslist:
        tkey += char
        pass

    server.UpdateNode(auth,nid,{ 'key' : tkey })
    
    return (nid,nnid)

def plcDeleteNode(hostname):
    auth = getXMLRPCAuthInfo()
    server = getXMLRPCServer()

    return server.DeleteNode(auth,hostname)

def plcUpdateNode(hostname,ip,mac):
    auth = getXMLRPCAuthInfo()
    server = getXMLRPCServer()

    fmac = addMACDelim(mac,':')

    nodelist = server.GetNodes(auth,
                               { 'hostname' : hostname },
                               [ 'nodenetwork_ids' ])

    if len(nodelist) != 1:
        print "ERROR: more than one node %s found!" % hostname
        return -1

    if len(nodelist[0]['nodenetwork_ids']) != 1:
        print "WARNING: more than one node network for %s; " + \
              "using first!" % hostname
        pass

    nnid = nodelist[0]['nodenetwork_ids'][0]

    nnlist = server.GetNodeNetworks(auth,[ nnid ],[ 'ip','mac' ])

    if len(nnlist) != 1:
        print "WARNING: more than one node network for %s; " + \
              "using first!" % hostname
        pass
    
    if ip != nnlist[0]['ip'] or fmac != nnlist[0]['mac']:
        return server.UpdateNodeNetwork(auth,nnid,{ 'ip' : ip,
                                                    'mac' : fmac })
        pass

    # even if we don't do anything, return 1.
    return 1

def plcUpdateNodes(uplist=[]):
    """
    uplist should be a list of (hostname,ip,mac) tuples.
    """
    auth = getXMLRPCAuthInfo()
    server = getXMLRPCServer()

    pnodelist = server.GetNodes(auth,{},
                                [ 'hostname','nodenetwork_ids','node_id' ])
    #pnetlist = server.GetNodeNetworks(auth,{},[ 'ip','hostname','mac' ])

    # go through the uplist and pnodelist/pnetlist and figure out what needs
    # adding, deleting, or updating:
    alist = list() # list of tuples from uplist
    dlist = list() # list of hostnames to be deleted
    ulist = list() # list of tuples from uplist

    for ntuple in uplist:
        found = False
        for pnode in pnodelist:
            if ntuple[0] == pnode['hostname']:
                found = True
                break
            pass
        if found:
            ulist.append(ntuple)
            pass
        else:
            alist.append(ntuple)
            pass
        pass

    for pnode in pnodelist:
        found = False
        for ntuple in uplist:
            if ntuple[0] == pnode['hostname']:
                found = True
                break
            pass
        if not found:
            dlist.append(ntuple[0])
            pass
        pass

    #print "alist = %s\nulist = %s\ndlist = %s" % (alist,ulist,dlist)

    for als in alist:
        print "Adding node %s" % als[0]
        plcAddNode(*als)
        pass

    for uls in ulist:
        print "Updating node %s" % uls[0]
        plcUpdateNode(*uls)
        pass

    for dls in dlist:
        print "Deleting node %s" % dls
        plcDeleteNode(dls)
        pass

    return None

# XXX: what to do when we delete a node on one swapmod, then later re-add it?
# Will both nids hang around in the db?
def plcGetNodeID(hostname):
    auth = getXMLRPCAuthInfo()
    server = getXMLRPCServer()

    nodelist = server.GetNodes(auth,{ 'hostname' : hostname },[ 'node_id' ])
    if len(nodelist) > 0:
        return nodelist[0]['node_id']
    else:
        raise "Could not find node_id for %s" % hostname

    return None

def plcGetNodeConfig(hostname):
    """
    Returns a list of strings that should be lines in a node config file.
    """
    auth = getXMLRPCAuthInfo()
    server = getXMLRPCServer()
    
    nlist = server.GetNodes(auth,
                               { 'hostname' : hostname },
                               [ 'hostname','node_id','key' ])

    if len(nlist) != 1:
        return []

    nnlist = server.GetNodeNetworks(auth,
                                    { 'hostname' : hostname },
                                    [ 'mac','method' ])

    if len(nnlist) != 1:
        return []

    (host,network) = hostname.split('.',1)

    return [ 'NODE_ID="%d"' % nlist[0]['node_id'],
             'NODE_KEY="%s"' % nlist[0]['key'],
             'NET_DEVICE="%s"' % nnlist[0]['mac'],
             'IP_METHOD="%s"' % nnlist[0]['method'],
             'HOST_NAME="%s"' % host,
             'DOMAIN_NAME="%s"' % network ]


def readRootAcctInfo(refresh=False,pwfile=DEF_ROOT_ACCOUNT_INFO_FILE):
    global cachedRootUID,cachedRootPasswd

    if not os.path.isfile(pwfile):
        return None

    if not refresh and (cachedRootUID != None and cachedRootPasswd != None):
        return (cachedRootUID,cachedRootPasswd)
    
    info = list()
    try:
        fd = open(pwfile,'r')
        line = fd.readline().strip('\n')
        while line != '':
            if not line.startswith('#'):
                info.append(line)
                pass
            line = fd.readline().strip('\n')
            pass
        pass
    except:
        if debug:
            print "Failed to read root account info from %s" % pwfile
            traceback.print_exc()
            pass
        raise

    if info == []:
        return None

    (cachedRootUID,cachedRootPasswd) = tuple(info)
    
    return tuple(info)


def writeRootAcctInfo(uid=DEF_ROOT_UID,passwd=None,
                      pwfile=DEF_ROOT_ACCOUNT_INFO_FILE):
    global cachedRootUID,cachedRootPasswd
    
    tpass = passwd
    if tpass == None:
        astr = "abcdefghijklmnopqrstuvwxyz"
        rstr = astr + astr.upper() + '0123456789' + '!@#$%^&*'
        
        tpasslist = random.sample(rstr,8)
        tpass = ''
        for char in tpasslist:
            tpass += char
            pass
        pass

    try:
        fd = open(pwfile,'w')
        fd.write("%s\n" % uid)
        fd.write("%s\n" % tpass)
        fd.close()
    except:
        if debug:
            print "Failed to write root account info to %s" % pwfile
            traceback.print_exc()
            pass
        raise

    cachedRootUID = uid
    cachedRootPasswd = tpass

    return (cachedRootUID,cachedRootPasswd)



