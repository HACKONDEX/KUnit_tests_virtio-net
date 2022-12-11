#include <kunit/test.h>
#include <linux/device.h>
#include <linux/virtio_config.h>

#include <linux/virtio_ring.h>

#include "../../fs/kernfs/kernfs-internal.h"
#include "../../drivers/base/base.h"

#define to_vvq(_vq) container_of(_vq, struct vring_virtqueue, vq)

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

    /* Last used index we've seen. */
    u16 last_used_idx;

    union {
        /* Available for split ring */
        struct {
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

            /* DMA address and size information */
            dma_addr_t queue_dma_addr;
            size_t queue_size_in_bytes;
        } split;

        /* Available for packed ring */
        struct {
            /* Actual memory layout for this queue. */
            struct {
                unsigned int num;
                struct vring_packed_desc *desc;
                struct vring_packed_desc_event *driver;
                struct vring_packed_desc_event *device;
            } vring;

            /* Driver ring wrap counter. */
            bool avail_wrap_counter;

            /* Device ring wrap counter. */
            bool used_wrap_counter;

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
            struct vring_desc_extra_packed *desc_extra;

            /* DMA address and size information */
            dma_addr_t ring_dma_addr;
            dma_addr_t driver_event_dma_addr;
            dma_addr_t device_event_dma_addr;
            size_t ring_size_in_bytes;
            size_t event_size_in_bytes;
        } packed;
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

static void sum_example_test(struct kunit *test)
{
    KUNIT_EXPECT_EQ(test, 2, 2);
}

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
    struct vring_virtqueue *vvrq = to_vvq(vinfo->rq[0].vq);
    if (vvrq->packed_ring) {
        vvrq->packed.vring.num = 1;
    } else {
        vvrq->split.vring.num = 1;
    }
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
    // drv_s.feature_table_size = sizeof(feature_table);
    struct virtio_driver drv_s = {.feature_table_legacy = feature_table};
    drv_s.feature_table_size_legacy = sizeof(feature_table);
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
    //  KUNIT_EXPECT_EQ(test, 4, 0);
    //     return; 
    // }

    // Virtnet probe test
    err = virtnet_probe(dev);
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
