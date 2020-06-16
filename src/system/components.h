//
// Created by viciu on 15.06.2020.
//

#ifndef NAMF_COMPONENTS_H
#define NAMF_COMPONENTS_H
#include <Arduino.h>
#include "sensors/sensirion_sps30.h"

namespace SimpleScheduler {
    void getResults( String &res) {
        SPS30::results(res);
    }
}


#endif //NAMF_COMPONENTS_H
