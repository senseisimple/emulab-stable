#!/usr/bin/perl
#-----------------------------#
#  PROGRAM:  log_analyzer.pl  #
#-----------------------------#
#This script analyzes the log files of the supafly application.

$numArgs = $#ARGV +1;

# foreach $argnum (0 .. $#ARGV) {
#    print "$ARGV[$argnum]\n";
# }

#print `clear`; 

print "------------------------------------------\n";
print "              Log Analysis \n";
print "\n USAGE: perl log_analyzer.pl <logfilename>\n";
print "------------------------------------------\n\n";

#Process the Logfile

open(LOG,"$ARGV[0]") or die "Unable to open logfile: $!\n\n";

print "The name of the log file was: $ARGV[0] \n\n";

open(TMP_DUMP, ">tmp_dump.txt") or die " $!\n";

while(<LOG>){
    print TMP_DUMP if /\bBLOCKTIME\b/i;
}
close(LOG);
close(TMP_DUMP);

#@name = split(/./, $ARGV[0]);

open(TMP_DUMP, "tmp_dump.txt") or die " $!\n";
open(VALUES, ">values.txt") or die " $!\n";

$printed = 0;
$i = 0;
$baseT = 0.0;
while (<TMP_DUMP>) {
    @tmp = split;  
    if($printed == 0)
    {
	print "OPERATION PERFORMED: ";
	&determine_operation($tmp[0]);
	$printed = 1;
    }

    if (!$i) {
	$baseT = $tmp[3];
	++$i;
    }
    
    $tmp1 = $tmp[3]-$baseT;
    print VALUES "$tmp1 $tmp[1]\n";
    push (@duration, $tmp[1]);
    push (@finish_time, $tmp1); 
}
close(VALUES);
close(TMP_DUMP);

$tmp = `rm tmp_dump.txt`;

#&print_array(@duration);

#&print_array(@finish_time);

$length = scalar(@duration);
print "Length             = " . $length . "\n\n";

# Print the statistics for the Duration
print "Statistics for Duration\n";
print "-----------------------\n";
print "Max Duration       = " . &maximum(@duration) . " microseconds\n";
print "Min Duration       = " . &minimum(@duration) . " microseconds\n";
$mean = &mean(@duration);
$vari = &variance($mean, @duration);
$std = sprintf("%.3f",sqrt($vari));
print "Mean Duration      = " . $mean . " microseconds\n";
print "Duration Variance  = " . $vari . " \n";
print "Duration Std. Dev. = " . $std . " \n\n";

#Print the statistics for the Finish Times
print "Statistics for Finish Times\n";
print "---------------------------\n";
print "Max Finish Time(FT)= " . &maximum(@finish_time) . " microseconds\n";
print "Min Finish Time(FT)= " . &minimum(@finish_time) . " microseconds\n";
$mean = &mean(@finish_time);
$vari = &variance($mean, @finish_time);
$std = sprintf("%.3f",sqrt($vari));
print "Mean FT            = " . $mean . " microseconds\n";
print "FT Variance        = " . $vari . " \n";
print "FT Std. Dev.       = " . $std . " \n";

$tmp = `gnuplot plot.cmd; gv dur_vs_fin.eps &`;

## Procedures in PERL

sub print_array
{
    for my $val (@_) {
	print $val . "\n";
    }
}

sub determine_operation
{
    my $val = $_[0];
    if($val =~ m/BLOCKTIME\(de/i){
	print "Decryption and Encryption \n\n";
    }
    elsif ($val =~ m/BLOCKTIME\(e/i){
	print "Encryption only \n\n";
    }
    else{
	print "ERROR: Unknown Operation\n\n";
    }
}

sub maximum
{
    my $max = 0;
    foreach my $item (@_)
    {
	if($item > $max)
	{
	    $max = $item;
	}
    }
    return $max;
}

sub minimum
{
    my $min = 1000000;
    foreach my $item (@_)
    {
	if($item < $min)
	{
	    $min = $item;
	}
    }
    return $min;

}

sub mean
{
    my $sum = 0;
    my $length=scalar(@_);
    foreach my $item (@_)
    { 
	$sum += $item;   
    }
    return sprintf("%.3f",$sum/$length);
}

sub variance
{
    my ($mean, @array) = @_;
    my $length = scalar(@array);
    my $variance, $sum; 
    foreach my $item (@array)
    { 	
	$sum += ($item - $mean)*($item - $mean);  
    }
    $variance = $sum/$length;
    return sprintf("%.3f",$variance);
    
}
