#!/usr/bin/perl
#-----------------------------#
#  PROGRAM:  log_analyzer.pl  #
#-----------------------------#
#This script analyzes the log files of the supafly application.





## Initialization
print "-----------------------------------------------------------------------------\n";
print "                                Log Analysis \n\n";
print "                All files are in the \"generated\" subdirectory \n";
print "\n USAGE  : ./log_analyzer.pl <middleman> <sender> <receiver> <<fillme>lab>\n";
print "\n Example: ./log_analyzer.pl middleman.log sender.log receiver.log planetlab\n";
print "-----------------------------------------------------------------------------\n\n";

$numArgs = $#ARGV +1;

if ($numArgs != 4){
    die "Incorrect Usage!\n";
}
 
# foreach $argnum (0 .. $#ARGV) {
#    print "$ARGV[$argnum]\n";
# }





## Make the output directories if they do not exist
`mkdir generated`;
`mkdir generated/$ARGV[3]`;
`mkdir generated/$ARGV[3]/tmp`;
$dirpath = "./generated/$ARGV[3]";
$tmppath = "./generated/$ARGV[3]/tmp";





#Read the imput logfiles

open(LOG1,"$ARGV[0]") or die "Unable to open logfile: $!\n\n";
open(LOG2,"$ARGV[1]") or die "Unable to open logfile: $!\n\n";
open(LOG3,"$ARGV[2]") or die "Unable to open logfile: $!\n\n";

print "\nThe name of the middleman log file was : $ARGV[0] \n\n";
print "The name of the sender log file was    : $ARGV[1] \n\n";
print "The name of the receiver log file was  : $ARGV[2] \n\n";
print "The analysis is being done for         : $ARGV[3] \n\n";





## Process the log files to get the lines that have relevant data
## Processing the Middleman Log
$tmpfilename = &prepend_tmp("mid_tmp_dump.txt");
open(MID_TMP_DUMP, ">$tmpfilename") or die " $!\n";

while(<LOG1>){
    print MID_TMP_DUMP if /\bBLOCKTIME\b/i;
}
close(LOG1);
close(MID_TMP_DUMP);

## Processing the Sender Log
$tmpfilename = &prepend_tmp("snd_tmp_dump.txt");
open(SND_TMP_DUMP, ">$tmpfilename") or die " $!\n";

while(<LOG2>){
    print SND_TMP_DUMP if /\bTIME\b/i;
}
close(LOG2);
close(SND_TMP_DUMP);

## Processing the Receiver Log
$tmpfilename = &prepend_tmp("rcv_tmp_dump.txt");
open(RCV_TMP_DUMP, ">$tmpfilename") or die " $!\n";

while(<LOG3>){
    print RCV_TMP_DUMP if /\bTIME\b/i;
}
close(LOG3);
close(RCV_TMP_DUMP);





## Get the values from the processed Middleman logfile into arrays
## The format of the input for the Middleman should be
##    <Tag>      <Block_Num> <Duration> <Text>    <Finish_Time>
## BLOCKTIME(de):    16         323       at    1160440380.674084
##
# Middleman Processing
$tmpfilename1 = &prepend_tmp("mid_tmp_dump.txt");
$tmpfilename2 = &prepend("middleman_values.dat");
open(MID_TMP_DUMP, "$tmpfilename1") or die " $!\n";
open(MID_VALUES, ">$tmpfilename2") or die " $!\n";

$printed = 0;
$i = 0;
$baseT = 0.0;
while (<MID_TMP_DUMP>) {
    @tmp = split;  
    if($printed == 0)
    {
	print "OPERATION PERFORMED: ";
	&determine_operation($tmp[0]);
	$printed = 1;
    }

## ***********************************************************
## *************************NOTE******************************
## ***********************************************************
## USE THE FOLLOWING ONLY FOR PROCESSING EMULAB AND PELAB LOGS
## FOR PLANETLAB COMMENT THIS OUT AND UNCOMMENT THE BELOW

## START_NEW_CODE
    if (!$i) {
	$baseT = $tmp[4];
	++$i;
    }
    
    $time_diff = $tmp[4]-$baseT;
    
    ## Write the values to a text file for Gnuplot processing
    ## FORMAT:     <Block_Num> <Time-Diff> <Duration>
    print MID_VALUES "$tmp[1]   $time_diff   $tmp[2]\n";

    ## At the same time put the values into an array as well
    push (@blocknum, $tmp[1]);
    push (@duration, $tmp[2]);
    push (@finish_time, $time_diff); 
## END_NEW_CODE


## ***********************************************************
## *************************NOTE******************************
## ***********************************************************
## USE THE FOLLOWING ONLY FOR PROCESSING PLANETLAB LOGS
## FOR ALL OTHERS COMMENT THIS OUT AND UNCOMMENT THE ABOVE

## START_OLD_CODE
#     if (!$i) {
#         $baseT = $tmp[3];
#         ++$i;
#     }

#     $time_diff = $tmp[3]-$baseT;
#     ## Write the values to a text file for Gnuplot processing
#     ## FORMAT:       <Time-Diff> <Duration>
#     print MID_VALUES "$time_diff  $tmp[1]\n";
#     push (@duration, $tmp[1]);
#     push (@finish_time, $time_diff);
## END_OLD_CODE

}
close(MID_VALUES);
close(MID_TMP_DUMP);





## Sender and Receiver logfile processing 
## The format of the input for the Sender should be
##    <Tag>      <Block_Num>   <Send_Time>
##    TIME           56       1160440380.7564
##
## The format of the input for the Receiver should be
##    <Tag>      <Block_Num>   <Receive_Time>
##    TIME           16       1160445708.338260

#Sender and Receiver Processing
$tmpfilename1 = &prepend_tmp("snd_tmp_dump.txt");
$tmpfilename2 = &prepend_tmp("rcv_tmp_dump.txt");
open(SND_TMP_DUMP, "$tmpfilename1") or die " $!\n";
open(RCV_TMP_DUMP, "$tmpfilename2") or die " $!\n";

while (<SND_TMP_DUMP>) {
    @tmp = split;  
    push (@snd_time, $tmp[2]); 
}

while (<RCV_TMP_DUMP>) {
    @tmp = split;  
    push (@rcv_time, $tmp[2]); 
}

close(RCV_TMP_DONE);
close(SND_TMP_DUMP);





## Write the final values to be plotted into a file
$datafilename = &prepend($ARGV[3] . ".dat");
open(SNDRCV_VALUES, ">$datafilename") or die " $!\n";

my $min = &minimum($#rcv_time, $#snd_time);
for($i=0; $i<=$min; $i++)
{
    my $time_diff = $rcv_time[$i] - $snd_time[$i];
    push (@time_diff, $time_diff); 
    my $tmp_val = $finish_time[$i];
    print SNDRCV_VALUES "$tmp_val $time_diff\n";
}
close(SNDRCV_VALUES);





## Clean up all unnecessary files
#$tmp = `rm mid_tmp_dump.txt`;





## Print statistics into the Statistics file as needed
$statfilename = &prepend($ARGV[3] . "_stat_file.txt");
open(STATS_FILE, ">$statfilename") or die " $!\n";

#&print_array(@blocknum);

#&print_array(@duration);

#&print_array(@finish_time);

$length = scalar(@duration);
print STATS_FILE "Length             = " . $length . "\n\n";

# Print the statistics for the Duration measured at middleman
print STATS_FILE "Statistics for duration measured at middleman\n";
print STATS_FILE "---------------------------------------------\n";
print STATS_FILE "Max Duration       = " . &maximum(@duration) . " microseconds\n";
print STATS_FILE "Min Duration       = " . &minimum(@duration) . " microseconds\n";
$mean = &mean(@duration);
$vari = &variance($mean, @duration);
$std = sprintf("%.3f",sqrt($vari));
print STATS_FILE "Mean Duration      = " . $mean . " microseconds\n";
print STATS_FILE "Duration Variance  = " . $vari . " \n";
print STATS_FILE "Duration Std. Dev. = " . $std . " \n\n";

#Print the statistics for the Finish Times measured at middleman
print STATS_FILE "Statistics for timestamps measured at middleman\n";
print STATS_FILE "-----------------------------------------------\n";
$max_ft = &maximum(@finish_time);
$min_ft = &minimum(@finish_time);
print STATS_FILE "Max Finish Time(FT)= " . $max_ft . " microseconds\n";
print STATS_FILE "Min Finish Time(FT)= " . $min_ft . " microseconds\n";
$mean = &mean(@finish_time);
$vari = &variance($mean, @finish_time);
$std = sprintf("%.3f",sqrt($vari));
print STATS_FILE "Mean FT            = " . $mean . " microseconds\n";
print STATS_FILE "FT Variance        = " . $vari . " \n";
print STATS_FILE "FT Std. Dev.       = " . $std . " \n\n";

# Print the statistics for the (receiverTS - senderTS) measured at sender and receiver 
#&print_array(@time_diff);
print STATS_FILE "Statistics for (receiverTS - senderTS)\n";
print STATS_FILE "--------------------------------------\n";
$max_td = &maximum(@time_diff);
$min_td = &minimum(@time_diff);
print STATS_FILE "Max Time_Diff        = " . $max_td . " microseconds\n";
print STATS_FILE "Min Time_Diff        = " . $min_td . " microseconds\n";
$mean = &mean(@time_diff);
$vari = &variance($mean, @time_diff);
$std = sprintf("%.3f",sqrt($vari));
print STATS_FILE "Mean Time_Diff       = " . $mean . " microseconds\n";
print STATS_FILE "Time_Diff Variance   = " . $vari . " \n";
print STATS_FILE "Time_Diff Std. Dev.  = " . $std . " \n\n";





## Gnuplot Commands
## Call the other script as follows
$datfilename = &prepend($ARGV[3]);

`./plot.sh $datfilename $min_ft $max_ft $min_td $max_td`;

## Display the graph
$fromfilename = &prepend($ARGV[3] . ".eps");
$tofilename   = &prepend($ARGV[3] . "_lag.eps");
`mv $fromfilename $tofilename`;
`gv $tofilename &`;





## Functions

sub prepend
{
    return $dirpath . "/" . $_[0]; 
}

sub prepend_tmp
{
    return $tmppath . "/" . $_[0]; 
}

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
