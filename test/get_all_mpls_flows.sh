echo " " > /home/fnl/flows.txt
for i in {1..48} 
do  
ovsName="ovs_s"${i}
ovsNum="s"${i}
printf ${ovsName}\\n >> /home/fnl/flows.txt
docker exec ${ovsName} ovs-ofctl dump-flows ${ovsNum} |grep output:100 >> /home/fnl/flows.txt
docker exec ${ovsName} ovs-ofctl dump-flows ${ovsNum} |grep _mpls >> /home/fnl/flows.txt
done

