#!/usr/bin/perl -w
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002 University of Utah and the Flux Group.
# All rights reserved.
#
use Getopt::Std;
use English;
use HTTP::Daemon;
use HTTP::Status;
use URI::http;

#
# Trivial web server that redirects everthing back to emulab.
#
sub usage()
{
    die("Usage: webserver.pl\n");
}
my $logname = "/var/emulab/logs/webserver.log";
my $pidfile = "/var/run/webserver.pid";
my $webpage = "http://www.emulab.net/widearea_redirect.php";
my $IP;

#
# Untaint path
#
$ENV{'PATH'} = '/bin:/sbin:/usr/bin:/usr/local/bin';
delete @ENV{'IFS', 'CDPATH', 'ENV', 'BASH_ENV'};

#
# Turn off line buffering on output
#
$| = 1;

my $webserver = HTTP::Daemon->new(LocalPort => 'http(80)',
				  Reuse     => 1)
    || die("*** $0:\n".
	   "    Could not create a new daemon object!\n");

#
# Put ourselves into the background, directing output to the log file.
#
my $mypid = fork();
if ($mypid) {
    exit(0);
}

#
# Write our pid into the pid file so we can be killed.
#
open(PFILE, "> $pidfile")
    or die("Could not open $pidfile: $!");
print PFILE "$PID\n";
close(PFILE);

#
# We have to disconnect from the caller by redirecting STDIN and STDOUT.
#
open(STDIN,  "< /dev/null") or die("opening /dev/null for STDIN: $!");
open(STDERR, ">  $logname") or die("opening $logname for STDERR: $!");
open(STDOUT, ">> $logname") or die("opening $logname for STDOUT: $!");

#
# We would like our IP address to pass along.
# 
my $hostname = `hostname`;
if (defined($hostname)) {
    # Untaint and strip newline.
    if ($hostname =~ /^([-\w\.]+)$/) {
	$hostname = $1;

	my (undef,undef,undef,undef,@ipaddrs) = gethostbyname($hostname);
	$IP = inet_ntoa($ipaddrs[0]);
	$webpage .= "?IP=$IP";
    }
}

#
# Flip to user nobody after binding to port 80 (a reserved port).
# 
my (undef,undef,$unix_uid) = getpwnam("nobody") or
    die("No such user nobody\n");
my (undef,undef,$unix_gid) = getgrnam("nobody") or
    die("No such group nobody\n");

$EGID = $GID = $unix_gid;
$EUID = $UID = $unix_uid;

#
# Loop, always returning a redirect no matter what the request.
# 
while (my $c = $webserver->accept) {
    while (my $r = $c->get_request) {
	#
	# We do not actually care what the request is. Just send
	# back a redirect. 
	#
	$c->send_redirect($webpage);
    }
    $c->close;
    undef($c);
}

exit 0;
