#include <virtio_net_test.h>

void fake_klist_get(struct klist_node* n) {}

void fake_get(struct virtio_device *vdev, unsigned offset,
		    void *buf, unsigned len) {}

u8 fake_get_status(struct virtio_device *vdev) { return 0; }

void fake_set_status(struct virtio_device *vdev, u8 status) {}

void fake_reset(struct virtio_device *vdev) {}

int fake_test_find_vqs(struct virtio_device *dev, unsigned nvqs,
			struct virtqueue *vqs[], vq_callback_t *callbacks[],
			const char * const names[], const bool *ctx,
			struct irq_affinity *desc) { return 0; }

void mock_get(struct virtio_device *vdev, unsigned offset,
		    void *buf, unsigned len) {
	u8* ptr = buf;
	ptr[0] = 1;
	ptr[1] = 1;
}

bool mock_notify(struct virtqueue *vq) {
	return true;
}

void mock_init_virtqueues(struct virtio_device *dev,
						  struct virtqueue *vqs[],
						  unsigned nvqs) {
	struct vring_virtqueue* vring_vq;
	int i;

	for (i = 0; i < nvqs; ++i) {
 		vring_vq = kcalloc(1, sizeof(struct vring_virtqueue), GFP_KERNEL);
		vqs[i] = &(vring_vq->vq);
        vqs[i]->vdev = dev;
    }
}

void mock_init_kobjects(struct virtnet_info *vi) {
	struct device* init_dev;
	struct kernfs_node* kn;
    struct klist_node* k_node;

	init_dev = &vi->dev->dev;
	kn = kcalloc(1, sizeof(struct kernfs_node), GFP_KERNEL);

	init_dev->parent->kobj.sd = kn;
	kn->parent = NULL;

	kn->dir.root = kcalloc(1, KERNFS_ROOT_MEM_SIZE, GFP_KERNEL);
	kn->dir.root->ino_idr.idr_rt.xa_flags = IDR_RT_MARKER;
	atomic_set(&kn->count, 1);

	// init kernfs_node & klist objects
	kn->flags = 17;

	init_dev->parent->p = kcalloc(1, sizeof(struct device_private), GFP_KERNEL);
	init_dev->parent->p->klist_children.get = fake_klist_get;
	
	k_node = kcalloc(1, sizeof(struct klist_node), GFP_KERNEL);
	init_dev->parent->p->klist_children.k_list.prev = &(k_node->n_node);

	// init parent device name
	init_dev->parent->init_name = "DummyParentDeviceName";
}

void find_vqs_common(struct virtnet_info *vinfo, struct virtio_device *dev, 
					 unsigned nvqs, struct virtqueue *vqs[]) {
    struct vring_virtqueue* vring_vq;

	// init queues
	mock_init_virtqueues(dev, vqs, nvqs);

    vinfo->rq[0].vq = vqs[0];
    vinfo->sq[0].vq = vqs[1];
	vring_vq = to_vvq(vinfo->rq[0].vq);

	if (vring_vq->packed_ring)
		vring_vq->packed.vring.num = 1;
    else
		vring_vq->split.vring.num = 1;

	// init kobjects
	mock_init_kobjects(vinfo);
}

void init_vring_objects(struct virtnet_info *vinfo, 
						struct virtio_device *dev,
						unsigned nvqs,
						struct virtqueue *vqs[]) {
	struct vring_virtqueue* vring_vq;
    struct vring_virtqueue* vring_cvq;
    struct vring_virtqueue *cvq_vvrq;
    struct virtqueue* cvq;

	vring_cvq = kcalloc(1, sizeof(struct vring_virtqueue), GFP_KERNEL);
    vinfo->cvq = &(vring_cvq->vq);
    cvq = vinfo->cvq;
    cvq->vdev = dev;
    vring_vq = to_vvq(cvq);
    vring_vq->notify = mock_notify;

	cvq = vqs[nvqs - 1];
	cvq_vvrq = to_vvq(cvq);
	cvq_vvrq->indirect = true;
	cvq_vvrq->use_dma_api = false;
	cvq_vvrq->vq.num_free = 1;
	cvq_vvrq->notify = mock_notify;

	cvq_vvrq->split.desc_state = kcalloc(1, sizeof(struct vring_desc_state_split), GFP_KERNEL);
	cvq_vvrq->split.vring.desc = kcalloc(1, sizeof(struct vring_desc), GFP_KERNEL);
    cvq_vvrq->split.desc_extra = kcalloc(1, sizeof(struct vring_desc_extra), GFP_KERNEL);
    cvq_vvrq->split.vring.avail = kcalloc(1, sizeof(vring_avail_t) + 2 * sizeof(__virtio16), GFP_KERNEL);
    cvq_vvrq->split.vring.used = kcalloc(1, sizeof(vring_used_t) + 2 * sizeof(vring_used_elem_t), GFP_KERNEL);

    cvq_vvrq->last_used_idx = 1;
    cvq_vvrq->split.vring.num = 1;
}

int simple_find_vqs(struct virtio_device *dev, unsigned nvqs,
			struct virtqueue *vqs[], vq_callback_t *callbacks[],
			const char * const names[], const bool *ctx,
			struct irq_affinity *desc) {
    struct virtnet_info *vinfo;
	vinfo = (struct virtnet_info *)(dev->priv);
	find_vqs_common(vinfo, dev, nvqs, vqs);
	return 0;
}

int with_rss_init_find_vqs(struct virtio_device *dev, unsigned nvqs,
			struct virtqueue *vqs[], vq_callback_t *callbacks[],
			const char * const names[], const bool *ctx,
			struct irq_affinity *desc) 
{
	struct virtnet_info *vinfo;
	vinfo = (struct virtnet_info *)(dev->priv);
	find_vqs_common(vinfo, dev, nvqs, vqs);

	vinfo->ctrl = kcalloc(1, sizeof(struct control_buf), GFP_KERNEL);
	vinfo->rss_key_size = VIRTIO_NET_RSS_MAX_KEY_SIZE;
	vinfo->rss_indir_table_size = 0;

	return 0;
}


int stress_find_vqs(struct virtio_device *dev, unsigned nvqs,
			struct virtqueue *vqs[], vq_callback_t *callbacks[],
			const char * const names[], const bool *ctx,
			struct irq_affinity *desc) 
{
    struct virtnet_info *vinfo;
    struct vring_virtqueue* vring_vq;

    int i, j;

	// init queues
	mock_init_virtqueues(dev, vqs, nvqs);
    vinfo = (struct virtnet_info *)(dev->priv);

    i = 0;
	j = 0;
	for (; i < nvqs / 2; ++i) {
		vinfo->rq[i].vq = vqs[j];
		vinfo->sq[i].vq = vqs[j + 1];
		{
			vring_vq = to_vvq(vqs[j]);
			vring_vq->notify = mock_notify;
			vring_vq = to_vvq(vqs[j + 1]);
			vring_vq->notify = mock_notify;

		}
		j += 2;

        // init revceive queue
		vring_vq = to_vvq(vinfo->rq[i].vq);
        vring_vq->notify = mock_notify;
		if (vring_vq->packed_ring) {
			vring_vq->packed.vring.num = 1;
		} else {
			vring_vq->split.vring.num = 1;
		}

        // init send queue
		vring_vq = to_vvq(vinfo->sq[i].vq);
        vring_vq->notify = mock_notify;
		if (vring_vq->packed_ring) {
			vring_vq->packed.vring.num = 1;
		} else {
			vring_vq->split.vring.num = 1;
		}
	}

	// init vring queues
    init_vring_objects(vinfo, dev, nvqs, vqs);

	// init kobject
	mock_init_kobjects(vinfo);

	return 0;           
}

/// VirtioConfigOptions

static const struct virtio_config_ops options_0_2 = {
	.get = fake_get,
	.get_status = fake_get_status,
	.set_status = fake_set_status,
	.reset = fake_reset,
	.find_vqs = simple_find_vqs,
};

static const struct virtio_config_ops options_1_4_5 = {
	.get = mock_get,
	.get_status = fake_get_status,
	.set_status = fake_set_status,
	.reset = fake_reset,
	.find_vqs = stress_find_vqs,
};

static const struct virtio_config_ops options_3 = {
	.get = fake_get,
	.get_status = fake_get_status,
	.set_status = fake_set_status,
	.reset = fake_reset,
	.find_vqs = stress_find_vqs,
};

static const struct virtio_config_ops options_6_7 = {
	.get = fake_get,
	.get_status = fake_get_status,
	.set_status = fake_set_status,
	.reset = fake_reset,
	.find_vqs = with_rss_init_find_vqs,
};

///////////////////// Test 0 /////////////////////

static void test_0(struct kunit *test)
{
    struct virtio_device *dev;
    struct virtio_driver *drv;
    struct virtio_device dev_s = {.config = &options_0_2};
	struct virtio_driver drv_s = {
		.feature_table = features,
		.feature_table_size = ARRAY_SIZE(features),
		.feature_table_legacy = features_legacy,
		.feature_table_size_legacy = ARRAY_SIZE(features_legacy)};
    int err = 0;

    // minimal device initialization
    device_initialize(&dev_s.dev);
    dev_s.dev.parent = NULL;
    dev_s.dev.kobj.parent = NULL;
    dev_s.dev.driver = &drv_s.driver;

    // virtio_device initialization
    dev = &dev_s;
    drv = drv_to_virtio(dev->dev.driver);

    // call virtnet_probe
    err = virtnet_probe(dev);

    KUNIT_EXPECT_EQ(test, err, 0);
}

///////////////////// Test 1 /////////////////////

static void test_1(struct kunit *test)
{
    struct virtio_device *dev;
    struct virtio_driver *drv;
    struct virtio_device dev_s = {.config = &options_1_4_5};
	struct virtio_driver drv_s = {
		.feature_table = features,
		.feature_table_size = ARRAY_SIZE(features),
		.feature_table_legacy = features_legacy,
		.feature_table_size_legacy = ARRAY_SIZE(features_legacy)};
    int err = 0;
    
    // minimal device initialization
    device_initialize(&dev_s.dev);
    dev_s.dev.parent = NULL;
    dev_s.dev.kobj.parent = NULL;
    dev_s.dev.driver = &drv_s.driver;

	// switch on features
	dev_s.features |= BIT_ULL(VIRTIO_NET_F_MQ);

    // virtio_device initialization
    dev = &dev_s;
    drv = drv_to_virtio(dev->dev.driver);

    // call virtnet probe
    err = virtnet_probe(dev);

    KUNIT_EXPECT_EQ(test, err, 0);
}

///////////////////// Test 2 /////////////////////

static void test_2(struct kunit *test) {
    struct virtio_device *dev;
    struct virtio_driver *drv;
	// Handler functions
    struct virtio_device dev_s = {.config = &options_0_2}; 
        struct virtio_driver drv_s = {
		.feature_table = features,
		.feature_table_size = ARRAY_SIZE(features),
		.feature_table_legacy = features_legacy,
		.feature_table_size_legacy = ARRAY_SIZE(features_legacy)};
    int err = 0;

    // minimal device initialization
    device_initialize(&dev_s.dev);
    dev_s.dev.parent = NULL;
    dev_s.dev.kobj.parent = NULL;
    dev_s.dev.driver = &drv_s.driver;

    // switch on features
	dev_s.features |= BIT_ULL(VIRTIO_NET_F_CSUM);

    // virtio_device initialization
    dev = &dev_s;
    drv = drv_to_virtio(dev->dev.driver);

    // call virtnet probe
    err = virtnet_probe(dev);

    KUNIT_EXPECT_EQ(test, err, 0);
}

///////////////////// Test 3 /////////////////////

static void test_3(struct kunit *test)
{
    struct virtio_device *dev;
    struct virtio_driver *drv;
    struct virtio_device dev_s = {.config = &options_3};
	struct virtio_driver drv_s = {
		.feature_table = features,
		.feature_table_size = ARRAY_SIZE(features),
		.feature_table_legacy = features_legacy,
		.feature_table_size_legacy = ARRAY_SIZE(features_legacy)};
    int err = 0;

    // minimal device initialization
    device_initialize(&dev_s.dev);
    dev_s.dev.parent = NULL;
    dev_s.dev.kobj.parent = NULL;
    dev_s.dev.driver = &drv_s.driver;

	// switch on features
	dev_s.features |= BIT_ULL(VIRTIO_NET_F_MAC);

	// without VIRTIO_NET_F_MQ, conflicts with othres
	dev_s.features |= BIT_ULL(VIRTIO_NET_F_CTRL_VQ);
	dev_s.features |= BIT_ULL(VIRTIO_NET_F_CSUM);

    // virtio_device initialization
    dev = &dev_s;
    drv = drv_to_virtio(dev->dev.driver);

    // call virtnet_probe
    err = virtnet_probe(dev);

	if (err) {
		// with address sanitizer not enough memory
    	KUNIT_EXPECT_EQ(test, err, -12);
		return;
	}

    KUNIT_EXPECT_EQ(test, err, 0);
}

///////////////////// Test 4 /////////////////////

static void test_4(struct kunit *test)
{
    struct virtio_device *dev;
    struct virtio_driver *drv;
    struct virtio_device dev_s = {.config = &options_1_4_5};
	struct virtio_driver drv_s = {
		.feature_table = features,
		.feature_table_size = ARRAY_SIZE(features),
		.feature_table_legacy = features_legacy,
		.feature_table_size_legacy = ARRAY_SIZE(features_legacy)};
    int err = 0;

    // minimal device initialization
    device_initialize(&dev_s.dev);
    dev_s.dev.parent = NULL;
    dev_s.dev.kobj.parent = NULL;
    dev_s.dev.driver = &drv_s.driver;

	// switch on features
	dev_s.features |= BIT_ULL(VIRTIO_NET_F_CTRL_VQ);
	dev_s.features |= BIT_ULL(VIRTIO_NET_F_CSUM);

    // virtio_device initialization
    dev = &dev_s;
    drv = drv_to_virtio(dev->dev.driver);

    // call virtnet_probe
    err = virtnet_probe(dev);

    KUNIT_EXPECT_EQ(test, err, 0);
}

///////////////////// Test 5 /////////////////////

static void test_5(struct kunit *test)
{
    struct virtio_device *dev;
    struct virtio_driver *drv;
    struct virtio_device dev_s = {.config = &options_1_4_5};
	struct virtio_driver drv_s = {
		.feature_table = features,
		.feature_table_size = ARRAY_SIZE(features),
		.feature_table_legacy = features_legacy,
		.feature_table_size_legacy = ARRAY_SIZE(features_legacy)};
    int err = 0;

    // minimal device initialization
    device_initialize(&dev_s.dev);
    dev_s.dev.parent = NULL;
    dev_s.dev.kobj.parent = NULL;
    dev_s.dev.driver = &drv_s.driver;

	// switch on features
	dev_s.features |= BIT_ULL(VIRTIO_NET_F_CTRL_VQ);
	dev_s.features |= BIT_ULL(VIRTIO_NET_F_CSUM);
	dev_s.features |= BIT_ULL(VIRTIO_NET_F_GSO);
	dev_s.features |= BIT_ULL(VIRTIO_NET_F_HOST_TSO4);
	dev_s.features |= BIT_ULL(VIRTIO_NET_F_HOST_TSO6);
	dev_s.features |= BIT_ULL(VIRTIO_NET_F_HOST_ECN);
	dev_s.features |= BIT_ULL(VIRTIO_NET_F_HOST_USO);

    // virtio_device initialization
    dev = &dev_s;
    drv = drv_to_virtio(dev->dev.driver);

    // call virtnet_probe
    err = virtnet_probe(dev);

    KUNIT_EXPECT_EQ(test, err, 0);
}

///////////////////// Test 6 /////////////////////

static void test_6(struct kunit *test) {
struct virtio_device *dev;
    struct virtio_driver *drv;
	// init options with fake functions
    struct virtio_device dev_s = {.config = &options_6_7};
    struct virtio_driver drv_s = {
		.feature_table = features,
		.feature_table_size = ARRAY_SIZE(features),
		.feature_table_legacy = features_legacy,
		.feature_table_size_legacy = ARRAY_SIZE(features_legacy)};
    int err = 0;

    // minimal device initialization
    device_initialize(&dev_s.dev);
    dev_s.dev.parent = NULL;
    dev_s.dev.kobj.parent = NULL;
    dev_s.dev.driver = &drv_s.driver;

	// switch on features
	dev_s.features |= BIT_ULL(VIRTIO_NET_F_NOTF_COAL);
	dev_s.features |= BIT_ULL(VIRTIO_NET_F_HASH_REPORT);

    // virtio_device initialization
    dev = &dev_s;
    drv = drv_to_virtio(dev->dev.driver);

    // call virtnet_probe
    err = virtnet_probe(dev);

    KUNIT_EXPECT_EQ(test, err, 0);
}

///////////////////// Test 7 /////////////////////

static void test_7(struct kunit *test) {
struct virtio_device *dev;
    struct virtio_driver *drv;
    struct virtio_device dev_s = {.config = &options_6_7};
    struct virtio_driver drv_s = {
		.feature_table = features,
		.feature_table_size = ARRAY_SIZE(features),
		.feature_table_legacy = features_legacy,
		.feature_table_size_legacy = ARRAY_SIZE(features_legacy)};
    int err = 0;

    // minimal device initialization
    device_initialize(&dev_s.dev);
    dev_s.dev.parent = NULL;
    dev_s.dev.kobj.parent = NULL;
    dev_s.dev.driver = &drv_s.driver;

	// switch on features
	dev_s.features |= BIT_ULL(VIRTIO_NET_F_NOTF_COAL);
	dev_s.features |= BIT_ULL(VIRTIO_NET_F_HASH_REPORT);
	dev_s.features |= BIT_ULL(VIRTIO_NET_F_MTU);
	dev_s.features |= BIT_ULL(VIRTIO_NET_F_RSS);
	dev_s.features |= BIT_ULL(VIRTIO_NET_F_STANDBY);

    // virtio_device initialization
    dev = &dev_s;
    drv = drv_to_virtio(dev->dev.driver);

    // call virtnet_probe
    err = virtnet_probe(dev);

    KUNIT_EXPECT_EQ(test, err, 0);
}

// add test cases to test suite
static struct kunit_case example_test_cases[] = {
	KUNIT_CASE(test_0),
	KUNIT_CASE(test_1),
	KUNIT_CASE(test_2),
	KUNIT_CASE(test_3),
	KUNIT_CASE(test_4),
	KUNIT_CASE(test_5),
	KUNIT_CASE(test_6),
	KUNIT_CASE(test_7),
    {}
};

static struct kunit_suite example_test_suite = {
	.name = "virtio_net_test",
	.test_cases = example_test_cases,
};

kunit_test_suites(&example_test_suite);

MODULE_LICENSE("GPL");
