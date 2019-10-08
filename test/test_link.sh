ip link add tap1 type veth peer name tap2
for i in `seq 1 2`;do
	ip link set tap$i up
	ovs-vsctl add-port sbr$i tap$i -- set Interface tap$i ofport_request=1
done
