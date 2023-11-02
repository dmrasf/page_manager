#include "page_manager.h"
#include "lvgl.h"
#include "page_log.h"
#include "page_base.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static page_manager *default_page_manager = NULL;

extern void page_state_run(page_base *page);
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
 * @brief default_page_manager init
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
static bool page_register(page_desc *desc)
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
 * @brief Init page_desc
 * @param page Pointer to page_desc
 * @param cb create_page callback function
 * @param name page name
 * @param indev indev driver
 * @return true successful
 * @return false failed
 */
bool page_desc_init(page_desc *page, create_page_t cb, const char *name, lv_indev_t *indev)
{
    if (cb == NULL || name == NULL)
        return false;
    page->page_name = (char *)name;
    page->style_node = NULL;
    page->create_page = cb;
    page->indev = indev;
    return page_register(page);
}

void page_desc_add_style(page_desc *page, lv_style_t *style)
{
    page_desc_style_node *new_node = calloc(1, sizeof(page_desc_style_node));
    new_node->style = style;
    new_node->next = NULL;

    if (page->style_node == NULL) {
        page->style_node = new_node;
        return;
    }

    page_desc_style_node *node = page->style_node;
    while (node->next != NULL)
        node = node->next;
    node->next = new_node;
}

/**
 * @brief Unregister page from default_page_manager pools
 * @param desc Pointer to page description struct
 * @return true successful
 * @return false failed
 */
bool page_uninstall(page_desc *desc)
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

    //  state: load->will appear->start appear->appeared->avtivity
    page_state_run(&new_pbn->base);

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

    if (default_page_manager->page_stack->next != NULL)
        default_page_manager->page_stack->next->base.is_push = false;
    //  state: will appear->start appear->appeared->avtivity
    page_state_run(&default_page_manager->page_stack->next->base);

    page_base_node *pbn = default_page_manager->page_stack;
    pbn->base.is_push = false;
    default_page_manager->page_stack = pbn->next;

    // avtivity->will disappear->start disappear->disappeared->->will_appear->unload
    page_state_run(&pbn->base);

    return &default_page_manager->page_stack->base;
}
