#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"


#define MY_UUID { 0x85, 0xDB, 0xE8, 0x9F, 0x2C, 0x39, 0x42, 0x0F, 0x8C, 0x2B, 0x6F, 0x23, 0x81, 0x00, 0xC7, 0x28 }
PBL_APP_INFO(MY_UUID,
             "Silent Hill", "Alexander Maricich",
             1, 0, /* App version */
             DEFAULT_MENU_ICON,
             APP_INFO_WATCH_FACE);

Window window;

// BmpContainer background_container;

RotBmpContainer order_outer_container;
RotBmpContainer order_inner_container;
InverterLayer inverter_layer;

// RotBmpPairContainer img_container;

#define MAX(a,b) (((a)>(b))?(a):(b))

static int32_t integer_sqrt(int32_t x) {
  if (x < 0) {
    ////    PBL_LOG(LOG_LEVEL_ERROR, "Looking for sqrt of negative number");
    return 0;
  }

  int32_t last_res = 0;
  int32_t res = (x + 1)/2;
  while (last_res != res) {
    last_res = res;
    res = (last_res + x / last_res) / 2;
  }
  return res;
}

void handle_deinit(AppContextRef ctx) {
  (void)ctx;

  rotbmp_deinit_container(&order_outer_container);
  rotbmp_deinit_container(&order_inner_container);
}

void set_hand_angle(RotBmpContainer *hand_image_container, unsigned int hand_angle) {

  signed short x_fudge = 0;
  signed short y_fudge = 0;


  hand_image_container->layer.rotation =  TRIG_MAX_ANGLE * hand_angle / 360;

  //
  // Due to rounding/centre of rotation point/other issues of fitting
  // square pixels into round holes by the time hands get to 6 and 9
  // o'clock there's off-by-one pixel errors.
  //
  // The `x_fudge` and `y_fudge` values enable us to ensure the hands
  // look centred on the minute marks at those points. (This could
  // probably be improved for intermediate marks also but they're not
  // as noticable.)
  //
  // I think ideally we'd only ever calculate the rotation between
  // 0-90 degrees and then rotate again by 90 or 180 degrees to
  // eliminate the error.
  //
  if (hand_angle == 0)
  {
    x_fudge = 0;
  }
  else if (hand_angle == 90)
  {
    y_fudge = 0;
  }
  else if (hand_angle == 180) {
    x_fudge = 0;
  } else if (hand_angle == 270) {
    y_fudge = 0;
  }

  // (144 = screen width, 168 = screen height)
  hand_image_container->layer.layer.frame.origin.x = (144/2) - (hand_image_container->layer.layer.frame.size.w/2) + x_fudge;
  hand_image_container->layer.layer.frame.origin.y = (168/2) - (hand_image_container->layer.layer.frame.size.h/2) + y_fudge;

  layer_mark_dirty(&hand_image_container->layer.layer);
}

void rot_bitmap_set_src_ic(RotBitmapLayer *image, GPoint ic) {
  image->src_ic = ic;

  // adjust the frame so the whole image will still be visible
  const int32_t horiz = MAX(ic.x, abs(image->bitmap->bounds.size.w - ic.x));
  const int32_t vert = MAX(ic.y, abs(image->bitmap->bounds.size.h - ic.y));

  GRect r = layer_get_frame(&image->layer);
  //// const int32_t new_dist = integer_sqrt(horiz*horiz + vert*vert) * 2;
  const int32_t new_dist = (integer_sqrt(horiz*horiz + vert*vert) * 2) + 1; //// Fudge to deal with non-even dimensions--to ensure right-most and bottom-most edges aren't cut off.

  r.size.w = new_dist;
  r.size.h = new_dist;
  layer_set_frame(&image->layer, r);

  r.origin = GPoint(0, 0);
  ////layer_set_bounds(&image->layer, r);
  image->layer.bounds = r;

  image->dest_ic = GPoint(new_dist / 2, new_dist / 2);

  layer_mark_dirty(&(image->layer));
}


void handle_tick(AppContextRef ctx, PebbleTickEvent* event) {
  PblTm t;
  get_time(&t);

  // set_hand_angle(&order_inner_container, ((t.tm_hour % 12) * 30) + (t.tm_min/2)); // ((((t.tm_hour % 12) * 6) + (t.tm_min / 10))) / (12 * 6));

  // set_hand_angle(&order_outer_container, t.tm_min * 6);
  set_hand_angle(&order_outer_container, t.tm_min * 6);

  set_hand_angle(&order_inner_container, t.tm_hour * 6);
}

void handle_init(AppContextRef ctx) {
  (void)ctx;

  window_init(&window, "Silent Hill");
  window_stack_push(&window, true /* Animated */);
  // window_set_background_color(&window, GColorBlack);

  resource_init_current_app(&APP_RESOURCES);

  // Set up the background image
  // bmp_init_container(RESOURCE_ID_BACKGROUND, &background_container);
  // layer_add_child(&window.layer, &background_container.layer.layer);

  // Set up the outer ring
  rotbmp_init_container(RESOURCE_ID_ORDER_OUTER, &order_outer_container);
  // rot_bitmap_set_src_ic(&order_outer_container.layer, GPoint(74, 83)); //70, 81
    rot_bitmap_set_src_ic(&order_outer_container.layer, GPoint(72, 84)); //70, 81
  order_outer_container.layer.compositing_mode = GCompOpClear;
  layer_add_child(&window.layer, &order_outer_container.layer.layer);

  // Set up the inner ring. rotate at a different axsis to center the circles
  rotbmp_init_container(RESOURCE_ID_ORDER_INNER, &order_inner_container);
  rot_bitmap_set_src_ic(&order_inner_container.layer, GPoint(72, 84)); //70, 81
  order_inner_container.layer.compositing_mode = GCompOpClear;
  layer_add_child(&window.layer, &order_inner_container.layer.layer);

  layer_init(&inverter_layer.layer, GRect(0,0,144,168));
  layer_add_child(&window.layer, &inverter_layer.layer);

  handle_tick(ctx, NULL);

}

void pbl_main(void *params) {
  PebbleAppHandlers handlers = {
    .init_handler = &handle_init,
    .deinit_handler = &handle_deinit,
    .tick_info = {
      .tick_handler = &handle_tick,
      .tick_units = SECOND_UNIT
    }
  };
  app_event_loop(params, &handlers);
}
