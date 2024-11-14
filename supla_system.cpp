#include <SuplaDevice.h>
#include <supla/network/esp_wifi.h>
#include <supla/control/relay.h>
#include <supla/control/button.h>
#include <supla/control/action_trigger.h>
#include <supla/storage/littlefs_config.h>
#include <supla/network/esp_web_server.h>
#include <supla/network/html/device_info.h>
#include <supla/network/html/protocol_parameters.h>
#include <supla/network/html/wifi_parameters.h>
#include <supla/network/html/custom_parameter.h>
#include <supla/network/html/custom_text_parameter.h>
#include <supla/network/html/time_parameters.h>
#include <supla/network/html/hvac_parameters.h>
#include <supla/device/supla_ca_cert.h>
#include <supla/events.h>
#include <supla/storage/eeprom.h>
#include <supla/control/virtual_relay.h>
#include <supla/sensor/SHT3x.h>
#include <supla/sensor/virtual_therm_hygro_meter.h>
#include <supla/control/hvac_base.h>
#include <supla/control/internal_pin_output.h>
#include <supla/clock/clock.h>

#include "TempDisplay.h"
#include "multi_virtual_relay.h"
#include "xiaomi_therm_hygro_meter.h"
#include "hvac_base_ee.h"
#include "action_handler_with_callbacks.h"

#include <NimBLEDevice.h>
#if !CONFIG_BT_NIMBLE_EXT_ADV
#  error Must enable extended advertising, see nimconfig.h file.
#endif

#define DEFAULT_SS_SCAN_INTERVAL      120
#define DEFAULT_BT_SCAN_INTERVAL      97
#define DEFAULT_BT_SCAN_WINDOW        37
#define DEFAULT_SCREEN_SAVER_INTERVAL 30

#define SHT3X_MIN_TEMPERATURE         -400
#define SHT3X_MAX_TEMPERATURE         1250

#undef USE_HARDCODED_DATA
#define USE_HARDCODED_DATA

// Supla section

Supla::Eeprom                             eeprom;


Supla::ESPWifi                            wifi;
Supla::LittleFsConfig                     configSupla(3000);

Supla::EspWebServer                       suplaServer;

Supla::Html::DeviceInfo                   htmlDeviceInfo(&SuplaDevice);
Supla::Html::WifiParameters               htmlWifi;
Supla::Html::ProtocolParameters           htmlProto;
Supla::Html::TimeParameters               htmlTimeParameters(&SuplaDevice);
Supla::Html::HvacParameters               htmlHvacParameters;

Supla::Sensor::SHT3x                      *SHT31;

Supla::Sensor::ProgDisplay                *DPrograms;
Supla::Control::Multi_VirtualRelay        *PDstep_mvr;

Supla::Control::HvacBaseEE                  *ESP32_hvac;

#define MAX_SENSORS                        9
#define THT_OUTPUT_PIN                     2

Supla::Sensor::XiaomiThermHygroMeter      *xiaomiSensors[MAX_SENSORS];
Supla::Sensor::XiaomiCalcThermHygroMeter  *hvac_therm;

int32_t                                   Sensors_cnt = 0;

bool                                      local_mode = true;

int32_t                                   ScreenSaverTimer    = 0;
int32_t                                   ssScanIntervalTimer = 0;
int32_t                                   ssScanInterval      = DEFAULT_SS_SCAN_INTERVAL;
int32_t                                   btScanInterval      = DEFAULT_BT_SCAN_INTERVAL;
int32_t                                   btScanWindow        = DEFAULT_BT_SCAN_WINDOW;
int32_t                                   ScreenSaverInterval = DEFAULT_SCREEN_SAVER_INTERVAL; 
int32_t                                   MenuRedrawTimer     = 0;

//end of Supla section

NimBLEScan                                *pBLEScan;
NimBLEClient                              *pClient;


static constexpr const char* const week_days[7] = { "ND", "PN", "WT", "ŚR", "CZ", "PT", "SB"};

bool      nm_menu_redraw    = true;

bool      screen_saver_on   = false;

bool      nm_config_mode    = false;

bool      ms_tht_change     = true;

time_t    tht_timer_time    = 0;
uint32_t  tht_timer_temp    = 2100;


//custom config section

const char          XIAOMI_CNT[] = "Xiaomi_cnt";

const static char   *XIAOMI_PARAMS[] PROGMEM = {
  "Xiaomi#1", "Xiaomi#2", "Xiaomi#3", "Xiaomi#4", "Xiaomi#5", "Xiaomi#6", "Xiaomi#7", "Xiaomi#8", "Xiaomi#9" };

const static char   *LOCS_PARAMS[] PROGMEM = {
  "loc#1", "loc#2", "loc#3", "loc#4", "loc#5", "loc#6", "loc#7", "loc#8", "loc#9" };

const static char   HVAC_T_TIME []  PROGMEM = "Hvac_T_time";
const static char   HVAC_T_TEMP []  PROGMEM = "Hvac_T_temp";

const static char   BT_SCAN_INTERVAL_PARAM [] PROGMEM = "BT_scan_interval";
const static char   BT_SCAN_WINDOW_PARAM []     PROGMEM = "BT_scan_window";

const static char   SCRS_INTERVAL_PARAM []     PROGMEM = "scrs_interval";

#ifdef USE_HARDCODED_DATA

#include "hcd/hcd_data.h"
  
#endif //USE_HARDCODED_DATA

char  xiaomiBleDeviceMacs[MAX_SENSORS][18];
char  xiaomiBleDeviceLocs[MAX_SENSORS][32];

const static char *program_names[] PROGMEM = {
"minimum",
"średnia",
"maximum",
"lokalizacja 1",
"lokalizacja 2",
"lokalizacja 3",
"lokalizacja 4",
"lokalizacja 5",
"lokalizacja 6",
"lokalizacja 7",
"lokalizacja 8",
"lokalizacja 9",
"avg - lokalizacja 1",
"avg - lokalizacja 2",
"avg - lokalizacja 3",
"avg - lokalizacja 4",
"avg - lokalizacja 5",
"avg - lokalizacja 6",
"avg - lokalizacja 7",
"avg - lokalizacja 8",
"avg - lokalizacja 9"
};

double  sensors_temperature [MAX_SENSORS];
double  sensors_humidity    [MAX_SENSORS];
byte    sensors_battery     [MAX_SENSORS];
int     sensors_rssi        [MAX_SENSORS];
byte    sensors_tick        [MAX_SENSORS];


std::string                 strServiceData;

class MyAdvertisedDeviceCallbacks : public NimBLEAdvertisedDeviceCallbacks {
  void onResult(NimBLEAdvertisedDevice* advertisedDevice) {
    if (advertisedDevice->haveName() && advertisedDevice->haveServiceData()) {

      int serviceDataCount = advertisedDevice->getServiceDataCount();
      strServiceData = advertisedDevice->getServiceData(0);

      String macAddress = advertisedDevice->getAddress().toString().c_str();
      macAddress.toUpperCase();
      for (int i = 0; i < Sensors_cnt; i++) {
        if (macAddress == xiaomiBleDeviceMacs[i]) {
          int16_t rawTemperature = (strServiceData[7] | (strServiceData[6] << 8));
          //Serial.print("raw Temperature = ");
          //Serial.println(rawTemperature);
          //Serial.println(macAddress);
          float current_temperature = rawTemperature * 0.1;
          byte current_humidity = (float)(strServiceData[8]);
          byte current_batt = (float)(strServiceData[9]);

          sensors_temperature[i] = current_temperature;
          sensors_humidity[i] = current_humidity;
          sensors_battery[i] = current_batt;
          sensors_rssi[i] = advertisedDevice->getRSSI();
          sensors_tick[i] = 0; 
          break;  
        }
      }
    }
  }
};

//Supla::Sensor::DisplayAH  *THT_dah;

Supla::ActionHandlerWithCallbacks  *THT_ahcb;

void status_func(int status, const char *msg) {

  if (status != STATUS_REGISTERED_AND_READY) local_mode = true;
  else local_mode = false;
  if (status == STATUS_REGISTER_IN_PROGRESS) screen_saver_on = false;
}

void supla_system_init() {
  
  eeprom.setStateSavePeriod(5000);

  new Supla::Clock;
  
  new Supla::Html::CustomParameter(XIAOMI_CNT,"Ilość czujników (1-9)", 0);

  for (int j = 0; j < MAX_SENSORS; j++) {

    new Supla::Html::CustomTextParameter(XIAOMI_PARAMS[j], "Adres czujnika #", 17);
    new Supla::Html::CustomTextParameter(LOCS_PARAMS[j], "lokalizacja czujnika #", 32);
  }

  Supla::Storage::Init();

  auto cfg = Supla::Storage::ConfigInstance();
  
  #ifdef USE_HARDCODED_DATA

  if (cfg) {
    
    char buf[100];
    
    if (!cfg->getGUID(buf)) {
      cfg->setGUID(USER_GUID);
      cfg->setAuthKey(USER_AUTH_KEY);
      cfg->setWiFiSSID(SUPLA_WIFI_SSID);
      cfg->setWiFiPassword(SUPLA_WIFI_PASS);
      cfg->setSuplaServer(SUPLA_SVR);
      cfg->setEmail(SUPLA_EMAIL);
    }

    cfg->getInt32(XIAOMI_CNT, &Sensors_cnt);
    
    if (Sensors_cnt == 0) { 

      Sensors_cnt = HCD_SENSORS;
      
      cfg->setInt32(XIAOMI_CNT, Sensors_cnt);
      
      for (int j = 0; j < HCD_SENSORS; j++) {

        cfg->setString(XIAOMI_PARAMS[j],xiaomiBleDeviceMacsTEMP[j][0]);
        cfg->setString(LOCS_PARAMS[j],xiaomiBleDeviceMacsTEMP[j][1]);
      }
    }
  }

#endif //USE_HARDCODED_DATA

  if (cfg) {

    if(!cfg->getInt32(XIAOMI_CNT, &Sensors_cnt)) Sensors_cnt = 0;

    for (int j = 0; j < Sensors_cnt; j++) {
      cfg->getString(XIAOMI_PARAMS[j],xiaomiBleDeviceMacs[j],18);
      cfg->getString(LOCS_PARAMS[j],xiaomiBleDeviceLocs[j],31);
    }

    uint32_t t_param_tmp;

    if (cfg->getUInt32(HVAC_T_TIME, &t_param_tmp)) tht_timer_time = (time_t)t_param_tmp;
    if (cfg->getUInt32(HVAC_T_TEMP, &t_param_tmp)) tht_timer_temp  = t_param_tmp;
    if (cfg->getUInt32(BT_SCAN_INTERVAL_PARAM, &t_param_tmp)) btScanInterval  = t_param_tmp;
    if (cfg->getUInt32(BT_SCAN_WINDOW_PARAM, &t_param_tmp)) btScanWindow = t_param_tmp;
    if (cfg->getUInt32(SCRS_INTERVAL_PARAM, &t_param_tmp)) ScreenSaverInterval = t_param_tmp;
}
  
  for (int i = 0; i < Sensors_cnt; i++) {

    sensors_temperature[i]  = -275.0;
    sensors_humidity[i]     = -1;
    sensors_battery[i]      = 255;
    sensors_rssi[i]         = -100;
    sensors_tick[i]         = 0;
  }
  
  NimBLEDevice::setScanFilterMode(CONFIG_BTDM_SCAN_DUPL_TYPE_DEVICE);
  NimBLEDevice::setScanDuplicateCacheSize(200);
  NimBLEDevice::init("TFT_ESP32");

  pBLEScan = NimBLEDevice::getScan(); //create new scan
  // Set the callback for when devices are discovered, no duplicates.
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks(), false);
  pBLEScan->setActiveScan(true); // Set active scanning, this will get more data from the advertiser.
  pBLEScan->setInterval(btScanInterval); // How often the scan occurs / switches channels; in milliseconds,
  pBLEScan->setWindow(btScanWindow);  // How long to scan during the interval; in milliseconds.
  pBLEScan->setMaxResults(0); // do not store the scan results, use callback only.
  
  auto output = new Supla::Control::InternalPinOutput(THT_OUTPUT_PIN);

  ESP32_hvac = new Supla::Control::HvacBaseEE(output);

  htmlHvacParameters.setHvacPtr(ESP32_hvac);

  SHT31 = new Supla::Sensor::SHT3x(0x44);
  SHT31->setInitialCaption("pomiar lokalny");

  //THT_dah   = new Supla::Sensor::DisplayAH(&nm_menu_redraw);

  THT_ahcb  = new Supla::ActionHandlerWithCallbacks();

  DPrograms = new Supla::Sensor::ProgDisplay(0,Sensors_cnt, &sensors_temperature[0]);
  DPrograms->setInitialCaption("aktualny program");
  
  PDstep_mvr = new Supla::Control::Multi_VirtualRelay(DPrograms);
  PDstep_mvr->setInitialCaption("zmiana programu");
  PDstep_mvr->setDefaultFunction(SUPLA_CHANNELFNC_POWERSWITCH);
  PDstep_mvr->addAction(Supla::TURN_ON, THT_ahcb,Supla::ON_CHANGE);

  hvac_therm = new Supla::Sensor::XiaomiCalcThermHygroMeter(DPrograms, &sensors_temperature[0], &sensors_humidity[0], Sensors_cnt, SHT31);

  ESP32_hvac->setMainThermometerChannelNo(4);
  
  for (int i = 0; i < Sensors_cnt; i++) {
    xiaomiSensors[i] = new Supla::Sensor::XiaomiThermHygroMeter(&sensors_temperature[i], &sensors_humidity[i],&sensors_battery[i],&sensors_rssi[i], xiaomiBleDeviceMacs[i]);
    xiaomiSensors[i]->setInitialCaption(xiaomiBleDeviceLocs[i]);
    xiaomiSensors[i]->getChannel()->setFlag(SUPLA_CHANNEL_FLAG_CHANNELSTATE);
  }

  ESP32_hvac->addAction(Supla::TURN_ON, THT_ahcb,Supla::ON_HVAC_MODE_OFF);
  ESP32_hvac->addAction(Supla::TURN_ON, THT_ahcb,Supla::ON_HVAC_MODE_HEAT);
  ESP32_hvac->addAction(Supla::TURN_ON, THT_ahcb,Supla::ON_HVAC_WEEKLY_SCHEDULE_ENABLED);
  ESP32_hvac->addAction(Supla::TURN_ON, THT_ahcb,Supla::ON_HVAC_WEEKLY_SCHEDULE_DISABLED);
  ESP32_hvac->addAction(Supla::TURN_ON, THT_ahcb,Supla::ON_HVAC_STANDBY);
  ESP32_hvac->addAction(Supla::TURN_ON, THT_ahcb,Supla::ON_HVAC_HEATING);
  ESP32_hvac->addAction(Supla::TURN_ON, THT_ahcb,Supla::ON_CHANGE);

  ESP32_hvac->allowWrapAroundTemperatureSetpoints();

  ESP32_hvac->getChannel()->setDefault(SUPLA_CHANNELFNC_HVAC_THERMOSTAT);

  //ESP32_hvac->setDefaultTemperatureRoomMin(SUPLA_CHANNELFNC_HVAC_THERMOSTAT, 0);

  //new Supla::M5RTC(&M5Dial.Rtc);
  
  SuplaDevice.setStatusFuncImpl(&status_func);

  SuplaDevice.setSuplaCACert(suplaCACert);
  SuplaDevice.setSupla3rdPartyCACert(supla3rdCACert);
  
  SuplaDevice.setName("TFT_ESP32 Supla thermostat");
  //wifi.enableSSL(true);

  SuplaDevice.setActivityTimeout(240);

  SuplaDevice.begin();
  
  ScreenSaverTimer    = millis();
  ssScanIntervalTimer = millis();
  MenuRedrawTimer     = millis();
}

void supla_system_loop() {

  SuplaDevice.iterate();
  
  for (int i = 0; i < Sensors_cnt; i++)
    if (sensors_tick[i] > 5) {

      sensors_temperature[i]  =  -275;
      sensors_humidity[i]     =  -1;
      sensors_battery[i]      =   255;
      sensors_rssi[i]         =  -100;
      sensors_tick[i]         =   0;
    }
  

  if ((millis() - ssScanIntervalTimer) > (ssScanInterval * 1000)) {
     
    ssScanIntervalTimer = millis();

    for (int i = 0; i < Sensors_cnt; i++) {
      xiaomiSensors[i]->setTemp(sensors_temperature[i]);
      xiaomiSensors[i]->setHumi(sensors_humidity[i]);
      sensors_tick[i] = sensors_tick[i] + 1;
    }

    nm_menu_redraw = true;
  }

  if (ESP32_hvac->isCountdownEnabled() && ((millis() - MenuRedrawTimer) > 1000)) {

    MenuRedrawTimer = millis();
    nm_menu_redraw  = true;
  }
  
//BLE section
if(pBLEScan->isScanning() == false) {
      // Start scan with: duration = 0 seconds(forever), no scan end callback, not a continuation of a previous scan.
    pBLEScan->start(0, nullptr, false);}

//end of BLE section
    
      ScreenSaverTimer = millis();
      
      if (screen_saver_on) {
        screen_saver_on = false;
        nm_menu_redraw  = true;
      }

  if ((ScreenSaverInterval > 0) && (!screen_saver_on)) {
    if (millis() - ScreenSaverTimer > (ScreenSaverInterval * 1000)) screen_saver_on = true;
  }
}

short supla_hvac_getPrimaryTemp()
{
    return  ESP32_hvac->getPrimaryTemp();
}

short supla_hvac_getTemperatureRoomMin()
{
    return ESP32_hvac->getTemperatureRoomMin();
}

short supla_hvac_getTemperatureRoomMax()
{
    return  ESP32_hvac->getTemperatureRoomMax();
}

short supla_hvac_getTemperatureHisteresis()
{
    return  ESP32_hvac->getTemperatureHisteresis();
}

short supla_hvac_getTemperatureSetpointHeat()
{
    return  ESP32_hvac->getTemperatureSetpointHeat();
}

short supla_hvac_getTemperatureHisteresisMin()
{
    return  ESP32_hvac->getTemperatureHisteresisMin();
}

short supla_hvac_getTemperatureHisteresisMax()
{
    return  ESP32_hvac->getTemperatureHisteresisMax();
}

short supla_hvac_getTemperatureFreezeProtection()
{
    return  ESP32_hvac->getTemperatureFreezeProtection();
}

short supla_hvac_getTemperatureHeatProtection()
{
    return  ESP32_hvac->getTemperatureHeatProtection();
}

short supla_hvac_getMinOnTimeS()
{
    return  ESP32_hvac->getMinOnTimeS();
}

short supla_hvac_getMinOffTimeS()
{
    return  ESP32_hvac->getMinOffTimeS();
}

bool  supla_hvac_isWeeklyScheduleEnabled()
{
  return  ESP32_hvac->isWeeklyScheduleEnabled();
}

bool  supla_hvac_isManualModeEnabled()
{
    return  ESP32_hvac->isManualModeEnabled();
}

bool  supla_hvac_isCountdownEnabled()
{
    return  ESP32_hvac->isCountdownEnabled();
}

bool  supla_hvac_isThermostatDisabled()
{
    return  ESP32_hvac->isThermostatDisabled();
}

bool  supla_hvac_isTemperatureSetpointChangeSwitchesToManualMode()
{
    return  ESP32_hvac->isTemperatureSetpointChangeSwitchesToManualMode();
}

bool  supla_hvac_isAntiFreezeAndHeatProtectionEnabled()
{
    return  ESP32_hvac->isAntiFreezeAndHeatProtectionEnabled();
}

void  supla_hvac_IncreaseHeatingTemperature()
{
    ESP32_hvac->handleAction(0, Supla::INCREASE_HEATING_TEMPERATURE);
}

void  supla_hvac_DecreaseHeatingTemperature()
{
    ESP32_hvac->handleAction(0, Supla::DECREASE_HEATING_TEMPERATURE);

}

void  supla_hvac_ToggleOffManualWeeklyScheduleModes()
{
    ESP32_hvac->handleAction(0,Supla::TOGGLE_OFF_MANUAL_WEEKLY_SCHEDULE_MODES);
}

void  supla_hvac_setTemperatureSetpointHeat(short temperature)
{
    ESP32_hvac->setTemperatureSetpointHeat(temperature);
}

void  supla_hvac_setTemperatureHisteresis(short temperature)
{
    ESP32_hvac->setTemperatureHisteresis(temperature);
}

void  supla_hvac_setTemperatureRoomMin(short temperature)
{
    ESP32_hvac->setTemperatureRoomMin(temperature);
}


void  supla_hvac_setTemperatureRoomMax(short temperature)
{
    ESP32_hvac->setTemperatureRoomMax(temperature);
}


void  supla_hvac_setTemperatureHisteresisMin(short temperature)
{
    ESP32_hvac->setTemperatureHisteresisMin(temperature);
}

void  supla_hvac_setTemperatureHisteresisMax(short temperature)
{
    ESP32_hvac->setTemperatureHisteresisMax(temperature);
}

void  supla_hvac_setTemperatureSetpointChangeSwitchesToManualMode(bool enabled)
{
    ESP32_hvac->setTemperatureSetpointChangeSwitchesToManualMode(enabled);
}

void  supla_hvac_setAntiFreezeAndHeatProtectionEnabled(bool enabled)
{
    ESP32_hvac->setAntiFreezeAndHeatProtectionEnabled(enabled);
}

void  supla_hvac_setTemperatureFreezeProtection(short temperature)
{
    ESP32_hvac->setTemperatureFreezeProtection(temperature);
}

void  supla_hvac_setTemperatureHeatProtection(short temperature)
{
    ESP32_hvac->setTemperatureHeatProtection(temperature);
}

void  supla_hvac_setMinOnTimeS(short seconds)
{
    ESP32_hvac->setMinOnTimeS(seconds);
}

void  supla_hvac_setMinOffTimeS(short seconds)
{
    ESP32_hvac->setMinOffTimeS(seconds);
}

void  supla_hvac_setActionHandlerCallback(_actionhandler_callback actionhandler_callback)
{
    THT_ahcb->setActionHandlerCallback(actionhandler_callback);   
}