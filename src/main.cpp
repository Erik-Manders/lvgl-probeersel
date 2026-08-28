
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
#define CYD_RED_LED 4

SPIClass mySpi = SPIClass(VSPI);
XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ);
TFT_eSPI tft = TFT_eSPI();

static const uint16_t screenWidth  = 320;
static const uint16_t screenHeight = 240;
static const int16_t touchXMin = 200;
static const int16_t touchXMax = 3700;
static const int16_t touchYMin = 240;
static const int16_t touchYMax = 3800;
alignas(4) static lv_color_t buf[screenWidth * 10];
static bool redLedOn = false;
static lv_indev_t *touchIndev = nullptr;

// SquareLine's generated C code calls this function when Button3 is clicked.
extern "C" void btn_click_action(lv_event_t *event) {
    (void)event;
    redLedOn = !redLedOn;
    // The CYD red LED uses inverted logic: LOW is on, HIGH is off.
    digitalWrite(CYD_RED_LED, redLedOn ? LOW : HIGH);
}

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

// LVGL calls this function to obtain the current touchscreen state.
void my_touchpad_read(lv_indev_t *indev_driver, lv_indev_data_t *data) {
    (void)indev_driver;

    // The XPT2046 interrupt sets tirqTouched() when the display is touched.
    if (!ts.tirqTouched() || !ts.touched()) {
        data->state = LV_INDEV_STATE_RELEASED;
        return;
    }

    const TS_Point point = ts.getPoint();
    data->point.x = constrain(
        map(point.x, touchXMin, touchXMax, 0, screenWidth - 1),
        0, screenWidth - 1);
    data->point.y = constrain(
        map(point.y, touchYMin, touchYMax, 0, screenHeight - 1),
        0, screenHeight - 1);
    data->state = LV_INDEV_STATE_PRESSED;
}

void setup() {
    Serial.begin(115200);
    pinMode(CYD_RED_LED, OUTPUT);
    digitalWrite(CYD_RED_LED, HIGH);

    // Initialiseer TFT & Touch
    tft.begin();
    tft.setRotation(1); // Lanschap modus
    mySpi.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
    ts.begin(mySpi);
    ts.setRotation(1);

    // Initialiseer LVGL
    lv_init();
    lv_display_t *display = lv_display_create(screenWidth, screenHeight);
    lv_display_set_default(display);
    lv_display_set_flush_cb(display, my_disp_flush);
    lv_display_set_buffers(display, buf, NULL, sizeof(buf), LV_DISPLAY_RENDER_MODE_PARTIAL);

    lv_indev_t *indev = lv_indev_create();
    touchIndev = indev;
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_display(indev, display);
    lv_indev_set_read_cb(indev, my_touchpad_read);

    // Start de SquareLine UI!
    ui_init();
 
}

void loop() {
    // Touch IRQ -> XPT2046 wake flag -> LVGL input callback -> SquareLine event.
    lv_indev_read(touchIndev);
    lv_timer_handler();
    delay(10);
}