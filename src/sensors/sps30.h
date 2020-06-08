//
// Created by viciu on 08.06.2020.
//

#ifndef NAMF_SPS30_H
#define NAMF_SPS30_H

#include "variables.h"
#include "helpers.h"

unsigned long SPS30_init(void) {
    debug_out("************** SPS30 init", DEBUG_MIN_INFO, true);
    return 10*1000;
}

unsigned long SPS30_process(void) {
    debug_out("************** SPS30 process", DEBUG_MIN_INFO, true);
    return 15*1000;
}

#endif //NAMF_SPS30_H
