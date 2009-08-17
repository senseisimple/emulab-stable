#!/usr/bin/perl
use Modern::Perl;
use File::Temp;
use Data::Dumper;
use IPC::Run3;
my $fn;

my @todos;
while(my $line = <STDIN>) {
  given($line) {
    when(/^not ok \d+ - Pod coverage on (\S+)/) {
      $fn = $1;
      $fn =~ s/::/\//g;
      $fn .= '.pm';
      my $a = <STDIN> for (1..4);
    }
    when( /Looks like you failed/) {
     next;
    }
    when( /^#\s+(\S+)/ ) {
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
  run3("vim lib/$fn -s $sfn");
}

#say Dumper(\@todos);
exec 'reset';
