
# system

> qemu 中使用了很多 glib 提供的函数, 阅读本文之前请先阅读 [glib](./glib.md)

qemu 的代码写的还是很简洁的, ~~相比于 kernel~~阅读和调试起来都比较友好

qemu 可以进行用户模式的仿真也可以进行系统的仿真, 系统级别的仿真会模拟 CPU/内存/磁盘/外设等等, 本节主要研究 `qemu-system-*` 的函数逻辑, 其主函数入口位于 `system/main.c`

主函数首先调用 `qemu_init` 读取所有参数并初始化整个系统, 然后进入 `qemu_main_loop`, 当终止时做清理工作

```c
int qemu_default_main(void)
{
    int status;

    status = qemu_main_loop();
    qemu_cleanup(status);

    return status;
}

int (*qemu_main)(void) = qemu_default_main;

int main(int argc, char **argv)
{
    qemu_init(argc, argv);
    return qemu_main();
}
```

## 初探主循环

正如一个操作系统内核的实现, 系统初始化的部分固然重要, 但是也仅仅初始化一次, 系统的主体循环部分才是模拟的重点.

qemu 采用 C 实现, 但是设计了大量的数据结构用于面向对面编程(OOP)的思想, 先来介绍一下主体逻辑

`qemu_main_loop` 中 while 循环判断是否结束, 每一次循环中持续等待. 循环结束后将 `status` 交由 `qemu_cleanup` 完成收尾工作

```c
int qemu_main_loop(void)
{
    int status = EXIT_SUCCESS;

    while (!main_loop_should_exit(&status)) {
        main_loop_wait(false);
    }

    return status;
}
```

判断是否终止的函数有点长, 但是可以通过其返回的位置判断, 只在 `shutdown_request()` 为 TRUE 时退出, 将退出状态(status)带出并返回 true, 其余情况均返回 false. 即用户通过 <kbd>ctrl</kbd> + <kbd>a</kbd> + <kbd>x</kbd> 等方式终止时

```c{23-33}
static bool main_loop_should_exit(int *status)
{
    RunState r; // 运行状态
    ShutdownCause request; // 关机请求的原因

    // 如果收到调试请求,则停止虚拟机并进入调试状态
    if (qemu_debug_requested()) {
        vm_stop(RUN_STATE_DEBUG);
    }
    
    // 如果收到挂起请求,则挂起系统
    if (qemu_suspend_requested()) {
        qemu_system_suspend();
    }

    // 检查是否有关机请求
    request = qemu_shutdown_requested();
    if (request) {
        qemu_kill_report(); // 报告关机事件
        qemu_system_shutdown(request); // 执行系统关机

        // 如果关机动作是暂停,则停止虚拟机
        if (shutdown_action == SHUTDOWN_ACTION_PAUSE) {
            vm_stop(RUN_STATE_SHUTDOWN);
        } else {
            // 根据不同的关机原因设置退出状态码
            if (shutdown_exit_code != EXIT_SUCCESS) {
                *status = shutdown_exit_code;
            } else if (request == SHUTDOWN_CAUSE_GUEST_PANIC &&
                panic_action == PANIC_ACTION_EXIT_FAILURE) {
                *status = EXIT_FAILURE;
            }
            return true; // 返回true,表示主循环应该退出
        }
    }

    // 检查是否有重置请求
    request = qemu_reset_requested();
    if (request) {
        pause_all_vcpus(); // 暂停所有虚拟CPU
        qemu_system_reset(request); // 执行系统重置
        resume_all_vcpus(); // 恢复所有虚拟CPU

        // 运行状态可以在pause_all_vcpus中改变
        if (!runstate_check(RUN_STATE_RUNNING) &&
                !runstate_check(RUN_STATE_INMIGRATE) &&
                !runstate_check(RUN_STATE_FINISH_MIGRATE)) {
            runstate_set(RUN_STATE_PRELAUNCH); // 设置运行状态为预启动
        }
    }

    // 检查是否有唤醒请求
    if (qemu_wakeup_requested()) {
        pause_all_vcpus(); // 暂停所有虚拟CPU
        qemu_system_wakeup(); // 执行系统唤醒
        notifier_list_notify(&wakeup_notifiers, &wakeup_reason); // 通知唤醒事件
        wakeup_reason = QEMU_WAKEUP_REASON_NONE; // 重置唤醒原因
        resume_all_vcpus(); // 恢复所有虚拟CPU
        qapi_event_send_wakeup(); // 发送唤醒事件
    }

    // 检查是否有电源关闭请求
    if (qemu_powerdown_requested()) {
        qemu_system_powerdown(); // 执行系统电源关闭
    }

    // 检查是否有虚拟机停止请求
    if (qemu_vmstop_requested(&r)) {
        vm_stop(r); // 停止虚拟机
    }

    return false; // 返回false,表示主循环不应该退出
}
```

循环主体 `main_loop_wait` 负责等待和处理事件循环中的事件.这个函数使用多路复用(poll)机制来等待事件,并在事件到达时进行处理.

其中的 gpollfds, 非阻塞(nonblocking)时 timeout=0 为 Glib 事件循环的内容, 在 [glib](./glib.md) 中已有介绍

```c{17-20,40}
void main_loop_wait(int nonblocking)
{
    // 定义并初始化MainLoopPoll结构
    MainLoopPoll mlpoll = {
        .state = MAIN_LOOP_POLL_FILL, // 设置初始状态为FILL
        .timeout = UINT32_MAX, // 设置超时时间为最大值
        .pollfds = gpollfds, // 使用全局poll文件描述符数组
    };
    int ret;
    int64_t timeout_ns;

    // 如果是非阻塞模式,将超时时间设为0
    if (nonblocking) {
        mlpoll.timeout = 0;
    }

    // 处理任何事件
    g_array_set_size(gpollfds, 0); // 重置poll文件描述符数组的大小以进行新一轮迭代
    // 通知所有main_loop_poll_notifiers
    notifier_list_notify(&main_loop_poll_notifiers, &mlpoll);

    // 根据超时时间设置timeout_ns
    if (mlpoll.timeout == UINT32_MAX) {
        timeout_ns = -1; // 无限等待
    } else {
        timeout_ns = (uint64_t)mlpoll.timeout * (int64_t)(SCALE_MS); // 转换为纳秒
    }

    // 获取最早的定时器超时时间
    timeout_ns = qemu_soonest_timeout(timeout_ns,
                                      timerlistgroup_deadline_ns(
                                          &main_loop_tlg));

    // 进行系统级的事件等待
    ret = os_host_main_loop_wait(timeout_ns);

    // 根据等待结果设置mlpoll的状态
    mlpoll.state = ret < 0 ? MAIN_LOOP_POLL_ERR : MAIN_LOOP_POLL_OK;
    // 通知所有main_loop_poll_notifiers
    notifier_list_notify(&main_loop_poll_notifiers, &mlpoll);

    // 如果启用了icount模式,启动warp定时器
    if (icount_enabled()) {
        /*
         * CPU线程可能会在错过warp后无限等待事件
         */
        icount_start_warp_timer();
    }

    // 运行所有的定时器
    qemu_clock_run_all_timers();
}
```

简单阅读主循环函数代码之后对整个系统有了一个初步的概念, qemu 是一个**事件驱动**的模拟器, 其运行主要依赖于事件的处理和调度,而不是通过一个持续的循环或固定的时间步长来进行处理. 事件驱动模型使得 qemu 能够高效地管理和响应多种异步事件,如I/O操作、定时器事件、中断和用户输入等

对于 system 级别的模拟, 包括 CPU/memory/device 等都有对应的模拟事件和计时器, 当事件发生以及处理完毕之后还会通知所有 `notifier` 来进一步响应.

qemu 的函数命名很规范, 封装的也很好, 初次阅读不易过于深入, 在详细介绍系统初始化关键数据结构之后会 "再谈主循环"

## 系统初始化

系统初始化的过程主要分为四个阶段

1. 初始化一些默认配置信息和全局变量
2. 初次尝试匹配参数, 对于一些不合法的参数直接报错
3. 再次尝试匹配参数, 将参数信息收集并记录到对应的变量中
4. 初始化整个系统

```c
void qemu_init(int argc, char **argv) {

    // 初始化一些默认配置
    qemu_add_opts(&qemu_drive_opts);
    qemu_add_drive_opts(&qemu_legacy_drive_opts);
    qemu_add_opts(&qemu_accel_opts);
    qemu_add_opts(&qemu_mem_opts);
    // ...
    
    // 初次尝试匹配参数
    optind = 1;
    while (optind < argc) {
        popt = lookup_opt(argc, argv, &optarg, &optind);
        switch (popt->index) {
        case QEMU_OPTION_nouserconfig:
            userconfig = false;
            break;
        }
    }

    // 再次尝试匹配参数, 处理对应 case
    optind = 1;
    while (optind < argc) {
        popt = lookup_opt(argc, argv, &optarg, &optind);
        switch (popt->index) {
          case QEMU_OPTION_cpu:
            // do something
            break;
          case QEMU_OPTION_kernel:
            qdict_put_str(machine_opts_dict, "kernel", optarg);
            break;
          case QEMU_OPTION_drive:
            opts = qemu_opts_parse_noisily(qemu_find_opts("drive"),
                                           optarg, false);
            if (opts == NULL) {
                exit(1);
            }
            break;
        }
    }
    
    qemu_create_machine(machine_opts_dict);
    // ...
}
```

### qemu_add_opts

在初始化的第一阶段是添加一些默认配置, 其中核心函数为 `qemu_add_opts`, 调用该函数传递的参数均是以 `*_opts` 结尾的全局变量指针

该函数的实现并不复杂, 只是在全局变量 `vm_config_groups` 中找到第一个非空的位置然后保存 list 指针. 另一个全局变量 `drive_config_groups` 则是为 `qemu_add_drive_opts` 来服务的

```c
QemuOptsList *vm_config_groups[48];
QemuOptsList *drive_config_groups[5];

void qemu_add_opts(QemuOptsList *list)
{
    int entries, i;

    entries = ARRAY_SIZE(vm_config_groups);
    entries--; /* keep list NULL terminated */
    for (i = 0; i < entries; i++) {
        if (vm_config_groups[i] == NULL) {
            vm_config_groups[i] = list;
            return;
        }
    }
    fprintf(stderr, "ran out of space in vm_config_groups");
    abort();
}
```

简单来说开头的一大段就是将所有的全局变量 `qemu_*_opts` 保存到 vm_config_groups 中以便后续处理, 以 `qemu_device_opts` 为例, 其中 `head` 字段采用的是循环引用

```c
#define QTAILQ_HEAD_INITIALIZER(head)                                   \
        { .tqh_circ = { NULL, &(head).tqh_circ } }

QemuOptsList qemu_device_opts = {
    .name = "device",
    .implied_opt_name = "driver",
    .head = QTAILQ_HEAD_INITIALIZER(qemu_device_opts.head),
    .desc = {
        /*
         * no elements => accept any
         * sanity checking will happen later
         * when setting device properties
         */
        { /* end of list */ }
    },
};
```

`QemuOptsList` 中的 `head` 字段是一个比较有意思的设计, 其采用双向链表, 这种设计主要是为了在编写代码时可以通用化的处理

```c
/*
 * Tail queue definitions.  The union acts as a poor man template, as if
 * it were QTailQLink<type>.
 */
#define QTAILQ_HEAD(name, type)                                         \
union name {                                                            \
        struct type *tqh_first;       /* first element */               \
        QTailQLink tqh_circ;          /* link for circular backwards list */ \
}

struct QemuOptsList {
    const char *name;
    const char *implied_opt_name;
    bool merge_lists;  /* Merge multiple uses of option into a single list? */
    QTAILQ_HEAD(, QemuOpts) head;
    QemuOptDesc desc[];
};

typedef struct QTailQLink {
    void *tql_next;
    struct QTailQLink *tql_prev;  
}   QTailQLink;
```

在 `include/qemu/queue.h` 该文件中定义了四种数据结构 **单向链表、(双向)链表、单向队列和尾(双向)队列**, 并包含了大量的宏处理, 以本例中的尾队列为例

```c
#define QTAILQ_INSERT_HEAD(head, elm, field)
#define QTAILQ_INSERT_TAIL(head, elm, field)
#define QTAILQ_INSERT_AFTER(head, listelm, elm, field)
#define QTAILQ_INSERT_BEFORE(listelm, elm, field)
#define QTAILQ_REMOVE(head, elm, field)
#define QTAILQ_FOREACH(var, head, field) 
```

### lookup_opt

配置初始化之后会尝试第一次解析参数, 遍历每一个 argv 并调用 `lookup_opt` 进行解析

```c{10}
/* first pass of option parsing */
optind = 1;
while (optind < argc) {
    if (argv[optind][0] != '-') {
        /* disk image */
        optind++;
    } else {
        const QEMUOption *popt;

        popt = lookup_opt(argc, argv, &optarg, &optind);
        switch (popt->index) {
        case QEMU_OPTION_nouserconfig:
            userconfig = false;
            break;
        }
    }
}
```

该函数本身也并不复杂, 值得一提的是 qemu 推荐的传参方式不同于大多数软件采用的 `--` 传递的长参数, 而普遍采用单 `-` 的长参数, 例如 `-kernel` `-initrd`, 不过在代码中可以看到也对于 `--` 的参数传递做了 `r++` 的处理.

循环主体部分就是依次遍历判断所有的 `qemu_options` 的名字是否相同

```c{12,13,20-22}
static const QEMUOption *lookup_opt(int argc, char **argv,
                                    const char **poptarg, int *poptind)
{
    const QEMUOption *popt;
    int optind = *poptind;
    char *r = argv[optind];
    const char *optarg;

    loc_set_cmdline(argv, optind, 1);
    optind++;
    /* Treat --foo the same as -foo.  */
    if (r[1] == '-')
        r++;
    popt = qemu_options;
    for(;;) {
        if (!popt->name) {
            error_report("invalid option");
            exit(1);
        }
        if (!strcmp(popt->name, r + 1))
            break;
        popt++;
    }
    if (popt->flags & HAS_ARG) {
        if (optind >= argc) {
            error_report("requires an argument");
            exit(1);
        }
        optarg = argv[optind++];
        loc_set_cmdline(argv, optind - 2, 2);
    } else {
        optarg = NULL;
    }

    *poptarg = optarg;
    *poptind = optind;

    return popt;
}
```

这里的 `qemu_options` 稍微有点意思, 改变量结合 C 预处理器来完成对于文件 "qemu-options.def" 内容的导入, 该文件会在编译时生成, 文件内容就是 DEF 这些宏的使用, 然后匹配其中的 `.name` 字段

```c
static const QEMUOption qemu_options[] = {
    { "h", 0, QEMU_OPTION_h, QEMU_ARCH_ALL },

#define DEF(option, opt_arg, opt_enum, opt_help, arch_mask)     \
    { option, opt_arg, opt_enum, arch_mask },
#define DEFHEADING(text)
#define ARCHHEADING(text, arch_mask)

#include "qemu-options.def"
    { /* end of list */ }
};
```

![20240707140822](https://raw.githubusercontent.com/learner-lu/picbed/master/20240707140822.png)

如果初次匹配通过则说明所有的**参数格式**都是合法的, 此时尝试进行进行二次匹配, 确定所有传参的字符串等是否合法

## device

```txt
main [system/main.c]
  qemu_init [system/vl.c]
    qmp_x_exit_preconfig [system/vl.c]
      qemu_create_cli_devices [system/vl.c]
        qemu_opts_foreach [util/qemu-option.c]
          device_init_func [system/vl.c]
            qdev_device_add [system/qdev-monitor.c]
              qdev_device_add_from_qdict [system/qdev-monitor.c]
                qdev_new [hw/core/qdev.c]
                  object_new [qom/object.c]
                    object_new_with_type [qom/object.c]
                      object_initialize_with_type [qom/object.c]
                        object_init_with_type [qom/object.c]
                          object_init_with_type [qom/object.c]
                            vhost_user_fs_pci_instance_init [hw/virtio/vhost-user-fs-pci.c]
                              virtio_instance_init_common [hw/virtio/virtio.c]
                                object_initialize_child_with_props [qom/object.c]
                                  object_initialize_child_with_propsv [qom/object.c]
                                    object_initialize [qom/object.c]
                                      object_initialize_with_type [qom/object.c]
                                        object_init_with_type [qom/object.c]
                                          vuf_instance_init [hw/virtio/vhost-user-fs.c]

```
qmp_x_exit_preconfig
qemu_create_cli_devices

## 参考

- [菜鸡读QEMU源码](https://www.cnblogs.com/from-zero/p/12383652.html)
- [lifeislife QEMU-源码分析](https://lifeislife.cn/categories/QEMU-%E6%BA%90%E7%A0%81%E5%88%86%E6%9E%90/)
- [QEMU代码详解](https://blog.csdn.net/home19900111/article/details/127702727) *
- [虚拟化收纳箱](https://blog.csdn.net/home19900111/category_12073604.html)
- [【QEMU 中文文档】0. Hello QEMU!](https://blog.csdn.net/qq_37434641/article/details/139352019)
- [qemu的详细资料大全(入门必看!!!)](https://blog.csdn.net/kangkanglhb88008/article/details/126299695)
- [how to add a new device in qemu source code](https://stackoverflow.com/questions/28315265/how-to-add-a-new-device-in-qemu-source-code)
- [vmsplice blog](https://blog.vmsplice.net/2011/03/qemu-internals-big-picture-overview.html)
- [qemu源码分析(一)-- QOM类创建流程](https://zhuanlan.zhihu.com/p/636763565)
- [qemu源码分析(二)-- QOM类设备初始化](https://zhuanlan.zhihu.com/p/637216343)
- [虚拟仿真技术 专栏](https://www.zhihu.com/column/c_1571975874009907200)
- [【原创】Linux虚拟化KVM-Qemu分析(五)之内存虚拟化](https://www.cnblogs.com/LoyenWang/p/13943005.html)