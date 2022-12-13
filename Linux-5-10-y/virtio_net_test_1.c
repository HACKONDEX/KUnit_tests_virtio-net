void test_get_1(struct virtio_device *vdev, unsigned offset,
		    void *buf, unsigned len) {
    printk("\nLOG:---Get---\n");
	printk("\nLOG: len: %u\n", len);
	u8* ptr = buf;
	ptr[0] = 1;
	ptr[1] = 1;
}

bool test_notify_1(struct virtqueue *vq) {
	printk("\nLOG: --- Notify test 1 --- \n");
	return true;
}

int test_find_vqs_1(struct virtio_device *dev, unsigned nvqs,
			struct virtqueue *vqs[], vq_callback_t *callbacks[],
			const char * const names[], const bool *ctx,
			struct irq_affinity *desc) 
{
    struct virtnet_info *vinfo = (struct virtnet_info *)(dev->priv);

	
	printk("\nLOG: vinfo->cvq: %p\n", vinfo->cvq);

	// Init queues
	printk("\nLOG: nvqs: %u\n", nvqs);
	int i, j;
	for (i = 0; i < nvqs; ++i) {
		vqs[i] = kcalloc(1, sizeof(struct virtqueue), GFP_KERNEL);
		vqs[i]->vdev = dev;
	}

	j = 0;
	for (i = 0; i < nvqs / 2; ++i) {
		vinfo->rq[i].vq = vqs[j];
		vinfo->sq[i].vq = vqs[j + 1];
		j += 2;
		struct vring_virtqueue *vvrq = to_vvq(vinfo->rq[i].vq);
		if (vvrq->packed_ring) {
			vvrq->packed.vring.num = 1;
		} else {
			vvrq->split.vring.num = 1;
		}
		struct vring_virtqueue *vvsq = to_vvq(vinfo->sq[i].vq);
		if (vvsq->packed_ring) {
			vvsq->packed.vring.num = 1;
		} else {
			vvsq->split.vring.num = 1;
		}
	}

	struct virtqueue* cvq = vqs[nvqs - 1];
	struct vring_virtqueue *vvrq = to_vvq(cvq);
	vvrq->indirect = true;
	vvrq->use_dma_api = false;
	vvrq->vq.num_free = 0;
	vvrq->notify = test_notify_1;
	vvrq->split.desc_state = kcalloc(1, sizeof(struct vring_desc_state_split), GFP_KERNEL);
	vvrq->split.vring.desc = kcalloc(1, sizeof(struct vring_desc), GFP_KERNEL);
	// vvrq->split.vring.num = 1;


	// init kobject
	struct net_device* net_dev = vinfo->dev;
	struct device* init_dev = &net_dev->dev;
	struct kernfs_node *kn = kcalloc(1, sizeof(struct kernfs_node), GFP_KERNEL);

	init_dev->parent->kobj.sd = kn;
	kn->parent = NULL;
	const int KERNFS_ROOT_MEM_SIZE = 120;
	kn->dir.root = kcalloc(1, KERNFS_ROOT_MEM_SIZE, GFP_KERNEL);
	kn->dir.root->ino_idr.idr_rt.xa_flags = IDR_RT_MARKER;
	atomic_set(&kn->count, 1);

	// Init kernfs_node flags(-EINVAL otherwise)
	kn->flags = 17;
	// Init klist objects
	init_dev->parent->p = kcalloc(1, sizeof(struct device_private), GFP_KERNEL);
	init_dev->parent->p->klist_children.get = klist_get_test;
	
	struct klist_node* k_node = kcalloc(1, sizeof(struct klist_node), GFP_KERNEL);
	init_dev->parent->p->klist_children.k_list.prev = &(k_node->n_node);

	// Init parent name
	init_dev->parent->init_name = "ParentDevice\0";

	printk("\nLOG: Find VQS finished\n");
	return 0;           
}

void test_reset_1(struct virtio_device *vdev) {
	// Only for debug
	// struct virtnet_info *vinfo = (struct virtnet_info *)(vdev->priv);
	// struct vring_virtqueue *vvrq = to_vvq(vinfo->cvq);
	// printk("\nLOG %d: awesome num == %d \n", __LINE__, vvrq->split.vring.desc[0].flags);
}

// Test 1
static void virnet_probe_test_1(struct kunit *test)
{
    printk("\n\n\nStart Test 1\n\n\n");
    // device initialization

	// set handler functions

	const struct virtio_config_ops config_ops = {
	.get = test_get_1,
	.get_status = test_get_status,
	.set_status = test_set_status,
	.reset = test_reset_1,
	.find_vqs = test_find_vqs_1,
    .set_vq_affinity = test_set_vq_affinity,
    .get_shm_region = test_get_shm_region,
	};


    struct virtio_device dev_s = {.config = &config_ops, };
    device_initialize(&dev_s.dev);
    dev_s.dev.parent = NULL;
    dev_s.dev.kobj.parent = NULL;

    /// Features for device_driver
    const unsigned int feature_table[] = {VIRTIO_NET_F_MQ, 
                                          VIRTIO_NET_F_RSS,
                                          VIRTIO_NET_F_CTRL_VQ,
                                          VIRTIO_NET_F_CSUM,
                                          VIRTIO_NET_F_GUEST_CSUM,
                                          VIRTIO_NET_F_GUEST_TSO4,
                                          VIRTIO_NET_F_GUEST_TSO6,
                                          VIRTIO_NET_F_CTRL_GUEST_OFFLOADS,
                                          VIRTIO_NET_F_MAC, 
                                          VIRTIO_NET_F_GUEST_ECN,
                                          VIRTIO_NET_F_GUEST_UFO,
                                          VIRTIO_NET_F_MRG_RXBUF,
                                          VIRTIO_NET_F_HASH_REPORT,
                                          VIRTIO_F_ANY_LAYOUT,
                                          VIRTIO_NET_F_MTU,
                                          VIRTIO_NET_F_STATUS,
										  VIRTIO_NET_F_CTRL_VLAN};
    // struct virtio_driver drv_s = {.feature_table = feature_table};
    // drv_s.feature_table_size = 16;
    struct virtio_driver drv_s = {.feature_table_legacy = feature_table};
    drv_s.feature_table_size_legacy = 17;
    dev_s.dev.driver = &drv_s.driver;

	// Add features
	dev_s.features |= BIT_ULL(VIRTIO_NET_F_MQ);
	dev_s.features |= BIT_ULL(VIRTIO_NET_F_CTRL_VQ);


    // virtio_device initialization
    // struct virtio_device *dev = dev_to_virtio(&dev_s.dev);
    struct virtio_device *dev = &dev_s;
    struct virtio_driver *drv = drv_to_virtio(dev->dev.driver);
    u64 device_features;
	u64 driver_features;
	u64 driver_features_legacy;
    int err = 0;

    // Virtnet probe test
    err = virtnet_probe(dev);

    printk("\nLOG: TEST 1 finished \n");

    KUNIT_EXPECT_EQ(test, err, 0);
	// KUNIT_EXPECT_EQ(test, -1, 0);
}
