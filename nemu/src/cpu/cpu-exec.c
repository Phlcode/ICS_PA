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

#include <cpu/cpu.h>
#include <cpu/decode.h>
#include <cpu/difftest.h>//L 编译时将nemu/include指定为查找头文件的目录，当编译器遇到尖括号括起的文件时，就会在这些指定的目录中寻找匹配的文件.
#include <locale.h>
// #include "iringbuf/iringbuf.h"

/* The assembly code of instructions executed is only output to the screen
 * when the number of instructions executed is less than this value.
 * This is useful when you use the `si' command.
 * You can modify this value as you want.
 */
#define MAX_INST_TO_PRINT 10

int update_watchpoint();
#ifdef CONFIG_ITRACE
void display_inst();
#endif
void trace_inst(word_t pc, uint32_t inst);

CPU_state cpu = {};
uint64_t g_nr_guest_inst = 0;
static uint64_t g_timer = 0; // unit: us
static bool g_print_step = false;

// static iringbuf* rb = NULL;

void device_update();

static void trace_and_difftest(Decode *_this, vaddr_t dnpc) {
#ifdef CONFIG_ITRACE_COND
  if (ITRACE_COND) { log_write("%s\n", _this->logbuf); }
#endif
  if (g_print_step) { IFDEF(CONFIG_ITRACE, puts(_this->logbuf)); }//L 执行10条以上的指令时，g_print_step是flase. puts将一个字符串输出到标准输出设备（通常是屏幕），并在末尾自动追加一个换行符 \n
  IFDEF(CONFIG_DIFFTEST, difftest_step(_this->pc, dnpc));
  #ifdef CONFIG_WATCHPOINT
    if (update_watchpoint() > 0){//L 当监视点的值发生变化 则暂停程序.
      nemu_state.state = NEMU_STOP;
    }
  #endif
}

static void exec_once(Decode *s, vaddr_t pc) {//L 执行一条指令 传进来的是cpu.pc
  s->pc = pc;//L 当前指令的PC
  s->snpc = pc;//L static next PC下一条指令的PC
  isa_exec_once(s);//L 一条指令的具体执行，不同架构的指令执行不一样。
  cpu.pc = s->dnpc;
#ifdef CONFIG_ITRACE
  char *p = s->logbuf;
  p += snprintf(p, sizeof(s->logbuf), FMT_WORD ":", s->pc);
  int ilen = s->snpc - s->pc;//L 指令的字节数
  int i;
  uint8_t *inst = (uint8_t *)&s->isa.inst.val;
  for (i = ilen - 1; i >= 0; i --) {
    p += snprintf(p, 4, " %02x", inst[i]);//L 按照字节顺序打印指令的十六进制值
  }
  //L log加space_len个空格的间隔.
  int ilen_max = MUXDEF(CONFIG_ISA_x86, 8, 4);
  int space_len = ilen_max - ilen;
  if (space_len < 0) space_len = 0;
  space_len = space_len * 3 + 1;
  memset(p, ' ', space_len);
  p += space_len;

  // L log加入反汇编的结果
  /* L 参数包括
  	p: 指向日志缓冲区的指针
    s->logbuf + sizeof(s->logbuf) - p: 指定可用的缓冲区大小
    MUXDEF(CONFIG_ISA_x86, s->snpc, s->pc): 指定程序计数器
    (uint8_t *)&s->isa.inst.val：指向当前指令的代码
    ilen:指令的字节数
  */
#ifndef CONFIG_ISA_loongarch32r
  void disassemble(char *str, int size, uint64_t pc, uint8_t *code, int nbyte);
  disassemble(p, s->logbuf + sizeof(s->logbuf) - p,
      MUXDEF(CONFIG_ISA_x86, s->snpc, s->pc), (uint8_t *)&s->isa.inst.val, ilen);
#else
  p[0] = '\0'; // the upstream llvm does not support loongarch32r
#endif
#endif
}

static void execute(uint64_t n) {
  Decode s;
  for (;n > 0; n --) {//L 每次调用exec_once时，当前指令的地址都是通过cpu.pc指定的.
    exec_once(&s, cpu.pc);//L 让CPU执行当前PC指向的一条指令，然后更新PC. PC的初始值0x8000 0000.
    g_nr_guest_inst ++;//L 指令数目. 
    trace_and_difftest(&s, cpu.pc);
    if (nemu_state.state != NEMU_RUNNING) break;//L 每次调用exec_once后/即每次运行完一条指令后还会检查NEMU的状态。可能程序没执行到n条指令就结束了（例如，程序本身就没有n条指令），在这种情况下，程序结束运行后应当跳出循环。
    IFDEF(CONFIG_DEVICE, device_update());
  }
}

static void statistic() {
  IFNDEF(CONFIG_TARGET_AM, setlocale(LC_NUMERIC, ""));
#define NUMBERIC_FMT MUXDEF(CONFIG_TARGET_AM, "%", "%'") PRIu64
  Log("host time spent = " NUMBERIC_FMT " us", g_timer);//L 花费的时间. us
  Log("total guest instructions = " NUMBERIC_FMT, g_nr_guest_inst);//L 指令数目. 
  if (g_timer > 0) Log("simulation frequency = " NUMBERIC_FMT " inst/s", g_nr_guest_inst * 1000000 / g_timer);
  else Log("Finish running in less than 1 us and can not calculate the simulation frequency");
}

void assert_fail_msg() {
#ifdef CONFIG_ITRACE
  display_inst();
#endif
  isa_reg_display();
  statistic();
}

/** Simulate how the CPU works. 
 * 这个函数的作用是 
 * 1.执行n条指令，这一功能交给函数execute(n)完成
 * 2.在调用execute(n)前后检查NEMU的运行状态（即nemu_state.state）
 * 3.计算调用execute(n)耗费的时间（保存在变量g_timer中），以测量CPU的性能
*/
void cpu_exec(uint64_t n) {//L 传入-1时会发生隐式转换，变成一个很大的无符号数，这里可以理解为执行“无穷”步.
  g_print_step = (n < MAX_INST_TO_PRINT);//L 这个地方执行10条以上的指令时，g_print_step是flase.
  switch (nemu_state.state) {//L 默认state==NEMU_STOP.
    case NEMU_END: case NEMU_ABORT:
      printf("Program execution has ended. To restart the program, exit NEMU and run again.\n");
      return;
    default: nemu_state.state = NEMU_RUNNING;//0
  }

  uint64_t timer_start = get_time();
  // rb = (iringbuf *) malloc(sizeof(iringbuf));
  // init_ringbuf(rb);

  execute(n);//L 模拟CPU的工作方式，不断地执行指令.

  uint64_t timer_end = get_time();
  g_timer += timer_end - timer_start;//L 花费的时间

  switch (nemu_state.state) {
    case NEMU_RUNNING: nemu_state.state = NEMU_STOP; break;

    //L 当前状态如果不是NEMU_ABORT，同时nemu_state.halt_ret不为0时，则会打印HIT BAD TRAP
    case NEMU_END: case NEMU_ABORT://L 两个case分支公用一个代码块      
      Log("nemu: %s at pc = " FMT_WORD,
          (nemu_state.state == NEMU_ABORT ? ANSI_FMT("ABORT", ANSI_FG_RED) :
           (nemu_state.halt_ret == 0 ? ANSI_FMT("HIT GOOD TRAP", ANSI_FG_GREEN) :
            ANSI_FMT("HIT BAD TRAP", ANSI_FG_RED))),
          nemu_state.halt_pc);
      // print_ringbuf(rb);
      // destroy_ringbuf(rb);
      // fall through 没有break继续执行NEMU_QUIT
    case NEMU_QUIT: statistic();
  }
}
