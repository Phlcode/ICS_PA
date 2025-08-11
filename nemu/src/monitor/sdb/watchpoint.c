/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include "sdb.h"

#define NR_WP 32//L 定义监视点的最大数量为32个

typedef struct watchpoint {
  int NO;//L 监视点编号
  struct watchpoint *next;//L 指向下一个结构体的指针 链表指针

  /* TODO: Add more members if necessary */
  char str[128];//保存监视的表达式字符串
  word_t old_value;//保存表达式的旧值
} WP;

static WP wp_pool[NR_WP] = {};//L 包含32个WP结构体的静态数组
//L head指向已激活监视点组成的链表（使用中的监视点） free_指向可用监视点组成的链表（空闲池）
//L 两个链表的总结点数为32
static WP *head = NULL, *free_ = NULL;//L 这个地方定义的是头指针

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    //L 构建链表：当前元素指向下一个，最后一个指向NULL 
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
  }

  head = NULL;//L 初始化使用中的监视点 初始化为空
  free_ = wp_pool;//L 空闲链表指向数组首元素
}

/* TODO: Implement the functionality of watchpoint */

void add_watchpoint(char* str){//L 给定表达式str 添加一个监视点 用于实现命令w
  Assert(free_ != NULL, "no free memory for new watchpoint");
  bool success = false;
  int value = expr(str, &success);
  if (!success){
    printf("error: wrong expression %s\n", str);
    return;
  }
  WP *ptr = free_;
  free_ = free_->next;
  int str_length = strlen(str);//L 计算从str指针位置开始扫描，直到遇到'\0'前的字符数量 最大127
  strncpy(ptr->str, str, str_length);
  ptr->str[str_length] = '\0';//L 这个地方要注意，strncpy需要加结合符'\0'
  ptr->old_value = value;

  //L 在链表head的头部插入一个新节点 头插法.
  ptr->next = head;
  head = ptr;

  printf("Watchpoint %d: %s\n", ptr->NO, ptr->str);
}

void delete_watchpoint(int no){//L 删除编号对应的监视点，用于实现命令d 
  if (head->NO == no){//L 删除一号节点的情况
    WP* tmp = head;//创建临时指针tmp 保存当前头节点，当head不为空时，head指针就是指向的第一个节点
    head = head->next;//将链表头指针head后移
    tmp->next = free_;//被删节点的next指向 空闲链表的表头"head"，即空闲链表的第一个节点地址
    free_ = tmp;//将被删节点的地址赋给free_空闲链表头
  }
  else {//L 删除中间/尾部节点的情况 遍历链表 从删除二号节点开始
    WP* ptr = head;
    while(ptr->next){
      if (ptr->next->NO == no){
        WP* tmp = ptr->next;
        ptr->next = tmp->next;
        tmp->next = free_;
        free_ = tmp;
        break;
      }
      ptr = ptr->next;
    }
  }
}

void print_watchpoint(){//L 输出所有的监视点信息 用于实现命令info w
  if (head == NULL){
    printf("No watchpoints!\n");
    return;
  }
  printf("Num\t\tWhat\n");
  WP* ptr = head;
  while(ptr){
    printf("%d\t\t%s\n", ptr->NO, ptr->str);
    ptr = ptr->next;
  }
}

/**
 * @brief 观察所有监视点的值 若发生变化，更新对应监视点的值
 * 
 * @return int 监视点发生变化的个数
 */
int update_watchpoint(){
  int n_changed = 0;
  WP* ptr = head;
  while (ptr != NULL){
    bool success = false;
    word_t value = expr(ptr->str, &success);
    Assert(success, "Wrong expression %s\n", ptr->str);

    if (value != ptr->old_value){
      n_changed += 1;
      printf("Watchpoint %d: %s\n", ptr->NO, ptr->str);
      printf("                 Old value = 0x%08x(%d)\n", ptr->old_value, ptr->old_value);
      printf("                 New value = 0x%08x(%d)\n", value, value);
      ptr->old_value = value;
    }
    ptr = ptr->next;
  }
  return n_changed;
}
