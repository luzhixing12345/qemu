
# intro

qemu 支持 CXL2.0 Type3 [qemu cxl](https://www.qemu.org/docs/master/system/devices/cxl.html)

qemu 用于模拟 CXL 硬件设备, 完整 CXL 功能还需要内核支持, 需要开启如下内核选项

```txt
CONFIG_CXL_BUS
CONFIG_CXL_PCI
CONFIG_CXL_ACPI
CONFIG_CXL_PMEM
CONFIG_CXL_MEM
CONFIG_CXL_PORT
CONFIG_CXL_REGION
```

位于

```txt
Device Drivers
    -> PCI support (PCI [=y])
        -> CXL (Compute Express Link) Devices Support (CXL_BUS [=y])
```

![20240705113348](https://raw.githubusercontent.com/learner-lu/picbed/master/20240705113348.png)

方便 Vscode 调试, 把 makefile 的输出结果转成 `launch.json` 的 args 模式

```bash
make cxl -n | tr ' ' '\n' | grep -v '\\' | grep . | sed 's/.*/"&",/'
```

```txt
main [system/main.c]
  qemu_init [system/vl.c]
    qemu_create_machine [system/vl.c]
      object_new_with_class [qom/object.c]
        object_new_with_type [qom/object.c]
          object_initialize_with_type [qom/object.c]
            object_init_with_type [qom/object.c]
              object_init_with_type [qom/object.c]
                pc_machine_initfn [hw/i386/pc.c]
                  cxl_machine_init [hw/cxl/cxl-host.c]
```

## 参考

- [qemu cxl](https://www.qemu.org/docs/master/system/devices/cxl.html)