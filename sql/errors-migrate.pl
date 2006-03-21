#!/usr/bin/perl -w

use strict;

use DBI;

my $database = 'tbdb';
my $username = 'root';
my $passwd = '';

my $dbh = DBI->connect("DBI:mysql:database=$database", $username, $passwd);

my $sth = $dbh->prepare("select * from scripts");
$sth->execute;
my %script_map;
my @script_map;
while ((my $ref = $sth->fetchrow_arrayref)) {
   $script_map{$ref->[1]} = $ref->[0];
   $script_map[$ref->[0]] = $ref->[1];
}
$script_map{unknown} = 0;
$script_map[0] = 'unknown';

$sth = $dbh->prepare("select stamp,session,exptidx,mesg from log where type = 'thecause' order by session");
$sth->execute;

my $sth2 = $dbh->prepare("select script_name,cause,type from log natural join scripts where session = ? and relevant order by seq desc");

my $ins = $dbh->prepare("insert into errors (session,stamp,exptidx,script,cause,confidence,mesg) values (?,?,?,?,?,?,?)");

my $ready = 0;

while ((my $ref = $sth->fetchrow_arrayref)) {
  my ($stamp,$session,$exptidx,$mesg) = @$ref;
  my $script = "unknown";
  my $cause = "unknown";
  my $confidence = 0.0;
  local $_ = $mesg;
  if (/^\d+: ([^:]+)/) {
    $script = $1;
  } else {
    if (s/\nCause: (\S+)\nConfidence: (\S+)$//) {
      $cause = $1;
      $confidence = $2;
    }
    s/^  //gm;
    $mesg = $_;
    ($_) = split /\n\s*\n/;
    if (/\(([^\)]+)\)$/ && defined $script_map{$1}) {
      $script = $1;
    } else {
      warn "Unable to find script in: $_\n" unless /No clue as to what went wrong/;
    }
    $sth2->execute($session);
    my @rel;
    while ((my $r = $sth2->fetchrow_hashref)) {
      push @rel, {%$r};
    }

    if (@rel) {

      warn "Script name mismatch: $rel[0]{script_name} ne $script in: $_\n" 
	if $rel[0]{script_name} ne $script;

      $script = $rel[0]{script_name};

      my $conf = 0.5;
      if ($script =~ /^(assign|parse)/) {
	$conf = 0.9;
      } elsif ($script =~ /^(os_setup)/) {
	$conf = 0.2;
      }

      my $t = '';
      my $c = '';
      foreach (@rel) {
	$ready = 1 if $_->{cause} && $_->{cause} ne 'unknown';
	if (!$c && $_->{cause}) {
	  $c = $_->{cause};
	  $t = $_->{type};
	} elsif ($_->{cause} &&
		 $_->{type} eq $t && $_->{cause} ne $c) {
	  $c = 'unknown';
	}
      }
      $c = 'unknown' unless $c;

      if ($cause eq 'unknown' && $ready) {
	$cause = $c;
	$confidence = $conf;
      } elsif ($cause ne 'unknown') {
	warn "Cause mismatch" unless $cause eq $c;
	warn "Confidence mismatched" unless $conf == $confidence;
      }
    }
  }

  $ins->execute($session,$stamp,$exptidx,$script_map{$script},$cause,$confidence,$mesg);
}


