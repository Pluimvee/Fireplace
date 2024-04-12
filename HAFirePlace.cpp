#include "HAFirePlace.h"
#include <HAMqtt.h>
#include <String.h>
#include <Clock.h>

#include <DatedVersion.h>
DATED_VERSION(0, 9)
#define LOG_REMOTE
#define LOG_LEVEL 2
#include <Logging.h>
#include <Timer.h>

////////////////////////////////////////////////////////////////////////////////////////////
#define DEVICE_NAME  "Fireplace"
#define DEVICE_MODEL "Fireplace Remote esp8266"
#define MULTIPLIER   1000

extern Clock rtc;
////////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////////
void onState(bool state, HASwitch* sender) {
  ((HAFireplace *)(HAMqtt::instance()->getDevice()))->on_off(state);
}

void onLower(HAButton *sender) {
  ((HAFireplace *)(HAMqtt::instance()->getDevice()))->lower();
}

void onHigher(HAButton *sender) {
  ((HAFireplace *)(HAMqtt::instance()->getDevice()))->higher();
}

void onPilot(HAButton *sender) {
  ((HAFireplace *)(HAMqtt::instance()->getDevice()))->pilot();
}

void onHigh(HAButton *sender) {
  ((HAFireplace *)(HAMqtt::instance()->getDevice()))->high();
}

void onLow(HAButton *sender) {
  ((HAFireplace *)(HAMqtt::instance()->getDevice()))->low();
}

////////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////////
HAFireplace::HAFireplace()
: flame_level(NO_FLAME),
sw_onoff("ignite_or_off"), btn_higher("flame_higher"), btn_lower("flame_lower"), btn_pilot("flame_pilot"), btn_high("big_fire"), btn_low("small_fire")
{
  sw_onoff.setName("ignite");
  sw_onoff.setIcon("mdi:fire-off");
  sw_onoff.onCommand(onState);

  btn_higher.setName("higher");
  btn_higher.setIcon("mdi:arrow-up-drop-circle");
  btn_higher.onCommand(onHigher);

  btn_lower.setName("lower");
  btn_lower.setIcon("mdi:arrow-down-drop-circle");
  btn_lower.onCommand(onLower);

  btn_pilot.setName("pilot");
  btn_pilot.setIcon("mdi:candle");
  btn_pilot.onCommand(onPilot);

  btn_high.setName("big_fire");
  btn_high.setIcon("mdi:fire");
  btn_high.onCommand(onHigh);

  btn_low.setName("small_fire");
  btn_low.setIcon("mdi:campfire");
  btn_low.onCommand(onLow);
}

////////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////////
bool HAFireplace::begin(const byte mac[6], HAMqtt *mqtt) 
{
  setUniqueId(mac, 6);
  setManufacturer("InnoVeer");
  setName(DEVICE_NAME);
  setSoftwareVersion(VERSION);
  setModel(DEVICE_MODEL);

  mqtt->addDeviceType(&sw_onoff);  
  mqtt->addDeviceType(&btn_higher);  
  mqtt->addDeviceType(&btn_lower);  
  mqtt->addDeviceType(&btn_pilot);  
  mqtt->addDeviceType(&btn_high);  
  mqtt->addDeviceType(&btn_low);  

  pinMode(D0, OUTPUT);  digitalWrite(D0, HIGH);
  pinMode(D1, OUTPUT);  digitalWrite(D1, HIGH);
  pinMode(D2, OUTPUT);  digitalWrite(D2, HIGH);
  pinMode(D3, OUTPUT);  digitalWrite(D3, HIGH);

  flame_level = PILOT_FLAME;
  _sync_dashboard();
  return true;
}

////////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////////
bool HAFireplace::loop()
{
  if (_btn_higher.passed())
    digitalWrite(D0, HIGH); // off
  else
    digitalWrite(D0, LOW);  // on

  if (_btn_lower.passed())
    digitalWrite(D2, HIGH);
  else
    digitalWrite(D2, LOW);

  if (_btn_off.passed())
    digitalWrite(D1, HIGH);
  else
    digitalWrite(D1, LOW);
  
  if (_changing_flame.passed())
  {
    if (_new_flame != flame_level)
    {
      flame_level = _new_flame;
      _sync_dashboard();
    }
  }
  return true;
}

////////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////////
void HAFireplace::_change_flame(uint32_t ms, FlameLevel new_level)
{
  flame_level = CHANGING_FLAME_LVL;
  _changing_flame.set(ms);
  _new_flame = new_level;
  _sync_dashboard();
}

////////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////////
void HAFireplace::_sync_dashboard()
{
  bool on = (flame_level != NO_FLAME) && (flame_level != CHANGING_FLAME_LVL);

  sw_onoff.setState(on);
  sw_onoff.setAvailability(flame_level != CHANGING_FLAME_LVL);
  btn_higher.setAvailability(on);
  btn_lower.setAvailability(on);
  btn_pilot.setAvailability(on);
  btn_high.setAvailability(on);
  btn_low.setAvailability(on && (flame_level != CUSTOM_FLAME_LVL));
}

////////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////////
bool HAFireplace::on_off(bool state)
{
  if (state)  // switch on
  {
    INFO("[%s] - Ignite (5s)\n",
          rtc.now().timestamp(DateTime::TIMESTAMP_TIME).c_str());
    _btn_off.set(5 *MULTIPLIER);
    sw_onoff.setState(true);
    _change_flame(15000, FULL_FLAME);
  }
  else
  {
    INFO("[%s] - Switch off (2s)\n",
          rtc.now().timestamp(DateTime::TIMESTAMP_TIME).c_str());
    _btn_off.set(2 *MULTIPLIER);
    sw_onoff.setState(false);
    _change_flame(5000, NO_FLAME);
  }
  return true;
}

bool HAFireplace::higher()
{
  INFO("[%s] - Turning up\n",
        rtc.now().timestamp(DateTime::TIMESTAMP_TIME).c_str());
  _btn_higher.set(1 *MULTIPLIER);
  _change_flame(1 *MULTIPLIER, CUSTOM_FLAME_LVL);
  return true;
}

bool HAFireplace::lower()
{
  INFO("[%s] - Turning down\n",
        rtc.now().timestamp(DateTime::TIMESTAMP_TIME).c_str());
  _btn_lower.set(1 *MULTIPLIER);
  _change_flame(1 *MULTIPLIER, CUSTOM_FLAME_LVL);
  return true;
}

bool HAFireplace::pilot()
{
  INFO("[%s] - Turning down all the way to pilot\n",
        rtc.now().timestamp(DateTime::TIMESTAMP_TIME).c_str());
  _btn_lower.set(10 *MULTIPLIER);
  _change_flame(10 *MULTIPLIER, PILOT_FLAME);
  return true;
}

bool HAFireplace::high()
{
  INFO("[%s] - Turning up all the way to full flame\n",
        rtc.now().timestamp(DateTime::TIMESTAMP_TIME).c_str());
  _btn_higher.set(10 *MULTIPLIER);
  _change_flame(10 *MULTIPLIER, FULL_FLAME);
  return true;
}

bool HAFireplace::low()
{
  if (flame_level == PILOT_FLAME)
  {
    INFO("[%s] - Turning up to low flame level\n",
          rtc.now().timestamp(DateTime::TIMESTAMP_TIME).c_str());
    _btn_higher.set(4 *MULTIPLIER);
    _change_flame(4 *MULTIPLIER, LOW_FLAME);
  }
  else if (flame_level == FULL_FLAME)
  {
    INFO("[%s] - Turning down to low flame level\n",
          rtc.now().timestamp(DateTime::TIMESTAMP_TIME).c_str());
    _btn_lower.set(4 *MULTIPLIER);
    _change_flame(4 *MULTIPLIER, LOW_FLAME);
  }
  return true;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

