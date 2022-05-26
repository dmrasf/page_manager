#include "page_base.h"
#include "page_manager.h"
#include "src/core/lv_disp.h"
#include "src/core/lv_obj.h"
#include "src/core/lv_obj_tree.h"
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
        // 动画完成后运行 page_state_run(page);
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
        // 动画完成后运行 page_state_run(page);
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

// 创建lv_obj
static page_state do_load(page_base *page)
{
    if (page->desc->on_will_load != NULL)
        page->desc->on_will_load(NULL);
    page->desc->create_page(page);
    if (page->desc->on_loaded != NULL)
        page->desc->on_loaded(page->lv_root);
    return PAGE_STATE_WILL_APPEAR;
}

// 清除hidden flag
static page_state do_will_appear(page_base *page)
{
    if (page->desc->on_will_appear != NULL)
        page->desc->on_will_appear(page->lv_root);
    lv_obj_clear_flag(page->lv_root, LV_OBJ_FLAG_HIDDEN);
    // 初始化动画属性
    if (page->is_push)
        page_set_appear_anim(page, &page->desc->anim_desc.page_push_in);
    else
        page_set_appear_anim(page, &page->desc->anim_desc.page_pop_in);
    page->is_anim_busy = true;
    return PAGE_STATE_DID_APPEAR;
}

// 已经显示在屏幕上
static page_state do_did_appear(page_base *page)
{
    if (page->desc->on_appeared != NULL)
        page->desc->on_appeared(page->lv_root);
    page->is_anim_busy = false;

    // 新栈的页面动画结束之后再运行状态
    if (page->is_push)
        if (page->node->next != NULL)
            if (page->node->next->base.desc->anim_desc.page_push_out.anim_type == PAGE_ANIM_NONE)
                page_state_run(&page->node->next->base);
    return PAGE_STATE_ACTIVITY;
}

// 做屏幕消失前的工作
static page_state do_will_disappear(page_base *page)
{
    if (page->desc->on_will_disappear != NULL)
        page->desc->on_will_disappear(page->lv_root);
    // 初始化动画属性
    if (page->is_push)
        page_set_disappear_anim(page, &page->desc->anim_desc.page_push_out);
    else
        page_set_disappear_anim(page, &page->desc->anim_desc.page_pop_out);
    page->is_anim_busy = true;
    return PAGE_STATE_DID_DISAPPEAR;
}

// 设置hidden flag
static page_state do_did_disappear(page_base *page)
{
    lv_obj_add_flag(page->lv_root, LV_OBJ_FLAG_HIDDEN);
    if (page->desc->on_disappeared != NULL)
        page->desc->on_disappeared(page->lv_root);
    page->is_anim_busy = false;
    if (page->is_push == true)
        return PAGE_STATE_WILL_APPEAR;
    else
        return PAGE_STATE_UNLOAD;
}

// 删除lv_obj
static page_state do_unload(page_base *page)
{
    if (page->desc->on_will_unload != NULL)
        page->desc->on_will_unload(page->lv_root);
    lv_obj_del(page->lv_root);
    if (page->desc->on_unloaded != NULL)
        page->desc->on_unloaded(NULL);
    free(page->node);
    return PAGE_STATE_IDLE;
}
