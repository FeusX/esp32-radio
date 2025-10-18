#include <Wire.h>

#define POT_PIN 34
#define DEAD_ZONE 5

uint8_t last_val = 0;

void setFrequency(float freq_mhz)
{
  uint16_t freq_byte = 4 * (freq_mhz * 1000000 + 225000) / 32768;
  uint8_t freq_high = freq_byte >> 8;
  uint8_t freq_low = freq_byte & 0xFF;

  Wire.beginTransmission(0x60); // tea5767 address
  Wire.write(freq_high);
  Wire.write(freq_low);
  Wire.write(0xB0);
  Wire.write(0x10);
  Wire.write(0x00);
  Wire.endTransmission();
}

void setup()
{
  Serial.begin(115200);
  Wire.begin(21, 22);  
}

void loop()
{
  if(abs(analogRead(POT_PIN) - last_val > DEAD_ZONE))
  {
    float freq = map(analogRead(POT_PIN), 0, 4095, 875, 1080) / 10.0;

    setFrequency(freq);
    last_val = analogRead(POT_PIN);
    Serial.print(freq);
    Serial.print("\n");
  }
}
