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