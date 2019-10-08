#!/bin/bash

# 删除之前的配置
tc qdisc del dev eno3 root

# 添加时延和丢包
tc qdisc add dev eno3 root handle 1:0 netem delay 15ms loss 0.01%

# 添加速率控制
tc qdisc add dev eno3 parent 1:1 handle 10: tbf rate 10mbit buffer 5000 latency 4ms