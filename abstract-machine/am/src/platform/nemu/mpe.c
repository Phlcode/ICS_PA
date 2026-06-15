#include <am.h>
#include <stdatomic.h>
#include <klib-macros.h>

bool mpe_init(void (*entry)()) {//L 启动多处理器
  entry();
  panic("MPE entry returns");
}

int cpu_count() {//L 处理器个数
  return 1;
}

int cpu_current() {//L 返回当前执行流的CPU编号，处理器的编号
  return 0;
}

int atomic_xchg(int *addr, int newval) {//L 内存交换
  return atomic_exchange(addr, newval);
}
