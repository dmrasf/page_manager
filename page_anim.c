#include "page_base.h"
#include "page_manager.h"
#include "page_log.h"
#include "src/core/lv_disp.h"
#include "src/core/lv_obj.h"
#include "src/core/lv_obj_pos.h"
#include "src/core/lv_obj_tree.h"
#include <stdio.h>
#include "src/misc/lv_anim.h"
#include "src/misc/lv_color.h"

// 页面消失
static lv_anim_t page_appear_anim;
// 页面显示
static lv_anim_t page_disappear_anim;

static void anim_set_path(lv_anim_t *a, page_anim_curve path);
static void anim_set_type(lv_anim_t *a, page_anim_type type, bool is_appear);

static void page_anim_move_y_callback(struct _lv_anim_t *a, int32_t v)
{
    lv_obj_set_pos(((page_base *)a->user_data)->lv_root, 0, v);
    if (a->end_value == v) {
        lv_obj_set_pos(((page_base *)a->user_data)->lv_root, 0, 0);
        page_state_run(a->user_data);
    }
}

static void page_anim_move_x_callback(struct _lv_anim_t *a, int32_t v)
{
    lv_obj_set_pos(((page_base *)a->user_data)->lv_root, v, 0);
    if (a->end_value == v) {
        lv_obj_set_pos(((page_base *)a->user_data)->lv_root, 0, 0);
        page_state_run(a->user_data);
    }
}

static void page_anim_fade_callback(struct _lv_anim_t *a, int32_t v)
{
    lv_obj_set_style_opa(((page_base *)a->user_data)->lv_root, v, 0);
    if (a->end_value == v) {
        lv_obj_set_style_opa(((page_base *)a->user_data)->lv_root, LV_OPA_MAX, 0);
        page_state_run(a->user_data);
    }
}

static void page_anim_none_callback(struct _lv_anim_t *a, int32_t v)
{
    if (a->end_value == v)
        page_state_run(a->user_data);
}

void page_anim_appear_start(void)
{
    lv_anim_start(&page_appear_anim);
}

void page_anim_disappear_start(void)
{
    lv_anim_start(&page_disappear_anim);
}

void page_anim_init(void)
{
    lv_anim_init(&page_appear_anim);
    lv_anim_init(&page_disappear_anim);
}

// 设置页面显示时动画，包括入栈页面和出栈后露出的页面
void page_set_appear_anim(page_base *page, page_anim_attr *attr)
{
    lv_anim_set_var(&page_appear_anim, page->lv_root);
    lv_anim_set_time(&page_appear_anim, attr->duration);
    anim_set_path(&page_appear_anim, attr->anim_curve);
    page_appear_anim.user_data = page;
    anim_set_type(&page_appear_anim, attr->anim_type, true);
}

// 设置页面消失时动画，包括入栈时被覆盖的页面和出栈的页面
void page_set_disappear_anim(page_base *page, page_anim_attr *attr)
{
    lv_anim_set_var(&page_disappear_anim, page->lv_root);
    lv_anim_set_time(&page_disappear_anim, attr->duration);
    anim_set_path(&page_disappear_anim, attr->anim_curve);
    page_disappear_anim.user_data = page;
    anim_set_type(&page_disappear_anim, attr->anim_type, false);
}

// 设置动画曲线
static void anim_set_path(lv_anim_t *a, page_anim_curve path)
{
    switch (path) {
    case PAGE_ANIM_LINEAR:
        lv_anim_set_path_cb(a, lv_anim_path_linear);
        break;
    case PAGE_ANIM_EASE_IN:
        lv_anim_set_path_cb(a, lv_anim_path_ease_in_out);
        break;
    case PAGE_ANIM_EASE_OUT:
        lv_anim_set_path_cb(a, lv_anim_path_ease_out);
        break;
    }
}

// 设置动画函数
static void anim_set_type(lv_anim_t *a, page_anim_type type, bool is_appear)
{
    switch (type) {
    case PAGE_ANIM_NONE:
        lv_anim_set_values(a, 0, 0);
        lv_anim_set_custom_exec_cb(a, page_anim_none_callback);
        break;
    case PAGE_MOVE_TO_UP:
        if (is_appear)
            lv_anim_set_values(a, -LCD_H, 0);
        else
            lv_anim_set_values(a, 0, LCD_H);
        lv_anim_set_custom_exec_cb(a, page_anim_move_y_callback);
        break;
    case PAGE_MOVE_TO_DOWN:
        if (is_appear)
            lv_anim_set_values(a, LCD_H, 0);
        else
            lv_anim_set_values(a, 0, -LCD_H);
        lv_anim_set_custom_exec_cb(a, page_anim_move_y_callback);
        break;
    case PAGE_MOVE_TO_LEFT:
        if (is_appear)
            lv_anim_set_values(a, LCD_V, 0);
        else
            lv_anim_set_values(a, 0, -LCD_V);
        lv_anim_set_custom_exec_cb(a, page_anim_move_x_callback);
        break;
    case PAGE_MOVE_TO_RIGHT:
        if (is_appear)
            lv_anim_set_values(a, -LCD_V, 0);
        else
            lv_anim_set_values(a, 0, LCD_V);
        lv_anim_set_custom_exec_cb(a, page_anim_move_x_callback);
        break;
    case PAGE_FADE:
        if (is_appear)
            lv_anim_set_values(a, LV_OPA_MIN, LV_OPA_MAX);
        else
            lv_anim_set_values(a, LV_OPA_MAX, LV_OPA_MIN);
        lv_anim_set_custom_exec_cb(a, page_anim_fade_callback);
        break;
    }
}
