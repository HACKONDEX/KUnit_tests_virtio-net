# Kernel-Virtio-Research
Linux Kernel VirtioNet test research

# Virtio-Net test research

## KUnit test

### Commands

- `./tools/testing/kunit/kunit.py run "virt*"` __-__ run all kunit tests that starts with `virt`

- `./tools/testing/kunit/kunit.py run --arch=x86_64` __-__ run on other architectures using qemu

- `./tools/testing/kunit/kunit.py run --arch=s390 --cross_compile=s390x-linux-gnu` __-__ specify toolchain for compilation

- `scripts/decode_stacktrace.sh .kunit/vmlinux .kunit < .kunit/test.log | tee .kunit/decoded.log | ./tools/testing/kunit/kunit.py parse` __-__ get more detailed, advantage is that stacktrace contains filenames of the functions

- `qemu-system-x86_64 -kernel ./arch/x86_64/boot/bzImage -initrd ramdisk.img -m 4G` __-__ run kernel image in qemu

- `mkinitramfs -o ramdiks.img` __-__ create ramfs

- `unmkinitramfs <file> <target directory>` __-__ unwrap ramfs into a dircetory

- `mount -t debugfs none /sys/kernel/debug` __-__ mount debugfs and get the coverage gdna gdno files

- `lcov -t "virtio_test" -o coverage.info -c -d gcovFiles` __-__ get coverage from __gcda__ & __gcno__ files which lay in __gcovFiles__ directory

- `genhtml -o coverage.result coverage.info` __-__ generates coverage visualisation in directory __coverage.results__

---------------------

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


- Confgis for _gcov_ to get coverage data for kernel

		CONFIG_DEBUG_FS=y
		CONFIG_GCOV_KERNEL=y

- COnfigs for virtio-fs

		CONFIG_VIRTIO
	        CONFIG_VIRTIO_FS
	        CONFIG_DAX
	        CONFIG_FS_DAX
	        CONFIG_DAX_DRIVER
	        CONFIG_ZONE_DEVICE

---------------------


## Running in `qemu` with kunit_tool

- Run test isolated using arg `--run_isolated "test"`

- `./tools/testing/kunit/kunit.py run --arch=x86_64 --run_isolated "test"`

### Running with `gcov` tool (Doesn't work)

- We want __virtio_net.c__ to be profiled with `gcov`, so we add these line in `linux/drivers/net/Makefile`

		GCOV_PROFILE_virtio_net.o := y

- Also to run with gcov we should add appropriate configs to `.kuintconfig` in `linux/.kunit`

		CONFIG_DEBUG_FS=y
		CONFIG_GCOV_KERNEL=y

- Then we just run kuint test using basic run command of kunit tool, with gcov it will take a little more time to compile and run. It may not finish successfuly, in that case try again.

- File `.kunit/drivers/net/virtio_net.gcno` must be created after a successful run.


---------------------

## Virtual machine manager with Virtio-FS

- Compile the kernel with required configs on

			CONFIG_VIRTIO
	        CONFIG_VIRTIO_FS
	        CONFIG_DAX
	        CONFIG_FS_DAX
	        CONFIG_DAX_DRIVER
	        CONFIG_ZONE_DEVICE

- Create ramfs `ramdisk.img` file with command `mkinitramfs -o ramdiks.img` and keep it in one directory with `bzImage`

- Download, install and open __Virtual Machine Manager__

- Click `Create new instance` and choose manual installaion

- As operating system choose __Generic linux 2020__

- Memory 4096, and cpu 2 (recommended parameters)

- Disable storage for this virtual machine

- Name VM and enable option `Customize configuration before isntall`

- In `Memory` section enable __Shared Memory__ option

- In `Boot options` section open __kernel manual boot__ and click `Browse` for __Kernel Path__

- In opened menu click `Add pool` and choose the directory where `bzImage` and `ramdisk.img` already lay

- Then from your __new pool__ choose file `bzImage`, and click __choose Volume__

- Click `Browse` for __initrd path__, and choose `ramdisk.img` from the same pool, click `apply`

- Click `add Hardware` in the left bottom corner, choose section `filesystem`, choose `virtiofs`, choose path to the future shared directory, and also give it a name in `target path` field

- Click start installation, after which you should see terminal of the VM

- `mount -t virtiofs <name> <path where you want to mount shared directory>` - run this command to mount virtiofs shared folder

- Now we can transfer files from `host machine` to `VM` and vice versa

---------------------

### Notes, Kernel, basics

__Classes__

Classes are not a construct that is built into the C programming language; however, it is an easily derived concept. Accordingly, in most cases, every project that does not use a standardized object oriented library (like GNOME’s GObject) has their own slightly different way of doing object oriented programming; the Linux kernel is no exception.

The central concept in kernel object oriented programming is the class. In the kernel, a class is a struct that contains function pointers. This creates a contract between implementers and users since it forces them to use the same function signature without having to call the function directly. To be a class, the function pointers must specify that a pointer to the class, known as a class handle, be one of the parameters. Thus the member functions (also known as methods) have access to member variables (also known as fields) allowing the same implementation to have multiple instances.

A class can be overridden by child classes by embedding the parent class in the child class. Then when the child class method is called, the child implementation knows that the pointer passed to it is of a parent contained within the child. Thus, the child can compute the pointer to itself because the pointer to the parent is always a fixed offset from the pointer to the child. This offset is the offset of the parent contained in the child struct.

---------------------

## Useful links

[KernelSource](https://cdn.kernel.org/pub/linux/kernel/)

[Introduction to virtio-networking and vhost-net](https://www.redhat.com/en/blog/introduction-virtio-networking-and-vhost-net)

[Build linux kernel](https://phoenixnap.com/kb/build-linux-kernel)

[Linux kernel with qemu](https://medium.com/@daeseok.youn/prepare-the-environment-for-developing-linux-kernel-with-qemu-c55e37ba8ade)

[Booting custom linux kernel in qemu](http://nickdesaulniers.github.io/blog/2018/10/24/booting-a-custom-linux-kernel-in-qemu-and-debugging-it-with-gdb/)

[Tips for running kunit tests](https://docs.kernel.org/dev-tools/kunit/running_tips.html)

[Using gcov with linux kernel](https://docs.kernel.org/dev-tools/gcov.html)

[List of maintainers and how to submit kernel changes](https://docs.kernel.org/process/maintainers.html?highlight=virtio_net)

[Tips for running kunit test](https://kunit.dev/third_party/kernel/docs/running_tips.html)

[Kernel compilers](https://mirrors.edge.kernel.org/pub/tools/crosstool/)

[Kcov code coverage for fuzzing](https://docs.kernel.org/dev-tools/kcov.html)

[Runnign test with kunit tool](https://www.kernel.org/doc/html/latest/dev-tools/kunit/run_wrapper.html)

[How gcov works](https://github.com/shenxianpeng/gcov-example)

[virtio-fs kernel](https://docs.kernel.org/filesystems/virtiofs.html)

[virtio-fs.gitlab how to qemu](https://virtio-fs.gitlab.io/howto-qemu.html)

[virtio-fs con](https://www.youtube.com/watch?v=wM5OB0PGIQA)

[Securely booting confidential VMs with encrypting disk](https://www.youtube.com/watch?v=4wZnl0njxm8)

[elixir.bootlin.com](https://elixir.bootlin.com/linux/latest/source)

[KConfig language](https://www.kernel.org/doc/html/next/kbuild/kconfig-language.html)

[Martin Fowler, vocabulary](https://martinfowler.com/bliki/TestDouble.html)

---

[linux kernel testing and debugging](https://www.linuxjournal.com/content/linux-kernel-testing-and-debugging)

[virtio networking red hat](https://www.redhat.com/en/blog/introduction-virtio-networking-and-vhost-net)

[docs oasis](https://docs.oasis-open.org/virtio/virtio/v1.2/cs01/virtio-v1.2-cs01.html#x1-20001)

1. [ibm software testing](https://www.ibm.com/topics/software-testing)

2. [testing](https://www.guru99.com/software-testing-introduction-importance.html)

3. [unit testing](https://www.techtarget.com/searchsoftwarequality/definition/unit-testing#:~:text=Unit%20testing%20is%20a%20software,tests%20during%20the%20development%20process.)

4. [habr testing](https://habr.com/ru/articles/549054/)

5. [amazon debug](https://aws.amazon.com/what-is/debugging/#:~:text=Debugging%20is%20the%20process%20of,determine%20why%20any%20errors%20occurred.)

6. [Martin Fowler, vocabulary](https://martinfowler.com/bliki/TestDouble.html)


# StepByStep

- `cd ~`

- `mkdir workspace`

- `cd workspace`

- `wget https://cdn.kernel.org/pub/linux/kernel/v5.x/linux-5.10.3.tar.xz`

- `tar xvf linux-5.10.3.tar.xz`

- `wget https://mirrors.edge.kernel.org/pub/tools/crosstool/files/bin/x86_64/11.3.0/x86_64-gcc-11.3.0-nolibc-x86_64-linux.tar.xz`

- `tar xvf x86_64-gcc-11.3.0-nolibc-x86_64-linux.tar.xz`

- `cd linux-5.10.3`

- `./tools/testing/kunit/kunit.py run`

- `cd drivers/net`

- `touch virtio_net_test.c virtio_net_test.h`

- `cd ../..`

- 
