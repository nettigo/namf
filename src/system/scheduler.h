//
// Created by viciu on 04.06.2020.
//

#ifndef NAMF_SCHEDULER_H
#define NAMF_SCHEDULER_H
#include <Arduino.h>
#include <ArduinoJson.h>
#define SCHEDULER_SIZE  10

namespace SimpleScheduler {

    typedef enum {
        EMPTY,
        SPS30,
        NTW_WTD,
        SHT3x,
        NAMF_LOOP_SIZE
    } LoopEntryType;

    LoopEntryType operator++(LoopEntryType &entry, int);

    typedef enum {
        INIT,
        RUN,
        STOP,
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
        const __FlashStringHelper *slotCode;
    };

    String selectConfigForm(LoopEntryType sensor);

    void getConfigForms(String &page);
    void getConfigJSON(String &json);
    void readConfigJSON(LoopEntryType sensor, JsonObject& json);
    void readConfigJSON(JsonObject& json);

    const __FlashStringHelper *findSlotDescription(LoopEntryType sensor);
    const __FlashStringHelper *findSlotKey(LoopEntryType sensor);

    class NAMFScheduler {
    public:
        NAMFScheduler();

        void process(void);

        void init(void);
        void init(LoopEntryType);

        /******************************************************
         * register sensor/subsytem to to be run by SimpleScheduler
         * slot - enum from SimpleScheduler::LoopEntryType - identifies sensor
         * processF - function called by scheduler with current status (init/normal run)
         * code - key used to store configuration in JSON, suggested practice - use the same name as for enum LoopEntryType, just lowercase
         * name - name of sensor/subsytem. Will be used to display configuration checkbox to enable/disable subsystem
         */
        int registerSensor(LoopEntryType slot, loopTimerFunc processF, const __FlashStringHelper *code);
        int unregisterSensor(LoopEntryType slot);
        bool isRegistered(LoopEntryType);

        void getConfigForms(String &page);
        void getConfigJSON(LoopEntryType);
        void getConfigJSON(String &json);

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
