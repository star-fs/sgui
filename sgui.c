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
        /* PlugData has no Tk, so it can't run sgui's drawing: it shows
        sbng/sknb as plain boxes and draws stgl/svsl/shsl (classes "tgl",
        "vsl", "hsl") with its own toggle and sliders, which look different.
        Leave all the names free for the pdlua versions (*.pd_lua), which
        it draws natively. */
    sgui_banner();
    if (sgui_in_plugdata())
    {
        post("sgui: PlugData detected, the objects come from *.pd_lua");
        return;
    }
    sbng_setup();
    svsl_setup();
    shsl_setup();
    stgl_setup();
    sknb_setup();
}
