#include "app.h"
#include "ui.h"
#include "communication.h"
#include "data.h"

static void hour_tick_handler(struct tm *tick_time, TimeUnits units_changed)
{
    if (units_changed & HOUR_UNIT)
    {
        send_update_all_message();
    }
}

static void init(void)
{
    data_init();
    ui_init();
    communication_init();
    tick_timer_service_subscribe(HOUR_UNIT, hour_tick_handler);
}

static void deinit(void)
{
    communication_deinit();
    ui_deinit();
    data_deinit();
    tick_timer_service_unsubscribe();
}

int main(void)
{
    init();
    app_event_loop();
    deinit();
    return 0;
}
