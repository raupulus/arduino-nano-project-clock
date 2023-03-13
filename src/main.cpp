#include <Arduino.h>
#include <Wire.h>
#include "SPI.h"

#ifdef __AVR__
#include <avr/power.h>
#endif

// Pausa entre loops
const int DELAYVAL = 950;

// RTC DS1307 (RELOJ)
#define DS1307_I2C_ADDRESS 0x68

// IR proximity sensor
const int IR_SENSOR_PIN = 6;

// Bosh BMP180 - Sensor de humedad y temperatura
#include <Adafruit_BMP085.h>
Adafruit_BMP085 bmp;
float temperature = 0;
float pressure = 0;
float altitude = 0;
int sealevelpresure = 0;
float realaltitude = 0;

// RGB leds con Neopixel de 12 leds.
#include <Adafruit_NeoPixel.h>
const int LED_RGB_12_PIN = 8;
const int LED_RGB_12_NUM_LEDS = 12;
Adafruit_NeoPixel pixels(LED_RGB_12_NUM_LEDS, LED_RGB_12_PIN, NEO_GRB + NEO_KHZ800);
int COLORS[4][3] // Listado de colores (verde, azul, naranja, rojo)
    {
        {0, 255, 0},
        {0, 0, 255},
        {255, 159, 35},
        {255, 0, 0}};

// TM1637 Display 7segmentos x 4 bloques
#include <TM1637Display.h>
const int CLK = SCK;                                           // Set the CLK pin connection to the display
const int DIO = 9;                                             // Set the DIO pin connection to the display
TM1637Display display(CLK, DIO);                               //set up the 4-Digit Display.
byte second, minute, hour, dayOfWeek, dayOfMonth, month, year; // Devuelve un int preparado para ser mostrado por la pantalla directamente

// LiquidCrystal i2c (pantalla 16x2) - Display 7 segmentos - Puede ser la dirección: 0x27, 0x38
#include <LiquidCrystal_I2C.h>
#define BACKLIGHT_PIN 7
const int DISPLAY_16X2_BACKLIGHT_PIN = 7;
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

/**
 * Convierte un numero decimal a su codificación en binario.
 */
byte decToBcd(byte val)
{
    return ((val / 10 * 16) + (val % 10));
}

/**
 * Convierte un binario (decimal codificado) en un número decimal puro.
 */
byte bcdToDec(byte val)
{
    return ((val / 16 * 10) + (val % 16));
}

/**
 * Agrega timestamp al módulo RTC DS1307 (RELOJ)
 */
bool setDateTimeRTC(byte second, byte minute, byte hour, byte dayOfWeek,
                    byte dayOfMonth, byte month, byte year)
{
    Wire.beginTransmission(DS1307_I2C_ADDRESS);
    Wire.write(0x00);                    // set next input to start at the seconds register
    Wire.write(decToBcd(second) & 0x7F); // set seconds
    Wire.write(decToBcd(minute));        // set minutes
    Wire.write(decToBcd(hour));          // set hours
    Wire.write(decToBcd(dayOfWeek));     // set day of week (1=Sunday, 7=Saturday)
    Wire.write(decToBcd(dayOfMonth));    // set date (1 to 31)
    Wire.write(decToBcd(month));         // set month
    Wire.write(decToBcd(year));          // set year (0 to 99)

    // Cierra la transmisión e indica si se hizo correctamente.
    if (Wire.endTransmission() != 0)
    {
        Serial.print("DS1307 → La transmisión para establecer hora ha fallado.");
        return false;
    }

    Serial.print("DS1307 → Se ha establecido la hora correctamente.");

    return true;
}

/**
 * Lee el módulo RTC DS1307 (RELOJ)
 */
void getDateTimeRTC(byte *second, byte *minute, byte *hour, byte *dayOfWeek,
                    byte *dayOfMonth, byte *month, byte *year)
{
    Wire.beginTransmission(DS1307_I2C_ADDRESS);
    Wire.write(0); // Set DS1307 register pointer to 00h
    Wire.endTransmission();
    Wire.requestFrom(DS1307_I2C_ADDRESS, 7);
    // request seven bytes of data from DS3231 starting from register 00h
    *second = bcdToDec(Wire.read() & 0x7f);
    *minute = bcdToDec(Wire.read());
    *hour = bcdToDec(Wire.read() & 0x3f);
    *dayOfWeek = bcdToDec(Wire.read());
    *dayOfMonth = bcdToDec(Wire.read());
    *month = bcdToDec(Wire.read());
    *year = bcdToDec(Wire.read());
}

// Muestra datos por consola del módulo RTC DS1307 (RELOJ)
void displayTime()
{
    byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
    // retrieve data from DS3231
    getDateTimeRTC(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);
    // send it to the serial monitor
    Serial.print(hour, DEC);
    // convert the byte variable to a decimal number when displayed
    Serial.print(":");

    // TENER EN CUENTA LOS cambios de horario:
    // sabado 28 de marzo al domingo 29 (2:00) → UTC + 2h
    // Sabado 25 de Octubre al domingo 26 (3:00) → UTC + 1h

    if (minute < 10)
    {
        Serial.print("0");
    }
    Serial.print(minute, DEC);
    Serial.print(":");
    if (second < 10)
    {
        Serial.print("0");
    }
    Serial.print(second, DEC);
    Serial.print(" ");
    Serial.print(dayOfMonth, DEC);
    Serial.print("/");
    Serial.print(month, DEC);
    Serial.print("/");
    Serial.print(year, DEC);
    Serial.print(" Día de la semana: ");
    switch (dayOfWeek)
    {
    case 1:
        Serial.println("Domingo");
        break;
    case 2:
        Serial.println("Lunes");
        break;
    case 3:
        Serial.println("Martes");
        break;
    case 4:
        Serial.println("Miércoles");
        break;
    case 5:
        Serial.println("Jueves");
        break;
    case 6:
        Serial.println("Viernes");
        break;
    case 7:
        Serial.println("Sábado");
        break;
    }
}

/**
 * Funcion auxiliar para imprimir siempre 2 digitos
 */
void print2digits(int number)
{
    if (number >= 0 && number < 10)
    {
        Serial.write('0');
    }
    Serial.print(number);
}

/*
 * Enciende o apaga el brillo de la pantalla según el detector IR de proximidad.
 */
void displayBacklightToggle()
{
    bool has_proximity = digitalRead(IR_SENSOR_PIN);

    if (has_proximity)
    {
        lcd.on();
        lcd.backlight();
        digitalWrite(DISPLAY_16X2_BACKLIGHT_PIN, HIGH);
        pixels.setBrightness(180);
        display.setBrightness(0x0a);
    }
    else
    {
        lcd.noBacklight();
        lcd.off();
        digitalWrite(DISPLAY_16X2_BACKLIGHT_PIN, LOW);
        pixels.setBrightness(1);
        display.setBrightness(0x01);
    }
}

/**
 * Obtiene los datos del sensor Bosh BMP180.
 */
void getAtmosphericData()
{
    temperature = bmp.readTemperature();
    pressure = bmp.readPressure(); // Presión en pascales (multiplicar por 10 para milibares)
    altitude = bmp.readAltitude();
    altitude = altitude >= 0 ? altitude : (altitude * -1);
    sealevelpresure = bmp.readSealevelPressure();
    realaltitude = bmp.readAltitude(101325.0F);
}

/**
 * Muestra todos los datos por Serial.
 */
void printBySerial()
{
    Serial.print("Temperature = ");
    Serial.print(temperature);
    Serial.println(" *C");

    Serial.print("Pressure = ");
    Serial.print(pressure);
    Serial.println(" Pa");

    Serial.print("Pressure = ");
    Serial.print((int)(pressure / 10));
    Serial.println(" hPa");

    // Calculate altitude assuming 'standard' barometric
    // pressure of 1013.25 millibar = 101325 Pascal
    Serial.print("Altitude = ");
    Serial.print(altitude);
    Serial.println(" meters");

    Serial.print("Pressure at sealevel (calculated) = ");
    Serial.print(sealevelpresure);
    Serial.println(" Pa");

    // you can get a more precise measurement of altitude
    // if you know the current sea level pressure which will
    // vary with weather and such. If it is 1015 millibars
    // that is equal to 101500 Pascals.
    Serial.print("Real altitude = ");
    Serial.print(realaltitude);
    Serial.println(" meters");

    Serial.println();
}

/**
 * Dibuja los datos por la pantalla LCD de 16x2
 */
void printByDisplayLCD16x2()
{
    // LiquidCrystal i2c (pantalla 16x2)
    lcd.clear();
    lcd.home();
    lcd.print(temperature);
    lcd.print("C ");
    lcd.print((int)(pressure / 10));
    lcd.print("hPa ");
    lcd.setCursor(0, 1);
    lcd.print("Altitud: ");
    lcd.print(altitude);
    lcd.print("m");
}

/**
 * Dibuja la hora en la pantalla de 7 segmentos y 4 dígitos (TM1637).
 */
void printByDisplayHour()
{
    // TM1637 Display 7segmentos x 4 bloques
    int value;
    if (hour > 0)
    {
        value = (hour * 100) + minute;
    }
    else
    {
        value = minute;
    }

    //uint8_t segto;
    //segto = 0x80 | display.encodeDigit((value / 100)%10);
    //Serial.print(segto);
    //Serial.println();
    //display.setSegments(&segto, 1, 1);
    //display.showNumberDec(value, true);
    //display.showNumberDecEx(value, true, true);
    display.showNumberDecEx(value, 0b11100000, true, 4, 0);
}

/**
 * Calcula y enciende los leds RGB del neopixel con 12 leds.
 */
void neopixelLedRgb12()
{
    // RGB
    // Cada 5 segundos enciende un led (12*5 = 1 minuto)
    // Cada bloque de 3 led cambiará de color (verde, azul, naranja, rojo)
    int n_leds_on = int((second / 60.0) * 12);
    int color_leds = int((n_leds_on / 12.0) * 4);

    /*
    Serial.print(second);
    Serial.println();
    Serial.print(n_leds_on);
    Serial.println();
    Serial.print(color_leds);
    Serial.println();
    */

    if (n_leds_on < 1)
    {
        pixels.clear();
        pixels.setPixelColor(0, pixels.Color(COLORS[color_leds][0], COLORS[color_leds][1], COLORS[color_leds][2]));
        pixels.show();
    }
    else
    {
        pixels.fill(pixels.Color(COLORS[color_leds][0], COLORS[color_leds][1], COLORS[color_leds][2]), 0, n_leds_on + 1);

        // Muestra los leds habilitados para esta iteración
        pixels.show();
    }
}

void setup()
{
    // Inicio Serial
    Serial.begin(9600);

    // Preparar la librería Wire (I2C)
    Wire.begin();

    // RTC DS1307 (RELOJ) - Inicialización en hora UTC.
    //setDateTimeRTC(00, 31, 21, 5, 25, 12, 21); // seconds, minutes, hours, day, date, month, year

    // RGB leds con Neopixel de 12 leds.
    pixels.begin();
    pixels.setBrightness(180); // Establece el brillo de los leds encendidos.
    pixels.show();             // Inicializa todos los píxeles a 'off' (Apagados).

    // TM1637 Display 7segmentos x 4 bloques
    display.setBrightness(0x01);       // 0x0a Es el brillo máximo.
    display.showNumberDec(0000, true); // Activa para mostrar decimales

    // Bosh BMP180
    if (!bmp.begin())
    {
        Serial.println("No se puede inicializar el sensor Bosh BMP180");
        delay(1000);
    }

    // LiquidCrystal i2c (pantalla 16x2) - Se controla la salida de el pin que activa la retroiluminación por proximidad.
    pinMode(DISPLAY_16X2_BACKLIGHT_PIN, OUTPUT);
    digitalWrite(DISPLAY_16X2_BACKLIGHT_PIN, HIGH);
    lcd.begin(16, 2); // initialize the lcd
    lcd.setBacklight(LOW);
    lcd.home(); // go home
    lcd.print("ARDUINO BY RAUL");
    lcd.setCursor(0, 1); // go to the next line
    lcd.print(" INICIALIZANDO...   ");

    // IR proximity sensor
    pinMode(IR_SENSOR_PIN, INPUT);

    delay(1000);
}

void loop()
{
    displayBacklightToggle();

    // Leo DateTime del módulo RTC.
    getDateTimeRTC(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);

    // Enciendo leds correspondientes a los segundos/5
    neopixelLedRgb12();

    // RTC
    displayTime();

    // Lectura de los datos atmosféricos actuales.
    getAtmosphericData();

    // Muestro los datos por serial y pantallas.
    printBySerial();
    printByDisplayLCD16x2();
    printByDisplayHour();

    // Pausa entre iteraciones
    delay(DELAYVAL);
}
