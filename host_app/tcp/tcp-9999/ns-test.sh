#!/bin/bash

ip link add tap1 type veth peer name tap2

for i in `seq 1 2`;do
    ip netns add ns$i
    ip link set tap$i netns ns$i
    cmd="ip netns exec ns$i"
    ${cmd} ip addr add 192.168.$i.2/24 brd 192.168.$i.255 dev tap$i
    ${cmd} ip link set lo up
    ${cmd} ip link set tap$i up
    ${cmd} ip route add default via 192.168.$i.1 dev tap$i
    ${cmd} ip neigh add 192.168.$i.1 lladdr ff:ff:ff:ff:ff:ff dev tap$i
done
