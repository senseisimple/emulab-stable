
all: testbed.tcl labels.cse
	wish testbed.tcl | ./labels_form.tcl
	cat tb[0-9].ps tb1[0-9].ps > tb.ps
	./make_labels_sf
	cat labelsf[0-9].cse.ps > labelsf.ps
