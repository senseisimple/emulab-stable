#!/usr/local/bin/python

import sys
import getopt
import string
import time
import os
import os.path
import random
import re
import math

###############
## Structure:
##  - read config info
##  - for each section:
##     - read all data per frame
##     - compute average error given the section title (local coords) (and 
##       std dev and variance)
##     - compute the precision over the per-frame measurements for each of
##     - local, global, and radial posit/orient estimates
###############

# Quick and dirty debug.
DEBUG = False

def debug(msg):
    if DEBUG:
        print "DEBUG: "+str(msg)
        pass
    pass

# Classes
class ObjectData:
    
    def __init__(self,lx,ly,lt,rx,ry,rt,gx,gy,gt):
        self.lx = lx
        self.ly = ly
        self.lt = lt
        self.rx = rx
        self.ry = ry
        self.rt = rt
        self.gx = gx
        self.gy = gy
        self.gt = gt
        pass

    def toString(self):
        return 'ObjectData=(' + \
               ' lx='+str(self.lx)+',ly='+str(self.ly)+',lt='+str(self.lt)+ \
               ',rx='+str(self.rx)+',ry='+str(self.ry)+',rt='+str(self.rt)+ \
               ',gx='+str(self.gx)+',gy='+str(self.gy)+',gt='+str(self.gt)+ \
               ') '
    
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
        for k,v in self.objects.iteritems():
            s += str(k)+'=>'+str(v.toString())+','
            pass
        s += '}'
        return s

    pass # end of FrameData

class Section:

    def __init__(self,lx,ly,frames):
        self.lx = lx
        self.ly = ly
        # we assume PI/2 for now...
        self.lt = 1.570796
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
results = dict([])
framedata = []
line = ""
options = dict({ 'x_offset': 0.0,
                 'y_offset': 0.0,
                 'z_offset': 0.0,
                 'number of frames': 0,
                 'frame interval': 0,
                 })

def mean(list):
    retval = 0.0
    if len(list) == 0:
        pass
    else:
        sum = 0.0
        for n in list:
            sum += n
            pass
        retval = sum / len(list)
        pass
    return retval
    
def stddev(list):
    m = mean(list)
    deviation_list = []
    for n in list:
        deviation_list.append(n - m)
        pass
    sum_of_squared_deviations = 0.0
    for n in deviation_list:
        sum_of_squared_deviations += math.pow(n,2)
        pass
    stddev = math.sqrt(sum_of_squared_deviations / (len(list) - 1))
    return stddev

def analyze(section):
    # takes a Section, and does crap on it, and puts the result of the crap
    # in the results global, under this section string title:
    debug(section.toString())

    # the local results var:
    sr = dict([])
    sr['valid_frames'] = 0
    sr['invalid_frames'] = 0
    sr['mult_meas_frames'] = 0
    #sr['lx_sum'] = 0.0

    # these are the stats we'll be grabbing, but no point to declaring
    # them here
    #sr['mean_lx'] = 0.0
    #sr['stddev_lx'] = 0.0
    #sr['mean_lx_minus_actual_lx'] = 0.0
    #sr['stddev_lx_minus_actual_lx'] = 0.0
    #sr['mean_ly'] = 0.0
    #sr['stddev_ly'] = 0.0
    #sr['mean_ly_minus_actual_ly'] = 0.0
    #sr['stddev_ly_minus_actual_ly'] = 0.0
    #sr['mean_lt'] = 0.0
    #sr['stddev_lt'] = 0.0
    #sr['mean_lt_minus_actual_lt'] = 0.0
    #sr['stddev_lt_minus_actual_lt'] = 0.0

    #sr['mean_rx'] = 0.0
    #sr['stddev_rx'] = 0.0
    #sr['mean_rx_minus_actual_lx'] = 0.0
    #sr['stddev_rx_minus_actual_lx'] = 0.0
    #sr['mean_ry'] = 0.0
    #sr['stddev_ry'] = 0.0
    #sr['mean_ry_minus_actual_ly'] = 0.0
    #sr['stddev_ry_minus_actual_ly'] = 0.0
    #sr['mean_rt'] = 0.0
    #sr['stddev_rt'] = 0.0
    #sr['mean_rt_minus_actual_lt'] = 0.0
    #sr['stddev_rt_minus_actual_lt'] = 0.0

    temp = dict([])
    temp['lx_minus_actual_lx'] = []
    temp['lx'] = []
    temp['ly_minus_actual_ly'] = []
    temp['ly'] = []
    temp['lt_minus_actual_lt'] = []
    temp['lt'] = []
    
    temp['rx_minus_actual_lx'] = []
    temp['rx'] = []
    temp['ry_minus_actual_ly'] = []
    temp['ry'] = []
    temp['rt_minus_actual_lt'] = []
    temp['rt'] = []
    

    for f in section.frames:
        c = len(f.objects.keys())
        if c > 1:
            sr['mult_meas_frames'] += 1
            pass
        elif c == 1:
            sr['valid_frames'] += 1
            keys = f.objects.keys()
            #sr['lx_sum'] += f.objects[keys[0]].lx
            temp['lx_minus_actual_lx'].append(f.objects[keys[0]].lx - \
                                              section.lx)
            temp['lx'].append(f.objects[keys[0]].lx)
            temp['ly_minus_actual_ly'].append(f.objects[keys[0]].ly - \
                                              section.ly)
            temp['ly'].append(f.objects[keys[0]].ly)
            temp['lt_minus_actual_lt'].append(f.objects[keys[0]].lt - \
                                              section.lt)
            temp['lt'].append(f.objects[keys[0]].lt)
            
            temp['rx_minus_actual_lx'].append(f.objects[keys[0]].rx - \
                                              section.lx)
            temp['rx'].append(f.objects[keys[0]].rx)
            temp['ry_minus_actual_ly'].append(f.objects[keys[0]].ry - \
                                              section.ly)
            temp['ry'].append(f.objects[keys[0]].ry)
            temp['rt_minus_actual_lt'].append(f.objects[keys[0]].rt - \
                                              section.lt)
            temp['rt'].append(f.objects[keys[0]].rt)
            
            pass
        elif c == 0:
            sr['invalid_frames'] += 1
            pass
        pass

    # calc the stats
    sr['mean_lx'] = mean(temp['lx'])
    sr['stddev_lx'] = stddev(temp['lx'])
    sr['mean_lx_minus_actual_lx'] = mean(temp['lx_minus_actual_lx'])
    sr['stddev_lx_minus_actual_lx'] = stddev(temp['lx_minus_actual_lx'])
    sr['mean_ly'] = mean(temp['ly'])
    sr['stddev_ly'] = stddev(temp['ly'])
    sr['mean_ly_minus_actual_ly'] = mean(temp['ly_minus_actual_ly'])
    sr['stddev_ly_minus_actual_ly'] = stddev(temp['ly_minus_actual_ly'])
    sr['mean_lt'] = mean(temp['lt'])
    sr['stddev_lt'] = stddev(temp['lt'])
    sr['mean_lt_minus_actual_lt'] = mean(temp['lt_minus_actual_lt'])
    sr['stddev_lt_minus_actual_lt'] = stddev(temp['lt_minus_actual_lt'])
    
    sr['mean_rx'] = mean(temp['rx'])
    sr['stddev_rx'] = stddev(temp['rx'])
    sr['mean_rx_minus_actual_lx'] = mean(temp['rx_minus_actual_lx'])
    sr['stddev_rx_minus_actual_lx'] = stddev(temp['rx_minus_actual_lx'])
    sr['mean_ry'] = mean(temp['ry'])
    sr['stddev_ry'] = stddev(temp['ry'])
    sr['mean_ry_minus_actual_ly'] = mean(temp['ry_minus_actual_ly'])
    sr['stddev_ry_minus_actual_ly'] = stddev(temp['ry_minus_actual_ly'])
    sr['mean_rt'] = mean(temp['rt'])
    sr['stddev_rt'] = stddev(temp['rt'])
    sr['mean_rt_minus_actual_lt'] = mean(temp['rt_minus_actual_lt'])
    sr['stddev_rt_minus_actual_lt'] = stddev(temp['rt_minus_actual_lt'])
    
    
    # store results
    results["("+str(section.lx)+", "+str(section.ly)+")"] = sr
    pass

# start reading the file:
if sys.argv[1] != "":
    datafile = file(sys.argv[1])
    pass
else:
    usage()
    pass

in_section = False
in_config = True

# set up the regexes
re_config_option_float = re.compile("  \- ([ \w]+): (\-*\d+\.\d+)")
re_config_option_int = re.compile("  \- ([ \w]+): (\-*\d+)")
re_config_option_string = re.compile("  \- ([ \w]+): (.*)")
re_section = re.compile("section:\s*\((\-*\d+\.\d+)\s*,\s*(\-*\d+\.\d+)\s*\)")
re_section_sep = re.compile("\+\+\+")
re_frame_title = re.compile("frame (\d+) \(timestamp (\d+\.\d+)\):")
re_frame_data_line = re.compile("  \- (\w+)\[(\d+)\]:\s*x = (\-*\d+\.\d+), y = (\-*\d+\.\d+), theta = (\-*\d+\.\d+)")

# a Section obj
current_section = None
# a FrameData obj
current_frame = None
# ObjectData objs
current_objs = dict([])

line = datafile.readline()

while line != "":
    # chop the newline
    line = line.strip('\n')
    #print "line = "+line

    if line == "":
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
            #print "m1.group(2) = "+m1.group(2)
            if not current_objs.has_key(int(m1.group(2))):
                current_objs[int(m1.group(2))] = ObjectData(0,0,0,0,0,0,0,0,0)
                pass
            if m1.group(1) == 'local':
                debug("found local: "+str(float(m1.group(3)))+","+ \
                      str(float(m1.group(4)))+","+str(float(m1.group(5))))
                current_objs[int(m1.group(2))].lx = float(m1.group(3))
                current_objs[int(m1.group(2))].ly = float(m1.group(4))
                current_objs[int(m1.group(2))].lt = float(m1.group(5))
                pass
            elif m1.group(1) == 'radial':
                debug("found radial: "+str(float(m1.group(3)))+","+ \
                      str(float(m1.group(4)))+","+str(float(m1.group(5))))
                current_objs[int(m1.group(2))].rx = float(m1.group(3))
                current_objs[int(m1.group(2))].ry = float(m1.group(4))
                current_objs[int(m1.group(2))].rt = float(m1.group(5))
                pass
            elif m1.group(1) == 'global':
                debug("found global: "+str(float(m1.group(3)))+","+ \
                      str(float(m1.group(4)))+","+str(float(m1.group(5))))
                current_objs[int(m1.group(2))].gx = float(m1.group(3))
                current_objs[int(m1.group(2))].gy = float(m1.group(4))
                current_objs[int(m1.group(2))].gt = float(m1.group(5))
                pass
            pass
        elif m2 != None:
            # new frame; if old frame != 0, add it
            debug("found frame title")
            if current_frame != None:
                current_frame.objects = current_objs
                current_objs = dict([])
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
                current_objs = dict([])
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
# finis!

# dump results:

debug(options)

for k,v in results.iteritems():
    print "section "+str(k)+" results:"
    # individual stats:
    print "  valid_frames = "+v['valid_frames']
    print "  invalid_frames = "+v['invalid_frames']
    print "  mult_meas_frames = "+v['mult_meas_frames']
    print "  mean_lx = "+v['mean_lx']
    print "  stddev_lx = "+v['stddev_lx']
    print "  mean_ly = "+v['mean_ly']
    print "  stddev_ly = "+v['stddev_ly']
    print "  mean_lt = "+v['mean_lt']
    print "  stddev_lt = "+v['stddev_lt']
    print "  mean_lx_minus_actual_lx = "+v['mean_lx_minus_actual_lx']
    print "  stddev_lx_minus_actual_lx = "+v['stddev_lx_minus_actual_lx']
    print "  mean_ly_minus_actual_ly = "+v['mean_ly_minus_actual_ly']
    print "  stddev_ly_minus_actual_ly = "+v['stddev_ly_minus_actual_ly']
    print "  mean_lt_minus_actual_lt = "+v['mean_lt_minus_actual_lt']
    print "  stddev_lt_minus_actual_lt = "+v['stddev_lt_minus_actual_lt']
    print "  mean_rx_minus_actual_lx = "+v['mean_rx_minus_actual_lx']
    print "  stddev_rx_minus_actual_lx = "+v['stddev_rx_minus_actual_lx']
    print "  mean_ry_minus_actual_ly = "+v['mean_ry_minus_actual_ly']
    print "  stddev_ry_minus_actual_ly = "+v['stddev_ry_minus_actual_ly']
    print "  mean_rt_minus_actual_lt = "+v['mean_rt_minus_actual_lt']
    print "  stddev_rt_minus_actual_lt = "+v['stddev_rt_minus_actual_lt']


    
#    for k2,v2 in v.iteritems():
#        print "  "+str(k2)+" = "+str(v2)
#        pass
#    pass

