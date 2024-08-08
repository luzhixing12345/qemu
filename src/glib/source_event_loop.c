#include <glib.h>
#include <stdio.h>

// 定义自定义事件源的数据结构
typedef struct {
    GSource source;  // 基础事件源结构
    int counter;     // 计数器
} CustomSource;

// 自定义事件源的准备函数
static gboolean custom_source_prepare(GSource *source, gint *timeout) {
    *timeout = 1000;  // 设置超时时间为1000毫秒
    return FALSE;     // 返回FALSE表示不立即触发事件
}

// 自定义事件源的检查函数
static gboolean custom_source_check(GSource *source) {
    return TRUE;  // 总是返回TRUE表示总是触发事件
}

// 自定义事件源的分派函数
static gboolean custom_source_dispatch(GSource *source, GSourceFunc callback, gpointer user_data) {
    CustomSource *custom_source = (CustomSource *)source;  // 将事件源转换为CustomSource
    custom_source->counter--;                              // 计数器减1

    printf("Counter: %d\n", custom_source->counter);  // 打印当前计数器值

    if (custom_source->counter <= 0) {
        return FALSE;  // 当计数器小于等于0时返回FALSE停止事件源
    }

    return TRUE;  // 返回TRUE表示继续运行事件源
}

// 定义自定义事件源的函数表
static GSourceFuncs custom_source_funcs = {
    custom_source_prepare,   // 准备函数
    custom_source_check,     // 检查函数
    custom_source_dispatch,  // 分派函数
    NULL                     // 关闭函数(可选)
};

int callback_func(gpointer user_data) {
    printf("Callback function\n");
    return 0;
}

int main(int argc, char *argv[]) {
    GMainLoop *loop = g_main_loop_new(NULL, FALSE);  // 创建主循环

    // 创建自定义事件源
    CustomSource *custom_source = (CustomSource *)g_source_new(&custom_source_funcs, sizeof(CustomSource));
    custom_source->counter = 10;  // 初始化计数器

    g_source_attach((GSource *)custom_source, NULL);  // 将事件源添加到默认上下文中

    g_main_loop_run(loop);  // 运行主循环

    g_main_loop_unref(loop);                   // 释放主循环
    g_source_unref((GSource *)custom_source);  // 释放事件源

    return 0;
}
