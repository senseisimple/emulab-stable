#!/usr/bin/ruby
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006 University of Utah and the Flux Group.
# All rights reserved.
#

#
# Get pairwise predictions from iplane
#
# To run:
# iplane_query_client iplane.cs.washington.edu 1 iplane_pairwise.rb
#     'space seperated list of IP addresses'
#

require 'iplane'

#
# Get pairwise information for all nodes passed on the command line
#
nodes = ARGV

iplane = IPlane.new

#
# For now, query one-way only - the upper right corner of the NxN matrix
#
nodes.each_index{ |n1|
    (n1 + 1 .. nodes.length() - 1).each{ |n2| 
        puts "Adding path #{nodes[n1]} to #{nodes[n2]}"
        iplane.addPath(nodes[n1],nodes[n2])
    }
}

#
# Run the query on iplane
#
puts "Getting responses..."
responses = iplane.queryPendingPaths
puts "Got Respnses!"

#
# And, simply print the data we care about from the responses
#
responses.each{ |r|
    puts "src=#{r.src} dst=#{r.dst} lat=#{r.lat} predicted?=#{r.predicted_flag}"
}
