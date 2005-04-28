The camera_data directory contains camera calibration files and data.

----------------------------------------------------------------
	GNU Makefile targets

figs: Does just the first paper figure, and some more composite figures.
  camera1_blend8_tris_mag50.eps # Camera 1 only, grid pts, error vecs, tris.
  all_mag50.eps                 # All cameras "before" (cos dewarp only.)
  all_blend8_mag50.eps          # All cameras "after" (pts and error vecs.)
  all_blend8_tris_mag50.eps     # All "after", (pts, vecs, and triangles.)
show-figs (default): Runs gv on the above figs.
      You can use "killall gv", or just type "q" in each gv to quit.

plots: Does analysis and plotting for all raw cosine-dewarped data.
show-plots: Runs gv on the plots, both per-camera and composite.

blend8: Pushes the raw data through error blending, both analysis and plots.
show-blend8: gv.

blend8-tris: Adds error cancellation blending triangles to the plots.
show-blend8-tris: gv.

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

*_mag*.eps - Gnuplot figures, corresponding to the analysis_* files.

camera*_calpts - Calibration points in mezzanine.opt format.

*.{grid_points,error_lines} - Plot data in gnuplot format.

*.tri_lines - Per-camera Error cancellation blending triangles for gnuplot.
camera*.tri_lines_world - Ditto, in global coords for all_blend8_tris_mag50.eps.

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

