//
// Created by viciu on 04.06.2020.
//

#ifndef NAMF_SCHEDULER_H
#define NAMF_SCHEDULER_H
#include <Arduino.h>

#define SCHEDULER_SIZE  10

namespace SimpleScheduler {

    typedef enum {
        EMPTY,
        SPS30,
        NAMF_LOOP_SIZE
    } LoopEntryType;

    typedef enum {
        INIT,
        RUN,
        LOOP_EVENT_TYPE_SIZE
    } LoopEventType;

    class NAMFSchedulerEntry {
    public:
        NAMFSchedulerEntry() { nextRun = 0; };

        void init(void) {}

        unsigned long process(void);

    private:
        unsigned long nextRun;
    };

    typedef unsigned long (*loopTimerFunc)(LoopEventType);

    extern unsigned long nullF(LoopEventType);


    struct LoopEntry {
        loopTimerFunc process;
        unsigned long nextRun;
        LoopEntryType slotID;
    };


    class NAMFScheduler {
    public:
        NAMFScheduler();

        void process(void);

        void init(void);

        int registerSensor(byte slot,loopTimerFunc processF);

        void runIn(byte slot, unsigned long time, loopTimerFunc func);

        void runIn(byte slot, unsigned long time);

    private:
        LoopEntry _tasks[SCHEDULER_SIZE];
        byte loopSize;

        int findSlot(byte id);
    };
/*
NAMFScheduler scheduler;

//przykłady użycia

typedef enum {
    IDLE,
    HEATING,
    POSTPROCESIN
} SHT30_STATES;

//funkcja robiąca odczyty, przygotująca
unsigned long SHT30_processF(void) {
    static SHT30_STATE STATE = IDLE;

    //zrób pomiar, zapisz w tablicy
    //kod .........
    switch (STATE) {
        case IDLE:
            STATE = HEATING;
            enableHeatingPlateOnSHT();
            //dokonaj pomiaru za 10 ms
            return 10;
        case HEATING:
            callMeasurementOnSHT30();
            STATE = POSTPROCESIN;
            //posprzątaj za 5 ms
            return 5;
        case POSTPROCESIN:
            STATE = IDLE;
            //jakieś czynności post pomiarowe

            //następny pomiar za 5 minut, więc wróc za 5min-10s
            return 290;

    }
}

unsigned long SHT30_initF(void) {
    // kod ....

    // pierwsze wywołanie za 2000 ms
    return 2000;
}

String SHT30_resultF(void) {
    //zwróc bieżący odczyt
    return String.
    new("{SHT30_temp:20.1,SHT30_hum:12.34}");
}

void config_scheduler() {
    scheduler.registerSensor(SHT_30, SHT30_initF, SHT30_processF, SHT30_resultF);
}

void process_sheduler() {
    scheduler.process();
}
*/

}

#endif //NAMF_SCHEDULER_H
