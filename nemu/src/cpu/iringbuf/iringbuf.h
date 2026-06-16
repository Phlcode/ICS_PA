#include <common.h>
#define IRINGBUF_MAX_LEN 10
typedef struct Node{
    char Msg[128];
    struct Node *next;
} Node;

typedef struct {
    Node *head;// 头节点指针，最早进入的消息
    Node *tail;// 尾节点指针，最晚进入的消息
    int count;// 当前链表中的元素数量 
} iringbuf;
// 整个缓冲区就是一条单项链表，head 指向最老的消息，tail 指向最新的消息。
// count 记录消息的条数，确保它永远不会超过 IRINGBUF_MAX_LEN

// 函数声明
void init_ringbuf(iringbuf *rb);
void pop(iringbuf *rb);
void push(iringbuf *rb, const char *msg);
void destroy_ringbuf(iringbuf *rb);
void print_ringbuf(iringbuf *rb);