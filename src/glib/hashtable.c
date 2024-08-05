#include <glib.h>
#include <stdio.h>

struct MyData {
    int x;
    int y;
};

// 回调函数,用于处理每个键值对
void print_key_value(gpointer key, gpointer value, gpointer user_data) {
    printf("Key: %s, Value: %s\n", (char *)key, (char *)value);
    struct MyData *data = (struct MyData *)user_data;
    printf("x = %d, y = %d\n", data->x, data->y);
}

int main() {
    // 创建一个哈希表
    GHashTable *hash_table = g_hash_table_new(g_str_hash, g_str_equal);

    // 添加键值对到哈希表
    g_hash_table_insert(hash_table, "name", "Alice");
    g_hash_table_insert(hash_table, "city", "Wonderland");
    g_hash_table_insert(hash_table, "occupation", "Adventurer");

    // 使用 g_hash_table_foreach 遍历哈希表
    struct MyData data = { 10, 20 };
    g_hash_table_foreach(hash_table, print_key_value, &data);

    // 销毁哈希表
    g_hash_table_destroy(hash_table);

    return 0;
}