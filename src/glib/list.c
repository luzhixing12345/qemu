#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct _Teacher {
    gint age;
    gchar *name;
} Teacher;

void each_callback(gpointer data, gpointer user_data) {
    Teacher *t = (Teacher *)data;
    g_print("t->name = %s, user param:%s\n", t->name, (char *)user_data);
}

int main(int argc, char *argv[]) {
    GList *list = NULL;
    Teacher *pTeacher1 = g_new0(Teacher, 1);
    pTeacher1->name = g_new0(char, 128);
    strcpy(pTeacher1->name, "tiny Jason");
    list = g_list_append(list, pTeacher1);

    Teacher *pTeacher2 = g_new0(Teacher, 1);
    pTeacher2->name = g_new0(char, 128);
    strcpy(pTeacher2->name, "Rorash");
    list = g_list_prepend(list, pTeacher2);

    Teacher *pTeacher3 = g_new0(Teacher, 1);
    pTeacher3->name = g_new0(char, 128);
    strcpy(pTeacher3->name, "alibaba");
    list = g_list_prepend(list, pTeacher3);

    g_list_foreach(list, each_callback, "user_data");

    GList *second = g_list_find(list, pTeacher2);
    if (second != NULL) {
        Teacher *t = (Teacher *)second->data;
        g_print("name :%s\n", t->name);
    }

    list = g_list_remove(list, pTeacher2);

    g_list_foreach(list, each_callback, "user_data");

    gint len = g_list_length(list);
    g_print("len :%d\n", len);

    GList *nNode = g_list_nth(list, 1);
    if (nNode != NULL) {
        Teacher *t = (Teacher *)nNode->data;
        g_print("name :%s\n", t->name);
    }

    g_list_free(list);

    return 0;
}
