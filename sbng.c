/* Copyright (c) 1997-1999 Miller Puckette.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution. */

/* g_7_guis.c written by Thomas Musil (c) IEM KUG Graz Austria 2000-2001 */
/* thanks to Miller Puckette, Guenther Geiger and Krzystof Czaja */

/* sbng - by Star Morin (c), based on g_bang.c */

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "m_pd.h"
#include "g_canvas.h"
#include "t_tk.h"
#include "g_all_guis.h"
#include <math.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_IO_H
#include <io.h>
#endif


/* ------------ sbng rectangular resizable bang ----------------------- */

t_widgetbehavior sbng_widgetbehavior;
static t_class *sbng_class;
t_symbol *iemgui_key_sym=0;   /* taken from g_all_guis.c */

typedef struct _sbng
{
    t_iemgui x_gui;
    int      x_flashed;
    int      x_flashtime_break;
    int      x_flashtime_hold;
    t_clock  *x_clock_hld;
    t_clock  *x_clock_brk;
    t_clock  *x_clock_lck;
} t_sbng;


/* widget helper functions */

static void sbng_draw_update(t_gobj *client, t_glist *glist)
{
    t_sbng *x = (t_sbng *)client;

    if (glist_isvisible(glist))
    {
      sys_vgui(".x%lx.c itemconfigure %lxBNG -fill #%6.6x\n", glist_getcanvas(glist), x,
                 x->x_flashed?x->x_gui.x_fcol:x->x_gui.x_bcol);
    }
}

static void sbng_draw_new(t_sbng *x, t_glist *glist)
{
    int xpos=text_xpix(&x->x_gui.x_obj, glist);
    int ypos=text_ypix(&x->x_gui.x_obj, glist);
    t_canvas *canvas=glist_getcanvas(glist);

    sys_vgui(".x%lx.c create rectangle %d %d %d %d -fill #%6.6x -tags %lxBNG\n",
             canvas, xpos, ypos,
             xpos + x->x_gui.x_w, ypos + x->x_gui.x_h,
             x->x_gui.x_bcol, x);
    sys_vgui(".x%lx.c create text %d %d -text {%s} -anchor w \
             -font {{%s} %d %s} -fill #%6.6x -tags %lxLABEL\n",
             canvas, xpos+x->x_gui.x_ldx,
             ypos+x->x_gui.x_ldy,
             strcmp(x->x_gui.x_lab->s_name, "empty")?x->x_gui.x_lab->s_name:"",
             x->x_gui.x_font, x->x_gui.x_fontsize, sys_fontweight,
			 x->x_gui.x_lcol, x);
    if(!x->x_gui.x_fsf.x_snd_able)
        sys_vgui(".x%lx.c create rectangle %d %d %d %d -tags %lxOUT%d\n",
             canvas, xpos, 
             ypos + x->x_gui.x_h-1, xpos + IOWIDTH,
             ypos + x->x_gui.x_h, x, 0);
    if(!x->x_gui.x_fsf.x_rcv_able)
        sys_vgui(".x%lx.c create rectangle %d %d %d %d -tags %lxIN%d\n",
             canvas, xpos, ypos,
             xpos + IOWIDTH, ypos+1, x, 0);
}

static void sbng_draw_move(t_sbng *x, t_glist *glist)
{
    int xpos=text_xpix(&x->x_gui.x_obj, glist);
    int ypos=text_ypix(&x->x_gui.x_obj, glist);
    t_canvas *canvas=glist_getcanvas(glist);

    sys_vgui(".x%lx.c coords %lxBNG %d %d %d %d\n",
             canvas, x,
             xpos, ypos,
             xpos + x->x_gui.x_w, ypos + x->x_gui.x_h);
    sys_vgui(".x%lx.c coords %lxLABEL %d %d\n",
             canvas, x, xpos+x->x_gui.x_ldx, ypos+x->x_gui.x_ldy);
    if(!x->x_gui.x_fsf.x_snd_able)
        sys_vgui(".x%lx.c coords %lxOUT%d %d %d %d %d\n",
             canvas, x, 0, xpos,
             ypos + x->x_gui.x_h-1, xpos + IOWIDTH,
             ypos + x->x_gui.x_h);

    if(!x->x_gui.x_fsf.x_rcv_able)
        sys_vgui(".x%lx.c coords %lxIN%d %d %d %d %d\n",
             canvas, x, 0, xpos, ypos,
             xpos + IOWIDTH, ypos+1);
}

static void sbng_draw_erase(t_sbng* x,t_glist* glist)
{
    t_canvas *canvas=glist_getcanvas(glist);

    sys_vgui(".x%lx.c delete %lxBNG\n", canvas, x);
    sys_vgui(".x%lx.c delete %lxLABEL\n", canvas, x);
    if(!x->x_gui.x_fsf.x_snd_able)
        sys_vgui(".x%lx.c delete %lxOUT%d\n", canvas, x, 0);
    if(!x->x_gui.x_fsf.x_rcv_able)
        sys_vgui(".x%lx.c delete %lxIN%d\n", canvas, x, 0);
}

static void sbng_draw_config(t_sbng* x,t_glist* glist)
{
    t_canvas *canvas=glist_getcanvas(glist);

    sys_vgui(".x%lx.c itemconfigure %lxLABEL -font {{%s} %d %s} -fill #%6.6x -text {%s} \n",
             canvas, x, x->x_gui.x_font, x->x_gui.x_fontsize, sys_fontweight,
             x->x_gui.x_fsf.x_selected?IEM_GUI_COLOR_SELECTED:x->x_gui.x_lcol,
             strcmp(x->x_gui.x_lab->s_name, "empty")?x->x_gui.x_lab->s_name:"");
    sys_vgui(".x%lx.c itemconfigure %lxBNG -fill #%6.6x\n", canvas, x, x->x_gui.x_bcol);
}

static void sbng_draw_io(t_sbng* x,t_glist* glist, int old_snd_rcv_flags)
{
    int xpos=text_xpix(&x->x_gui.x_obj, glist);
    int ypos=text_ypix(&x->x_gui.x_obj, glist);
    t_canvas *canvas=glist_getcanvas(glist);

    if((old_snd_rcv_flags & IEM_GUI_OLD_SND_FLAG) && !x->x_gui.x_fsf.x_snd_able)
        sys_vgui(".x%lx.c create rectangle %d %d %d %d -tags %lxOUT%d\n",
             canvas, xpos, ypos + x->x_gui.x_h-1,
             xpos, ypos + x->x_gui.x_h, x, 0);
    if(!(old_snd_rcv_flags & IEM_GUI_OLD_SND_FLAG) && x->x_gui.x_fsf.x_snd_able)
        sys_vgui(".x%lx.c delete %lxOUT%d\n", canvas, x, 0);
    if((old_snd_rcv_flags & IEM_GUI_OLD_RCV_FLAG) && !x->x_gui.x_fsf.x_rcv_able)
        sys_vgui(".x%lx.c create rectangle %d %d %d %d -tags %lxIN%d\n",
             canvas, xpos, ypos,
             xpos, ypos+1, x, 0);
    if(!(old_snd_rcv_flags & IEM_GUI_OLD_RCV_FLAG) && x->x_gui.x_fsf.x_rcv_able)
        sys_vgui(".x%lx.c delete %lxIN%d\n", canvas, x, 0);
}

static void sbng_draw_select(t_sbng* x,t_glist* glist)
{
    t_canvas *canvas=glist_getcanvas(glist);

    if(x->x_gui.x_fsf.x_selected)
    {
        sys_vgui(".x%lx.c itemconfigure %lxBNG -outline #%6.6x\n", canvas, x, IEM_GUI_COLOR_SELECTED);
        sys_vgui(".x%lx.c itemconfigure %lxLABEL -fill #%6.6x\n", canvas, x, IEM_GUI_COLOR_SELECTED);
    }
    else
    {
        sys_vgui(".x%lx.c itemconfigure %lxBNG -outline #%6.6x\n", canvas, x, IEM_GUI_COLOR_NORMAL);
        sys_vgui(".x%lx.c itemconfigure %lxLABEL -fill #%6.6x\n", canvas, x, x->x_gui.x_lcol);
    }
}

void sbng_draw(t_sbng *x, t_glist *glist, int mode)
{
    if(mode == IEM_GUI_DRAW_MODE_UPDATE)
        sys_queuegui(x, glist, sbng_draw_update);
    else if(mode == IEM_GUI_DRAW_MODE_MOVE)
        sbng_draw_move(x, glist);
    else if(mode == IEM_GUI_DRAW_MODE_NEW)
        sbng_draw_new(x, glist);
    else if(mode == IEM_GUI_DRAW_MODE_SELECT)
        sbng_draw_select(x, glist);
    else if(mode == IEM_GUI_DRAW_MODE_ERASE)
        sbng_draw_erase(x, glist);
    else if(mode == IEM_GUI_DRAW_MODE_CONFIG)
        sbng_draw_config(x, glist);
    else if(mode >= IEM_GUI_DRAW_MODE_IO)
        sbng_draw_io(x, glist, mode - IEM_GUI_DRAW_MODE_IO);
}

/* ------------------------ sbng widgetbehaviour----------------------------- */


static void sbng_getrect(t_gobj *z, t_glist *glist,
                            int *xp1, int *yp1, int *xp2, int *yp2)
{
    t_sbng* x = (t_sbng*)z;

    *xp1 = text_xpix(&x->x_gui.x_obj, glist);
    *yp1 = text_ypix(&x->x_gui.x_obj, glist);
    *xp2 = *xp1 + x->x_gui.x_w;
    *yp2 = *yp1 + x->x_gui.x_h;
}

static void sbng_save(t_gobj *z, t_binbuf *b)
{
    t_sbng *x = (t_sbng *)z;
    int bflcol[3];
    t_symbol *srl[3];

    iemgui_save(&x->x_gui, srl, bflcol);
    binbuf_addv(b, "ssiisiiiiisssiiiiiii", gensym("#X"),gensym("obj"),
                (int)x->x_gui.x_obj.te_xpix, (int)x->x_gui.x_obj.te_ypix,
                gensym("sbng"), x->x_gui.x_w, x->x_gui.x_h,
                x->x_flashtime_hold, x->x_flashtime_break,
                iem_symargstoint(&x->x_gui.x_isa),
                srl[0], srl[1], srl[2],
                x->x_gui.x_ldx, x->x_gui.x_ldy,
                iem_fstyletoint(&x->x_gui.x_fsf), x->x_gui.x_fontsize,
                bflcol[0], bflcol[1], bflcol[2]);
    binbuf_addv(b, ";");
}

void sbng_check_minmax(t_sbng *x, int ftbreak, int fthold)
{
    if(ftbreak > fthold)
    {
        int h;
        h = ftbreak;
        ftbreak = fthold;
        fthold = h;
    }
    if(ftbreak < IEM_BNG_MINBREAKFLASHTIME)
        ftbreak = IEM_BNG_MINBREAKFLASHTIME;
    if(fthold < IEM_BNG_MINHOLDFLASHTIME)
        fthold = IEM_BNG_MINHOLDFLASHTIME;
    x->x_flashtime_break = ftbreak;
    x->x_flashtime_hold = fthold;

}

static void sbng_properties(t_gobj *z, t_glist *owner)
{
    t_sbng *x = (t_sbng *)z;
    char buf[800];
    t_symbol *srl[3];

    iemgui_properties(&x->x_gui, srl);
    sprintf(buf, "pdtk_iemgui_dialog %%s |sbng| \
            --------dimensions(pix)(pix):-------- %d %d width: %d %d height: \
            --------flash-time(ms)(ms):--------- %d intrrpt: %d hold: %d\
            %d empty empty %d %d empty %d \
            %s %s \
            %s %d %d \
            %d %d \
            %d %d %d\n",
            x->x_gui.x_w, IEM_SL_MINSIZE, x->x_gui.x_h, IEM_GUI_MINSIZE,
            x->x_flashtime_break, x->x_flashtime_hold, 2,/*min_max_schedule+clip*/
            -1, x->x_gui.x_isa.x_loadinit, -1, -1,/*no linlog, no multi*/
            srl[0]->s_name, srl[1]->s_name,
            srl[2]->s_name, x->x_gui.x_ldx, x->x_gui.x_ldy,
            x->x_gui.x_fsf.x_font_style, x->x_gui.x_fontsize,
            0xffffff & x->x_gui.x_bcol, 0xffffff & x->x_gui.x_fcol, 0xffffff & x->x_gui.x_lcol);
    gfxstub_new(&x->x_gui.x_obj.ob_pd, x, buf);
}

static void sbng_set(t_sbng *x)    /* bugfix */
{
    if(x->x_flashed)
    {
        x->x_flashed = 0;
        (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
        clock_delay(x->x_clock_brk, x->x_flashtime_break);
        x->x_flashed = 1;
    }
    else
    {
        x->x_flashed = 1;
        (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
    }
    clock_delay(x->x_clock_hld, x->x_flashtime_hold);
}

static void sbng_bout1(t_sbng *x)/*wird nur mehr gesendet, wenn snd != rcv*/
{
    if(!x->x_gui.x_fsf.x_put_in2out)
    {
        x->x_gui.x_isa.x_locked = 1;
        clock_delay(x->x_clock_lck, 2);
    }
    outlet_bang(x->x_gui.x_obj.ob_outlet);
    if(x->x_gui.x_fsf.x_snd_able && x->x_gui.x_snd->s_thing && x->x_gui.x_fsf.x_put_in2out)
        pd_bang(x->x_gui.x_snd->s_thing);
}

static void sbng_bout2(t_sbng *x)/*wird immer gesendet, wenn moeglich*/
{
    if(!x->x_gui.x_fsf.x_put_in2out)
    {
        x->x_gui.x_isa.x_locked = 1;
        clock_delay(x->x_clock_lck, 2);
    }
    outlet_bang(x->x_gui.x_obj.ob_outlet);
    if(x->x_gui.x_fsf.x_snd_able && x->x_gui.x_snd->s_thing)
        pd_bang(x->x_gui.x_snd->s_thing);
}

static void sbng_bang(t_sbng *x)/*wird nur mehr gesendet, wenn snd != rcv*/
{
    if(!x->x_gui.x_isa.x_locked)
    {
        sbng_set(x);
        sbng_bout1(x);
    }
}

static void sbng_bang2(t_sbng *x)/*wird immer gesendet, wenn moeglich*/
{
    if(!x->x_gui.x_isa.x_locked)
    {
        sbng_set(x);
        sbng_bout2(x);
    }
}

static void sbng_dialog(t_sbng *x, t_symbol *s, int argc, t_atom *argv)
{
    t_symbol *srl[3];
    int w = (int)atom_getintarg(0, argc, argv);
    int h = (int)atom_getintarg(1, argc, argv);
    int fthold = (int)atom_getintarg(2, argc, argv);
    int ftbreak = (int)atom_getintarg(3, argc, argv);
    int sr_flags = iemgui_dialog(&x->x_gui, srl, argc, argv);

    x->x_gui.x_w = w;
    x->x_gui.x_h = h;
    sbng_check_minmax(x, ftbreak, fthold);
    (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_CONFIG);
    (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_IO + sr_flags);
    (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_MOVE);
    canvas_fixlinesfor(glist_getcanvas(x->x_gui.x_glist), (t_text*)x);
}

static void sbng_motion(t_sbng *x, t_floatarg dx, t_floatarg dy)
{
    sbng_set(x);
    sbng_bout2(x);
}

static void sbng_click(t_sbng *x, t_floatarg xpos, t_floatarg ypos,
                          t_floatarg shift, t_floatarg ctrl, t_floatarg alt)
{
    sbng_set(x);
    sbng_bout2(x);
}

static int sbng_newclick(t_gobj *z, struct _glist *glist,
                            int xpix, int ypix, int shift, int alt, int dbl, int doit)
{
    if(doit)
        sbng_click((t_sbng *)z, (t_floatarg)xpix, (t_floatarg)ypix, (t_floatarg)shift, 0, (t_floatarg)alt);
    return (1);
}

static void sbng_float(t_sbng *x, t_floatarg f)
{sbng_bang2(x);}

static void sbng_symbol(t_sbng *x, t_symbol *s)
{sbng_bang2(x);}

static void sbng_pointer(t_sbng *x, t_gpointer *gp)
{sbng_bang2(x);}

static void sbng_list(t_sbng *x, t_symbol *s, int ac, t_atom *av)
{
    sbng_bang2(x);
}

static void sbng_anything(t_sbng *x, t_symbol *s, int argc, t_atom *argv)
{sbng_bang2(x);}

static void sbng_loadbang(t_sbng *x)
{
    if(!sys_noloadbang && x->x_gui.x_isa.x_loadinit)
    {
        sbng_set(x);
        sbng_bout2(x);
    }
}

static void sbng_size(t_sbng *x, t_symbol *s, int ac, t_atom *av)
{
    x->x_gui.x_w = iemgui_clip_size((int)atom_getintarg(0, ac, av));
    x->x_gui.x_h = x->x_gui.x_w;
    iemgui_size((void *)x, &x->x_gui);
}

static void sbng_delta(t_sbng *x, t_symbol *s, int ac, t_atom *av)
{iemgui_delta((void *)x, &x->x_gui, s, ac, av);}

static void sbng_pos(t_sbng *x, t_symbol *s, int ac, t_atom *av)
{iemgui_pos((void *)x, &x->x_gui, s, ac, av);}

static void sbng_flashtime(t_sbng *x, t_symbol *s, int ac, t_atom *av)
{
    sbng_check_minmax(x, (int)atom_getintarg(0, ac, av),
                     (int)atom_getintarg(1, ac, av));
}

static void sbng_color(t_sbng *x, t_symbol *s, int ac, t_atom *av)
{iemgui_color((void *)x, &x->x_gui, s, ac, av);}

static void sbng_send(t_sbng *x, t_symbol *s)
{iemgui_send(x, &x->x_gui, s);}

static void sbng_receive(t_sbng *x, t_symbol *s)
{iemgui_receive(x, &x->x_gui, s);}

static void sbng_label(t_sbng *x, t_symbol *s)
{iemgui_label((void *)x, &x->x_gui, s);}

static void sbng_label_pos(t_sbng *x, t_symbol *s, int ac, t_atom *av)
{iemgui_label_pos((void *)x, &x->x_gui, s, ac, av);}

static void sbng_label_font(t_sbng *x, t_symbol *s, int ac, t_atom *av)
{iemgui_label_font((void *)x, &x->x_gui, s, ac, av);}

static void sbng_init(t_sbng *x, t_floatarg f)
{
    x->x_gui.x_isa.x_loadinit = (f==0.0)?0:1;
}

static void sbng_tick_hld(t_sbng *x)
{
    x->x_flashed = 0;
    (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
}

static void sbng_tick_brk(t_sbng *x)
{
    (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
}

static void sbng_tick_lck(t_sbng *x)
{
    x->x_gui.x_isa.x_locked = 0;
}

static void *sbng_new(t_symbol *s, int argc, t_atom *argv)
{
    t_sbng *x = (t_sbng *)pd_new(sbng_class);
    int bflcol[]={-262144, -1, -1};
    int w=IEM_GUI_DEFAULTSIZE;
    int h=IEM_GUI_DEFAULTSIZE;
    int ldx=17, ldy=7;
    int fs=10;
    int ftbreak=IEM_BNG_DEFAULTBREAKFLASHTIME,
        fthold=IEM_BNG_DEFAULTHOLDFLASHTIME;
    char str[144];

    iem_inttosymargs(&x->x_gui.x_isa, 0);
    iem_inttofstyle(&x->x_gui.x_fsf, 0);

    if((argc == 15)&&IS_A_FLOAT(argv,0)
       &&IS_A_FLOAT(argv,1)&&IS_A_FLOAT(argv,2)
       &&IS_A_FLOAT(argv,3)
       &&IS_A_FLOAT(argv,4)
       &&(IS_A_SYMBOL(argv,5)||IS_A_FLOAT(argv,5))
       &&(IS_A_SYMBOL(argv,6)||IS_A_FLOAT(argv,6))
       &&(IS_A_SYMBOL(argv,7)||IS_A_FLOAT(argv,7))
       &&IS_A_FLOAT(argv,8)&&IS_A_FLOAT(argv,9)
       &&IS_A_FLOAT(argv,10)&&IS_A_FLOAT(argv,11)&&IS_A_FLOAT(argv,12)
       &&IS_A_FLOAT(argv,13)&&IS_A_FLOAT(argv,14))
    {
        w = (int)atom_getintarg(0, argc, argv);
        h = (int)atom_getintarg(1, argc, argv);
        fthold = (int)atom_getintarg(2, argc, argv);
        ftbreak = (int)atom_getintarg(3, argc, argv);
        iem_inttosymargs(&x->x_gui.x_isa, atom_getintarg(4, argc, argv));
        iemgui_new_getnames(&x->x_gui, 5, argv);
        ldx = (int)atom_getintarg(8, argc, argv);
        ldy = (int)atom_getintarg(9, argc, argv);
        iem_inttofstyle(&x->x_gui.x_fsf, atom_getintarg(10, argc, argv));
        fs = (int)atom_getintarg(11, argc, argv);
        bflcol[0] = (int)atom_getintarg(12, argc, argv);
        bflcol[1] = (int)atom_getintarg(13, argc, argv);
        bflcol[2] = (int)atom_getintarg(14, argc, argv);
    }
    else iemgui_new_getnames(&x->x_gui, 4, 0);

    x->x_gui.x_draw = (t_iemfunptr)sbng_draw;

    x->x_gui.x_fsf.x_snd_able = 1;
    x->x_gui.x_fsf.x_rcv_able = 1;
    x->x_flashed = 0;
    x->x_gui.x_glist = (t_glist *)canvas_getcurrent();
    if (!strcmp(x->x_gui.x_snd->s_name, "empty"))
        x->x_gui.x_fsf.x_snd_able = 0;
    if (!strcmp(x->x_gui.x_rcv->s_name, "empty"))
        x->x_gui.x_fsf.x_rcv_able = 0;
    if(x->x_gui.x_fsf.x_font_style == 1) strcpy(x->x_gui.x_font, "helvetica");
    else if(x->x_gui.x_fsf.x_font_style == 2) strcpy(x->x_gui.x_font, "times");
    else { x->x_gui.x_fsf.x_font_style = 0;
        strcpy(x->x_gui.x_font, sys_font); }

    if (x->x_gui.x_fsf.x_rcv_able)
        pd_bind(&x->x_gui.x_obj.ob_pd, x->x_gui.x_rcv);
    x->x_gui.x_ldx = ldx;
    x->x_gui.x_ldy = ldy;

    if(fs < 4)
        fs = 4;
    x->x_gui.x_fontsize = fs;
    x->x_gui.x_w = w;
    x->x_gui.x_h = h;
    sbng_check_minmax(x, ftbreak, fthold);
    iemgui_all_colfromload(&x->x_gui, bflcol);
    x->x_gui.x_isa.x_locked = 0;
    iemgui_verify_snd_ne_rcv(&x->x_gui);
    x->x_clock_hld = clock_new(x, (t_method)sbng_tick_hld);
    x->x_clock_brk = clock_new(x, (t_method)sbng_tick_brk);
    x->x_clock_lck = clock_new(x, (t_method)sbng_tick_lck);
    outlet_new(&x->x_gui.x_obj, &s_bang);
    post("in sbng_new, symbod data: %s %s %s", x->x_gui.x_snd->s_name, x->x_gui.x_rcv->s_name, x->x_gui.x_font);
    return (x);
}

static void sbng_free(t_sbng *x)
{
    if(x->x_gui.x_fsf.x_rcv_able)
        pd_unbind(&x->x_gui.x_obj.ob_pd, x->x_gui.x_rcv);
    clock_free(x->x_clock_lck);
    clock_free(x->x_clock_brk);
    clock_free(x->x_clock_hld);
    gfxstub_deleteforkey(x);
}

void sbng_setup(void)
{
    sbng_class = class_new(gensym("sbng"), (t_newmethod)sbng_new,
                              (t_method)sbng_free, sizeof(t_sbng), 0, A_GIMME, 0);
    class_addcreator((t_newmethod)sbng_new, gensym("sbng"), A_GIMME, 0);
    class_addbang(sbng_class,sbng_bang);
    class_addfloat(sbng_class,sbng_float);
    class_addmethod(sbng_class, (t_method)sbng_click, gensym("click"),
                    A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, 0);
    class_addmethod(sbng_class, (t_method)sbng_motion, gensym("motion"),
                    A_FLOAT, A_FLOAT, 0);
    class_addmethod(sbng_class, (t_method)sbng_dialog, gensym("dialog"), A_GIMME, 0);
    class_addmethod(sbng_class, (t_method)sbng_loadbang, gensym("loadbang"), 0);
    class_addmethod(sbng_class, (t_method)sbng_set, gensym("set"), A_FLOAT, 0);
    class_addmethod(sbng_class, (t_method)sbng_size, gensym("size"), A_GIMME, 0);
    class_addmethod(sbng_class, (t_method)sbng_delta, gensym("delta"), A_GIMME, 0);
    class_addmethod(sbng_class, (t_method)sbng_pos, gensym("pos"), A_GIMME, 0);
    class_addmethod(sbng_class, (t_method)sbng_color, gensym("color"), A_GIMME, 0);
    class_addmethod(sbng_class, (t_method)sbng_send, gensym("send"), A_DEFSYM, 0);
    class_addmethod(sbng_class, (t_method)sbng_receive, gensym("receive"), A_DEFSYM, 0);
    class_addmethod(sbng_class, (t_method)sbng_label, gensym("label"), A_DEFSYM, 0);
    class_addmethod(sbng_class, (t_method)sbng_label_pos, gensym("label_pos"), A_GIMME, 0);
    class_addmethod(sbng_class, (t_method)sbng_label_font, gensym("label_font"), A_GIMME, 0);
    class_addmethod(sbng_class, (t_method)sbng_init, gensym("init"), A_FLOAT, 0);
    sbng_widgetbehavior.w_getrectfn =    sbng_getrect;
    sbng_widgetbehavior.w_displacefn =   iemgui_displace;
    sbng_widgetbehavior.w_selectfn =     iemgui_select;
    sbng_widgetbehavior.w_activatefn =   NULL;
    sbng_widgetbehavior.w_deletefn =     iemgui_delete;
    sbng_widgetbehavior.w_visfn =        iemgui_vis;
    sbng_widgetbehavior.w_clickfn =      sbng_newclick;
    class_setwidget(sbng_class, &sbng_widgetbehavior);
    class_sethelpsymbol(sbng_class, gensym("sbng"));
    class_setsavefn(sbng_class, sbng_save);
    class_setpropertiesfn(sbng_class, sbng_properties);
}
