#ifndef __PAGE_BASE_H__
#define __PAGE_BASE_H__

#include "lvgl.h"
#include "src/core/lv_obj.h"
#include <stdint.h>

#define LCD_V 240
#define LCD_H 240

typedef struct page_manager_t page_manager;
typedef struct page_base_node_t page_base_node;
typedef struct page_base_t page_base;

typedef void (*create_page_t)(const lv_obj_t *);
typedef void (*page_state_callback)(const lv_obj_t *);

typedef enum page_anim_type_e {
    PAGE_ANIM_NONE = 0,
    PAGE_MOVE_TO_LEFT,
    PAGE_MOVE_TO_RIGHT,
    PAGE_MOVE_TO_UP,
    PAGE_MOVE_TO_DOWN,
    PAGE_FADE,
} page_anim_type;

typedef enum page_anim_curve_e {
    PAGE_ANIM_LINEAR = 0,
    PAGE_ANIM_EASE_IN,
    PAGE_ANIM_EASE_OUT,
    PAGE_ANIM_EASE_IN_OUT,
    PAGE_ANIM_STEP,
    PAGE_ANIM_OVERSHOOT,
    PAGE_ANIM_BOUNCE,
} page_anim_curve;

typedef struct page_anim_attr_t {
    page_anim_type anim_type;
    page_anim_curve anim_curve;
    uint32_t duration;
} page_anim_attr;

typedef struct page_anim_desc_t {
    page_anim_attr page_push_in;
    page_anim_attr page_push_out;
    page_anim_attr page_pop_out;
    page_anim_attr page_pop_in;
} page_anim_desc;

typedef struct page_desc_t {
    char *page_name;                       /* 页面名字 */
    create_page_t create_page;             /* 页面创建函数 */
    page_state_callback on_will_load;      /* 即将创建 */
    page_state_callback on_loaded;         /* 创建完成 */
    page_state_callback on_will_appear;    /* 即将显示 */
    page_state_callback on_appeared;       /* 显示到屏幕 */
    page_state_callback on_will_disappear; /* 即将消失 */
    page_state_callback on_disappeared;    /* 设置为不可见 */
    page_state_callback on_will_unload;    /* 即将移除 */
    page_state_callback on_unloaded;       /* 已经移除 */
    page_anim_desc anim_desc;              /* 页面切换动画参数 */
} page_desc;

typedef enum {
    PAGE_STATE_IDLE,
    PAGE_STATE_LOAD,
    PAGE_STATE_WILL_APPEAR,
    PAGE_STATE_DID_APPEAR,
    PAGE_STATE_ACTIVITY,
    PAGE_STATE_WILL_DISAPPEAR,
    PAGE_STATE_DID_DISAPPEAR,
    PAGE_STATE_UNLOAD,
} page_state;

typedef struct page_base_t {
    lv_obj_t *lv_root;    /* lvgl节点 */
    page_state state;     /* 页面状态 */
    page_desc *desc;      /* 页面描述 */
    page_base_node *node; /* 保存页面在栈中地址，用于free */
    bool is_push;         /* 由push发起动作 */
    bool is_anim_busy;    /* 页面切换动画执行中 */
} page_base;

#endif /* __PAGE_BASE_H__ */
