Timeline:

Absolute Essential and #1 Priority Tasks:
* Setup accounts
	-Current Status: We can currently create local users according
	to information received from tmcc.  The main outstanding
	issues is that our system is not intragrated into the normal
	perl/emulab way of doing things and the password is the
	default password.
  Current Task:
	-I am working on getting the testbed boot code to create users
	using the Microsoft addusers.exe program.  Right now I am
	going to have a dependency on cygwin, hopfully that can go
	away in the future.
  Questions:
	-How do I figure out how the testbed code starts up?
	-How do we store a password as a user accounts password that
	has already went through the NThash algorithm?
  Notes:
	-FreeBSD supports storing the NThash password hash, the code to
	 do this is in src/lib/libcrypt/crypt-nthash.c
	-Kirk will work on getting the password to sync between Unix
	and Windows
	-Ideas Windows Services for Unix 3.0 is something that could
	be looked into.  The problem that we have with this software
	is the documentation is just really poor
	-Okay users now autocreate when Windows first boots!!!  The
	following is the current command I use inside of cygwin to
	make "it" happen.
	    - $ cygrunsrv -I startupService -p /cygdrive/c/Python22/python.exe -c /cygdrive/c
	        /thesis -a "c:\thesis\createUsers.py"
	-I am now working to get this to happen from the Perl Script.
	So right now becuase we are not mounting directories like we
	need to I created a local directory /proj, this should be
	removed later.
		-The script to start with is testbed/tmcd/common/rc.setup
		-I am using /etc/testbed as the location to "put things"
	-The perl script will now setup accounts and get things up and running
	-We really need to get some kind of script to install the
	emulab infestructore onto a Windows Node, or at least I need
	to come up with a more systimatic way of setting this up from
	a clean install of Windows.
  Tasks Complete:
	-I helped port TMCC to Windows and saw that the code was checked in
* Get the computer to change it's SID and hostname on bootup (Completion Date: 26 June)
        -The SID is basically a private key generated for a machine
        when Windows is installed.  We will probably use
        (http://www.sysinternals.com/ntw2k/source/newsid.shtml) to
        change the SID.
	       -For the time being the SID will be created randomly if
	       the file "i_have_changed_the_sid" does not exist
	       -A weblink for info on SIDs
	       http://www.win2000mag.com/Articles/Print.cfm?ArticleID=3469
	-Changing the hostname should be pretty straightforward, but I
	have not yet tried to do this from a command line, and I am
	unclear on exactly what the Windows "hostname" should be.
        -I should also mention that without really ugly (and dangerous)
        hacking setting this info this will require a
        reboot. Personally I just don't think there is a safe way
        around this issue at least with the current version of
        Windows.  Rebooting should add 1-2 min to the experiment
        configure time.  So if people have ideas then please let me
        know.
* Get the home directories to mount (Completion Date: 30 June)
        -Our plan here is to use Windows Services for Unix and just
        have Windows mount the NFS shares.
	-There are various other ways that this could be done.  We
	could setup SAMBA on users or setup an active directory
	Windows Server and there is probably a couple of other things
	that would work that I am not thinking of now.
* Interface support (5 July)
	-Interface's need to be setup "correctly"
* Write Report (15 July)

-----------------------------------------------------------------------------

Things That Must Be Done, but After the "Absolute Essential tasks are done:
* idle daemon
* automatically software install (sysinstall or something else), maybe also do auto .tar.gz files
* startup commands

Things That Would Be Nice To Have:
* event system
* ipod 
* traffic generation
* program objects
* node virtualization/link virtualization
* console login
* program objects 

Things Kirk will look into:
* windows client support for elvin
* idle detection
* NFS -russ driving
* password syncing between Windows and Unix

Other notes:
We want to use cygwin as much as possible so we can keep the same source code base.

How are people going to want to use this?  How much will they use specific features of Windows like "windows networking"?

Security of windows is going to be a concern among our users.

Are users going to want to use the Windows Clients to test performance?

Are users going to want Windows generating automated web traffic?


TMCC Notes:

[These are the TMCC commands that need to be used on windows, if the
TMCC command is not listed here it will not be supported.  There is
also some notes about the command and what it will do in a windows
Environment, for the most part I will just use the already existing
Perl code to do this stuff.  But I will not worry about merging my
changes back in someone else will take my work and make sure it is
correctly merged back into the system.]

ifconfig - match MAC address of the card with the interface in Windows

routing - add routes

state - "I am up"

nodeid - Returned result goes into a file.

hostnames 

tarballs

startupcmd - path of stuff to execute

startstatus -whatever execs start command report w/start

mounts

reset - look to see if this is used anymore

routing - do

creator - This is something that should happen, but it might prove to
be quite difficult to change who you are in windows.  Kirk will look
into this.

state - report state

isalive - do this

Time Synronozation
     -Windows has the option to sync with an ntp server and should be
     configured to sync with users.emulab.net or boss.  As a long term
     goal it might be worth looking to supporting ntpinfo, and
     ntpdrift.


-------
Notes to self:
/usr/testbed/bin/tip.old -9600 pc35_tty

state - report state

isalive - do this

ntp info - long term -services

ntp drift - long term unix file

runas pg 649

sharutils

changing SID

TMCC - creates users
Mount home directories
      User directories u: drive
      project directories p: drive

creates a service
set the host name
change SSIDE
setup accounts
get home directories to mount
merge TMCC

services for Unix

made up password
proper groups

ifconfig - match MAC address with interface
routing - add routes
turn on/off ip forwarding
goal 1 reboot
state - I am up
account
nodeid - goes in file
hostnames[V2] - wahtever one the scripts do

you want to use libsetup.pm - maybe this is an end goal and then merge

libloc

look @ rpms

startupcmd - path of stuff to execute

startstatus - whatever execs startup command reports w/startcmd

mounts

reset - look to see if this is used anymore

routing -do

creator - look into something that should happen 

$group $id $username @name

