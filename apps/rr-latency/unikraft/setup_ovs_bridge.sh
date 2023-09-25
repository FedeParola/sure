#!/bin/bash

sudo ip link set br0 down
sudo brctl delbr br0
sudo ovs-vsctl del-br br0
sudo ovs-vsctl add-br br0 -- set bridge br0 datapath_type=netdev
sudo ovs-vsctl add-port br0 vhost-user-0 -- set Interface vhost-user-0 type=dpdkvhostuser
sudo ovs-vsctl add-port br0 vhost-user-1 -- set Interface vhost-user-1 type=dpdkvhostuser
sudo ovs-vsctl set Open_vSwitch . other_config:pmd-cpu-mask=0x4