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
        /* PlugData draws svsl/shsl/stgl with its own sliders and toggle,
        but has no widget for sbng/sknb and can't run their Tk drawing;
        leave those names free for the pdlua versions (sbng.pd_lua,
        sknb.pd_lua), which it draws natively */
    int plugdata = sgui_in_plugdata();
    if (plugdata)
        post("sgui: PlugData detected, sbng/sknb come from *.pd_lua");
    else sbng_setup();
    svsl_setup();
    shsl_setup();
    stgl_setup();
    if (!plugdata)
        sknb_setup();

    post("sgui by star morin");
    post("Contact: star@autospkr.org");
    post("sgui: version: 0.2");
    post("sgui: compiled: " __DATE__);
    post("");
}
