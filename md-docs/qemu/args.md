
# args

这个命令通过 `taskset` 命令绑定了 CPU 核心并启动了一个 QEMU 虚拟机.让我逐一解释每个参数的含义:

### `taskset -c 48-55`
- 将指定的进程绑定到 CPU 核心 48 到 55 上运行.这个参数确保 QEMU 只在这些核心上执行,以提高性能和确定性.

### `qemu-system-x86_64`
- 启动 QEMU 虚拟机管理程序,用于 x86_64 架构的虚拟机.

### `-name guest=vm0,debug-threads=off`
- `guest=vm0`:给虚拟机命名为 `vm0`.
- `debug-threads=off`:关闭调试线程,减少运行时的性能开销.

### `-machine pc-i440fx-2.9`
- 指定虚拟机的硬件架构类型为 `pc-i440fx-2.9`,这是一个仿真传统的 i440FX 芯片组的 PC 机器类型.

### `-cpu host`
- 使用主机的 CPU 进行虚拟化,这意味着虚拟机会尽可能使用主机 CPU 的功能集.

### `-m 120G`
- 为虚拟机分配 120GB 的内存.

### `-enable-kvm`
- 启用 KVM(Kernel-based Virtual Machine)硬件加速,这可以显著提高虚拟化性能.

### `-overcommit mem-lock=off`
- 允许虚拟机使用的内存超过物理内存而不锁定内存.`mem-lock=off` 表示内存不被锁定在物理 RAM 中.

### `-smp 8`
- 为虚拟机分配 8 个虚拟 CPU.

### `-object memory-backend-ram,size=16G,host-nodes=0,policy=bind,prealloc=no,id=m0`
- 创建一个内存对象 `m0`,大小为 16GB,绑定到主机的 NUMA 节点 0.
- `prealloc=no`:表示不预先分配内存.
  
### `-object memory-backend-ram,size=104G,host-nodes=3,policy=bind,prealloc=no,id=m1`
- 创建另一个内存对象 `m1`,大小为 104GB,绑定到主机的 NUMA 节点 3.
- `prealloc=no`:表示不预先分配内存.

### `-numa node,nodeid=0,memdev=m0`
- 将 NUMA 节点 0 绑定到内存对象 `m0`.

### `-numa node,nodeid=1,memdev=m1`
- 将 NUMA 节点 1 绑定到内存对象 `m1`.

### `-uuid 9bc02bdb-58b3-4bb0-b00e-313bdae0ac81`
- 为虚拟机指定一个唯一的 UUID,用于标识该虚拟机实例.

### `-device ich9-usb-ehci1,id=usb,bus=pci.0,addr=0x5.0x7`
- 添加一个虚拟 USB 控制器(基于 Intel ICH9 芯片的 USB 设备),并将其挂载到 PCI 总线的地址 `0x5.0x7` 上.

### `-device virtio-serial-pci,id=virtio-serial0,bus=pci.0,addr=0x6`
- 添加一个基于 Virtio 的串行接口设备,挂载到 PCI 总线的地址 `0x6`.

### `-drive file=path/to/vm.qcow2,format=qcow2,if=none,id=drive-ide0-0-0`
- 定义一个虚拟磁盘驱动器,使用 `path/to/vm.qcow2` 文件作为磁盘镜像,文件格式为 `qcow2`,并分配一个 ID 为 `drive-ide0-0-0`.

### `-device ide-hd,bus=ide.0,unit=0,drive=drive-ide0-0-0,id=ide0-0-0,bootindex=1`
- 将前面定义的虚拟磁盘设备挂载为 IDE 硬盘,并指定该硬盘为启动盘 (`bootindex=1`).

### `-drive if=none,id=drive-ide0-0-1,readonly=on`
- 添加另一个驱动器,设为只读(比如一个 ISO 光盘).

### `-device ide-cd,bus=ide.0,unit=1,drive=drive-ide0-0-1,id=ide0-0-1`
- 将前面定义的只读驱动器挂载为 IDE 光驱.

### `-device virtio-balloon-pci,id=balloon0,bus=pci.0,addr=0x7`
- 添加一个 Virtio 内存气球设备,用于动态管理虚拟机的内存.

### `-netdev user,id=ndev.0,hostfwd=tcp::5555-:22`
- 使用用户模式网络,并将主机的 5555 端口转发到虚拟机的 SSH 端口 22.

### `-device e1000,netdev=ndev.0`
- 添加一个 `e1000` 虚拟网络接口卡,并将其与网络设备 `ndev.0` 关联.

### `-nographic`
- 运行虚拟机时不显示图形输出(无图形界面),仅使用命令行控制台.

### `-append "root=/dev/sda1 console=ttyS0"`
- 将内核参数追加到虚拟机启动命令行,指定根文件系统位于 `/dev/sda1`,并将控制台输出重定向到串口 `ttyS0`.

总结来说,这个命令启动了一个带有 8 个虚拟 CPU 和 120GB 内存的虚拟机,其中的内存通过 NUMA 拆分为两个节点,并且设置了多个虚拟设备,如磁盘、网卡和 USB 控制器.