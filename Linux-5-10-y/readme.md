# Research

---

## KUnit virtio_net 5.10.y

### Commands

- `./tools/testing/kunit/kunit.py run` __-__ run all kunit tests

- `./tools/testing/kunit/kunit.py run --kunitconfig=lib/kunit/.kunitconfig` __-__ run tests with given configuration(doesn't work on branch 5.10.y)

### .kunitconfig variant

		CONFIG_KUNIT=y
		CONFIG_KUNIT_TEST=y
		CONFIG_KUNIT_EXAMPLE_TEST=y
		CONFIG_VIRTIO_TEST=y
		CONFIG_KASAN=y # optional
