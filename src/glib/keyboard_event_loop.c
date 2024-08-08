#include <fcntl.h>
#include <glib.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>

typedef struct {
    GSource source;
    GMainLoop *loop;
    char buf;
} KeyboardSource;

// 函数用于设置终端为非阻塞模式
void set_terminal_mode() {
    struct termios new_termios;
    tcgetattr(0, &new_termios);
    new_termios.c_lflag &= ~ICANON;  // 关闭规范模式
    new_termios.c_lflag &= ~ECHO;    // 关闭回显
    tcsetattr(0, TCSANOW, &new_termios);
}

// 函数用于恢复终端为阻塞模式
void reset_terminal_mode() {
    struct termios new_termios;
    tcgetattr(0, &new_termios);
    new_termios.c_lflag |= ICANON;  // 开启规范模式
    new_termios.c_lflag |= ECHO;    // 开启回显
    tcsetattr(0, TCSANOW, &new_termios);
}

// 设置文件描述符为非阻塞模式
void set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

// 自定义事件源的准备函数
static gboolean keyboard_source_prepare(GSource *source, gint *timeout) {
    *timeout = 0;  // 没有延迟
    return FALSE;  // 返回 FALSE 说明需要 poll(fd)
}

// 自定义事件源的检查函数
static gboolean keyboard_source_check(GSource *source) {
    KeyboardSource *keyboard_source = (KeyboardSource *)source;
    if (read(STDIN_FILENO, &keyboard_source->buf, 1) > 0) {
        return TRUE;
    }
    return FALSE;
}

// 自定义事件源的分派函数
static gboolean keyboard_source_dispatch(GSource *source, GSourceFunc callback, gpointer user_data) {
    KeyboardSource *keyboard_source = (KeyboardSource *)source;

    char buf = keyboard_source->buf;
    if (buf != 'q') {
        printf("Key '%c' pressed\n", buf);
    } else {
        printf("Key 'q' pressed, exiting...\n");
        g_main_loop_quit(keyboard_source->loop);
        return FALSE;
    }
    return TRUE;
}

// 自定义事件源的销毁函数
static void keyboard_source_finalize(GSource *source) {
    printf("Keyboard source finalized\n");
}

static GSourceFuncs keyboard_source_funcs = {
    keyboard_source_prepare, keyboard_source_check, keyboard_source_dispatch, keyboard_source_finalize};

int main(int argc, char *argv[]) {
    set_terminal_mode();
    set_nonblocking(STDIN_FILENO);

    GMainContext *context = g_main_context_new();       // 创建新的上下文
    GMainLoop *loop = g_main_loop_new(context, FALSE);  // 使用新上下文创建主循环

    // 创建自定义事件源
    GSource *source = g_source_new(&keyboard_source_funcs, sizeof(KeyboardSource));
    KeyboardSource *keyboard_source = (KeyboardSource *)source;
    keyboard_source->loop = loop;

    g_source_attach(source, context);  // 将事件源添加到指定上下文中

    g_main_loop_run(loop);  // 运行主循环

    g_main_loop_unref(loop);        // 释放主循环
    g_source_unref(source);         // 释放事件源
    g_main_context_unref(context);  // 释放上下文

    reset_terminal_mode();

    return 0;
}
