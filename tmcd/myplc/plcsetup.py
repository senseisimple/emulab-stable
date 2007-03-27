#!/usr/bin/python

import traceback
import sys
import os.path
import socket
import re

#
# Helper functions.
#
def runCommand(cmd):
    output = list()
    cp = os.popen(cmd,'r')
    line = cp.readline()
    while line != '':
        output.append(line)
        line = cp.readline()
        pass
    cp.close()
    return output

def getHostname():
    return runCommand('/bin/hostname')[0].strip('\n')

def doService(serviceName,serviceAction):
    #print "Sending %s to %s" % (serviceAction,serviceName)
    pid = os.fork()
    if pid == 0:
        #
        # These hacks seem necessary to prevent /sbin/service from somehow
        # giving us zombies that then hang the parent perl process in
        # rc.bootsetup.  Not sure why...
        # 
        sys.stdin.close()
        sys.stderr.close()
        sys.stdout.close()
        sys.stdout = open('/dev/null','w')
        sys.stderr = open('/dev/null','w')
        os.execvp("/sbin/service",('service',serviceName,serviceAction))
        sys.exit(0)
        pass
    else:
        os.wait()
        pass
    #retval = runCommand("/sbin/service %s %s" % (serviceName,serviceAction))
    #if retval != None:
    #    for line in retval:
    #        print line
    #        pass
    #    pass
    #return None

def readUserKeys(uid):
    """
    Reads the Emulab-generated /users/@uid@/.ssh/authorized_keys file
    and returns a list of the keys contained therein.
    """
    keyfile = '/users/%s/.ssh/authorized_keys' % uid
    fd = file(keyfile)
    keylist = list()

    for line in fd:
        if line != '' and not line.startswith('#'):
            keylist.append(line.strip('\n'))
            pass
        pass

    fd.close()

    return keylist

TMCC = '/usr/local/etc/emulab/tmcc'

def runTMCC(cmd):
    """
    Runs the tmcc command indicated and returns, for each line, a list of
    keys with no value, and a dict of key/value pairs.
    """
    retlist = list()
    retval = runCommand("%s %s" % (TMCC,cmd))
    
    for line in retval:
        rline = line.strip('\n')
        lineDict = dict()
        lineDict['onlykey'] = []

        while len(rline) > 0:
            [k,rline] = rline.split('=',1)
            if k.count(' ') > 0:
                # "key" is really multiple keys...
                realkeys = k.split(' ')
                k = realkeys[-1]
                for r in realkeys[0:-1]:
                    lineDict['onlykey'].append(r)
                    pass
                pass
            if rline[0] == '"':
                # need to split at the next '" '
                [v,rline] = rline[1:].split('"',1)
                if len(rline) > 0:
                    # get rid of the next space...
                    rline = rline[1:]
                    pass
                pass
            else:
                if rline.count(' ') > 0:
                    [v,rline] = rline.split(' ',1)
                    pass
                else:
                    v = rline
                    rline = ''
                    pass
                pass
            lineDict[k] = v
            pass
        
        retlist.append(lineDict)
        pass

    return retlist

# -4. Must be root.
# -3. Stop and mount plc.
# -2. Create some dirs we need.
# -1. Read in necessary Emulab tmcc data.
#  0. Read in (set if can't read) plc root account info.
#  1. Read in plc config, reset any of the key defaults that are incorrect.
#  2. Restart plc so we can talk to xmlrpc server and so changes are grabbed.
#  3. Do a diff on the current plc nodes/networks/users and add/remove as
#     necessary.
#  4. Extract crap from the bootcd, make config imgs, setup pxe configs.
#  5. Stop plc from running on boot, since this setup will run it.


# -4.
if not os.getuid() == 0:
    print "ERROR: must be root to run this script!"
    sys.exit(-1)
    pass

# do this right away so we can exit without doing a single thing to the plc
# config.
tmccEPlabConfig = runTMCC('eplabconfig')

if tmccEPlabConfig == None or len(tmccEPlabConfig) == 0:
    print "plabinelab: nothing to do, exiting"
    sys.exit(0)
    pass

# -3.
print "plabinelab: stopping plc:"
doService('plc','stop')
print "plabinelab: mounting plc:"
doService('plc','mount')

# ick!  need to patch the bootmanager so that 1) it doesn't ask if we're sure
# before installing, and 2) it doesn't fail the hardware check on slow nodes.
#
# Also have to patch the bootmanager/WriteNetworkConfig.py file so that it
# writes the correct ifcfg-ethX file, and does not assume eth0.
if not os.path.exists('/plc/emulab/bootmanager.patch.done'):
    print "plabinelab: patching bootmanager"
    cwd = os.getcwd()
    os.chdir('/plc/root/usr/share/bootmanager')
    os.system('patch -p0 < /plc/emulab/bootmanager.patch')
    os.chdir(cwd)
    # now do a specific one needed for plab-on-the-exp-net
    os.chdir('/plc/root/usr/share/bootmanager/source/steps')
    os.system('patch -p0 < /share/plabplc/files/WriteNetworkConfig.patch')
    os.chdir(cwd)
    bpd = open('/plc/emulab/bootmanager.patch.done','w')
    bpd.write('done\n')
    bpd.close()
    pass

# can only import this after plc is mounted; else we can't get access to the
# python modules.
from libplcsetup import *

# XXX: need to switch this stuff to be a little more intelligent so
# we can configure private planetlab networks from the control net.
DEF_PLC_HOST = getHostname()
DEF_PLC_IP = socket.gethostbyname(DEF_PLC_HOST)


#
# Kick it all off.
#

# -2.
#try:
#    os.makedirs('/plc/emulab/setup')
#except:
#    pass
try:
    os.makedirs('/plc/emulab/nodes')
except:
    pass

# -1.
print "plabinelab: gathering info from tmcd"
tmccAccounts = runTMCC('accounts')
tmccCreator = runTMCC('creator')

(creatorUID,creatorEmail) = (None,None)
for acct in tmccAccounts:
    if 'ADDUSER' in acct['onlykey'] \
       and acct['LOGIN'] == tmccCreator[0]['CREATOR']:
        # found creator
        (creatorUID,creatorEmail) = (acct['LOGIN'],acct['EMAIL'])
        break
    pass
if creatorUID == None or creatorEmail == None:
    print "ERROR: could not find experiment creator's user info!"
    sys.exit(-1)
    pass


# 0.
(rootUID,rootPasswd) = (None,None)
try:
    ret = readRootAcctInfo()
    if ret != None and len(ret) == 2:
        (rootUID,rootPasswd) = ret
        print "plabinelab: read root account info"
        pass
except:
    print "ERROR: could not read root account info!"
    traceback.print_exc()
    sys.exit(-1)
    pass

if rootUID == None or rootPasswd == None:
    try:
        (rootUID,rootPasswd) = writeRootAcctInfo()
        print "plabinelab: wrote new root account info"
    except:
        print "ERROR: could not write root account info!"
        traceback.print_exc()
        sys.exit(-1)
        pass
    pass

# 1.
# XXX: can eventually store these in some other xml file, then merge them in
# with the main plc config file.  Better to do it this way since then
# plc-config-tty will still work...

# grab the PLC's name while we're at it...
PLC_NAME = plcReadConfigVar('plc','name')

plab_ip = ''
plab_host = ''

# Now, we ALWAYS must put the plc_www variables on the control net so that
# users can get to it without a proxy.  However, we want to move boot,api,db
# servers to the experimental net if that's where the plc is supposed to be.
for lineDict in tmccEPlabConfig:
    # look for lines that have PLCNETWORK=X and ROLE=plc
    if lineDict.has_key('ROLE') and lineDict['ROLE'] == 'plc' \
       and lineDict.has_key('PLCNETWORK'):
        plab_ip = lineDict['IP']
        #plab_host = socket.gethostbyaddr(plab_ip)[0]
        plab_host = lineDict['VNAME']
        break
    pass

if plab_ip == '':
    plab_ip = DEF_PLC_IP
    plab_host = DEF_PLC_HOST
    pass

isControlNet = True
if not plab_ip == DEF_PLC_IP:
    isControlNet = False
    pass

print "plabinelab: updating config for PLC '%s'" % str(PLC_NAME)
configVarsList = [ [ 'plc_www','ip',              plab_ip ],
                   [ 'plc_www','host',            plab_ip ],
                   [ 'plc','root_user',           rootUID ],
                   [ 'plc','root_password',       rootPasswd ],
                   [ 'plc_boot','ip',             plab_ip ],
                   [ 'plc_boot','host',           plab_ip ],
                   [ 'plc_mail','support_address',creatorEmail ],
                   [ 'plc_mail','boot_address',   creatorEmail ],
                   [ 'plc_mail','slice_address',  creatorEmail ],
                   [ 'plc_api','ip',              plab_ip ],
                   [ 'plc_api','host',            plab_ip ],
                   [ 'plc_db','ip',               plab_ip ],
                   [ 'plc_db','host',             plab_ip ] ]
plcUpdateConfig(configVarsList)

# XXX: can't find a good way to grab this, but it's unlikely that it will
# change during emulab exp runtime.
PLC_BOOTCD_VERSION = '3.3'

# Ensure that Emulab files are overlayed into the PlanetLab-Bootstrap tarball.
PLC_BOOTSTRAP_TARBALL = '/plc/data/var/www/html/boot/PlanetLab-Bootstrap.tar.bz2'
EMULAB_ROOTBALL = '/share/plabplc/files/tbroot.tar.bz2'
# Make sure we use the right tarball.  Unfortunately, the plab nodes don't like
# the normal emulab linux bootscripts if contacting plc on the experimental
# net.
EMULAB_ROOTBALL_ROLE = 'CONTROL'
if not isControlNet:
    EMULAB_ROOTBALL_ROLE = 'EXP'
    pass

print "Emulabifying PlanetLab-Bootstrap tarball..."
cwd = os.getcwd()
if not os.path.exists('/tmp/proot'):
    os.makedirs('/tmp/proot')
    pass
os.chdir('/tmp')
os.system("sudo tar -xjpf %s -C /tmp/proot" % PLC_BOOTSTRAP_TARBALL)
os.system("sudo tar -xjpf %s-%s -C /tmp/proot" % (EMULAB_ROOTBALL,
                                                  EMULAB_ROOTBALL_ROLE))
os.chdir('/tmp/proot')
os.system("sudo tar -cjpf %s ." % PLC_BOOTSTRAP_TARBALL)
os.chdir(cwd)
os.system("sudo rm -rf /tmp/proot")
print "Finished Emulabification."

# 2.
print "plabinelab: restarting plc"
doService('plc','start')

# update PLC's inner /etc/hosts with our canonical outer one, lest the
# plab nodes not resolve when we try to add them.
print "plabinelab: modifying Emulab hosts file"
hfd = open('/etc/hosts','r')
outlines = list()
line = hfd.readline()
lrm = re.compile('[\s\t]+')
while line != None and line != '':
    line = line.rstrip('\n')
    if line == '' \
           or line.startswith('#') or line.__contains__('127.0.0.1') \
           or line.__contains__('localhost') or line.__contains__('.plab'):
        outlines.append(line)
        pass
    else:
        # probably a valid line.
        # note that we postfix our domain to any entry without a domain.
        lsp = lrm.split(line)
        nlsp = list()
        for elm in lsp:
            if not elm.__contains__('.'):
                nlsp.append("%s.plab" % elm)
                pass
            pass
        outstr = ''
        for bit in lsp:
            outstr += bit + '\t'
            pass
        for bit in nlsp:
            outstr += bit + '\t'
            pass
        outlines.append(outstr)
        pass
    line = hfd.readline()
    pass
hfd.close()
# writeback
hfd = open('/etc/hosts','w')
for ol in outlines:
    hfd.write(ol + "\n")
    pass
hfd.close()

print "plabinelab: adding Emulab hosts to chroot"
os.system("cat /etc/hosts >> /plc/root/etc/hosts")

# 3.
# first update users:
print "plabinelab: updating user accounts"
userlist = list()
for lineDict in tmccAccounts:
    if 'ADDUSER' in lineDict['onlykey']:
        root = False
        if lineDict['ROOT'] == '1':
            root = True
            pass
        pi = False
        if lineDict['LOGIN'] == creatorUID:
            pi = True
            pass
        # we use the @emulab.net address because it makes it easier for us
        # to later remove users during a swapmod (there are legit plc users
        # in the db that are needed for plc maint).
        userlist.append((lineDict['NAME'],
                         lineDict['LOGIN'] + '@emulab.net',lineDict['PSWD'],
                         readUserKeys(lineDict['LOGIN']),root,pi))
        pass
    pass

plcUpdateUsers(userlist)

pnameTrans = dict()

# now do nodes:
# need to save off the control net interface info, then if there's an
# experimental interface specified for a node to be/contact plc on, overwrite.
print "plabinelab: updating nodes"
nodelist = list()
for lineDict in tmccEPlabConfig:
    if lineDict.has_key('PLCNETWORK') and \
           lineDict.has_key('ROLE') and lineDict['ROLE'] == 'node':
        for i in range(0,len(nodelist)):
            print "checking nodelist entry with pname %s against %s" % \
                  (nodelist[i][0],lineDict['PNAME'])
            if nodelist[i][0] == lineDict['PNAME']:
                print "changing ne IP %s to IP %s" % \
                      (nodelist[i][1],lineDict['IP'])
                nodelist[i] = (lineDict['VNAME'] + '.plab',lineDict['IP'],
                               lineDict['MAC'],False,
                               lineDict['NETMASK'],lineDict['IP'])
                pnameTrans[lineDict['PNAME']] = lineDict['VNAME'] + '.plab'
                break
            pass
        pass
    elif lineDict.has_key('ROLE') and lineDict['ROLE'] == 'node':
        nodelist.append((lineDict['PNAME'],
                         lineDict['CNETIP'],lineDict['CNETMAC'],True,
                         '',''))
        pnameTrans[lineDict['PNAME']] = lineDict['PNAME']
        pass
    pass

print "nodelist = %s" % str(nodelist)

plcUpdateNodes(nodelist)

noRemoteConfig = True

# 4.
# first create the local node config info:
vnameToNID = dict()
nidToMAC = dict()
# XXX: this does depend on tmcd returning the ROLE lines before the private
# iface lines for each vname.
for lineDict in tmccEPlabConfig:
    if lineDict.has_key('ROLE') and lineDict['ROLE'] == 'node' \
           and not lineDict.has_key('PLCNETWORK'):
        nid = plcGetNodeID(pnameTrans[lineDict['PNAME']])
        
        print "plabinelab: generating config files for node id %d" % nid
        
        vnameToNID[lineDict['VNAME']] = nid
        nidToMAC[nid] = lineDict['CNETMAC']
        
        configLines = plcGetNodeConfig(pnameTrans[lineDict['PNAME']])
        macLines = [ addMACDelim(lineDict['CNETMAC'],'-') ]
        
        if not os.path.exists('/plc/emulab/nodes/%d' % nid):
            os.makedirs('/plc/emulab/nodes/%d' % nid)
            pass
        
        cfd = open('/plc/emulab/nodes/%d/conf' % nid,'w')
        for cl in configLines:
            print "configLine(%d) = '%s'" % (nid,cl)
            cfd.write('%s\n' % cl)
            pass
        cfd.close()
        
        mfd = open('/plc/emulab/nodes/%d/mac' % nid,'w')
        for ml in macLines:
            mfd.write('%s\n' % ml)
            pass
        mfd.close()
        
        pass
    elif not lineDict.has_key('ROLE') and lineDict.has_key('VNAME') \
             and vnameToNID.has_key(lineDict['VNAME']):
        nid = vnameToNID[lineDict['VNAME']]

        # This step is "commented" out right now since the
        # PlanetLab-Bootstrap.tar.bz2 is whacked with the emulab client side
        # runscripts... thus, we no longer need it.  But we still leave it as
        # an option so that images that don't whack the bootstrap tarball can
        # use it.
        if not noRemoteConfig:
            pfd = open('/plc/emulab/nodes/%d/ifcfg-eth1' % nid,'w')
            pfd.write("DEVICE=eth1\n")
            pfd.write("BOOTPROTO=none\n")
            pfd.write("IPADDR=%s\n" % lineDict['IP'])
            pfd.write("NETMASK=%s\n" % lineDict['NETMASK'])
            pfd.write("HWADDR=%s\n" % addMACDelim(lineDict['MAC'],':'))
            pfd.write("ONBOOT=yes\n")
            pfd.write("TYPE=Ethernet\n")
            pfd.close()
            pass
        pass
    pass

# tar it up...
for nid in vnameToNID.values():
    print "plabinelab: creating config tarball for node id %d" % nid
    os.system('rm -rf /tmp/ncfg-root')
    os.makedirs('/tmp/ncfg-root/etc')
    os.system('cp /etc/hosts /tmp/ncfg-root/etc/')
    if os.path.isfile('/plc/emulab/nodes/%d/ifcfg-eth1' % nid):
        os.makedirs('/tmp/ncfg-root/etc/sysconfig/network-scripts')
        os.system('cp /plc/emulab/nodes/%d/ifcfg-eth1 /tmp/ncfg-root/etc/sysconfig/network-scripts' % nid)
        pass
    cwd = os.getcwd()
    os.chdir('/tmp/ncfg-root')
    os.system('tar cf /plc/data/var/www/html/download/%d.tar .' % nid)
    os.chdir(cwd)
    os.system('rm -rf /tmp/ncfg-root')
    pass
pass


# setup tftp:
# we have to extract the node kernel that chainboots into the real kernel,
# the various img files, and create an img with config data for each node.
# also end up adjusting the isolinux boot cmdline.

print "plabinelab: extracting info from BootCD"

if not os.path.exists('/mnt/bootcd'):
    os.makedirs('/mnt/bootcd')
    pass
os.system('mount -o loop "/plc/data/var/www/html/download/' \
          '%s-BootCD-%s-serial.iso" /mnt/bootcd' % (PLC_NAME,
                                                    PLC_BOOTCD_VERSION))
os.system('cp /mnt/bootcd/pl_version /tftpboot')
os.system('cp /mnt/bootcd/kernel /tftpboot')
os.system('cp /mnt/bootcd/*.img /tftpboot')

ilfd = open('/mnt/bootcd/isolinux.cfg','r')
ilconfigLines = ilfd.read().split('\n')
ilfd.close()

for nid in vnameToNID.values():
    print "plabinelab: configuring pxelinux for node id %d" % nid
    
    os.system('rm -rf /tmp/real-ncfg-%d' % nid)
    os.makedirs('/tmp/real-ncfg-%d/usr/boot' % nid)
    os.system('cp /plc/emulab/nodes/%d/conf ' \
              '/tmp/real-ncfg-%d/usr/boot/plnode.txt' % (nid,nid))
    os.makedirs('/tmp/real-ncfg-%d/etc' % nid)
    os.system('cp /etc/hosts /tmp/real-ncfg-%d/etc' % nid)
    os.system('cp /etc/host.conf /tmp/real-ncfg-%d/etc' % nid)
    cwd = os.getcwd()
    os.chdir('/tmp/real-ncfg-%d' % nid)
    os.system('find . | cpio -o -c | gzip -9 > /tftpboot/config-%d.img' % nid)
    #os.system('rm -rf /tmp/real-ncfg')
    os.chdir(cwd)

    if not os.path.exists('/tftpboot/pxelinux.cfg'):
        os.makedirs('/tftpboot/pxelinux.cfg')
        pass

    # notice that we still use the control net mac, even if setting up PLC
    # on the experimental LAN, since pxeboot is always from control.
    pfd = open('/tftpboot/pxelinux.cfg/01-%s' % addMACDelim(nidToMAC[nid],'-'),'w')
    for iline in ilconfigLines:
        rline = ''
        if iline.count('initrd') > 0:
            sline = iline.split('img')
            for sl in sline[:-1]:
                rline += sl + 'img'
                pass
            rline += ',config-%d.img' % nid
            rline += sline[-1]
            pass
        else:
            rline = iline
            pass
        pfd.write('%s\n' % rline)
        pass
    pfd.close()
    pass

os.system('umount /mnt/bootcd')

# 5.
os.system('chkconfig plc off')

# Finis.
print "plabinelab: done!"

sys.stdin.close()
sys.stdout.close()
sys.stderr.close()

sys.exit(0)
