#include "nemu.h"
#include <stdlib.h>
/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>

enum {
	NOTYPE = 256, EQ, NEQ, REG, HEX, NUM, OR, AND

	/* TODO: Add more token types */

};

static struct rule {
	char *regex;
	int token_type;
} rules[] = {

	/* TODO: Add more rules.
	 * Pay attention to the precedence level of different rules.
	 */

	{" +",	NOTYPE},				// spaces
	{"\\+", '+'},					// plus
	{"==", EQ},						// equal

	{"\\-", '-'},  // minus
	{"-[0-9]+", NUM},// negtive numbers
  	{"\\*", '*'},  // multiply
	{"/", '/'},    // divide
  	{"\\(", '('},
  	{"\\)", ')'},
  	{"!=",NEQ},        //not equal
	{"!", '!'},
  	{"\\$(e[abcd]x|e[sd]i|ebp|eip|esp|[abcd]h|[abcd]l|\\$0|ra|[sgt]p|t[0-6]|a[0-7]|s([0-9]|1[0-1]))", REG},		//registers
  	{"0[xX][0-9a-fA-F]+",HEX},    //hex numbers
  	{"[0-9]+", NUM},   //numbers
  	{"\\|\\|",OR},
  	{"&&",AND}
};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]) )

static regex_t re[NR_REGEX];

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
	int i;
	char error_msg[128];
	int ret;

	for(i = 0; i < NR_REGEX; i ++) {
		ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
		if(ret != 0) {
			regerror(ret, &re[i], error_msg, 128);
			Assert(ret == 0, "regex compilation failed: %s\n%s", error_msg, rules[i].regex);
		}
	}
}

typedef struct token {
	int type;
	char str[32];
} Token;

Token tokens[32];
int nr_token;

bool make_token(char *e) {
	int position = 0;
	int i;
	regmatch_t pmatch;
	
	nr_token = 0;

	while(e[position] != '\0') {
		/* Try all rules one by one. */
		for(i = 0; i < NR_REGEX; i ++) {
			if(regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
				char *substr_start = e + position;
				int substr_len = pmatch.rm_eo;

				Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s", i, rules[i].regex, position, substr_len, substr_len, substr_start);
				position += substr_len;

				/* Record the token in the array `tokens'. */
				switch(rules[i].token_type) {
					case NOTYPE:
						break;
					case '!':
					case '+':
					case '-':
					case '*':
					case '/':
					case '(':
					case ')':
					case EQ:
					case NEQ:
					case OR:
					case AND:
   						tokens[nr_token].type = rules[i].token_type;
    					tokens[nr_token].str[0] = substr_start[0];  
    					tokens[nr_token].str[1] = '\0';
    					nr_token++;
    					break;
					case REG:
					case HEX:
					case NUM:
						tokens[nr_token].type = rules[i].token_type;
						strncpy(tokens[nr_token].str, substr_start, substr_len);
						tokens[nr_token].str[substr_len] = '\0';
						nr_token++;
						break;
					default:
						panic("O!M!G! Something went wrong!");
				}

				break;
			}
		}

		if(i == NR_REGEX) {
			printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
			return false;
		}
	}

	return true; 
}


bool check_parentheses(int p, int q) {
	if (tokens[p].type != '(' || tokens[q].type != ')') {
		return false;
	}
	int balance = 0;
	int i;
	for (i = p; i <= q; i++) {
		if (tokens[i].type == '(') {
			balance++;
		} else if (tokens[i].type == ')') {
			balance--;
		}
		if (balance < 0) {
			return false;
		}
	}
	return balance == 0;
}

int find_dominant_operator(int p, int q) {
    int min_priority = 999;
    int dominant_op = -1;
    int i;
    int paren_level = 0;  
    
    for (i = q; i >= p; i--) {
        // 括号层级
        if (tokens[i].type == ')') {
            paren_level++;
        } else if (tokens[i].type == '(') {
            paren_level--;
        }
        
        // 括号外的运算符
        if (paren_level == 0) {
            int current_priority = -1;
            
            switch (tokens[i].type) {
                case OR:
                    current_priority = 0;  // 逻辑或
                    break;
                case AND:
                    current_priority = 1;  // 逻辑与
                    break;
                case EQ:
                case NEQ:
                    current_priority = 2;  // 比较运算符
                    break;
                case '+':
                case '-':
                    current_priority = 3;  // 加减
                    break;
                case '*':
                case '/':
                    current_priority = 4;  // 乘除
                    break;
				case '!':
        			current_priority = 5;  // 逻辑非
        			break;
                default:
                    continue;  
            }
            if (current_priority <= min_priority) {
                min_priority = current_priority;
                dominant_op = i;
            }
        }
    }
    
    return dominant_op;
}

int32_t eval(int p, int q) {
	if (p > q) {
		panic("eval: p > q");
	}
	else if (p == q) {
    if (tokens[p].type == NUM) {
        return atoi(tokens[p].str);
    } else if (tokens[p].type == HEX) {
        return strtoul(tokens[p].str, NULL, 16); 
    } else if (tokens[p].type == REG) {

		char *reg_name = tokens[p].str + 1;
    
    	if (strcmp(reg_name, "eax") == 0) return cpu.eax;
    	else if (strcmp(reg_name, "ecx") == 0) return cpu.ecx;
    	else if (strcmp(reg_name, "edx") == 0) return cpu.edx;
    	else if (strcmp(reg_name, "ebx") == 0) return cpu.ebx;
    	else if (strcmp(reg_name, "esp") == 0) return cpu.esp;
    	else if (strcmp(reg_name, "ebp") == 0) return cpu.ebp;
    	else if (strcmp(reg_name, "esi") == 0) return cpu.esi;
    	else if (strcmp(reg_name, "edi") == 0) return cpu.edi;
		else if (strcmp(reg_name, "eip") == 0) return cpu.eip;
		else {
        printf("Warning: Register %s 这啥也没有啊\n", tokens[p].str);
        return 0;
		}
    } else {
        panic("eval: 这不是数嘞");
    }
}
	else if (check_parentheses(p, q)) {
		return eval(p + 1, q - 1);
	} else if (tokens[p].type == '!' && p < q) {
		// 逻辑非
		uint32_t operand = eval(p + 1, q);
		return !operand;
	}
	else {
		int dominant_op = find_dominant_operator(p, q);
		if (dominant_op == -1) {
			panic("eval: 俺没找到嘞");
		}

		uint32_t left = eval(p, dominant_op - 1);
		uint32_t right = eval(dominant_op + 1, q);

		switch (tokens[dominant_op].type) {
    		case '+': return left + right;
    		case '-': return left - right;
    		case '*': return left * right;
    		case '/': return left / right;
    		case EQ: return left == right;
    		case NEQ: return left != right;
    		case OR: return left || right;
   			case AND: return left && right;
    		default: panic("eval: 俺不认得嘞");
		}
	}
}

uint32_t expr(char *e, bool *success) {
	if(!make_token(e)) {
		*success = false;
		return 0;
	}
	*success = true;
	return eval(0,nr_token-1);
}

int cmd_p_run(char *args) {
    if (args == NULL || args[0] == '\0') {
        printf("啥也不输啥意思\n");
        return 0;
    }

    bool success;
    uint32_t result = expr(args, &success);

    if (success) {
        printf("Result: %d\n", result);
    } else {
        printf("Someting went wrong!\n");
    }

    return 0;
}