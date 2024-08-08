#include <glib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

// 线程数据结构
typedef struct {
    GMainContext *context;
    GMainLoop *loop;
} ThreadData;

// 线程函数,运行事件循环
void *thread_func(void *data) {
    ThreadData *thread_data = (ThreadData *)data;
    g_main_context_push_thread_default(thread_data->context);
    g_main_loop_run(thread_data->loop);
    g_main_context_pop_thread_default(thread_data->context);
    return NULL;
}

// 超时回调函数
gboolean timeout_callback(gpointer data) {
    const char *message = (const char *)data;
    printf("%s\n", message);
    return TRUE; // 返回 TRUE 使得定时器继续运行
}

int main() {
    // 创建两个上下文和主循环
    GMainContext *context1 = g_main_context_new();
    GMainLoop *loop1 = g_main_loop_new(context1, FALSE);

    GMainContext *context2 = g_main_context_new();
    GMainLoop *loop2 = g_main_loop_new(context2, FALSE);

    // 创建线程数据
    ThreadData thread_data1 = {context1, loop1};
    ThreadData thread_data2 = {context2, loop2};

    // 在每个上下文中添加超时事件源
    GSource *timeout_source1 = g_timeout_source_new(1000);
    g_source_set_callback(timeout_source1, timeout_callback, "Context 1: Hello every second!", NULL);
    g_source_attach(timeout_source1, context1);
    g_source_unref(timeout_source1);

    GSource *timeout_source2 = g_timeout_source_new(2000);
    g_source_set_callback(timeout_source2, timeout_callback, "Context 2: Hello every two seconds!", NULL);
    g_source_attach(timeout_source2, context2);
    g_source_unref(timeout_source2);

    // 创建线程
    pthread_t thread1, thread2;
    pthread_create(&thread1, NULL, thread_func, &thread_data1);
    pthread_create(&thread2, NULL, thread_func, &thread_data2);

    // 等待线程结束
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

    // 清理
    g_main_loop_unref(loop1);
    g_main_context_unref(context1);

    g_main_loop_unref(loop2);
    g_main_context_unref(context2);

    return 0;
}
