#ifndef __PAGE_MANAGER_H__
#define __PAGE_MANAGER_H__

#include "page_base.h"
#include "src/core/lv_obj.h"
#include "src/misc/lv_anim.h"

typedef struct page_desc_node_t {
    page_desc *desc;
    struct page_desc_node_t *next;
} page_desc_node;

typedef struct page_base_node_t {
    page_base base;
    struct page_base_node_t *next;
} page_base_node;

typedef struct page_manager_t {
    page_desc_node *page_all;
    page_base_node *page_stack;
} page_manager;

void page_manager_init(void);

// 注册函数
bool page_register(page_desc *);
bool page_unregister(page_desc *);

// route函数
page_base *page_push(page_desc *);
page_base *page_pop(void);

// 生命周期函数
void page_state_run(page_base *);

// 页面切换动画函数
void page_anim_init(void);
void page_set_appear_anim(page_base *, page_anim_attr *);
void page_set_disappear_anim(page_base *, page_anim_attr *);
void page_anim_appear_start(void);
void page_anim_disappear_start(void);

#endif
