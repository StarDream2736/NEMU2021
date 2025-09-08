#include "monitor/monitor.h"
#include "monitor/expr.h"
#include "monitor/watchpoint.h"
#include "nemu.h"

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

void cpu_exec(uint32_t);

/* We use the `readline' library to provide more flexibility to read from stdin. */
char* rl_gets() {
	static char *line_read = NULL;

	if (line_read) {
		free(line_read);
		line_read = NULL;
	}

	line_read = readline("(nemu) ");

	if (line_read && *line_read) {
		add_history(line_read);
	}

	return line_read;
}

static int cmd_c(char *args) {	//继续运行被暂停的程序
	cpu_exec(-1);
	return 0;
}

static int cmd_q(char *args) {	//退出NEMU
	return -1;
}

static int cmd_si(char *args) {	//逐步执行程序
    char *arg = strtok(args, " ");
    int num = 1;
    if (arg != NULL) {
        num = atoi(arg);
    }
    cpu_exec(num);
    return 0;
}

static int cmd_info(char *args) {	//打印寄存器状态
    char *op = strtok(args, " ");
    if (op == NULL) {
        return 0;
    }

    if (strcmp(op, "r") == 0) {
    extern CPU_state cpu;
    printf("eax\t0x%08x\t%-10u\n", cpu.eax, cpu.eax);
    printf("ecx\t0x%08x\t%-10u\n", cpu.ecx, cpu.ecx);
    printf("edx\t0x%08x\t%-10u\n", cpu.edx, cpu.edx);
    printf("ebx\t0x%08x\t%-10u\n", cpu.ebx, cpu.ebx);
    printf("esp\t0x%08x\t%-10u\n", cpu.esp, cpu.esp);
    printf("ebp\t0x%08x\t%-10u\n", cpu.ebp, cpu.ebp);
    printf("esi\t0x%08x\t%-10u\n", cpu.esi, cpu.esi);
    printf("edi\t0x%08x\t%-10u\n", cpu.edi, cpu.edi);
	printf("eip\t0x%08x\t%-10u\n", cpu.eip, cpu.eip);
	} else {
        printf("Unknown info option: %s\n", op);
    }
    return 0;
}

static int cmd_x(char *args) {	//扫描内存
  char* s_num1 = strtok(args, " ");
  if (s_num1 == NULL) {
    return 0;
  }
  int num1 = atoi(s_num1);
  char* s_num2 = strtok(args, " ");
  if (s_num2 == NULL) {
    return 0;
  }

  uint32_t addr = (uint32_t)strtol(s_num2+2, NULL, 16);

  printf("%s:\t", s_num2);
  int i,j;
  for (i = 0; i < num1 << 2; i++) { 
    printf("0x%02x ", swaddr_read(addr + i, 1)); 
    if ((i + 1) % 4 == 0) {
      printf("\t");
      for (j = i - 3; j <= i; j++) {
        printf("%-4d ", swaddr_read(addr + j, 1));
      }
      printf("\n");
      if (i + 1 == num1 << 2) {
        printf("\n");
      } else {
        printf("0x%x:\t", addr + i + 1);
      }
    }
  }
  return 0;  
}

static int cmd_help(char *args);

static struct {
	char *name;
	char *description;
	int (*handler) (char *);
} cmd_table [] = {
	{ "help", "Display informations about all supported commands", cmd_help },
	{ "c", "Continue the execution of the program", cmd_c },
	{ "q", "Exit NEMU", cmd_q },
	{ "si", "逐步执行", cmd_si },
	{ "info", "打印程序状态", cmd_info },
	{ "x", "扫描内存", cmd_x},

	/* TODO: Add more commands */

};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

static int cmd_help(char *args) {
	/* extract the first argument */
	char *arg = strtok(NULL, " ");
	int i;

	if(arg == NULL) {
		/* no argument given */
		for(i = 0; i < NR_CMD; i ++) {
			printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
		}
	}
	else {
		for(i = 0; i < NR_CMD; i ++) {
			if(strcmp(arg, cmd_table[i].name) == 0) {
				printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
				return 0;
			}
		}
		printf("Unknown command '%s'\n", arg);
	}
	return 0;
}

void ui_mainloop() {
	while(1) {
		char *str = rl_gets();
		char *str_end = str + strlen(str);

		/* extract the first token as the command */
		char *cmd = strtok(str, " ");
		if(cmd == NULL) { continue; }

		/* treat the remaining string as the arguments,
		 * which may need further parsing
		 */
		char *args = cmd + strlen(cmd) + 1;
		if(args >= str_end) {
			args = NULL;
		}

#ifdef HAS_DEVICE
		extern void sdl_clear_event_queue(void);
		sdl_clear_event_queue();
#endif

		int i;
		for(i = 0; i < NR_CMD; i ++) {
			if(strcmp(cmd, cmd_table[i].name) == 0) {
				if(cmd_table[i].handler(args) < 0) { return; }
				break;
			}
		}

		if(i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
	}
}
