/* sgui - replacement IEM-style GUI objects for Pure Data
 *
 * sgui.c - external entry point.
 *
 * Registers the sgui objects in one shared object:
 *   sbng  resizable "smart" bang
 *   svsl  vertical slider   (also shadows pd's built-in "vsl")
 *   shsl  horizontal slider (also shadows pd's built-in "hsl")
 *   stgl  toggle            (also shadows pd's built-in "tgl")
 *   sknb  rotary knob
 *
 * by Star Morin (c), star@autospkr.org
 */

#include "sgui.h"

SGUI_EXPORT void sgui_setup(void)
{
    sbng_setup();
    svsl_setup();
    shsl_setup();
    stgl_setup();
    sknb_setup();

    post("sgui by star morin");
    post("Contact: star@autospkr.org");
    post("sgui: version: 0.2");
    post("sgui: compiled: " __DATE__);
    post("");
}
