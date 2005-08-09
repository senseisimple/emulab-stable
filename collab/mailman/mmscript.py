from Mailman import MailList
from Mailman import Utils
from Mailman import Message
from Mailman import Errors
from Mailman import UserDesc

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

def modmember(mlist, addr, name, passwd):
    try:
        mlist.setMemberPassword(addr, passwd)
        mlist.setMemberName(addr, name)
        mlist.Save()
    except Errors.NotAMemberError:
        print 'Not a member:', addr
        sys.exit(1);
    except:
        print 'Error resetting name/password'
        sys.exit(1);
        pass
    pass

def findmember(mlist, addr):
    if mlist.isMember(addr):
        print mlist.internal_name();
        pass
    pass
