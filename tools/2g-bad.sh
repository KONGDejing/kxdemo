#!/bin/sh
tc qdisc add dev wlo1 root handle 1: htb default 1
tc class add dev wlo1 parent 1: classid 1:1 htb rate 64kbps ceil 128kbps
tc qdisc add dev wlo1 parent 1:1 netem delay 800ms 200ms distribution normal loss 15% duplicate 2% corrupt 2%
sudo tc -s qdisc show dev wlo1