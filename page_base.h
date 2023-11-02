#ifndef __PAGE_BASE_H__
#define __PAGE_BASE_H__

#include "lvgl.h"
#include <stdint.h>

#define LCD_W 240
#define LCD_H 240

typedef struct page_manager_t page_manager;
typedef struct page_base_node_t page_base_node;
typedef struct page_base_t page_base;
typedef struct page_desc_t page_desc;
typedef struct page_desc_style_node_t page_desc_style_node;

typedef void (*create_page_t)(lv_obj_t *, page_desc *);
typedef void (*page_state_callback)(const lv_obj_t *);

typedef struct page_desc_t {
    char *page_name;                       /* 页面名字 */
    lv_group_t *group;                     /* 组 */
    lv_indev_t *indev;                     /* 输入设备 */
    lv_timer_t *ui_timer;                  /* 页面刷新 */
    void *user_data;                       /* 传递消息 */
    page_desc_style_node *style_node;      /* 保存style指针，用于回收 */
    create_page_t create_page;             /* 页面创建函数 */
    page_state_callback on_will_load;      /* 即将创建 */
    page_state_callback on_loaded;         /* 创建完成 */
    page_state_callback on_will_appear;    /* 即将显示 */
    page_state_callback on_appeared;       /* 显示到屏幕 */
    page_state_callback on_will_disappear; /* 即将消失 */
    page_state_callback on_disappeared;    /* 设置为不可见 */
    page_state_callback on_will_unload;    /* 即将移除 */
    page_state_callback on_unloaded;       /* 已经移除 */
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
    lv_obj_t *lv_root;    /* root节点 */
    page_state state;     /* 页面状态 */
    page_desc *desc;      /* 页面描述 */
    page_base_node *node; /* 保存页面在栈中地址，用于free */
    bool is_push;         /* 由push发起动作，判断是否unload */
} page_base;

#endif /* __PAGE_BASE_H__ */
