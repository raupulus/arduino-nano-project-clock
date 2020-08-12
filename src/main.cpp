#include <Arduino.h>

// RGB leds
#include <Adafruit_NeoPixel.h>

// TM1637 Display 7segmentos x 4 bloques
#include <TM1637Display.h>

// RTC DS1307 (RELOJ)
#include <Wire.h>

// Bosh BMP180
#include <Adafruit_BMP085.h>

// LiquidCrystal i2c (pantalla 16x2)
#include <LiquidCrystal_I2C.h>

// RGB leds
#ifdef __AVR__
#include <avr/power.h>
#endif
#define PIN 8
#define NUMPIXELS 12
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
#define DELAYVAL 950
// Listado de colores (verde, azul, naranja, rojo)
int COLORS[4][3]{
    {0, 255, 0},
    {0, 0, 255},
    {255, 159, 35},
    {255, 0, 0}
};

// TM1637 Display 7segmentos x 4 bloques
const int CLK = SCK; //Set the CLK pin connection to the display
const int DIO = 9; //Set the DIO pin connection to the display
 
TM1637Display display(CLK, DIO); //set up the 4-Digit Display.
// Devuelve un int preparado para ser mostrado por la pantalla directamente
  byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;

// RTC DS1307 (RELOJ)
#define DS3231_I2C_ADDRESS 0x68

// Convert normal decimal numbers to binary coded decimal
byte decToBcd(byte val){
  return( (val/10*16) + (val%10) );
}
// Convert binary coded decimal to normal decimal numbers
byte bcdToDec(byte val){
  return( (val/16*10) + (val%16) );
}

// Bosh BMP180
Adafruit_BMP085 bmp;


// LiquidCrystal i2c (pantalla 16x2) Puede ser la dirección: 0x27, 0x38
#define BACKLIGHT_PIN 7
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  // Set the LCD I2C address
//LiquidCrystal_I2C lcd(0x38, BACKLIGHT_PIN, POSITIVE);  // Set the LCD I2C address

// IR proximity sensor
const int IR_SENSOR_PIN = 6;

// Agrega timestamp al módulo RTC DS1307 (RELOJ)
void setDS3231time(byte second, byte minute, byte hour, byte dayOfWeek, byte
dayOfMonth, byte month, byte year){
  // sets time and date data to DS3231
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0); // set next input to start at the seconds register
  Wire.write(decToBcd(second)); // set seconds
  Wire.write(decToBcd(minute)); // set minutes
  Wire.write(decToBcd(hour)); // set hours
  Wire.write(decToBcd(dayOfWeek)); // set day of week (1=Sunday, 7=Saturday)
  Wire.write(decToBcd(dayOfMonth)); // set date (1 to 31)
  Wire.write(decToBcd(month)); // set month
  Wire.write(decToBcd(year)); // set year (0 to 99)
  Wire.endTransmission();
}

// Lee el módulo RTC DS1307 (RELOJ)
void readDS3231time(byte *second,
byte *minute,
byte *hour,
byte *dayOfWeek,
byte *dayOfMonth,
byte *month,
byte *year) {
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0); // set DS3231 register pointer to 00h
  Wire.endTransmission();
  Wire.requestFrom(DS3231_I2C_ADDRESS, 7);
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
void displayTime(){
  byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
  // retrieve data from DS3231
  readDS3231time(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month,
  &year);
  // send it to the serial monitor
  Serial.print(hour, DEC);
  // convert the byte variable to a decimal number when displayed
  Serial.print(":");
  if (minute<10){
    Serial.print("0");
  }
  Serial.print(minute, DEC);
  Serial.print(":");
  if (second<10){
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
  switch(dayOfWeek){
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

void setup() {
  // Inicio Serial
  Serial.begin(9600);

  // RGB
  pixels.begin();
  pixels.setBrightness(10);
  pixels.show(); // Inicializa todos los píxeles a 'off'

  // TM1637 Display 7segmentos x 4 bloques
  //display.setBrightness(0x0a); //set the diplay to maximum brightness
  display.setBrightness(0x01);
  // Mostrar decimales
  display.showNumberDec(0000, true);


  // RTC DS1307 (RELOJ)
  // Setea valores del reloj: DS1307 seconds, minutes, hours, day, date, month, year
  //setDS3231time(00,39,20,5,11,2,20);


  // Bosh BMP180
  if (!bmp.begin()) {
	  Serial.println("Could not find a valid BMP085 sensor, check wiring!");
    delay ( 5000 );
	  //while (1) {}
  }

  // LiquidCrystal i2c (pantalla 16x2)
  // Se controla la salida de el pin que activa la retroiluminación por proximidad.
  pinMode(BACKLIGHT_PIN, OUTPUT);
  digitalWrite (BACKLIGHT_PIN, HIGH);

  
  lcd.begin(16,2);  // initialize the lcd 
  lcd.setBacklight(LOW);
  
  lcd.home ();                   // go home
  lcd.print("ESP32 BY RAUL");  
  lcd.setCursor ( 0, 1 );        // go to the next line
  lcd.print (" INICIALIZANDO...   ");
  delay ( 1000 );


  // IR proximity sensor
  pinMode (IR_SENSOR_PIN, INPUT);
}

/*
 * Enciende o apaga el brillo de la pantalla según el detector IR de proximidad.
 */ 
void displayBacklightToggle() {
  bool has_proximity = digitalRead(IR_SENSOR_PIN);

  if (has_proximity) {
    lcd.on();
    digitalWrite (BACKLIGHT_PIN, HIGH);
    pixels.setBrightness(180);
    display.setBrightness(0x0a);
  } else {
    lcd.off();
    digitalWrite (BACKLIGHT_PIN, LOW);
    pixels.setBrightness(1);
    display.setBrightness(0x00);
  }
}

void loop() {
  Serial.println("--- Comienza todo el loop ---");


  // retrieve data from DS3231
  readDS3231time(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);

  // RGB
  // Cada 5 segundos en ciende un led (12*5 = 1 minuto)
  // Cada bloque de 3 led cambiará de color (verde, azul, naranja, rojo)
  int n_leds_on = int ((second / 60.0) * 12);
  int color_leds = int ((n_leds_on / 12.0) * 4);
  
  Serial.print(second);
  Serial.println();
  Serial.print(n_leds_on);
  Serial.println();
  Serial.print(color_leds);
  Serial.println();

  if (n_leds_on < 1) {
    pixels.clear();
    pixels.setPixelColor(0, pixels.Color(COLORS[color_leds][0], COLORS[color_leds][1], COLORS[color_leds][2]));
    pixels.show();
  } else {
    pixels.fill(pixels.Color(COLORS[color_leds][0], COLORS[color_leds][1], COLORS[color_leds][2]), 0, n_leds_on + 1);

    // Muestra los leds habilitados para esta iteración
    pixels.show();
  }

  


  // TM1637 Display 7segmentos x 4 bloques
  int value;
  if (hour > 0) {
    value = (hour*100) + minute;
  } else {
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

  // RTC
  displayTime();


  // Bosh BMP180
  float temperature = bmp.readTemperature();
  int pressure = bmp.readPressure();
  float altitude = bmp.readAltitude();
  int sealevelpresure = bmp.readSealevelPressure();
  float realaltitude = bmp.readAltitude(101500);

  Serial.print("Temperature = ");
  Serial.print(temperature);
  Serial.println(" *C");
  
  Serial.print("Pressure = ");
  Serial.print(pressure);
  Serial.println(" Pa");
  
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

  // LiquidCrystal i2c (pantalla 16x2)
  lcd.clear();
  lcd.home();
  lcd.print(temperature);
  lcd.print("C ");
  lcd.print(pressure);
  lcd.print("Pa ");
  lcd.setCursor ( 0, 1 );
  lcd.print("Altitud: ");
  lcd.print(altitude);
  lcd.print("m");

  displayBacklightToggle();

  // Pausa entre iteraciones
  delay(DELAYVAL);
}

/**
   Funcion auxiliar para imprimir siempre 2 digitos
*/
void print2digits(int number) {
  if (number >= 0 && number < 10) {
    Serial.write('0');
  }
  Serial.print(number);
}