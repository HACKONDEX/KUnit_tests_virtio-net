#include <kunit/test.h>

#include <linux/device.h>
#include <linux/virtio_config.h>
#include <linux/virtio_ring.h>
#include <linux/device.h>
#include <linux/virtio_config.h>
#include <linux/virtio_ring.h>
#include "../../fs/kernfs/kernfs-internal.h"
#include "../../drivers/base/base.h"

#define to_vvq(_vq) container_of(_vq, struct vring_virtqueue, vq)

static const int KERNFS_ROOT_MEM_SIZE = 120;

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
