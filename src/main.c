//LIBRARIES
#include <pebble.h>
#include "dithered_rects.h"
#include <math.h>
#include "drawarc.h"  
  
//SET VARIABLES  
Window *my_window;
Layer * base_layer;
Layer * minute_layer;
Layer * circle_and_crosshair_layer;
static TextLayer *s_timeText;

//BASE LAYER - DITHERED IMAGE FOR OG PEBBLE
void draw_base_layer(Layer *cell_layer, GContext *ctx){
  draw_dithered_rect(ctx,GRect(0,0,144,168), GColorBlack,GColorWhite, DITHER_75_PERCENT);
 }

void draw_minute_layer(Layer *cell_layer, GContext *ctx){
  //GET A TM STRUCTURE
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  int mins =tick_time->tm_min;
  int dither = 0;
  #ifdef PBL_COLOR //Pebble Time code
    dither = 0;
    graphics_context_set_stroke_color(ctx,GColorDarkGray);
  #else
    dither = 1;
  #endif 
    if (mins >0 && mins <= 15){
      graphics_draw_arc(ctx,GPoint(72,84),111,111,270,mins*6+270,dither);
    } else if (mins >15){
      graphics_draw_arc(ctx,GPoint(72,84),111,111,270,360,dither);
      graphics_draw_arc(ctx,GPoint(72,84),111,111,0,(mins-15)*6,dither);
    }
}

void draw_circle_and_crosshair_layer(Layer * cell_layer, GContext *ctx){
  //CROSSHAIRS
  graphics_context_set_stroke_color(ctx,GColorBlack);
  graphics_draw_line(ctx,GPoint(72,0),GPoint(72,168));
  graphics_draw_line(ctx,GPoint(0,84),GPoint(144,84));
  
  //CIRCLES
  graphics_context_set_stroke_color(ctx,GColorWhite);
  #ifdef PBL_COLOR
    //FOR PEBBLE TIME, WE CAN SET THE STROKE WIDTH TO 3PX TO ELIMINATE PIXELS THAT DON'T GET FILLED
    //WHEN DRAWING AT 3 DIFFERENT RADII
    graphics_context_set_stroke_width(ctx,3);
    graphics_draw_circle(ctx,GPoint(72,84),52);
    graphics_draw_circle(ctx,GPoint(72,84),62);
  #else
    //FOR PEBBLE ORIGINAL AND STEEL, WE DRAW INDIVIDUAL CIRCLES TO GET THE 3 PIXEL WIDTH
    //THIS COULD BE ACCOMPLISHED BY DRAWING FILLED CIRCLES, THEN PLACING A DITHERED CIRCLE IN THE CENTER, BUT 
    //PROBABLY ISN'T WORTH THE EXTRA OVERHEAD FOR A FEW PIXELS.
    graphics_draw_circle(ctx,GPoint(72,84),52);
    graphics_draw_circle(ctx,GPoint(72,84),62);
    graphics_draw_circle(ctx,GPoint(72,84),51);
    graphics_draw_circle(ctx,GPoint(72,84),63);
    graphics_draw_circle(ctx,GPoint(72,84),50);
    graphics_draw_circle(ctx,GPoint(72,84),64);
  #endif
}

static void update_time(){
  //GET A TM STRUCTURE
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  
  //CREATE LONG LIVED BUFFERS FOR DATE AND TIME
  static char hourBuffer[] = "  ";
  
  //WRITE THE CURRENT HOURSINTO THE TIME BUFFER
  if(clock_is_24h_style() == true) {
    //USE 24 HOUR FORMAT
    strftime(hourBuffer,sizeof("  "), "%H", tick_time);
  } else {
    //USE 12 HOUR FORMAT
    strftime(hourBuffer,sizeof("  "), "%l", tick_time);
  }
  //UPDATE TEXT LAYER WITH TIME
  text_layer_set_text(s_timeText, hourBuffer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed){
  //EVENT HANDLER FOR TIME CHANGES
  update_time();
}

void handle_init(void) {
  //CREATE WINDOW
  my_window = window_create();
  
  //SET BACKGROUND LAYER
  #ifdef PBL_COLOR  
    //GColorLightGray BACKGROUND FOR PEBBLE TIME
    window_set_background_color(my_window, GColorLightGray);		
	#else  
    //DITHERED BACKGROUND FOR ORIGINAL GENERATION PEBBLE
    base_layer = layer_create(GRect(0,0,144,168));
	  layer_set_update_proc(base_layer, draw_base_layer);
	  layer_add_child(window_get_root_layer(my_window), base_layer);
  #endif 
  
  //CREATE MINUTE LAYER
   minute_layer = layer_create(GRect(0,0,144,168));
	layer_set_update_proc(minute_layer, draw_minute_layer);
	
	layer_add_child(window_get_root_layer(my_window), minute_layer);
  //TODO: WRITE THIS CODE
  
  //CREATE CIRCLE AND CROSSHAIR LAYER
  circle_and_crosshair_layer = layer_create(GRect(0,0,144,168));
  layer_set_update_proc(circle_and_crosshair_layer, draw_circle_and_crosshair_layer);
  layer_add_child(window_get_root_layer(my_window), circle_and_crosshair_layer);
    
  //CREATE TIME TEXT LAYER
  s_timeText = text_layer_create(GRect(1, 50, 140, 108));
	text_layer_set_background_color(s_timeText, GColorClear);
  text_layer_set_text_color(s_timeText, GColorBlack);
	text_layer_set_text(s_timeText, "");
	text_layer_set_font(s_timeText, fonts_get_system_font(FONT_KEY_ROBOTO_BOLD_SUBSET_49));
	text_layer_set_text_alignment(s_timeText, GTextAlignmentCenter);
  layer_add_child(window_get_root_layer(my_window), text_layer_get_layer(s_timeText));
  
  //PUSH EVERYTHING TO THE WINDOW
  window_stack_push(my_window, true);
  
  //SUBSCRIBE TO TIMER SERVICE
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
}

void handle_deinit(void) {
  #ifdef PBL_COLOR  //destroy layer only if running aplite
    layer_destroy(base_layer);
  #endif
  layer_destroy(circle_and_crosshair_layer);
  layer_destroy(minute_layer);
  text_layer_destroy(s_timeText);
  window_destroy(my_window);
}

int main(void) {
  handle_init();
  app_event_loop();
  handle_deinit();
}
