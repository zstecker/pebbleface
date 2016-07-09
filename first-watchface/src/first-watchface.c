#include <pebble.h>

static Window *s_main_window;
static Layer *s_canvas_layer;
static TextLayer *s_time_layer;
static TextLayer *s_date_layer;
static TextLayer *s_steps_layer;

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);

  // Write the current hours and minutes into a buffer
  static char s_buffer[8];
  strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ?
                                          "%H:%M" : "%l:%M", tick_time);

  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, s_buffer);
}

static void update_date() {
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  static char s_buffer[8];
  strftime(s_buffer, sizeof(s_buffer), "%b %e", tick_time);

  text_layer_set_text(s_date_layer, s_buffer);
}

static void update_step_count() {
  HealthMetric metric = HealthMetricStepCount;
  time_t start = time_start_of_today();
  time_t end = time(NULL);

  HealthServiceAccessibilityMask mask = health_service_metric_accessible(metric, start, end);

  if (mask & HealthServiceAccessibilityMaskAvailable) {
    int step_count = (int) health_service_sum_today(metric);
    static char buf[16];
    snprintf(buf, sizeof(buf), "\U0001F49C %d", step_count);
    text_layer_set_text(s_steps_layer, buf);
  } else {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Data unavailable");
  }
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  if ((units_changed & MINUTE_UNIT) != 0) {
    update_time();
  }

  if ((units_changed & DAY_UNIT) != 0) {
    update_date();
  }
}

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

static void canvas_update_proc(Layer *layer, GContext *ctx) {
  GRect rect_bounds = GRect(60, 110, 60, 25);
  graphics_context_set_fill_color(ctx, GColorDarkCandyAppleRed);

  int corner_radius = 5;
  graphics_fill_rect(ctx, rect_bounds, corner_radius, GCornersAll);
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
}

static void main_window_unload(Window *window) {
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_date_layer);
  text_layer_destroy(s_steps_layer);
  layer_destroy(s_canvas_layer);
}


static void init() {
  // Create main Window element and assign to pointer
  s_main_window = window_create();

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  window_set_background_color(s_main_window, GColorSunsetOrange);

  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);

  // Make sure the time is displayed from the start
  update_time();
  update_date();
  update_step_count();

  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT | DAY_UNIT, tick_handler);

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