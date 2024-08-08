#include <glib.h>
#include <stdio.h>
#include <stdlib.h>

// 超时回调函数
gboolean timeout_callback(gpointer data) {
    const char *message = (const char *)data;
    printf("%s\n", message);
    return TRUE; // 返回 TRUE 使得定时器继续运行
}

// 销毁通知函数
void destroy_notify(gpointer data) {
    char *message = (char *)data;
    printf("Destroying message: %s\n", message);
    free(message);
}

int main() {
    // 创建上下文和主循环
    GMainContext *context = g_main_context_new();
    GMainLoop *loop = g_main_loop_new(context, FALSE);

    // 动态分配内存用于消息
    char *message = g_strdup("Hello every second!");

    // 创建超时事件源
    GSource *timeout_source = g_timeout_source_new(1000);

    // 设置回调函数和销毁通知函数
    g_source_set_callback(timeout_source, timeout_callback, message, destroy_notify);

    // 将事件源附加到上下文
    g_source_attach(timeout_source, context);

    // 启动一个线程在5秒后停止主循环
    g_timeout_add_seconds(5, (GSourceFunc)g_main_loop_quit, loop);

    // 运行主循环
    g_main_loop_run(loop);

    // 清理
    g_source_unref(timeout_source);
    g_main_loop_unref(loop);
    g_main_context_unref(context);

    return 0;
}
