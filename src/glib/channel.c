#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <gio/gio.h>
#include <glib-unix.h>
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define PORT        8080
#define BUFFER_SIZE 1024

// 客户端数据结构
typedef struct {
    GIOChannel *channel;
} ClientData;

gboolean client_io_callback(GIOChannel *source, GIOCondition condition, gpointer data) {
    gchar buffer[BUFFER_SIZE];
    gsize bytes_read;
    GError *error = NULL;

    if (condition & G_IO_IN) {
        GIOStatus status = g_io_channel_read_chars(source, buffer, sizeof(buffer), &bytes_read, &error);
        if (status == G_IO_STATUS_NORMAL) {
            buffer[bytes_read] = '\0';  // 确保字符串终止
            printf("Received: %s\n", buffer);
            // 回显数据
            g_io_channel_write_chars(source, buffer, bytes_read, NULL, &error);
            g_io_channel_flush(source, NULL);
        } else if (status == G_IO_STATUS_ERROR) {
            // 处理错误
            fprintf(stderr, "Read error: %s\n", error->message);
            g_error_free(error);
            return FALSE;  // 移除回调
        } else if (status == G_IO_STATUS_EOF) {
            // 处理客户端断开
            printf("Client disconnected.\n");
            return FALSE;  // 移除回调
        }
    }

    return TRUE;  // 保持回调
}

gboolean server_io_callback(GIOChannel *source, GIOCondition condition, gpointer data) {
    int server_fd = g_io_channel_unix_get_fd(source);
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);

    if (client_fd < 0) {
        perror("accept");
        return TRUE;
    }

    printf("Client connected.\n");

    // 设置客户端套接字为非阻塞模式
    int flags = fcntl(client_fd, F_GETFL, 0);
    fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);

    // 创建 GIOChannel 用于客户端套接字
    GIOChannel *client_channel = g_io_channel_unix_new(client_fd);
    ClientData *client_data = g_new0(ClientData, 1);
    client_data->channel = client_channel;

    // 添加客户端套接字到主循环中
    g_io_add_watch(client_channel, G_IO_IN | G_IO_HUP | G_IO_ERR, client_io_callback, client_data);

    return TRUE;  // 保持回调
}

int main(int argc, char *argv[]) {
    int server_fd;
    struct sockaddr_in server_addr;

    // 创建服务器套接字
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // 设置服务器地址和端口
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // 绑定套接字
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // 开始监听
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // 设置服务器套接字为非阻塞模式
    int flags = fcntl(server_fd, F_GETFL, 0);
    fcntl(server_fd, F_SETFL, flags | O_NONBLOCK);

    // 创建 GIOChannel 用于服务器套接字
    GIOChannel *server_channel = g_io_channel_unix_new(server_fd);

    // 添加服务器套接字到主循环中
    g_io_add_watch(server_channel, G_IO_IN | G_IO_HUP | G_IO_ERR, server_io_callback, NULL);

    // 创建和运行主循环
    GMainLoop *main_loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(main_loop);

    // 清理资源
    g_io_channel_unref(server_channel);
    g_main_loop_unref(main_loop);
    close(server_fd);

    return 0;
}
