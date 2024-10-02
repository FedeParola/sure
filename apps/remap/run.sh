#!/bin/bash

eval qemu-system-x86_64 \
	-m 8M \
	-nographic \
	-vga none \
	-net none \
	-kernel "$(dirname $0)/build/remap_qemu-x86_64" \
	-enable-kvm \
	-cpu host,migratable=no \
        -append \""$@"\"
