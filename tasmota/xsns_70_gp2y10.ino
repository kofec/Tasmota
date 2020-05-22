/*
  xsns_70_GP2Y10.ino - ESP8266 GP2Y10 support for Sonoff-Tasmota

  Copyright (C) 2019  Arnout Smeenge

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifdef USE_GP2Y10
/*********************************************************************************************\
 * GP2Y10 support
\*********************************************************************************************/

#define XSNS_70                       70

// Parameters for equation

struct GP2Y10 {
   bool gpy_active = true;
   bool gpy_gpio_pin = false;
   int qual_last_value = 0;
   int ACO_last_value = 0;
   int AQI_value = 0;
} Gp2y10;

void GPYInit(void)
{
  if ((ADC0_GP2Y10 == my_adc0) && Gp2y10.gpy_gpio_pin) {
    Settings.adc_param_type = ADC0_GP2Y10;
    pinMode(Pin(GPIO_GP2Y10), OUTPUT);
    digitalWrite(Pin(GPIO_GP2Y10), HIGH);
  } else {
    //deactivate GPY
    Gp2y10.gpy_active = false;
  }
}

bool GPYPinState()
{
  if ((XdrvMailbox.index == GPIO_GP2Y10) && (ADC0_GP2Y10 == my_adc0)) {
    XdrvMailbox.index = GPIO_GP2Y10;
    Gp2y10.gpy_gpio_pin = true;
    return true;
  }
  return false;
}

uint16_t GPYRead(uint8_t factor)
{
  // factor 1 = 2 samples
  // factor 2 = 4 samples
  // factor 3 = 8 samples
  // factor 4 = 16 samples
  // factor 5 = 32 samples
  uint8_t samples = 1 << factor;
  uint16_t analog = 0;

  byte x;
  for (x = 0; x < samples; x++)
  {
    noInterrupts();
    digitalWrite(Pin(GPIO_GP2Y10), LOW);
    delayMicroseconds(280);
    Gp2y10.ACO_last_value = analogRead(A0);
    analog += Gp2y10.ACO_last_value;
    delayMicroseconds(40);
    digitalWrite(Pin(GPIO_GP2Y10), HIGH);
    interrupts();
    if (x < (samples - 1)) { delayMicroseconds(9680); }

  }


  // exponential running average
//  int qual = ((analog*10000) >> (factor+10));
    int qual = analog;
  Gp2y10.qual_last_value = (0.05 * qual) + 0.95 * Gp2y10.qual_last_value;

  Gp2y10.AQI_value = Gp2y10.qual_last_value * 6 * 3 / 100; //Dust density sensing range 0 to 600 μg/m3
}                            // AQI 3 * Dust

#ifdef USE_RULES
void GPYEvery250ms(void)
{

}
#endif  // USE_RULES

void GPYEverySecond(void)
{
  GPYRead(0);
}

/*********************************************************************************************\
 * Commands
\*********************************************************************************************/
#ifdef USE_WEBSERVER
const char HTTP_SNS_GP2Y10[]    PROGMEM =
  "{s}GP2Y10 "  D_ANALOG_INPUT      "{m}%d{e}"
  "{s}GP2Y10 "  D_AIR_QUALITY       "{m}%d%%{e}";
#endif  // USE_WEBSERVER

void GPYShow(bool json)
{
  if (Gp2y10.gpy_active) {
      GPYRead(0);   //only one sample
        if (json) {
          ResponseAppend_P(JSON_SNS_AIRQUALITY, D_SENSOR_GP2Y10, Gp2y10.qual_last_value);
    #ifdef USE_WEBSERVER
        } else {
          WSContentSend_PD(HTTP_SNS_GP2Y10, Gp2y10.ACO_last_value, Gp2y10.qual_last_value);
    #endif  // USE_WEBSERVER
        }
  }
}

/*********************************************************************************************\
 * Interface
\*********************************************************************************************/

bool Xsns70(uint8_t function)
{
  bool result = false;

  if (Gp2y10.gpy_active) {
    switch (function) {
#ifdef USE_RULES
      case FUNC_EVERY_250_MSECOND:
        GPYEvery250ms();
        break;
#endif  // USE_RULES
      case FUNC_EVERY_SECOND:
        GPYEverySecond();
        break;
      case FUNC_INIT:
        GPYInit();
        break;
      case FUNC_PIN_STATE:  //before init
        result = GPYPinState();
        break;
      case FUNC_JSON_APPEND:
        GPYShow(1);
        break;
#ifdef USE_WEBSERVER
      case FUNC_WEB_SENSOR:
        GPYShow(0);
        break;
#endif  // USE_WEBSERVER
    }
  }
  return result;
}

#endif  // USE_GP2Y10
