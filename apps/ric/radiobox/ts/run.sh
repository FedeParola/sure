#!/bin/bash

eval $HOME/src/qemu/build/qemu-system-x86_64 \
	-nographic \
	-vga none \
	-net none \
	-kernel "$(dirname $0)/build/ts_qemu-x86_64" \
	-enable-kvm \
	-cpu host \
	-m 512 \
	-device ivshmem-doorbell,vectors=1,chardev=id \
	-chardev socket,path=/tmp/ivshmem_socket,id=id \
	-object memory-backend-file,size=2M,share=true,mem-path=/dev/hugepages/unimsg_buffers,id=hostmem \
	-device ivshmem-plain,memdev=hostmem \
        -append \""$@"\"