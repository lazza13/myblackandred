#include <pebble.h>

static Window *s_main_window;
static TextLayer *s_time_layer;
static TextLayer *s_minute_layer;
static TextLayer *s_date_layer;
static int s_battery_level;
static Layer *s_battery_layer;
static bool s_is_charging;
static bool s_blue;
static BitmapLayer *s_bt_icon_layer;
static GBitmap *s_bt_icon_bitmap;


static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);

  // Write the current hours and minutes into a buffer
  static char s_buffer[8];
  strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ?
                                          "%H" : "%I", tick_time);

  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, s_buffer);
	
	// Write the current hours and minutes into a buffer
  static char s_buffer2[8];
  strftime(s_buffer2, sizeof(s_buffer2),"%M", tick_time);

  // Display this time on the TextLayer
  text_layer_set_text(s_minute_layer, s_buffer2);
	
	// Copy date into buffer from tm structure
	static char date_buffer[16];
	strftime(date_buffer, sizeof(date_buffer), "%a %d %b", tick_time);

	// Show the date
	text_layer_set_text(s_date_layer, date_buffer);	
}

static void battery_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  // Find the width of the bar
  int width = (int)(float)(((float)s_battery_level / 100.0F) * bounds.size.w);

  // Draw the background
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  // Draw the bar
  graphics_context_set_fill_color(ctx, (s_is_charging? GColorGreen : (s_battery_level <= 20 ? GColorRed: (s_blue?GColorBlue:GColorWhite))));
  graphics_fill_rect(ctx, GRect(0, 0, width, 5), 0, GCornerNone);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

static void bluetooth_callback(bool connected) {
  // Show icon if disconnected
  layer_set_hidden(bitmap_layer_get_layer(s_bt_icon_layer), connected);
	s_blue = connected;
  if(!connected) {
    // Issue a vibrating alert
    vibes_double_pulse();
  }
}

static void main_window_load(Window *window) {
  // Get information about the Window
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // Create the TextLayer with specific bounds
  s_time_layer = text_layer_create(
      GRect(5, 3, bounds.size.w-4, 75));
	
	s_minute_layer = text_layer_create(
      GRect(5, 53, bounds.size.w-4, 75));
	
	GFont custom_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_MYFONT_72));
	GFont custom_font2 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_MYFONT_15));
	
  // Improve the layout to be more like a watchface
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorRed);
  text_layer_set_text(s_time_layer, "00");
  text_layer_set_font(s_time_layer, custom_font);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
	
	// Improve the layout to be more like a watchface
  text_layer_set_background_color(s_minute_layer, GColorClear);
  text_layer_set_text_color(s_minute_layer, GColorWhite);
  text_layer_set_text(s_minute_layer, "00");
  text_layer_set_font(s_minute_layer, custom_font);
  text_layer_set_text_alignment(s_minute_layer, GTextAlignmentCenter);

  // Add it as a child layer to the Window's root layer
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
	layer_add_child(window_layer, text_layer_get_layer(s_minute_layer));
	
	
	// Create date TextLayer
	s_date_layer = text_layer_create(GRect(0, 140, bounds.size.w, 30));
	text_layer_set_text_color(s_date_layer, GColorWhite);
	text_layer_set_background_color(s_date_layer, GColorClear);
	text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
	text_layer_set_font(s_date_layer, custom_font2);

	// Add to Window
	layer_add_child(window_layer, text_layer_get_layer(s_date_layer));
	
	
	// Create battery meter Layer
	s_battery_layer = layer_create(GRect(14, 54, bounds.size.w, 2));
	layer_set_update_proc(window_layer, battery_update_proc);
	
	
	// Create the Bluetooth icon GBitmap
	s_bt_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_NO_BT);

	// Create the BitmapLayer to display the GBitmap
	s_bt_icon_layer = bitmap_layer_create(GRect(bounds.size.w -30 , bounds.size.h-30, 30, 30));
	bitmap_layer_set_bitmap(s_bt_icon_layer, s_bt_icon_bitmap);
	layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_bt_icon_layer));

	// Add to Window
	layer_add_child(window_get_root_layer(window), s_battery_layer);
}

static void main_window_unload(Window *window) {
  // Destroy TextLayer
  text_layer_destroy(s_time_layer);
	text_layer_destroy(s_minute_layer);
	text_layer_destroy(s_date_layer);
	layer_destroy(s_battery_layer);
	gbitmap_destroy(s_bt_icon_bitmap);
	bitmap_layer_destroy(s_bt_icon_layer);
}

static void battery_callback(BatteryChargeState state) {
  // Record the new battery level
  s_battery_level = state.charge_percent;
	s_is_charging = state.is_charging;
	// Update meter
  layer_mark_dirty(s_battery_layer);
}

static void init() {
  // Create main Window element and assign to pointer
  s_main_window = window_create();

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
	
	// Register for Bluetooth connection updates
	connection_service_subscribe((ConnectionHandlers) {
  .pebble_app_connection_handler = bluetooth_callback
	});
	
  // Show the Window on the watch, with animated=true
	window_set_background_color(s_main_window, GColorBlack);
  window_stack_push(s_main_window, true);

  // Make sure the time is displayed from the start
  update_time();

  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
	
	// Register for battery level updates
	battery_state_service_subscribe(battery_callback);
	
	// Ensure battery level is displayed from the start
	battery_callback(battery_state_service_peek());
	
	// Show the correct state of the BT connection from the start
  bluetooth_callback(connection_service_peek_pebble_app_connection());
	

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