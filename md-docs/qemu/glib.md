
# glib

Glib 是一个广泛使用的 C 语言库,提供了许多数据结构、线程、消息传递、对象系统和其他功能.许多大型和知名的开源项目都使用了 Glib, 如 QEMU/GNOME/GTK/Systemd.

glib 的代码封装的很完善, 这里我们先介绍几种十分常见且有用的函数接口

```bash
sudo apt install libglib2.0-dev
```

编译的时候

```Makefile
CFLAGS += `pkg-config --cflags glib-2.0`
LDFLAGS = -pthread `pkg-config --libs glib-2.0`
```

## 事件循环

QEMU 作为一个持续运行的状态机, 需要不停地检查是否有新事件发生, 如果发生那么执行对应的一些处理. 我们可能会写出一个常见的 while 循环

```c
while (1) {
    if (something_happened) {
        do_something();
    }
    // ...
}
```

但是实际情况可能比较复杂, 比如希望间隔 0.1 秒执行一次, 事件触发对应的回调函数, 可以在循环过程中注册新事件, 删除某些事件等等. glib 提供了一种简洁的事件循环接口

下面是一个简单的示例, 首先创建了一个主循环 loop. 然后使用 `g_timeout_add` 注册了两个回调事件. 第一个参数为间隔时间, 第二个参数为回调函数指针, 第三个参数是传参

> `gpointer` 就是 `void*`, 如果参数很多则需要作为一个结构体传进去, 回调函数内部再强转取出来

然后通过 `g_main_loop_run` 进入主循环, 这是一个 while 死循环, 此时程序会执行所有的注册事件. 执行效果如下图所示

```c{10,11,13}
#include <glib.h>
#include <stdio.h>

gboolean count_down(gpointer data);
gboolean cancel_fire(gpointer data);

int main(int argc, char *argv[]) {
    GMainLoop *loop = g_main_loop_new(NULL, FALSE);

    g_timeout_add(500, count_down, NULL);
    g_timeout_add(8000, cancel_fire, loop);

    g_main_loop_run(loop);
    g_main_loop_unref(loop);

    return 0;
}

gboolean count_down(gpointer data) {
    static int counter = 10;

    if (counter < 1) {
        printf("-----[FIRE]-----\n");
        return FALSE;
    }

    printf("-----[% 4d]-----\n", counter--);
    return TRUE;
}

gboolean cancel_fire(gpointer data) {
    GMainLoop *loop = data;

    printf("-----[QUIT]-----\n");
    g_main_loop_quit(loop);

    return FALSE;
}
```

> 代码见 src/glib/event_loop.c

每 0.5 输出一次 counter, 8s 后执行了一次 `cancel_file` 退出主循环终止. `count_down` 和 `cancel_file` 的返回值表示是否继续运行, 例如把 count_down 的返回值改为 FALSE 则执行一次即停止

![sdmkl9l](https://raw.githubusercontent.com/learner-lu/picbed/master/sdmkl9l.gif)

如果希望主循环在空闲间隔执行某些一些函数可以使用 `g_idle_add(funcp, data)`

## hashtable

`g_hash_table_foreach` 是 GLib 提供的一个函数,用于遍历哈希表(`GHashTable`)中的所有键值对,并对每个键值对执行指定的回调函数.它的函数签名如下:

```c
void g_hash_table_foreach(GHashTable *hash_table, GHFunc func, gpointer user_data);
```

### 参数说明
- `hash_table`:要遍历的哈希表.
- `func`:用于处理每个键值对的回调函数,类型为 `GHFunc`.
- `user_data`:传递给回调函数的用户数据.

### 回调函数 `GHFunc`
`GHFunc` 是一个函数指针类型,其定义如下:

```c
typedef void (*GHFunc) (gpointer key, gpointer value, gpointer user_data);
```

- `key`:哈希表中的一个键.
- `value`:与该键关联的值.
- `user_data`:传递给 `g_hash_table_foreach` 的用户数据.

### 用法示例
以下是一个使用 `g_hash_table_foreach` 遍历哈希表并打印每个键值对的示例:

```c
// src/glib/hashtable.c
#include <glib.h>
#include <stdio.h>

struct MyData {
    int x;
    int y;
};

// 回调函数,用于处理每个键值对
void print_key_value(gpointer key, gpointer value, gpointer user_data) {
    printf("Key: %s, Value: %s\n", (char *)key, (char *)value);
    struct MyData *data = (struct MyData *)user_data;
    printf("x = %d, y = %d\n", data->x, data->y);
}

int main() {
    // 创建一个哈希表
    GHashTable *hash_table = g_hash_table_new(g_str_hash, g_str_equal);

    // 添加键值对到哈希表
    g_hash_table_insert(hash_table, "name", "Alice");
    g_hash_table_insert(hash_table, "city", "Wonderland");
    g_hash_table_insert(hash_table, "occupation", "Adventurer");

    // 使用 g_hash_table_foreach 遍历哈希表
    struct MyData data = { 10, 20 };
    g_hash_table_foreach(hash_table, print_key_value, &data);

    // 销毁哈希表
    g_hash_table_destroy(hash_table);

    return 0;
}
```

### 示例解释
1. **创建哈希表**:使用 `g_hash_table_new` 创建一个哈希表,其中键和值都是字符串.
2. **添加键值对**:使用 `g_hash_table_insert` 向哈希表中添加键值对.
3. **遍历哈希表**:使用 `g_hash_table_foreach` 遍历哈希表,并对每个键值对调用回调函数 `print_key_value`.
4. **回调函数**:`print_key_value` 打印每个键值对.
5. **销毁哈希表**:使用 `g_hash_table_destroy` 销毁哈希表,释放内存.

通过这种方式,可以方便地对 `GHashTable` 中的所有键值对进行操作.

## 参考

- [GLib介绍与使用](https://blog.csdn.net/u014338577/article/details/52932358)
- [Glib 主事件循环轻度分析与编程应用](https://blog.csdn.net/song_lee/article/details/116809089)