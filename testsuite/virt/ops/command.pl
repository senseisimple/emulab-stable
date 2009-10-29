#!/usr/bin/perl

# Usage: command.pl <command> <args>

# Begin executing command
$commandPid = fork();
if ($commandPid == 0)
{
  # Command Child
  exec(@ARGV);
}
else
{
  # Parent
  $readPid = fork();
  if ($readPid == 0)
  {
    # Read Child
    $readResult = undef;
    $readBuffer = 0;
    while (! defined($readResult) || $readResult != 0)
    {
      $readResult = sysread(STDIN, $readBuffer, 1);
    }
  }
  else
  {
    # Parent
    $deadPid = wait();
    if ($deadPid != $commandPid)
    {
	kill('KILL', $commandPid);
    }
    if ($deadPid != $readPid)
    {
	kill('KILL', $readPid);
    }
  }
}

