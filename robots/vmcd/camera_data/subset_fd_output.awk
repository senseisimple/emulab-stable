#! /usr/bin/awk -f
# 
#   EMULAB-COPYRIGHT
#   Copyright (c) 2005 University of Utah and the Flux Group.
#   All rights reserved.
# 
# subset_fd_output.awk - Extract a subset of points from file_dumper output.
# 
# Option vars, set with -v var=arg:
#  -v subset=input_* - REQUIRED.  This is a file_dumper point input file name.
# 
# std input : an output_camera* file from file_dumper.
#
# std output: A file_dumper output file containing the header fom the input,
#  and just the point sections listed in the subset file.

BEGIN {	
  getline < subset;		 # Read in the subset points.
  while ($0) {
    sections["section: "$0] = 1; # Remember selected section lines.
    $0="";			 # (getline doesn't clear $0 when it hits EOF.)
    getline < subset;
  }
  header = dumping = 1;		 # Start out by dumping the file header.
}

dumping { print }		 # Print selected lines.

/^+++/ {			 
  if ( selected ) print "";	 # After separator lines at end of selected section.
  dumping = selected = 0;	 # No output until next selected section.
}

/^section: / && sections[$0] {	 # Section line for a selected point.
  if ( !header ) print "+++";	 # Print the prefix separator and section line
  print;
  header = 0; selected = dumping = 1;
}
