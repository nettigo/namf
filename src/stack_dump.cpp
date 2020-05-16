#include "stack_dump.h"

extern "C" void custom_crash_callback(struct rst_info *rst_info, uint32_t stack, uint32_t stack_end) {
    register uint32_t sp asm("a1");
    cont_t g_cont __attribute__ ((aligned (16)));
    File out;

    out = SPIFFS.open("stack_dump", "w");
    time_t now = time(nullptr);
    char tmp[48];
    strncpy(tmp, ctime(&now), 47);
    out.print(tmp);

    uint32_t cont_stack_start = (uint32_t) &(g_cont.stack);
    uint32_t cont_stack_end = (uint32_t) g_cont.stack_end;
    uint32_t stack_end2 = stack_end;
    uint32_t offset = 0;
    if (rst_info->reason == REASON_SOFT_WDT_RST) {
        offset = 0x1b0;
    } else if (rst_info->reason == REASON_EXCEPTION_RST) {
        offset = 0x1a0;
    } else if (rst_info->reason == REASON_WDT_RST) {
        offset = 0x10;
    }
    if (stack > cont_stack_start && stack < cont_stack_end) {
        out.print("ctx: cont");
    } else {
        out.print("ctx: sys");
    }


    snprintf(tmp, 47, "sp: %08x end: %08x offset: %04x\n", stack, stack_end, offset);
//    getStack(stack, stack_end, out);
    for (uint32_t pos = stack; pos < stack_end; pos += 0x10) {
        uint32_t *values = (uint32_t *) (pos);
        //rough indicator: stack frames usually have SP saved as the second word
        bool looksLikeStackFrame = (values[2] == pos + 0x10);
        snprintf(tmp,47, "%08x:  %08x %08x %08x %08x %c", pos, values[0], values[1], values[2], values[3],
                (looksLikeStackFrame) ? '<' : ' ');
        out.print(tmp);
    }
    out.close();

}