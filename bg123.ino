#include <SmartWiFi.h>
#include <CertManager.h>
#include <GetRate.h>
#include <esp_task_wdt.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
//Define pins
#define I2C_A_SDA 8 // GPIO8
#define I2C_A_SCL 9 // GPIO9
// OLED parameters
#define SCREEN_WIDTH 128     // OLED display width, in pixels
#define SCREEN_HEIGHT 64     // OLED display height, in pixels
#define OLED_RESET -1        // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C  // Change if required
#define ROTATION 0           // Rotates text on OLED 1=90 degrees, 2=180 degrees
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
// Global pointers to delay construction
SmartWiFi* wifi = nullptr;
CertManager* certManager = nullptr;
GetRate* rateFetcher = nullptr;

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("[Main] Serial initialized");

    // Configure watchdog timer (8s timeout, no panic)
    Serial.println("[Main] Configuring watchdog...");
    esp_task_wdt_config_t wdt_config = {
        .timeout_ms = 8000, // 8 seconds
        .idle_core_mask = 0x1, // Monitor core 0 only
        .trigger_panic = false // Don't panic on timeout
    };
    esp_err_t wdt_err = esp_task_wdt_init(&wdt_config);
    if (wdt_err != ESP_OK) {
        Serial.println("[Main] Watchdog config failed: " + String(wdt_err) + ", continuing...");
    } else {
        Serial.println("[Main] Watchdog configured");
    }

    // Initialize SmartWiFi
    Serial.println("[Main] Constructing SmartWiFi...");
    wifi = new SmartWiFi();
    if (!wifi) {
        Serial.println("[Main] Failed to allocate SmartWiFi");
        return;
    }
    Serial.println("[Main] Initializing WiFi...");
    if (!wifi->begin()) {
        Serial.println("[Main] WiFi initialization failed");
        return;
    }
    Serial.println("[Main] WiFi initialized");

    // Verify WiFi connection
    Serial.println("[Main] Verifying WiFi connection...");
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[Main] WiFi not connected, aborting setup");
        return;
    }

    // Initialize CertManager
    Serial.println("[Main] Constructing CertManager...");
    certManager = new CertManager();
    if (!certManager) {
        Serial.println("[Main] Failed to allocate CertManager");
        return;
    }
    Serial.println("[Main] Setting certificates...");
    if (!certManager->setCertificates()) {
        Serial.println("[Main] Certificate setup failed");
        return;
    }
    Serial.println("[Main] Certificates set");

    // Initialize GetRate
    Serial.println("[Main] Constructing GetRate...");
    rateFetcher = new GetRate(*certManager);
    if (!rateFetcher) {
        Serial.println("[Main] Failed to allocate GetRate");
        return;
    }
    Serial.println("[Main] Initializing rate fetcher...");
    if (!rateFetcher->begin()) {
        Serial.println("[Main] Rate fetcher initialization failed");
        return;
    }
    Serial.println("[Main] Rate fetcher initialized");
    while (!Serial)
    ;
    Wire.begin(I2C_A_SDA, I2C_A_SCL);
    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println(F("Display1 SSD1306 allocation failed"));
        for (;;)
            ;  // Don't proceed, loop forever
    }
    display.display();
    delay(2000);
    display.clearDisplay();
    display.setTextSize(2);             // Normal 1:1 pixel scale
    display.setTextColor(WHITE);        // Draw white text
    display.setCursor(0,0);             // Start at top-left corner
    display.setRotation(ROTATION);      // Set screen rotation
    display.print(" XRP/RLUSD");
}

void loop() {
        display.display();
    delay(2000);
    //display.clearDisplay();
    display.setTextSize(2);             // Normal 1:1 pixel scale
    display.setTextColor(WHITE);        // Draw white text
    display.setCursor(0,0);             // Start at top-left corner
    display.setRotation(ROTATION);      // Set screen rotation
    display.print(" XRP/RLUSD");

    if (wifi && wifi->check()) {
        static unsigned long last_request = 0;
        unsigned long now = millis();
        if (now - last_request >= 60000) {
            Serial.println("[Main] Fetching rate...");
            String rate = rateFetcher->getRate();
            if (rate != "") {
                Serial.println("[Main] XRP/RLUSD Rate: " + rate);
                display.clearDisplay();
                display.setTextSize(2);             // Normal 1:1 pixel scale
                display.setTextColor(WHITE);        // Draw white text
                display.setCursor(0,0);             // Start at top-left corner
                display.setRotation(ROTATION);      // Set screen rotation
                display.print(" XRP/RLUSD");
                display.setTextSize(2);
                display.setCursor(0,25);
                display.print("$ " +rate);
            } else {
                Serial.println("[Main] Rate fetch failed");
            }
            last_request = now;
        }
    } else {
        Serial.println("[Main] WiFi check failed, retrying...");
        delay(5000);
    }

}