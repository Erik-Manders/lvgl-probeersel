
#include <Arduino.h>

#include <lvgl.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include "ui/ui.h" // Geïmporteerde SquareLine bestanden

// Touchscreen pinnen voor CYD
#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33

SPIClass mySpi = SPIClass(VSPI);
XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ);
TFT_eSPI tft = TFT_eSPI();

static const uint16_t screenWidth  = 320;
static const uint16_t screenHeight = 240;
static lv_color_t buf[screenWidth * 10];

// Display Flush Callback (stuurt pixels naar TFT_eSPI)
void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors(reinterpret_cast<uint16_t *>(px_map), w * h, true);
    tft.endWrite();
    lv_display_flush_ready(disp);
}

// Touchpad Read Callback
void my_touchpad_read(lv_indev_t *indev_driver, lv_indev_data_t *data) {
    if (ts.touched()) {
        TS_Point p = ts.getPoint();
        // Kalibratie en schalen voor het CYD scherm
        data->point.x = map(p.x, 200, 3700, 0, screenWidth);
        data->point.y = map(p.y, 240, 3800, 0, screenHeight);
        data->state = LV_INDEV_STATE_PR;
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}

void setup() {
    Serial.begin(115200);

    // Initialiseer TFT & Touch
    tft.begin();
    tft.setRotation(1); // Lanschap modus
    mySpi.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
    ts.begin(mySpi);
    ts.setRotation(1);

    // Initialiseer LVGL
    lv_init();
    lv_display_t *display = lv_display_create(screenWidth, screenHeight);
    lv_display_set_flush_cb(display, my_disp_flush);
    lv_display_set_buffers(display, buf, NULL, sizeof(buf), LV_DISPLAY_RENDER_MODE_PARTIAL);

    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, my_touchpad_read);

    // Start de SquareLine UI!
    ui_init();
}

void loop() {
    lv_timer_handler(); // Houdt LVGL en de UI actief
    delay(10);
}

// Event-functie aangeroepen vanuit SquareLine UI (indien gedefinieerd)
// void btn_click_action(lv_event_t * e) {
//     Serial.println("Knop geklikt op het CYD scherm!");
// nog een andere regel
// extra regel
// }