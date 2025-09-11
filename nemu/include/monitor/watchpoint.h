#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#include "common.h"

typedef struct watchpoint {
    int NO;                    
    struct watchpoint *next;   
    char expr[256];           
    uint32_t old_value;      
    bool enabled;             
} WP;

WP* new_wp();
void free_wp(WP *wp);
int set_watchpoint(char *expr_str);
bool delete_watchpoint(int no);
int check_watchpoints();
void info_watchpoints();

#endif
