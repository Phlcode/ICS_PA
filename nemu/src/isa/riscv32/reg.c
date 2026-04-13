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

#include <isa.h>
#include "local-include/reg.h"

const char *regs[] = {//L 寄存器的名字 对应的值在cpu.gpr中
  "$0", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
  "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
  "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
  "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
};

void isa_reg_display() {
  bool success = false;
  printf("Reg \t Hex\t\t Dec\n");
  printf("%s = \t 0x%08x \t %0u \n", "PC", cpu.pc, cpu.pc);
  for (int i = 0; i < sizeof(regs)/sizeof(const char*); i++){
    word_t RegVal = isa_reg_str2val(regs[i], &success);
    printf("%s = \t 0x%08x \t %0u \n", regs[i], RegVal, RegVal);
}

}

//L 通过寄存器名称来获取寄存器值的函数
word_t isa_reg_str2val(const char *s, bool *success) {
  for (int i = 0; i < sizeof(regs)/sizeof(const char*); i++){
    if (strcmp(s, regs[i]) == 0){
      *success = true;
      return cpu.gpr[i];
    }
  }
  // *success = false;
  printf("the %s reg name is incorrect!s\n", s);
  return 0;
}
