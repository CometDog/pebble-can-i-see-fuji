#include "communication.h"
#include "data.h"
#include "ui.h"
#include <pebble-events/pebble-events.h>

static void inbox_received_callback(DictionaryIterator *iter, void *context)
{
    Tuple *type_tuple = dict_find(iter, MESSAGE_KEY_type);
    if (!type_tuple)
        return;

    if (strcmp(type_tuple->value->cstring, "ready") == 0)
    {
        send_update_all_message();
        return;
    }

    if (strcmp(type_tuple->value->cstring, "new_score") == 0)
    {
        Tuple *region_tuple = dict_find(iter, MESSAGE_KEY_region);
        Tuple *time_tuple = dict_find(iter, MESSAGE_KEY_time);
        Tuple *score_tuple = dict_find(iter, MESSAGE_KEY_score);

        if (region_tuple && time_tuple && score_tuple)
        {
            Region region = (strcmp(region_tuple->value->cstring, "north") == 0) ? REGION_NORTH : REGION_SOUTH;
            TimePeriod time = (strcmp(time_tuple->value->cstring, "morning") == 0) ? TIME_MORNING : TIME_AFTERNOON;
            int8_t score = (int8_t)score_tuple->value->int32;

            set_region_score(region, time, score);

            if (region == get_current_region())
            {
                update_score(time);
            }
        }
        return;
    }

    if (strcmp(type_tuple->value->cstring, "new_scores") == 0)
    {
        Tuple *north_morning = dict_find(iter, MESSAGE_KEY_northMorning);
        Tuple *north_afternoon = dict_find(iter, MESSAGE_KEY_northAfternoon);
        Tuple *south_morning = dict_find(iter, MESSAGE_KEY_southMorning);
        Tuple *south_afternoon = dict_find(iter, MESSAGE_KEY_southAfternoon);

        if (north_morning && north_afternoon && south_morning && south_afternoon)
        {
            bool already_loaded = get_data_loaded_progress() == 4;

            set_region_score(REGION_NORTH, TIME_MORNING, (int8_t)north_morning->value->int32);
            set_region_score(REGION_NORTH, TIME_AFTERNOON, (int8_t)north_afternoon->value->int32);
            set_region_score(REGION_SOUTH, TIME_MORNING, (int8_t)south_morning->value->int32);
            set_region_score(REGION_SOUTH, TIME_AFTERNOON, (int8_t)south_afternoon->value->int32);

            if (!already_loaded && get_data_loaded_progress() == 4)
            {
                show_main_window();
            }
            else
            {
                update_all();
            }
        }
    }
}

bool send_update_all_message(void)
{
    DictionaryIterator *iter;
    AppMessageResult appMessageResult = app_message_outbox_begin(&iter);

    if (appMessageResult != APP_MSG_OK)
    {
        APP_LOG(APP_LOG_LEVEL_ERROR, "[AppMessage] Outbox begin failed: %d", appMessageResult);
        return false;
    }

    DictionaryResult dictResult = dict_write_cstring(iter, MESSAGE_KEY_type, "update_all");
    if (dictResult != DICT_OK)
    {
        APP_LOG(APP_LOG_LEVEL_ERROR, "[AppMessage] Write failed: %d", dictResult);
        return false;
    }

    appMessageResult = app_message_outbox_send();
    if (appMessageResult != APP_MSG_OK)
    {
        APP_LOG(APP_LOG_LEVEL_ERROR, "[AppMessage] Outbox send failed: %d", appMessageResult);
        return false;
    }

    return true;
}

void communication_init(void)
{
    events_app_message_request_inbox_size(128);
    events_app_message_request_outbox_size(128);
    events_app_message_register_inbox_received(inbox_received_callback, NULL);
    events_app_message_open();
}

void communication_deinit(void)
{
}
