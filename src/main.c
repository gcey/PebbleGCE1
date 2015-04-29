#include <pebble.h>
  
enum {
  KEY_TEMPERATURE = 0,
  KEY_CONDITIONS = 1,
  KEY_PLACE = 2
};

static Window *s_main_window;
static TextLayer *s_time_layer;
static TextLayer *s_blabla_layer;
static GFont s_time_font;
static GFont s_blabla_font;
static BitmapLayer *s_background_layer;
static GBitmap *s_background_bitmap;

static void update_time() {
  APP_LOG(APP_LOG_LEVEL_INFO, "Enter update_time()");

  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);

  // Create a long-lived buffer
  static char buffer[] = "XX:XX";

  // Write the current hours and minutes into the buffer
  if(clock_is_24h_style()) {
    // Use 24 hour format
    strftime(buffer, sizeof("XX:XX"), "%H:%M", tick_time);
  } else {
    // Use 12 hour format
    strftime(buffer, sizeof("XX:XX"), "%I:%M", tick_time);
  }

  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, buffer);
  APP_LOG(APP_LOG_LEVEL_INFO, buffer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
  
  // Get weather update every 30 minutes
  if(tick_time->tm_min % 30 == 0) {
    // Begin dictionary
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);

    // Add a key-value pair
    // dict_write_uint8(iter, 0, 0);

    // Send the message!
    APP_LOG(APP_LOG_LEVEL_INFO, "Request weather info update from phone");
    app_message_outbox_send();
  }
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Enter inbox_received_callback()");
  
  // Store incoming information
  static char temperature_buffer[8];
  static char conditions_buffer[32];
  static char place_buffer[32];
  static char weather_layer_buffer[32];
  
  // Read first item
  Tuple *t = dict_read_first(iterator);

  // For all items
  while(t != NULL) {
    // Which key was received?
    switch(t->key) {
    case KEY_TEMPERATURE:
      snprintf(temperature_buffer, sizeof(temperature_buffer), "%d°C", (int)t->value->int32);
      break;
    case KEY_CONDITIONS:
      snprintf(conditions_buffer, sizeof(conditions_buffer), "%s", t->value->cstring);
      break;
    case KEY_PLACE:
      snprintf(place_buffer, sizeof(place_buffer), "%s", t->value->cstring);
      break;
    default:
      APP_LOG(APP_LOG_LEVEL_ERROR, "Key %d not recognized!", (int)t->key);
      break;
    }

    // Look for next item
    t = dict_read_next(iterator);
  }
  
  // Assemble full string and display
  snprintf(weather_layer_buffer, sizeof(weather_layer_buffer), "%s, %s, %s",
           temperature_buffer, conditions_buffer, place_buffer);
  APP_LOG(APP_LOG_LEVEL_INFO, "Got weather info:");
  APP_LOG(APP_LOG_LEVEL_INFO, weather_layer_buffer);
  text_layer_set_text(s_blabla_layer, weather_layer_buffer);
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Inbox message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success");
}

static void main_window_load(Window *window) {
  // Create GBitmap, then set to created BitmapLayer
  s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_GCE);
  s_background_layer = bitmap_layer_create(GRect(0, 84, 144, 84));
  bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_background_layer));

  // Create time TextLayer
  s_time_layer = text_layer_create(GRect(0, 0, 144, 60));
  text_layer_set_background_color(s_time_layer, GColorClear);
#ifdef PBL_COLOR
  text_layer_set_text_color(s_time_layer, GColorDukeBlue);
#else
  text_layer_set_text_color(s_time_layer, GColorBlack);
#endif
  
  // Create GFont and apply it to TextLayer
  s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_GCE_DRUCKSCHRIFT_55));
  text_layer_set_font(s_time_layer, s_time_font);
  
  // Set alignment to center
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);

  // Add it as a child layer to the Window's root layer
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer));

  // blabla TextLayer
  s_blabla_layer = text_layer_create(GRect(0, 60, 144, 30));
  text_layer_set_background_color(s_blabla_layer, GColorClear);
#ifdef PBL_COLOR
  text_layer_set_text_color(s_blabla_layer, GColorDukeBlue);
#else
  text_layer_set_text_color(s_blabla_layer, GColorBlack);
#endif
  s_blabla_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_GCE_DRUCKSCHRIFT_16));
  text_layer_set_font(s_blabla_layer, s_blabla_font);
  text_layer_set_text_alignment(s_blabla_layer, GTextAlignmentCenter);
  text_layer_set_text(s_blabla_layer, "Loading...");  
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_blabla_layer));
}

static void main_window_unload(Window *window) {
  // Destroy layers and bitmaps
  text_layer_destroy(s_time_layer);
  bitmap_layer_destroy(s_background_layer);
  gbitmap_destroy(s_background_bitmap);
  
  // Unload fonts
  fonts_unload_custom_font(s_time_font);
  fonts_unload_custom_font(s_blabla_font);
}

static void init() {
  // Create main Window element and assign to pointer
  s_main_window = window_create();

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);
  
  // Make sure the time is displayed from the start
  update_time();
  
  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  
  // Register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  
  // Open AppMessage
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
}

static void deinit() {
    // Destroy main Window
    window_destroy(s_main_window);
}

int main(void) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Enter main()");
  init();
  app_event_loop();
  deinit();
}
