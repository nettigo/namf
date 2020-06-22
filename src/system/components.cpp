#include "system/components.h"


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

