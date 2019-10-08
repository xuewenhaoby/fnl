#!/bin/bash
#make clean
make clean

#clean iptables
iptables -F

#reset ETHERNET1_5
ifconfig Ethernet1_5 190.0.133.2 netmask 255.255.255.0

#clean routes
ip route | awk '{if($3!="eth1") {cmd="ip route del "$1; system(cmd)}}'

#add routes
SUBNETLIST=("190.0.1.0/24" "190.0.2.0/24" "190.0.3.0/24" "190.0.4.0/24" "190.0.133.0/24")
for (( i=1; i<6; i=i+1 ))
do
	ip route add ${SUBNETLIST[$i-1]} dev Ethernet1_$i
done

NXTHOPLIST=("190.0.1.40" "190.0.2.40" "190.0.3.40" "190.0.4.40" "190.0.133.40")
PEERLIST=("190.0.1.2" "190.0.2.1" "190.0.3.2" "190.0.4.2" "190.0.133.2")

for (( i=1; i<6; i=i+1 ))
do
	ip route add ${PEERLIST[$i-1]} via ${NXTHOPLIST[$i-1]}
done
