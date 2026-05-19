#include <am.h>
#include <klib-macros.h>

extern char _heap_start;
int main(const char *args);

extern char _pmem_start;
#define PMEM_SIZE (128 * 1024 * 1024)
#define PMEM_END  ((uintptr_t)&_pmem_start + PMEM_SIZE)

Area heap = RANGE(&_heap_start, PMEM_END);//L 指示堆区的起始和末尾
#ifndef MAINARGS
#define MAINARGS ""
#endif
static const char mainargs[] = MAINARGS;

void putch(char ch) {//L 用于输出一个字符
}

void halt(int code) {//L 用于结束程序的运行
  while (1);
}

void _trm_init() {//L 用于进行TRM相关的初始化工作
  int ret = main(mainargs);
  halt(ret);
}
