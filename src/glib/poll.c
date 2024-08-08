#include <fcntl.h>
#include <glib.h>
#include <poll.h>
#include <stdio.h>
#include <unistd.h>

typedef struct {
    GSource source;
    GPollFD poll_fd;
} FDSource;

// 自定义事件源的准备函数
static gboolean fd_source_prepare(GSource *source, gint *timeout) {
    *timeout = -1;  // 无限等待
    return FALSE;   // 不准备好,继续等待
}

// 自定义事件源的检查函数
static gboolean fd_source_check(GSource *source) {
    FDSource *fd_source = (FDSource *)source;
    return (fd_source->poll_fd.revents & POLLIN) != 0;
}

// 自定义事件源的分派函数
static gboolean fd_source_dispatch(GSource *source, GSourceFunc callback, gpointer user_data) {
    FDSource *fd_source = (FDSource *)source;
    if (fd_source->poll_fd.revents & POLLIN) {
        char buf[256];
        ssize_t len = read(fd_source->poll_fd.fd, buf, sizeof(buf) - 1);
        if (len > 0) {
            buf[len] = '\0';
            printf("Read: %s\n", buf);
            return TRUE;
        }
    }
    return FALSE;  // 错误或关闭文件描述符时返回 FALSE
}

// 自定义事件源的销毁函数
static void fd_source_finalize(GSource *source) {
    FDSource *fd_source = (FDSource *)source;
    close(fd_source->poll_fd.fd);
}

static GSourceFuncs fd_source_funcs = {fd_source_prepare, fd_source_check, fd_source_dispatch, fd_source_finalize};

int main(int argc, char *argv[]) {
    GMainContext *context = g_main_context_new();       // 创建新的上下文
    GMainLoop *loop = g_main_loop_new(context, FALSE);  // 使用新上下文创建主循环

    // 创建自定义事件源
    GSource *source = g_source_new(&fd_source_funcs, sizeof(FDSource));
    FDSource *fd_source = (FDSource *)source;
    fd_source->poll_fd.fd = STDIN_FILENO;  // 使用标准输入
    fd_source->poll_fd.events = POLLIN;

    // 将事件源的 poll_fd 注册到上下文中
    g_source_add_poll(source, &fd_source->poll_fd);
    g_source_attach(source, context);  // 将事件源添加到指定上下文中

    g_main_loop_run(loop);  // 运行主循环

    g_main_loop_unref(loop);        // 释放主循环
    g_source_unref(source);         // 释放事件源
    g_main_context_unref(context);  // 释放上下文

    return 0;
}
