#ifndef HOME_ASSIST_FIREPLACE
#define HOME_ASSIST_FIREPLACE

#include <HADevice.h>
#include <device-types\HANumber.h>
#include <device-types\HASwitch.h>
#include <device-types\HAButton.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include <Timer.h>

#define SENSOR_COUNT 10    // its actually 4 but well..... give it some slack

////////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////////
class HAFireplace : public HADevice 
{
private:
  enum FlameLevel {
    NO_FLAME,
    FULL_FLAME,
    LOW_FLAME,
    PILOT_FLAME,
    CUSTOM_FLAME_LVL,
    CHANGING_FLAME_LVL
  };

  Timer _btn_higher;  // timers to steer the relays
  Timer _btn_lower;
  Timer _btn_off;
  
  Timer _changing_flame;
  FlameLevel _new_flame;
  void _change_flame(uint32_t ms, FlameLevel new_level);
  void _sync_dashboard();
public:
  HAFireplace();

  FlameLevel flame_level;  // current fire level

  HASwitch sw_onoff;  // switching on or off
  HAButton btn_higher;
  HAButton btn_lower;
  HAButton btn_pilot;
  HAButton btn_high;
  HAButton btn_low;
  
  bool begin(const byte mac[6], HAMqtt *mqqt);
  bool loop();  

  bool on_off(bool state);//switch on (or off)
  bool higher();// turn flame higher  
  bool lower(); // turn flame lower
  bool pilot(); // turn to pilot mode (all the way down)
  bool high();  // turn to highest level (all the way up)
  bool low();   // turn to lowest possible fire level 
};

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

#endif