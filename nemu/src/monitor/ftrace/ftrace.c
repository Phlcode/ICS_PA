/**L
提供给监视器，用来解析ELF文件的函数 parse_elf
提供给译码器，用来记录函数调用和返回指令信息的函数
*/
#include <common.h>
#ifdef CONFIG_FTRACE

#include <device/map.h>
#include <fcntl.h>
#include <elf.h>
#include <unistd.h>

typedef struct SymbolEntry {
	char name[32];	// Locate at .strtab
	unsigned char info;//L Type和Bind
	paddr_t address;
	word_t size;
} SymbolEntry;

static SymbolEntry* sym_entrys = NULL;
static uint32_t sym_num = 0;
static uint32_t call_depth = 0;
static uint32_t trace_func_call_flag = 0;	// Flag to determine whether to trace function calls

void init_symtab_entrys(FILE *file);
char *get_strtab(Elf32_Shdr *strtab, FILE *file);
void parse_elf(const char *elf_file);
void print_sym_entrys();
char *get_function_name_by_addres(paddr_t addr);
void trace_func_call(paddr_t pc, paddr_t target);
void trace_func_ret(paddr_t pc);
void trace_dread(paddr_t addr, int len, IOMap *map);
void trace_dwrite(paddr_t addr, int len, word_t data, IOMap *map);



void parse_elf(const char *elf_file) {
	if (elf_file == NULL) {
		return;
	}
	
	Log("The elf file is %s\n", elf_file);
	trace_func_call_flag = 1;
	FILE *file = fopen(elf_file, "rb");
	assert(file != NULL);

	init_symtab_entrys(file);
	fclose(file);
	//print_sym_entrys();
}

char *get_function_name_by_addres(paddr_t addr) {
	for (int i = 0; i < sym_num; i++) {
		if (ELF32_ST_TYPE(sym_entrys[i].info) == STT_FUNC) {
			if (addr >= sym_entrys[i].address && addr < (sym_entrys[i].size + sym_entrys[i].address)) {//L [)
				return sym_entrys[i].name;
			}
		}
	}
	return NULL;
}

void init_symtab_entrys(FILE *elf_file) {//L elf_file 是已经打开了的elf文件流
    /**
     * 从已打开的 ELF 文件流中解析符号表（.symtab），将符号信息存入全局自定义结构数组 sym_entrys
     * 同时更新全局符号数量 sym_num
    */
	if (elf_file == NULL) assert(0);
	// Get ELF header
	Elf32_Ehdr ehdr;
    // L 从elf_file中读取1个类型为Elf32_Ehdr的完整结构体，存入ehdr;如果成功读入完整的 ELF header，返回 1；
	int result = fread(&ehdr, sizeof(Elf32_Ehdr), 1, elf_file);
	assert(result == 1);

	// Get Section header by ELF header
    //L ELF 文件里有 e_shnum 个节（section），每个节的描述信息都是固定大小的 Elf32_Shdr 结构体。这些描述信息在文件里是连续存放的“节头表”。
    //L 要把它们一次性读入内存，所以需要分配一块能放下所有节头的空间
	Elf32_Shdr *shdrs = malloc(sizeof(Elf32_Shdr) * ehdr.e_shnum);
    //L 将文件流的读写位置从文件开头（SEEK_SET）向后移动 ehdr.e_shoff 字节。 移动成功返回 0。
	result = fseek(elf_file, ehdr.e_shoff, SEEK_SET);//L 将文件流的读写位置指针移动到 ELF 文件的节头表（Section Header Table）的起始位置。
	assert(result == 0);
    //L 从当前位置（节头表起始处）读取 ehdr.e_shnum 个 Elf32_Shdr，存入 shdrs。
	result = fread(shdrs, sizeof(Elf32_Shdr), ehdr.e_shnum, elf_file);//L shdrs可以被看作一个Elf32_Shdr类型的数组
	// L返回 实际成功读取元素的个数
    assert(result == ehdr.e_shnum);

	// Get Symtab from Section headr entrys
	Elf32_Shdr *symtab = NULL;
	for (int i = 0; i < ehdr.e_shnum; i++) {//L 遍历所有节头，找出类型为 SHT_SYMTAB（符号表）的那个，让 symtab 指向它。
		if (shdrs[i].sh_type == SHT_SYMTAB) {
			symtab = shdrs + i;
 	  }
    }
	assert(symtab != NULL);//L 如果没有找到符号表，就报错

	// Get entry num and offset
	uint32_t entry_num = symtab->sh_size / symtab->sh_entsize;
	sym_num = entry_num;	// Set global entry num L 符号数量
	uint32_t offset = symtab->sh_offset;//L 该节(符号表)数据在文件中的起始偏移


	// Get symtab entrys
    //L 分配一块能容纳 entry_num 个 Elf32_Sym 结构体的连续内存，用来存放符号表节的原始数据。
	Elf32_Sym *symbol_tables = malloc(sizeof(Elf32_Sym) * entry_num);
    //L 移动到符号表数据的文件偏移处，读取所有的符号表项
	result = fseek(elf_file, offset, SEEK_SET);
	assert(result == 0);
    //L fread 的第二个参数是每个符号结构体的大小
	result = fread(symbol_tables, sizeof(Elf32_Sym), entry_num, elf_file);
	assert(result != 0);

	// Initialize sym_entrys
	sym_entrys = malloc(sizeof(SymbolEntry) * entry_num);
	char *str = get_strtab(&shdrs[ehdr.e_shnum - 2], elf_file);//L 获取字符串表.strtab的内容;通常 .strtab 是倒数第二个节
	assert(str != NULL);
	for (int i = 0; i < entry_num; i++) {
		strcpy(sym_entrys[i].name, str + symbol_tables[i].st_name);//L st_name是符号名在字符串表中的偏移量
		sym_entrys[i].info = symbol_tables[i].st_info;//L Type和Bind
		sym_entrys[i].address = (paddr_t) symbol_tables[i].st_value;
		sym_entrys[i].size = (word_t) symbol_tables[i].st_size;
	}

	// Free ELF headers, Symbol Entrys structure arrays and str
	free(shdrs);
	free(symbol_tables);
	free(str);//L free(str) 对应的是 get_strtab 内部 malloc 出来的整份字符串表内存
}


void print_sym_entrys() {
	assert(sym_entrys != NULL);
	for (int i = 0; i < sym_num; i++) {
		printf("Num:%2d, Name: %20s, Info: %d, Addr:%08x, size: %04x\n",
		i, sym_entrys[i].name, sym_entrys[i].info, sym_entrys[i].address, sym_entrys[i].size);
	}
}



void trace_func_call(paddr_t pc, paddr_t target) {
	if (trace_func_call_flag == 0) return; //No elf file
	++call_depth;//L 函数调用层数，用来控制可视化的格式化输出

	// if (call_depth <= 2) return; // ignore _trm_init & main
	
	char *name  = get_function_name_by_addres(target);
	// Example output: 0x800001f8:     call [f0@0x80000010]
	
	/** 输入ftrace到nemu-log中太乱了，直接输出到终端
	 * ftrace_write(FMT_PADDR ": %*scall [%s@" FMT_PADDR "]\n", 
		pc,
		(call_depth-1)*2, "", 
		name?name:"???",
		target
	);
	*/
	printf("0x%08x: ", pc);
	for (int i = 0; i < call_depth; i++) {
		printf(" ");
	}
	printf("call [%s@0x%x]\n", name?name:"???", target);
	
}

//TODO L 这个地方没有实现对尾调函数的处理 导致 部分函数的调用和返回部匹配e.g.只有call没有ret
void trace_func_ret(paddr_t pc) {
	if (trace_func_call_flag == 0) return; //No elf file

	// if (call_depth <= 2) return; // ignore _trm_init & main

	char *name = get_function_name_by_addres(pc);

	// ftrace_write(FMT_PADDR ": %*sret [%s]\n",
	// 	pc,
	// 	(call_depth-1)*2, "",
	// 	name?name:"???"
	// );
	printf("0x%08x: ", pc);
	for (int i = 0; i < call_depth; i++) {
		printf(" ");
	}
	printf("ret [%s@0x%x]\n", name?name:"???", pc);

	--call_depth;
}


char *get_strtab(Elf32_Shdr *strtab, FILE *file) {
	char *str = malloc(strtab->sh_size);

	int result = fseek(file, strtab->sh_offset, SEEK_SET);
	assert(result == 0);
    result = fread(str, 1, strtab->sh_size, file);
	assert(result != 0);

	return str;
}


// dtrace
void trace_dread(paddr_t addr, int len, IOMap *map) {
	dtrace_write("dtrace: read %10s at " FMT_PADDR ",%d\n",
	map->name, addr, len);
}

void trace_dwrite(paddr_t addr, int len, word_t data, IOMap *map) {
	dtrace_write("dtrace: write %10s at " FMT_PADDR ",%d with " FMT_WORD "\n",
	map->name, addr, len, data);
}

#endif
// #ifdef TEST
// int main() {
// 	char *elf_file = "/home/crx/study/ics2023/nemu/src/monitor/ftrace/string.elf";
// 	parse_elf(elf_file);
// 	return 0;
// }
// #endif