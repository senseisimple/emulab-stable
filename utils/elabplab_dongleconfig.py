#!/usr/bin/python

import sys
import traceback
import getopt
import os
import os.path
import pwd
import optparse
import xmlrpclib

#
# Depending on where you've installed m2crypto, edit/remove the following
# path addition...
#
#sys.path.append("/home/johnsond/r/usr/local/lib/python2.5/site-packages")
from M2Crypto.m2xmlrpclib import SSL_Transport
from M2Crypto import SSL

#
# Depending on where you've installed the emulab xmlrpc client
# support libs, tweak the following path append.
#
#sys.path.append("/usr/testbed/lib")
sys.path.append("/home/johnsond/elab")
import emulabclient

debug = 1

#
# The package version number
#
PACKAGE_VERSION = 0.1

BOSSNODE = "boss.emulab.net"

XMLRPC_SERVER = BOSSNODE
XMLRPC_PORT = 3069
SERVER_PATH = "/usr/testbed/devel/johnsond"
SERVER_DIR = "sbin"
ELAB_URI = "https://" + XMLRPC_SERVER + ":" + str(XMLRPC_PORT) + SERVER_PATH

PLAB_URI = "https://plc.protogeni.net/PLCAPI/"
PLAB_USER = None
PLAB_PASSWD = None

#
# check args
#
opp = optparse.OptionParser()
opp.add_option("-d", "--debug", dest="debug", default=False,
               action="store_true", help="Say A LOT about internal stuff")
opp.add_option("-e", "--emulab", dest="onlyelab", default=False,
               action="store_true", help="Only configure the Emulab slice")
opp.add_option("-p", "--planetlab", dest="onlyplab", default=False,
               action="store_true", help="Only configure the PlanetLab slice")
opp.add_option("-s", "--eslice", dest="eslice", default=None,
               action="store", help="Slice containing Emulab stuff")
opp.add_option("-S", "--pslice", dest="pslice", default=None,
               action="store", help="Slice containing PlanetLab stuff")
opp.add_option("-D", "--device", dest="device", default=None,
               action="store",
               help="Block device containing Emulab and PlanetLab slices.")
opp.add_option("-i", "--idempotent", dest="nowrite", default=False,
               action="store_true", help="Do not modify slices")
opp.add_option("-m", "--emnt", dest="emnt", default=None,
               action="store", help="Mount point of/for Emulab slice")
opp.add_option("-M", "--pmnt", dest="pmnt", default=None,
               action="store", help="Mount point of/for PlanetLab slice")
opp.add_option("-u", "--uid", dest="user", default=None,
               action="store", help="Use Emulab certs for this username")

opts, sys.argv = opp.parse_args(sys.argv)

if len(sys.argv) < 2:
    opp.error("must supply at least a node_id!")
else:
    node_id = sys.argv[1]
    pass

#
# Get currently mounted devices and their mount points.
#
def getMounts():
    retval = dict({})
    fd = os.popen('/sbin/mount')
    dmd = dict({})
    mdd = dict({})
    line = fd.readline()
    while line:
        ls = line.split(' on ')
        if len(ls) > 1:
            mp = ls[1].split(' ')[0]
            dmd[ls[0]] = mp
            mdd[mp] = ls[0]
            pass
        line = fd.readline()
        pass
    fd.close()
    return (dmd,mdd)

(onlyelab,onlyplab) = (opp.values.onlyelab,opp.values.onlyplab)
nowrite = opp.values.nowrite
emount = '/tmp/epwa.emnt'
pmount = '/tmp/epwa.pmnt'
username = opp.values.user
if opp.values.emnt:
    emount = opp.values.emnt
if opp.values.pmnt:
    pmount = opp.values.pmnt
eslice = None
pslice = None
if opp.values.eslice:
    eslice = opp.values.eslice
if opp.values.pslice:
    pslice = opp.values.pslice
device = None
if opp.values.device:
    device = opp.values.device

if onlyelab and onlyplab:
    opp.error("can only set one of '-e' and '-p'")

# Make sure we can grab a uid/cert to talk to Emulab
if username:
    try:
        pw = pwd.getpwnam(username)
    except KeyError:
        sys.stderr.write("error: unknown user id %d" % username)
        sys.exit(2)
        pass
    pass
else:
    try:
        pw = pwd.getpwuid(os.getuid())
    except KeyError:
        sys.stderr.write("error: unknown user id %d" % os.getuid())
        sys.exit(2)
        pass
    pass

ELAB_USER = pw.pw_name
ELAB_HOME = pw.pw_dir
ELAB_CERT = os.path.join(ELAB_HOME, ".ssl", "emulab.pem")

if not os.path.exists(ELAB_CERT) and not onlyplab:
    sys.stderr.write("error: missing emulab certificate: %s\n" %
                     ELAB_CERT)
    sys.exit(2)
    pass

(devmap,mntmap) = getMounts()

if debug:
    print "devmap: '%s'" % str(devmap)
    print "mntmap: '%s'" % str(mntmap)
    pass

def umount():
    (ndevmap,nmntmap) = getMounts()
    if nmntmap.has_key(emount) and not mntmap.has_key(emount):
        os.system('/sbin/umount %s' % emount)
    if nmntmap.has_key(pmount) and not mntmap.has_key(pmount):
        os.system('/sbin/umount %s' % pmount)
    pass

# safety hatch
mountopts = ''
if nowrite:
    mountopts = '-r'
    pass

# figure out what our partition names should look like
(elab_part,plab_part) = (None,None)
(ufs_opts,vfat_opts) = ('','')
if sys.platform.startswith('linux'):
    (elab_part,plab_part) = ('1','2')
    (ufs_opts,vfat_opts) = ('-t ufs -o ufstype=ufs2','-t vfat')
elif sys.platform.startswith('freebsd'):
    (elab_part,plab_part) = ('s1a','s2')
    (ufs_opts,vfat_opts) = ('','-t msdosfs')
else:
    print "Unsupported platform %s, exiting!" % sys.platform
    sys.exit(99)
    pass

# Figure out what/where to mount the emulab device
if not onlyplab:
    if mntmap.has_key(emount):
        if eslice:
            raise RuntimeError("Device already mounted at %s; cannot mount" \
                               " %s there!" % (emount,eslice))
        if device:
            print "Warning: using mount %s instead of device %s" \
                  % (emount,device)
            pass
        pass
    elif os.path.isdir(emount) or not os.path.exists(emount):
        if not os.path.isdir(emount):
            os.makedirs(emount)
            pass
        # figure out if we need to mount using the slice or the device:
        if eslice:
            if os.path.exists(eslice):
                retval = os.system('/sbin/mount %s %s %s %s' \
                                   % (mountopts,ufs_opts,eslice,emount))
                if not retval == 0:
                    raise RuntimeError("mount(%s,%s) failed: %d" \
                                       % (eslice,emount,retval >> 8))
                else:
                    raise RuntimeError("Slice %s does not exist!" % eslice)
                pass
            pass
        elif device:
            if not device.find('/dev') == 0:
                rdev = "%s%s%s" % ('/dev/',device,elab_part)
            else:
                rdev = "%s%s" % (device,elab_part)
                pass
            if devmap.has_key(rdev):
                raise RuntimeError("Device %s already mounted!" % rdev)
            if os.path.exists(rdev):
                retval = os.system('/sbin/mount %s %s %s %s' \
                                   % (mountopts,ufs_opts,rdev,emount))
                if not retval == 0:
                    raise RuntimeError("mount(%s,%s) failed: %d" \
                                       % (rdev,emount,retval >> 8))
                pass
            else:
                raise RuntimeError("Device %s does not exist!" % rdev)
            pass
        pass
    else:
        raise RuntimeError("Invalid mount point %s!" % emount)
    pass

# Figure out what/where to mount the planetlab device
if not onlyelab:
    if mntmap.has_key(pmount):
        if pslice:
            umount()
            raise RuntimeError("Device already mounted at %s; cannot mount" \
                               " %s there!" % (pmount,pslice))
        if device:
            print "Warning: using mount %s instead of device %s" \
                  % (pmount,device)
            pass
        pass
    elif os.path.isdir(pmount) or not os.path.exists(pmount):
        if not os.path.isdir(pmount):
            os.makedirs(pmount)
            pass
        # figure out if we need to mount using the slice or the device:
        if pslice:
            if os.path.exists(pslice):
                retval = os.system('/sbin/mount %s %s %s %s' \
                                   % (mountopts,vfat_opts,pslice,pmount))
                if not retval == 0:
                    umount()
                    raise RuntimeError("mount(%s,%s) failed: %d" \
                                       % (pslice,pmount,retval >> 8))
                pass
            else:
                umount()
                raise RuntimeError("Slice %s does not exist!" % pslice)
            pass
        elif device:
            if not device.find('/dev') == 0:
                rdev = "%s%s%s" % ('/dev/',device,plab_part)
            else:
                rdev = "%s%s" % (device,plab_part)
                pass
            if devmap.has_key(rdev):
                umount()
                raise RuntimeError("Device %s already mounted!" % rdev)
            if os.path.exists(rdev):
                retval = os.system('/sbin/mount %s %s %s %s' \
                                   % (mountopts,vfat_opts,rdev,pmount))
                if not retval == 0:
                    umount()
                    raise RuntimeError("mount(%s,%s) failed: %d" \
                                       % (rdev,pmount,retval >> 8))
                pass
            else:
                umount()
                raise RuntimeError("Device %s does not exist!" % rdev)
            pass
        pass
    else:
        umount()
        raise RuntimeError("Invalid mount point %s!" % pmount)
    pass

def addMACDelim(mac,delim=':'):
    if mac.count(delim) == 0:
        return mac[0:2] + delim + mac[2:4] + delim + mac[4:6] + delim + \
               mac[6:8] + delim + mac[8:10] + delim + mac[10:12]
    else:
        return mac
    pass

#
# Grab the emulab widearea config for this node.
#
if not onlyplab:
    ctx = SSL.Context("sslv23")
    ctx.load_cert(ELAB_CERT, ELAB_CERT)
    ctx.set_verify(SSL.verify_none, 16)
    ctx.set_allow_unknown_ca(0)
    try:
        eserver = xmlrpclib.ServerProxy(ELAB_URI,SSL_Transport(ctx),
                                        verbose=debug,allow_none=1)
        waconfig = eserver.node.waconfig(PACKAGE_VERSION,{ 'node' : node_id })
        waconfig = waconfig['value']
        if debug:
            print "waconfig: '%s'" % str(waconfig)
            pass
        possible_keys = [ 'WA_BOOTMETHOD','WA_HOSTNAME','WA_DOMAIN','WA_MAC',
                          'WA_IP_ADDR','WA_IP_NETMASK','WA_IP_GATEWAY',
                          'WA_IP_DNS1','WA_IP_DNS2','WA_BWLIMIT' ]
        if not waconfig:
            print "error: received empty config from Emulab!"
            sys.exit(3)
            pass

        # whack the mac value into colon-delimited form
        if waconfig.has_key("WA_MAC"):
            waconfig["WA_MAC"] = addMACDelim(waconfig["WA_MAC"])
            pass

        elabstr = ''
        for k in possible_keys:
            if waconfig.has_key(k):
                elabstr = "%s%s=\"%s\"\n" % (elabstr,k,str(waconfig[k]))
                pass
            pass
        if debug:
            print "Emulab waconfig file contents:\n\n%s" % elabstr
            print "Emulab waconfig privkey: %s" % str(waconfig['PRIVKEY'])
            pass
        pass
    except:
        print "ERROR: contacting Emulab via XMLRPC..."
        traceback.print_exc()
        umount()
        sys.exit(2)
        pass
    pass

#
# Grab the plab config for this node.
#
if not onlyelab:
    if onlyplab:
        # user had better specify a real hostname, not an elab node_id
        hostname = node_id
    else:
        # grab the hostname from the elab db
        hostname = "%s.%s" % (waconfig['WA_HOSTNAME'],waconfig['WA_DOMAIN'])
        pass
    try:
        pserver = xmlrpclib.ServerProxy(PLAB_URI,allow_none=1)
        auth_info = { 'Username' : PLAB_USER, 'AuthMethod' : 'password',
                      'Role' : 'admin', 'AuthString' : PLAB_PASSWD }
        plabstr = pserver.AdmGenerateNodeConfFile(auth_info,hostname)
        if debug:
            print "Plab config file contents:\n\n%s" % plabstr
            pass
        pass
    except:
        print "ERROR: contacting Plab via XMLRPC..."
        traceback.print_exc()
        umount()
        sys.exit(2)
        pass
    pass

# Finally, actually write everything...
if not nowrite:
    if not onlyplab:
        try:
            fd = open('%s/etc/emulab/waconfig' % emount,'w')
            fd.write(elabstr)
            fd.close()
        except:
            print "Error writing Emulab waconfig file!"
            traceback.print_exc()
            umount()
            sys.exit(4)
        try:
            fd = open('%s/etc/emulab/emulab-privkey' % emount,'w')
            fd.write("%s\n" % waconfig['PRIVKEY'])
            fd.close()
        except:
            print "Error writing Emulab privkey file!"
            traceback.print_exc()
            umount()
            sys.exit(4)
        os.system('touch %s/etc/emulab/isrem' % emount)
        os.system('touch %s/etc/emulab/isflash' % emount)
        os.system('touch %s/etc/emulab/ismfs' % emount)
        try:
            fd = open('%s/etc/emulab/bossnode' % emount,'w')
            fd.write("%s\n" % BOSSNODE)
            fd.close()
        except:
            print "Error writing Emulab bossnode file!"
            traceback.print_exc()
            umount()
            sys.exit(4)
        pass
    if not onlyelab:
        try:
            fd = open('%s/plnode.txt' % pmount,'w')
            fd.write(plabstr)
            fd.close()
        except:
            print "Error writing PlanetLab plnode.txt file!"
            traceback.print_exc()
            umount()
            sys.exit(4)
        pass
    pass

umount()

print "Done."
sys.exit(0)
