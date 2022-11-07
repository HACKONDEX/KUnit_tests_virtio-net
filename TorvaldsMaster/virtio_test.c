#include <kunit/test.h>
#include <linux/device.h>
#include <linux/virtio_config.h>

#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/ethtool.h>
#include <linux/module.h>
#include <linux/virtio.h>
#include <linux/virtio_net.h>
#include <linux/bpf.h>
#include <linux/bpf_trace.h>
#include <linux/scatterlist.h>
#include <linux/if_vlan.h>
#include <linux/slab.h>
#include <linux/cpu.h>
#include <linux/average.h>
#include <linux/filter.h>
#include <linux/kernel.h>
#include <net/route.h>
#include <net/xdp.h>
#include <net/net_failover.h>

#include "../../fs/kernfs/kernfs-internal.h"
#include "../../drivers/base/base.h"

static void sum_example_test(struct kunit *test)
{
	KUNIT_EXPECT_EQ(test, 2, 2);
}

///// structs

struct virtnet_sq_stats {
	struct u64_stats_sync syncp;
	u64 packets;
	u64 bytes;
	u64 xdp_tx;
	u64 xdp_tx_drops;
	u64 kicks;
	u64 tx_timeouts;
};

struct virtnet_rq_stats {
	struct u64_stats_sync syncp;
	u64 packets;
	u64 bytes;
	u64 drops;
	u64 xdp_packets;
	u64 xdp_tx;
	u64 xdp_redirects;
	u64 xdp_drops;
	u64 kicks;
};


/* Internal representation of a send virtqueue */
struct send_queue {
	/* Virtqueue associated with this send _queue */
	struct virtqueue *vq;

	/* TX: fragments + linear part + virtio header */
	struct scatterlist sg[MAX_SKB_FRAGS + 2];

	/* Name of the send queue: output.$index */
	char name[40];

	struct virtnet_sq_stats stats;

	struct napi_struct napi;
};

/* Internal representation of a receive virtqueue */
struct receive_queue {
	/* Virtqueue associated with this receive_queue */
	struct virtqueue *vq;

	struct napi_struct napi;

	struct bpf_prog __rcu *xdp_prog;

	struct virtnet_rq_stats stats;

	/* Chain pages by the private ptr. */
	struct page *pages;

	/* Average packet length for mergeable receive buffers. */
	//struct ewma_pkt_len mrg_avg_pkt_len; // replaced (struct ewma_pkt_len)
	u64 x;

	/* Page frag for packet buffer allocation. */
	struct page_frag alloc_frag;

	/* RX: fragments + linear part + virtio header */
	struct scatterlist sg[MAX_SKB_FRAGS + 2];

	/* Min single buffer size for mergeable buffers case. */
	unsigned int min_buf_len;

	/* Name of this receive queue: input.$index */
	char name[40];

	struct xdp_rxq_info xdp_rxq;
};

struct virtnet_info {
	struct virtio_device *vdev;
	struct virtqueue *cvq;
	struct net_device *dev;
	struct send_queue *sq;
	struct receive_queue *rq;
	unsigned int status;

	/* Max # of queue pairs supported by the device */
	u16 max_queue_pairs;

	/* # of queue pairs currently used by the driver */
	u16 curr_queue_pairs;

	/* # of XDP queue pairs currently used by the driver */
	u16 xdp_queue_pairs;

	/* xdp_queue_pairs may be 0, when xdp is already loaded. So add this. */
	bool xdp_enabled;

	/* I like... big packets and I cannot lie! */
	bool big_packets;

	/* Host will merge rx buffers for big packets (shake it! shake it!) */
	bool mergeable_rx_bufs;

	/* Host supports rss and/or hash report */
	bool has_rss;
	bool has_rss_hash_report;
	u8 rss_key_size;
	u16 rss_indir_table_size;
	u32 rss_hash_types_supported;
	u32 rss_hash_types_saved;

	/* Has control virtqueue */
	bool has_cvq;

	/* Host can handle any s/g split between our header and packet data */
	bool any_header_sg;

	/* Packet virtio header size */
	u8 hdr_len;

	/* Work struct for refilling if we run low on memory. */
	struct delayed_work refill;

	/* Work struct for config space updates */
	struct work_struct config_work;

	/* Does the affinity hint is set for virtqueues? */
	bool affinity_hint_set;

	/* CPU hotplug instances for online & dead */
	struct hlist_node node;
	struct hlist_node node_dead;

	struct control_buf *ctrl;

	/* Ethtool settings */
	u8 duplex;
	u32 speed;

	unsigned long guest_offloads;
	unsigned long guest_offloads_capable;

	/* failover when STANDBY feature enabled */
	struct failover *failover;
};

void klist_get_test(struct klist_node* n) {
	printk("\nLOG: In virtio_net.c klist_get_test *** FAKE METHOD ***\n");
}

int virtnet_test_find_vqs_test(struct virtio_device *dev, unsigned nvqs,
			struct virtqueue *vqs[], vq_callback_t *callbacks[],
			const char * const names[], const bool *ctx,
			struct irq_affinity *desc) 
{
    struct virtnet_info *vinfo = (struct virtnet_info *)(dev->priv);

	// Init queues
	for (int i = 0; i < nvqs; ++i) {
		vqs[i] = kcalloc(1, sizeof(struct virtqueue), GFP_KERNEL);
		vqs[i]->vdev = dev;
	}


    vinfo->rq[0].vq = vqs[0];
    vinfo->sq[0].vq = vqs[1];

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
    // return virtnet_test_find_vqs(dev, nvqs, vqs, callbacks, names, ctx, desc);
    return virtnet_test_find_vqs_test(dev, nvqs, vqs, callbacks, names, ctx, desc);
}

void test_del_vqs(struct virtio_device *vdev) {
    printk("\nLOG:---Del VQS---\n");
}

void test_synchronize_cbs(struct virtio_device *vdev) {
    printk("\nLOG:---Synchronize CBS---\n");
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

static const struct virtio_config_ops VIRTIO_TEST_CONFIG_OPS = {
	.get = test_get,
	.set = test_set,
    .generation = test_generation,
	.get_status = test_get_status,
	.set_status = test_set_status,
	.reset = test_reset,
	.find_vqs = test_find_vqs,
	.del_vqs = test_del_vqs,
    .synchronize_cbs = test_synchronize_cbs,
	.get_features = test_get_features,
	.finalize_features = test_finalize_features,
	.bus_name = test_bus_name,
    .set_vq_affinity = test_set_vq_affinity,
    .get_shm_region = test_get_shm_region,
};

static void virnet_probe_test(struct kunit *test)
{
    printk("\n\n\nStart Virtnet_Probe_Test\n\n\n");
    // device initialization
    struct virtio_device dev_s = {.config = &VIRTIO_TEST_CONFIG_OPS, };
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
                                          VIRTIO_NET_F_STATUS};
    // struct virtio_driver drv_s = {.feature_table = feature_table};
    // drv_s.feature_table_size = 16;
    struct virtio_driver drv_s = {.feature_table_legacy = feature_table};
    drv_s.feature_table_size_legacy = 16;
    dev_s.dev.driver = &drv_s.driver;


    // virtio_device initialization
    // struct virtio_device *dev = dev_to_virtio(&dev_s.dev);
    struct virtio_device *dev = &dev_s;
    struct virtio_driver *drv = drv_to_virtio(dev->dev.driver);
    u64 device_features;
	u64 driver_features;
	u64 driver_features_legacy;
    int err = 0;

    // err = virtio_features_ok_caller(dev);
	// if (err) {
	// 	KUNIT_EXPECT_EQ(test, 4, 0);
    //     return; 
    // }

    // Virtnet probe test
    err = virtnet_probe_caller(dev);
    printk("\nLOG: TEST probe finished \n");
    KUNIT_EXPECT_EQ(test, err, 0);
    KUNIT_EXPECT_EQ(test, 0, 0);
}

static struct kunit_case example_test_cases[] = {
	KUNIT_CASE(sum_example_test),
    KUNIT_CASE(virnet_probe_test),
	{}
};

static struct kunit_suite example_test_suite = {
	.name = "virtio test",
	.test_cases = example_test_cases,
};

kunit_test_suites(&example_test_suite);

MODULE_LICENSE("GPL");
