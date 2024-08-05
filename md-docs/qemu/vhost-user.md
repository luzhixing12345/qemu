
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