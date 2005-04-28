#!/usr/local/bin/python
# 
#   EMULAB-COPYRIGHT
#   Copyright (c) 2005 University of Utah and the Flux Group.
#   All rights reserved.
# 
import sys
import getopt
import string
import time
import os
import os.path
import random
import re
import math
import worldTransform

# Parse options and (perhaps multiple) filename args.
DEBUG = False
world_coords = False
opts,args = getopt.getopt(sys.argv[1:], 'wd')
for o,a in opts:
    if o == "-w":
        # Apply world_x, world_y, and world_theta from config in input file.
        world_coords = True
        pass
    if o == "-d":
        DEBUG = True
        pass
    pass
if len(args) < 1:
    print "Usage: dump_analyzer.py [-w] [-d] dump_file..."
    sys.exit(1)
    pass

###############
## Structure per file:
##  - read config info
##  - for each section:
##     - read all data per frame
##     - compute average error given the section title (local coords) (and 
##       std dev and variance)
##     - compute the precision over the per-frame measurements for each of
##     - local, global, and radial posit/orient estimates
###############

# Quick and dirty debug.

def debug(msg):
    if DEBUG:
        print "DEBUG: "+str(msg)
        pass
    pass

# Classes
class ObjectData:
    
    def __init__(self,ax,ay,bx,by,wx,wy,wa):

        # Pixel coords of blob centers.
        self.ax = ax
        self.ay = ay
        self.bx = bx
        self.by = by

        if world_coords:
            # Match up grid points with other cameras globally.
            self.wx, self.wy, self.wa = worldTransform.worldTransform(
                options, wx, wy, wa)
        else:
            # Subtract local camera offset to match nominal grid point coords.
            self.wx = wx - options['camera_x']
            self.wy = wy - options['camera_y']
            self.wa = wa
        pass
    
    def toString(self):
        return 'ObjectData=('+\
               'ax='+str(self.ax)+',ay='+str(self.ay)+\
               ',bx='+str(self.bx)+',by='+str(self.by)+\
               ',wx='+str(self.wx)+',wy='+str(self.wy)+',wa='+str(self.wa)+\
               ')'
    
    pass # end of ObjectData

class FrameData:
    
    def __init__(self,id,timestamp,objects):
        self.id = id
        self.timestamp = timestamp
        self.objects = objects
        pass

    def toString(self):
        s = 'FrameData=(n = '+str(self.id)+', t = '+str(self.timestamp)+ \
            ', objects = {'  # +str(self.objects)
        l = len(self.objects)
        for i in range(l):
            s += str(self.objects[i].toString())
            if i<l-1: s += ','
            pass
        s += '}'
        return s

    pass # end of FrameData

class Section:

    def __init__(self,lx,ly,frames):
        if world_coords:
            # Match up nominal grid points with other cameras globally.
            # Add in the local offset from camera center to surveyed grid point.
            self.lx, self.ly, self.lt = worldTransform.worldTransform(
                options, lx + options['camera_x'], ly + options['camera_y'], 0)
        else:
            # Leave the nominal grid point coords alone.
            self.lx = lx
            self.ly = ly
            self.lt = options['world_theta']
            pass

        self.frames = frames
        pass

    def addFrameData(self,frame):
        self.frames.append(frame)
        pass

    def toString(self):
        s = "Section=(lx = "+str(self.lx)+", ly = "+str(self.ly)+ \
            ", frames = ["
        for f in self.frames:
            s += str(f.toString())+', '
            pass
        s +=']'
        return s

    pass # end of Section


# globals
datafile = 0
results = []
framedata = []
line = ""
options = dict({ 'x_offset': 0.0,
                 'y_offset': 0.0,
                 'z_offset': 0.0,
                 'number of frames': 0,
                 'frame interval': 0,
                 'camera_x': 0.0,     # Camera offset, in meters.
                 'camera_y': 0.0,
                 'world_x': 0.0,      # Offset in meters to camera center.
                 'world_y': 0.0,
                 'world_theta': 0.0,  # Rotation of camera in radians CW.
                 })

def mean(list):
    if len(list) == 0:
        return 0.0
    else:
        return sum(list) / len(list)

# Mean-square-error of a deviation list.
def msqerr(dev_list):
    return mean([n*n for n in dev_list])

# Root-Mean-Square: Sqrt of variance of the error between real and expected values.
def rmserr(dev_list):
    return math.sqrt(msqerr(dev_list))

# Standard deviation: square root of the variance of a value list from its mean.
def stddev(list):
    m = mean(list)
    return math.sqrt(msqerr([n-m for n in list]))

def analyze(section):
    # takes a Section, and does crap on it, and puts the result of the crap
    # in the results global, under this section string title:
    debug(section.toString())

    # the local results var:
    sr = dict([])
    sr['valid_frames'] = 0
    sr['invalid_frames'] = 0
    sr['mult_meas_frames'] = 0
    sr['bad_data_frames'] = 0

    temp = dict([])
    temp['ax'] = []
    temp['ay'] = []
    temp['bx'] = []
    temp['by'] = []
    
    temp['wx'] = []
    temp['wx_offset'] = []
    temp['wx_error'] = []

    temp['wy'] = []
    temp['wy_offset'] = []
    temp['wy_error'] = []

    temp['wxy_error'] = []
    temp['wa'] = []
    

    for f in section.frames:
        c = len(f.objects)
        if c > 1:
            sr['mult_meas_frames'] += 1
            pass
        elif c == 1:

            ax = f.objects[0].ax
            ay = f.objects[0].ay
            bx = f.objects[0].bx
            by = f.objects[0].by

            # Filter out bad, glitchy pixel data where the two blobs do not
            # agree in X and Y.  They have to be within about a
            # fiducial-diameter of each other.
            fid = 13                    # Diameter of a fiducial at the center.
            eps = fid*1.5
            ##print "ax=%f, ay=%f, bx=%f, by=%f"%(ax, ay, bx, by)
            if abs(bx-ax) > eps or abs(by-ay) > eps:
                ##print "bad"
                sr['bad_data_frames'] += 1
            else:
                ##print "good"
                sr['valid_frames'] += 1
                temp['ax'].append(ax)
                temp['ay'].append(ay)

                temp['bx'].append(bx)
                temp['by'].append(by)

                temp['wx'].append(f.objects[0].wx)
                xdiff = f.objects[0].wx - section.lx
                temp['wx_offset'].append(xdiff)
                temp['wx_error'].append(math.fabs(xdiff))

                temp['wy'].append(f.objects[0].wy)
                ydiff = f.objects[0].wy - section.ly
                temp['wy_offset'].append(ydiff)
                temp['wy_error'].append(math.fabs(ydiff))

                temp['wxy_error'].append(math.hypot(xdiff,ydiff))
                temp['wa'].append(f.objects[0].wa)
                pass
            pass
        elif c == 0:
            sr['invalid_frames'] += 1
            pass
        pass

    # calc the stats
    sr['mean_ax'] = mean(temp['ax'])
    sr['stddev_ax'] = stddev(temp['ax'])
    sr['mean_ay'] = mean(temp['ay'])
    sr['stddev_ay'] = stddev(temp['ay'])
    
    sr['mean_bx'] = mean(temp['bx'])
    sr['stddev_bx'] = stddev(temp['bx'])
    sr['mean_by'] = mean(temp['by'])
    sr['stddev_by'] = stddev(temp['by'])
    
    sr['mean_wx'] = mean(temp['wx'])
    sr['stddev_wx'] = stddev(temp['wx'])
    sr['mean_wy'] = mean(temp['wy'])
    sr['stddev_wy'] = stddev(temp['wy'])

    sr['mean_wx_offset'] = mean(temp['wx_offset'])
    tr['wx_offsets'].append(sr['mean_wx_offset'])
    sr['stddev_wx_offset'] = stddev(temp['wx_offset'])
    sr['mean_wy_offset'] = mean(temp['wy_offset'])
    tr['wy_offsets'].append(sr['mean_wy_offset'])
    sr['stddev_wy_offset'] = stddev(temp['wy_offset'])

    sr['mean_wx_error'] = mean(temp['wx_error'])
    tr['wx_errors'].append(sr['mean_wx_error'])
    sr['stddev_wx_error'] = stddev(temp['wx_error'])
    sr['mean_wy_error'] = mean(temp['wy_error'])
    tr['wy_errors'].append(sr['mean_wy_error'])
    sr['stddev_wy_error'] = stddev(temp['wy_error'])
    sr['mean_wxy_error'] = mean(temp['wxy_error'])
    sr['rmserr_wxy_error'] = rmserr(temp['wxy_error'])
    sr['stddev_wxy_error'] = stddev(temp['wxy_error'])
    # Accumulate the per-point deviations for total stats.
    tr['wxy_errors'].append(sr['mean_wxy_error'])

    sr['mean_wa'] = mean(temp['wa'])
    sr['stddev_wa'] = stddev(temp['wa'])
    
    # store results
    results.append(["("+str(section.lx)+", "+str(section.ly)+")", sr])
    pass

# set up the regexes
fpnum = "\s*(\-*\d+\.\d+)\s*"
re_config_option_float = re.compile("  \- ([ \w]+):"+fpnum)
re_config_option_int = re.compile("  \- ([ \w]+): (\-*\d+)")
re_config_option_string = re.compile("  \- ([ \w]+): (.*)")
re_section = re.compile("section:\s*\("+fpnum+","+fpnum+"\)")
re_section_sep = re.compile("\+\+\+")
re_frame_title = re.compile("frame (\d+) \(timestamp"+fpnum+"\):")
re_frame_data_line = re.compile("(\[[0-9]+\] )*a\("+fpnum+","+fpnum+"\)\s*"
                                "b\("+fpnum+","+fpnum+"\)\s*"
                                "-- wc\("+fpnum+","+fpnum+","+fpnum+"\)\s*")

# Totals over the data point sections handled in analyze(), print at the end.
tr = dict([])
tr['wx_offsets'] = []              # Bias.
tr['wy_offsets'] = []
tr['wx_errors'] = []               # Deviation.
tr['wy_errors'] = []
tr['wxy_errors'] = []

def doFile(fname):
    # start reading the file:
    datafile = file(fname)

    in_section = False
    in_config = True

    # a Section obj
    current_section = None
    # a FrameData obj
    current_frame = None
    # ObjectData objs
    current_objs = []

    line = datafile.readline()

    while line != "":
        # chop the newline
        line = line.strip('\n')
        #print "line = "+line

        # Ignore null lines and lines commented-out by pound-signs.
        if line == "" or line[0] == '#':
            in_config = False
            pass
        elif in_config:
            # look for the following regexes:
            m1 = re_config_option_float.match(line)
            m2 = re_config_option_int.match(line)
            m3 = re_config_option_string.match(line)
            if m1 != None:
                options[m1.group(1)] = float(m1.group(2))
                pass
            elif m2 != None:
                options[m2.group(1)] = int(m2.group(2))
                pass
            elif m3 != None:
                options[m3.group(1)] = str(m3.group(2))
                pass
            else:
                debug("unrecognizable option line '"+str(line))
                pass
            pass
        elif not in_section:
            # we have to find the next section
            #print "not in section"
            #print "blah = "+line
            m = re_section.match(line)
            if m != None:
                debug("found sect")
                current_section = Section(float(m.group(1)),
                                          float(m.group(2)),
                                          []
                                          )
                in_section = True
                pass
            pass
        elif in_section and (not in_config):
            m1 = re_frame_data_line.match(line)
            m2 = re_frame_title.match(line)
            m3 = re_section_sep.match(line)
            if m1 != None:
                debug("found frame data")
                # The fields in the regex are what we want, after the optional obj num.
                current_objs.append(ObjectData( *[float(d) for d in m1.groups()[1:]] ))
                pass
            elif m2 != None:
                # new frame; if old frame != 0, add it
                debug("found frame title")
                if current_frame != None:
                    current_frame.objects = current_objs
                    current_objs = []
                    current_section.addFrameData(current_frame)
                    pass
                current_frame = FrameData(int(m2.group(1)),
                                          float(m2.group(2)),
                                          []
                                          )
                pass
            elif m3 != None:
                debug("found section sep")
                # end of this section; if there is a current frame,
                #   add current_objs to frame.objects
                if current_frame != None:
                    current_frame.objects = current_objs
                    current_objs = []
                    current_section.addFrameData(current_frame)
                    current_frame = None
                    pass

                #### analyze the section data:
                analyze(current_section)

                current_section = None
                in_section = False
                pass
            pass
        # grab the next line...
        line = datafile.readline()
        pass

    datafile.close()
    pass

# Process each of the files.
for fname in args:
    doFile(fname)

# dump results:
debug(options)
for k,v in results:
    print "section "+str(k)+" results:"
    # individual stats:
    print "  valid_frames = "+str(v['valid_frames'])
    print "  bad_data_frames = "+str(v['bad_data_frames'])
    print "  invalid_frames = "+str(v['invalid_frames'])
    print "  mult_meas_frames = "+str(v['mult_meas_frames'])

    print "  mean_ax = "+str(v['mean_ax'])
    print "  stddev_ax = "+str(v['stddev_ax'])
    print "  mean_ay = "+str(v['mean_ay'])
    print "  stddev_ay = "+str(v['stddev_ay'])

    print "  mean_bx = "+str(v['mean_bx'])
    print "  stddev_bx = "+str(v['stddev_bx'])
    print "  mean_by = "+str(v['mean_by'])
    print "  stddev_by = "+str(v['stddev_by'])

    print "  mean_wx = "+str(v['mean_wx'])
    print "  stddev_wx = "+str(v['stddev_wx'])
    print "  mean_wy = "+str(v['mean_wy'])
    print "  stddev_wy = "+str(v['stddev_wy'])
    print "  mean_wa = "+str(v['mean_wa'])
    print "  stddev_wa = "+str(v['stddev_wa'])

    print "  mean_wx_offset = "+str(v['mean_wx_offset'])
    print "  stddev_wx_offset = "+str(v['stddev_wx_offset'])
    print "  mean_wy_offset = "+str(v['mean_wy_offset'])
    print "  stddev_wy_offset = "+str(v['stddev_wy_offset'])

    print "  mean_wx_error = "+str(v['mean_wx_error'])
    print "  stddev_wx_error = "+str(v['stddev_wx_error'])
    print "  mean_wy_error = "+str(v['mean_wy_error'])
    print "  stddev_wy_error = "+str(v['stddev_wy_error'])
    print "  mean_wxy_error = "+str(v['mean_wxy_error'])
    print "  stddev_wxy_error = "+str(v['stddev_wxy_error'])
    print "  rmserr_wxy_error = "+str(v['rmserr_wxy_error'])
    
#    for k2,v2 in v.iteritems():
#        print "  "+str(k2)+" = "+str(v2)
#        pass

    pass

print "\ntotal results:"
print ""
print "  max_wx_offset = "+str(max(tr['wx_offsets']))
print "  min_wx_offset = "+str(min(tr['wx_offsets']))
print "  mean_wx_offset = "+str(mean(tr['wx_offsets']))
print "  stddev_wx_offset = "+str(stddev(tr['wx_offsets']))
print ""
print "  max_wy_offset = "+str(max(tr['wy_offsets']))
print "  min_wy_offset = "+str(min(tr['wy_offsets']))
print "  mean_wy_offset = "+str(mean(tr['wy_offsets']))
print "  stddev_wy_offset = "+str(stddev(tr['wy_offsets']))
print ""
print "  max_wx_error = "+str(max(tr['wx_errors']))
print "  min_wx_error = "+str(min(tr['wx_errors']))
print "  mean_wx_error = "+str(mean(tr['wx_errors']))
print "  stddev_wx_error = "+str(stddev(tr['wx_errors']))
print ""
print "  max_wy_error = "+str(max(tr['wy_errors']))
print "  min_wy_error = "+str(min(tr['wy_errors']))
print "  mean_wy_error = "+str(mean(tr['wy_errors']))
print "  stddev_wy_error = "+str(stddev(tr['wy_errors']))
print ""
print "  num_pts = "+str(len(tr['wxy_errors']))
print "  max_wxy_error = "+str(max(tr['wxy_errors']))
print "  min_wxy_error = "+str(min(tr['wxy_errors']))
print "  mean_wxy_error = "+str(mean(tr['wxy_errors']))
print "  stddev_wxy_error = "+str(stddev(tr['wxy_errors']))
print "  rmserr_wxy_error = "+str(rmserr(tr['wxy_errors']))
