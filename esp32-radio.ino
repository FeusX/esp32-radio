#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <math.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define POT_PIN 34
#define DEAD_ZONE 20
#define LED_PIN 32

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

uint16_t last_val = 0;
float current_freq = 0.0;
float smoothed_signal = 0.0;
const float smooth_alpha = 0.15;
const int led_min_rssi = 12;
const int led_max_rssi = 18;

void setFrequency(float freq_mhz)
{
  uint16_t freq_word = 4 * ((freq_mhz * 1000000) + 225000) / 32768;
  uint8_t freq_high = freq_word >> 8;
  uint8_t freq_low = freq_word & 0xFF;

  Wire.beginTransmission(0x60); // tea5767 address
  Wire.write(freq_high);
  Wire.write(freq_low);
  Wire.write(0xB0); // stereo on
  Wire.write(0x10); // mute off, no standby
  Wire.write(0x00);
  Wire.endTransmission();
}

int readSignal()
{
  Wire.requestFrom(0x60, 5);
  int available = Wire.available();
  if(available >= 5) 
  {
    for (uint8_t k = 0; k < 3; k++) Wire.read();
    uint8_t byte_four = Wire.read();
    Wire.read();
    int signal = byte_four & 0x1F;
    return signal;
  }
  Serial.println("Not enough bytes.");
  return -1; 
}

void displayWave()
{
  static float phase = 0.0;
  display.clearDisplay();

  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.printf("%.1f MHz", current_freq);

  float wave_freq = map(current_freq * 10, 875, 1080, 10, 25) / 100.0;
  float amplitude = map(current_freq * 10, 875, 1080, 8, 15);

  for(int x = 0; x < SCREEN_WIDTH; x++)
  {
    float y = sin((x * wave_freq) + phase) * amplitude;
    display.drawPixel(x, 48 + y, SSD1306_WHITE);
  }

  phase += 0.3;

  display.display();
}

void setup()
{
  Serial.begin(115200);
  Wire.begin(21, 22);

  while(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
    Serial.println(F("display failed"));
    delay(1000);

  ledcAttach(LED_PIN, 5000, 8);
  ledcWrite(LED_PIN, 128);
  delay(2000);
  ledcWrite(LED_PIN, 0);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  for(int i = 0; i < 3; i++)
  {
    display.clearDisplay();
    display.setCursor(0, 24);
    display.print("starting");

    for(int k = 0; k <= i; k++)
    {
      display.print(".");
    }

    display.display();
    delay(500);
  }


  delay(1000);

  smoothed_signal = readSignal();

  setFrequency(90.0);
}

void loop()
{
  uint16_t pot_val = analogRead(POT_PIN);

  if(abs(pot_val - last_val) > DEAD_ZONE)
  {
    float freq = map(pot_val, 0, 4095, 1080, 875) / 10.0;

    setFrequency(freq);
    current_freq = freq;
    last_val = pot_val;
  }

  int raw_signal = readSignal();

  if(raw_signal <= 0 || raw_signal > 31)
  { raw_signal = (int)round(smoothed_signal); }

  smoothed_signal = smoothed_signal + smooth_alpha * (raw_signal - smoothed_signal);

  float clamped_signal = constrain(smoothed_signal, led_min_rssi, led_max_rssi);

  int duty = map((int)round(clamped_signal), led_min_rssi, led_max_rssi, 0, 255);
  duty = constrain(duty, 0, 255);
  float corrected = pow((float)duty / 255.0, 1.0);
  ledcWrite(LED_PIN, (int)(corrected * 255 + 0.5));

  displayWave();

  delay(40);
}
