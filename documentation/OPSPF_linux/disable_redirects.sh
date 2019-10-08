#!/bin/bash

sysctl -p

sysctl net.ipv4 | egrep "(Ethernet|all|default)" | grep "accept_redirects" | awk '{print $1}'| while read p; 
do
    sysctl -w $p=0
done
