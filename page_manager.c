#include "page_manager.h"
#include "lvgl.h"
#include "page_log.h"
#include "src/core/lv_obj_tree.h"
#include "src/misc/lv_anim.h"
#include "page_base.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static page_manager *default_page_manager;

static bool is_right_page_desc(page_desc *desc);
static bool find_page_desc_in_pool(page_desc *desc);
static bool find_page_desc_in_stack(page_desc *desc);

void page_manager_init(void)
{
    default_page_manager = malloc(sizeof(page_manager));
    default_page_manager->page_all = NULL;
    default_page_manager->page_stack = NULL;
    page_anim_init();
}

// 注册页面到管理器
bool page_register(page_desc *desc)
{
    if (!is_right_page_desc(desc)) {
        debug("%s: page_desc格式错误\n", __FUNCTION__);
        return false;
    }
    if (find_page_desc_in_pool(desc)) {
        debug("%s: page_desc已经存在于pool中\n", __FUNCTION__);
        return false;
    }

    page_desc_node *new_pdb = calloc(1, sizeof(page_desc_node));
    new_pdb->desc = desc;
    new_pdb->next = default_page_manager->page_all;
    default_page_manager->page_all = new_pdb;
    return true;
}

bool page_unregister(page_desc *desc)
{
    if (!is_right_page_desc(desc)) {
        debug("%s: page_desc格式错误\n", __FUNCTION__);
        return false;
    }
    // 页面当前处于栈中，不可移除
    if (find_page_desc_in_stack(desc)) {
        debug("%s: 页面处于栈中，不可移除\n", __FUNCTION__);
        return false;
    }
    if (!find_page_desc_in_pool(desc)) {
        debug("%s: page_desc不在pool中\n", __FUNCTION__);
        return false;
    }

    page_desc_node *pdn = default_page_manager->page_all;
    page_desc_node *pdn_pre = default_page_manager->page_all;
    while (pdn->desc != desc) {
        pdn_pre = pdn;
        pdn = pdn->next;
    }
    pdn_pre->next = pdn->next;
    free(pdn);

    return true;
}

static bool is_page_anim_done(void)
{
    bool top1 = false;
    bool top2 = false;
    if (default_page_manager->page_stack != NULL) {
        top1 = default_page_manager->page_stack->base.is_anim_busy;
        if (default_page_manager->page_stack->next != NULL)
            top2 = default_page_manager->page_stack->next->base.is_anim_busy;
    }
    return !top2 && !top1;
}

page_base *page_push(page_desc *desc)
{
    // 判断当前栈的前两个页面动画是否完成
    if (!is_page_anim_done()) {
        debug("动画未完成\n");
        return NULL;
    }

    if (!find_page_desc_in_pool(desc)) {
        debug("%s: page_desc不在pool中\n", __FUNCTION__);
        return NULL;
    }

    // 新建堆栈节点  在do_unload()中free
    page_base_node *new_pbn = calloc(1, sizeof(page_base_node));
    new_pbn->base.desc = desc;
    new_pbn->base.lv_root = NULL;
    new_pbn->base.state = PAGE_STATE_IDLE;
    new_pbn->base.is_push = true;
    new_pbn->base.node = new_pbn;
    new_pbn->next = NULL;

    // 如果栈为空
    if (default_page_manager->page_stack == NULL)
        default_page_manager->page_stack = new_pbn;
    // 否则将page放到栈顶
    else {
        new_pbn->next = default_page_manager->page_stack;
        default_page_manager->page_stack->base.is_push = true;
        default_page_manager->page_stack = new_pbn;
    }

    new_pbn->base.state = PAGE_STATE_LOAD;
    //  load->avtivity
    page_state_run(&new_pbn->base);

    // 判断原栈页面是否有相应动画，没有的话需要等新栈动画结束之后更新状态，否则直接更新状态，同步运行动画
    if (new_pbn->next != NULL)
        if (new_pbn->next->base.desc->anim_desc.page_push_out.anim_type != PAGE_ANIM_NONE)
            // avtivity->will_appear
            page_state_run(&new_pbn->next->base);

    return &default_page_manager->page_stack->base;
}

// 页面弹出，返回栈顶
page_base *page_pop(void)
{
    // 判断当前栈的前两个页面动画是否完成
    if (!is_page_anim_done()) {
        debug("动画未完成\n");
        return NULL;
    }

    // 如果栈为空，不操作
    if (default_page_manager->page_stack == NULL)
        return NULL;

    // 第二个栈先显示
    default_page_manager->page_stack->next->base.is_push = false;
    if (default_page_manager->page_stack->next != NULL)
        // will_appear->avtivity
        page_state_run(&default_page_manager->page_stack->next->base);

    page_base_node *pbn = default_page_manager->page_stack;
    pbn->base.is_push = false;
    // avtivity->will_appear
    page_state_run(&pbn->base);

    default_page_manager->page_stack = pbn->next;
    return &default_page_manager->page_stack->base;
}

static bool is_right_page_desc(page_desc *desc)
{
    if (desc == NULL)
        return false;
    if (desc->create_page != NULL)
        return true;
    return false;
}

static bool find_page_desc_in_pool(page_desc *desc)
{
    page_desc_node *pdn = default_page_manager->page_all;
    if (pdn == NULL)
        return false;
    while (pdn->desc != desc) {
        pdn = pdn->next;
        if (pdn == NULL)
            return false;
    }
    return true;
}

static bool find_page_desc_in_stack(page_desc *desc)
{
    page_base_node *pbn = default_page_manager->page_stack;
    if (pbn == NULL)
        return false;
    while (pbn->base.desc != desc) {
        pbn = pbn->next;
        if (pbn == NULL)
            return false;
    }
    return true;
}
