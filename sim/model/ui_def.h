#ifndef UI_DEF_H
#define UI_DEF_H

#include "base/types.h"

typedef struct sim_ui {
    void (*reset)(void *ui);
    bool (*reset_pending)(void *ui);
    void (*uart_out)(void *ui, u8 ch);
    bool (*uart_in)(void *ui, u8 *ch);
    void (*gpio_change)(void *ui, u32 val);
    bool (*gpio_in_poll)(void *ui, u32 *val);
    void (*cleanup)(void *ui);
} sim_ui_t;

extern sim_ui_t *ui_term_create(void);
extern sim_ui_t *ui_web_create(void);

#endif
