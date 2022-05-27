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

bool page_manager_init(void);
bool page_desc_init(page_desc *page, create_page_t cb, const char *name);
bool page_uninstall(page_desc *);

// route function
page_base *page_push(page_desc *);
page_base *page_pop(void);

// state function
void page_state_run(page_base *);

// page animation function
void page_anim_init(void);
void page_set_appear_anim(page_base *, page_anim_attr *);
void page_set_disappear_anim(page_base *, page_anim_attr *);
void page_anim_appear_start(void);
void page_anim_disappear_start(void);

#endif /* __PAGE_MANAGER_H__ */
