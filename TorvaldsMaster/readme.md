# VirtioNet test research

## KUnit test

### Commands

- `./tools/testing/kunit/kunit.py run "virt*"` __-__ run all kunit tests that starts with `virt`

- `./tools/testing/kunit/kunit.py run --arch=x86_64` __-__ run on other architectures using qemu

- `./tools/testing/kunit/kunit.py run --arch=s390 --cross_compile=s390x-linux-gnu` __-__ specify toolchain for compilation

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


### Changed files

- [drivers/net/Kconfig](drivers-net-Kconfig)

- [drivers/net/Makefile](drivers-net-Makefile)

- [drivers/net/virtio_net.c](virtio_net.c)

- [include/linux/virtio_net.h](virtio_net.h)

- [drivers/net/virtio_test.c](virtio_test.c)

-----------------

- [drivers/base/power/qos-test.c](drivers-base-power-qos-test.c)

- [drivers/base/test/Kconfig](drivers-base-test-Kconfig)

-----------------

- [lib/slub_kunit.c](lib-slub_kunit.c)

- [lib/Kconfig.debug](lib-Kconfig.debug)

-----------------

- [net/core/dev_addr_lists_test.c](net-core-dev_addr_lists_test.c)

- [net/Kconfig](net-Kconfig)