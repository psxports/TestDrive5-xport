#include "startup.h"
#include "presentation_render.h"

STARTUP_BUFFER startup_buffer;

void startup_show_images(void)
{
    FUNCTION_MARKER(0x8001A0DC, "SLUS_006.10");
    presentation_show_image(startup_buffer.bytes, sizeof(startup_buffer.bytes), "PRES\\LEGAL1.TIM", 6, 1);
    presentation_show_image(startup_buffer.bytes, sizeof(startup_buffer.bytes), "PRES\\LEGAL2.TIM", 1, 1);
}
