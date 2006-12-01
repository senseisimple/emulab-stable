#
# EMULAB-COPYRIGHT
# Copyright (c) 2004-2006 University of Utah and the Flux Group.
# All rights reserved.
#
from Mailman import mm_cfg
from Mailman import MailList
from Mailman import Utils
from Mailman import Message
from Mailman import Errors
from Mailman import UserDesc
import sha
import sys

def addmember(mlist, addr, name, passwd):
    userdesc = UserDesc.UserDesc(address=addr, fullname=name, password=passwd)
    try:
        mlist.ApprovedAddMember(userdesc, 0, 0)
        mlist.Save()
    except Errors.MMAlreadyAMember:
        print 'Already a member:', addr
    except Errors.MMBadEmailError:
        print 'Bad address:', addr
        sys.exit(1);
    except Errors.MMHostileAddress:
        print 'Hostile address:', addr
        sys.exit(1);
    except:
        print 'Error adding name'
        sys.exit(1);
        pass
    pass

def modmember(mlist, oldaddr, newaddr, name, passwd):
    try:
        mlist.setMemberPassword(oldaddr, passwd)
        mlist.setMemberName(oldaddr, name)
        if oldaddr <> newaddr:
            mlist.ApprovedChangeMemberAddress(oldaddr, newaddr, False)
            pass
        mlist.Save()
    except Errors.NotAMemberError:
        print 'Not a member:', oldaddr
        sys.exit(1);
    except:
        print 'Error resetting name/password'
        sys.exit(1);
        pass
    pass

def setadmin(mlist, addr, passwd):
    try:
        mlist.owner = [addr]
        mlist.password = sha.new(passwd).hexdigest()
        mlist.Save()
    except:
        print 'Error resetting name/password for list'
        sys.exit(1);
        pass
    pass

def findmember(mlist, addr):
    if mlist.isMember(addr):
        print mlist.internal_name();
        pass
    pass

def getcookie(mlist, addr, cookietype):
    # If we want the admin interface, we do not care if the addr is
    # a member of the list, since the caller (boss) already verified
    # the operation was allowed.
    if cookietype == "admin":
        print mlist.MakeCookie(mm_cfg.AuthListAdmin, addr)
        return
    
    # But for a user cookie, must be a member.
    if mlist.isMember(addr):
        print mlist.MakeCookie(mm_cfg.AuthUser, addr)
        pass
    pass
