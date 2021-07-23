//
// Created by viciu on 08.06.2020.
//
#include "scheduler.h"
#include "helpers.h"
#include "components.h"
#include "lang/select_lang.h"


namespace SimpleScheduler {
    const char LET_0 [] PROGMEM = "EMPTY";
    const char LET_1 [] PROGMEM = "SPS30";
    const char LET_2 [] PROGMEM = "NTW_WTD";
    const char LET_3 [] PROGMEM = "SHT3x";
    const char LET_4 [] PROGMEM = "MHZ14A";
    const char LET_5 [] PROGMEM = "SDS011";
    const char LET_6 [] PROGMEM = "HECA";
    const char LET_SIZE [] PROGMEM = "SIZE";

    const char *LET_NAMES[] PROGMEM = {
            LET_0, LET_1, LET_2, LET_3, LET_4, LET_5, LET_6, LET_SIZE
    };

    unsigned long nullF(LoopEventType event) { return 0; }


    NAMFScheduler::NAMFScheduler() {
        loopSize = 0;
#ifdef DBG_NAMF_TIMES
        _runTimeMax = 0;
        _lastRunTime = 0;
        _runTimeMaxSystem = EMPTY;
#endif
        for (byte i = 0; i < SCHEDULER_SIZE; i++) {
            _tasks[i].process = nullF;
            _tasks[i].nextRun = 0;
            _tasks[i].slotID = EMPTY;

        }
    }

    void NAMFScheduler::process() {
        for (byte i = 0; i < SCHEDULER_SIZE; i++) {
            yield();    // let internals run
            //run if not EMPTY slot, has set nextRun and time has passed
            if (_tasks[i].slotID && _tasks[i].nextRun && _tasks[i].nextRun < millis()) {
                unsigned long startTime = micros();
                unsigned long nextRun = _tasks[i].process(RUN);
#ifdef DBG_NAMF_TIMES
                _lastRunTime = micros() - startTime;
                if (( _lastRunTime) > 1000*1000) {
                    Serial.printf("Long run time for sensor %s (%lu ms)\n", LET_NAMES[_tasks[i].slotID], startTime/1000);
                }
#endif
                if (nextRun) {
                    _tasks[i].nextRun = millis() + nextRun;
                } else {
                    _tasks[i].nextRun = 0;
                }
#ifdef DBG_NAMF_TIMES
                if (_lastRunTime > _runTimeMax) {
                            _runTimeMax = _lastRunTime;
                            _runTimeMaxSystem = _tasks[i].slotID;
                }
#endif
            }

        }

    }

    void NAMFScheduler::init(LoopEntryType slot) {
        if (slot == EMPTY) return;
        int i = findSlot(slot);
        if (i < 0) return;
        unsigned long nextRun = _tasks[i].process(INIT);
        if (nextRun) {
            _tasks[i].nextRun = millis() + nextRun;
        } else {
            _tasks[i].nextRun = 0;
        }


    }

    void NAMFScheduler::getConfigForms(String &page) {
        String s = F("");
        page += F("<div id='ncf'>");
        LoopEntryType i = EMPTY;
        i++;
        for (; i < NAMF_LOOP_SIZE; i++) {
            String templ = F(
                    "<form method='POST' action='/simple_config?sensor={sensor}' style='width:100%;'>\n"
            );
            templ += F("<hr/><h2>");
            templ += findSlotDescription(i);
            templ += F("</h2>");
            boolean checked = findSlot(i) >= 0; // check if sensor is enabled
            templ += form_checkbox(F("enabled-{sensor}"), FPSTR(INTL_ENABLE), checked, true);
            templ += F("<br/>");
            String lines[] = {"","","",""};
            if (SimpleScheduler::displaySensor(i,lines)) {
                checked = sensorWantsDisplay(i);
                templ += form_checkbox(F("display-{sensor}"), FPSTR(INTL_DISPLAY_NEW), checked, true);
                templ += F("<div class='spacer'></div>");
            }
            //HTML to enable/disable given sensor

            s = SimpleScheduler::selectConfigForm(i);
            templ += F("{body}<input type='submit' value='");
            templ += FPSTR(INTL_SAVE);
            templ += F("'/></form>\n");
            templ.replace(F("{sensor}"), String(i));
            templ.replace(F("{body}"), s);
            page += templ;

        }
        page += F("</div>");
    }

    void NAMFScheduler::unregisterSensor(LoopEntryType slot) {
        int i = findSlot(slot);
        if (i < 0) return;
        loopSize--;
        _tasks[i].slotID = EMPTY;
        _tasks[i].process(STOP);
        return;

    }

    void NAMFScheduler::enableSubsystem(LoopEntryType subsyst, bool enabled, loopTimerFunc initFunc,
                                        const __FlashStringHelper *code) {
        if (enabled) {
            if (!scheduler.isRegistered(subsyst)) {
                scheduler.registerSensor(subsyst, initFunc, code);
                scheduler.init(subsyst);
            }
        } else {
            scheduler.unregisterSensor(subsyst);
        }
    };

    bool NAMFScheduler::isRegistered(LoopEntryType slot) {
        return findSlot(slot) >= 0;
    }

    //how many slots for sensors is available in total
    byte NAMFScheduler::sensorSlots(void) {
        return SCHEDULER_SIZE;
    };
    //how many free slots for
    byte NAMFScheduler::freeSlots(void){
        return SCHEDULER_SIZE - loopSize;
    };

    String NAMFScheduler::registeredNames(){
        String t = F("");

        for (byte i = 0; i < SCHEDULER_SIZE; i++) {
            if (_tasks[i].slotID != EMPTY) {
                t += FPSTR(LET_NAMES[_tasks[i].slotID]);
                t += F(" ");
            }
        }
        return t;

    }

#ifdef DBG_NAMF_TIMES
    String NAMFScheduler::maxRunTimeSystemName(){

            String t  = F("");
            t+= FPSTR(LET_NAMES[scheduler.timeMaxSystem()]);
            return t;

        }
#endif

    //takes which screen to display and goes through list of all "display" sensor,
    //counting which one is current. *minor* returns which screen from singe sensor should
    //be displayed (sensor can register more than one screen to display)
    LoopEntryType NAMFScheduler::selectSensorToDisplay(byte current_pos, byte &minor) {
        byte sum = 0;
        minor = 0;
        for (byte i = 0; i < SCHEDULER_SIZE; i++) {

            if (sum + _tasks[i].hasDisplay > current_pos) {
                minor = current_pos - sum;
                return _tasks[i].slotID;
            }
            sum += _tasks[i].hasDisplay;
        };
        return EMPTY;

    }

    //inform scheduler that we want to display data on LCD
    int NAMFScheduler::registerDisplay(LoopEntryType slot, byte screens) {
        int i = findSlot(slot);
        if (i < 0) return -1;
        _tasks[i].hasDisplay = screens;
        return 0;
    }

    //how many screens is being registered
    unsigned NAMFScheduler::countScreens(void) {
        unsigned sum = 0;
        for (byte i = 0; i < SCHEDULER_SIZE; i++) {
            if (_tasks[i].slotID == EMPTY) continue;
            sum += _tasks[i].hasDisplay;

        }
        return sum;
    }

    //register new sensor
    int NAMFScheduler::registerSensor(LoopEntryType slot, loopTimerFunc processF, const __FlashStringHelper *code) {
        {
            if (loopSize + 1 >= SCHEDULER_SIZE)
                return -1;
            int i = findSlot(slot);
            //no sensor yet
            if (i < 0) i = findSlot(EMPTY);
            if (i < 0) { return -1; };
            _tasks[i].nextRun = 0;
            _tasks[i].process = processF;
            _tasks[i].slotID = slot;
            _tasks[i].slotCode = code;

            loopSize += 1;
            //return idx
            return loopSize - 1;
        };
    }

//find scheduler entry based on sensor type (slot ID)
    int NAMFScheduler::findSlot(byte id) {
        for (byte i = 0; i < SCHEDULER_SIZE; i++) {
            if (_tasks[i].slotID == id)
                return i;
        }
        //no match
        return -1;

    }

    void NAMFScheduler::dumpTable() {
        for (byte i = 0; i < SCHEDULER_SIZE; i++) {
            Serial.print(i);
            Serial.print(",");
            Serial.print(_tasks[i].slotID);
            Serial.print(",");
            Serial.print(_tasks[i].nextRun);
            Serial.print(",");
            Serial.println(_tasks[i].hasDisplay);
        }
    }


    void NAMFScheduler::runIn(byte slot, unsigned long time, loopTimerFunc func) {
        int idx;
        idx = findSlot(slot);
        if (idx < 0) return;

        if (time > 0) {
            _tasks[idx].nextRun = millis() + time;
        } else {
            _tasks[idx].nextRun = 0;
        }
        _tasks[idx].process = func;
    };

    void NAMFScheduler::runIn(byte slot, unsigned long time) {
        int idx;
        idx = findSlot(slot);
        if (idx < 0) return;

        if (time > 0) {
            _tasks[idx].nextRun = millis() + time;
        } else {
            _tasks[idx].nextRun = 0;
        }

    }

    LoopEntryType operator++(LoopEntryType &entry, int) {
        LoopEntryType current = entry;
        if (NAMF_LOOP_SIZE < entry + 1) entry = NAMF_LOOP_SIZE;
        else entry = static_cast<LoopEntryType>( entry + 1);
        return (current);
    }

}
