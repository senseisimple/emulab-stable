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

die "Usage: perl sharedBottle.pl logsDir newProj_name newExp_name initial_conditions.txt DansScript"
if($#ARGV < 4);

$logsDir = $ARGV[0];
$newProjName = $ARGV[1];
$newExpName = $ARGV[2];
$initialConditionsFilename = $ARGV[3];
$DansScriptPath = $ARGV[4];

#$logsDir = "/proj/$projName/exp/$expName/logs/dump";


# Get the initial conditions.
#$elabInitScript = "/proj/tbres/duerig/testbed/pelab/init-elabnodes.pl";
#$initConditionsCommand = $elabInitScript . " -o /tmp/initial-conditions.txt " . $newProjName . " " . $newExpName; 

#if($#ARGV == 2) 
#{
#    system($initConditionsCommand);
#    $initialConditionsFilename = "/tmp/initial-conditions.txt";
#}

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

`/usr/testbed/bin/tevc -w -e $newProjName/$newExpName now elabc reset`;
`$tevc elabc create start`;

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
        }

# Create a mapping of the initial conditions.
        $bwMap{$1}{$3} = $4;
        $delayMap{$1}{$3} = $5;
    }
}

opendir(logsDirHandle, $logsDir);

my $addrIndex = 0;
my %addrNodeMapping = {};

foreach $sourceName (readdir(logsDirHandle))
{
# Map the elab IP address in the initial conditions file, with
# the node names in the gather-results logs directory.

    if( (-d $logsDir . "/" . $sourceName ) && $sourceName ne "." && $sourceName ne ".." )
    {
        $addrNodeMapping{$sourceName} = $addressList[$addrIndex];
        $addrIndex++;
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
        opendir(sourceDirHandle, "$logsDir/$sourceName");

        foreach $destOne (readdir(sourceDirHandle))
        {
            if( (-d "$logsDir/$sourceName/$destOne") && $destOne ne "." && $destOne ne ".." )
            {

# Inside each destination directory, look for 
# all possible second destinations.
                opendir(destDirHandle, "$logsDir/$sourceName/$destOne");

                foreach $destTwo (readdir(destDirHandle))
                {
                    if( (-d "$logsDir/$sourceName/$destOne/$destTwo") && $destTwo ne "." && $destTwo ne ".." )
                    {
                        $fullPath = "$logsDir/$sourceName/$destOne/$destTwo";
# Run Rubenstein's code on the ".filter" files
# inside the second destination directory.
                
                        if(-r "$fullPath/delay.log")
                        {
                            ($path1Variance, $path2Variance) = &CheckVariance("$fullPath/delay.log");

                        # One of the two paths had low variance - declare this pair of paths as being
                        # uncorrelated.
                            if(not( $path1Variance >= 25 and $path2Variance >= 25) )
                            {
                                push(@destLists,$destOne);
                                push(@destLists,$destTwo);

                                printf("WARNING: One or more of paths($sourceName -> $destOne, $sourceName -> $destTwo) have low variance(%.3f,%.3f)\n.Considering these paths as uncorrelated\n", $path1Variance, $path2Variance);
                                next;
                            }
                        }
                        else
                        {
                            print "Missing file(delay.log). Cannot process $fullPath\n";
                            next;
                        }
                        system("perl delay2dump.pl $fullPath/delay.log $fullPath");
                        system("perl dump2filter.pl $fullPath");

                        $filterFile1 = $fullPath . "/" . "source.filter";
                        $filterFile2 = $fullPath . "/" . "dest1.filter";
                        $filterFile3 = $fullPath . "/" . "dest2.filter";

                        if (!(-r $filterFile1) || !(-r $filterFile2) || !(-r $filterFile3))
                        {
                            print "Missing file. Cannot process $fullPath\n";
                            next;
                        }
                        $sharedBottleneckCheck = $DansScriptPath ." ". $filterFile1." ". $filterFile2 ." ". $filterFile3;
                            
                        print "EXECUTE: $sharedBottleneckCheck\n";
                        my @scriptOutput = ();
                        @scriptOutput = `$sharedBottleneckCheck | tail -n 2`;
#system("$sharedBottleneckCheck");

#			$scriptOutput[0] = "last CHANGE was CORRELATED, corr case: 30203 pkts, test case: 30203 pkts";
#			$scriptOutput[1] = "testingabcdef";

# "CORRELATED" means that these two nodes have
# a shared bottleneck.

                        if($scriptOutput[$#scriptOutput - 1] =~ /last CHANGE\s(\w*)\s(\w*)\,[\w\d\W\:\,]*/)
                        {
                            print "For source $sourceName: Comparing $destOne $destTwo: $scriptOutput[$#scriptOutput]";
                            $corrResult = $2;
                            if($corrResult eq "CORRELATED"
                                    && !($scriptOutput[$#scriptOutput] =~ /nan/))
                            {
                                push(@{ $bottleNecks{$sourceName} },$destOne . ":" . $destTwo);
                                push(@destLists,$destOne);
                                push(@destLists,$destTwo);
                                print "CORRELATED\n\n";

                            }
                            elsif($corrResult eq "UNCORRELATED"
                                    && !($scriptOutput[$#scriptOutput] =~ /nan/))
                            {
                                push(@destLists,$destOne);
                                push(@destLists,$destTwo);
                                print "UNCORRELATED\n\n";
                            }
                            else
                            {
                                print "ERROR:($sourceName) Something went wrong in pattern matching phase.\n";
                                print "Skipping this source node\n";
                                next;
                            }
                        }
                        else
                        {
                            print "ERROR:($sourceName) Output line from Rubenstein's code did not match regular expression.\n";
                            print "Skipping this source node\n";
                            next;
                        }
                    }

                }

                closedir(destDirHandle);
            }
        }
        closedir(sourceDirHandle);

# Make an adjacency matrix.

        local @destSeen = ();
        my $flagDestOne = 0;
        my $flagDestTwo = 0;

# Count the number of unique destinations which have measurements
# from this source node.
        my $numDests = 0;
        my %destHash;
        $tmpName = "";
        $destElement = "";

        foreach $destElement (@destLists)
        {
            $flagDest = 0;

            foreach $tmpName (@destSeen)
            {
                if($destElement eq $tmpName)
                {
                    $flagDest = 1;
                }
            }

            if($flagDest == 0)
            {
                push(@destSeen,$destElement); 
                $destHash{$destElement} = $numDests;
                $numDests++;
            }
        }

        local @adjMatrix = ();

# Zero out the adjacency matrix.
        for($i = 0; $i < $numDests; $i++)
        {
            for($j = 0; $j < $numDests; $j++)
            {
                $adjMatrix[$i][$j] = 0;
            }
        }

        foreach $destPair (@{ $bottleNecks{$sourceName} })
        {
            @destNames = split(/:/, $destPair);

            $adjMatrix[$destHash{$destNames[0]}][$destHash{$destNames[1]}] = 1;
            $adjMatrix[$destHash{$destNames[1]}][$destHash{$destNames[0]}] = 1;
        }

        local @equivClasses = ();
        my $numEquivClasses = 0;

        for($i = 0; $i < $numDests; $i++)
        {
            for($j = 0; $j < $numDests; $j++)
            {
                if($adjMatrix[$i][$j] == 1)
                {

# Check if any of these destinations is already in an equivalence class.
# If it is, then add the second destination to that class ( do some sanity checking ).
                    $firstDestFlag = 0;
                    $secondDestFlag = 0;
                    $firstDestClassNum = 0;
                    $secondDestClassNum = 0;
                    $equivClassNum = 0;

                    foreach $classArray (@equivClasses)
                    {
                        $firstDestFlag = 0;
                        $secondDestFlag = 0;

                        foreach $classDestMember (@{ $classArray } )
                        {
                            if($i == $classDestMember )
                            {
                                $firstDestFlag = 1;
                                $firstDestClassNum = $equivClassNum;
                            }
                            if($j == $classDestMember )
                            {
                                $secondDestFlag = 1;
                                $secondDestClassNum = $equivClassNum;
                            }
                        }

                        if($firstDestFlag == 1 or $secondDestFlag == 1)
                        {
                            last;
                        }
                        $equivClassNum++;
                    }

# Both these destinations are already in an equivalence class - do nothing.
                    if($firstDestFlag == 1 && $secondDestFlag == 1)
                    {
                        if($firstDestClassNum != $secondDestClassNum)
                        {
                            print "ERROR($sourceName): Two destinations sharing a bottleneck link are in different equiv. classes $i $j\n";
                            print "Skipping this source node\n";
                        }

                    }
                    elsif($firstDestFlag == 1) # Add the second destinaton to the equivalence class.
                    {
                        push(@{ $equivClasses[$firstDestClassNum] }, $j); 
                    }
                    elsif($secondDestFlag == 1) # Add the first destination to the equivalence class.
                    {
                        push(@{ $equivClasses[$secondDestClassNum] }, $i); 
                    }
                    else # Create a new equivalence class, and put these two destinations in it.
                    {
                        my @newEquivClass = ();
                        push(@newEquivClass, $i);
                        push(@newEquivClass, $j);

                        $equivClasses[$numEquivClasses] =  [ @newEquivClass ];
                        $numEquivClasses++;


                    }
                }
            }
        }


# After processing all the ".filter" files for each of
# the sources, try to find equivalence classes
# from each source. Check to ensure that the
# transitive property is not being violated.
#############
        $counter1 = 0;
        $counter2 = 0;
        my @deletionFlagArray = ();
        foreach $classVar1 (@equivClasses)
        {
            push(@deletionFlagArray,0);
        }

        foreach $classVar1 (@equivClasses)
        {
            $counter2 = 0;
            $intersectionFlag = 0;
            foreach $classVar2 (@equivClasses)
            {
                if( ($deletionFlagArray[$counter1] != 1) and ($deletionFlagArray[$counter2] != 1) and ($counter1 != $counter2) )
                {
                    $intersectionFlag = &CheckIntersection($counter1, $counter2);
                    if($intersectionFlag)
                    {
                        &MergeClasses( $counter1, $counter2);

                        #Mark the equivalence class $counter2 for deletion.
                        $deletionFlagArray[$counter2] = 1;
                    }
                }
                $counter2 += 1;
            }
            $counter1 += 1;
        }

        my @tmpEquivClasses = @equivClasses;

        @equivClasses = ();
        $counter1 = 0;
        foreach $classIter (@tmpEquivClasses)
        {
            if($deletionFlagArray[$counter1] == 0)
            {
                push(@equivClasses, [@$classIter]);
            }
            $counter1++;
        }

############

# For debugging.

        $retVal = &CheckSanity(@equivClasses, @adjMatrix);

        if($retVal != 0)
        {
            print "WARNING:($sourceName) Transitive property has been violated.\n";
        }
	print "(Debugging) Equivalence classes - $sourceName\n";
foreach $tmpName (@equivClasses)
	{
    foreach $tmpName2 (@$tmpName)
	    {
		print $destSeen[$tmpName2] . " ";
	    }
	    print "\n";

	}

for($i = 0; $i < $numDests; $i++)
{
    my $lonerFlag = 0;
    for($j = 0; $j < $numDests; $j++)
    {
        if($adjMatrix[$i][$j] == 1)
        {
            $lonerFlag = 1;
            last;
        }
    }

    if($lonerFlag == 0)
    {
		print $destSeen[$i] . "\n";
    }
}

$nodeClasses{$sourceName} = [ @equivClasses ];


# Send the events to all the nodes which form an equivalent class.
foreach $tmpName (@equivClasses)
{
    my $bwEventCommand = "$tevc $elabMap{$addrNodeMapping{$sourceName}} modify DEST=";
            my $firstDestFlag = 0;

            my $maxBw = 0;

# Find the maximum available bandwidth in all the paths of this equivalence class.
            foreach $tmpName2 (@$tmpName)
            {
                if($bwMap{$addrNodeMapping{$sourceName}}{$addrNodeMapping{$destSeen[$tmpName2]}} > $maxBw)
                {
                    $maxBw = $bwMap{$addrNodeMapping{$sourceName}}{$addrNodeMapping{$destSeen[$tmpName2]}};
                }
            }

            foreach $tmpName2 (@$tmpName)
            {
                if($firstDestFlag == 0)
                {
                    $bwEventCommand = $bwEventCommand . $addrNodeMapping{$destSeen[$tmpName2]};
                    $firstDestFlag = 1;
                }
                else
                {
                    $bwEventCommand = $bwEventCommand . "," . $addrNodeMapping{$destSeen[$tmpName2]};
                }


# 		my $delayEventCommand = "$tevc $elabMap{$addrNodeMapping{$sourceName}} modify DEST=" . $addrNodeMapping{$destSeen[$tmpName2]};
                my $delayEventCommand = "$tevc ".$elabMap{$addrNodeMapping{$destSeen[$tmpName2]}}." modify DEST=" . $addrNodeMapping{$destSeen[$tmpName2]}." SRC=".$addrNodeMapping{$sourceName};

                $delayEventCommand = $delayEventCommand . " " . "DELAY=" . ($delayMap{$addrNodeMapping{$sourceName}}{$addrNodeMapping{$destSeen[$tmpName2]}});
# Execute the delay event command.
                print "EXECUTE $delayEventCommand\n";
                `$delayEventCommand`;
            }
            $bwEventCommand = $bwEventCommand . " " . "BANDWIDTH=" . $maxBw;
# Execute the event to set the bandwidth for this equivalence class.
            print "EXECUTE $bwEventCommand\n";
            `$bwEventCommand`;
        }

# Create and send events for all the loner dest nodes reachable from this source node.
        for($i = 0; $i < $numDests; $i++)
        {
            my $lonerFlag = 0;
            for($j = 0; $j < $numDests; $j++)
            {
                if($adjMatrix[$i][$j] == 1)
                {
                    $lonerFlag = 1;
                    last;
                }
            }

            if($lonerFlag == 0)
            {
                my $bwEventCommand = "$tevc $elabMap{$addrNodeMapping{$sourceName}} modify DEST=";
                $bwEventCommand = $bwEventCommand . $addrNodeMapping{$destSeen[$i]};
                $bwEventCommand = $bwEventCommand . " BANDWIDTH=".$bwMap{$addrNodeMapping{$sourceName}}{$addrNodeMapping{$destSeen[$i]}};

# Execute the event to set the bandwidth for this path.
                print "EXECUTE: $bwEventCommand\n";
                `$bwEventCommand`;

                my $delayEventCommand = "$tevc ".$elabMap{$addrNodeMapping{$destSeen[$i]}}." modify DEST=" . $addrNodeMapping{$destSeen[$i]}." SRC=".$addrNodeMapping{$sourceName};

                $delayEventCommand = $delayEventCommand . " " . "DELAY=" . ($delayMap{$addrNodeMapping{$sourceName}}{$addrNodeMapping{$destSeen[$i]}});

# Execute the delay event command.
                print "EXECUTE: $delayEventCommand\n";
                `$delayEventCommand`;
            }
        }

    }
    print "\n\n";
}

closedir(logsDirHandle);

# Check whether the transitive property has been
# violated in any equivalence classes - If it has been
# correct it (for now) by assuming that all the nodes
# have the transitive property wrt shared bottlenecks.
sub CheckSanity()
{
    $retVal = 0;
    foreach $equivSet (@equivClasses)
    {
        foreach $classElement ( @$equivSet ) 
        {
            foreach $secondIter (@$equivSet)
            {
                if($classElement != $secondIter)
                {
                    if($adjMatrix[$classElement][$secondIter] != 1)
                    {
                        print "WARNING: $destSeen[$classElement] $destSeen[$secondIter] violates transitive property\n";
                        $adjMatrix[$classElement][$secondIter] = 1;
                        $retVal = -1;
                    }
                }
            }
        }
    }

    return $retVal;
}

sub CheckIntersection()
{
    ($index1, $index2) = @_;
    @array1 = @{ $equivClasses[$index1]};
    @array2 = @{ $equivClasses[$index2]};
    foreach $arrayMember (@array1)
    {
        foreach $checkVal (@array2)
        {
            if($arrayMember == $checkVal)
            {
                return 1;
            } 
    
        }
    }
    return 0;
}

sub MergeClasses()
{
    ($index1, $index2) = @_;
    @secondArray = @{ $equivClasses[$index2] };
    $intersectionFlag = 0;

    foreach $tmpVal (@secondArray)
    {
        foreach $arrayMember (@{ $equivClasses[$index1] })
        {
            if($arrayMember == $tmpVal)
            {
                $intersectionFlag = 1;
                last;
            }
        }

        if($intersectionFlag == 0)
        {
            push(@{ $equivClasses[$index1]}, $tmpVal);
        }
    }
}

sub CheckVariance()
{
    open(HANDLE, $_[0]);

    my @delayArray1 = (), @delayArray2 = ();

    while($line = <HANDLE>)
    {
        if($line =~ /^(\d*)\s(\d*)\s(\d*)\s(\d*)/)
        {
            push(@delayArray1, $1);
            push(@delayArray2, $3);
        }
    }
    close(HANDLE);

    my $delay1Avg = 0, $delay2Avg = 0;
    my $delay1Variance = 0, $delay2Variance = 0;

# Calculate the avg delay for both the paths.
    for($i = 0; $i <= $#delayArray1; $i++)
    {
        $delay1Avg = $delay1Avg + $delayArray1[$i];
    }
    $delay1Avg = $delay1Avg/($#delayArray1 + 1);

    for($i = 0; $i <= $#delayArray2; $i++)
    {
        $delay2Avg = $delay2Avg + $delayArray2[$i];
    }
    $delay2Avg = $delay2Avg/($#delayArray2 + 1);

    # Get the variance for each path.
    for($i = 0; $i <= $#delayArray1; $i++)
    {
        $delay1Variance = $delay1Variance + ($delayArray1[$i] - $delay1Avg)*($delayArray1[$i] - $delay1Avg);
    }
    $delay1Variance = sqrt($delay1Variance);

    for($i = 0; $i <= $#delayArray2; $i++)
    {
        $delay2Variance = $delay2Variance + ($delayArray2[$i] - $delay2Avg)*($delayArray2[$i] - $delay2Avg);
    }
    $delay2Variance = sqrt($delay2Variance);

    my @retArray = ();
    push(@retArray, $delay1Variance);
    push(@retArray, $delay2Variance);

    return @retArray;
}
