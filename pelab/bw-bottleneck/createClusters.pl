#!/usr/bin/perl
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006 University of Utah and the Flux Group.
# All rights reserved.
#
use Getopt::Long;

local $RubensteinScriptPath = "/proj/tbres/pramod/bottleneck-emulab/filter/genjitco.i386";
local $MinSikKimScriptPath = "/proj/tbres/pramod/bottleneck-emulab/devel/MinSikKim/MinSikKimProgram";
local $ClusterProgramPath = "/proj/tbres/pramod/bottleneck-emulab/devel/Clustering/c++-samples/wvcluster";
local $elabInitScript = "/proj/tbres/duerig/testbed/pelab/init-elabnodes.pl";


local $newExpName = "";
local $newProjName = "";
local $logsDir = "";
local $nodeListFilename = "";
local $Algorithm = "";
local $initialConditionsFilename = "";

local $scratchSpaceDir = "/tmp/bw-bottleneck/rubenstein";

# Print usage and die.

sub PrintUsage()
{
        print "Usage: perl createClusters.pl -NewProj=newProjName -NewExp=newExpName -LogDir=/path/to/logs -nodeFile=/path/to/nodeListFile -Algorithm=rubestein/wavelet (rest optional) -RubensteinPath=/path/to/genjitco.arch -MinSikKimPath=/path/to/MinSikKimProgram -ClusterProgPath=/path/to/wvcluster -InitConditions=/path/to/initial_conditions.txt\n";
        exit(1);

}

# Check for the validity of command line arguments.

sub processOptions()
{
    &GetOptions("LogDir=s"=>\$logsDir,
            "NewProj=s"=>\$newProjName,
            "NewExp=s"=>\$newExpName,
            "InitConditions=s"=>\$initialConditionsFilename,
            "NodeFile=s"=>\$nodeListFilename,
            "Algorithm=s"=>\$Algorithm,
            "RubensteinPath=s"=>\$RubensteinScriptPath,
            "MinSikKimPath=s"=>\$MinSikKimScriptPath,
            "ClusterProgPath=s"=>\$ClusterProgramPath);

    if($newExpName eq "" or $newProjName eq "" or $logsDir eq "" or $nodeListFilename eq "" or $Algorithm eq "")
    {
        print "ERROR: One of the mandatory arguments is empty.\n";
        &PrintUsage();
    }

    if(not(-e $logsDir))
    {
        print "ERROR: Path provided to the logs directory is invalid.\n";
        &PrintUsage();
    }

    if(not(-e $nodeListFilename))
    {
        print "ERROR: Path provided to the file with node names is invalid.\n";
        &PrintUsage();

    }

    if( not(  ($Algorithm =~ /rubenstein/i) || ($Algorithm =~ /wavelet/i) ) )
    {
        print "ERROR: Please choose either 'rubenstein' or 'wavelet' ( without quotes, case insensitive ) as the algorithm\n";
        &PrintUsage();
    }

    if($initialConditionsFilename ne "")
    {
        if(not(-e $initialConditionsFilename))
        {
            print "WARNING: The initial conditions file path provided at command line is invalid.\n";
            print "Reverting back to using init-elabnodes.pl\n";
                
        }
    }
}


#############################################################################
############################    MAIN    #####################################
#############################################################################

# Process the command line options.
&processOptions();

# Get the initial conditions.
# Get initial conditions for the paths of interest
# from the database, using init-elabnodes.pl

$initConditionsCommand = $elabInitScript . " -o /tmp/initial-conditions.txt " . $newProjName . " " . $newExpName; 

if( ($initialConditionsFilename eq "") or (not(-e $initialConditionsFilename)) ) 
{
    system($initConditionsCommand);
    $initialConditionsFilename = "/tmp/initial-conditions.txt";
}

open(CONDITIONS, $initialConditionsFilename);
my @initialConditions = <CONDITIONS>;
chomp(@initialConditions);
close(CONDITIONS);

my @addressList = ();
my $addrFlag = 0;

my %bwMap = {};
my %delayMap = {};
my %elabMap = {};
my $numNodes = 0;
# Create a list of the IP addresses listed in the initial-conditions file.
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
            $numNodes += 1;

            $elabMap{$srcAddress} = "elabc-elab-" . $2;
            print "Mapping $srcAddress to $elabMap{$srcAddress}\n";
        }

# Create a mapping of the initial conditions.
        $bwMap{$1}{$3} = $4;
        $delayMap{$1}{$3} = $5;
    }
}

# Read the file listing the hostnames of the nodes involved in this
# particular measurement.

open(INFILE, "<$nodeListFilename");
my @nodeNameList = <INFILE>;
chomp(@nodeNameList);
close(INFILE);

# A little checking to ensure that the number of nodes in the NodeList file
# matches the values in the initial conditions file.
if($numNodes != ($#nodeNameList+1) )
{
    print "ERROR: Mismatch between the number of nodes in InitialConditions.txt and in the NodeList file.\n";
    print "Both the files should have the same number of nodes for a 1-to-1 mapping to be possible.\n";
    exit(1);
}

foreach $tmpDirName (@nodeNameList)
{
    # Create empty directories if some of the log files were missing.
    if(not (-d "$logsDir/$tmpDirName"))
    {
        print "@ WARNING: Creating dummy directory $logsDir/$tmpDirName and its sub-directories.\n";
        foreach $tmpDirName2 (@nodeNameList)
        {
            if($tmpDirName2 ne $tmpDirName)
            {
                system("mkdir -p $logsDir/$tmpDirName/$tmpDirName2");
            }
        }
    }
}

# Convert the delay data produced by the wavelet code
# into something palatable to the Rubenstein program.
if($Algorithm =~ /rubenstein/i)
{
    if(-e $scratchSpaceDir)
    {
        `rm -rf $scratchSpaceDir`;
    }

    `mkdir -p $scratchSpaceDir`;

    opendir(logsDirHandle, $logsDir);
    foreach $sourceName (readdir(logsDirHandle))
    {
        if( (-d $logsDir . "/" . $sourceName ) and $sourceName ne "." and $sourceName ne ".." )
        {
            if( grep(/^$sourceName$/, @nodeNameList) )
            {
                @dirNameList = ();

                opendir(sourceDirHandle, $logsDir . "/" . $sourceName);
                foreach $destOne (readdir(sourceDirHandle))
                {
                    if( (-d $logsDir . "/" . $sourceName . "/" . $destOne) && $destOne ne "." && $destOne ne ".." )
                    {
                        if( grep(/^$destOne$/, @nodeNameList) )
                        {
                            if(-e "$logsDir/$sourceName/$destOne/delay.log")
                            {
                                push(@dirNameList, $destOne);
                            }
                            else
                            {
                                push(@dirNameList, $destOne);
                                print "WARNING: Missing log file $logsDir/$sourceName/$destOne/delay.log\n";
                            }
                        }
                    }
                }
                closedir(sourceDirHandle);

                my $firstLevelIndex = 0, $secondLevelIndex = 0;

                foreach $destOne (@dirNameList)
                {
                    $secondLevelIndex = 0;
                    foreach $destTwo (@dirNameList)
                    {
                        if($secondLevelIndex > $firstLevelIndex)
                        {
                            `mkdir -p $scratchSpaceDir/$sourceName/$destOne/$destTwo`;
                            if((-e "$logsDir/$sourceName/$destOne/delay.log") and (-e "$logsDir/$sourceName/$destTwo/delay.log"))
                            {
                                `paste -d ' ' $logsDir/$sourceName/$destOne/delay.log $logsDir/$sourceName/$destTwo/delay.log > $scratchSpaceDir/$sourceName/$destOne/$destTwo/delay.log `;
                            }
                        }

                        $secondLevelIndex++;
                    }
                    $firstLevelIndex++;
                }
            }
        }
    }

    closedir(sourceDirHandle);
# Call our sharedBottle.pl to feed these .filter files we created to
# sharedBottle.pl - It will then create equivalence classes for the
# paths.

    system("perl sharedBottle.pl $scratchSpaceDir $newProjName $newExpName $initialConditionsFilename $RubensteinScriptPath");

# Our work here is done, sign off.
    exit(0);
}

#############################################################################
######################### Wavelet processing ################################
#############################################################################

%bottleNecks = {};
my %nodeClasses;
my %nodeNameMap = {};

# Create & send events.
my $tevc = "/usr/testbed/bin/tevc -e $newProjName/$newExpName now";

#`/usr/testbed/bin/tevc -w -e $newProjName/$newExpName now elabc reset`;
#`$tevc elabc create start`;

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
        print "Mapping $sourceName to $addressList[$addrIndex]\n";
        $addrIndex++;
        $numDests++;
    }
}

rewinddir(logsDirHandle);

# Descend into all the source directories
foreach $sourceName (readdir(logsDirHandle))
{
    if( (-d "$logsDir/$sourceName" ) && $sourceName ne "." && $sourceName ne ".." )
    {

        my @destLists;
# Then search for all possible destinations for 
# a particular source.
        opendir(sourceDirHandle, $logsDir . "/" . $sourceName);

        my @denoisedDelays = ();

        for($i = 0; $i < $numDests; $i++)
        {
            push(@denoisedDelays, 0);
        }

        foreach $destOne (readdir(sourceDirHandle))
        {
            if( (-d "$logsDir/$sourceName/$destOne") && $destOne ne "." && $destOne ne ".." )
            {
                # Inside each destination directory, look for 
                # delay.log file with delay values.

                $fullPath = "$logsDir/$sourceName/$destOne/delay.log";
                my @scriptOutput = ();
                if(not(-r $fullPath))
                {
                    $denoisedDelays[$nodeNameMapping{$destOne}] = [ @scriptOutput ];
                    next;
                }
                $waveletScript = "$MinSikKimScriptPath $fullPath";

# Indicates low variance of the delay samples.
                if((&CheckVariance($fullPath) == 1))
                {
                    $denoisedDelays[$nodeNameMapping{$destOne}] = [ @scriptOutput ];
                }
                else
                {
                    @scriptOutput = `$waveletScript`;
                    $denoisedDelays[$nodeNameMapping{$destOne}] = [ @scriptOutput ];
                }

            }
        }
        closedir(sourceDirHandle);

        my $numOfSamples = 128;
        local @equivClasses = ();

        $tmpTreeRecordFile = "/tmp/bw-wavelet-clustering.rcd";

        open(RECORDFILE, ">$tmpTreeRecordFile");
        print RECORDFILE "$numOfSamples\n";
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
                if( ($#scriptOutput == 0) or ($#scriptOutput < $numOfSamples) )
                {
                    my @newEquivClass = ();
                    push(@newEquivClass, $i);

                    push(@equivClasses, [@newEquivClass]);
                    print "@@ $sourceName: WARNING: Creating a new equiv class/cluster for $nodeNumberMapping{$i} due to lack of samples($#scriptOutput) or low variance\n";
                }
                else
                {
                    my $counter = 0;
                    my $avgValue = 0;
                    my @delayValueArray = ();
                    foreach $delayValue (@scriptOutput)
                    {
                        chomp($delayValue);

                        if($counter < $numOfSamples)
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
                    #############################################
                    #$avgValue = $avgValue/$numOfSamples;

                    #my $denominator = 0;
                    #foreach $delayValue (@delayValueArray)
                    #{
                    #    $denominator += ($delayValue - $avgValue)*($delayValue - $avgValue);
                    #}
                    #$denominator = sqrt($denominator);

                    # Exclude paths with low-variance.
                    #if($denominator < 25)
                    #{
                    #    my @newEquivClass = ();
                    #    push(@newEquivClass, $i);
#
#                        push(@equivClasses, [@newEquivClass]);
#                        print "$sourceName: WARNING: Creating a new equiv class/cluster for $nodeNumberMapping{$i} due to low variance of samples($denominator)\n";
#                        next;
#                    }
                    #############################################

                    foreach $delayValue (@delayValueArray)
                    {
                        #$delayValue = ($delayValue - $avgValue)/$denominator;
                        #print RECORDFILE "$delayValue:";
                        printf (RECORDFILE "%.4f:",$delayValue);
                    }

                    print RECORDFILE "$i\n";
                }
            }
        }
        close(RECORDFILE);


        $clusteringProgram = "$ClusterProgramPath $tmpTreeRecordFile /tmp/tmp.idx";

        my @clusteringOutput  = (); 

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

        print "@@@ Clusters for $sourceName:\n";
        foreach $tmpName (@equivClasses)
        {
            print "@@@ ";
            foreach $tmpName2 (@$tmpName)
            { 
                print "$nodeNumberMapping{$tmpName2} ";
            }
            print " \n";
        }

        print "@@@ \n";

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


                my $delayEventCommand = "$tevc ".$elabMap{$addrNodeMapping{$nodeNumberMapping{$tmpName2}}}." modify DEST=" . $addrNodeMapping{$nodeNumberMapping{$tmpName2}}." SRC=".$addrNodeMapping{$sourceName};

                $delayEventCommand = $delayEventCommand . " " . "DELAY=" . ($delayMap{$addrNodeMapping{$sourceName}}{$addrNodeMapping{$nodeNumberMapping{$tmpName2}}});
# Execute the delay event command.
                print "EXECUTE $delayEventCommand\n";
                #`$delayEventCommand`;
            }
            $bwEventCommand = $bwEventCommand . " " . "BANDWIDTH=" . $maxBw;
# Execute the event to set the bandwidth for this equivalence class.
            print "EXECUTE $bwEventCommand\n";
            #`$bwEventCommand`;
        }

            print "\n\n";
    }
}

closedir(logsDirHandle);

sub CheckVariance()
{
    open(FILEHANDLE, $_[0]);
    my @delayValueArray = ();
    while(<FILEHANDLE>)
    {
        if(/(\-?[0-9]*) ([0-9]*)/)
        {
            push(@delayValueArray, $1);
        }

    }
    close(FILEHANDLE);
    chomp($delayValueArray);


    my $avgValue = 0;
    if($#delayValueArray == -1)
    {
        return 1;
    }

    foreach $delayValue (@delayValueArray)
    {
        $avgValue += $delayValue;
    }
    $avgValue = $avgValue/($#delayValueArray+1);

    my $variance = 0;
    foreach $delayValue (@delayValueArray)
    {
        $variance += ($delayValue - $avgValue)*($delayValue - $avgValue);
    }
    $variance = sqrt($variance);

# Exclude paths with low-variance.
    if($variance < 25)
    {
        print "@@ WARNING: Low Variance($variance) for $fullPath\n";
        return 1;
    }
    else
    {
        return 0;
    }
}

