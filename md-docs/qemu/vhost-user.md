
# vhost-user

```txt
main [system/main.c]
  qemu_init [system/vl.c]
    qmp_x_exit_preconfig [system/vl.c]
      qemu_create_cli_devices [system/vl.c]
        qemu_opts_foreach [util/qemu-option.c]
          device_init_func [system/vl.c]
            qdev_device_add [system/qdev-monitor.c]
              qdev_device_add_from_qdict [system/qdev-monitor.c]
                qdev_realize [hw/core/qdev.c]
                  object_property_set_bool [qom/object.c]
                    object_property_set_qobject [qom/qom-qobject.c]
                      object_property_set [qom/object.c]
                        property_set_bool [qom/object.c]
                          device_set_realized [hw/core/qdev.c]
                            virtio_pci_dc_realize [hw/virtio/virtio-pci.c]
                              pci_qdev_realize [hw/pci/pci.c]
                                virtio_pci_realize [hw/virtio/virtio-pci.c]
                                  vhost_user_fs_pci_realize [hw/virtio/vhost-user-fs-pci.c]
                                    qdev_realize [hw/core/qdev.c]
                                      object_property_set_bool [qom/object.c]
                                        object_property_set_qobject [qom/qom-qobject.c]
                                          object_property_set [qom/object.c]
                                            property_set_bool [qom/object.c]
                                              device_set_realized [hw/core/qdev.c]
                                                virtio_device_realize [hw/virtio/virtio.c]
                                                  vuf_device_realize [hw/virtio/vhost-user-fs.c]
                                                    vhost_dev_init [hw/virtio/vhost.c]
                                                      vhost_user_backend_init [hw/virtio/vhost-user.c]
```

```txt
main [system/main.c]
  qemu_init [system/vl.c]
    qemu_create_early_backends [system/vl.c]
      qemu_opts_foreach [util/qemu-option.c]
        chardev_init_func [system/vl.c]
          qemu_chr_new_from_opts [chardev/char.c]
            qemu_chardev_new [chardev/char.c]
              chardev_new [chardev/char.c]
                qemu_char_open [chardev/char.c]
                  qmp_chardev_open_socket [chardev/char-socket.c]
                    qmp_chardev_open_socket_client [chardev/char-socket.c]
                      tcp_chr_connect_client_sync [chardev/char-socket.c]
                        qio_channel_socket_connect_sync [io/channel-socket.c]
                          socket_connect [util/qemu-sockets.c]
                            unix_connect_saddr [util/qemu-sockets.c]
                              error_setg_errno_internal [util/error.c]
                                error_setv [util/error.c]
                                  error_handle [util/error.c]
```

vhost_dev_init

```txt
================ Vhost user message ================
Request: VHOST_USER_GET_FEATURES (1)
Flags:   0x1
Size:    0
Sending back to guest u64: 0x0000000175000000
================ Vhost user message ================
Request: VHOST_USER_GET_PROTOCOL_FEATURES (15)
Flags:   0x1
Size:    0
================ Vhost user message ================
Request: VHOST_USER_SET_PROTOCOL_FEATURES (16)
Flags:   0x1
Size:    8
u64: 0x0000000000008c2b
================ Vhost user message ================
Request: VHOST_USER_GET_QUEUE_NUM (17)
Flags:   0x1
Size:    0
================ Vhost user message ================
Request: VHOST_USER_GET_MAX_MEM_SLOTS (36)
Flags:   0x1
Size:    0
u64: 0x00000000000001fd
================ Vhost user message ================
Request: VHOST_USER_SET_BACKEND_REQ_FD (21)
Flags:   0x9
Size:    0
Fds: 7
Got backend_fd: 7
================ Vhost user message ================
Request: VHOST_USER_SET_OWNER (3)
Flags:   0x1
Size:    0
================ Vhost user message ================
Request: VHOST_USER_GET_FEATURES (1)
Flags:   0x1
Size:    0
Sending back to guest u64: 0x0000000175000000
================ Vhost user message ================
Request: VHOST_USER_SET_VRING_CALL (13)
Flags:   0x1
Size:    8
Fds: 8
u64: 0x0000000000000000
Got call_fd: 8 for vq: 0
================ Vhost user message ================
Request: VHOST_USER_SET_VRING_ERR (14)
Flags:   0x1
Size:    8
Fds: 9
u64: 0x0000000000000000
================ Vhost user message ================
Request: VHOST_USER_SET_VRING_CALL (13)
Flags:   0x1
Size:    8
Fds: 10
u64: 0x0000000000000001
Got call_fd: 10 for vq: 1
================ Vhost user message ================
Request: VHOST_USER_SET_VRING_ERR (14)
Flags:   0x1
Size:    8
Fds: 11
u64: 0x0000000000000001
```

```c{6,9,10}
void vu_set_queue_handler(VuDev *dev, VuVirtq *vq,
                          vu_queue_handler_cb handler)
{
    int qidx = vq - dev->vq;

    vq->handler = handler;
    if (vq->kick_fd >= 0) {
        if (handler) {
            dev->set_watch(dev, vq->kick_fd, VU_WATCH_IN,
                           vu_kick_cb, (void *)(long)qidx);
        } else {
            dev->remove_watch(dev, vq->kick_fd);
        }
    }
}
```

```c{14,13}
#define container_of()
static void
set_watch(VuDev *vu_dev, int fd, int vu_evt, vu_watch_cb cb, void *pvt)
{
    GSource *src;
    VugDev *dev;

    g_assert(vu_dev);
    g_assert(fd >= 0);
    g_assert(cb);

    dev = container_of(vu_dev, VugDev, parent);
    src = vug_source_new(dev, fd, vu_evt, cb, pvt);
    g_hash_table_replace(dev->fdmap, GINT_TO_POINTER(fd), src);
}
```

```c{18}
static void
vu_kick_cb(VuDev *dev, int condition, void *data)
{
    int index = (intptr_t)data;
    VuVirtq *vq = &dev->vq[index];
    int sock = vq->kick_fd;
    eventfd_t kick_data;
    ssize_t rc;

    rc = eventfd_read(sock, &kick_data);
    if (rc == -1) {
        vu_panic(dev, "kick eventfd_read(): %s", strerror(errno));
        dev->remove_watch(dev, dev->vq[index].kick_fd);
    } else {
        DPRINT("Got kick_data: %016"PRIx64" handler:%p idx:%d\n",
               kick_data, vq->handler, index);
        if (vq->handler) {
            vq->handler(dev, index);
        }
    }
}
```

## 参考

- [virtio网络Data Plane卸载原理 _ vhost协议协商流程](https://blog.csdn.net/huang987246510/article/details/125215994)
- [深入分析vhost-user网卡实现原理 _ VirtIO Features协商](https://blog.csdn.net/huang987246510/article/details/126925516)
- [vhost(vhost-user)网络I/O半虚拟化详解:一种 virtio 高性能的后端驱动实现](https://blog.csdn.net/Rong_Toa/article/details/114175631)
- [Linux虚拟化KVM-Qemu分析(八)之virtio初探](https://rtoax.blog.csdn.net/article/details/113819423)
- [Vhost-user详解](https://www.jianshu.com/p/ae54cb57e608)
- [浅谈网络I/O全虚拟化、半虚拟化和I/O透传](https://www.jianshu.com/p/fc40104eba23)
- [少阁主_enfj](https://www.jianshu.com/u/7a226aa7834e)
- [qemu内存管理以及vhost-user的协商机制下的前后端内存布局](https://www.jianshu.com/p/dee1e723d6b0)
- [从qemu-virtio到vhost-user](https://juejin.cn/post/7111539346867486756)
- [虚拟化 技术博客](https://blog.csdn.net/huang987246510/category_6939709.html)

- [Linux虚拟化KVM-Qemu分析(九)之virtio设备](https://blog.csdn.net/u012294613/article/details/137653687)
- [Multiqueue - KVM](https://www.linux-kvm.org/page/Multiqueue)
- [使用 virtiofs 在主机及其虚拟机间共享文件 | Red Hat Product Documentation](https://docs.redhat.com/zh_hans/documentation/red_hat_enterprise_linux/9/html/configuring_and_managing_virtualization/sharing-files-between-the-host-and-its-virtual-machines-using-virtio-fs_sharing-files-between-the-host-and-its-virtual-machines)
- [virtio简介(三) _ virtio-balloon qemu设备创建 - Edver](https://www.cnblogs.com/edver/p/14684117.html)
- [virtio-fs 主机<->客机共享文件系统 — The Linux Kernel documentation](https://docs.kernel.org/translations/zh_CN/filesystems/virtiofs.html)
- [【原创】Linux虚拟化KVM-Qemu分析(十一)之virtqueue - LoyenWang - 博客园](https://www.cnblogs.com/LoyenWang/p/14589296.html)
- [虚拟文件系统之争:VirtioFS、gRPC FUSE、osxfs (Legacy)大比拼-CSDN博客](https://blog.csdn.net/Mrxiao_bo/article/details/134546584)
- [Linux虚拟化KVM-Qemu分析之Virtqueue – 后浪云](https://www.idc.net/help/221370/)
- [Virtiofs: Shared file system](https://github.com/virtio-win/kvm-guest-drivers-windows/wiki/Virtiofs:-Shared-file-system)
- [shared file system for virtual machines](https://virtio-fs.gitlab.io/)
- [Virtio-blk Multi-queue Conversion and QEMU Optimization](https://events.static.linuxfound.org/sites/events/files/slides/virtio-blk_qemu_v0.96.pdf)
- [Virtio-fs介绍与性能优化 - tycoon3 - 博客园](https://www.cnblogs.com/dream397/p/13831034.html)
