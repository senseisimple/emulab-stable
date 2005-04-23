The camera_data directory contains camera calibration files and data.

----------------------------------------------------------------
	GNU Makefile targets

plots (default): Does all analysis and plotting for the raw data.
show: Runs gv on the plots, both per-camera and composite.
      You can use "killall gv", or just type "q" in each gv to quit.

blend8: Pushes the raw data through error blending, both analysis and plots.
show-blend8: gv.

calpts: Extracts the calibration points in mezzanine.opt format.

----------------------------------------------------------------
	Guide to source files

input_camera[012367] - Grid point specs for each camera (file_dumper input).

input_camera[012367]_cal - Subset spec for the 9 calibration points at
			   the center, edge midpoints, and corner points.

output_camera[012367] - Measured grid points (file_dumper output).

../etc/mezzanine.opt.camera[012367] - Mezzanine options, including dewarp config.
  e.g. for camera 0:

    # The zoom center pixel of the camera.
    dewarp.cameraCenter = (296,201)

    # Height of the iris above the floor in meters.
    dewarp.ocHeight = 2.30

    # Pixels per meter in X and Y.
    dewarp.scaleFactor = (174.0,180.0)

    # Adjust the period of the cosine for dewarping barrel distortion.
    dewarp.warpFactor = 0.870

    # Offset from the zoom point to the nearest grid point.
    dewarp.gridoff = (0.040,0.003)

    # Dewarp calibration points, define error cancellation blending triangles.
    dewarp.wpos[0] = (-1.000, -1.500)
    dewarp.ipos[0] = (154, 427)
   ...
    dewarp.wpos[8] = (1.000, 1.000)
    dewarp.ipos[8] = (455, 41)

../etc/client[012367] - Vmc_client params, global location of each camera
  Offset in meters to camera center, rotation of camera in radians CW.
  e.g. for camera 0:

    -x 5.939 -y 8.499 -z 2.3 -o -1.5707

----------------------------------------------------------------
	Guide to generated files (see makefile for details.)

analysis_{camera*,all} - Output from dump_analyzer on output_camera files.

*_mag*.eps - Gnuplot figures, corresponding to the analysis*

camera*_calpts - Calibration points in mezzanine.opt format

*.{grid_points,error_lines} - Data for gnuplot.

*.tri_lines - Blending triangles for gnuplot.

----------------------------------------------------------------
	Processing camera data gathered at measured grid points

Do this on the machine hosting Mezzanine and the frame grabber for the camera.

    # Some programs are only in the user's build trees.
    set rbuild = /tmp/tbuild.$USER/robots
    ls -l $rbuild/vmcd/file_dumper

    # Do the work in a subdir of vmcd.
    setenv SRCDIR ~/flux/testbed/robots/vmcd
    cd $srcdir/camera_data

    # Pick a camera.
    set cam = 0 cnum = 0
    set cam = 1 cnum = 1
    set cam = 2 cnum = 2
    set cam = 3 cnum = 3
    set cam = 6 cnum = 2
    set cam = 7 cnum = 3

    # Run file_dumper.  The input_camera file specifies the points to be read.
    set mezzvid = /tmp/mezz_instance/dev/video$cnum.ipc
      ls -l $mezzvid
    set dumpInput = input_camera$cam
    set tmpOutput = output_camera$cam.tmp
    set dumpOutput = output_camera$cam
    v $dumpInput $dumpOutput
    sudo rm $dumpOutput
    sudo $rbuild/vmcd/file_dumper -n 30 -i 1 -f $dumpInput -F $tmpOutput $mezzvid

    # Add config params into the dump output file before analysis.
    ./output_config.awk -v mezzopt=../etc/mezzanine.opt.camera$cam \
      -v vmcopt=../etc/client.$cam $tmpOutput > $dumpOutput

