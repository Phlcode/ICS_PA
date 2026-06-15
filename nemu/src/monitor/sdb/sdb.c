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
#include <cpu/cpu.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "sdb.h"

word_t vaddr_read(vaddr_t addr, int len);
static int is_batch_mode = false;
void print_watchpoint();
void add_watchpoint(char* str);
void delete_watchpoint(int no);

void init_regex();
void init_wp_pool();

//L rl_gets 获得用户在命令行中敲入的字符串(回车键之前)；
/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
  static char *line_read = NULL;
  if (line_read) {
    free(line_read);
    line_read = NULL;
  }
//L 参数是一个字符串，调用函数的时候会在屏幕上输出，这个函数还会读取一行输入，然后返回一个指向输入字符串的指针
  line_read = readline("(nemu) ");//L readline函数实现了shell中的“显示命令提示符”和“读取用户输入的命令字符串”

  if (line_read && *line_read) {
    add_history(line_read);//L 实现记录历史命令的功能
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);//L 传入-1，
  return 0;
}

/**
 * @brief 执行n条指令后暂停执行:si n
 * 
 * @param args 
 * @return int 
 */
static int cmd_si(char *args) {
  uint64_t n = 0;
  /*
  if(NULL == args)//L 方法可行.
  {
    n = 1;
  }
  else
  {
    Assert(sscanf(args, "%lu", &n), "The input step n is not a number");
  }
  */
  char *token = strtok(NULL, " ");
  if (token != NULL){
    Assert(sscanf(token, "%lu", &n), "The input step n is not a number");//L 如果没有匹配到任何项sscanf返回0，则打印后面的字符串.从字符串token解析无符号长整数到n；sscanf函数的返回值表示成功匹配并赋值的参数个数
  }
  else{
    n = 1;
  }

  cpu_exec(n);
  return 0;
}

/**
 * @brief 打印寄存器状态：info r/info w
 * 
 * @param args 
 * @return int 
 */
static int cmd_info(char *args) {
  char* token = strtok(NULL, " ");
  Assert(token != NULL, "Your input is incorrect");
  if(strcmp(token, "r") == 0)
  {
    isa_reg_display();//L 打印寄存器状态
  }
  else if(strcmp(token, "w")== 0)
  {
    print_watchpoint();//L 打印监视点信息
    //L TODO
  }
  else{
    printf("You need to put the right op!\n");
  }

  return 0;
}

/** 打印起始地址Addr后的连续N个地址单元中的值：x N Addr    Addr是默认16进制 不需要输入0x
 * 
 * @param args 
 * @return int 
 */
static int cmd_x(char *args) {
  // char *str_end = args + strlen(args);

  // 读取第一个参数 无符号参数ArgsN，代表读取几个单元的地址
  uint32_t ArgsN = 0;
  char* token1 = strtok(NULL, " ");//L NULL 参数告诉 strtok 继续从上次的位置开始搜索
  if (token1 == NULL) {
		printf("error: missing arguments\n");
		return 0;
	}
  //或者使用Assert(token1 != NULL, "error: missing arguments\n")
  Assert((sscanf(token1, "%u", &ArgsN)), "The format of parameter N is incorrect.");//L 这里如果token1是NULL，sscanf内部会尝试访问该地址，导致段错误，所以需要对指针进行非空校验.

  // 读取第二个参数 无符号参数 ArgsAddr ，代表起始地址
  uint32_t ArgsAddr = 0;
  char* token2 = strtok(NULL, " ");
  if (token2 == NULL) {
		printf("error: missing arguments Addr\n");
		return 0;
	}
  
  Assert((sscanf(token2, "%x", &ArgsAddr)), "The format of parameter Addr is incorrect.");
  printf("[ADDRESS]:      VALUE(hex)\n");
  for(int i = 0; i < ArgsN; i++){
    printf("0x%08x:\t0x%08x\n", ArgsAddr, vaddr_read(ArgsAddr, 4)); //L 0x08x代表 以十六进制形式输出整数 共计8位，不够的使用0补足.
    ArgsAddr = ArgsAddr + 4; //L  内存地址
  }
  return 0;
}

/** 计算表达式的值：p 2 + 3 - 5
 * 
 * @param args 
 * @return int 
 */
static int cmd_p(char *args){
  if (args == NULL){
    printf("error: no experssion given\n");
    return 0;
  }
  bool success = false;
  word_t value = expr(args, &success);//L 可以计算表达式的值
  if (!success){
    printf("Token Match Failed, Please Check Input Expression\n");
    return 0;
  }
  printf("The Value of Expression is 0x%x(Hex)\t%u(Dec)\n", value, value);
  return 0;
}

/**
 * @brief 设置监视点 w expr 监视表达式expr的值
 * 
 * @param args 
 * @return int 
 */
static int cmd_w(char* args){
  add_watchpoint(args);
  return 0;
}

/**
 * @brief 删除监视点
 * 
 * @param args 
 * @return int 
 */
static int cmd_d(char* args){
  if (args == NULL){
    printf("no watchpoint Num is given!");
  }
  else{
    delete_watchpoint(atoi(args));
  }
  return 0;
}


static int cmd_q(char *args) {
  nemu_state.state = NEMU_QUIT;//L 添加这行代码 可以直接键入q 优雅地退出.
  return -1;
}

static int cmd_help(char *args);

static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display information about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "si", "Single-step N-instruction execution of the program", cmd_si },
  { "info", "Print program status(include r or watch point)", cmd_info },
  { "x", "Print Value at Memry Address" , cmd_x }, 
  { "p", "Calculate Expr Value" , cmd_p}, 
  { "q", "Exit NEMU", cmd_q },
  { "w", "Add watchpoint", cmd_w },
  { "d", "Delete watchpoint num", cmd_d },

  /* TODO: Add more commands */

};

#define NR_CMD ARRLEN(cmd_table)//L 命令数量的大小

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void sdb_set_batch_mode() {
  is_batch_mode = true;
}

void sdb_mainloop() {
  if (is_batch_mode) {//L 默认没有走里面. 批处理走里面
    cmd_c(NULL);
    return;
  }

  /*L 每次for循环 将调用一次rl_gets()函数，获得用户在命令行中敲入的字符串(回车键之前)；
  然后通过strtok函数提取第一个 空格 以前的字符串作为命令，其余字符串作为参数；然后查找注册的命令表，
  匹配；执行.
   */

  for (char *str; (str = rl_gets()) != NULL; ) {//L 这个地方处于一直读取命令行的状态.
    char *str_end = str + strlen(str);
    // int a = strlen(str);
    /* extract the first token as the command */
    char *cmd = strtok(str, " ");//L 以" "为分割符，返回切割下的字符串首地址，str也会发生改变，指向分割后的字符串首地址
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }//L 只有当调用cmd_q时，才会 return 到 engine_start 函数.
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}

void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();

  /* Initialize the watchpoint pool. */
  init_wp_pool();
}
