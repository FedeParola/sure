#!/bin/bash

eval qemu-system-x86_64 \
	-m 8M \
	-cpu host,migratable=no \
	-nographic \
	-vga none \
	-net none \
	-kernel $(dirname $0)/build/unikraft_qemu-x86_64 \
	-enable-kvm
