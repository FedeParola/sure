#!/bin/bash

start=$(awk '/^now/ {print $3; exit}' /proc/timer_list)
stop=$(docker run --rm boot)
echo $(($stop-$start))
