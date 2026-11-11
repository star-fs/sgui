/* sgui - replacement IEM-style GUI objects for Pure Data
 *
 * sgui.h - private header.
 *
 * sgui is self-contained: it vendors the IEM-GUI helper machinery
 * (structs and common functions, ported from pd-extended 0.44's
 * g_all_guis.h/.c) so that it builds against the *public* m_pd.h of
 * any 0.5x-generation pd - vanilla, pd-extended (Purr Data) or
 * PlugData - without needing pd's internal headers (g_canvas.h,
 * g_all_guis.h, s_stuff.h, t_tk.h, ...).
 */

#ifndef SGUI_H
#define SGUI_H

#include "m_pd.h"

/*
    The external is built with -fvisibility=hidden (see Makefile) so the
    vendored iemgui_* functions are NOT exported: vanilla pd ships its own
    copies of them inside the pd binary, and exporting ours from the .so
    would get our calls interposed onto pd's (which use a different
    t_iemgui layout).  Only the setup entry points are exported.
*/
#ifdef __GNUC__
#define SGUI_EXPORT __attribute__((visibility("default")))
#else
#define SGUI_EXPORT
#endif

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/*
    The declarations below are part of pd's canvas subsystem.  They exist
    in every 0.5x-generation pd build (vanilla, pd-extended, PlugData)
    but are not declared in the public m_pd.h, so we declare them here.
    Verified identical in: pd 0.54, pure-data main, plugdata-pd.
*/

#define IOWIDTH 7   /* width of an inlet/outlet in pixels */
#define IHEIGHT 3   /* height of an inlet in pixels */

typedef void (*t_glistmotionfn)(void *z, t_floatarg dx, t_floatarg dy,
    t_floatarg up);
typedef void (*t_glistkeyfn)(void *z, t_symbol *keysym, t_floatarg key);

typedef void (*t_getrectfn)(t_gobj *x, struct _glist *glist,
    int *xp1, int *yp1, int *xp2, int *yp2);
typedef void (*t_displacefn)(t_gobj *x, struct _glist *glist, int dx, int dy);
typedef void (*t_selectfn)(t_gobj *x, struct _glist *glist, int state);
typedef void (*t_activatefn)(t_gobj *x, struct _glist *glist, int state);
typedef void (*t_deletefn)(t_gobj *x, struct _glist *glist);
typedef void (*t_visfn)(t_gobj *x, struct _glist *glist, int flag);
typedef int (*t_clickfn)(t_gobj *x, struct _glist *glist,
    int xpix, int ypix, int shift, int alt, int dbl, int doit);

struct _widgetbehavior
{
    t_getrectfn w_getrectfn;
    t_displacefn w_displacefn;
    t_selectfn w_selectfn;
    t_activatefn w_activatefn;
    t_deletefn w_deletefn;
    t_visfn w_visfn;
    t_clickfn w_clickfn;
};

EXTERN t_canvas *glist_getcanvas(t_glist *x);
EXTERN int glist_isvisible(t_glist *x);
EXTERN void glist_grab(t_glist *x, t_gobj *y, t_glistmotionfn motionfn,
    t_glistkeyfn keyfn, int xpos, int ypos);
EXTERN int text_xpix(t_text *x, t_glist *glist);
EXTERN int text_ypix(t_text *x, t_glist *glist);
EXTERN void canvas_fixlinesfor(t_canvas *x, t_text *text);
EXTERN void canvas_deletelinesfor(t_canvas *x, t_text *text);
EXTERN t_symbol *canvas_realizedollar(t_canvas *x, t_symbol *s);
EXTERN void canvas_dirty(t_canvas *x, t_floatarg n);

EXTERN int sys_noloadbang;

/*
    ---------------- vendored IEM-GUI machinery ----------------
    ported from pd-extended 0.44 src/g_all_guis.h
    (Copyright (c) 1997-1999 Miller Puckette;
     g_7_guis written by Thomas Musil (c) IEM KUG Graz Austria 2000-2001)
*/

#define IEM_GUI_COLOR_SELECTED       0x0000FF
#define IEM_GUI_COLOR_NORMAL         0x000000

#define IEM_GUI_MAX_COLOR            30

#define IEM_GUI_DEFAULTSIZE 15
#define IEM_GUI_MINSIZE 8
#define IEM_GUI_MAXSIZE 1000
#define IEM_SL_DEFAULTSIZE 128
#define IEM_SL_MINSIZE 2
#define IEM_FONT_MINSIZE 4

#define IEM_BNG_DEFAULTHOLDFLASHTIME  250
#define IEM_BNG_DEFAULTBREAKFLASHTIME 50
#define IEM_BNG_MINHOLDFLASHTIME      50
#define IEM_BNG_MINBREAKFLASHTIME     10

#define IEM_MAX_SYM_LEN 127

#define IEM_GUI_DRAW_MODE_UPDATE 0
#define IEM_GUI_DRAW_MODE_MOVE   1
#define IEM_GUI_DRAW_MODE_NEW    2
#define IEM_GUI_DRAW_MODE_SELECT 3
#define IEM_GUI_DRAW_MODE_ERASE  4
#define IEM_GUI_DRAW_MODE_CONFIG 5
#define IEM_GUI_DRAW_MODE_IO     6

#define IS_A_POINTER(atom,index) ((atom+index)->a_type == A_POINTER)
#define IS_A_FLOAT(atom,index)   ((atom+index)->a_type == A_FLOAT)
/* a color argument: old-style number, or "#rrggbb" as saved by pd >= 0.51 */
#define IS_A_COLOR(atom,index)   (IS_A_FLOAT(atom,index)||IS_A_SYMBOL(atom,index))
#define IS_A_SYMBOL(atom,index)  ((atom+index)->a_type == A_SYMBOL)
#define IS_A_DOLLAR(atom,index)  ((atom+index)->a_type == A_DOLLAR)
#define IS_A_DOLLSYM(atom,index) ((atom+index)->a_type == A_DOLLSYM)

/* "loadbang" actions (g_canvas.h): pd >= 0.47 sends loadbang once per
   phase with one of these as argument; older pd sends no argument (= 0) */
#ifndef LB_LOAD
#define LB_LOAD 0
#endif

#define IEM_GUI_OLD_SND_FLAG 1
#define IEM_GUI_OLD_RCV_FLAG 2

typedef struct _iem_fstyle_flags
{
    unsigned int x_font_style:6;
    unsigned int x_rcv_able:1;
    unsigned int x_snd_able:1;
    unsigned int x_lab_is_unique:1;
    unsigned int x_rcv_is_unique:1;
    unsigned int x_snd_is_unique:1;
    unsigned int x_lab_arg_tail_len:6;
    unsigned int x_lab_is_arg_num:6;
    unsigned int x_shiftdown:1;
    unsigned int x_selected:1;
    unsigned int x_finemoved:1;
    unsigned int x_put_in2out:1;
    unsigned int x_change:1;
    unsigned int x_thick:1;
    unsigned int x_lin0_log1:1;
    unsigned int x_steady:1;
    unsigned int dummy:1;
} t_iem_fstyle_flags;

typedef struct _iem_init_symargs
{
    unsigned int x_loadinit:1;
    unsigned int x_rcv_arg_tail_len:6;
    unsigned int x_snd_arg_tail_len:6;
    unsigned int x_rcv_is_arg_num:6;
    unsigned int x_snd_is_arg_num:6;
    unsigned int x_scale:1;
    unsigned int x_flashed:1;
    unsigned int x_locked:1;
    unsigned int x_reverse:1;
    unsigned int dummy:3;
} t_iem_init_symargs;

typedef void (*t_iemfunptr)(void *x, t_glist *glist, int mode);

/*
    PlugData has no Tk GUI: it draws objects whose class is named
    vsl/hsl/tgl/bng/... with its own native widgets, casting them to the
    *modern* (pd 0.51+) t_iemgui/t_slider/t_toggle and calling the host's
    own iemgui_* functions on them.  Those read x_private, so our layout
    must match modern pd: the x_private pointer sits after x_w and points
    at a block laid out like pd's t_iemgui_private.  sgui itself never
    uses it; it is zeroed (p_binbuf_valid = 0, no draw overrides), which
    makes the host functions fall back to x_snd/x_rcv/x_lab.
*/
typedef void (*t_iemdrawfunptr)(void *x, t_glist *glist);
typedef struct _iemgui_private
{
    int             p_prevX, p_prevY;
    t_iemdrawfunptr p_draw_new, p_draw_config;
    t_iemfunptr     p_draw_iolets;
    t_iemdrawfunptr p_draw_update, p_draw_select, p_draw_erase, p_draw_move;
    int             p_binbuf_valid;
} t_iemgui_private;

/* matches pd's t_iem_orientation (t_slider.x_orientation) */
typedef enum {
    horizontal = 0,
    vertical = 1,
} t_iem_orientation;

typedef struct _iemgui
{
    t_object           x_obj;
    t_glist            *x_glist;
    t_iemfunptr        x_draw;
    int                x_h;
    int                x_w;
    t_iemgui_private   *x_private;  /* see above; for PlugData */
    int                x_ldx;
    int                x_ldy;
    char               x_font[MAXPDSTRING]; /* font names can be long! */
    t_iem_fstyle_flags x_fsf;
    int                x_fontsize;
    t_iem_init_symargs x_isa;
    int                x_fcol;
    int                x_bcol;
    int                x_lcol;
    /* send/receive/label as used ($args expanded) */
    t_symbol           *x_snd;
    t_symbol           *x_rcv;
    t_symbol           *x_lab;
    /* same, with '$' unexpanded */
    t_symbol           *x_snd_unexpanded;
    t_symbol           *x_rcv_unexpanded;
    t_symbol           *x_lab_unexpanded;
    int                x_binbufindex;
    int                x_labelbindex;
} t_iemgui;

/* any iemgui object starts with a t_iemgui; these callbacks cast to a
   dummy struct so the x_gui offset is well defined */
typedef struct _iemguidummy
{
    t_iemgui x_gui;
    int      x_dum1;
} t_iemguidummy;

EXTERN int iemgui_clip_size(int size);
EXTERN int iemgui_clip_font(int size);
EXTERN int iemgui_modulo_color(int col);
EXTERN t_symbol *iemgui_dollar2raute(t_symbol *s);
EXTERN t_symbol *iemgui_raute2dollar(t_symbol *s);
EXTERN void iemgui_verify_snd_ne_rcv(t_iemgui *iemgui);
EXTERN t_symbol *iemgui_new_dogetname(t_iemgui *iemgui, int indx, t_atom *argv);
EXTERN void iemgui_new_getnames(t_iemgui *iemgui, int indx, t_atom *argv);
EXTERN void iemgui_all_dollararg2sym(t_iemgui *iemgui, t_symbol **srlsym);
EXTERN void iemgui_all_sym2dollararg(t_iemgui *iemgui, t_symbol **srlsym);
EXTERN void iemgui_all_col2save(t_iemgui *iemgui, int *bflcol);
EXTERN void iemgui_all_colfromload(t_iemgui *iemgui, int *bflcol);
EXTERN int iemgui_compatible_col(int i);
EXTERN void iemgui_all_loadcolors(t_iemgui *iemgui, t_atom *bcol,
    t_atom *fcol, t_atom *lcol);
EXTERN void iemgui_all_dollar2raute(t_symbol **srlsym);
EXTERN void iemgui_all_raute2dollar(t_symbol **srlsym);
EXTERN void iemgui_send(void *x, t_iemgui *iemgui, t_symbol *s);
EXTERN void iemgui_receive(void *x, t_iemgui *iemgui, t_symbol *s);
EXTERN void iemgui_label(void *x, t_iemgui *iemgui, t_symbol *s);
EXTERN void iemgui_label_pos(void *x, t_iemgui *iemgui, t_symbol *s,
    int ac, t_atom *av);
EXTERN void iemgui_label_font(void *x, t_iemgui *iemgui, t_symbol *s,
    int ac, t_atom *av);
EXTERN void iemgui_size(void *x, t_iemgui *iemgui);
EXTERN void iemgui_delta(void *x, t_iemgui *iemgui, t_symbol *s,
    int ac, t_atom *av);
EXTERN void iemgui_pos(void *x, t_iemgui *iemgui, t_symbol *s,
    int ac, t_atom *av);
EXTERN void iemgui_color(void *x, t_iemgui *iemgui, t_symbol *s,
    int ac, t_atom *av);
EXTERN void iemgui_displace(t_gobj *z, t_glist *glist, int dx, int dy);
EXTERN void iemgui_select(t_gobj *z, t_glist *glist, int selected);
EXTERN void iemgui_delete(t_gobj *z, t_glist *glist);
EXTERN void iemgui_vis(t_gobj *z, t_glist *glist, int vis);
EXTERN void iemgui_save(t_iemgui *iemgui, t_symbol **srl, t_symbol **bflcol);
EXTERN void iemgui_properties(t_iemgui *iemgui, t_symbol **srl);
EXTERN int iemgui_dialog(t_iemgui *iemgui, t_symbol **srl, int argc,
    t_atom *argv);

EXTERN void sgui_iemgui_init(t_iemgui *iemgui);
EXTERN void sgui_iemgui_free(t_iemgui *iemgui);

EXTERN void iem_inttosymargs(t_iem_init_symargs *symargp, int n);
EXTERN int iem_symargstoint(t_iem_init_symargs *symargp);
EXTERN void iem_inttofstyle(t_iem_fstyle_flags *fstylep, int n);
EXTERN int iem_fstyletoint(t_iem_fstyle_flags *fstylep);

/* bound to "#keyname" while a widget is selected (see sknb.c) */
EXTERN t_symbol *iemgui_key_sym;

/* entry points (exported from the otherwise hidden-visibility objects) */
SGUI_EXPORT void sgui_setup(void);
SGUI_EXPORT void sbng_setup(void);
SGUI_EXPORT void svsl_setup(void);
SGUI_EXPORT void shsl_setup(void);
SGUI_EXPORT void stgl_setup(void);
SGUI_EXPORT void sknb_setup(void);

#endif /* SGUI_H */
