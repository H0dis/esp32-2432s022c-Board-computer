#include <Arduino.h>
#include <Wire.h>
#include <LovyanGFX.hpp>

// Definire touchscreen CST816S
#define I2C_TOUCH_ADDRESS 0x15  

// Moduri de afisare
enum ModAfisare {
    MOD_PRINCIPAL,
    MOD_TEMPERATURA_APA,
    MOD_TEMPERATURA_ULEI,
    MOD_PRESIUNE_ULEI
};

// Variabile globale
ModAfisare modCurent = MOD_PRINCIPAL;
unsigned long lastTouchTime = 0;
bool touchRegistered = false;

// Variabile pentru temperaturi si presiune
float temperaturaApa = 0, ultimaTempApa = -1;
float temperaturaUlei = 0, ultimaTempUlei = -1;
float presiuneUlei = 0, ultimaPresiuneUlei = -1;
int directieApa = 1;

// Clasa pentru display ST7789 cu magistrala paralela 8-bit
class LGFX : public lgfx::LGFX_Device {
    lgfx::Panel_ST7789 _panel_instance;
    lgfx::Bus_Parallel8 _bus_instance;

public:
    LGFX(void) {
        auto cfg = _bus_instance.config();
        cfg.freq_write = 25000000;  
        cfg.pin_wr = 4;
        cfg.pin_rd = 2;
        cfg.pin_rs = 16;  
        cfg.pin_d0 = 15;
        cfg.pin_d1 = 13;
        cfg.pin_d2 = 12;
        cfg.pin_d3 = 14;
        cfg.pin_d4 = 27;
        cfg.pin_d5 = 25;
        cfg.pin_d6 = 33;
        cfg.pin_d7 = 32;
        _bus_instance.config(cfg);
        _panel_instance.setBus(&_bus_instance);

        auto cfgPanel = _panel_instance.config();
        cfgPanel.pin_cs = 17;  
        cfgPanel.pin_rst = -1;
        cfgPanel.panel_width = 240;
        cfgPanel.panel_height = 320;
        cfgPanel.offset_x = 0;
        cfgPanel.offset_y = 0;
        cfgPanel.offset_rotation = 0;
        cfgPanel.invert = false;
        cfgPanel.rgb_order = false;
        _panel_instance.config(cfgPanel);

        setPanel(&_panel_instance);
    }
};

// Inițializare display
LGFX tft;

// Touchscreen Init
void touchscreenInit() {
    Wire.beginTransmission(I2C_TOUCH_ADDRESS);
    Wire.write(0xD0);
    Wire.write(0x01);
    Wire.endTransmission();
    delay(100);
}

// Functie verificare atingere
bool touchscreenAtingere() {
    Wire.beginTransmission(I2C_TOUCH_ADDRESS);
    Wire.write(0x02);
    Wire.endTransmission();
    
    Wire.requestFrom(I2C_TOUCH_ADDRESS, 1);
    if (Wire.available()) {
        uint8_t status = Wire.read();
        return status & 0x01;
    }
    return false;
}

// Afisare bara doar daca valoarea s-a schimbat
void desenareBara(int y, float& valoare, float& ultimaValoare, float maxVal, const char* text) {
    if (abs(valoare - ultimaValoare) < 0.5) return;  // Nu actualiza daca diferenta e mica

    int lungimeBara = map(valoare, 0, maxVal, 10, 220);

    // Sterge doar bara veche
    tft.fillRect(10, y, 220, 20, TFT_BLACK);

    // Deseneaza noua bara
    tft.fillRect(10, y, lungimeBara, 20, TFT_ORANGE);

    // Sterge doar textul vechi valorile din capat
    tft.fillRect(250, y - 25, 60, 20, TFT_BLACK);

    // Textul deasupra barei
    tft.setCursor(10, y - 25);
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE);
    tft.print(text);

    // Valoarea numerică în dreapta
    tft.setCursor(250, y - 25);
    tft.printf("%.1f", valoare);

    ultimaValoare = valoare;  // Actualizeaza ultima valoare
}

// Afisare ecrane diferite
void afiseazaEcran(bool fortat = false) {
    static ModAfisare ultimMod = MOD_PRINCIPAL;

    if (modCurent != ultimMod || fortat) {
        tft.fillScreen(TFT_BLACK);
        ultimMod = modCurent;
    }

    if (modCurent == MOD_PRINCIPAL) {
        desenareBara(60, temperaturaApa, ultimaTempApa, 120, "Temperatura Apa");
        desenareBara(140, temperaturaUlei, ultimaTempUlei, 130, "Temperatura Ulei");
        desenareBara(220, presiuneUlei, ultimaPresiuneUlei, 10, "Presiune Ulei(bar)");
    } else if (modCurent == MOD_TEMPERATURA_APA) {
        desenareBara(100, temperaturaApa, ultimaTempApa, 120, "Temperatura Apa");
    } else if (modCurent == MOD_TEMPERATURA_ULEI) {
        desenareBara(100, temperaturaUlei, ultimaTempUlei, 130, "Temperatura Ulei");
    } else if (modCurent == MOD_PRESIUNE_ULEI) {
        desenareBara(100, presiuneUlei, ultimaPresiuneUlei, 10, "Presiune ulei(bar)");
    }
}

void setup() {
    Serial.begin(115200);
    Wire.begin(21, 22);
    touchscreenInit();
    tft.init();
    tft.setRotation(1);
    afiseazaEcran(true);
}

void loop() {
    // Verificare atingere
    if (touchscreenAtingere()) {
        if (!touchRegistered && millis() - lastTouchTime > 1000) {
            touchRegistered = true;
            modCurent = static_cast<ModAfisare>((modCurent + 1) % 4);
            afiseazaEcran(true);
            lastTouchTime = millis();
        }
    } else {
        touchRegistered = false;
    }

    // Actualizare valori în timp real
    temperaturaApa += directieApa * 0.5;
    if (temperaturaApa >= 120 || temperaturaApa <= 0) directieApa *= -1;
    temperaturaUlei = temperaturaApa + 10;
    presiuneUlei = (temperaturaApa >= 90) ? 5.0 : map(temperaturaApa, 0, 120, 0, 10);

    // Update doar la ecranul activ
    afiseazaEcran();
    delay(30);
}
