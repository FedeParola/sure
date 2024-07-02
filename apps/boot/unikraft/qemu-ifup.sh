#!/bin/bash

# TAP interface will be passed in $1
ip link set $1 up
brctl addif br0 $1
