
# intro

源码仓库位于 [gitlab qemu](https://gitlab.com/qemu-project/qemu)

## QEMU

qemu 在 linux 上的编译过程参考 [Hosts/Linux](https://wiki.qemu.org/Hosts/Linux)

```bash
sudo apt-get install git libglib2.0-dev libfdt-dev libpixman-1-dev zlib1g-dev ninja-build
```

```bash
sudo apt install libvirt-clients libvirt-daemon-system bridge-utils virtinst libvirt-daemon virt-manager
```

## kvm

```bash
sudo apt install qemu-kvm libvirt-daemon-system libvirt-clients bridge-utils virt-manager
```

```bash
sudo systemctl enable --now libvirtd
```

```bash
ls -l /dev/kvm
sudo apt install cpu-checker
kvm-ok
```

`virt-manager` 是一个图形化的虚拟机管理工具,可以通过以下命令启动

```bash
virt-manager
```

## 参考

- [QEMU: A proper guide!](https://www.youtube.com/watch?v=AAfFewePE7c)
- [Security in QEMU: How Virtual Machines Provide Isolation by Stefan Hajnoczi](https://www.youtube.com/watch?v=YAdRf_hwxU8)
- [[2016] An Introduction to PCI Device Assignment with VFIO by Alex Williamson](https://www.youtube.com/watch?v=WFkdTFTOTpA)
- [KVM Forum](https://www.youtube.com/channel/UCRCSQmAOh7yzgheq-emy1xA)
- [[2015] Improving the QEMU Event Loop by Fam Zheng](https://www.youtube.com/watch?v=sX5vAPUDJVU)
- [【VIRT.0x01】Qemu - II:VNC 模块源码分析](https://arttnba3.cn/2022/07/22/VIRTUALIZATION-0X01-QEMU-PART-II/)
- [【VIRT.0x00】Qemu - I:Qemu 简易食用指南](https://arttnba3.cn/2022/07/15/VIRTUALIZATION-0X00-QEMU-PART-I/)
- [QEMU](https://juniorprincewang.github.io/2018/11/15/QEMU/)
- [whats a good source to learn about qemu](https://stackoverflow.com/questions/155109/whats-a-good-source-to-learn-about-qemu)
- [qemu wiki](https://wiki.qemu.org/Documentation)