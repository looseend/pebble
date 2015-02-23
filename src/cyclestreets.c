#include <pebble.h>

static Window *window;
static TextLayer *instruction_layer;
static TextLayer *turn_text_layer;
static BitmapLayer *turn_layer;
static bool messageProcessing;

static GBitmap* current_turn = NULL;

static char instruction_text[128];
static char turn_text[32];

#define TURN 0
#define STREET 1
#define DISTANCE 2
#define RUNNING 3
#define INSTRUCTION 4
#define STATE 5

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  current_turn = gbitmap_create_with_resource(RESOURCE_ID_START);
  turn_layer = bitmap_layer_create((GRect) { .origin = {0, 0}, .size = {72,72}});
  APP_LOG(APP_LOG_LEVEL_DEBUG, "%ix%i", current_turn->bounds.size.w, current_turn->bounds.size.h);
  bitmap_layer_set_bitmap(turn_layer, current_turn);
  layer_add_child(window_layer, bitmap_layer_get_layer(turn_layer));

  turn_text_layer = text_layer_create((GRect) { .origin = {74, 0 }, .size = {68, 72}});
  text_layer_set_font(turn_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text_alignment(turn_text_layer, GTextAlignmentRight);
  layer_add_child(window_layer, text_layer_get_layer(turn_text_layer));
  strncpy(turn_text, "", 1);
  text_layer_set_text(turn_text_layer, turn_text);
  
  instruction_layer = text_layer_create((GRect) { .origin = {0, 74}, .size = {bounds.size.w,86}});
  text_layer_set_font(instruction_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text_alignment(instruction_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(instruction_layer));
  strncpy(instruction_text, "Welcome to Cyclestreets", 23);
  text_layer_set_text(instruction_layer, instruction_text);
}

static void window_unload(Window *window) {
  text_layer_destroy(instruction_layer);
  text_layer_destroy(turn_text_layer);
  bitmap_layer_destroy(turn_layer);
  if (current_turn != NULL) {
      gbitmap_destroy(current_turn);
  }
}


static void set_turn_bitmap(Tuple *t) {
    if (t != NULL) {
        int instruction = -1;
        char* instr = t->value->cstring;
        if (0 == strcmp("Bear left", instr)) {
            instruction = RESOURCE_ID_BEAR_LEFT;
        } else if (0 == strcmp("Bear right", instr)) {
            instruction = RESOURCE_ID_BEAR_RIGHT;
        } else if (0 == strcmp("Double back", instr)) {
            instruction = RESOURCE_ID_DOUBLE_BACK;
        } else if (0 == strcmp("First exit", instr)) {
            instruction = RESOURCE_ID_FIRST_EXIT;
        } else if (0 == strcmp("Roundabout", instr)) {
            instruction = RESOURCE_ID_ROUNDABOUT;
        } else if (0 == strcmp("Second exit", instr)) {
            instruction = RESOURCE_ID_SECOND_EXIT;
        } else if (0 == strcmp("Sharp left", instr)) {
            instruction = RESOURCE_ID_SHARP_LEFT;
        } else if (0 == strcmp("Sharp right", instr)) {
            instruction = RESOURCE_ID_SHARP_RIGHT;
        } else if (0 == strcmp("Straight on", instr)) {
            instruction = RESOURCE_ID_STRAIGHT_ON;
        } else if (0 == strcmp("Third exit", instr)) {
            instruction = RESOURCE_ID_THIRD_EXIT;
        } else if (0 == strcmp("Turn left", instr)) {
            instruction = RESOURCE_ID_TURN_LEFT;
        } else if (0 == strcmp("Turn right", instr)) {
            instruction = RESOURCE_ID_TURN_RIGHT;
        }
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Got '%s' turn, instruction %i", instr, instruction);
        
        if (instruction != -1) {
            GBitmap *old = current_turn;
            current_turn = gbitmap_create_with_resource(instruction);
            bitmap_layer_set_bitmap(turn_layer, current_turn);
            gbitmap_destroy(old);
        }
    }
}

static void set_turn_instruction(DictionaryIterator *iter) {
    Tuple *turn = dict_find(iter, TURN);
    
    snprintf(turn_text, 32, "%s", 
             (turn == NULL) ? "" : turn->value->cstring);
    text_layer_set_text(turn_text_layer, turn_text);
}

static void inbox_handler(DictionaryIterator *iter, void *context) {
    set_turn_bitmap(dict_find(iter, TURN));
    set_turn_instruction(iter);
    
    Tuple *state = dict_find(iter, STATE);
    Tuple *street = dict_find(iter, STREET);
    Tuple *distance = dict_find(iter, DISTANCE);

    if (strcmp("NearingTurn", state->value->cstring) == 0) {
        snprintf(instruction_text, 128, "%s\n%s",
                 (street == NULL) ? "" : street->value->cstring,
                 (distance == NULL) ? "" : distance->value->cstring);
        vibes_long_pulse();
        light_enable_interaction();
    } else if (strcmp("AdvanceToSegment", state->value->cstring) == 0) {
        snprintf(instruction_text, 128, "%s\nNow",
                 (street == NULL) ? "" : street->value->cstring);
        vibes_double_pulse();
        light_enable_interaction();
    } else if (strcmp("ReplanFromHere", state->value->cstring) == 0) {
        snprintf(instruction_text, 128, "Replanning");
        vibes_double_pulse();
        vibes_double_pulse();
    } else if (strcmp("OnTheMove", state->value->cstring) == 0) {
        snprintf(instruction_text, 128, "%s",
                 (street == NULL) ? "" : street->value->cstring);
    } else if (strcmp("LiveRideStart", state->value->cstring) == 0) {
        snprintf(instruction_text, 128, "Starting\nRide");        
    } else if (strcmp("NearingTurn", state->value->cstring) == 0){
        snprintf(instruction_text, 128, "%s\n%s",
                 (street == NULL) ? "" : street->value->cstring,
                 (distance == NULL) ? "" : distance->value->cstring);
    } else {
        snprintf(instruction_text, 128, "%s", state->value->cstring);
    }
     
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Instructing: %s", instruction_text);
    text_layer_set_text(instruction_layer, instruction_text);
    
    Tuple *t = dict_read_first(iter);
    while (t != NULL) {
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Got %u message, %s", (unsigned int)t->key, t->value->cstring);
        t = dict_read_next(iter);
    }
}

static void inbox_dropped_handler(AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Dropped ");
    switch (reason) {
    case APP_MSG_OK : APP_LOG(APP_LOG_LEVEL_DEBUG, "OK"); break;
    case APP_MSG_SEND_TIMEOUT : APP_LOG(APP_LOG_LEVEL_DEBUG, "Timeout"); break;
    case APP_MSG_SEND_REJECTED : APP_LOG(APP_LOG_LEVEL_DEBUG, "rejected"); break;
    case APP_MSG_NOT_CONNECTED : APP_LOG(APP_LOG_LEVEL_DEBUG, "not connected"); break;
    case APP_MSG_APP_NOT_RUNNING : APP_LOG(APP_LOG_LEVEL_DEBUG, "not running"); break;
    case APP_MSG_INVALID_ARGS : APP_LOG(APP_LOG_LEVEL_DEBUG, "invalid args"); break;
    case APP_MSG_BUSY : APP_LOG(APP_LOG_LEVEL_DEBUG, "busy"); break;
    case APP_MSG_BUFFER_OVERFLOW : APP_LOG(APP_LOG_LEVEL_DEBUG, "overflow"); break;
    case APP_MSG_ALREADY_RELEASED : APP_LOG(APP_LOG_LEVEL_DEBUG, "released"); break;
    case APP_MSG_CALLBACK_ALREADY_REGISTERED : APP_LOG(APP_LOG_LEVEL_DEBUG, "already registered"); break;
    case APP_MSG_CALLBACK_NOT_REGISTERED : APP_LOG(APP_LOG_LEVEL_DEBUG, "not registered"); break;
    case APP_MSG_OUT_OF_MEMORY : APP_LOG(APP_LOG_LEVEL_DEBUG, "out of memoery"); break;
    case APP_MSG_CLOSED : APP_LOG(APP_LOG_LEVEL_DEBUG, "closed"); break;
    case APP_MSG_INTERNAL_ERROR : APP_LOG(APP_LOG_LEVEL_DEBUG, "internal"); break;
    }
    messageProcessing = false;
}

static void outbox_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
}

static void register_handlers() {
    app_message_register_inbox_received(inbox_handler);
    app_message_register_inbox_dropped(inbox_dropped_handler);
    app_message_register_outbox_failed(outbox_failed_handler);
    app_message_open(2048, 512);
}

static void init(void) {
    register_handlers();
  window = window_create();
  window_set_click_config_provider(window, click_config_provider);
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_set_fullscreen(window, true);
  const bool animated = true;
  window_stack_push(window, animated);
  messageProcessing = false;
}

static void deinit(void) {
  window_destroy(window);
}

int main(void) {
  init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);

  app_event_loop();
  deinit();
}
