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

die "Usage: perl sharedBottle.pl proj_name exp_name newProj_name newExp_name initial_conditions.txt(Optional)"
if($#ARGV < 3);

$projName = $ARGV[0];
$expName = $ARGV[1];
$newProjName = $ARGV[2];
$newExpName = $ARGV[3];
$initialConditionsFilename = $ARGV[4];

$logsDir = "/proj/$projName/exp/$expName/logs/dump";


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
    if( (-d $logsDir . "/" . $sourceName ) && $sourceName ne "." && $sourceName ne ".." )
    {

        my @destLists;
# Then search for all possible destinations for 
# a particular source.
        opendir(sourceDirHandle, $logsDir . "/" . $sourceName);

        foreach $destOne (readdir(sourceDirHandle))
        {
            if( (-d $logsDir . "/" . $sourceName . "/" . $destOne) && $destOne ne "." && $destOne ne ".." )
            {

# Inside each destination directory, look for 
# all possible second destinations.
                opendir(destDirHandle, $logsDir . "/" . $sourceName . "/" . $destOne);

                foreach $destTwo (readdir(destDirHandle))
                {
                    if( (-d $logsDir . "/" . $sourceName . "/" . $destOne . "/" . $destTwo) && $destTwo ne "." && $destTwo ne ".." )
                    {
                        $fullPath = "$logsDir/$sourceName/$destOne/$destTwo";
# Run Rubenstein's code on the ".filter" files
# inside the second destination directory.
                        `perl /proj/tbres/duerig/testbed/pelab/bw-bottleneck/dump2filter.pl $fullPath`;
                        $DansScript = "/proj/tbres/duerig/filter/genjitco.FreeBSD";
                        $filterFile1 = $fullPath . "/" . "source.filter";
                        $filterFile2 = $fullPath . "/" . "dest1.filter";
                        $filterFile3 = $fullPath . "/" . "dest2.filter";

                        if (!(-r $filterFile1) || !(-r $filterFile2) || !($filterFile3))
                        {
                            print "Missing file. Cannot process $fullPath\n";
                            continue;
                        }
                        $sharedBottleneckCheck = $DansScript ." ". $filterFile1
                            ." ". $filterFile2 ." ". $filterFile3;

                        print "EXECUTE: $sharedBottleneckCheck\n";
                        my @scriptOutput = ();
                        @scriptOutput = `$sharedBottleneckCheck | tail -n 2`;

#			$scriptOutput[0] = "last CHANGE was CORRELATED, corr case: 30203 pkts, test case: 30203 pkts";
#			$scriptOutput[1] = "testingabcdef";

# "CORRELATED" means that these two nodes have
# a shared bottleneck.

                        if($scriptOutput[$#scriptOutput - 1] =~ /last CHANGE\s(\w*)\s(\w*)\,[\w\d\W\:\,]*/)
                        {
                            print "For source $sourceName: Comparing $destOne $destTwo: $scriptOutput[$#scriptOutput]";
                            if($2 eq "CORRELATED")
                            {
                                push(@{ $bottleNecks{$sourceName} },$destOne . ":" . $destTwo);
                                push(@destLists,$destOne);
                                push(@destLists,$destTwo);
                                print "CORRELATED\n\n";

                            }
                            elsif($2 eq "UNCORRELATED")
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
# For debugging.

        $retVal = &checkSanity(@equivClasses, @adjMatrix);

        if($retVal != 0)
        {
            print "WARNING:($sourceName) Transitive property has been violated.\n";
        }
#	print "(Debugging) Equivalence classes - $sourceName\n";
#foreach $tmpName (@equivClasses)
#	{
#	    foreach $tmpName2 (@$tmpName)
#	    {
#		print $destSeen[$tmpName2] . " ";
#	    }
#	    print "\n";
#
#	}

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
}

closedir(logsDirHandle);

# Check whether the transitive property has been
# violated in any equivalence classes - If it has been
# correct it (for now) by assuming that all the nodes
# have the transitive property wrt shared bottlenecks.
sub checkSanity()
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
