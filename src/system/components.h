//
// Created by viciu on 15.06.2020.
//

#ifndef NAMF_COMPONENTS_H
#define NAMF_COMPONENTS_H
#include <Arduino.h>

namespace SimpleScheduler {
    String getResults( String res) {
        res += SPS30_results();
    }
}


#endif //NAMF_COMPONENTS_H
