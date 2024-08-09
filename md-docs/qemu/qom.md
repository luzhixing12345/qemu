
# qom

QEMU 整个项目是 C 语言写的, 虽然 C 语言在语法层面不能很方便的做到面向对象, 但是也可以通过一些方法实现面向对象的设计思路. 一种简单的实现思路是结构体内部函数指针的第一个参数是当前结构体的类型

```c
struct MyClass {
    int a;
    int b;
    void (*f)(struct Myclass *myclass, int x);
};
```

但是面向 QEMU 处理的对象例如主板,CPU, 总线,外设实际上存在很多继承的关系. 为了方便整个系统的构建, QEMU 实现了自己的一套的面向对象机制,也就是 QEMU Object Model(下面称为 QOM). 作为一个复杂且庞大的项目, 我们需要一种优雅的设计实现面向对象 下面我们将会分析 QEMU 是如何实现这些特性,以及 QEMU 扩展的高级特性

- 继承(inheritance)
- 静态成员(static field)
- 构造函数和析构函数(constructor and destructor)
- 多态(polymorphic)
  - 动态绑定(override)
  - 静态绑定(overload)
- 抽象类/虚基类(abstract class)
- 动态类型装换(dynamic cast)
- 接口(interface)

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

`type_init` 是一个调用 `module_init` 的宏定义, 而 `module_init` 比较特殊, 它的展开是一个拼接的函数定义, 同时注意到它是一个 constructor 的构造函数, 意味着该函数会是**程序初始化时在 main 函数之前执行的构造函数**

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

因此整个 `type_init` 的流程如下所示, 从 init_type_list 中找到 MODULE_INIT_QOM 的 ModuleTypeList, 将传入的函数指针 fn 尾插到链表中新元素的 init 字段

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

`module_call_init` 根据传入的type类型,遍历对应的链表,并执行当前元素 e->init()

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

介绍完 type_init 的过程之后我们来重点看一下被调用的 init 函数实现. 该函数将两个全局变量 serial_info 和 serial_mm_info 注册到系统中

```c
static void serial_register_types(void)
{
     type_register_static(&serial_info);
     type_register_static(&serial_mm_info);
}

type_init(serial_register_types);
```

深入函数实现可以发现就是将 `TypeInfo` 对象通过 `type_new` 创建一个新的 `TypeImpl`, 并将其插入到哈希表中.

```c{21,23}
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

`TypeInfo` 和 `TypeImpl` 的区别不大, 设备都使用 `TypeInfo` 注册类型, `TypeImpl` 只是用于管理 QOM 对象类型的具体实现细节调整了一些结构体字段, 开发者通常不会直接与 `TypeImpl` 交互. 在 type_new 的时候也可以保证所有注册的 `TypeInfo` 都是不冲突的.

总的来说

- **TypeInfo**: 是 QOM 类型的公共接口,用于定义类型的元数据和行为.开发者在定义新的 QOM 类型时会用到 `TypeInfo`.
- **TypeImpl**: 是 QOM 类型在框架内部的实现细节,主要由 QOM 系统内部管理和使用.开发者通常不会直接与 `TypeImpl` 交互.

在实践中,开发者定义新类型时主要关注 `TypeInfo`,而 `TypeImpl` 由 QOM 框架自动处理.

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


## QObject

第二次参数匹配时会将长参数后面的参数信息(比如 vmlinux 的路径)等保存起来, 这里我们先来关注一下和 kernel 启动相关的参数处理.

这里首先创建了一个字典 dict 然后使用 `qdict_put_str` 保存参数. 

> 获取该参数则使用 `qdict_get_str`

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

对于 drive/numa 等参数配置比较复杂的情况, 传参模式类似 "-drive format=raw,file=disk/lfs.img,if=virtio", 此时解析会调用 `qemu_opts_parse_noisily` -> `opts_do_parse` 做进一步的参数解析, 将其分割为 `[(format,raw), (file,disk/lfs.img),(if,virto)]` 格式再做后续处理

重点看一下 qdict 字典的创建过程, 申请指针内存然后调用 `qobject_init` 完成类型和引用计数的初始化

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

## 参考

- [qom](https://martins3.github.io/qemu/qom.html)