//
// Created by viciu on 08.06.2020.
//

#ifndef NAMF_SPS30_H
#define NAMF_SPS30_H

#include "variables.h"
#include "system/scheduler.h"
#include "helpers.h"
#include <sps30.h>
namespace SPS30 {
    unsigned long init(SimpleScheduler::LoopEventType e) {
        debug_out("************** SPS30 init", DEBUG_MIN_INFO, true);
        return 10*1000;
    }

    unsigned long process(SimpleScheduler::LoopEventType e) {
        switch (e) {
            case SimpleScheduler::INIT:
                break;
        }
        debug_out("************** SPS30 process", DEBUG_MIN_INFO, true);
        return 15*1000;
    }
    String results(void) {
        String s("\"SPS30\":{\"PM10\":0.20}");
        return s;
    }

}

#endif //NAMF_SPS30_H
