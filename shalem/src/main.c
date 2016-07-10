#include <pebble.h>
#include "main.h"

static Window *s_main_window;
static Layer *s_canvas_layer;
static TextLayer *s_time_layer;
static TextLayer *s_date_layer;
static TextLayer *s_steps_layer;

ClaySettings settings;

static void init_default_settings() {
  settings.BackgroundColor = GColorMintGreen;
  settings.SecondaryColor = GColorOxfordBlue;
  settings.ClockTextColor = GColorBlack;
  settings.DateTextColor = GColorWhite;
  settings.StepsTextColor = GColorBlack;
  settings.ShowSteps = true;
}

static void load_settings() {
  init_default_settings();
  persist_read_data(SETTINGS_KEY, &settings, sizeof(settings));
}

static void save_settings() {
  persist_write_data(SETTINGS_KEY, &settings, sizeof(settings));
  update_display();
}

static void update_display() {
  // change background color based on settings
  window_set_background_color(s_main_window, settings.BackgroundColor);

  // hide step count depending on settings
  layer_set_hidden((Layer *) s_steps_layer, !settings.ShowSteps);

  // redraw canvas (rings, date background) using colors from settings
  layer_mark_dirty(s_canvas_layer);

  // change text colors
  text_layer_set_text_color(s_time_layer, settings.ClockTextColor);
  text_layer_set_text_color(s_date_layer, settings.DateTextColor);
  text_layer_set_text_color(s_steps_layer, settings.StepsTextColor);
}

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);

  // Write the current hours and minutes into a buffer
  static char s_buffer[8];

  if (clock_is_24h_style()) {
    strftime(s_buffer, sizeof(s_buffer), "%H:%M", tick_time);
    text_layer_set_text(s_time_layer, s_buffer);
  } else {
    strftime(s_buffer, sizeof(s_buffer), "%I:%M", tick_time);
    // get rid of 0 and leading space, e.g., in 08:30
    text_layer_set_text(s_time_layer, s_buffer + (('0' == s_buffer[0])?1:0));
  }
}

static void update_date() {
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  static char s_buffer[8];
  // format: e.g., Jul 21
  strftime(s_buffer, sizeof(s_buffer), "%b %e", tick_time);

  text_layer_set_text(s_date_layer, s_buffer);
}

static void update_step_count() {
  HealthMetric metric = HealthMetricStepCount;
  time_t start = time_start_of_today();
  time_t end = time(NULL);

  HealthServiceAccessibilityMask mask = health_service_metric_accessible(metric, start, end);

  // get step count if available, add heart emoji
  if (mask & HealthServiceAccessibilityMaskAvailable) {
    int step_count = (int) health_service_sum_today(metric);
    static char buf[16];
    snprintf(buf, sizeof(buf), "\U0001F49C %d", step_count);
    text_layer_set_text(s_steps_layer, buf);
  } else {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Data unavailable");
  }
}

// update time and date on tick
static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  if ((units_changed & MINUTE_UNIT) != 0) {
    update_time();
  }

  if ((units_changed & DAY_UNIT) != 0) {
    update_date();
  }
}

// update step count when step count changes
static void health_handler(HealthEventType event, void *context) {
  switch (event) {
    case HealthEventSignificantUpdate:
      break;
    case HealthEventMovementUpdate:
      update_step_count();
      break;
    case HealthEventSleepUpdate:
      break;
  }
}

// draw outer rings and background for date
static void canvas_update_proc(Layer *layer, GContext *ctx) {
  // date background
  int rect_width = text_layer_get_content_size(s_date_layer).w + 10;
  GRect rect_bounds = GRect(((180 - rect_width) / 2), 110, rect_width, 25);
  graphics_context_set_fill_color(ctx, settings.SecondaryColor);
  int corner_radius = 5;
  graphics_fill_rect(ctx, rect_bounds, corner_radius, GCornersAll);

  // outer rings
  GPoint circle_center = GPoint(90, 90);
  uint16_t circle_radius = 75;
  graphics_context_set_stroke_color(ctx, settings.SecondaryColor);
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_circle(ctx, circle_center, circle_radius);

  graphics_context_set_stroke_width(ctx, 3);
  uint16_t outer_circle_radius = 80;
  graphics_draw_circle(ctx, circle_center, outer_circle_radius);
}

static void main_window_load(Window *window) {
  // Get information about the Window
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // CANVAS
  s_canvas_layer = layer_create(bounds);
  layer_set_update_proc(s_canvas_layer ,canvas_update_proc);
  layer_add_child(window_layer, s_canvas_layer);

  // TIME
  s_time_layer = text_layer_create(
      GRect(0, PBL_IF_ROUND_ELSE(58, 52), bounds.size.w, 50));

  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorBlack);
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);

  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));

  // DATE
  s_date_layer = text_layer_create(GRect(0, 105, bounds.size.w, 30));

  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_color(s_date_layer, GColorWhite);
  text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);

  layer_add_child(window_layer, text_layer_get_layer(s_date_layer));

  // STEPS
  s_steps_layer = text_layer_create(GRect(0, 40, bounds.size.w, 30));

  text_layer_set_background_color(s_steps_layer, GColorClear);
  text_layer_set_text_color(s_steps_layer, GColorBlack);
  text_layer_set_font(s_steps_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(s_steps_layer, GTextAlignmentCenter);

  layer_add_child(window_layer, text_layer_get_layer(s_steps_layer));

  update_display();
}

static void main_window_unload(Window *window) {
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_date_layer);
  text_layer_destroy(s_steps_layer);
  layer_destroy(s_canvas_layer);
}

// get app settings and set ClaySettings object
static void inbox_received_handler(DictionaryIterator *iter, void *context) {
  // Background Color
  Tuple *bg_color_t = dict_find(iter, MESSAGE_KEY_BackgroundColor);
  if (bg_color_t) {
    settings.BackgroundColor = GColorFromHEX(bg_color_t->value->int32);
  }

  // Secondary Color
  Tuple *sg_color_t = dict_find(iter, MESSAGE_KEY_SecondaryColor);
  if (sg_color_t) {
    settings.SecondaryColor = GColorFromHEX(sg_color_t->value->int32);
  }

  // Text colors
  Tuple *clock_color_t = dict_find(iter, MESSAGE_KEY_ClockTextColor);
  if (clock_color_t) {
    settings.ClockTextColor = GColorFromHEX(clock_color_t->value->int32);
  }
  Tuple *date_color_t = dict_find(iter, MESSAGE_KEY_DateTextColor);
  if (date_color_t) {
    settings.DateTextColor = GColorFromHEX(date_color_t->value->int32);
  }
  Tuple *steps_color_t = dict_find(iter, MESSAGE_KEY_StepsTextColor);
  if (steps_color_t) {
    settings.StepsTextColor = GColorFromHEX(steps_color_t->value->int32);
  }

  // Steps toggle
  Tuple *steps = dict_find(iter, MESSAGE_KEY_ShowSteps);
  if (steps) {
    settings.ShowSteps = steps->value->int32 == 1;
  }

  save_settings();
}

static void init() {
  load_settings();

  // get app settings, handle with inbox_received_handler
  app_message_register_inbox_received(inbox_received_handler);
  app_message_open(128, 128);

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
  update_date();
  update_step_count();

  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT | DAY_UNIT, tick_handler);

  // Register with Health if available
  #if defined(PBL_HEALTH)
  if (!health_service_events_subscribe(health_handler, NULL)) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "health not available");
  }
  #else
  APP_LOG(APP_LOG_LEVEL_ERROR, "health not available");
  #endif
}

static void deinit() {
  // Destroy Window
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}