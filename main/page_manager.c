#include "page_manager.h"

static page_id_t current_page = PAGE_HOME;

void page_manager_init(void) {
    current_page = PAGE_HOME;
}

void page_manager_next(void) {
    current_page = (current_page + 1) % PAGE_COUNT;
}

void page_manager_prev(void) {
    if (current_page == PAGE_HOME) {
        current_page = PAGE_COUNT - 1;
    } else {
        current_page = current_page - 1;
    }
}

void page_manager_select(void) {
    // 选择当前页面的处理逻辑
    // 可以根据 current_page 执行不同的操作
    // 目前仅作为占位
}

page_id_t page_manager_get_current(void) {
    return current_page;
}
