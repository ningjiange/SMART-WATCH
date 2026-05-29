#ifndef PAGE_MANAGER_H
#define PAGE_MANAGER_H

typedef enum {
    PAGE_HOME,
    PAGE_SPORT,
    PAGE_TOOLS,
    PAGE_GAME,
    PAGE_SETTINGS,
    PAGE_COUNT
} page_id_t;

void page_manager_init(void);
void page_manager_next(void);
void page_manager_prev(void);
void page_manager_select(void);
page_id_t page_manager_get_current(void);

#endif // PAGE_MANAGER_H
