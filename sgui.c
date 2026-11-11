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
#ifndef _WIN32
#include <dlfcn.h>
#endif

    /* PlugData exports plugdata_* functions from its own binary */
static int sgui_in_plugdata(void)
{
#ifndef _WIN32
    return (dlsym(RTLD_DEFAULT, "plugdata_forward_message") != 0);
#else
    return (0);
#endif
}

SGUI_EXPORT void sgui_setup(void)
{
        /* PlugData draws svsl/shsl with its own native sliders; it can't
        run the Tk drawing of sbng/sknb, and would draw stgl (class "tgl")
        as its own square X toggle.  Leave those names free for the pdlua
        versions (sbng/stgl/sknb.pd_lua), which it draws natively. */
    int plugdata = sgui_in_plugdata();
    sgui_banner();
    if (plugdata)
        post("sgui: PlugData detected, sbng/stgl/sknb come from *.pd_lua");
    else sbng_setup();
    svsl_setup();
    shsl_setup();
    if (!plugdata)
    {
        stgl_setup();
        sknb_setup();
    }

}
