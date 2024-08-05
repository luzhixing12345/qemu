
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

判断是否终止的函数有点长, 但是可以通过其返回的位置判断, 只有收到 shutdown_request 时退出, 将退出状态(status)带出并返回 true, 其余情况均返回 false

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

循环主体 `main_loop_wait` 负责等待和处理事件循环中的事件.这个函数使用多路复用(poll)机制来等待事件,并在事件到达时进行处理

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
4. 检验参数合法性, 并初始化整个系统

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
    qemu_validate_options(machine_opts_dict); // 检验参数合法性
    qemu_apply_machine_options(machine_opts_dict); // 各种设备和系统的初始化
    // ...
}
```

### qemu_add_opts

在初始化的第一阶段是添加一些默认配置, 其中核心函数为 `qemu_add_opts`, 调用该函数传递的参数均是以 `*_opts` 结尾的全局变量指针

该函数的实现并不复杂, 只是在全局变量 `vm_config_groups` 中找到第一个非空的位置然后保存 list 指针. 另一个全局变量 `drive_config_groups` 则是为 `qemu_add_drive_opts` 来服务的

> 注意到这里使用了 48 和 5 的硬编码, 一般来说应当在软件开发中避免此类问题

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



全局变量的定义, 以 `qemu_device_opts` 为例, 其中 `head` 字段采用的是循环引用

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

### QObject

第二次参数匹配时会将长参数后面的参数信息(比如 vmlinux 的路径)等保存起来, 这里我们先来关注一下和 kernel 启动相关的参数处理.

这里首先创建了一个 dict 然后使用 `qdict_put_str` 保存参数. 相信学习过 python/js 等其他编程语言的同学对于字典的数据结构肯定并不陌生, 在 C++ 里面也有标准库的 `unordered_map` 来完成键值存储.

```c{1,6,9}
machine_opts_dict = qdict_new();
for(;;) {
    popt = lookup_opt(argc, argv, &optarg, &optind);
    switch(popt->index) {
        case QEMU_OPTION_kernel:
            qdict_put_str(machine_opts_dict, "kernel", optarg);
            break;
        case QEMU_OPTION_initrd:
            qdict_put_str(machine_opts_dict, "initrd", optarg);
            break;
        case QEMU_OPTION_drive:
            opts = qemu_opts_parse_noisily(qemu_find_opts("drive"),
                                           optarg, false);
            break;
    }
}
```

对于 drive/numa 等参数配置比较复杂, 传参模式类似 "-drive format=raw,file=disk/lfs.img,if=virtio", 此时解析会调用 `qemu_opts_parse_noisily` -> `opts_do_parse` 做进一步的参数解析, 将其分割为 `[(format,raw), (file,disk/lfs.img),(if,virto)]` 格式再做后续处理

重点看一下字典的创建过程, 申请指针内存然后调用 `qobject_init` 完成类型和引用计数的初始化

```c
// qobject/qdict.c
QDict *qdict_new(void)
{
    QDict *qdict;

    qdict = g_malloc0(sizeof(*qdict));
    qobject_init(QOBJECT(qdict), QTYPE_QDICT);

    return qdict;
}

static inline void qobject_init(QObject *obj, QType type)
{
    assert(QTYPE_NONE < type && type < QTYPE__MAX);
    obj->base.refcnt = 1;
    obj->base.type = type;
}
```

在 qemu 当中的 object 类型共有 6 种(除去 QTYPE_NONE 和 QTYPE__MAX), 分别对应 NULL, NUM, STRING, DICT, LIST, BOOL 等常用数据结构

```c
typedef enum QType {
    QTYPE_NONE,
    QTYPE_QNULL,
    QTYPE_QNUM,
    QTYPE_QSTRING,
    QTYPE_QDICT,
    QTYPE_QLIST,
    QTYPE_QBOOL,
    QTYPE__MAX,
} QType;
```

qemu 中所有 object, 所有对象的第一个成员都是 `QObjectBase_` 类型的 base. 而 `QOBJECT` 简单来说就是找到这个 base

```c{8,14,19}
#define QOBJECT_INTERNAL(obj, _obj) ({                          \
    typeof(obj) _obj = (obj);                                   \
    _obj ? container_of(&_obj->base, QObject, base) : NULL;     \
})
#define QOBJECT(obj) QOBJECT_INTERNAL((obj), MAKE_IDENTFIER(_obj))

struct QDict {
    struct QObjectBase_ base;
    size_t size;
    // ...
};

struct QList {
    struct QObjectBase_ base;
    // ...
};

struct QString {
    struct QObjectBase_ base;
    const char *string;
};
```

> 这里的 container_of 和 linux 中的用法相同, 见 [macro](https://luzhixing12345.github.io/libc/articles/linux/macro/)

而在为字典添加键值时将 `char *` 变量又封装为了 `QString` 变量

```c
void qdict_put_str(QDict *qdict, const char *key, const char *value)
{
    qdict_put(qdict, key, qstring_from_str(value));
}
```

`QString` 的创建和 `QDict` 类似, 申请内存然后使用 QTYPE_QSTRING 初始化, 只不过 string 类型较为简单, 可以直接拷贝字符串值

```c
/**
 * qstring_from_substr(): Create a new QString from a C string substring
 *
 * Return string reference
 */
QString *qstring_from_substr(const char *str, size_t start, size_t end)
{
    QString *qstring;

    assert(start <= end);
    qstring = g_malloc(sizeof(*qstring));
    qobject_init(QOBJECT(qstring), QTYPE_QSTRING);
    qstring->string = g_strndup(str + start, end - start);
    return qstring;
}

/**
 * qstring_from_str(): Create a new QString from a regular C string
 *
 * Return strong reference.
 */
QString *qstring_from_str(const char *str)
{
    return qstring_from_substr(str, 0, strlen(str));
}
```

添加字典值的 `qdict_put` 则是将任意一个 Object 对象放到字典中, 求 hash 然后 insert

```c
#define qdict_put(qdict, key, obj) \
        qdict_put_obj(qdict, key, QOBJECT(obj))

void qdict_put_obj(QDict *qdict, const char *key, QObject *value)
{
    unsigned int bucket;
    QDictEntry *entry;

    bucket = tdb_hash(key) % QDICT_BUCKET_MAX;
    entry = qdict_find(qdict, key, bucket);
    if (entry) {
        /* replace key's value */
        qobject_unref(entry->value);
        entry->value = value;
    } else {
        /* allocate a new entry */
        entry = alloc_entry(key, value);
        QLIST_INSERT_HEAD(&qdict->table[bucket], entry, next);
        qdict->size++;
    }
}
```

## QOM类创建流程

所有的外设都存放在 `hw` 目录下, 先从简单的串口设备讲起. 这里以用的比较多的IP核为16550的UART为例,16550串口实现代码位于`hw/char/serial.c`中, 该设备由 `type_init` 完成初始化

```c
static void serial_register_types(void)
{
    type_register_static(&serial_info);
    type_register_static(&serial_mm_info);
}

type_init(serial_register_types);
```

`type_init` 是一个调用 `module_init` 的宏定义, 而 `module_init` 比较特殊, 它的展开是一个拼接的函数定义, 同时注意到它是一个 constructor 的构造函数, 意味着该函数会是程序初始化时在 main 函数之前执行的构造函数

```c
#define type_init(function) module_init(function, MODULE_INIT_QOM)

#define module_init(function, type)                                         \
static void __attribute__((constructor)) do_qemu_init_ ## function(void)    \
{                                                                           \
    register_module_init(function, type);                                   \
}
```

该函数内部进一步调用 `register_module_init`, 第一个参数是用于初始化的函数指针, 第二个参数是 module 类型. 该函数的创建一个 `ModuleEntry` 在 init 字段保存函数指针 fn, 然后找到 type 对应的 `ModuleTypeList` 将 e 插入进去

```c{7,10,12}
void register_module_init(void (*fn)(void), module_init_type type)
{
    ModuleEntry *e;
    ModuleTypeList *l;

    e = g_malloc0(sizeof(*e));
    e->init = fn;
    e->type = type;

    l = find_type(type);

    QTAILQ_INSERT_TAIL(l, e, node);
}
```

qemu 中的 module_type 共有如下几种类型, 该串口设备的创建属于 `MODUELE_INIT_QOM`, 其他几类注册流程类似

```c{15}
typedef enum {
    MODULE_INIT_MIGRATION,
    MODULE_INIT_BLOCK,
    MODULE_INIT_OPTS,
    MODULE_INIT_QOM,
    MODULE_INIT_TRACE,
    MODULE_INIT_XEN_BACKEND,
    MODULE_INIT_LIBQOS,
    MODULE_INIT_FUZZ_TARGET,
    MODULE_INIT_MAX
} module_init_type;

#define block_init(function) module_init(function, MODULE_INIT_BLOCK)
#define opts_init(function) module_init(function, MODULE_INIT_OPTS)
#define type_init(function) module_init(function, MODULE_INIT_QOM)
#define trace_init(function) module_init(function, MODULE_INIT_TRACE)
#define xen_backend_init(function) module_init(function, \
                                               MODULE_INIT_XEN_BACKEND)
#define libqos_init(function) module_init(function, MODULE_INIT_LIBQOS)
#define fuzz_target_init(function) module_init(function, \
                                               MODULE_INIT_FUZZ_TARGET)
#define migration_init(function) module_init(function, MODULE_INIT_MIGRATION)
```

`find_type` 就是从 type 列表中找到对应的 typelist

```c
static ModuleTypeList init_type_list[MODULE_INIT_MAX];

static ModuleTypeList *find_type(module_init_type type)
{
    init_lists();

    return &init_type_list[type];
}
```

因此整个 `type_init` 的流程如下所示

![20240708163328](https://raw.githubusercontent.com/learner-lu/picbed/master/20240708163328.png)

至此在完成对于串口设备的注册, 系统初始化时调用 `qemu_init_subsystems` 完成 TRACE/QOM/MIGRATION 的初始化

```c{8,12,13}
// system/runstate.c
void qemu_init_subsystems(void)
{
    Error *err = NULL;

    os_set_line_buffering();

    module_call_init(MODULE_INIT_TRACE);

    atexit(qemu_run_exit_notifiers);

    module_call_init(MODULE_INIT_QOM);
    module_call_init(MODULE_INIT_MIGRATION);

    // ...
}
```

`module_call_init` 根据传入的type类型,遍历对应的链表,并执行当前元素e->init()

```c{17}
#define QTAILQ_FOREACH() ...
static ModuleTypeList init_type_list[MODULE_INIT_MAX];
static bool modules_init_done[MODULE_INIT_MAX];

void module_call_init(module_init_type type)
{
    ModuleTypeList *l;
    ModuleEntry *e;

    if (modules_init_done[type]) {
        return;
    }

    l = find_type(type);

    QTAILQ_FOREACH(e, l, node) {
        e->init();
    }

    modules_init_done[type] = true;
}
```

整体设备注册到设备初始化的函数调用流程如下, 由 type_init 根据对应的 MODULE_TYPE 注册函数, 在 init_subsystem 时调用设备初始化:

![20240708165604](https://raw.githubusercontent.com/learner-lu/picbed/master/20240708165604.png)

## QOM类设备初始化

介绍完 type_init 的过程之后我们来重点看一下被调用的 init 函数实现

```c
static void serial_register_types(void)
{
     type_register_static(&serial_info);
     type_register_static(&serial_mm_info);
}

type_init(serial_register_types);
```

我们先来看一下 qemu 当中优秀的面向对象的设计 `TypeInfo`, 其结构体字段含义如下. 这些字段的命名很规范, 包括了面向对象的常见需求, 包括对象实例创建/创建完成/销毁, 类初始化, 继承等操作, 这里具体字段用法在后文详细介绍

```c
// include/qom/object.h
struct TypeInfo
{
    const char *name; // 类型的名称
    const char *parent; // 父类型的名称

    size_t instance_size; // 对象的大小, 通常为 sizeof(<class>), 如果置 0 则大小为父类型大小
    size_t instance_align; // 0 正常 malloc, 不为 0 需要使用 qemu_memalign 分配
    void (*instance_init)(Object *obj); // 初始化实例(__init__)
    void (*instance_post_init)(Object *obj); // 在 instance_init 之后调用
    void (*instance_finalize)(Object *obj); // 实例销毁

    bool abstract; // 是否为抽象类, 抽象类不能被直接继承
    size_t class_size; // 类对象的大小

    void (*class_init)(ObjectClass *klass, void *data); // 初始化类的函数
    void (*class_base_init)(ObjectClass *klass, void *data); // 在 class_init 之前调用
    void *class_data; // 传递给class_init和class_base_init的数据

    InterfaceInfo *interfaces; // 关联的接口列表
};
```

串口定义的两个结构体 `serial_info` 和 `serial_mm_info`. 其 parent 分别是 "device" 和 "sys-bus-device". 并将 `.class_init` 绑定到的初始化函数上

```c
static void serial_class_init(ObjectClass *klass, void* data);
static void serial_mm_class_init(ObjectClass *oc, void *data);
static void serial_mm_instance_init(Object *o);

static const TypeInfo serial_info = {
    .name = TYPE_SERIAL,   // "serial"
    .parent = TYPE_DEVICE, // "device"
    .instance_size = sizeof(SerialState),
    .class_init = serial_class_init,
};

static const TypeInfo serial_mm_info = {
    .name = TYPE_SERIAL_MM,        // "serial-mm"
    .parent = TYPE_SYS_BUS_DEVICE, // "sys-bus-device"
    .class_init = serial_mm_class_init,
    .instance_init = serial_mm_instance_init,
    .instance_size = sizeof(SerialMM),
};
```

传递结构体的函数 `type_register_static` 主要负责将 `TypeInfo` 注册为 `TypeImpl`

`type_new` 函数的实现主要是把结构体 info 的成员都复制给 ti, 并将其插入到哈希表中

```c{30}
TypeImpl *type_register_static(const TypeInfo *info)
{
    return type_register(info);
}

TypeImpl *type_register(const TypeInfo *info)
{
    assert(info->parent);
    return type_register_internal(info);
}

static TypeImpl *type_register_internal(const TypeInfo *info)
{
    TypeImpl *ti;

    if (!type_name_is_valid(info->name)) {
        fprintf(stderr, "Registering '%s' with illegal type name\n", info->name);
        abort();
    }

    ti = type_new(info);

    type_table_add(ti);
    return ti;
}

static void type_table_add(TypeImpl *ti)
{
    assert(!enumerating_types);
    g_hash_table_insert(type_table_get(), (void *)ti->name, ti);
}
```

到目前为止 `type_init` 所做的工作就是将两个 TypeInfo 转变为 TypeImpl 注册到 hashtable 中. 我们关心的问题是什么时候会用到这个 hashtable, 设备模型如何工作? 

对于每一个使用 `TypeInfo` 结构体声明的函数,有一个关键的成员 `.class_init` 作为设备核心的入口函数,从软件角度来说就理解为资源初始化, 通过 gdb 断点方式回溯函数调用栈的方式可以找到初始化的位置

```txt
main [system/main.c]
  qemu_init [system/vl.c]
    qemu_create_machine [system/vl.c]
      select_machine [system/vl.c]
        object_class_get_list [qom/object.c]
          object_class_foreach [qom/object.c]
            g_hash_table_foreach [nown]
              object_class_foreach_tramp [qom/object.c]
                type_initialize [qom/object.c]
                  serial_class_init [hw/char/serial.c]
```

创建 machine 使用到的 QDict 就是前面提到的 `machine_opts_dict`

```c{5}
static QDict *machine_opts_dict;

static void qemu_create_machine(QDict *qdict)
{
    MachineClass *machine_class = select_machine(qdict, &error_fatal);
    object_set_machine_compat_props(machine_class->compat_props);
    // ...
}
```

再跟进几步函数调用直到 `object_class_foreach`, 其中 `g_hash_table_foreach` 遍历哈希表中的每一个元素,然后执行object_class_foreach_tramp函数,这个函数的参数是构造的 data. 而哈希表就是通过 type_init 注册的所有 TypeImpl 元素

```c{8}
void object_class_foreach(void (*fn)(ObjectClass *klass, void *opaque),
                          const char *implements_type, bool include_abstract,
                          void *opaque)
{
    OCFData data = { fn, implements_type, include_abstract, opaque };

    enumerating_types = true;
    g_hash_table_foreach(type_table_get(), object_class_foreach_tramp, &data);
    enumerating_types = false;
}
```

循环执行的函数 `object_class_foreach_tramp` 则调用 `type_initialize` 执行 hashtable 每一个 TypeImpl

```c
static void object_class_foreach_tramp(gpointer key, gpointer value,
                                       gpointer opaque)
{
    OCFData *data = opaque;
    TypeImpl *type = value;
    ObjectClass *k;

    type_initialize(type);
    k = type->class;

    if (!data->include_abstract && type->abstract) {
        return;
    }

    if (data->implements_type && 
        !object_class_dynamic_cast(k, data->implements_type)) {
        return;
    }

    data->fn(k, data->opaque);
}
```

`type_initialize` 函数很长, 主要是做对象初始化的一些处理, 最终调用其内部的 `.class_init` 字段指向的函数指针

```c{5}
static void type_initialize(TypeImpl *ti)
{
    // ...
    if (ti->class_init) {
        ti->class_init(ti->class, ti->class_data);
    }
}
```

总结一下整体流程就是: type_init中调用type_register_static 将函数初始化的设备信息插入到 type_hashtable 中,在qemu_machine_create 时通过从hash表中遍历, 找到每一个 TypeImpl, 然后往下调用 type_initialize 执行 class_init

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