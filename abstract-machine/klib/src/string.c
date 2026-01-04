#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

//L 返回字符串的长度 遇到\0为止 不会返回\0的长度
size_t strlen(const char *s) {
  size_t len = 0;
  while(*(s++) != '\0'){
    len += 1;
  }
  return len;
  // panic("Not implemented");
}
//L 从位置src复制到位置dst,直到遇到字符'\0'为止,返回dst的初始地址。 整个字符串copy
char *strcpy(char *dst, const char *src) {
  char *temp = dst;
  while(*src != '\0'){
    *dst++ = *src++;
  }
  *dst = '\0';//L 需要拷贝末尾的'\0'，这个结束符号很重要，要加上
  return temp;
  // panic("Not implemented");
}
//L 
char *strncpy(char *dst, const char *src, size_t n) {
  char *temp = dst;
  while(n){
    if()
    n -= 1; 
  }
  panic("Not implemented");
}

char *strcat(char *dst, const char *src) {
  panic("Not implemented");
}

int strcmp(const char *s1, const char *s2) {
  panic("Not implemented");
}

int strncmp(const char *s1, const char *s2, size_t n) {
  panic("Not implemented");
}

void *memset(void *s, int c, size_t n) {
  panic("Not implemented");
}

void *memmove(void *dst, const void *src, size_t n) {
  panic("Not implemented");
}

void *memcpy(void *out, const void *in, size_t n) {
  panic("Not implemented");
}

int memcmp(const void *s1, const void *s2, size_t n) {
  panic("Not implemented");
}

#endif
