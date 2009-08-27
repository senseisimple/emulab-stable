#!/usr/bin/perl
use Modern::Perl;
use File::Temp;
use Data::Dumper;
use IPC::Run3;
my $fn = "BOZO";

my @todos;
while(my $line = <STDIN>) {
  given($line) {
    when(/#   Failed test/) {}
    when(/#   at /) {}
    when(/^# Coverage for (\S+)/) {
      $fn = $1;
      $fn =~ s/::/\//g;
      $fn .= '.pm';
    }
    when( /Looks like you failed/) {
     next;
    }
    when( /^#\s+(\S+)/ ) {
      say "pushed $fn $1";
      push @todos, [$fn, $1];
    } 
  }
}

for (@todos) {
  my $temp = File::Temp->new();
  my $sfn = $temp->filename;
  my $fn = $_->[0];
  my $subname = $_->[1];
  $temp->print("/$subname\n");
  my $cmd = "vim lib/$fn -s $sfn";
  run3($cmd);
}

#say Dumper(\@todos);
exec 'reset';
