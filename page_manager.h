#ifndef __PAGE_MANAGER_H__
#define __PAGE_MANAGER_H__

#include "page_base.h"

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

typedef struct page_desc_style_node_t {
    lv_style_t *style;
    struct page_desc_style_node_t *next;
} page_desc_style_node;

bool page_manager_init(void);
bool page_desc_init(page_desc *page, create_page_t cb, const char *name, lv_indev_t *indev);
void page_desc_add_style(page_desc *page, lv_style_t *style);

page_base *page_push(page_desc *);
page_base *page_pop(void);

#endif /* __PAGE_MANAGER_H__ */
