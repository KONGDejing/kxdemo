#!/bin/sh
tc qdisc add dev wlo1 root handle 1: htb default 1
tc class add dev wlo1 parent 1: classid 1:1 htb rate 128kbps ceil 256kbps
tc qdisc add dev wlo1 parent 1:1 netem delay 500ms 100ms distribution normal loss 10% duplicate 1% corrupt 1%
sudo tc -s qdisc show dev wlo1