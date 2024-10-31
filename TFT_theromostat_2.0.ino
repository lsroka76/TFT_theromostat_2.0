#include "gfx_system.h"
#include "ui.h"

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

void setup()
{
  Serial.begin(115200);
  // while (!Serial);
    init_gfx_system();  
    ui_init();
    rftime = millis();
  }
}

void loop()
{
  lv_timer_handler(); /* let the GUI do its work */
  delay(5);
  if (millis() - rftime > 10000) {
    lv_textarea_set_text(ui_TextArea1, getPsram().c_str());
    Serial.println(ESP.getMinFreeHeap());
    //Serial.println(ESP.getMaxAllocHeap()); 
    Serial.println(ESP.getPsramSize());
    Serial.println(ESP.getFreePsram());
    Serial.println(ESP.getMinFreePsram());
    Serial.println(ESP.getMaxAllocPsram());

    rftime = millis(); }

}
