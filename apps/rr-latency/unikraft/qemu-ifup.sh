#!/bin/bash

# TAP interface will be passed in $1
bridge=br0
guest_device=$1
brctl addbr $bridge
ip link set $bridge up
ip link set $guest_device up
brctl addif $bridge $guest_device