#include <glib.h>
#include <stdio.h>

static int num = 0;
GMutex mutex;
GCond cond;

gboolean _thread_main1(gpointer data) {
    while (1) {
        g_mutex_lock(&mutex);
        while (num <= 0) {
            g_print("consumer[%d] is wating.....\n", (int)data);
            g_cond_wait(&cond, &mutex);
            g_print("consumer[%d] wake up.....\n", (int)data);
        }
        g_print("consmuer[%d] before,num = %d.....\n", (int)data, num);
        num--;
        g_print("consmuer[%d] after,num = %d.....\n", (int)data, num);
        g_mutex_unlock(&mutex);
        sleep(1);
    }
    return TRUE;
}

gboolean _thread_main2(gpointer data) {
    while (1) {
        g_mutex_lock(&mutex);
        num++;
        if (num > 0) {
            g_print("prepare to sigal...please wait for 5 seconds\n");
            sleep(5);
            g_cond_signal(&cond);
            g_print("after g_cond_signal\n");
        }
        g_mutex_unlock(&mutex);
        sleep(2);
    }
    return TRUE;
}

int main(int argc, char *argv[]) {
    GThread *consumer1 = NULL;
    GThread *consumer2 = NULL;
    GThread *consumer3 = NULL;

    GThread *thread2 = NULL;

    g_mutex_init(&mutex);
    g_cond_init(&cond);

    consumer1 =
        g_thread_new("consumer1", (GThreadFunc)_thread_main1, (void *)1);
    consumer2 =
        g_thread_new("consumer2", (GThreadFunc)_thread_main1, (void *)2);
    consumer3 =
        g_thread_new("consumer3", (GThreadFunc)_thread_main1, (void *)3);

    thread2 = g_thread_new("thread2", (GThreadFunc)_thread_main2, NULL);

    g_thread_join(consumer1);
    g_thread_join(consumer2);
    g_thread_join(consumer3);

    g_thread_join(thread2);
    return 0;
}