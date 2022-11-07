int virtnet_probe_caller(struct virtio_device *vdev)
{
	return virtnet_probe(vdev);
}
EXPORT_SYMBOL_GPL(virtnet_probe_caller);
