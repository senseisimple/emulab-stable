#!/usr/bin/perl

if (scalar(@ARGV) < 6)
{
    print STDERR "Usage: master-virt.pl <proj> <exp> <\"type\" | \"name\">\n"
	        ."    <typeName | nodeName> <runPath> <resultPath>\n"
		."    <pairCount [...]>\n";
}

$proj = shift(@ARGV);
$exp = shift(@ARGV);
$select = shift(@ARGV);
$name = "";
$type = "";
if ($select eq "type")
{
    $type = shift(@ARGV);
}
else
{
    $name = shift(@ARGV);
}
$runPath = shift(@ARGV);
$resultPath = shift(@ARGV);
@pairList = @ARGV;
$pairString = join(" ", @pairList);

%replacement = ("PROJ" => $proj,
		"EXP" => $exp,
		"SELECT" => $select,
		"NAME" => $name,
		"TYPE" => $type,
		"RUN_PATH" => $runPath,
		"RESULT_PATH" => $resultPath,
		"PAIRS" => $pairString);

open SCRIPT_IN, "<run-virt.t";
open SCRIPT_OUT, ">final-run-virt.t";
while ($line = <SCRIPT_IN>)
{
    while (my ($key, $value) = each %replacement)
    {
	$line =~ s/\@$key\@/$value/g;
    }
    print SCRIPT_OUT $line;
}

close SCRIPT_IN;
close SCRIPT_OUT;

print("Running tests $pairString with experiment $proj/$exp\n");
system("./tbts final-run-virt.t");

