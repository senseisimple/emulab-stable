#!/usr/bin/perl
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006 University of Utah and the Flux Group.
# All rights reserved.
#


my $expName;
my $projName;
my $logsDir;

my $newExpName;
my $newProjName;

%bottleNecks = {};
my %nodeClasses;
my %nodeNameMap = {};

die "Usage: perl sharedBottle.pl proj_name exp_name newProj_name newExp_name initial_conditions.txt(Optional)"
if($#ARGV < 3);

$projName = $ARGV[0];
$expName = $ARGV[1];
$newProjName = $ARGV[2];
$newExpName = $ARGV[3];
$initialConditionsFilename = $ARGV[4];

$logsDir = "/tmp/testLogs";


# Get the initial conditions.
$elabInitScript = "/proj/tbres/duerig/testbed/pelab/init-elabnodes.pl";
$initConditionsCommand = $elabInitScript . " -o /tmp/initial-conditions.txt " . $newProjName . " " . $newExpName; 

if($#ARGV == 3) 
{
    system($initConditionsCommand);
    $initialConditionsFilename = "/tmp/initial-conditions.txt";
}

open(CONDITIONS, $initialConditionsFilename);

my @initialConditions = ();

while(<CONDITIONS>)
{
    chomp( $_ );
    push(@initialConditions, $_);
}

close(CONDITIONS);

my @addressList = ();
my $addrFlag = 0;

my %bwMap = {};
my %delayMap = {};
my %elabMap = {};

# Create & send events.
# Get initial conditions for the paths of interest
# from the database, using init-elabnodes.pl
my $tevc = "/usr/testbed/bin/tevc -e $newProjName/$newExpName now";

#@@@`/usr/testbed/bin/tevc -w -e $newProjName/$newExpName now elabc reset`;
#@@@`$tevc elabc create start`;

# Create a list of the IP addresses.
foreach $conditionLine (@initialConditions)
{
    if($conditionLine =~ /(\d*?\.\d*?\.\d*?\.(\d*?))\s(\d*?\.\d*?\.\d*?\.\d*?)\s(\d*?)\s(\d*?)\s[\d\w\-\.]*/)
    {
        $srcAddress = $1;
        $addrFlag = 0;

        foreach $addrEntry (@addressList)
        {
            if($addrEntry eq $srcAddress)
            {
                $addrFlag = 1;
            }
        }

        if($addrFlag == 0)
        {
            push(@addressList, $srcAddress);

            $elabMap{$srcAddress} = "elabc-elab-" . $2;
            print "Mapping $2 TO $elabMap{$srcAddress}\n";
        }

# Create a mapping of the initial conditions.
        $bwMap{$1}{$3} = $4;
        $delayMap{$1}{$3} = $5;
    }
}

opendir(logsDirHandle, $logsDir);

my $addrIndex = 0;
my %addrNodeMapping = {};
my %nodeNumberMapping = {};
my $numDests = 0;

foreach $sourceName (readdir(logsDirHandle))
{
# Map the elab IP address in the initial conditions file, with
# the node names in the gather-results logs directory.

    if( (-d $logsDir . "/" . $sourceName ) && $sourceName ne "." && $sourceName ne ".." )
    {
        $addrNodeMapping{$sourceName} = $addressList[$addrIndex];
        $nodeNameMapping{$sourceName} = $addrIndex + 1; 
        $nodeNumberMapping{$addrIndex + 1} = $sourceName; 
        $addrIndex++;
        $numDests++;
    }
}

rewinddir(logsDirHandle);

# Descend into all the source directories
foreach $sourceName (readdir(logsDirHandle))
{
    if( (-d $logsDir . "/" . $sourceName ) && $sourceName ne "." && $sourceName ne ".." )
    {

        my @destLists;
# Then search for all possible destinations for 
# a particular source.
        opendir(sourceDirHandle, $logsDir . "/" . $sourceName);

        my @denoisedDelays = ();

#        print "Into directory $sourceName\n";

        foreach $destOne (readdir(sourceDirHandle))
        {
            if( (-d $logsDir . "/" . $sourceName . "/" . $destOne) && $destOne ne "." && $destOne ne ".." )
            {
                # Inside each destination directory, look for 
                # delay.log file with delay values.

#                print "Into directory $sourceName/$destOne\n";

                $fullPath = "$logsDir/$sourceName/$destOne/delay.log";
                $waveletScript = "./MinSikKim/MinSikKimProgram $fullPath";
                my @scriptOutput;

                @scriptOutput = `$waveletScript`;
                $denoisedDelays[$nodeNameMapping{$destOne}] = [ @scriptOutput ];

            }
        }
        closedir(sourceDirHandle);

        local @equivClasses = ();

        $tmpTreeRecordFile = "/tmp/bw-wavelet-clustering.rcd";

        open(RECORDFILE, ">$tmpTreeRecordFile");
        print RECORDFILE "128\n";
        for($i = 1; $i <= $numDests; $i++)
        {
            if($i == $nodeNameMapping{$sourceName})
            {
                next;
            }
            else
            {
                my @scriptOutput;
                @scriptOutput = @{ $denoisedDelays[$i] };

                # Put this destination in a seperate cluster - we
                # have zero samples/delay values.
                if( ($#scriptOutput == 0) or ($#scriptOutput < 120) )
                {
                    my @newEquivClass = ();
                    push(@newEquivClass, $i);

                    push(@equivClasses, [@newEquivClass]);
                    print "$sourceName: WARNING: Creating a new equiv class/cluster for $nodeNumberMapping{$i} due to lack of samples($#scriptOutput)\n";
                }
                else
                {
                    my $counter = 0;
                    my $avgValue = 0;
                    my @delayValueArray = ();
                    foreach $delayValue (@scriptOutput)
                    {
                        chomp($delayValue);

                        if($counter < 128)
                        {
                            $avgValue += $delayValue;
                            push(@delayValueArray, $delayValue);
                            $counter++;
                        }
                        else
                        {
                            last;
                        }
                    }
                    $avgValue = $avgValue/128.0;

                    my $denominator = 0;
                    foreach $delayValue (@delayValueArray)
                    {
                        $denominator += ($delayValue - $avgValue)*($delayValue - $avgValue);
                    }
                    $denominator = sqrt($denominator);

                    # Exclude paths with low-variance.
                    if($denominator < 25)
                    {
                        my @newEquivClass = ();
                        push(@newEquivClass, $i);

                        push(@equivClasses, [@newEquivClass]);
                        print "$sourceName: WARNING: Creating a new equiv class/cluster for $nodeNumberMapping{$i} due to low variance of samples($denominator)\n";
                        next;
                    }

                    foreach $delayValue (@delayValueArray)
                    {
                        $delayValue = ($delayValue - $avgValue)/$denominator;
                        print RECORDFILE "$delayValue:";
                    }

                    print RECORDFILE "$i\n";
                }
            }
        }
        close(RECORDFILE);

        $clusteringProgram = "/tmp/clustering/c++-samples/wvcluster $tmpTreeRecordFile /tmp/tmp.idx";

        my @clusteringOutput ; 

        @clusteringOutput = `$clusteringProgram`;

        foreach $clusterLine (@clusteringOutput)
        {
            @clusterElements = split(/ /, $clusterLine);
            my @newEquivClass = ();

            foreach $nodeId (@clusterElements)
            {
                if($nodeId =~ /\d+/)
                {
                    push(@newEquivClass, $nodeId);
                }

            }
            push(@equivClasses, [@newEquivClass]);
        }

        print "Clusters for $sourceName:\n";
        foreach $tmpName (@equivClasses)
        {
            foreach $tmpName2 (@$tmpName)
            { 
                print "$nodeNumberMapping{$tmpName2} ";
            }
            print "\n";
        }

        print "\n";

# Send the events to all the nodes which form an equivalent class.
        foreach $tmpName (@equivClasses)
        {
            my $bwEventCommand = "$tevc $elabMap{$addrNodeMapping{$sourceName}} modify DEST=";
            my $firstDestFlag = 0;

            my $maxBw = 0;

# Find the maximum available bandwidth in all the paths of this equivalence class.
            foreach $tmpName2 (@$tmpName)
            {
                if($bwMap{$addrNodeMapping{$sourceName}}{$addrNodeMapping{$nodeNumberMapping{$tmpName2}}} > $maxBw)
                {
                    $maxBw = $bwMap{$addrNodeMapping{$sourceName}}{$addrNodeMapping{$nodeNumberMapping{$tmpName2}}};
                }
            }

            foreach $tmpName2 (@$tmpName)
            {
                if($firstDestFlag == 0)
                {
                    $bwEventCommand = $bwEventCommand . $addrNodeMapping{$nodeNumberMapping{$tmpName2}};
                    $firstDestFlag = 1;
                }
                else
                {
                    $bwEventCommand = $bwEventCommand . "," . $addrNodeMapping{$nodeNumberMapping{$tmpName2}};
                }


# 		my $delayEventCommand = "$tevc $elabMap{$addrNodeMapping{$sourceName}} modify DEST=" . $addrNodeMapping{$destSeen[$tmpName2]};
                my $delayEventCommand = "$tevc ".$elabMap{$addrNodeMapping{$nodeNumberMapping{$tmpName2}}}." modify DEST=" . $addrNodeMapping{$nodeNumberMapping{$tmpName2}}." SRC=".$addrNodeMapping{$sourceName};

                $delayEventCommand = $delayEventCommand . " " . "DELAY=" . ($delayMap{$addrNodeMapping{$sourceName}}{$addrNodeMapping{$nodeNumberMapping{$tmpName2}}});
# Execute the delay event command.
                print "EXECUTE $delayEventCommand\n";
   #@@@             `$delayEventCommand`;
            }
            $bwEventCommand = $bwEventCommand . " " . "BANDWIDTH=" . $maxBw;
# Execute the event to set the bandwidth for this equivalence class.
            print "EXECUTE $bwEventCommand\n";
      #@@@      `$bwEventCommand`;
        }

            print "\n\n";
    }
}

closedir(logsDirHandle);
