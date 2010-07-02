# Usage: run-monitor.sh <pid> <eid> <node-number>

sudo /sbin/sysctl -w net.ipv4.tcp_no_metrics_save=1
sudo /sbin/ip route flush cache

#perl make-multiplex.pl $3 

../libnetmon/netmond -v 3 -u -f 262144 | tee /local/logs/libnetmon.out | python multiplex-monitor.py --fake --initial=/proj/tbres/duerig/truth/init.txt --multiplex=/proj/tbres/duerig/truth/multiplex-$3.txt --stub-ip=10.0.0.$3 --ip=10.0.0.$3 --experiment=$1/$2 --interface=elabc-elab-$3

