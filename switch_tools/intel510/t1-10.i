# This ir file adds a system vlan - connects every interface to every
# other one
start vlan
net1 tbpc01:0 tbpc02:0 tbpc03:0 tbpc04:0 tbpc05:0 tbpc06:0 tbpc07:0 tbpc08:0 tbpc09:0 tbpc10:0
net2 tbpc01:1 tbpc02:1 tbpc03:1 tbpc04:1 tbpc05:1 tbpc06:1 tbpc07:1 tbpc08:1 tbpc09:1 tbpc10:1
net3 tbpc01:2 tbpc02:2 tbpc03:2 tbpc04:2 tbpc05:2 tbpc06:2 tbpc07:2 tbpc08:2 tbpc09:2 tbpc10:2
net4 tbpc01:3 tbpc02:3 tbpc03:3 tbpc04:3 tbpc05:3 tbpc06:3 tbpc07:3 tbpc08:3 tbpc09:3 tbpc10:3
end vlan
