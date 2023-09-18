#!/bin/bash

# Setup namespaces, veth interfaces and bridge for inter-ns communication

sudo ip netns add ns1
sudo ip link add veth1a type veth peer veth1b
sudo ip link set veth1a netns ns1
sudo ip netns exec ns1 ip link set veth1a up
sudo ip netns exec ns1 ip addr add 10.0.0.1/24 dev veth1a

sudo ip netns add ns2
sudo ip link add veth2a type veth peer veth2b
sudo ip link set veth2a netns ns2
sudo ip netns exec ns2 ip link set veth2a up
sudo ip netns exec ns2 ip addr add 10.0.0.2/24 dev veth2a

sudo ip netns add ns3
sudo ip link add veth3a type veth peer veth3b
sudo ip link set veth3a netns ns3
sudo ip netns exec ns3 ip link set veth3a up
sudo ip netns exec ns3 ip addr add 10.0.0.3/24 dev veth3a

sudo brctl addbr br0
sudo brctl addif br0 veth1b
sudo brctl addif br0 veth2b
sudo brctl addif br0 veth3b
sudo ip link set veth1b up
sudo ip link set veth2b up
sudo ip link set veth3b up
sudo ip link set br0 up
