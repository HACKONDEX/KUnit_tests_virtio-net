#include <virtio_net_test.h>

// mock and fake methods
void fake_klist_get(struct klist_node* n) {}

void fake_get(struct virtio_device *vdev, unsigned offset,
		    void *buf, unsigned len) {}

void fake_set_status(struct virtio_device *vdev, u8 status) {}

void fake_reset(struct virtio_device *vdev) {}

u8 mock_get_status(struct virtio_device *vdev) { return 0; }

bool mock_notify(struct virtqueue *vq) { return true; }

void mock_get(struct virtio_device *vdev, unsigned offset,
		    void *buf, unsigned len) {
	u8* ptr = buf;
	ptr[0] = 1;
	ptr[1] = 1;
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

int find_vqs_simple(struct virtio_device *dev, unsigned nvqs,
			struct virtqueue *vqs[], vq_callback_t *callbacks[],
			const char * const names[], const bool *ctx,
			struct irq_affinity *desc) {
    struct virtnet_info *vinfo;
    struct vring_virtqueue* vring_vq;

	// init queues
	mock_init_virtqueues(dev, vqs, nvqs);

    vinfo = (struct virtnet_info *)(dev->priv);

    vinfo->rq[0].vq = vqs[0];
	vring_vq = to_vvq(vinfo->rq[0].vq);

	if (vring_vq->packed_ring)
		vring_vq->packed.vring.num = 1;
    else
		vring_vq->split.vring.num = 1;
    
    vinfo->sq[0].vq = vqs[1];

	// init kobject
	mock_init_kobjects(vinfo);

	return 0;           
}

int mock_find_vqs_simple(struct virtio_device *dev, unsigned nvqs,
			struct virtqueue *vqs[], vq_callback_t *callbacks[],
			const char * const names[], const bool *ctx,
			struct irq_affinity *desc) {
    return find_vqs_simple(dev, nvqs, vqs, callbacks, names, ctx, desc);
}

int mock_find_vqs_stress(struct virtio_device *dev, unsigned nvqs,
			struct virtqueue *vqs[], vq_callback_t *callbacks[],
			const char * const names[], const bool *ctx,
			struct irq_affinity *desc) 
{
    struct virtnet_info *vinfo;
    struct vring_virtqueue* vring_vq;
    struct vring_virtqueue* vring_cvq;
    struct vring_virtqueue *cvq_vvrq;
    struct virtqueue* cvq;
    int i = 0, j = 0;

	mock_init_virtqueues(dev, vqs, nvqs);

    vinfo = (struct virtnet_info *)(dev->priv);
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

	// init kobject
	mock_init_kobjects(vinfo);

	return 0;           
}

static const struct virtio_config_ops mock_config_ops_1 = {
	.get = fake_get,
	.get_status = mock_get_status,
	.set_status = fake_set_status,
	.reset = fake_reset,
	.find_vqs = mock_find_vqs_simple,
};

static const struct virtio_config_ops mock_config_ops_2 = {
	.get = fake_get,
	.get_status = mock_get_status,
	.set_status = fake_set_status,
	.reset = fake_reset,
	.find_vqs = mock_find_vqs_stress,
};

static const struct virtio_config_ops mock_config_ops_3 = {
	.get = mock_get,
	.get_status = mock_get_status,
	.set_status = fake_set_status,
	.reset = fake_reset,
	.find_vqs = mock_find_vqs_stress,
};

///////////////////// Test 0 /////////////////////

static void test_0(struct kunit *test)
{
    struct virtio_device *dev;
    struct virtio_driver *drv;
    struct virtio_device dev_s = {.config = &mock_config_ops_1};
	struct virtio_driver drv_s = {
		.feature_table = features,
		.feature_table_size = ARRAY_SIZE(features),
		.feature_table_legacy = features_legacy,
		.feature_table_size_legacy = ARRAY_SIZE(features_legacy)};
    int err = 0;

    // device initialization
    device_initialize(&dev_s.dev);
    dev_s.dev.parent = NULL;
    dev_s.dev.kobj.parent = NULL;
    dev_s.dev.driver = &drv_s.driver;

    // virtio_device initialization
    dev = &dev_s;
    drv = drv_to_virtio(dev->dev.driver);

    // call tested function
    err = virtnet_probe(dev);

    KUNIT_EXPECT_EQ(test, err, 0);
}

///////////////////// Test 1 /////////////////////

static void test_1(struct kunit *test) {
    struct virtio_device *dev;
    struct virtio_driver *drv;
    struct virtio_device dev_s = {.config = &mock_config_ops_1};
        struct virtio_driver drv_s = {
		.feature_table = features,
		.feature_table_size = ARRAY_SIZE(features),
		.feature_table_legacy = features_legacy,
		.feature_table_size_legacy = ARRAY_SIZE(features_legacy)};
    int err = 0;

	// device initialization
    device_initialize(&dev_s.dev);
    dev_s.dev.parent = NULL;
    dev_s.dev.kobj.parent = NULL;
    dev_s.dev.driver = &drv_s.driver;

    // switch on features
	dev_s.features |= BIT_ULL(VIRTIO_NET_F_CSUM);

    // virtio_device initialization
    dev = &dev_s;
    drv = drv_to_virtio(dev->dev.driver);

    // call tested function
    err = virtnet_probe(dev);

    KUNIT_EXPECT_EQ(test, err, 0);
}

///////////////////// Test 2 /////////////////////

static void test_2(struct kunit *test)
{
    struct virtio_device *dev;
    struct virtio_driver *drv;
    struct virtio_device dev_s = {.config = &mock_config_ops_2};
	struct virtio_driver drv_s = {
		.feature_table = features,
		.feature_table_size = ARRAY_SIZE(features),
		.feature_table_legacy = features_legacy,
		.feature_table_size_legacy = ARRAY_SIZE(features_legacy)};
    int err = 0;

	// device initialization
    device_initialize(&dev_s.dev);
    dev_s.dev.parent = NULL;
    dev_s.dev.kobj.parent = NULL;
    dev_s.dev.driver = &drv_s.driver;

	// switch on features
	dev_s.features |= BIT_ULL(VIRTIO_NET_F_MAC);
	dev_s.features |= BIT_ULL(VIRTIO_NET_F_MQ);
	dev_s.features |= BIT_ULL(VIRTIO_NET_F_CTRL_VQ);
	dev_s.features |= BIT_ULL(VIRTIO_NET_F_CSUM);

    // virtio_device initialization
    dev = &dev_s;
    drv = drv_to_virtio(dev->dev.driver);

    // call tested function
    err = virtnet_probe(dev);

    KUNIT_EXPECT_EQ(test, err, 0);
}

///////////////////// Test 3 /////////////////////

static void test_3(struct kunit *test)
{
    struct virtio_device *dev;
    struct virtio_driver *drv;
    struct virtio_device dev_s = {.config = &mock_config_ops_3};
	struct virtio_driver drv_s = {
		.feature_table = features,
		.feature_table_size = ARRAY_SIZE(features),
		.feature_table_legacy = features_legacy,
		.feature_table_size_legacy = ARRAY_SIZE(features_legacy)};
    int err = 0;

	// device initialization
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

    // call tested function
    err = virtnet_probe(dev);

    KUNIT_EXPECT_EQ(test, err, 0);
}

// ///////////////////// Test 4 /////////////////////

static void test_4(struct kunit *test) {
 	struct virtio_device *dev;
    struct virtio_driver *drv;
    struct virtio_device dev_s = {.config = &mock_config_ops_3};
	struct virtio_driver drv_s = {
		.feature_table = features,
		.feature_table_size = ARRAY_SIZE(features),
		.feature_table_legacy = features_legacy,
		.feature_table_size_legacy = ARRAY_SIZE(features_legacy)};
    int err = 0;

	// device initialization
    device_initialize(&dev_s.dev);
    dev_s.dev.parent = NULL;
    dev_s.dev.kobj.parent = NULL;
    dev_s.dev.driver = &drv_s.driver;

	// switch on features
	dev_s.features |= BIT_ULL(VIRTIO_NET_F_MQ);
	dev_s.features |= BIT_ULL(VIRTIO_NET_F_CTRL_VQ);
	dev_s.features |= BIT_ULL(VIRTIO_NET_F_CTRL_VQ);
	dev_s.features |= BIT_ULL(VIRTIO_NET_F_CSUM);
	dev_s.features |= BIT_ULL(VIRTIO_NET_F_MTU);

    // virtio_device initialization
    dev = &dev_s;
    drv = drv_to_virtio(dev->dev.driver);

    // call tested function
    err = virtnet_probe(dev);

    KUNIT_EXPECT_EQ(test, err, 0);
}

///////////////////// Test 5 /////////////////////

static void test_5(struct kunit *test)
{
    struct virtio_device *dev;
    struct virtio_driver *drv;
    struct virtio_device dev_s = {.config = &mock_config_ops_3};
	struct virtio_driver drv_s = {
		.feature_table = features,
		.feature_table_size = ARRAY_SIZE(features),
		.feature_table_legacy = features_legacy,
		.feature_table_size_legacy = ARRAY_SIZE(features_legacy)};
    int err = 0;
    
	// device initialization
    device_initialize(&dev_s.dev);
    dev_s.dev.parent = NULL;
    dev_s.dev.kobj.parent = NULL;
    dev_s.dev.driver = &drv_s.driver;

	// switch on features
	dev_s.features |= BIT_ULL(VIRTIO_NET_F_MQ);
	dev_s.features |= BIT_ULL(VIRTIO_NET_F_CTRL_VQ);

    // virtio_device initialization
    dev = &dev_s;
    drv = drv_to_virtio(dev->dev.driver);

    // call tested function
    err = virtnet_probe(dev);

    KUNIT_EXPECT_EQ(test, err, 0);
}

static struct kunit_case test_cases[] = {
	KUNIT_CASE(test_0),
	KUNIT_CASE(test_1),
	KUNIT_CASE(test_2),
	KUNIT_CASE(test_3),
	KUNIT_CASE(test_4),
	KUNIT_CASE(test_5),
    {}
};

static struct kunit_suite example_test_suite = {
	.name = "virtio_net_test",
	.test_cases = test_cases,
};

kunit_test_suites(&example_test_suite);

MODULE_LICENSE("GPL");
