#!/bin/bash

# svc_names=("adservice" "emailservice" "paymentservice" "currencyservice"
# 	   "shippingservice" "productcatalogservice" "cartservice"
# 	   "recommendationservice" "checkoutservice" "frontend")
# num_svcs=10

# for ((svc_id=0; svc_id < 1; svc_id++)); do
# 	name=${svc_names[$svc_id]}
# 	cpu=$(($svc_id + 1))
# 	log=$name.log
# 	eval nohup taskset -c $cpu sudo ./$name/run.sh $cpu > $log &
# 	echo "Started $name, logging to $log"
# 	sleep 0.5
# done

eval ./test &