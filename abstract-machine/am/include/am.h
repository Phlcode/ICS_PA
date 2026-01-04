#ifndef AM_H__
#define AM_H__

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include ARCH_H // this macro is defined in $CFLAGS
                // examples: "arch/x86-qemu.h", "arch/native.h", ...

// Memory protection flags
#define MMAP_NONE  0x00000000 // no access
#define MMAP_READ  0x00000001 // can read
#define MMAP_WRITE 0x00000002 // can write

// Memory area for [@start, @end) L左闭右开
typedef struct {
  void *start, *end;
} Area;

// Arch-dependent processor context
typedef struct Context Context;

// An event of type @event, caused by @cause of pointer @ref
typedef struct {
  enum {
    EVENT_NULL = 0,
    EVENT_YIELD, EVENT_SYSCALL, EVENT_PAGEFAULT, EVENT_ERROR,
    EVENT_IRQ_TIMER, EVENT_IRQ_IODEV,
  } event;
  uintptr_t cause, ref;
  const char *msg;
} Event;

// A protected address space with user memory @area
// and arch-dependent @ptr
typedef struct {
  int pgsize;
  Area area;
  void *ptr;
} AddrSpace;

#ifdef __cplusplus
extern "C" {
#endif

// ----------------------- TRM: Turing Machine -----------------------
extern   Area        heap;//L 可以使用的物理内存堆区
void     putch       (char ch);//L 打印字符 向默认的调试终端打印ascii码为ch的字符
void     halt        (int code) __attribute__((__noreturn__));//L 终止整个AbstractMachine的运行，并返回数字编号 0-255

// -------------------- IOE: Input/Output Devices --------------------//L IO设备管理
bool     ioe_init    (void);//L 初始化I/O拓展
void     ioe_read    (int reg, void *buf);
void     ioe_write   (int reg, void *buf);//L I/O设备读写
#include "amdev.h"

// ---------- CTE: Interrupt Handling and Context Switching ----------//L 上下文和中断管理
bool     cte_init    (Context *(*handler)(Event ev, Context *ctx));
void     yield       (void);//L self-trapping
bool     ienabled    (void);
void     iset        (bool enable);//L 外部中断管理
Context *kcontext    (Area kstack, void (*entry)(void *), void *arg);

// ----------------------- VME: Virtual Memory -----------------------//L 虚拟地址空间
bool     vme_init    (void *(*pgalloc)(int), void (*pgfree)(void *));//L 初始化虚存管理
void     protect     (AddrSpace *as);
void     unprotect   (AddrSpace *as);//L 地址空间管理
void     map         (AddrSpace *as, void *vaddr, void *paddr, int prot);//L 修改地址空间映射
Context *ucontext    (AddrSpace *as, Area kstack, void *entry);//L 创建被保护的用户态进程上下文

// ---------------------- MPE: Multi-Processing ----------------------//L 共享内存多处理器
bool     mpe_init    (void (*entry)());//L 启动多处理器
int      cpu_count   (void);//L 返回系统中处理器的个数
int      cpu_current (void);//L 当前处理器的编号
int      atomic_xchg (int *addr, int newval);//L 内存交换，原子地交换内存地址中的数值

#ifdef __cplusplus
}
#endif

#endif
