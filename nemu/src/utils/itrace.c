#include <common.h>

#ifdef CONFIG_ITRACE

/*使用数组来实现环形缓冲区，用这个版本*/
#define MAX_IRINGBUF 16//L 最大可存放指令的条数
void disassemble(char *str, int size, uint64_t pc, uint8_t *code, int nbyte);

typedef struct {
  word_t pc;
  uint32_t inst;
} ItraceNode;

ItraceNode iringbuf[MAX_IRINGBUF];//大小为 16 的数组，每个元素记录一条指令的地址和机器码。
int p_cur = 0;
bool full = false;

void trace_inst(word_t pc, uint32_t inst) {//L 向缓冲区存
  iringbuf[p_cur].pc = pc;
  iringbuf[p_cur].inst = inst;
  p_cur = (p_cur + 1) % MAX_IRINGBUF;
  full = full || p_cur == 0;
}

void display_inst() {//L 对缓冲区进行取（展示经过反汇编的指令到屏幕上）
  if (!full && !p_cur) return;

  int end = p_cur;
  int i = full?p_cur:0;

  char buf[128]; // 128 should be enough!
  char *p;
  printf("Most recently executed instructions");
  do {
    p = buf;
    p += sprintf(buf, "%s" FMT_WORD ": %08x ", (i+1)%MAX_IRINGBUF==end?" --> ":"     ", iringbuf[i].pc, iringbuf[i].inst);
    disassemble(p, buf+sizeof(buf)-p, iringbuf[i].pc, (uint8_t *)&iringbuf[i].inst, 4);

    if ((i+1)%MAX_IRINGBUF==end) printf(ANSI_FG_RED);
    puts(buf);
  } while ((i = (i+1)%MAX_IRINGBUF) != end);
  puts(ANSI_NONE);
}
#endif

#ifdef CONFIG_MTRACE

void display_pread(paddr_t addr, int len) {
  printf("pread at " FMT_PADDR " len=%d\n", addr, len);
}

void display_pwrite(paddr_t addr, int len, word_t data) {
  printf("pwrite at " FMT_PADDR " len=%d, data=" FMT_WORD "\n", addr, len, data);
}
#endif