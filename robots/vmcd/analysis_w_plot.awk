#! /usr/bin/awk -f
# analysis_w_plot.awk - Make gnuplot data files from dump_analyzer output.
# 
# std input : an analysis_camera* file.
# Option vars, set with -v var=arg:
#  -v file=base. - Base name of output files, null string by default.
#  -v mag=10 - Magnification factor on error line length, 1 by default.
# 
# output: file.grid_points - The section title world coordinates.
#         file.error_lines - Lines from the grid points to mean_wx, mean_wy.
#
# gnuplot commands:
#     plot "grid_points" with points
#     plot "grid_points" with points ps 2, "error_lines" with lines lw 3

BEGIN { pfile = file "grid_points"
	efile = file "error_lines"
	if ( !mag ) mag=1; print mag
}

/^section /{ gsub("[(,)]","")
	     x=$2; y=$3; 
	     print x, y > pfile
	     next
}
/^  mean_wx /{ mx=x+($3-x)*mag
	       next
}
/^  mean_wy /{ my=y+($3-y)*mag
	       printf "%f %f\n%f %f\n\n", x, y, mx, my > efile
	       next
}
