#include "page_manager.h"
#include "lvgl.h"
#include "page_log.h"
#include "src/core/lv_obj_tree.h"
#include "src/misc/lv_anim.h"
#include "page_base.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static page_manager *default_page_manager = NULL;

/**
 * @brief Determine page description struct is valid
 * @param desc Pointer to page description struct
 * @return true valid
 * @return false invalid
 */
static bool is_right_page_desc(page_desc *desc)
{
    if (desc == NULL)
        return false;
    if (desc->create_page != NULL)
        return true;
    return false;
}

/**
 * @brief Determine page description struct is registered
 * @param desc Pointer to page description struct
 * @return true registered
 * @return false unregister
 */
static bool find_page_desc_in_pool(page_desc *desc)
{
    if (default_page_manager == NULL) {
        p_warning("default_page_manager is NULL");
        return false;
    }
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

/**
 * @brief Determine page description struct is in stack
 * @param desc Pointer to page description struct
 * @return true in stack
 * @return false not in stack
 */
static bool find_page_desc_in_stack(page_desc *desc)
{
    if (default_page_manager == NULL) {
        p_warning("%s: default_page_manager is NULL", __FUNCTION__);
        return false;
    }
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

/**
 * @brief default_page_manager init and animation init
 * @return true init successful
 * @return false init failed
 */
bool page_manager_init(void)
{
    if (default_page_manager != NULL) {
        p_warning("default_page_manager already exists");
        return true;
    }
    default_page_manager = calloc(1, sizeof(page_manager));
    if (default_page_manager != NULL) {
        default_page_manager->page_all = NULL;
        default_page_manager->page_stack = NULL;
        page_anim_init();
        p_log("default_page_manager calloc success");
        return true;
    }
    p_error("default_page_manager calloc error");
    return false;
}

/**
 * @brief Register page to default_page_manager pools
 * @param desc Pointer to page description struct
 * @return true successful
 * @return false failed
 */
bool page_register(page_desc *desc)
{
    if (!is_right_page_desc(desc)) {
        p_warning("%s, page_desc foramt error", __FUNCTION__);
        return false;
    }
    if (find_page_desc_in_pool(desc)) {
        p_warning("%s, page_desc already exists in pools", __FUNCTION__);
        return false;
    }

    page_desc_node *new_pdb = calloc(1, sizeof(page_desc_node));
    if (new_pdb == NULL) {
        p_warning("page_desc_node calloc failed");
        return false;
    }
    new_pdb->desc = desc;
    new_pdb->next = default_page_manager->page_all;
    default_page_manager->page_all = new_pdb;
    return true;
}

/**
 * @brief Unregister page from default_page_manager pools
 * @param desc Pointer to page description struct
 * @return true successful
 * @return false failed
 */
bool page_unregister(page_desc *desc)
{
    if (!is_right_page_desc(desc)) {
        p_warning("%s: page_desc foramt error", __FUNCTION__);
        return false;
    }
    if (!find_page_desc_in_pool(desc)) {
        p_warning("%s: page is not in pools", __FUNCTION__);
        return true;
    }
    if (find_page_desc_in_stack(desc)) {
        p_warning("%s: page is on the stack and cannot be unregister", __FUNCTION__);
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

/**
 * @brief Determine page animation is finished
 * @return true finished
 * @return false not yet
 */
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

/**
 * @brief Push page to stack
 * @param desc Pointer to page description struct
 * @return page_base* Pointer to page in stack top
 */
page_base *page_push(page_desc *desc)
{
    if (default_page_manager == NULL) {
        p_warning("%s: default_page_manager is NULL", __FUNCTION__);
        return false;
    }
    if (!is_page_anim_done()) {
        p_warning("page animation not finished");
        return NULL;
    }
    if (!find_page_desc_in_pool(desc)) {
        p_warning("%s: page is not in pools", __FUNCTION__);
        return NULL;
    }

    // free in do_unload()
    page_base_node *new_pbn = calloc(1, sizeof(page_base_node));
    if (new_pbn == NULL) {
        p_error("page_base_node calloc failed");
        return NULL;
    }
    new_pbn->base.desc = desc;
    new_pbn->base.lv_root = NULL;
    new_pbn->base.state = PAGE_STATE_IDLE;
    new_pbn->base.is_push = true;
    new_pbn->base.node = new_pbn;
    new_pbn->next = NULL;

    if (default_page_manager->page_stack == NULL) {
        default_page_manager->page_stack = new_pbn;
    } else {
        new_pbn->next = default_page_manager->page_stack;
        default_page_manager->page_stack->base.is_push = true;
        default_page_manager->page_stack = new_pbn;
    }

    new_pbn->base.state = PAGE_STATE_LOAD;

    //  state: load->will appear->start appear anim->animation finished->appeared->avtivity
    page_state_run(&new_pbn->base);

    if (new_pbn->next != NULL)
        // 判断原栈页面是否有相应动画，没有的话需要等新栈动画结束之后更新状态，否则直接更新状态，同步运行动画
        if (new_pbn->next->base.desc->anim_desc.page_push_out.anim_type != PAGE_ANIM_NONE)
            // avtivity->will disappear->start disappear anim->animation finishes->disappeared->->will_appear
            page_state_run(&new_pbn->next->base);

    return &default_page_manager->page_stack->base;
}

/**
 * @brief Pop page from stack
 * @return page_base* Pointer to page in stack top
 */
page_base *page_pop(void)
{
    if (default_page_manager == NULL) {
        p_warning("%s: default_page_manager is NULL", __FUNCTION__);
        return false;
    }
    if (default_page_manager->page_stack == NULL) {
        p_warning("page stack is NULL");
        return NULL;
    }
    if (!is_page_anim_done()) {
        p_warning("Page animation not finished");
        return NULL;
    }

    if (default_page_manager->page_stack->next != NULL)
        default_page_manager->page_stack->next->base.is_push = false;
    //  state: will appear->start appear anim->animation finished->appeared->avtivity
    page_state_run(&default_page_manager->page_stack->next->base);

    page_base_node *pbn = default_page_manager->page_stack;
    pbn->base.is_push = false;
    // avtivity->will disappear->start disappear anim->animation finishes->disappeared->->will_appear->unload
    page_state_run(&pbn->base);

    default_page_manager->page_stack = pbn->next;
    return &default_page_manager->page_stack->base;
}
