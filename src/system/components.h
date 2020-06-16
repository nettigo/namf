//
// Created by viciu on 15.06.2020.
//

#ifndef NAMF_COMPONENTS_H
#define NAMF_COMPONENTS_H
#include <Arduino.h>
#include "sensors/sensirion_sps30.h"

namespace SimpleScheduler {

    //collect results as JSON
    void getResults( String &res) {
        SPS30::results(res);
    }

    //did all API collect data?
    void afterSendData(bool status) {
        SPS30::afterSend(status);
    }

    //collect HTML table with current results
    void getResultsAsHTML( String &res) {
        SPS30::resultsAsHTML(res);
    }

}


#endif //NAMF_COMPONENTS_H
