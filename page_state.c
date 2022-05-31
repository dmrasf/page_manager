#include "page_base.h"
#include "page_manager.h"
#include "src/core/lv_disp.h"
#include "src/core/lv_obj.h"
#include "src/core/lv_obj_tree.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "page_log.h"
#include "src/misc/lv_mem.h"
#include "src/misc/lv_style.h"

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
        page_anim_appear_start();
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
        page_anim_disappear_start();
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
    page->lv_root = lv_obj_create(lv_scr_act());
    page->desc->create_page(page->lv_root);
    p_log("page %s: loaded", page->desc->page_name);
    if (page->desc->on_loaded != NULL)
        page->desc->on_loaded(page->lv_root);
    return PAGE_STATE_WILL_APPEAR;
}

// clear hidden flag
static page_state do_will_appear(page_base *page)
{
    p_log("page %s: will appear", page->desc->page_name);
    if (page->desc->on_will_appear != NULL)
        page->desc->on_will_appear(page->lv_root);
    lv_obj_clear_flag(page->lv_root, LV_OBJ_FLAG_HIDDEN);

    // animation init
    if (page->is_push)
        page_set_appear_anim(page, &page->desc->anim_desc.page_push_in);
    else
        page_set_appear_anim(page, &page->desc->anim_desc.page_pop_in);
    page->is_anim_busy = true;
    return PAGE_STATE_DID_APPEAR;
}

// page is show in screen
static page_state do_did_appear(page_base *page)
{
    p_log("page %s: appeared", page->desc->page_name);
    page->is_anim_busy = false;
    if (page->desc->on_appeared != NULL)
        page->desc->on_appeared(page->lv_root);

    if (page->is_push)
        if (page->node->next != NULL)
            // 旧页面没有动画，所以需要等新栈的页面动画结束之后再运行状态
            if (page->node->next->base.desc->anim_desc.page_push_out.anim_type == PAGE_ANIM_NONE)
                page_state_run(&page->node->next->base);
    return PAGE_STATE_ACTIVITY;
}

// page will disappear
static page_state do_will_disappear(page_base *page)
{
    p_log("page %s: will disappear", page->desc->page_name);
    if (page->desc->on_will_disappear != NULL)
        page->desc->on_will_disappear(page->lv_root);

    // animation init
    if (page->is_push)
        page_set_disappear_anim(page, &page->desc->anim_desc.page_push_out);
    else
        page_set_disappear_anim(page, &page->desc->anim_desc.page_pop_out);
    page->is_anim_busy = true;
    return PAGE_STATE_DID_DISAPPEAR;
}

// add hidden flag
static page_state do_did_disappear(page_base *page)
{
    lv_obj_add_flag(page->lv_root, LV_OBJ_FLAG_HIDDEN);
    p_log("page %s: disappeared", page->desc->page_name);
    if (page->desc->on_disappeared != NULL)
        page->desc->on_disappeared(page->lv_root);
    page->is_anim_busy = false;
    if (page->is_push == true)
        return PAGE_STATE_WILL_APPEAR;
    else
        return PAGE_STATE_UNLOAD;
}

// free styles mem
static void free_page_styles(const lv_obj_t *obj)
{
    if (obj == NULL)
        return;
    for (int i = 0; i < obj->style_cnt; i++) {
        // use user_data save style pointer
        if (obj->user_data != 0 && obj->styles[i].style == obj->user_data) {
            lv_style_reset(obj->styles[i].style);
            free(obj->styles[i].style);
        }
    }
    int child_cnt = lv_obj_get_child_cnt(obj);
    for (int i = 0; i < child_cnt; i++)
        free_page_styles(lv_obj_get_child(obj, i));
}

// del lv_obj
static page_state do_unload(page_base *page)
{
    p_log("page %s: will unload", page->desc->page_name);
    if (page->desc->on_will_unload != NULL)
        page->desc->on_will_unload(page->lv_root);
    // need to reset all style int root&root's children
    free_page_styles(page->lv_root);
    lv_obj_del(page->lv_root);
    p_log("page %s: unloaded", page->desc->page_name);
    if (page->desc->on_unloaded != NULL)
        page->desc->on_unloaded(NULL);
    // free node in page stack
    free(page->node);
    return PAGE_STATE_IDLE;
}
