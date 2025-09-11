#include "monitor/watchpoint.h"
#include "monitor/expr.h"
#include "nemu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define NR_WP 32

static WP wp_pool[NR_WP];
static WP *head, *free_;

void init_wp_pool() {
    int i;
    for(i = 0; i < NR_WP; i ++) {
        wp_pool[i].NO = i;
        wp_pool[i].next = &wp_pool[i + 1];
        memset(wp_pool[i].expr, 0, sizeof(wp_pool[i].expr));
        wp_pool[i].old_value = 0;
        wp_pool[i].enabled = false;
    }
    wp_pool[NR_WP - 1].next = NULL;

    head = NULL;
    free_ = wp_pool;
}

WP* new_wp() {
    if (free_ == NULL) {
        printf("Error: 糟糕，满溢了!\n");
        assert(0);
    }
    
    WP *wp = free_;
    free_ = free_->next;
    
    wp->next = head;
    head = wp;
    wp->enabled = true;
    
    return wp;
}

void free_wp(WP *wp) {
    assert(wp != NULL);
    
    if (head == wp) {
        head = head->next;
    } else {
        WP *prev = head;
        while (prev != NULL && prev->next != wp) {
            prev = prev->next;
        }
        assert(prev != NULL);
        prev->next = wp->next;
    }
    
    wp->enabled = false;
    memset(wp->expr, 0, sizeof(wp->expr));
    wp->old_value = 0;
    
    wp->next = free_;
    free_ = wp;
}

int set_watchpoint(char *expr_str) {
    bool success;
    uint32_t value = expr(expr_str, &success);
    
    if (!success) {
        printf("Error: 这一定是出错了 '%s'\n", expr_str);
        return -1;
    }
    
    WP *wp = new_wp();
    strncpy(wp->expr, expr_str, sizeof(wp->expr) - 1);
    wp->expr[sizeof(wp->expr) - 1] = '\0';  
    wp->old_value = value;
    
    printf("Watchpoint %d: %s\n", wp->NO, wp->expr);
    return wp->NO;
}

bool delete_watchpoint(int no) {
    WP *wp = head;
    while (wp != NULL) {
        if (wp->NO == no) {
            printf("删除 watchpoint %d: %s\n", wp->NO, wp->expr);
            free_wp(wp);
            return true;
        }
        wp = wp->next;
    }
    printf("Error: Watchpoint %d 找不到嘞\n", no);
    return false;
}

int check_watchpoints() {
    WP *wp = head;
    while (wp != NULL) {
        if (wp->enabled) {
            bool success;
            uint32_t new_value = expr(wp->expr, &success);
            
            if (success && new_value != wp->old_value) {
                extern CPU_state cpu;
                printf("Hit watchpoint %d at address 0x%08x\n", wp->NO, cpu.eip);
                printf("Old value = %u\n", wp->old_value);
                printf("New value = %u\n", new_value);
                
                wp->old_value = new_value;
                return wp->NO;
            }
        }
        wp = wp->next;
    }
    return -1;
}

void info_watchpoints() {
    if (head == NULL) {
        printf("什么都没有哦\n");
        return;
    }
    
    printf("Num     Type           What\n");
    WP *wp = head;
    while (wp != NULL) {
        printf("%-8d watchpoint     %s\n", wp->NO, wp->expr);
        wp = wp->next;
    }
}



