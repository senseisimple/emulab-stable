/usr/testbed/bin/tevc -e tbres/pelab now elabc-elab0 modify dest=10.0.0.2 delay=5
/usr/testbed/bin/tevc -e tbres/pelab now elabc-elab1 modify dest=10.0.0.1 delay=5
/usr/testbed/bin/tevc -e tbres/pelab now elabc-elab0 modify dest=10.0.0.2 bandwidth=100000
/usr/testbed/bin/tevc -e tbres/pelab now elabc-elab1 modify dest=10.0.0.1 bandwidth=100000
/usr/testbed/bin/tevc -e tbres/pelab now elabc-elab0 modify dest=10.0.0.2 lpr=0.0
/usr/testbed/bin/tevc -e tbres/pelab now elabc-elab1 modify dest=10.0.0.1 lpr=0.0
#/usr/testbed/bin/tevc -e tbres/pelab now planetc-planet0 modify dest=10.1.0.2 delay=5
#/usr/testbed/bin/tevc -e tbres/pelab now planetc-planet1 modify dest=10.1.0.1 delay=5
#/usr/testbed/bin/tevc -e tbres/pelab now planetc-planet0 modify dest=10.1.0.2 bandwidth=10000
#/usr/testbed/bin/tevc -e tbres/pelab now planetc-planet1 modify dest=10.1.0.1 bandwidth=10000
#/usr/testbed/bin/tevc -e tbres/pelab now planetc-planet0 modify dest=10.1.0.2 lpr=0.0
#/usr/testbed/bin/tevc -e tbres/pelab now planetc-planet1 modify dest=10.1.0.1 lpr=0.0

/usr/testbed/bin/tevc -e tbres/pelab now rlink-router0 modify dest=10.4.0.1 delay=$1
/usr/testbed/bin/tevc -e tbres/pelab now rlink-router1 modify dest=10.1.0.1 delay=$1
/usr/testbed/bin/tevc -e tbres/pelab now rlink-router0 modify dest=10.4.0.1 bandwidth=$2
/usr/testbed/bin/tevc -e tbres/pelab now rlink-router1 modify dest=10.1.0.1 bandwidth=$2
