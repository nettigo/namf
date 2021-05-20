//
// Created by viciu on 15.06.2020.
//

#ifndef NAMF_COMPONENTS_H
#define NAMF_COMPONENTS_H
#include <Arduino.h>
#include "variables.h"
#include "../sensors/sds011/sds011.h"
#include "../sensors/heca/heca.h"
#include "../sensors/sps30/sensor.h"
#include "../sensors/network_watchdog/tester.h"
#include "../sensors/sht3x/sht3x.h"
#include "../sensors/mhz14a/winsen-mhz.h"

namespace SimpleScheduler {

    //has sensor enabled dispalying in config?
    bool sensorWantsDisplay(LoopEntryType);

    //collect results as JSON
    void getResults( String &res);

    //push data to LuftDaten/SC
    void sendToSC();

    //did all API collect data?
    void afterSendData(bool status);

    //collect HTML table with current results
    void getResultsAsHTML(String &res);

    //collect HTML table for status page
    void getStatusReport(String &res);

    JsonObject &parseHTTPConfig(LoopEntryType sensor);

    String getConfigJSON(LoopEntryType sensor);

    bool displaySensor(SimpleScheduler::LoopEntryType, String(&)[4], byte cols = 0, byte rows = 0, byte minor = 0);

}


#endif //NAMF_COMPONENTS_H
