# VirtioNet test research

## KUnit test

### Commands

- `./tools/testing/kunit/kunit.py run "virt*"` __-__ run all kunit tests that starts with `virt`

- `./tools/testing/kunit/kunit.py run --arch=x86_64` __-__ run on other architectures using qemu

- `./tools/testing/kunit/kunit.py run --arch=s390 --cross_compile=s390x-linux-gnu` __-__ specify toolchain for compilation

- `scripts/decode_stacktrace.sh .kunit/vmlinux .kunit < .kunit/test.log | tee .kunit/decoded.log | ./tools/testing/kunit/kunit.py parse` __-__ get more detailed, advantage is that stacktrace contains filenames of the functions

### Configs

- To run kuint tests for `virtio_net` add configuration in file __`linux/drivers/net/Kconfig`__


		config VIRTIO_NET_TEST
			tristate "Test for virtio_net" if !KUNIT_ALL_TESTS
			depends on KUNIT=y
			select NETDEVICES
			select NET_CORE
			select VIRTIO
			select VIRTIO_NET
			select NET
			default KUNIT_ALL_TESTS


- `Select` will add the dependencies and they will compile with our `VIRTIO_NET_TEST`, also we add our file with test ot the config `VIRTIO_NET_TEST` in __`linux/drivers/net/Makefile`__

		obj-$(CONFIG_VIRTIO_NET_TEST) += virtio_net.o

- Finally enable test configuration in __`linux/.kunit/.config`__ and __`linux/.kunit/.config`__. ***Note*** that we should enable the configuration in both files because, `kunit.py` script parses both the existing `.config` and the `.kunitconfig` files to ensure that `.config` is a superset of `.kunitconfig`

		CONFIG_VIRTIO_NET_TEST=y

- Configs for _kernel address sanitizer_ 

		CONFIG_KASAN=y
		CONFIG_KASAN_VMALLOC=y
		CONFIG_KASAN_INLINE=y
		CONFIG_KASAN_GENERIC=y
		CONFIG_STACKTRACE=y

- Configs for _gdb debugger_

		CONFIG_DEBUG_INFO_DWARF_TOOLCHAIN_DEFAULT=y
		CONFIG_DEBUG_KERNEL=y

### Running with `gcov` tool

- We want __virtio_net.c__ to be profiled with `gcov`, so we add these line in `linux/drivers/net/Makefile`

		GCOV_PROFILE_virtio_net.o := y

- Also to run with gcov we should add appropriate configs to `.kuintconfig` in `linux/.kunit`

		CONFIG_DEBUG_FS=y
		CONFIG_GCOV_KERNEL=y

- Then we just run kuint test using basic run command of kunit tool, with gcov it will take a little more time to compile and run. It may not finish successfuly, in that case try again.

- File `.kunit/drivers/net/virtio_net.gcno` must be created after a successful run.

### Changed files

- [linux/drivers/net/Kconfig](/linux/drivers/net/Kconfig)

- [linux/drivers/net/Makefile](/linux/drivers/net/Makefile)

- [linux/drivers/net/virtio_net.c](/linux/drivers/net/virtio_net.c)

-----------------

- [linux/.kunit/.kunitconfig](/linux/.kuint/.kunitconfig)

-----------------

### Added files

- [linux/drivers/net/virtio_net_test.c](/linux/drivers/net/virtio_net_test.c)

- [linux/drivers/net/virtio_net_test.h](/linux/drivers/net/virtio_net_test.h)

-----------------

### Notes, Kernel, basics

__Classes__

Classes are not a construct that is built into the C programming language; however, it is an easily derived concept. Accordingly, in most cases, every project that does not use a standardized object oriented library (like GNOME’s GObject) has their own slightly different way of doing object oriented programming; the Linux kernel is no exception.

The central concept in kernel object oriented programming is the class. In the kernel, a class is a struct that contains function pointers. This creates a contract between implementers and users since it forces them to use the same function signature without having to call the function directly. To be a class, the function pointers must specify that a pointer to the class, known as a class handle, be one of the parameters. Thus the member functions (also known as methods) have access to member variables (also known as fields) allowing the same implementation to have multiple instances.

A class can be overridden by child classes by embedding the parent class in the child class. Then when the child class method is called, the child implementation knows that the pointer passed to it is of a parent contained within the child. Thus, the child can compute the pointer to itself because the pointer to the parent is always a fixed offset from the pointer to the child. This offset is the offset of the parent contained in the child struct.


