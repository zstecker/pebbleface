#pragma once
#include <pebble.h>

#define SETTINGS_KEY 1

typedef struct ClaySettings {
  GColor BackgroundColor;
  GColor SecondaryColor;
  GColor ClockTextColor;
  GColor DateTextColor;
  GColor StepsTextColor;
  bool ShowSteps;
} __attribute__((__packed__)) ClaySettings;

static void init_default_settings();
static void load_settings();
static void save_settings();
static void update_display();
static void update_time();
static void update_date();
static void update_step_count();
static void tick_handler(struct tm *tick_time, TimeUnits units_changed);
static void health_handler(HealthEventType event, void *context);
static void canvas_update_proc(Layer *layer, GContext *ctx);
static void main_window_load(Window *window);
static void main_window_unload(Window *window);
static void inbox_received_handler(DictionaryIterator *iter, void *context);
static void init();
static void deinit();