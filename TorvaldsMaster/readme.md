# VirtioNet test research

## KUnit test

### Commands

- `./tools/testing/kunit/kunit.py run "virt*"` __-__ run all kunit tests that starts with `virt`

- `./tools/testing/kunit/kunit.py run --arch=x86_64` __-__ run on other architectures using qemu

- `./tools/testing/kunit/kunit.py run --arch=s390 --cross_compile=s390x-linux-gnu` __-__ specify toolchain for compilation

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