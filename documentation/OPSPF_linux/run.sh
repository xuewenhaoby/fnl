#!/bin/bash

#---------------------------------
# compile the OPSPF source file
#---------------------------------

# compile
echo -e "\033[36mcompile the source file!\033[34m" # Dark Green to Blue
make

# when compile failed
if [ $? != 0 ]
then
    echo -e "\033[31make failed!" #! Red
    exit 1
fi

clear
echo -e "\033[36mcompile succeed!\n" #! Dark Green

#----------------------------------
# config the situation
#----------------------------------

# clean the iptables ( mangle , raw , nat , filter , security )
iptables -t mangle --flush

# set the iptables:
# usage:
#      sudo iptables
#		 -A <chain name>  when default { INPUT , OUTPUT , FORWARD }
#                                 when nat { PREROUTING , POSTROUTING }
#		 -p <protocol>
#		 -s <source>
#		 -j <target>
# use NAT: 
#      sudo iptables -A <chain> -t nat -j <DNAT/SNAT> -p <protocol>
#                    --to-destination/--to-source <src/dst address>
iptables -t mangle -A FORWARD -i Ethernet+ -j NFQUEUE --queue-num 0
iptables -t mangle -A INPUT -i Ethernet+ -j NFQUEUE --queue-num 0

# print the current iptables
echo -e "\033[36mThe iptables is:\033[33m" #! Dark Green to Yellow
iptables -nxvL -t mangle
echo -e "\033[0m"

# set the rp_filter
echo -e "\033[36mrp_filter:\033[33m" #! Dark Green to Yellow
sysctl -w net.ipv4.conf.all.rp_filter=2
sysctl -w net.ipv4.conf.all.arp_filter=1
sysctl net.ipv4 | grep "\.rp_filter"
echo -e "\033[0m"

#-------------------------------------------
# run the OPSPF protocol
#-------------------------------------------


echo -e "\033[32m" #! Green
sudo ./bin/OPSPF
echo -e "\033[0m"

#-------------------------------------------
# after the OPSPF protocol
#-------------------------------------------

# log the data flow in iptables
echo -e "\033[36mLog of iptables:\033[33m" # Dark Green to Yellow
iptables -nxvL -t mangle
echo -e "\033[0m"

# clean the iptables after the OPSPF
iptables -t mangle --flush

# print the status to file ( output.txt )
(
    route -n &&
    echo "" &&
    ifconfig Ethernet1_5
) > output.txt

# reset the routing table
echo ""
echo -e "\033[34mReset the routing table!" #! Blue
./clean.sh
echo -e "\033[0m"
