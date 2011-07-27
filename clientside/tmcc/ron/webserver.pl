#!/usr/bin/perl -w
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003 University of Utah and the Flux Group.
# All rights reserved.
#
use Getopt::Std;
use English;
use HTTP::Daemon;
use HTTP::Status;
use URI::http;
use Data::Dumper;

#
# Trivial web server that redirects everthing back to emulab.
#
sub usage()
{
    die("Usage: webserver.pl <start | stop> [config_file]\n");
}
my $logname = "/var/emulab/logs/webserver.log";
my $pidfile = "/var/run/emulab-webserver.pid";
my $webpage = "http://www.emulab.net/widearea_redirect.php";
my $IP;
my $dpid;
my $conf_file = "/usr/local/etc/emulab/webserver.conf";

my %redirect_ports = ();

#
# Untaint path
#
$ENV{'PATH'} = '/bin:/sbin:/usr/bin:/usr/local/bin';
delete @ENV{'IFS', 'CDPATH', 'ENV', 'BASH_ENV'};

#
# Turn off line buffering on output
#
$| = 1;

# Parse command line.
if (@ARGV > 2 or @ARGV < 1) {
    usage();
}
my $action = $ARGV[0];
if ($action ne "start" && $action ne "stop") {
    usage();
}

# read the config file if one was given
if (defined $ARGV[1]) {
    my $conf_file = $ARGV[1];
    if (!-f $conf_file) {
	die( "Config file $conf_file doesn't exist" );
    }
}
if (-f $conf_file) {
    open (CONF, "<$conf_file") or die ("Can't read conf file $conf_file");
    my @c = <CONF>;
    close (CONF);

    foreach my $line (@c) {
	if( $line =~ /^\#/ ) {
	    next;
	} elsif( $line =~ /^logname (\S+)$/ ) {
	    $logname = $1;
	} elsif( $line =~ /^pidfile (\S+)$/ ) {
	    $pidfile = $1;
	} elsif( $line =~ /^webpage (\S+)$/ ) {
	    $webpage = $1;
	} elsif( $line =~ /^domainport (\S+) (\d+)$/ ) {
	    $redirect_ports{$1} = $2;
	}
    }
}

#
# For stop, look to see if the pid file exists. If so, kill it and exit.
#
if ($action eq "stop") {
    if (! -e $pidfile) {
	exit(0);
    }
    system("kill `cat $pidfile`");
    exit($? >> 8);
}

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
system("echo '$PID' > $pidfile") == 0
    or die("Could not create $pidfile!");

#
# We have to disconnect from the caller by redirecting STDIN and STDOUT.
#
open(STDIN,  "< /dev/null") or die("opening /dev/null for STDIN: $!");
open(STDERR, ">  $logname") or die("opening $logname for STDERR: $!");
open(STDOUT, ">> $logname") or die("opening $logname for STDOUT: $!");

#
# Setup a handler to catch TERM.
#
sub handler () {
    $SIG{TERM} = 'IGNORE';
    $SIG{INT} = 'IGNORE';
    unlink($pidfile);
    kill('TERM', $dpid)
	if (defined($dpid));
    exit(0);
}
$SIG{TERM} = \&handler;
$SIG{INT}  = \&handler;

#
# Get uid/gid for "nobody" since port 80 is a reserved port, and we have
# to get the port, and then flip later. 
# 
my (undef,undef,$unix_uid) = getpwnam("nobody") or
    die("No such user nobody\n");
my (undef,undef,$unix_gid) = getgrnam("nobody") or
    die("No such group nobody\n");


#
# We would like our IP address to pass along.
# 
my $hostname = `hostname`;
if (defined($hostname)) {
    # Untaint and strip newline.
    if ($hostname =~ /^([-\w\.]+)$/) {
	$hostname = $1;

	my (undef,undef,undef,undef,@ipaddrs) = gethostbyname($hostname);
        if (@ipaddrs) {
	    $IP = inet_ntoa($ipaddrs[0]);
	    $webpage .= "?IP=$IP";
        }
    }
}

#
# Loop. Create a child to run the web server, and wait for it. If it dies,
# restart it. 
#
while (1) {
    $dpid = fork();
    # Child runs the server by continuing out of the loop.
    if (! $dpid) {
	print "Starting child webserver. PID = $PID\n";
	$SIG{TERM} = 'DEFAULT';
	last;
    }
    # Parent waits, and loops. Only a signal can stop us.
    waitpid($dpid, 0);
    print "Child webserver exited with $?. Restarting in a moment ...\n";
    sleep(2);
}

#
# Only the child executes from here on. 
# 
my $webserver = HTTP::Daemon->new(LocalPort => 'http(80)',
				  Reuse     => 1)
    || die("*** $0:\n".
	   "    Could not create a new daemon object!\n");

$EGID = $GID = $unix_gid;
$EUID = $UID = $unix_uid;

#
# Loop, always returning a redirect no matter what the request.
# 
while (my $c = $webserver->accept) {
    while (my $r = $c->get_request) {

	my $response_sent = 0;
	
	foreach my $domain (keys(%redirect_ports)) {

	    my $host = $r->header( "host" );

	    if ($host =~ /$domain$/ ) {
		$c->send_redirect ("http://" . $host . ":" . 
				   $redirect_ports{$domain}, 302);

		$response_sent = 1;
		last;
	    }

	}

	#
	# We do not actually care what the request 
	# is. Just send back a redirect. 
	#
	if (!$response_sent) {
	    $c->send_redirect($webpage);
	}

    }
    $c->close;
    undef($c);
}

exit 0;
