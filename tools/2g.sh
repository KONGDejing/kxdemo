#!/bin/sh
sudo tc qdisc add dev wlo1 root handle 1: htb default 1
sudo tc class add dev wlo1 parent 1: classid 1:1 htb rate 1mbit ceil 2mbit
sudo tc qdisc add dev wlo1 parent 1:1 netem delay 300ms loss 2%
sudo tc -s qdisc show dev wlo1