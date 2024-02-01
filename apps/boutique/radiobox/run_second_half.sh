#!/bin/bash

svc_names=("recommendationservice" "checkoutservice" "frontend")
num_svcs=3

for ((svc_id=0; svc_id < num_svcs; svc_id++)); do
	name=${svc_names[$svc_id]}
	cpu=$(($svc_id + 1))
	log=$name.log
	taskset -c $cpu ./$name/run.sh $cpu > $log 2>&1 &
	echo "Started $name, logging to $log"
	sleep 0.5
done