package TraceUse;

use Time::HiRes qw( gettimeofday tv_interval );
use Data::Dumper;
use English;

BEGIN
{
    unshift @INC, \&trace_use unless grep { "$_" eq \&trace_use . '' } @INC;
}

sub myrequire()
{
    my($filename) = @_;
    return 1 if $INC{$filename};
    my($realfilename,$result);
    my @TMPINC = [ @INC[1..$#INC] ];

  ITER: {
      foreach $prefix (@TMPINC) {
	  $realfilename = "$prefix/$filename";
	  if (-f $realfilename) {
	      $result = do $realfilename;
	      last ITER;
	  }
      }
      die "Can't find $filename in \@TMPINC";
  }
  die $@ if $@;
  die "$filename did not return true value" unless $result;
  $INC{$filename} = $realfilename;
  return $result;
}

sub trace_use
{
    my ($code, $module) = @_;
    (my $mod_name       = $module) =~ s{/}{::}g;
    $mod_name           =~ s/\\.pm$//;
    my ($package, $filename, $line) = caller( );
    my $elapsed         = 0;

    {
        #local *INC      = [ @INC[1..$#INC] ];
        my ($sec,$usec)  = gettimeofday();
	#print "$mod_name $sec,$usec\n";
        eval "package $package; TraceUse::myrequire('$mod_name');";
        $elapsed        = tv_interval( [$sec,$usec] );
	#print "$mod_name $elapsed\n";
    }
    $package            = $filename if $package eq 'main';
    push @used,
    {
        'file'   => $package,
        'line'   => $line,
        'time'   => $elapsed,
        'module' => $mod_name,
	'inc'    => "@INC",
    };

    return;
}

END
{
    my $first = $used[0];
    my %seen  = ( $first->{file} => 1 );
    my $pos   = 1;

    warn "Modules used from $first->{file}:\n";

    for my $mod (@used)
    {
        my $message = '';

        if (exists $seen{$mod->{file}})
        {
            $pos = $seen{$mod->{file}};
        }
        else
        {
            $seen{$mod->{file}} = ++$pos;
        }

        my $indent = '  ' x $pos;
        $message  .= "$indent$mod->{module}, line $mod->{line}";
        $message  .= sprintf(" (%lf)", $mod->{'time'}) if $mod->{time};
#	$message  .= " " . $mod->{inc};
        warn "$message\n";
    }
}

1;
