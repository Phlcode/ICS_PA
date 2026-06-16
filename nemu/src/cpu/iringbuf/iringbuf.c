/*L 存放环形缓冲区相关的代码
基于单向链表的环形缓冲区（Ring Buffer）。虽然它叫“环形”，但这里的链表本身并不是一个真正的环（即尾节点不指向头节点），
而是通过“当缓冲区满时，自动丢弃最旧元素”这个行为，模拟了环形缓冲区的覆盖写逻辑。

实际的环形缓冲区 通常是基于数组实现的，利用索引取模循环
*/ 

#include "iringbuf.h"



// 函数实现 初始化，刚创建时，缓冲区是空的
void init_ringbuf(iringbuf *rb) {
	rb->head = NULL;
	rb->tail = NULL;//头节点指针和尾节点指针初始化时都指向NULL
	rb->count = 0;
}


// 在缓冲区末尾保存指令信息。压入消息 —— push，同时包含了添加和条件删除
void push(iringbuf *rb, const char *msg) {
    // 1. 新建一个节点
	Node *new_node = (Node *) malloc(sizeof(Node));	// 分配新节点内存
 
	// 2. 如果缓冲区已经满了 (count == 10)
	if (rb->count == IRINGBUF_MAX_LEN) {
		pop(rb);		// 先把最旧的那条消息删掉，腾出一个位置
	}
 
	// 3. 将消息内容复制到新节点
	strncpy(new_node->Msg, msg, sizeof(new_node->Msg) - 1);	// 复制信息
    new_node->Msg[127] = '\0';
	new_node->next = NULL;// 新节点将成为最后一个节点，所以 next 指向 NULL
 
    // 4. 把新节点挂到链表末尾
	if (rb->count == 0) { // 原本是空的
		rb->head = new_node;
		rb->tail = new_node;
	} else {
		rb->tail->next = new_node;	// 让原来的尾节点指向新节点
		rb->tail = new_node;		// 更新尾指针
	}

	rb->count++;
}

// 删除队列的首元素。删除最旧元素 —— pop
void pop(iringbuf *rb) {
	if(rb->count == 0) return; // 如果环形缓冲区是空的就不用删了，直接return
 
	Node *temp = rb->head;			// 保存当前头节点，即最旧的那个节点
	rb->head = rb->head->next;	// 更新头节点为下一个节点，头指针后移一位
 
	if (rb->head == NULL) {			// 仅有一个元素且删除后，尾节点也为空。即如果删完后链表空了，尾指针也要清空
		rb->tail = NULL;
	}
 
	rb->count--;
	free(temp);
}
 
// 销毁整个缓冲区 
void destroy_ringbuf(iringbuf *rb) {
	while( rb->count > 0 ) {
		pop(rb);
	}
}
 
// 打印缓冲区内容 
void print_ringbuf(iringbuf *rb) {
	// 获取缓冲区的首元素地址
	Node *temp = rb->head;
 
	for (int i = 0; i < rb->count; i++) {
		// 最后一个元素的特殊输出，打印 -->箭头
		if(i == rb->count - 1) {
			printf("  --> ");
		} else {
			printf("      ");
		}
 
		// 输出当前元素的信息
		printf("%s\n", temp->Msg);
		temp = temp->next;	// 更新临时节点为下一个节点
	}
}





