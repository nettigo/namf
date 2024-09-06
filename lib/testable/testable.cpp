#include "testable.h"

int hourIsInRange(int hour, int start, int stop) {
    if (start > 24 || stop > 24 ) return false;
    bool ret = false;
    if (start < stop) {
        // no ret over midnight eg stp:22 start:6 -
        if (hour >= start && hour <= stop)
            ret = true;
        else
            ret = false;
    } else {
        if (hour <= start && hour >= stop)
            ret = false;
        else
            ret = true;
    }
    return ret;
}


