#include <kunit/test.h>

#include <linux/device.h>
#include <linux/virtio_config.h>

#include <linux/virtio_ring.h>

#include "../../fs/kernfs/kernfs-internal.h"
#include "../../drivers/base/base.h"

// from virtio_ring.c

struct vring_desc_state_split {
	void *data;			/* Data for callback. */
	struct vring_desc *indir_desc;	/* Indirect descriptor, if any. */
};

struct vring_desc_extra {
	dma_addr_t addr;		/* Descriptor DMA addr. */
	u32 len;			/* Descriptor length. */
	u16 flags;			/* Descriptor flags. */
	u16 next;			/* The next desc state in a list. */
};

struct vring_virtqueue_split {
	/* Actual memory layout for this queue. */
	struct vring vring;

	/* Last written value to avail->flags */
	u16 avail_flags_shadow;

	/*
	 * Last written value to avail->idx in
	 * guest byte order.
	 */
	u16 avail_idx_shadow;

	/* Per-descriptor state. */
	struct vring_desc_state_split *desc_state;
	struct vring_desc_extra *desc_extra;

	/* DMA address and size information */
	dma_addr_t queue_dma_addr;
	size_t queue_size_in_bytes;

	/*
	 * The parameters for creating vrings are reserved for creating new
	 * vring.
	 */
	u32 vring_align;
	bool may_reduce_num;
};

struct vring_virtqueue_packed {
	/* Actual memory layout for this queue. */
	struct {
		unsigned int num;
		struct vring_packed_desc *desc;
		struct vring_packed_desc_event *driver;
		struct vring_packed_desc_event *device;
	} vring;

	/* Driver ring wrap counter. */
	bool avail_wrap_counter;

	/* Avail used flags. */
	u16 avail_used_flags;

	/* Index of the next avail descriptor. */
	u16 next_avail_idx;

	/*
	 * Last written value to driver->flags in
	 * guest byte order.
	 */
	u16 event_flags_shadow;

	/* Per-descriptor state. */
	struct vring_desc_state_packed *desc_state;
	struct vring_desc_extra *desc_extra;

	/* DMA address and size information */
	dma_addr_t ring_dma_addr;
	dma_addr_t driver_event_dma_addr;
	dma_addr_t device_event_dma_addr;
	size_t ring_size_in_bytes;
	size_t event_size_in_bytes;
};

struct vring_virtqueue {
	struct virtqueue vq;

	/* Is this a packed ring? */
	bool packed_ring;

	/* Is DMA API used? */
	bool use_dma_api;

	/* Can we use weak barriers? */
	bool weak_barriers;

	/* Other side has made a mess, don't try any more. */
	bool broken;

	/* Host supports indirect buffers */
	bool indirect;

	/* Host publishes avail event idx */
	bool event;

	/* Head of free buffer list. */
	unsigned int free_head;
	/* Number we've added since last sync. */
	unsigned int num_added;

	/* Last used index  we've seen.
	 * for split ring, it just contains last used index
	 * for packed ring:
	 * bits up to VRING_PACKED_EVENT_F_WRAP_CTR include the last used index.
	 * bits from VRING_PACKED_EVENT_F_WRAP_CTR include the used wrap counter.
	 */
	u16 last_used_idx;

	/* Hint for event idx: already triggered no need to disable. */
	bool event_triggered;

	union {
		/* Available for split ring */
		struct vring_virtqueue_split split;

		/* Available for packed ring */
		struct vring_virtqueue_packed packed;
	};

	/* How to notify other side. FIXME: commonalize hcalls! */
	bool (*notify)(struct virtqueue *vq);

	/* DMA, allocation, and size information */
	bool we_own_ring;

#ifdef DEBUG
	/* They're supposed to lock for us. */
	unsigned int in_use;

	/* Figure out if their kicks are too delayed. */
	bool last_add_time_valid;
	ktime_t last_add_time;
#endif
};

// struct vring_virtqueue {
// 	struct virtqueue vq;

// 	/* Is this a packed ring? */
// 	bool packed_ring;

// 	/* Is DMA API used? */
// 	bool use_dma_api;

// 	/* Can we use weak barriers? */
// 	bool weak_barriers;

// 	/* Other side has made a mess, don't try any more. */
// 	bool broken;

// 	/* Host supports indirect buffers */
// 	bool indirect;

// 	/* Host publishes avail event idx */
// 	bool event;

// 	/* Head of free buffer list. */
// 	unsigned int free_head;
// 	/* Number we've added since last sync. */
// 	unsigned int num_added;

// 	/* Last used index we've seen. */
// 	u16 last_used_idx;

// 	union {
// 		/* Available for split ring */
// 		struct {
// 			/* Actual memory layout for this queue. */
// 			struct vring vring;

// 			/* Last written value to avail->flags */
// 			u16 avail_flags_shadow;

// 			/*
// 			 * Last written value to avail->idx in
// 			 * guest byte order.
// 			 */
// 			u16 avail_idx_shadow;

// 			/* Per-descriptor state. */
// 			struct vring_desc_state_split *desc_state;

// 			/* DMA address and size information */
// 			dma_addr_t queue_dma_addr;
// 			size_t queue_size_in_bytes;
// 		} split;

// 		/* Available for packed ring */
// 		struct {
// 			/* Actual memory layout for this queue. */
// 			struct {
// 				unsigned int num;
// 				struct vring_packed_desc *desc;
// 				struct vring_packed_desc_event *driver;
// 				struct vring_packed_desc_event *device;
// 			} vring;

// 			/* Driver ring wrap counter. */
// 			bool avail_wrap_counter;

// 			/* Device ring wrap counter. */
// 			bool used_wrap_counter;

// 			/* Avail used flags. */
// 			u16 avail_used_flags;

// 			/* Index of the next avail descriptor. */
// 			u16 next_avail_idx;

// 			/*
// 			 * Last written value to driver->flags in
// 			 * guest byte order.
// 			 */
// 			u16 event_flags_shadow;

// 			/* Per-descriptor state. */
// 			struct vring_desc_state_packed *desc_state;
// 			struct vring_desc_extra_packed *desc_extra;

// 			/* DMA address and size information */
// 			dma_addr_t ring_dma_addr;
// 			dma_addr_t driver_event_dma_addr;
// 			dma_addr_t device_event_dma_addr;
// 			size_t ring_size_in_bytes;
// 			size_t event_size_in_bytes;
// 		} packed;
// 	};

// 	/* How to notify other side. FIXME: commonalize hcalls! */
// 	bool (*notify)(struct virtqueue *vq);

// 	/* DMA, allocation, and size information */
// 	bool we_own_ring;

// #ifdef DEBUG
// 	/* They're supposed to lock for us. */
// 	unsigned int in_use;

// 	/* Figure out if their kicks are too delayed. */
// 	bool last_add_time_valid;
// 	ktime_t last_add_time;
// #endif
// };

#define to_vvq(_vq) container_of(_vq, struct vring_virtqueue, vq)


static const int KERNFS_ROOT_MEM_SIZE = 120;

void klist_get_dummy(struct klist_node* n) {
	printk("\nLOG: In virtio_net.c klist_get_test *** FAKE METHOD ***\n");
}

int virtnet_test_find_vqs_test(struct virtio_device *dev, unsigned nvqs,
			struct virtqueue *vqs[], vq_callback_t *callbacks[],
			const char * const names[], const bool *ctx,
			struct irq_affinity *desc) 
{
    struct virtnet_info *vinfo;
    struct vring_virtqueue* vring_vq;
    struct net_device* net_dev;
    struct device* init_dev;
    struct kernfs_node* kn;
    struct klist_node* k_node;
	int i;

    vinfo = (struct virtnet_info *)(dev->priv);

	// Init queues
	for (i = 0; i < nvqs; ++i) {
        vring_vq = kcalloc(1, sizeof(struct vring_virtqueue), GFP_KERNEL);
        vqs[i] = &(vring_vq->vq);
        vqs[i]->vdev = dev;
	}

    vinfo->rq[0].vq = vqs[0];
	vring_vq = to_vvq(vinfo->rq[0].vq);

	if (vring_vq->packed_ring)
		vring_vq->packed.vring.num = 1;
    else
		vring_vq->split.vring.num = 1;
    
    vinfo->sq[0].vq = vqs[1];

	// init kobject
	net_dev = vinfo->dev;
	init_dev = &net_dev->dev;
	kn = kcalloc(1, sizeof(struct kernfs_node), GFP_KERNEL);

	init_dev->parent->kobj.sd = kn;
	kn->parent = NULL;
	
	kn->dir.root = kcalloc(1, KERNFS_ROOT_MEM_SIZE, GFP_KERNEL);
	kn->dir.root->ino_idr.idr_rt.xa_flags = IDR_RT_MARKER;
	atomic_set(&kn->count, 1);

	// Init kernfs_node flags(-EINVAL otherwise)
	kn->flags = 17;
	// Init klist objects
	init_dev->parent->p = kcalloc(1, sizeof(struct device_private), GFP_KERNEL);
	init_dev->parent->p->klist_children.get = klist_get_dummy;
	
	k_node = kcalloc(1, sizeof(struct klist_node), GFP_KERNEL);
	init_dev->parent->p->klist_children.k_list.prev = &(k_node->n_node);

	// Init parent name
	init_dev->parent->init_name = "DummyParentDevice";

	return 0;           
}

//// VIRTIO_CONFGI_OPS methods

void test_get(struct virtio_device *vdev, unsigned offset,
		    void *buf, unsigned len) {
    printk("\nLOG:---Get---\n");
}

void test_set(struct virtio_device *vdev, unsigned offset,
		    const void *buf, unsigned len) {
    printk("\nLOG:---Set---\n");
}

u32 test_generation(struct virtio_device *vdev) {
    printk("\nLOG---Generation---\n");
    return 0;
}

u8 test_get_status(struct virtio_device *vdev) {
    printk("\nLOG:---Get Status---\n");
    return 0;
}

void test_set_status(struct virtio_device *vdev, u8 status) {
    printk("\nLOG:---Set Status---\n");
}

void test_reset(struct virtio_device *vdev) {
    printk("\nLOG:---Reset---\n");
}

int test_find_vqs(struct virtio_device *dev, unsigned nvqs,
			struct virtqueue *vqs[], vq_callback_t *callbacks[],
			const char * const names[], const bool *ctx,
			struct irq_affinity *desc) {
    printk("\nLOG:---Find VQS---\n");
    return virtnet_test_find_vqs_test(dev, nvqs, vqs, callbacks, names, ctx, desc);
}

void test_del_vqs(struct virtio_device *vdev) {
    printk("\nLOG:---Del VQS---\n");
}

u64 test_get_features(struct virtio_device *vdev) {
    printk("\nLOG:---Get Features---\n");
    return vdev->features;
}

int test_finalize_features(struct virtio_device *vdev) {
    printk("\nLOG:---Finalize Features---\n");
    return 0;
}

const char *test_bus_name(struct virtio_device *vdev) {
    printk("\nLOG:---Bus Name---\n");
    return NULL;
}

int test_set_vq_affinity(struct virtqueue *vq,
			       const struct cpumask *cpu_mask) {
    printk("\nLOG:---Set VQ Affinity---\n");  
    return 0;
}

const struct cpumask *test_get_vq_affinity(struct virtio_device *vdev,
			int index) {
    printk("\nLOG:---Get VQ Affinity---\n");
    return NULL;
}

bool test_get_shm_region(struct virtio_device *vdev,
			       struct virtio_shm_region *region, u8 id) {
    printk("\nLOG:---Get SHM Region---\n");
    return 0;
}

///

static const struct virtio_config_ops VIRTIO_TEST_MAIN_CONFIG_OPS = {
	.get = test_get,
	.set = test_set,
    .generation = test_generation,
	.get_status = test_get_status,
	.set_status = test_set_status,
	.reset = test_reset,
	.find_vqs = test_find_vqs,
	.del_vqs = test_del_vqs,
	.get_features = test_get_features,
	.finalize_features = test_finalize_features,
	.bus_name = test_bus_name,
    .set_vq_affinity = test_set_vq_affinity,
    .get_shm_region = test_get_shm_region,
};

// Test main
static void virnet_probe_test_main(struct kunit *test)
{
    struct virtio_device *dev;
    struct virtio_driver *drv;
    const unsigned int feature_table[] = {VIRTIO_NET_F_MQ, 
                                          VIRTIO_NET_F_RSS,
                                          VIRTIO_NET_F_CTRL_VQ,
                                          VIRTIO_NET_F_CSUM,
                                          VIRTIO_NET_F_GUEST_CSUM,
                                          VIRTIO_NET_F_GUEST_TSO4,
                                          VIRTIO_NET_F_GUEST_TSO6,
                                          VIRTIO_NET_F_CTRL_GUEST_OFFLOADS,
                                          VIRTIO_NET_F_MAC,
                                          VIRTIO_NET_F_CTRL_MAC_ADDR, 
                                          VIRTIO_NET_F_GUEST_ECN,
                                          VIRTIO_NET_F_GUEST_UFO,
                                          VIRTIO_NET_F_MRG_RXBUF,
                                          VIRTIO_NET_F_HASH_REPORT,
                                          VIRTIO_F_ANY_LAYOUT,
                                          VIRTIO_NET_F_MTU,
                                          VIRTIO_NET_F_STATUS};
    struct virtio_device dev_s = {.config = &VIRTIO_TEST_MAIN_CONFIG_OPS};
    struct virtio_driver drv_s = {.feature_table_legacy = feature_table};
    int err = 0;

    // device initialization
    device_initialize(&dev_s.dev);
    dev_s.dev.parent = NULL;
    dev_s.dev.kobj.parent = NULL;

    /// Features for device_driver
    

    drv_s.feature_table_size_legacy = 17;
    drv_s.feature_table_size = 0;
    dev_s.dev.driver = &drv_s.driver;


    // virtio_device initialization
    dev = &dev_s;
    drv = drv_to_virtio(dev->dev.driver);

    // Virtnet probe test
    err = virtnet_probe(dev);

    KUNIT_EXPECT_EQ(test, err, 0);
}

///////////////////// Test 1 /////////////////////

void test_get_1(struct virtio_device *vdev, unsigned offset,
		    void *buf, unsigned len) {
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
    struct virtnet_info *vinfo;
    struct vring_virtqueue* vring_vq;
    struct vring_virtqueue* vring_cvq;
    struct vring_virtqueue *cvq_vvrq;
    struct virtqueue* cvq;

    struct net_device* net_dev;
    struct device* init_dev;
    struct kernfs_node *kn;
    struct klist_node* k_node;
    int i, j;

    vinfo = (struct virtnet_info *)(dev->priv);

    for (i = 0; i < nvqs; ++i) {
        vring_vq = kcalloc(1, sizeof(struct vring_virtqueue), GFP_KERNEL);
        vqs[i] = &(vring_vq->vq);
        vqs[i]->vdev = dev;
    }

	// Init queues

    i = 0;
	j = 0;
	for (; i < nvqs / 2; ++i) {
		vinfo->rq[i].vq = vqs[j];
		vinfo->sq[i].vq = vqs[j + 1];
		{
			vring_vq = to_vvq(vqs[j]);
			vring_vq->notify = test_notify_1;
			vring_vq = to_vvq(vqs[j + 1]);
			vring_vq->notify = test_notify_1;

		}
		j += 2;

        // Init revceive queue
		vring_vq = to_vvq(vinfo->rq[i].vq);
        vring_vq->notify = test_notify_1;
		if (vring_vq->packed_ring) {
			vring_vq->packed.vring.num = 1;
		} else {
			vring_vq->split.vring.num = 1;
		}

        // Init send queue
		vring_vq = to_vvq(vinfo->sq[i].vq);
        vring_vq->notify = test_notify_1;
		if (vring_vq->packed_ring) {
			vring_vq->packed.vring.num = 1;
		} else {
			vring_vq->split.vring.num = 1;
		}
	}

	// Init dev->cvq
    vring_cvq = kcalloc(1, sizeof(struct vring_virtqueue), GFP_KERNEL);
    vinfo->cvq = &(vring_cvq->vq);
    cvq = vinfo->cvq;
    cvq->vdev = dev;
    vring_vq = to_vvq(cvq);
    vring_vq->notify = test_notify_1;

	cvq = vqs[nvqs - 1];
	cvq_vvrq = to_vvq(cvq);
	cvq_vvrq->indirect = true;
	cvq_vvrq->use_dma_api = false;
	cvq_vvrq->vq.num_free = 1;
	cvq_vvrq->notify = test_notify_1;

	cvq_vvrq->split.desc_state = kcalloc(1, sizeof(struct vring_desc_state_split), GFP_KERNEL);
	cvq_vvrq->split.vring.desc = kcalloc(1, sizeof(struct vring_desc), GFP_KERNEL);
    cvq_vvrq->split.desc_extra = kcalloc(1, sizeof(struct vring_desc_extra), GFP_KERNEL);
    cvq_vvrq->split.vring.avail = kcalloc(1, sizeof(vring_avail_t) + 2 * sizeof(__virtio16), GFP_KERNEL);
    cvq_vvrq->split.vring.used = kcalloc(1, sizeof(vring_used_t) + 2 * sizeof(vring_used_elem_t), GFP_KERNEL);

    cvq_vvrq->last_used_idx = 1;
    cvq_vvrq->split.vring.num = 1;

    // cvq_vvrq->num_added =  (1 << 16) - 2;
	// cvq_vvrq->split.vring.num = 1;


	// init kobject
	net_dev = vinfo->dev;
	init_dev = &net_dev->dev;
	kn = kcalloc(1, sizeof(struct kernfs_node), GFP_KERNEL);

	init_dev->parent->kobj.sd = kn;
	kn->parent = NULL;

	kn->dir.root = kcalloc(1, KERNFS_ROOT_MEM_SIZE, GFP_KERNEL);
	kn->dir.root->ino_idr.idr_rt.xa_flags = IDR_RT_MARKER;
	atomic_set(&kn->count, 1);

	// Init kernfs_node
	kn->flags = 17;
	// Init klist objects
	init_dev->parent->p = kcalloc(1, sizeof(struct device_private), GFP_KERNEL);
	init_dev->parent->p->klist_children.get = klist_get_dummy;
	
	k_node = kcalloc(1, sizeof(struct klist_node), GFP_KERNEL);
	init_dev->parent->p->klist_children.k_list.prev = &(k_node->n_node);

	// Init parent name
	init_dev->parent->init_name = "ParentDevice\0";

	return 0;           
}

void test_reset_1(struct virtio_device *vdev) {

}

static const struct virtio_config_ops VIRTIO_TEST_1_CONFIG_OPS = {
	.get = test_get_1,
	.get_status = test_get_status,
	.set_status = test_set_status,
	.reset = test_reset_1,
	.find_vqs = test_find_vqs_1,
    .set_vq_affinity = test_set_vq_affinity,
    .get_shm_region = test_get_shm_region,
	};

static void virtnet_probe_test_1(struct kunit *test)
{
    struct virtio_device *dev;
    struct virtio_driver *drv;
    const unsigned int feature_table[] = {VIRTIO_NET_F_MQ, 
                                          VIRTIO_NET_F_RSS,
                                          VIRTIO_NET_F_CTRL_VQ,
                                          VIRTIO_NET_F_CSUM,
                                          VIRTIO_NET_F_GUEST_CSUM,
                                          VIRTIO_NET_F_GUEST_TSO4,
                                          VIRTIO_NET_F_GUEST_TSO6,
                                          VIRTIO_NET_F_CTRL_GUEST_OFFLOADS,
                                          VIRTIO_NET_F_MAC,
                                          VIRTIO_NET_F_CTRL_MAC_ADDR,
                                          VIRTIO_NET_F_GUEST_ECN,
                                          VIRTIO_NET_F_GUEST_UFO,
                                          VIRTIO_NET_F_MRG_RXBUF,
                                          VIRTIO_NET_F_HASH_REPORT,
                                          VIRTIO_F_ANY_LAYOUT,
                                          VIRTIO_NET_F_MTU,
                                          VIRTIO_NET_F_STATUS,
										  VIRTIO_NET_F_CTRL_VLAN};
    struct virtio_device dev_s = {.config = &VIRTIO_TEST_1_CONFIG_OPS};
    struct virtio_driver drv_s = {.feature_table_legacy = feature_table};
    int err = 0;
    
    device_initialize(&dev_s.dev);
    dev_s.dev.parent = NULL;
    dev_s.dev.kobj.parent = NULL;
    drv_s.feature_table_size_legacy = 18;
    dev_s.dev.driver = &drv_s.driver;

	// Add features
	dev_s.features |= BIT_ULL(VIRTIO_NET_F_MQ);
	dev_s.features |= BIT_ULL(VIRTIO_NET_F_CTRL_VQ);

    // virtio_device initialization
    dev = &dev_s;
    drv = drv_to_virtio(dev->dev.driver);

    // Virtnet probe test
    err = virtnet_probe(dev);

    KUNIT_EXPECT_EQ(test, err, 0);
}

///////////////////// Test 2 /////////////////////

int test_find_vqs_2(struct virtio_device *dev, unsigned nvqs,
			struct virtqueue *vqs[], vq_callback_t *callbacks[],
			const char * const names[], const bool *ctx,
			struct irq_affinity *desc) 
{
	return 0;
}

static void virtnet_probe_test_2(struct kunit *test) {
    struct virtio_device *dev;
    struct virtio_driver *drv;
    const unsigned int feature_table[] = {VIRTIO_NET_F_MQ, 
                                          VIRTIO_NET_F_RSS,
                                          VIRTIO_NET_F_CTRL_VQ,
                                          VIRTIO_NET_F_CSUM,
                                          VIRTIO_NET_F_GUEST_CSUM,
                                          VIRTIO_NET_F_GUEST_TSO4,
                                          VIRTIO_NET_F_GUEST_TSO6,
                                          VIRTIO_NET_F_CTRL_GUEST_OFFLOADS,
                                          VIRTIO_NET_F_MAC,
                                          VIRTIO_NET_F_CTRL_MAC_ADDR, 
                                          VIRTIO_NET_F_GUEST_ECN,
                                          VIRTIO_NET_F_GUEST_UFO,
                                          VIRTIO_NET_F_MRG_RXBUF,
                                          VIRTIO_NET_F_HASH_REPORT,
                                          VIRTIO_F_ANY_LAYOUT,
                                          VIRTIO_NET_F_MTU,
                                          VIRTIO_NET_F_STATUS,
										  VIRTIO_NET_F_GSO,
										  VIRTIO_NET_F_HOST_TSO4,
										  VIRTIO_NET_F_HOST_TSO6,
										  VIRTIO_NET_F_HOST_ECN,
										  VIRTIO_NET_F_HOST_USO,
										  };
    struct virtio_device dev_s = {.config = &VIRTIO_TEST_MAIN_CONFIG_OPS}; // Handler functions
    struct virtio_driver drv_s = {.feature_table_legacy = feature_table};
    int err = 0;

    device_initialize(&dev_s.dev);
    dev_s.dev.parent = NULL;
    dev_s.dev.kobj.parent = NULL;

    drv_s.feature_table_size_legacy = 22;
    dev_s.dev.driver = &drv_s.driver;

    // Add features
	dev_s.features |= BIT_ULL(VIRTIO_NET_F_CSUM);

    // virtio_device initialization
    dev = &dev_s;
    drv = drv_to_virtio(dev->dev.driver);

    // Virtnet probe test
    err = virtnet_probe(dev);

    KUNIT_EXPECT_EQ(test, err, 0);
}

///////////////////// Test 3 /////////////////////

// static const struct virtio_config_ops VIRTIO_TEST_3_CONFIG_OPS = {
// 	.get = test_get,
// 	.get_status = test_get_status,
// 	.set_status = test_set_status,
// 	.reset = test_reset_1,
// 	.find_vqs = test_find_vqs_1,
//     .set_vq_affinity = test_set_vq_affinity,
//     .get_shm_region = test_get_shm_region,
// 	};

// static void virtnet_probe_test_3(struct kunit *test)
// {
//     struct virtio_device *dev;
//     struct virtio_driver *drv;
//     const unsigned int feature_table[] = {VIRTIO_NET_F_MQ, 
//                                           VIRTIO_NET_F_RSS,
//                                           VIRTIO_NET_F_CTRL_VQ,
//                                           VIRTIO_NET_F_CSUM,
//                                           VIRTIO_NET_F_GUEST_CSUM,
//                                           VIRTIO_NET_F_GUEST_TSO4,
//                                           VIRTIO_NET_F_GUEST_TSO6,
//                                           VIRTIO_NET_F_CTRL_GUEST_OFFLOADS,
//                                           VIRTIO_NET_F_MAC, 
//                                           VIRTIO_NET_F_GUEST_ECN,
//                                           VIRTIO_NET_F_GUEST_UFO,
//                                           VIRTIO_NET_F_MRG_RXBUF,
//                                           VIRTIO_NET_F_HASH_REPORT,
//                                           VIRTIO_F_ANY_LAYOUT,
//                                           VIRTIO_NET_F_MTU,
//                                           VIRTIO_NET_F_STATUS,
// 										  VIRTIO_NET_F_CTRL_VLAN,
// 										  VIRTIO_NET_F_GSO,
// 										  VIRTIO_NET_F_HOST_TSO4,
// 										  VIRTIO_NET_F_HOST_TSO6,
// 										  VIRTIO_NET_F_HOST_ECN,
// 										  VIRTIO_NET_F_HOST_USO};
//     struct virtio_device dev_s = {.config = &VIRTIO_TEST_3_CONFIG_OPS};
//     struct virtio_driver drv_s = {.feature_table_legacy = feature_table};
//     int err = 0;

//     device_initialize(&dev_s.dev);
//     dev_s.dev.parent = NULL;
//     dev_s.dev.kobj.parent = NULL;

//     drv_s.feature_table_size_legacy = 22;
//     dev_s.dev.driver = &drv_s.driver;

// 	// Add features
// 	dev_s.features |= BIT_ULL(VIRTIO_NET_F_MAC);
// 	dev_s.features |= BIT_ULL(VIRTIO_NET_F_MQ);
// 	dev_s.features |= BIT_ULL(VIRTIO_NET_F_CTRL_VQ);
// 	dev_s.features |= BIT_ULL(VIRTIO_NET_F_CSUM);

//     // virtio_device initialization
//     dev = &dev_s;
//     drv = drv_to_virtio(dev->dev.driver);

//     // Virtnet probe test
//     err = virtnet_probe(dev);

//     KUNIT_EXPECT_EQ(test, err, -12);
// }

// ///////////////////// Test 4 /////////////////////

static void virtnet_probe_test_4(struct kunit *test)
{
    struct virtio_device *dev;
    struct virtio_driver *drv;
    const unsigned int feature_table[] = {VIRTIO_NET_F_MQ, 
                                          VIRTIO_NET_F_RSS,
                                          VIRTIO_NET_F_CTRL_VQ,
                                          VIRTIO_NET_F_CSUM,
                                          VIRTIO_NET_F_GUEST_CSUM,
                                          VIRTIO_NET_F_GUEST_TSO4,
                                          VIRTIO_NET_F_GUEST_TSO6,
                                          VIRTIO_NET_F_CTRL_GUEST_OFFLOADS,
                                          VIRTIO_NET_F_MAC,
                                          VIRTIO_NET_F_CTRL_MAC_ADDR, 
                                          VIRTIO_NET_F_GUEST_ECN,
                                          VIRTIO_NET_F_GUEST_UFO,
                                          VIRTIO_NET_F_MRG_RXBUF,
                                          VIRTIO_NET_F_HASH_REPORT,
                                          VIRTIO_F_ANY_LAYOUT,
                                          VIRTIO_NET_F_MTU,
                                          VIRTIO_NET_F_STATUS,
										  VIRTIO_NET_F_CTRL_VLAN,
										  VIRTIO_NET_F_GSO,
										  VIRTIO_NET_F_HOST_TSO4,
										  VIRTIO_NET_F_HOST_TSO6,
										  VIRTIO_NET_F_HOST_ECN,
										  VIRTIO_NET_F_HOST_USO};
    struct virtio_device dev_s = {.config = &VIRTIO_TEST_1_CONFIG_OPS};
    struct virtio_driver drv_s = {.feature_table_legacy = feature_table};
    int err = 0;

    device_initialize(&dev_s.dev);
    dev_s.dev.parent = NULL;
    dev_s.dev.kobj.parent = NULL;

    /// Features for device_driver
    drv_s.feature_table_size_legacy = 23;
    dev_s.dev.driver = &drv_s.driver;

	// Add features
	dev_s.features |= BIT_ULL(VIRTIO_NET_F_CTRL_VQ);
	dev_s.features |= BIT_ULL(VIRTIO_NET_F_CSUM);

    // virtio_device initialization
    dev = &dev_s;
    drv = drv_to_virtio(dev->dev.driver);

    // Virtnet probe test
    err = virtnet_probe(dev);

    KUNIT_EXPECT_EQ(test, err, 0);
}

///////////////////// Test 5 /////////////////////


static void virtnet_probe_test_5(struct kunit *test)
{
    struct virtio_device *dev;
    struct virtio_driver *drv;
    const unsigned int feature_table[] = {VIRTIO_NET_F_MQ, 
                                          VIRTIO_NET_F_RSS,
                                          VIRTIO_NET_F_CTRL_VQ,
                                          VIRTIO_NET_F_CSUM,
                                          VIRTIO_NET_F_GUEST_CSUM,
                                          VIRTIO_NET_F_GUEST_TSO4,
                                          VIRTIO_NET_F_GUEST_TSO6,
                                          VIRTIO_NET_F_CTRL_GUEST_OFFLOADS,
                                          VIRTIO_NET_F_MAC,
                                          VIRTIO_NET_F_CTRL_MAC_ADDR, 
                                          VIRTIO_NET_F_GUEST_ECN,
                                          VIRTIO_NET_F_GUEST_UFO,
                                          VIRTIO_NET_F_MRG_RXBUF,
                                          VIRTIO_NET_F_HASH_REPORT,
                                          VIRTIO_F_ANY_LAYOUT,
                                          VIRTIO_NET_F_MTU,
                                          VIRTIO_NET_F_STATUS,
										  VIRTIO_NET_F_CTRL_VLAN,
										  VIRTIO_NET_F_GSO,
										  VIRTIO_NET_F_HOST_TSO4,
										  VIRTIO_NET_F_HOST_TSO6,
										  VIRTIO_NET_F_HOST_ECN,
										  VIRTIO_NET_F_HOST_USO};
    struct virtio_device dev_s = {.config = &VIRTIO_TEST_1_CONFIG_OPS};
    struct virtio_driver drv_s = {.feature_table_legacy = feature_table}; 
    int err = 0;

    device_initialize(&dev_s.dev);
    dev_s.dev.parent = NULL;
    dev_s.dev.kobj.parent = NULL;

    drv_s.feature_table_size_legacy = 23;
    dev_s.dev.driver = &drv_s.driver;

	// Add features
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

    // Virtnet probe test
    err = virtnet_probe(dev);

    KUNIT_EXPECT_EQ(test, err, 0);
}

static void sum_example_test(struct kunit *test)
{
	KUNIT_EXPECT_EQ(test, 2, 2);
}

static struct kunit_case example_test_cases[] = {
	KUNIT_CASE(sum_example_test),
	KUNIT_CASE(virnet_probe_test_main), // in 3735
	KUNIT_CASE(virtnet_probe_test_1), // out 3735, out 3755
	KUNIT_CASE(virtnet_probe_test_2), // in 3755, in 3755
	// KUNIT_CASE(virtnet_probe_test_3), // out 3735, in 3755
	KUNIT_CASE(virtnet_probe_test_4), // out 3730
	KUNIT_CASE(virtnet_probe_test_5), 
    {}

};

static struct kunit_suite example_test_suite = {
	.name = "virtio_net_test",
	.test_cases = example_test_cases,
};

kunit_test_suites(&example_test_suite);

MODULE_LICENSE("GPL");