sudo /usr/sbin/tcpdump -n -i eth1 "!(dst host 10.0.0.1) && dst net 10" | python monitor.py
