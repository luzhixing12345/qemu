#include <glib.h>
#include <glib/gprintf.h>
#include <stdio.h>

typedef struct {
    GSource source;
    gchar text[256];
    int counter;
} MySource;

static gboolean prepare(GSource *source, gint *timeout) {
    *timeout = 1000;
    printf("Source prepared\n");
    return FALSE;
}

static gboolean check(GSource *source) {
    printf("Source checked\n");
    return FALSE;
}

static gboolean dispatch(GSource *source, GSourceFunc callback, gpointer user_data) {
    MySource *mysource = (MySource *)source;
    if (callback) {
        callback(mysource->text);
    }

    if (mysource->counter > 0) {
        mysource->counter--;
        printf("Counter: %d\n", mysource->counter);
        return TRUE;
    } else {
        return FALSE;
    }
}

static void finalize(GSource *source) {
    printf("Source finalized\n");
}

int my_callback_func(gpointer user_data) {
    char *text = (char *)user_data;
    g_print("callback func:%s\n", text);
    return 0;
}

int main(void) {
    GMainLoop *loop = g_main_loop_new(NULL, TRUE);
    GMainContext *context = g_main_loop_get_context(loop);

    GSourceFuncs source_funcs = {prepare, check, dispatch, finalize};
    GSource *source = g_source_new(&source_funcs, sizeof(MySource));

    g_sprintf(((MySource *)source)->text, "Hello world!");
    ((MySource *)source)->counter = 5;
    g_source_set_callback(source, my_callback_func, ((MySource *)source)->text, NULL);
    g_source_attach(source, context);

    g_main_loop_run(loop);

    g_main_context_unref(context);
    g_source_unref(source);
    g_main_loop_unref(loop);

    return 0;
}