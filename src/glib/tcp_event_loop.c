#include <arpa/inet.h>
#include <ctype.h>
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>

// 启动服务器
// netcat -l 127.0.0.1 8888

// 自定义 GSource 结构体,用于处理回声服务器
typedef struct GSourceEcho {
    GSource source;
    GIOChannel *channel;
    GPollFD fd;
} GSourceEcho;

// 设置超时时间并返回 FALSE 表示等待 poll 调用
gboolean g_source_echo_prepare(GSource *source, gint *timeout) {
    *timeout = -1;
    return FALSE;
}

// 检查 poll 调用的结果以查看事件是否发生
gboolean g_source_echo_check(GSource *source) {
    GSourceEcho *source_echo = (GSourceEcho *)source;
    return source_echo->fd.revents == source_echo->fd.events;
}

// 分发事件
gboolean g_source_echo_dispatch(GSource *source, GSourceFunc callback, gpointer user_data) {
    GSourceEcho *source_echo = (GSourceEcho *)source;
    return callback ? callback(source_echo->channel) : G_SOURCE_REMOVE;
}

// 释放 GSource 持有的资源
void g_source_echo_finalize(GSource *source) {
    GSourceEcho *source_echo = (GSourceEcho *)source;
    if (source_echo->channel) {
        g_io_channel_unref(source_echo->channel);
    }
}

// 回声处理函数
gboolean echo(GIOChannel *channel) {
    gsize len = 0;
    gchar *buf = NULL;
    g_io_channel_read_line(channel, &buf, &len, NULL, NULL);
    if (len > 0) {
        g_print("read: %s", buf);
        // 所有 ASCII 字符转为大写
        for (gsize i = 0; i < len; i++) {
            buf[i] = toupper(buf[i]);
        }
        g_print("send: %s", buf);
        g_io_channel_write_chars(channel, buf, len, NULL, NULL);
        g_io_channel_flush(channel, NULL);
    }
    g_free(buf);
    return TRUE;
}

// 连接到服务器的客户端套接字
int client_fd;

// 初始化客户端并连接到服务器
int client(void) {
    struct sockaddr_in serv_addr;
    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        return EXIT_FAILURE;
    }
    memset(&serv_addr, 0, sizeof serv_addr);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serv_addr.sin_port = htons(8888);
    return connect(client_fd, (struct sockaddr *)&serv_addr, sizeof serv_addr) == -1 ? -1 : 0;
}

int main(int argc, char *argv[]) {
    if (client() < 0) {
        g_print("start client error\n");
        return -1;
    }

    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    GMainContext *context = g_main_loop_get_context(loop);

    GSourceFuncs g_source_echo_funcs = {
        g_source_echo_prepare,
        g_source_echo_check,
        g_source_echo_dispatch,
        g_source_echo_finalize,
    };

    GSource *source = g_source_new(&g_source_echo_funcs, sizeof(GSourceEcho));
    GSourceEcho *source_echo = (GSourceEcho *)source;
    source_echo->channel = g_io_channel_unix_new(client_fd);
    source_echo->fd.fd = client_fd;
    source_echo->fd.events = G_IO_IN;
    g_source_add_poll(source, &source_echo->fd);
    g_source_set_callback(source, (GSourceFunc)echo, NULL, NULL);
    g_source_attach(source, context);
    g_source_unref(source);

    g_main_loop_run(loop);

    g_main_context_unref(context);
    g_main_loop_unref(loop);

    return 0;
}
