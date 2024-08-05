# qemu

本项目为笔者对于虚拟化技术的学习笔记, 包括 

- qemu 的使用和源码阅读
- qemu cxl 的支持

观前提示: 对于 qemu 的源码参考并不具有时效性, 笔者正在积极参与 qemu 社区并改进源代码, 阅读时最新的版本为 v9.0.1, commit hash `7914bd`

在线文档: [qemu document](https://luzhixing12345.github.io/qemu/)

## 参考

- 书籍
  - 系统虚拟化 原理与实现
  - 深度探索Linux系统虚拟化:原理与实现 (王柏生  谢广军) 
  - NewBluePill深入理解硬件虚拟机 (于淼  戚正伟)
- 技术博客
  - [虚拟化 技术博客](https://github.com/zhangjaycee/real_tech/wiki/virtualcatalog)
  - [虚拟化 技术博客](https://www.junmajinlong.com/tags/Virtualization/)
  - [KVM 虚拟化详解](https://zhuanlan.zhihu.com/p/105499858)
  - [智能云计算 专栏](https://www.zhihu.com/column/c_1187367200796606464)
  - [虚拟化技术概览](https://houmin.cc/posts/65866329/)
  - [虚拟化技术系列](https://zhuanlan.zhihu.com/p/93289632)
  - [深入理解虚拟化](https://zhuanlan.zhihu.com/p/441287815)
  - [容器的底层技术:CGroup和NameSpace](https://zhuanlan.zhihu.com/p/690639138)
  - [基于qemu手写vhost/virtio,简化虚拟设备通信](https://zhuanlan.zhihu.com/p/689616659)
  - [深入理解VFIO:一种高效的虚拟化设备访问技术](https://zhuanlan.zhihu.com/p/689107103)
  - [KVM-Qemu-Libvirt三者之间的关系](https://hsinin.github.io/2017/01/16/KVM-Qemu-Libvirt%E4%B8%89%E8%80%85%E4%B9%8B%E9%97%B4%E7%9A%84%E5%85%B3%E7%B3%BB/)
  - [虚拟化技术博客](https://zhuanlan.zhihu.com/p/701266513)
- youtube talk
  - [VM live migration over CXL memory - Dragan Stancevic](https://www.youtube.com/watch?v=8glo1KUQrlY&list=PLbzoR-pLrL6rlmdpJ3-oMgU_zxc1wAhjS&index=10&pp=iAQB)
  - [VM memory overcommit - T.J. Alumbaugh, Yuanchu Xie](https://www.youtube.com/watch?v=K5QS7MtAMzw&list=PLbzoR-pLrL6rlmdpJ3-oMgU_zxc1wAhjS&index=12)
  - [Virtual machine memory passthrough - Pasha Tatashin](https://www.youtube.com/watch?v=MhPDLF8g3f0&list=PLbzoR-pLrL6rlmdpJ3-oMgU_zxc1wAhjS&index=38&pp=iAQB)
- github repo study:
  - [cloud-hypervisor](https://github.com/cloud-hypervisor/cloud-hypervisor)
  - [rust vmm](https://github.com/rust-vmm/community)
  - [Introduce_to_virtualization](https://github.com/0voice/Introduce_to_virtualization)
  - [虚拟化论文](https://github.com/dyweb/papers-notebook)
  - [KVM 虚拟化入门](https://github.com/yangcvo/KVM)
  - [容器虚拟化学习资料](https://github.com/charSLee013/docker?tab=readme-ov-file)
  - [kvm虚拟化实践](https://github.com/junneyang/kvm-practice)
  - [2023秋冬季训练营第三阶段-虚拟化方向](https://github.com/arceos-hypervisor/2023-virtualization-campus)
  - [learn-kvm](https://github.com/yifengyou/learn-kvm)
  - [刘超的通俗云计算](https://www.cnblogs.com/popsuper1982/category/589324.html)
  - [享乐主](https://blog.csdn.net/huang987246510?type=blog)
- qemu
  - [lifeislife QEMU-源码分析](https://lifeislife.cn/categories/QEMU-%E6%BA%90%E7%A0%81%E5%88%86%E6%9E%90/)