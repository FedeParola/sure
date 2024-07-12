#!/bin/bash

if [ $# -ne 3 ]; then
        echo "usage: $0 <url> <time> <clients>"
	exit 1
fi

url=$1
time=$2
clients=$3
ncpus=$(nproc)
if [ $ncpus -lt $clients ]; then
        workers=$ncpus
else
        workers=$clients
fi

echo "Running $clients clients on $workers workers"

locust --worker --processes $workers &
locust --master --headless -H $url --csv res -t $time -u $clients -r $clients
