#
# EMULAB-COPYRIGHT
# Copyright (c) 2010 University of Utah and the Flux Group.
# All rights reserved.
#
cd stub
gmake
cd ../libnetmon
gmake
cd ../magent
gmake
cd ../..
tar czvf /proj/tbres/duerig/pelab.tar.gz pelab
