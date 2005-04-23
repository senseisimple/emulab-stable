#! /usr/bin/awk -f
# 
#   EMULAB-COPYRIGHT
#   Copyright (c) 2005 University of Utah and the Flux Group.
#   All rights reserved.
# 
# output_config.awk - Add config data into the dump output file before analysis.
#
# Add configuration params into the output file to be analyzed.  The local offset
# between camera and grid is the Mezzanine dewarp.gridoff option.  The world
# coordinate offset and rotation of the camera come from the vmc_client args.
#
# std input: an output_camera* file.
#
# Required var args, set with -v var=arg:
#  -v mezzopt=../etc/mezzanine.opt/camera0 - Source of Mezzanine dewarp params. 
#  -v vmcopt=../etc/client.0 - Source of vmc_client config params.
#
# Optional var args:
#  -v debug=1
# 
# std output: The input stream, with camera_ and world_ config options added.

BEGIN{
    # The param file args are required.
    if ( ! mezzopt || ! vmcopt ) {
        msg = "usage: output_config -v mezzopt=... -v vmcopt=... infile >outfile";
        print msg >"/dev/stderr";
        exit 1;
    }

    # Parse the mezzanine.opt.camera* dewarp.gridoff option.
    "grep dewarp.gridoff " mezzopt | getline gridoff;
    cx = gensub(".*\\( *([^, ]+).*", "\\1", 1, gridoff);
    cy = gensub(".*, *([^ )]+).*", "\\1", 1, gridoff);
    if ( debug ) print "cx=" cx ", cy=" cy > "/dev/stderr";

    # Parse the vmc_client client.* dash option args.
    getline clientargs <vmcopt
    wx = gensub(".*-x +([^ ]+).*", "\\1", 1, clientargs);
    wy = gensub(".*-y +([^ ]+).*", "\\1", 1, clientargs);
    cz = gensub(".*-z +([^ ]+).*", "\\1", 1, clientargs);
    if ( index(cz, " ") ) cz = 0;
    wo = gensub(".*-o +([^ ]+).*", "\\1", 1, clientargs);
    if ( index(wo, " ") ) wo = 0;
    if ( debug ) print "wx=" wx ", wy=" wy ", cz=" cz ", wo=" wo > "/dev/stderr";
}

# The config section ends at the first +++ separator, the rest is passed through.
$1=="+++"{ pass=1 }
pass { print; next }

# Ignore any previous camera_ or world_ config values.
match($2,"camera_|world_"){ next }

# Insert our config options after the frame interval line.
$2=="frame" && $3=="interval:" {
    print;
    printf "  - camera_x: %.3f\n", cx;
    printf "  - camera_y: %.3f\n", cy;
    printf "  - camera_z: %f\n", cz;
    printf "  - world_x: %.3f\n", wx;
    printf "  - world_y: %.3f\n", wy;
    printf "  - world_theta: %f\n", wo;
    next;
}

# Print the config section.
{ print }
