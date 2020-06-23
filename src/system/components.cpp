#include "components.h"


namespace SimpleScheduler {

    //collect results as JSON
    void getResults(String &res) {
        SPS30::results(res);
    }

    //did all API collect data?
    void afterSendData(bool status) {
        SPS30::afterSend(status);
    }

    //collect HTML table with current results
    void getResultsAsHTML(String &res) {
        SPS30::resultsAsHTML(res);
    }

    //prepare forms with configuration
    String selectConfigForm(LoopEntryType sensor) {
        String s = F("");
        switch (sensor) {
            case SimpleScheduler::SPS30:
                return SPS30::getConfigHTML();
            default:
                return s;
        }
    }

    String parseHTTPConfig(LoopEntryType sensor) {
        String s = F("");
        switch (sensor) {
            case SimpleScheduler::SPS30:
                return SPS30::parseHTTPRequest();
            default:
                return s;
        }
    }

    const __FlashStringHelper *findSlotDescription(LoopEntryType sensor) {
        switch (sensor) {
            case SimpleScheduler::SPS30:
                return FPSTR(INTL_SPS30_SENSOR_DESC);
            default:
                return F("");
        }

    }
}

