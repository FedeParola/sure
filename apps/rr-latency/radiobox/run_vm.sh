#!/bin/bash

eval qemu-system-x86_64 \
	-nographic \
	-vga none \
	-net none \
	-kernel ./build/radiobox_qemu-x86_64 \
	-enable-kvm \
	-cpu host \
	-device ivshmem-doorbell,vectors=1,chardev=id \
	-chardev socket,path=/tmp/ivshmem_socket,id=id \
	-object memory-backend-file,size=1M,share=true,mem-path=/dev/shm/h2os_buffers,id=hostmem \
	-device ivshmem-plain,memdev=hostmem \
        -append \""$@"\"