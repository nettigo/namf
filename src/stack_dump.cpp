#include "stack_dump.h"

extern "C" void custom_crash_callback(struct rst_info *rst_info, uint32_t stack, uint32_t stack_end) {
#ifdef ARDUINO_ARCH_ESP8266
    cont_t g_cont __attribute__ ((aligned (16)));
    File out;
    boolean dump = true;
    noInterrupts();
    out = SPIFFS.open("/stack_dump", "w");
    time_t now = time(nullptr);
    char tmp[48];
    strncpy(tmp, ctime(&now), 47);
    out.print(tmp);
    out.println(SOFTWARE_VERSION);
    out.println(INTL_LANG);
    out.println(ESP.getSketchMD5());
    out.println();
    uint32_t cont_stack_start = (uint32_t) &(g_cont.stack);
    uint32_t cont_stack_end = (uint32_t) g_cont.stack_end;
    uint32_t offset = 0;
    switch (rst_info->reason) {
        case REASON_SOFT_WDT_RST:
            offset = 0x1b0;
            out.println(F("Soft WDT reset"));
            break;
        case REASON_EXCEPTION_RST:
            offset = 0x1a0;
            snprintf(tmp, 47, "Exception (%i):\nepc1=0x%08x epc2=0x%08x",
                     rst_info->exccause, rst_info->epc1, rst_info->epc2
            );
            out.print(tmp);
            snprintf(tmp, 47, " epc3=0x%08x excvaddr=0x%08x", rst_info->epc2, rst_info->excvaddr);
            out.print(tmp);
            snprintf(tmp, 47, " depc=0x%08x", rst_info->depc);
            out.println(tmp);
            break;
        case REASON_WDT_RST:
            offset = 0x10;
            out.println(F("wdt_reset"));
            dump = false;
            break;
        default:
            snprintf(tmp,47,"Unknown reset cause: %i", rst_info->reason);
            out.println(tmp);
    }

    out.println(F("\n>>>stack>>>\n"));
    if (stack - offset > cont_stack_start && stack - offset < cont_stack_end) {
        out.println("ctx: cont");
    } else {
        out.println("ctx: sys");
    }

    //Hardware WDT reset does not dump stack on serial...
    if (dump) {
        snprintf(tmp, 47, "sp: %08x end: %08x offset: %04x\n", stack - offset, stack_end, offset);
        out.print(tmp);
        for (uint32_t pos = stack; pos < stack_end; pos += 0x10) {
            uint32_t *values = (uint32_t *) (pos);
            //rough indicator: stack frames usually have SP saved as the second word
            bool looksLikeStackFrame = (values[2] == pos + 0x10);
            snprintf(tmp, 47, "%08x:  %08x %08x %08x %08x %c", pos, values[0], values[1], values[2], values[3],
                     (looksLikeStackFrame) ? '<' : ' ');
            out.println(tmp);
        }
        out.println(F("<<<stack<<<\n\n\n\n\n\n"));
    }
    out.close();
    interrupts();
    delay(30);
#endif
}