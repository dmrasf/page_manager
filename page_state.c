#include "page_base.h"
#include "page_manager.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "page_log.h"

static page_state do_load(page_base *);
static page_state do_will_appear(page_base *);
static page_state do_did_appear(page_base *);
static page_state do_will_disappear(page_base *);
static page_state do_did_disappear(page_base *);
static page_state do_unload(page_base *);

void page_state_run(page_base *page)
{
    if (page == NULL)
        return;

    switch (page->state) {
    case PAGE_STATE_IDLE:
        break;
    case PAGE_STATE_LOAD:
        page->state = do_load(page);
        page_state_run(page);
        break;
    case PAGE_STATE_WILL_APPEAR:
        page->state = do_will_appear(page);
        page_state_run(page);
        break;
    case PAGE_STATE_DID_APPEAR:
        page->state = do_did_appear(page);
        break;
    case PAGE_STATE_ACTIVITY:
        page->state = PAGE_STATE_WILL_DISAPPEAR;
        page_state_run(page);
        break;
    case PAGE_STATE_WILL_DISAPPEAR:
        page->state = do_will_disappear(page);
        page_state_run(page);
        break;
    case PAGE_STATE_DID_DISAPPEAR:
        page->state = do_did_disappear(page);
        if (page->state == PAGE_STATE_UNLOAD)
            page_state_run(page);
        break;
    case PAGE_STATE_UNLOAD:
        page->state = do_unload(page);
        break;
    }
}

// creat root lv_obj
static page_state do_load(page_base *page)
{
    p_log("page %s: will load", page->desc->page_name);
    if (page->desc->on_will_load != NULL)
        page->desc->on_will_load(NULL);
    // remove scrollbar
    lv_obj_set_scrollbar_mode(lv_scr_act(), LV_SCROLLBAR_MODE_OFF);

    page->lv_root = lv_obj_create(lv_scr_act());
    page->desc->group = lv_group_create();

    lv_obj_set_scrollbar_mode(page->lv_root, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_size(page->lv_root, LCD_W, LCD_H);

    page->desc->create_page(page->lv_root, page->desc);
    p_log("page %s: loaded", page->desc->page_name);
    if (page->desc->on_loaded != NULL)
        page->desc->on_loaded(page->lv_root);
    return PAGE_STATE_WILL_APPEAR;
}

// clear hidden flag
static page_state do_will_appear(page_base *page)
{
    uint32_t i = 0;

    p_log("page %s: will appear", page->desc->page_name);
    if (page->desc->on_will_appear != NULL)
        page->desc->on_will_appear(page->lv_root);

    lv_obj_clear_flag(page->lv_root, LV_OBJ_FLAG_HIDDEN);

    // indev binding
    if (page->desc->group != NULL && page->desc->indev != NULL)
        lv_indev_set_group(page->desc->indev, page->desc->group);

    return PAGE_STATE_DID_APPEAR;
}

// page is show in screen
static page_state do_did_appear(page_base *page)
{
    p_log("page %s: appeared", page->desc->page_name);
    if (page->desc->on_appeared != NULL)
        page->desc->on_appeared(page->lv_root);

    if (page->is_push)
        if (page->node->next != NULL)
            page_state_run(&page->node->next->base);
    return PAGE_STATE_ACTIVITY;
}

// page will disappear
static page_state do_will_disappear(page_base *page)
{
    p_log("page %s: will disappear", page->desc->page_name);
    if (page->desc->on_will_disappear != NULL)
        page->desc->on_will_disappear(page->lv_root);

    return PAGE_STATE_DID_DISAPPEAR;
}

// add hidden flag
static page_state do_did_disappear(page_base *page)
{
    lv_obj_add_flag(page->lv_root, LV_OBJ_FLAG_HIDDEN);

    p_log("page %s: disappeared", page->desc->page_name);
    if (page->desc->on_disappeared != NULL)
        page->desc->on_disappeared(page->lv_root);
    if (page->is_push == true)
        return PAGE_STATE_WILL_APPEAR;
    else
        return PAGE_STATE_UNLOAD;
}

// free styles mem
static void free_page_styles(page_desc *page)
{
    if (page == NULL)
        return;

    page_desc_style_node *node = page->style_node;
    page_desc_style_node *tmp;

    while (node != NULL) {
        if (node->style != NULL)
            lv_style_reset(node->style);
        tmp = node;
        node = node->next;
        free(tmp);
    }
    page->style_node = NULL;
}

// del lv_obj
static page_state do_unload(page_base *page)
{
    p_log("page %s: will unload", page->desc->page_name);
    if (page->desc->on_will_unload != NULL)
        page->desc->on_will_unload(page->lv_root);

    free_page_styles(page->desc);
    p_log("page %s: reset all styles", page->desc->page_name);

    lv_obj_del(page->lv_root);

    if (page->desc->group != NULL)
        lv_group_del(page->desc->group);

    if (page->desc->ui_timer != NULL)
        lv_timer_del(page->desc->ui_timer);

    p_log("page %s: unloaded", page->desc->page_name);
    if (page->desc->on_unloaded != NULL)
        page->desc->on_unloaded(NULL);
    // free node in page stack
    free(page->node);
    return PAGE_STATE_IDLE;
}
