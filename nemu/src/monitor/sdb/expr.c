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

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>

uint32_t eval_expr(int p, int q, bool *success);//先声明下
word_t expr(char *e, bool *success);
static bool check_parentheses(int p, int q);
static int find_main_operator_index(int p, int q);
static int priority(int operator);
word_t vaddr_read(vaddr_t addr, int len);

enum {
  TK_NOTYPE = 256, TK_EQ,

  /* TODO: Add more token types */
  TK_NEQ,//不等于
  TK_HEX,//十六进制整数
  TK_UINT,//十进制整数
  TK_AND,//逻辑与
  TK_REG,//寄存器
  TK_DEREF,//解引用
  TK_NEG,//负号类型
};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},    // spaces
  {"\\+", '+'},         // plus 第一个\是C中的第二个\的转义符号，第二个\是正则表达式的转义符号
  {"==", TK_EQ},        // equal
  {"!=", TK_NEQ}, 
  {"\\-", '-'},//减号
  {"\\*", '*'},
  {"\\/", '/'},
  {"\\(", '('},
  {"\\)", ')'},
  {"0x[0-9A-Fa-f]+", TK_HEX},
  {"[0-9]+", TK_UINT},
  {"&&", TK_AND},
  {"\\$(\\$0|ra|sp|gp|tp|t[0-6]|s[0-9]|s10|s11|a[0-7])", TK_REG}//会匹配$ra $$0等
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);//L 编译正则表达式，把指定的正则表达式rules[i].regex编译成一种特定的数据格式&re[i]
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

static Token tokens[32] __attribute__((used)) = {};//L 用于按顺序存放已经识别出的token信息
static int nr_token __attribute__((used))  = 0;//L 指示已经被识别出来的token数目,即记录数组保存的token的个数

static bool make_token(char *e) {//L 识别待求值表达式的中的token,对传入的字符串进行词法分析
  int position = 0;
  int i;
  regmatch_t pmatch;//L 

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {//L 匹配目标文本串，执行成功返回0；
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        switch (rules[i].token_type) {
          case TK_NOTYPE: 
            break;
          case TK_HEX:
          case TK_UINT:
          case TK_REG://REG也需要保存字符串
            tokens[nr_token].type = rules[i].token_type;
            Assert(substr_len < 32, "Error!token should less than 32 characters!");//L 当子串的长度>=32时，程序会自动触发assert
            strncpy(tokens[nr_token].str, substr_start, substr_len);//strncpy不会自动加上\0,因此需要手动加上结束符号
            tokens[nr_token++].str[substr_len] = '\0';
            break;
          case '+':
            tokens[nr_token++].type = '+';
            break;
          case '-':
            tokens[nr_token].type = '-';
            if (tokens[nr_token].type == '-'){//这里需要根据前一个token的类型判断其是否为 负号
              int tk = (nr_token == 0 ? -1 : tokens[nr_token - 1].type);//获取前一个token的类型，如果是第一个 直接赋值-1
              if (nr_token == 0 || tk == '+' || tk == '-' 
                  || tk == '*' || tk == '/' || tk == '('
                  || tk == TK_EQ || tk == TK_NEQ || tk == TK_AND){
                    tokens[nr_token].type = TK_NEG;
              }
            }
            nr_token++;
            break;
          case '*':
            tokens[nr_token].type = '*';
            if (tokens[nr_token].type == '*'){//这里需要根据前一个token的类型判断其是否为 负号
              int tk = (nr_token == 0 ? -1 : tokens[nr_token - 1].type);//获取前一个token的类型，如果是第一个 直接赋值-1
              if (nr_token == 0 || tk == '+' || tk == '-' 
                  || tk == '*' || tk == '/' || tk == '('
                  || tk == TK_EQ || tk == TK_NEQ || tk == TK_AND){
                    tokens[nr_token].type = TK_DEREF;
              }
            }
            nr_token++;
            break;
          case '/':
            tokens[nr_token++].type = '/';
            break;
          case '(':
            tokens[nr_token++].type = '(';
            break;
          case ')':
            tokens[nr_token++].type = ')';
            break;
          case TK_EQ:
            tokens[nr_token++].type = TK_EQ;
            break;
          case TK_NEQ:
            tokens[nr_token++].type = TK_NEQ;
            break;
          case TK_AND:
            tokens[nr_token++].type = TK_AND;
            break;  
          default: 
            TODO();
            break;
        }

        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

word_t expr(char *e, bool *success) {//L 对0～nr_token范围内的tokens进行求值，最后返回表达式的值.
  *success = true;

  if (!make_token(e)) {
    *success = false;//L 如果没有token匹配成功，这会将success标志记录为false
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  /**L 实现表达式求值 分治法
   * 1. 找出主运算符 记录主运算符的下标r，
   * 2. 调用eval_expr(p, r - 1)和eval_expr(r + 1, q),根据主运算符的类型计算整个表达式的结果。 
   * */
  return eval_expr(0, nr_token - 1, success);

  // TODO();

  return 0;
}

uint32_t eval_expr(int p, int q, bool *success){
  if (p > q){
    *success = false;//表达式求值异常
    // Assert(0, "表达式求值异常!!!");
    printf("表达式求值异常!!!");
    return 0;
  }
  else if (p == q){
    //单个token求值 应该是一个数字
    uint32_t result = 0;
    *success = true;
    switch(tokens[p].type){
      case TK_UINT:
        sscanf(tokens[p].str, "%d", &result);
        return result;
      case TK_HEX:
        sscanf(tokens[p].str, "%x", &result);
        return result;
      case TK_REG:
        return isa_reg_str2val(tokens[p].str + 1, success);// +1的目的是排除输入字符串$ra中的$ 从而传入 ra，查询成功后 返回ra的值
      default:
        Assert(false, "error token type %d", tokens[p].type);
        // return false;
    }  
  }
  else if (check_parentheses(p, q) == true){//如果表达式被一括号包围，那么需要去掉括号 这个函数需要自己实现
    return eval_expr(p + 1, q - 1, success);//去掉外层括号后，递归调用eval_expr函数
  }
  else {
    //此时表达式可以分为多个子表达式，需要寻找主运算符，再将子表达式按照主运算符进行运算
    int OpIndex = find_main_operator_index(p, q); //这个函数也需要自己实现
    //由于负号和解引用符号只需要右侧的 先计算右侧的表达式
    word_t value_right = eval_expr(OpIndex + 1, q, success);
    if (tokens[OpIndex].type == TK_NEG){
      return -value_right;
    }
    if (tokens[OpIndex].type == TK_DEREF){//此处的value_right表示地址
      return vaddr_read(value_right, 4);
    }
    word_t value_left = eval_expr(p, OpIndex - 1, success);

    switch (tokens[OpIndex].type){
      case '+': return value_left + value_right;
      case '-': return value_left - value_right;
      case '*': return value_left * value_right;
      case '/': return value_left / value_right;
      case TK_EQ: return value_left == value_right;
      case TK_NEQ: return value_left != value_right;
      case TK_AND: return value_left && value_right;
      // default: break;
    }
  }
  return 0;
}

static bool check_parentheses(int p, int q){
  // 首先检查整个表达式是否被括号包围
  if (tokens[p].type != '(' || tokens[q].type != ')'){
    return false;
  }
  // 然后遍历表达式，检查括号是否闭合 
  int diff = 0;//用diff变量来表示 左括号与右括号数量的差值
  for (int i = p; i <= q; i++){
    if (tokens[i].type == '('){
      diff++;
    }
    if (tokens[i].type == ')'){
      diff--;
    }
    if (diff < 0){
      // Assert(0, "表达式的右括号数量大于左括号数量!!!");
      printf("表达式的右括号数量大于左括号数量!!!");
      return false;//说明表达式的右括号已经多于左括号了，不论遍历到表达式的哪个位置.
    }
  }
  if (diff != 0){//当遍历完表达式时，diff不为0，说明左右括号不匹配
    // Assert(0, "表达式的左右括号不匹配!!!");
    printf("表达式的左/’右括号不匹配!!!");
    return false;
  }
  return true;
}

static int find_main_operator_index(int p, int q){
  int MainOperatorIndex = -1;//L 主运算符号在tokens数组的索引
  int MainOperator = -1;//L 保存当前找到的主运算符
  int LeftNum = 0;
  for (int i = p; i <= q; i++){
    int Operator = tokens[i].type;
    switch(Operator){
      case '(':
        LeftNum++;
        break;
      case ')':
        LeftNum--;
        break;
      case '+':
      case '-':
      case '*':
      case '/':
      case TK_AND:
      case TK_EQ:
      case TK_NEQ:
      case TK_DEREF:
      case TK_NEG:
        if (LeftNum == 0 && //排除了主运算符在括号内的情况;
                            (MainOperatorIndex == -1 || priority(Operator) > priority(MainOperator))){
            MainOperatorIndex = i;
            MainOperator = Operator;
           }
        break;
      default:
        break;
    }
  }
  return MainOperatorIndex;
}

static int priority(int operator){
  //优先级越高，数值越小
  switch (operator){
    case TK_AND:
      return 4;
    case TK_EQ:
    case TK_NEQ:
      return 3;
    case '+':
    case '-':
      return 2;
    case '*':
    case '/':
      return 1;
    case TK_DEREF:
    case TK_NEG:
      return 0;
    default:
      Assert(0,"Find No Main Operator!!!");
  }
}
