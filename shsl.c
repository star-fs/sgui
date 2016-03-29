/* Copyright (c) 1997-1999 Miller Puckette.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution. */

/* g_7_guis.c written by Thomas Musil (c) IEM KUG Graz Austria 2000-2001 */
/* thanks to Miller Puckette, Guenther Geiger and Krzystof Czaja */

/* 'svsl' gui object by Star Morin (c), based on g_hslider.c */

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


/* ------------ shsl gui-horicontal slider ----------------------- */

t_widgetbehavior shsl_widgetbehavior;
static t_class *shsl_class;

typedef struct _shsl
{
    t_iemgui x_gui;
    int      x_pos;
    int      x_val;
    int      x_lin0_log1;
    int      x_steady;
    double   x_min;
    double   x_max;
    double   x_k;
} t_shsl;

/* widget helper functions */

static void shsl_draw_update(t_gobj *client, t_glist *glist)
{
    t_shsl *x = (t_shsl *)client;
    t_canvas *canvas=glist_getcanvas(glist);
    int xpos=text_xpix(&x->x_gui.x_obj, glist);
    int ypos=text_ypix(&x->x_gui.x_obj, glist);

    if (glist_isvisible(glist))
    {
        int r = text_xpix(&x->x_gui.x_obj, glist) + (x->x_val + 50)/100;
        sys_vgui(".x%lx.c coords %lxHSL %d %d %d %d\n",
                 canvas, x, xpos, ypos,
                 r, (ypos + x->x_gui.x_h));
    }
}

static void shsl_draw_new(t_shsl *x, t_glist *glist)
{
    int xpos=text_xpix(&x->x_gui.x_obj, glist);
    int ypos=text_ypix(&x->x_gui.x_obj, glist);
    int r = xpos + (x->x_val + 50)/100;
    t_canvas *canvas=glist_getcanvas(glist);

    sys_vgui(".x%lx.c create rectangle %d %d %d %d -fill #%6.6x -tags %lxBASE\n",
             canvas, xpos, ypos, xpos + x->x_gui.x_w, ypos + x->x_gui.x_h,
             x->x_gui.x_bcol, x);
    sys_vgui(".x%lx.c create rectangle %d %d %d %d -fill #%6.6x -tags %lxHSL\n",
             canvas, xpos, ypos, r, (ypos + x->x_gui.x_h), x->x_gui.x_fcol, x);
    sys_vgui(".x%lx.c create text %d %d -text {%s} -anchor w \
             -font {{%s} %d %s} -fill #%6.6x -tags %lxLABEL\n",
             canvas, xpos+x->x_gui.x_ldx,
             ypos+x->x_gui.x_ldy,
             strcmp(x->x_gui.x_lab->s_name, "empty")?x->x_gui.x_lab->s_name:"",
             x->x_gui.x_font, x->x_gui.x_fontsize, sys_fontweight,
			 x->x_gui.x_lcol, x);

    if(!x->x_gui.x_fsf.x_snd_able)
        sys_vgui(".x%lx.c create rectangle %d %d %d %d -tags %lxOUT%d\n",
             canvas, xpos, ypos + x->x_gui.x_h-1,
             xpos+7, ypos + x->x_gui.x_h, x, 0);

    if(!x->x_gui.x_fsf.x_rcv_able)
        sys_vgui(".x%lx.c create rectangle %d %d %d %d -tags %lxIN%d\n",
             canvas, xpos, ypos,
             xpos+7, ypos+1, x, 0);
}

static void shsl_draw_move(t_shsl *x, t_glist *glist)
{
    int xpos=text_xpix(&x->x_gui.x_obj, glist);
    int ypos=text_ypix(&x->x_gui.x_obj, glist);
    int r = xpos + (x->x_val + 50)/100;
    t_canvas *canvas=glist_getcanvas(glist);

    sys_vgui(".x%lx.c coords %lxBASE %d %d %d %d\n",
             canvas, x,
             xpos, ypos,
             xpos + x->x_gui.x_w, ypos + x->x_gui.x_h);
    sys_vgui(".x%lx.c coords %lxHSL %d %d %d %d\n",
             canvas, x, xpos, ypos,
             r, ypos + x->x_gui.x_h);
    sys_vgui(".x%lx.c coords %lxLABEL %d %d\n",
             canvas, x, xpos+x->x_gui.x_ldx, ypos+x->x_gui.x_ldy);
    if(!x->x_gui.x_fsf.x_snd_able)
        sys_vgui(".x%lx.c coords %lxOUT%d %d %d %d %d\n",
             canvas, x, 0,
             xpos, ypos + x->x_gui.x_h-1,
             xpos+7, ypos + x->x_gui.x_h);
    if(!x->x_gui.x_fsf.x_rcv_able)
        sys_vgui(".x%lx.c coords %lxIN%d %d %d %d %d\n",
             canvas, x, 0,
             xpos, ypos,
             xpos+7, ypos+1);
}

static void shsl_draw_erase(t_shsl* x,t_glist* glist)
{
    t_canvas *canvas=glist_getcanvas(glist);

    sys_vgui(".x%lx.c delete %lxBASE\n", canvas, x);
    sys_vgui(".x%lx.c delete %lxHSL\n", canvas, x);
    sys_vgui(".x%lx.c delete %lxLABEL\n", canvas, x);
    if(!x->x_gui.x_fsf.x_snd_able)
        sys_vgui(".x%lx.c delete %lxOUT%d\n", canvas, x, 0);
    if(!x->x_gui.x_fsf.x_rcv_able)
        sys_vgui(".x%lx.c delete %lxIN%d\n", canvas, x, 0);
}

static void shsl_draw_config(t_shsl* x,t_glist* glist)
{
    t_canvas *canvas=glist_getcanvas(glist);

    sys_vgui(".x%lx.c itemconfigure %lxLABEL -font {{%s} %d %s} -fill #%6.6x -text {%s} \n",
             canvas, x, x->x_gui.x_font, x->x_gui.x_fontsize, sys_fontweight,
             x->x_gui.x_fsf.x_selected?IEM_GUI_COLOR_SELECTED:x->x_gui.x_lcol,
             strcmp(x->x_gui.x_lab->s_name, "empty")?x->x_gui.x_lab->s_name:"");
    sys_vgui(".x%lx.c itemconfigure %lxHSL -fill #%6.6x\n", canvas, x, x->x_gui.x_fcol);
    sys_vgui(".x%lx.c itemconfigure %lxBASE -fill #%6.6x\n", canvas, x, x->x_gui.x_bcol);
}

static void shsl_draw_io(t_shsl* x,t_glist* glist, int old_snd_rcv_flags)
{
    int xpos=text_xpix(&x->x_gui.x_obj, glist);
    int ypos=text_ypix(&x->x_gui.x_obj, glist);
    t_canvas *canvas=glist_getcanvas(glist);

    if((old_snd_rcv_flags & IEM_GUI_OLD_SND_FLAG) && !x->x_gui.x_fsf.x_snd_able)
        sys_vgui(".x%lx.c create rectangle %d %d %d %d -tags %lxOUT%d\n",
             canvas, xpos-3, ypos + x->x_gui.x_h-1,
             xpos+4, ypos + x->x_gui.x_h, x, 0);
    if(!(old_snd_rcv_flags & IEM_GUI_OLD_SND_FLAG) && x->x_gui.x_fsf.x_snd_able)
        sys_vgui(".x%lx.c delete %lxOUT%d\n", canvas, x, 0);
    if((old_snd_rcv_flags & IEM_GUI_OLD_RCV_FLAG) && !x->x_gui.x_fsf.x_rcv_able)
        sys_vgui(".x%lx.c create rectangle %d %d %d %d -tags %lxIN%d\n",
             canvas, xpos-3, ypos,
             xpos+4, ypos+1, x, 0);
    if(!(old_snd_rcv_flags & IEM_GUI_OLD_RCV_FLAG) && x->x_gui.x_fsf.x_rcv_able)
        sys_vgui(".x%lx.c delete %lxIN%d\n", canvas, x, 0);
}

static void shsl_draw_select(t_shsl* x,t_glist* glist)
{
    t_canvas *canvas=glist_getcanvas(glist);

    if(x->x_gui.x_fsf.x_selected)
    {
        sys_vgui(".x%lx.c itemconfigure %lxBASE -outline #%6.6x\n", canvas, x, IEM_GUI_COLOR_SELECTED);
        sys_vgui(".x%lx.c itemconfigure %lxLABEL -fill #%6.6x\n", canvas, x, IEM_GUI_COLOR_SELECTED);
    }
    else
    {
        sys_vgui(".x%lx.c itemconfigure %lxBASE -outline #%6.6x\n", canvas, x, IEM_GUI_COLOR_NORMAL);
        sys_vgui(".x%lx.c itemconfigure %lxLABEL -fill #%6.6x\n", canvas, x, x->x_gui.x_lcol);
    }
}

void shsl_draw(t_shsl *x, t_glist *glist, int mode)
{
    if(mode == IEM_GUI_DRAW_MODE_UPDATE)
        sys_queuegui(x, glist, shsl_draw_update);
    else if(mode == IEM_GUI_DRAW_MODE_MOVE)
        shsl_draw_move(x, glist);
    else if(mode == IEM_GUI_DRAW_MODE_NEW)
        shsl_draw_new(x, glist);
    else if(mode == IEM_GUI_DRAW_MODE_SELECT)
        shsl_draw_select(x, glist);
    else if(mode == IEM_GUI_DRAW_MODE_ERASE)
        shsl_draw_erase(x, glist);
    else if(mode == IEM_GUI_DRAW_MODE_CONFIG)
        shsl_draw_config(x, glist);
    else if(mode >= IEM_GUI_DRAW_MODE_IO)
        shsl_draw_io(x, glist, mode - IEM_GUI_DRAW_MODE_IO);
}

/* ------------------------ hsl widgetbehaviour----------------------------- */


static void shsl_getrect(t_gobj *z, t_glist *glist,
                            int *xp1, int *yp1, int *xp2, int *yp2)
{
    t_shsl* x = (t_shsl*)z;

    *xp1 = text_xpix(&x->x_gui.x_obj, glist);
    *yp1 = text_ypix(&x->x_gui.x_obj, glist);
    *xp2 = *xp1 + x->x_gui.x_w;
    *yp2 = *yp1 + x->x_gui.x_h;
}

static void shsl_save(t_gobj *z, t_binbuf *b)
{
    t_shsl *x = (t_shsl *)z;
    int bflcol[3];
    t_symbol *srl[3];

    iemgui_save(&x->x_gui, srl, bflcol);
    binbuf_addv(b, "ssiisiiffiisssiiiiiiiii", gensym("#X"),gensym("obj"),
                (int)x->x_gui.x_obj.te_xpix, (int)x->x_gui.x_obj.te_ypix,
                gensym("shsl"), x->x_gui.x_w, x->x_gui.x_h,
                (float)x->x_min, (float)x->x_max,
                x->x_lin0_log1, iem_symargstoint(&x->x_gui.x_isa),
                srl[0], srl[1], srl[2],
                x->x_gui.x_ldx, x->x_gui.x_ldy,
                iem_fstyletoint(&x->x_gui.x_fsf), x->x_gui.x_fontsize,
                bflcol[0], bflcol[1], bflcol[2],
                x->x_val, x->x_steady);
    binbuf_addv(b, ";");
}

void shsl_check_width(t_shsl *x, int w)
{
    if(w < IEM_SL_MINSIZE)
        w = IEM_SL_MINSIZE;
    x->x_gui.x_w = w;
    if(x->x_val > (x->x_gui.x_w*100 - 100))
    {
        x->x_pos = x->x_gui.x_w*100 - 100;
        x->x_val = x->x_pos;
    }
    if(x->x_lin0_log1)
        x->x_k = log(x->x_max/x->x_min)/(double)(x->x_gui.x_w - 1);
    else
        x->x_k = (x->x_max - x->x_min)/(double)(x->x_gui.x_w - 1);
}

void shsl_check_minmax(t_shsl *x, double min, double max)
{
    if(x->x_lin0_log1)
    {
        if((min == 0.0)&&(max == 0.0))
            max = 1.0;
        if(max > 0.0)
        {
            if(min <= 0.0)
                min = 0.01*max;
        }
        else
        {
            if(min > 0.0)
                max = 0.01*min;
        }
    }
    x->x_min = min;
    x->x_max = max;
    if(x->x_min > x->x_max)                /* bugfix */
        x->x_gui.x_isa.x_reverse = 1;
    else
        x->x_gui.x_isa.x_reverse = 0;
    if(x->x_lin0_log1)
        x->x_k = log(x->x_max/x->x_min)/(double)(x->x_gui.x_w - 1);
    else
        x->x_k = (x->x_max - x->x_min)/(double)(x->x_gui.x_w - 1);
}

static void shsl_properties(t_gobj *z, t_glist *owner)
{
    t_shsl *x = (t_shsl *)z;
    char buf[800];
    t_symbol *srl[3];

    iemgui_properties(&x->x_gui, srl);
    sprintf(buf, "pdtk_iemgui_dialog %%s |hsl| \
            --------dimensions(pix)(pix):-------- %d %d width: %d %d height: \
            -----------output-range:----------- %g left: %g right: %g \
            %d lin log %d %d empty %d \
            %s %s \
            %s %d %d \
            %d %d \
            %d %d %d\n",
            x->x_gui.x_w, IEM_SL_MINSIZE, x->x_gui.x_h, IEM_GUI_MINSIZE,
            x->x_min, x->x_max, 0.0,/*no_schedule*/
            x->x_lin0_log1, x->x_gui.x_isa.x_loadinit, x->x_steady, -1,/*no multi, but iem-characteristic*/
            srl[0]->s_name, srl[1]->s_name,
            srl[2]->s_name, x->x_gui.x_ldx, x->x_gui.x_ldy,
            x->x_gui.x_fsf.x_font_style, x->x_gui.x_fontsize,
            0xffffff & x->x_gui.x_bcol, 0xffffff & x->x_gui.x_fcol, 0xffffff & x->x_gui.x_lcol);
    gfxstub_new(&x->x_gui.x_obj.ob_pd, x, buf);
}

static void shsl_set(t_shsl *x, t_floatarg f)    /* bugfix */
{
    double g;

    if(x->x_gui.x_isa.x_reverse)    /* bugfix */
    {
        if(f > x->x_min)
            f = x->x_min;
        if(f < x->x_max)
            f = x->x_max;
    }
    else
    {
        if(f > x->x_max)
            f = x->x_max;
        if(f < x->x_min)
            f = x->x_min;
    }
    if(x->x_lin0_log1)
        g = log(f/x->x_min)/x->x_k;
    else
        g = (f - x->x_min) / x->x_k;
    x->x_val = (int)(100.0*g + 0.49999);
    x->x_pos = x->x_val;
    (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
}

static void shsl_bang(t_shsl *x)
{
    double out;

    if(x->x_lin0_log1)
        out = x->x_min*exp(x->x_k*(double)(x->x_val)*0.01);
    else
        out = (double)(x->x_val)*0.01*x->x_k + x->x_min;
    if((out < 1.0e-10)&&(out > -1.0e-10))
        out = 0.0;
    outlet_float(x->x_gui.x_obj.ob_outlet, out);
    if(x->x_gui.x_fsf.x_snd_able && x->x_gui.x_snd->s_thing)
        pd_float(x->x_gui.x_snd->s_thing, out);
}

static void shsl_dialog(t_shsl *x, t_symbol *s, int argc, t_atom *argv)
{
    t_symbol *srl[3];
    int w = (int)atom_getintarg(0, argc, argv);
    int h = (int)atom_getintarg(1, argc, argv);
    double min = (double)atom_getfloatarg(2, argc, argv);
    double max = (double)atom_getfloatarg(3, argc, argv);
    int lilo = (int)atom_getintarg(4, argc, argv);
    int steady = (int)atom_getintarg(17, argc, argv);
    int sr_flags;

    if(lilo != 0) lilo = 1;
    x->x_lin0_log1 = lilo;
    if(steady)
        x->x_steady = 1;
    else
        x->x_steady = 0;
    sr_flags = iemgui_dialog(&x->x_gui, srl, argc, argv);
    x->x_gui.x_h = iemgui_clip_size(h);
    shsl_check_width(x, w);
    shsl_check_minmax(x, min, max);
    (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_CONFIG);
    (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_IO + sr_flags);
    (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_MOVE);
    canvas_fixlinesfor(glist_getcanvas(x->x_gui.x_glist), (t_text*)x);
}

static void shsl_motion(t_shsl *x, t_floatarg dx, t_floatarg dy)
{
    int old = x->x_val;

    if(x->x_gui.x_fsf.x_finemoved)
        x->x_pos += (int)dx;
    else
        x->x_pos += 100*(int)dx;
    x->x_val = x->x_pos;
    if(x->x_val > (100*x->x_gui.x_w - 100))
    {
        x->x_val = 100*x->x_gui.x_w - 100;
        x->x_pos += 50;
        x->x_pos -= x->x_pos%100;
    }
    if(x->x_val < 0)
    {
        x->x_val = 0;
        x->x_pos -= 50;
        x->x_pos -= x->x_pos%100;
    }
    if(old != x->x_val)
    {
        (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
        shsl_bang(x);
    }
}

static void shsl_click(t_shsl *x, t_floatarg xpos, t_floatarg ypos,
                          t_floatarg shift, t_floatarg ctrl, t_floatarg alt)
{
    if(!x->x_steady)
        x->x_val = (int)(100.0 * (xpos - text_xpix(&x->x_gui.x_obj, x->x_gui.x_glist)));
    if(x->x_val > (100*x->x_gui.x_w - 100))
        x->x_val = 100*x->x_gui.x_w - 100;
    if(x->x_val < 0)
        x->x_val = 0;
    x->x_pos = x->x_val;
    (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
    shsl_bang(x);
    glist_grab(x->x_gui.x_glist, &x->x_gui.x_obj.te_g, (t_glistmotionfn)shsl_motion,
               0, xpos, ypos);
}

static int shsl_newclick(t_gobj *z, struct _glist *glist,
                            int xpix, int ypix, int shift, int alt, int dbl, int doit)
{
    t_shsl* x = (t_shsl *)z;

    if(doit)
    {
        shsl_click( x, (t_floatarg)xpix, (t_floatarg)ypix, (t_floatarg)shift,
                       0, (t_floatarg)alt);
        if(shift)
            x->x_gui.x_fsf.x_finemoved = 1;
        else
            x->x_gui.x_fsf.x_finemoved = 0;
    }
    return (1);
}

static void shsl_size(t_shsl *x, t_symbol *s, int ac, t_atom *av)
{
    shsl_check_width(x, (int)atom_getintarg(0, ac, av));
    if(ac > 1)
        x->x_gui.x_h = iemgui_clip_size((int)atom_getintarg(1, ac, av));
    iemgui_size((void *)x, &x->x_gui);
}

static void shsl_delta(t_shsl *x, t_symbol *s, int ac, t_atom *av)
{iemgui_delta((void *)x, &x->x_gui, s, ac, av);}

static void shsl_pos(t_shsl *x, t_symbol *s, int ac, t_atom *av)
{iemgui_pos((void *)x, &x->x_gui, s, ac, av);}

static void shsl_range(t_shsl *x, t_symbol *s, int ac, t_atom *av)
{
    shsl_check_minmax(x, (double)atom_getfloatarg(0, ac, av),
                         (double)atom_getfloatarg(1, ac, av));
}

static void shsl_color(t_shsl *x, t_symbol *s, int ac, t_atom *av)
{iemgui_color((void *)x, &x->x_gui, s, ac, av);}

static void shsl_send(t_shsl *x, t_symbol *s)
{iemgui_send(x, &x->x_gui, s);}

static void shsl_receive(t_shsl *x, t_symbol *s)
{iemgui_receive(x, &x->x_gui, s);}

static void shsl_label(t_shsl *x, t_symbol *s)
{iemgui_label((void *)x, &x->x_gui, s);}

static void shsl_label_pos(t_shsl *x, t_symbol *s, int ac, t_atom *av)
{iemgui_label_pos((void *)x, &x->x_gui, s, ac, av);}

static void shsl_label_font(t_shsl *x, t_symbol *s, int ac, t_atom *av)
{iemgui_label_font((void *)x, &x->x_gui, s, ac, av);}

static void shsl_log(t_shsl *x)
{
    x->x_lin0_log1 = 1;
    shsl_check_minmax(x, x->x_min, x->x_max);
}

static void shsl_lin(t_shsl *x)
{
    x->x_lin0_log1 = 0;
    x->x_k = (x->x_max - x->x_min)/(double)(x->x_gui.x_w - 1);
}

static void shsl_init(t_shsl *x, t_floatarg f)
{
    x->x_gui.x_isa.x_loadinit = (f==0.0)?0:1;
}

static void shsl_steady(t_shsl *x, t_floatarg f)
{
    x->x_steady = (f==0.0)?0:1;
}

static void shsl_float(t_shsl *x, t_floatarg f)
{
    double out;

    shsl_set(x, f);
    if(x->x_lin0_log1)
        out = x->x_min*exp(x->x_k*(double)(x->x_val)*0.01);
    else
        out = (double)(x->x_val)*0.01*x->x_k + x->x_min;
    if((out < 1.0e-10)&&(out > -1.0e-10))
        out = 0.0;
    if(x->x_gui.x_fsf.x_put_in2out)
    {
        outlet_float(x->x_gui.x_obj.ob_outlet, out);
        if(x->x_gui.x_fsf.x_snd_able && x->x_gui.x_snd->s_thing)
            pd_float(x->x_gui.x_snd->s_thing, out);
    }
}

static void shsl_loadbang(t_shsl *x)
{
    if(!sys_noloadbang && x->x_gui.x_isa.x_loadinit)
    {
        (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
        shsl_bang(x);
    }
}

static void *shsl_new(t_symbol *s, int argc, t_atom *argv)
{
    t_shsl *x = (t_shsl *)pd_new(shsl_class);
    int bflcol[]={-262144, -1, -1};
    int w=IEM_SL_DEFAULTSIZE, h=IEM_GUI_DEFAULTSIZE;
    int lilo=0, ldx=-2, ldy=-8, f=0, v=0, steady=1;
    int fs=10;
    double min=0.0, max=(double)(IEM_SL_DEFAULTSIZE-1);
    char str[144];

    iem_inttosymargs(&x->x_gui.x_isa, 0);
    iem_inttofstyle(&x->x_gui.x_fsf, 0);

    if(((argc == 17)||(argc == 18))&&IS_A_FLOAT(argv,0)&&IS_A_FLOAT(argv,1)
       &&IS_A_FLOAT(argv,2)&&IS_A_FLOAT(argv,3)
       &&IS_A_FLOAT(argv,4)&&IS_A_FLOAT(argv,5)
       &&(IS_A_SYMBOL(argv,6)||IS_A_FLOAT(argv,6))
       &&(IS_A_SYMBOL(argv,7)||IS_A_FLOAT(argv,7))
       &&(IS_A_SYMBOL(argv,8)||IS_A_FLOAT(argv,8))
       &&IS_A_FLOAT(argv,9)&&IS_A_FLOAT(argv,10)
       &&IS_A_FLOAT(argv,11)&&IS_A_FLOAT(argv,12)&&IS_A_FLOAT(argv,13)
       &&IS_A_FLOAT(argv,14)&&IS_A_FLOAT(argv,15)&&IS_A_FLOAT(argv,16))
    {
        w = (int)atom_getintarg(0, argc, argv);
        h = (int)atom_getintarg(1, argc, argv);
        min = (double)atom_getfloatarg(2, argc, argv);
        max = (double)atom_getfloatarg(3, argc, argv);
        lilo = (int)atom_getintarg(4, argc, argv);
        iem_inttosymargs(&x->x_gui.x_isa, atom_getintarg(5, argc, argv));
        iemgui_new_getnames(&x->x_gui, 6, argv);
        ldx = (int)atom_getintarg(9, argc, argv);
        ldy = (int)atom_getintarg(10, argc, argv);
        iem_inttofstyle(&x->x_gui.x_fsf, atom_getintarg(11, argc, argv));
        fs = (int)atom_getintarg(12, argc, argv);
        bflcol[0] = (int)atom_getintarg(13, argc, argv);
        bflcol[1] = (int)atom_getintarg(14, argc, argv);
        bflcol[2] = (int)atom_getintarg(15, argc, argv);
        v = (int)atom_getintarg(16, argc, argv);
    }
    else iemgui_new_getnames(&x->x_gui, 6, 0);
    if((argc == 18)&&IS_A_FLOAT(argv,17))
        steady = (int)atom_getintarg(17, argc, argv);

    x->x_gui.x_draw = (t_iemfunptr)shsl_draw;

    x->x_gui.x_fsf.x_snd_able = 1;
    x->x_gui.x_fsf.x_rcv_able = 1;

    x->x_gui.x_glist = (t_glist *)canvas_getcurrent();
    if(x->x_gui.x_isa.x_loadinit)
        x->x_val = v;
    else
        x->x_val = 0;
    x->x_pos = x->x_val;
    if(lilo != 0) lilo = 1;
    x->x_lin0_log1 = lilo;
    if(steady != 0) steady = 1;
    x->x_steady = steady;
    if (!strcmp(x->x_gui.x_snd->s_name, "empty"))
        x->x_gui.x_fsf.x_snd_able = 0;
    if (!strcmp(x->x_gui.x_rcv->s_name, "empty"))
        x->x_gui.x_fsf.x_rcv_able = 0;
    if(x->x_gui.x_fsf.x_font_style == 1) strcpy(x->x_gui.x_font, "helvetica");
    else if(x->x_gui.x_fsf.x_font_style == 2) strcpy(x->x_gui.x_font, "times");
    else { x->x_gui.x_fsf.x_font_style = 0;
        strcpy(x->x_gui.x_font, sys_font); }
    if(x->x_gui.x_fsf.x_rcv_able)
        pd_bind(&x->x_gui.x_obj.ob_pd, x->x_gui.x_rcv);
    x->x_gui.x_ldx = ldx;
    x->x_gui.x_ldy = ldy;
    if(fs < 4)
        fs = 4;
    x->x_gui.x_fontsize = fs;
    x->x_gui.x_h = iemgui_clip_size(h);
    shsl_check_width(x, w);
    shsl_check_minmax(x, min, max);
    iemgui_all_colfromload(&x->x_gui, bflcol);
    iemgui_verify_snd_ne_rcv(&x->x_gui);
    outlet_new(&x->x_gui.x_obj, &s_float);
    return (x);
}

static void shsl_free(t_shsl *x)
{
    if(x->x_gui.x_fsf.x_rcv_able)
        pd_unbind(&x->x_gui.x_obj.ob_pd, x->x_gui.x_rcv);
    gfxstub_deleteforkey(x);
}

void shsl_setup(void)
{
    shsl_class = class_new(gensym("hsl"), (t_newmethod)shsl_new,
                              (t_method)shsl_free, sizeof(t_shsl), 0, A_GIMME, 0);
#ifndef GGEE_HSLIDER_COMPATIBLE
    class_addcreator((t_newmethod)shsl_new, gensym("shsl"), A_GIMME, 0);
#endif
    class_addbang(shsl_class,shsl_bang);
    class_addfloat(shsl_class,shsl_float);
    class_addmethod(shsl_class, (t_method)shsl_click, gensym("click"),
                    A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, 0);
    class_addmethod(shsl_class, (t_method)shsl_motion, gensym("motion"),
                    A_FLOAT, A_FLOAT, 0);
    class_addmethod(shsl_class, (t_method)shsl_dialog, gensym("dialog"), A_GIMME, 0);
    class_addmethod(shsl_class, (t_method)shsl_loadbang, gensym("loadbang"), 0);
    class_addmethod(shsl_class, (t_method)shsl_set, gensym("set"), A_FLOAT, 0);
    class_addmethod(shsl_class, (t_method)shsl_size, gensym("size"), A_GIMME, 0);
    class_addmethod(shsl_class, (t_method)shsl_delta, gensym("delta"), A_GIMME, 0);
    class_addmethod(shsl_class, (t_method)shsl_pos, gensym("pos"), A_GIMME, 0);
    class_addmethod(shsl_class, (t_method)shsl_range, gensym("range"), A_GIMME, 0);
    class_addmethod(shsl_class, (t_method)shsl_color, gensym("color"), A_GIMME, 0);
    class_addmethod(shsl_class, (t_method)shsl_send, gensym("send"), A_DEFSYM, 0);
    class_addmethod(shsl_class, (t_method)shsl_receive, gensym("receive"), A_DEFSYM, 0);
    class_addmethod(shsl_class, (t_method)shsl_label, gensym("label"), A_DEFSYM, 0);
    class_addmethod(shsl_class, (t_method)shsl_label_pos, gensym("label_pos"), A_GIMME, 0);
    class_addmethod(shsl_class, (t_method)shsl_label_font, gensym("label_font"), A_GIMME, 0);
    class_addmethod(shsl_class, (t_method)shsl_log, gensym("log"), 0);
    class_addmethod(shsl_class, (t_method)shsl_lin, gensym("lin"), 0);
    class_addmethod(shsl_class, (t_method)shsl_init, gensym("init"), A_FLOAT, 0);
    class_addmethod(shsl_class, (t_method)shsl_steady, gensym("steady"), A_FLOAT, 0);
    shsl_widgetbehavior.w_getrectfn =    shsl_getrect;
    shsl_widgetbehavior.w_displacefn =   iemgui_displace;
    shsl_widgetbehavior.w_selectfn =     iemgui_select;
    shsl_widgetbehavior.w_activatefn =   NULL;
    shsl_widgetbehavior.w_deletefn =     iemgui_delete;
    shsl_widgetbehavior.w_visfn =        iemgui_vis;
    shsl_widgetbehavior.w_clickfn =      shsl_newclick;
    class_setwidget(shsl_class, &shsl_widgetbehavior);
    class_sethelpsymbol(shsl_class, gensym("shsl"));
    class_setsavefn(shsl_class, shsl_save);
    class_setpropertiesfn(shsl_class, shsl_properties);
}
