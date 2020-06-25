//
// Created by viciu on 15.06.2020.
//

#ifndef NAMF_COMPONENTS_H
#define NAMF_COMPONENTS_H
#include <Arduino.h>
#include "../sensors/sps30/sensor.h"

namespace SimpleScheduler {

    //collect results as JSON
    void getResults( String &res);

    //did all API collect data?
    void afterSendData(bool status);

    //collect HTML table with current results
    void getResultsAsHTML( String &res);

    String parseHTTPConfig (LoopEntryType sensor);

    String getConfigJSON( LoopEntryType sensor);
}


#endif //NAMF_COMPONENTS_H
