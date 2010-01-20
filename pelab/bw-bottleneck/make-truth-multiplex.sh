perl /proj/tbres/duerig/testbed/pelab/bw-bottleneck/make-multiplex.pl /proj/tbres/duerig/truth 1 2,3,4:5,6 > /proj/tbres/duerig/truth/multiplex-1.txt
perl /proj/tbres/duerig/testbed/pelab/bw-bottleneck/make-multiplex.pl /proj/tbres/duerig/truth 2 1,4:3,5:6 > /proj/tbres/duerig/truth/multiplex-2.txt
perl /proj/tbres/duerig/testbed/pelab/bw-bottleneck/make-multiplex.pl /proj/tbres/duerig/truth 3 1,4:2:5:6 > /proj/tbres/duerig/truth/multiplex-3.txt
perl /proj/tbres/duerig/testbed/pelab/bw-bottleneck/make-multiplex.pl /proj/tbres/duerig/truth 4 1:2,3,5,6 > /proj/tbres/duerig/truth/multiplex-4.txt
perl /proj/tbres/duerig/testbed/pelab/bw-bottleneck/make-multiplex.pl /proj/tbres/duerig/truth 5 1,4:2:3:6 > /proj/tbres/duerig/truth/multiplex-5.txt
perl /proj/tbres/duerig/testbed/pelab/bw-bottleneck/make-multiplex.pl /proj/tbres/duerig/truth 6 1,4:2:3:5 > /proj/tbres/duerig/truth/multiplex-6.txt
