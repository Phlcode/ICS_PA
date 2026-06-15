#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

//L 返回字符串的长度 遇到\0为止,不会返回\0的长度
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
  *dst = '\0';//L 需要拷贝末尾的'\0'，这个结束符号很重要，要加上，不然容易造成缓冲区溢出
  return temp;
  // panic("Not implemented");
}

//L strncpy 用于将源字符串 src 中最多 n 个字符复制到目标字符数组 dest 中。
/**
如果 src 的长度小于 n，则先完整复制 src（包括结尾的空字符 '\0'），然后将 dest 中剩余的位置全部用 '\0' 填充，直到总共写入了 n 个字符。
如果 src 的长度大于或等于 n，则只复制 src 的前 n 个字符到 dest，不会在末尾自动添加 '\0'。此时 dest 将不是一个以空字符结尾的合法 C 字符串。
*/
char *strncpy(char *dst, const char *src, size_t n) {
  // size_t src_len = strlen(src);
  char *temp = dst;
  
    // 复制直到 n 耗尽 或 遇到 '\0'
  while (n > 0 && (*dst++ = *src++) != '\0')//如果复制了n个字符后src还未结束 直接跳出循环，也不会添加\0
      n--;

  // 如果 n 还有剩余，用 '\0' 填充
  while (n-- > 0)
      *dst++ = '\0';
  
  // if(src_len < n){//src 的长度小于 n
  //   while(n--){
  //     if(*src == '\0'){
  //       *dst++ = '\0';
  //     }
  //     else{
  //       *dst++ = *src++;
  //     }
  //   }
  // }
  // else{
  //   while(n--){
  //   *dst++ = *src++;
  //   }
  // }
  return temp;
  // panic("Not implemented");
}

//L 字符串拼接 返回dst的首地址
/**
把 src 所指向的字符串（包括结尾的 '\0'）追加到 dest 所指向的字符串的末尾。
它会覆盖 dest 末尾的 '\0'，然后将 src 的字符依次复制过去。
拼接完成后，会自动在最后添加一个新的 '\0' 作为结束符。
要求： dest 指向的字符数组必须有足够的空间来容纳拼接后的整个字符串（dest 原长度 + src 长度 + 1）。空间不足会导致缓冲区溢出，引发未定义行为
 * 
*/
char *strcat(char *dst, const char *src) {
  char *temp = dst;
  while(*dst) // \0的ascii值是0
    dst++;
  while((*dst++ = *src++))// 必须带一个语句作为循环体,C中赋值表达式的值就是被赋的那个值
    ;
  return temp;
  // panic("Not implemented");
}

/**
 * 按字典序（字符的 ASCII 码）逐个比较 str1 和 str2，直到出现不同字符或遇到字符串结束符 '\0'  baseline s1
*/
int strcmp(const char *s1, const char *s2) {
  while(*s1 && (*s1 == *s2)){
    s1++;
    s2++;
  }
  return (unsigned char)*s1 - (unsigned char)*s2;

  // panic("Not implemented");
}

/**
 * 与 strcmp 类似，按字典序（ASCII 码）逐个比较 s1 和 s2，但最多比较 n 个字符。
遇到以下任一条件时停止：
  已比较了 n 个字符；
  遇到不同字符；
  遇到字符串结束符 '\0'。
'\0' 仍参与比较，且后续字符不再比较。
*/
int strncmp(const char *s1, const char *s2, size_t n) {
  if (n == 0) return 0;
  while(--n && s1 && *s1 == *s2){
    s1++;
    s2++;
  }
  return (unsigned char)*s1 - (unsigned char)*s2;
  // panic("Not implemented");
}

/** 
将指针 s 指向的内存块的前 n 个字节，每个字节都设置为值 c（c 会被转换为 unsigned char）。返回目标内存的起始地址
  常用于：清零数组、初始化结构体、填充缓冲区等。
  注意：是按字节设置，不是按 int 或其它类型设置。例如 memset(arr, 1, sizeof(arr)) 会把整型数组的每个字节都设成 0x01，而不是把每个 int 设成 1。
*/
void *memset(void *s, int c, size_t n) {
  unsigned char *temp = s;
  while(n--){
    *temp++ = (unsigned char)c;
  }
  return s;
  // panic("Not implemented");
}

/**
将 src 指向的内存块的前 n 个字节，复制到 dest 指向的内存块中。返回 dest（目标内存的起始地址）
与 memcpy 的关键区别：
  memmove 保证即使 dest 和 src 的内存区域存在重叠，复制结果也是正确的（如同先将 src 复制到一个临时缓冲区，再从缓冲区复制到 dest）。
  memcpy 不处理重叠情况，此时行为是未定义的。
*/
void *memmove(void *dst, const void *src, size_t n) {
  unsigned char *d = dst;
  unsigned const char *s = src;

  if (d < s){//dst 在 src 左侧 正向复制
    while(n--){
      *d++ = *s++;
    }
  }
  else if (d > s){//dst 在 src 右侧,反向复制
    d = d + n;
    s = s + n;
    while(n--){
      *(d--) = *s--;
    }
  }
  //如果 d == s,即首地址都相同,不需要操作,不move,
  return dst;
  // panic("Not implemented");
}
/**
将 src 指向的内存块的前 n 个字节，原样复制到 dest 指向的内存块中。 前提条件是源和目标内存区域不重叠。返回目标内存的起始地址
  重叠行为：如果 dest 和 src 存在重叠，结果是未定义的（可能正确，也可能出错）。
  适用于：复制独立缓冲区（如动态分配的内存、不同数组、I/O 缓冲区等）。
*/
void *memcpy(void *out, const void *in, size_t n) {
  unsigned char *d = out;
  const unsigned char *s = in;
  while (n--) {
      *d++ = *s++;
  }
  return out;
  // panic("Not implemented");
}

/**
将 s1 和 s2 指向的内存块的前 n 个字节进行比较，比较时每个字节被解释为 unsigned char。返回值是一个 int，表示两块内存的大小关系（按第一个不相等的字节的差值来定）：
  常用于：判断两个缓冲区或结构体的内容是否相等。
  比较是按字节的字典序，不会因为遇到 \0 而停止（这一点与 strcmp 完全不同）。
*/
int memcmp(const void *s1, const void *s2, size_t n) {
  const unsigned char *p1 = s1;
  const unsigned char *p2 = s2;
  while (n--) {
    if (*p1 != *p2){
      return *p1 - *p2;//返回两个unsigned char的差值, C中所有比int窄的整数列席在参与运算前都会县被隐是
    }
    p1++;
    p2++;
  }
  return 0;
  // panic("Not implemented");
}

#endif
