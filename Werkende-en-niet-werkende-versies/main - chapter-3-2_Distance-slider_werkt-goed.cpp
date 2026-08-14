
#include <Arduino.h>


#include <lvgl.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

// Install Adafruit Unified Sensor and Adafruit SI7021 Library (was BME280 Library)
#include <Wire.h>
#include <Adafruit_VL6180X.h>     //Lib for dist. sensor

// Settings voor de I2C-communicatie-pinnen
#define I2C_SDA 27
#define I2C_SCL 22

// Settings voor de distance-bar
#define DISTANCE_BAR_MIN 0
#define DISTANCE_BAR_MAX 200

// Settings voor de Touchscreen pins
#define XPT2046_IRQ 36   // T_IRQ
#define XPT2046_MOSI 32  // T_DIN
#define XPT2046_MISO 39  // T_OUT
#define XPT2046_CLK 25   // T_CLK
#define XPT2046_CS 33    // T_CS

// Settings voor de screen dimensions en memory
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320
#define DRAW_BUF_SIZE (SCREEN_WIDTH * SCREEN_HEIGHT / 10 * (LV_COLOR_DEPTH / 8))


// En de globale variabelen

//  Voor de sensor
Adafruit_VL6180X sensor = Adafruit_VL6180X();
//  en het touchscreen
SPIClass touchscreenSPI = SPIClass(VSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);
uint32_t draw_buf[DRAW_BUF_SIZE / 4];

//  De distance value text.
static lv_obj_t * text_label_distance_value;
// Touchscreen coordinates: (x, y) and pressure (z)
int x, y, z;


// If logging is enabled, it will inform the user about what is happening in the library
void log_print(lv_log_level_t level, const char * buf) {
  LV_UNUSED(level);
  Serial.println(buf);
  Serial.flush();
}

// Get the Touchscreen data
void touchscreen_read(lv_indev_t * indev, lv_indev_data_t * data) { 
  // Checks if Touchscreen was touched, and prints X, Y and Pressure (Z)
  if(touchscreen.tirqTouched() && touchscreen.touched()) {
    // Get Touchscreen points
    TS_Point p = touchscreen.getPoint();

    // Advanced Touchscreen calibration, LEARN MORE » https://RandomNerdTutorials.com/touchscreen-calibration/
    float alpha_x, beta_x, alpha_y, beta_y, delta_x, delta_y;

    // REPLACE WITH YOUR OWN CALIBRATION VALUES » https://RandomNerdTutorials.com/touchscreen-calibration/
    alpha_x = -0.000;
    beta_x = 0.088;
    delta_x = -24.3;
    alpha_y = 0.066;
    beta_y = 0.000;
    delta_y = -14.9;

    x = alpha_y * p.x + beta_y * p.y + delta_y;
    // clamp x between 0 and SCREEN_WIDTH - 1
    x = max(0, x);
    x = min(SCREEN_WIDTH - 1, x);

    y = alpha_x * p.x + beta_x * p.y + delta_x;
    // clamp y between 0 and SCREEN_HEIGHT - 1
    y = max(0, y);
    y = min(SCREEN_HEIGHT - 1, y);

    z = p.z;

    data->state = LV_INDEV_STATE_PRESSED;

    // Set the coordinates
    data->point.x = x;
    data->point.y = y;
    
  }
  else {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

// Set the distance value in the bar and text label
static void set_distance(void * bar, int32_t v) {
  // Get the distance from sensor
  
  uint8_t sensor_distance = sensor.readRange();
  if(sensor_distance <= 45.0) {
    lv_obj_set_style_text_color((lv_obj_t*) text_label_distance_value, lv_palette_main(LV_PALETTE_RED), 0);
  }
  else if(sensor_distance > 45.0 && sensor_distance <= 155.0) {
    lv_obj_set_style_text_color((lv_obj_t*) text_label_distance_value, lv_palette_main(LV_PALETTE_GREEN), 0);
  }
  else {
    lv_obj_set_style_text_color((lv_obj_t*) text_label_distance_value, lv_palette_main(LV_PALETTE_RED), 0);
  }
  
  lv_bar_set_value((lv_obj_t*) bar, sensor_distance, LV_ANIM_ON);

  String sensor_distance_text = String(int(float(sensor_distance)/10));
  lv_label_set_text((lv_obj_t*) text_label_distance_value, sensor_distance_text.c_str());
  Serial.print("Distance: ");
  Serial.println(sensor_distance_text);
}

void lv_create_main_gui(void) {
  // Create a text label "DISTANCE"
  lv_obj_t * text_label_distance = lv_label_create(lv_screen_active());
  lv_label_set_text(text_label_distance, "DISTANCE");
  lv_obj_set_width(text_label_distance, 150);
  lv_obj_set_style_text_align(text_label_distance, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(text_label_distance, LV_ALIGN_CENTER, 0, -20);
  lv_obj_set_style_text_font(text_label_distance, &lv_font_montserrat_26,0);


  // Create a text label in font size 36 to display the latest distance reading
  text_label_distance_value = lv_label_create(lv_screen_active());   
  lv_label_set_text(text_label_distance_value, "--.--");
  lv_obj_set_style_text_align(text_label_distance_value, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(text_label_distance_value, LV_ALIGN_CENTER, 0, 10);

  static lv_style_t style_distance;
  lv_style_init(&style_distance);
  lv_style_set_text_font(&style_distance, &lv_font_montserrat_36);
  lv_obj_add_style(text_label_distance_value, &style_distance, 0);
  lv_obj_add_style(text_label_distance, &style_distance, 0);


  // Create a vertical bar aligned on the left side to display the temperature value
  static lv_style_t style_indic_distance;
  lv_style_init(&style_indic_distance);
  lv_style_set_bg_opa(&style_indic_distance, LV_OPA_COVER);
  lv_style_set_bg_color(&style_indic_distance, lv_palette_main(LV_PALETTE_RED));
  // lv_style_set_bg_grad_color(&style_indic_distance, lv_palette_main(LV_PALETTE_LIGHT_BLUE));
  // lv_style_set_bg_grad_dir(&style_indic_distance, LV_GRAD_DIR_HOR);

  lv_obj_t * bar = lv_bar_create(lv_screen_active());
  lv_obj_add_style(bar, &style_indic_distance, LV_PART_INDICATOR);
  lv_obj_set_size(bar, 250, 20);
  lv_obj_align(bar, LV_ALIGN_BOTTOM_MID, 0, -50);
  lv_bar_set_range(bar, DISTANCE_BAR_MIN, DISTANCE_BAR_MAX);
  
  // Create an animation to update the bar and text label with the
  // latest temperature value every 10 seconds
  lv_anim_t a_distance;
  lv_anim_init(&a_distance);
  lv_anim_set_exec_cb(&a_distance, set_distance);
  lv_anim_set_duration(&a_distance, 100);
  lv_anim_set_playback_duration(&a_distance, 100);
  lv_anim_set_var(&a_distance, bar);
  lv_anim_set_values(&a_distance, DISTANCE_BAR_MIN, DISTANCE_BAR_MAX);
  lv_anim_set_repeat_count(&a_distance, LV_ANIM_REPEAT_INFINITE);
  lv_anim_start(&a_distance);

}

void setup() {
  String LVGL_Arduino = String("LVGL Library Version: ") + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();
  Serial.begin(115200);
  while (!Serial);
  Serial.println("connected to Serial monitor");
  Serial.println(LVGL_Arduino);

  // I2C7021.begin(I2C_SDA, I2C_SCL, 100000);
  Wire.begin(I2C_SDA, I2C_SCL);

  Serial.println("VL6180X test wordt gestart...");

  if (!sensor.begin()) {
    Serial.println("Fout: VL6180X sensor niet gevonden! Controleer de bedrading en I2C pinnen.");
    while (1);
  }

  Serial.println("VL6180X gevonden! Starten met meten...");

  if (!sensor.begin()) {
    Serial.println("Fout: VL6180X sensor niet gevonden! Controleer de bedrading en I2C pinnen.");
    while (1);
  }

  Serial.println("VL6180X gevonden! Starten met meten...");
  
  
  // Start LVGL
  lv_init();
  // Register print function for debugging
  lv_log_register_print_cb(log_print);

  // Start the SPI for the touchscreen and init the touchscreen
  touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  touchscreen.begin(touchscreenSPI);
  // Set the Touchscreen rotation in landscape mode
  // Note: in some displays, the touchscreen might be upside down, so you might need to set the rotation to 0: touchscreen.setRotation(0);
  touchscreen.setRotation(2);

  // Create a display object
  lv_display_t * disp;
  // Initialize the TFT display using the TFT_eSPI library
  disp = lv_tft_espi_create(SCREEN_WIDTH, SCREEN_HEIGHT, draw_buf, sizeof(draw_buf));
  lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_270);
  
  // Initialize an LVGL input device object (Touchscreen)
  lv_indev_t * indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  // Set the callback function to read Touchscreen input
  lv_indev_set_read_cb(indev, touchscreen_read);

  // Function to draw the GUI
  lv_create_main_gui();
}

void loop() {
  lv_task_handler();  // let the GUI do its work
  lv_tick_inc(5);     // tell LVGL how much time has passed
  delay(5);           // let this time pass
}