
#include <Arduino.h>

#include <lvgl.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include "ui/ui.h" // Geïmporteerde SquareLine bestanden

// Touchscreen pinnen voor CYD
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33
#define CYD_RED_LED 4

SPIClass mySpi = SPIClass(VSPI);
// Poll the controller instead of depending on the CYD touch IRQ line.
XPT2046_Touchscreen ts(XPT2046_CS);
TFT_eSPI tft = TFT_eSPI();

static const uint16_t screenWidth  = 320;
static const uint16_t screenHeight = 240;
static const uint16_t redButtonLeft = 196;
static const uint16_t redButtonRight = 258;
static const uint16_t redButtonTop = 101;
static const uint16_t redButtonBottom = 144;
alignas(4) static lv_color_t buf[screenWidth * 10];
static bool redLedOn = false;
static bool touchWasDown = false;
static uint32_t lastTouchPrint = 0;
static TS_Point touchPoint;

void red_button_event(lv_event_t *event) {
    (void)event;
    redLedOn = !redLedOn;
    Serial.printf("Rode knop klik, LED=%s\n", redLedOn ? "ON" : "OFF");
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

// Touchpad Read Callback
void my_touchpad_read(lv_indev_t *indev_driver, lv_indev_data_t *data) {
    (void)indev_driver;

    if (touchWasDown) {
        // Kalibratie en schalen voor het CYD scherm
        data->point.x = map(touchPoint.x, 200, 3700, 0, screenWidth);
        data->point.y = map(touchPoint.y, 240, 3800, 0, screenHeight);
        data->state = LV_INDEV_STATE_PR;
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("Touchscreen test gestart");
    pinMode(CYD_RED_LED, OUTPUT);
    digitalWrite(CYD_RED_LED, HIGH);

    // Initialiseer TFT & Touch
    tft.begin();
    tft.setRotation(1); // Lanschap modus
    ts.begin(mySpi);
    // XPT2046_Touchscreen::begin() resets the SPI bus to default VSPI pins.
    // Restore the CYD touch pins after initializing the driver.
    mySpi.end();
    mySpi.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
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
    const bool wasTouching = touchWasDown;
    const bool touched = ts.touched();
    if (touched) {
        touchPoint = ts.getPoint();
        const uint32_t now = millis();
        const int16_t screenX = map(touchPoint.x, 200, 3700, 0, screenWidth);
        const int16_t screenY = map(touchPoint.y, 240, 3800, 0, screenHeight);
        if (!touchWasDown || now - lastTouchPrint >= 100) {
            Serial.printf("Touch raw=(%d, %d), screen=(%d, %d)\n",
                          touchPoint.x, touchPoint.y, screenX, screenY);
            lastTouchPrint = now;
        }
        if (!wasTouching && screenX >= redButtonLeft && screenX <= redButtonRight &&
            screenY >= redButtonTop && screenY <= redButtonBottom) {
            red_button_event(NULL);
        }
        touchWasDown = true;
    } else if (touchWasDown) {
        Serial.println("Touch released");
        touchWasDown = false;
    }

    lv_timer_handler(); // Houdt LVGL en de UI actief
    delay(10);
}