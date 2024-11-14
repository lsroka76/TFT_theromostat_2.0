#include "gfx_system.h"
#include "supla_system.h"
#include "ui.h"
#include "lvgl_port_v8.h"

void value_changed_event_cb(lv_event_t * e);

String getHeapMemory(){
char temp[300];
sprintf(temp, "Heap: Free:%i, Min:%i, Size:%i, Alloc:%i", 
ESP.getFreeHeap(), ESP.getMinFreeHeap(), ESP.getHeapSize(), ESP.getMaxAllocHeap());
return temp;
}

String getPsram(){
char temp[300];
sprintf(temp, "PSRAM (total): %i \nFree PSRAM: %i \nMinPSR: %i \nAllocPSR: %i", 
ESP.getPsramSize(), ESP.getFreePsram(), ESP.getMinFreePsram(), ESP.getMaxAllocPsram());
return temp;
}


long rftime;


void ui_callback(int event, int action)
{
    Serial.println("Thermostat state changed"); 
    float temperature = supla_hvac_getTemperatureSetpointHeat()/100; 

    lvgl_port_lock(-1);

    lv_arc_set_value(ui_THTarc,temperature);
    lv_label_set_text_fmt(ui_shL,  "%.2F °C", temperature);
    lv_label_set_text_fmt(ui_rminL, "%2.1F °C", ((float)(supla_hvac_getTemperatureRoomMin())/100));
    lv_label_set_text_fmt(ui_rmaxL, "%2.1F °C", ((float)(supla_hvac_getTemperatureRoomMax())/100));
    lv_label_set_text_fmt(ui_ptL, "%.1F °C", ((float)(supla_hvac_getPrimaryTemp())/100));
    lv_arc_set_value(ui_THTarc2,supla_hvac_getPrimaryTemp());  

    lvgl_port_unlock();
}

void setup()
{
  Serial.begin(115200);
  // while (!Serial);
  supla_system_init();

  gfx_system_init();

  lvgl_port_lock(-1);

  ui_init();
  plus_Animation(ui_Image1,0);

  lvgl_port_unlock();

  supla_hvac_setActionHandlerCallback(&ui_callback);
  
  short temperature = supla_hvac_getTemperatureSetpointHeat();
  
  lvgl_port_lock(-1);
  
    lv_arc_set_range(ui_THTarc,supla_hvac_getTemperatureRoomMin(),supla_hvac_getTemperatureRoomMax());
    lv_arc_set_range(ui_THTarc2,supla_hvac_getTemperatureRoomMin(),supla_hvac_getTemperatureRoomMax());
  
   
    lv_arc_set_value(ui_THTarc,temperature);
    lv_label_set_text_fmt(ui_shL,  "%2.1F °C", ((float)(temperature)/100));
    lv_label_set_text_fmt(ui_rminL, "%2.1F °C", ((float)(supla_hvac_getTemperatureRoomMin())/100));
    lv_label_set_text_fmt(ui_rmaxL, "%2.1F °C", ((float)(supla_hvac_getTemperatureRoomMax())/100));
    lv_label_set_text_fmt(ui_ptL, "%2.1F °C", ((float)(supla_hvac_getPrimaryTemp())/100));
    lv_arc_set_value(ui_THTarc2,supla_hvac_getPrimaryTemp());

    lv_obj_add_event_cb(ui_THTarc, value_changed_event_cb, LV_EVENT_VALUE_CHANGED, ui_shL);

  lvgl_port_unlock();

  rftime = millis();
}

void loop()
{
  supla_system_loop();
    
  //lv_timer_handler(); /* let the GUI do its work */
  //delay(5);
  if (millis() - rftime > 10000) {
    lvgl_port_lock(-1);
    
      lv_arc_set_value(ui_THTarc2,supla_hvac_getPrimaryTemp());
    
    lvgl_port_unlock();

    Serial.println(ESP.getMinFreeHeap()); 
    Serial.println(ESP.getPsramSize());
    Serial.println(ESP.getFreePsram());
    Serial.println(ESP.getMinFreePsram());
    Serial.println(ESP.getMaxAllocPsram());

    rftime = millis(); }

}

void value_changed_event_cb(lv_event_t * e)
{
    lv_obj_t * arc = (lv_obj_t *)lv_event_get_target(e);
    int32_t arc_value = lv_arc_get_value(arc);
    Serial.println(arc_value);
    if (arc_value != supla_hvac_getTemperatureSetpointHeat()){
      Serial.println("adjusting temp");
      supla_hvac_setTemperatureSetpointHeat(arc_value);
    }
    lv_obj_t * label = (lv_obj_t *)lv_event_get_user_data(e);

    lv_label_set_text_fmt(label, "%2.1F °C", (float)(arc_value/100));

    /*Rotate the label to the current position of the arc*/
    //lv_arc_rotate_obj_to_angle(arc, label, 25);
}