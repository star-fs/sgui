/* Copyright (c) 1997-1999 Miller Puckette.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution. */

/* g_7_guis.c written by Thomas Musil (c) IEM KUG Graz Austria 2000-2001 */
/* thanks to Miller Puckette, Guenther Geiger and Krzystof Czaja */

/* 'svsl' gui object by Star Morin, based on g_vslider.c */

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


/* ------------ svsl gui-vertical slider ----------------------- */

t_widgetbehavior svsl_widgetbehavior;
static t_class *svsl_class;

typedef struct _svsl
{
    t_iemgui x_gui;
    int      x_pos;
    int      x_val;
    int      x_lin0_log1;
    int      x_steady;
    double   x_min;
    double   x_max;
    double   x_k;
} t_svsl;


/* widget helper functions */

static void svsl_draw_update(t_gobj *client, t_glist *glist)
{
    t_svsl *x = (t_svsl *)client;
    if (glist_isvisible(glist))
    {
        int r = text_ypix(&x->x_gui.x_obj, glist) + x->x_gui.x_h - (x->x_val + 50)/100;
        int xpos=text_xpix(&x->x_gui.x_obj, glist);
        int ypos=text_ypix(&x->x_gui.x_obj, glist);

        sys_vgui(".x%lx.c coords %lxVSL %d %d %d %d\n",
                 glist_getcanvas(glist), x, xpos, r,
                 xpos + x->x_gui.x_w, ypos + x->x_gui.x_h);
    }
}

static void svsl_draw_new(t_svsl *x, t_glist *glist)
{
    int xpos=text_xpix(&x->x_gui.x_obj, glist);
    int ypos=text_ypix(&x->x_gui.x_obj, glist);
    int r = ypos + x->x_gui.x_h - (x->x_val + 50)/100;
    t_canvas *canvas=glist_getcanvas(glist);

    sys_vgui(".x%lx.c create rectangle %d %d %d %d -fill #%6.6x -tags %lxBASE\n",
             canvas, xpos, ypos,
             xpos + x->x_gui.x_w, ypos + x->x_gui.x_h,
             x->x_gui.x_bcol, x);
    sys_vgui(".x%lx.c create rectangle %d %d %d %d -fill #%6.6x -tags %lxVSL\n",
             canvas, xpos, r,
             xpos + x->x_gui.x_w, ypos + x->x_gui.x_h, x->x_gui.x_fcol, x);
    sys_vgui(".x%lx.c create text %d %d -text {%s} -anchor w \
             -font {{%s} %d %s} -fill #%6.6x -tags %lxLABEL\n",
             canvas, xpos+x->x_gui.x_ldx, ypos+x->x_gui.x_ldy,
             strcmp(x->x_gui.x_lab->s_name, "empty")?x->x_gui.x_lab->s_name:"",
             x->x_gui.x_font, x->x_gui.x_fontsize, sys_fontweight, 
			 x->x_gui.x_lcol, x);
    if(!x->x_gui.x_fsf.x_snd_able)
        sys_vgui(".x%lx.c create rectangle %d %d %d %d -tags %lxOUT%d\n",
             canvas,
             xpos, ypos + x->x_gui.x_h-1,
             xpos+7, (ypos + x->x_gui.x_h),
             x, 0);
    if(!x->x_gui.x_fsf.x_rcv_able)
        sys_vgui(".x%lx.c create rectangle %d %d %d %d -tags %lxIN%d\n",
             canvas,
             xpos, ypos+1,
             xpos+7, ypos,
             x, 0);
}

static void svsl_draw_move(t_svsl *x, t_glist *glist)
{
    int xpos=text_xpix(&x->x_gui.x_obj, glist);
    int ypos=text_ypix(&x->x_gui.x_obj, glist);
    int r = ypos + x->x_gui.x_h - (x->x_val + 50)/100;
    t_canvas *canvas=glist_getcanvas(glist);

    sys_vgui(".x%lx.c coords %lxBASE %d %d %d %d\n",
             canvas, x,
             xpos, ypos,
             xpos + x->x_gui.x_w, ypos + x->x_gui.x_h);
    sys_vgui(".x%lx.c coords %lxVSL %d %d %d %d\n",
             canvas, x, xpos, r,
             xpos + x->x_gui.x_w, ypos + x->x_gui.x_h);
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
             xpos, ypos+1,
             xpos+7, ypos);
}

static void svsl_draw_erase(t_svsl* x,t_glist* glist)
{
    t_canvas *canvas=glist_getcanvas(glist);

    sys_vgui(".x%lx.c delete %lxBASE\n", canvas, x);
    sys_vgui(".x%lx.c delete %lxVSL\n", canvas, x);
    sys_vgui(".x%lx.c delete %lxLABEL\n", canvas, x);
    if(!x->x_gui.x_fsf.x_snd_able)
        sys_vgui(".x%lx.c delete %lxOUT%d\n", canvas, x, 0);
    if(!x->x_gui.x_fsf.x_rcv_able)
        sys_vgui(".x%lx.c delete %lxIN%d\n", canvas, x, 0);
}

static void svsl_draw_config(t_svsl* x,t_glist* glist)
{
    t_canvas *canvas=glist_getcanvas(glist);

    sys_vgui(".x%lx.c itemconfigure %lxLABEL -font {{%s} %d %s} -fill #%6.6x -text {%s} \n",
             canvas, x, x->x_gui.x_font, x->x_gui.x_fontsize, sys_fontweight, 
             x->x_gui.x_fsf.x_selected?IEM_GUI_COLOR_SELECTED:x->x_gui.x_lcol,
             strcmp(x->x_gui.x_lab->s_name, "empty")?x->x_gui.x_lab->s_name:"");
    sys_vgui(".x%lx.c itemconfigure %lxVSL -fill #%6.6x\n", canvas,
             x, x->x_gui.x_fcol);
    sys_vgui(".x%lx.c itemconfigure %lxBASE -fill #%6.6x\n", canvas,
             x, x->x_gui.x_bcol);
}

static void svsl_draw_io(t_svsl* x,t_glist* glist, int old_snd_rcv_flags)
{
    int xpos=text_xpix(&x->x_gui.x_obj, glist);
    int ypos=text_ypix(&x->x_gui.x_obj, glist);
    t_canvas *canvas=glist_getcanvas(glist);

    if((old_snd_rcv_flags & IEM_GUI_OLD_SND_FLAG) && !x->x_gui.x_fsf.x_snd_able)
        sys_vgui(".x%lx.c create rectangle %d %d %d %d -tags %lxOUT%d\n",
             canvas,
             xpos, ypos + x->x_gui.x_h+2,
             xpos+7, ypos + x->x_gui.x_h+3,
             x, 0);
    if(!(old_snd_rcv_flags & IEM_GUI_OLD_SND_FLAG) && x->x_gui.x_fsf.x_snd_able)
        sys_vgui(".x%lx.c delete %lxOUT%d\n", canvas, x, 0);
    if((old_snd_rcv_flags & IEM_GUI_OLD_RCV_FLAG) && !x->x_gui.x_fsf.x_rcv_able)
        sys_vgui(".x%lx.c create rectangle %d %d %d %d -tags %lxIN%d\n",
             canvas,
             xpos, ypos-2,
             xpos+7, ypos-1,
             x, 0);
    if(!(old_snd_rcv_flags & IEM_GUI_OLD_RCV_FLAG) && x->x_gui.x_fsf.x_rcv_able)
        sys_vgui(".x%lx.c delete %lxIN%d\n", canvas, x, 0);
}

static void svsl_draw_select(t_svsl *x, t_glist *glist)
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

void svsl_draw(t_svsl *x, t_glist *glist, int mode)
{
    if(mode == IEM_GUI_DRAW_MODE_UPDATE)
        sys_queuegui(x, glist, svsl_draw_update);
    else if(mode == IEM_GUI_DRAW_MODE_MOVE)
        svsl_draw_move(x, glist);
    else if(mode == IEM_GUI_DRAW_MODE_NEW)
        svsl_draw_new(x, glist);
    else if(mode == IEM_GUI_DRAW_MODE_SELECT)
        svsl_draw_select(x, glist);
    else if(mode == IEM_GUI_DRAW_MODE_ERASE)
        svsl_draw_erase(x, glist);
    else if(mode == IEM_GUI_DRAW_MODE_CONFIG)
        svsl_draw_config(x, glist);
    else if(mode >= IEM_GUI_DRAW_MODE_IO)
        svsl_draw_io(x, glist, mode - IEM_GUI_DRAW_MODE_IO);
}

/* ------------------------ vsl widgetbehaviour----------------------------- */


static void svsl_getrect(t_gobj *z, t_glist *glist,
                            int *xp1, int *yp1, int *xp2, int *yp2)
{
    t_svsl* x = (t_svsl*)z;

    *xp1 = text_xpix(&x->x_gui.x_obj, glist);
    *yp1 = text_ypix(&x->x_gui.x_obj, glist);
    *xp2 = *xp1 + x->x_gui.x_w;
    *yp2 = *yp1 + x->x_gui.x_h;
}

static void svsl_save(t_gobj *z, t_binbuf *b)
{
    t_svsl *x = (t_svsl *)z;
    int bflcol[3];
    t_symbol *srl[3];

    iemgui_save(&x->x_gui, srl, bflcol);
    binbuf_addv(b, "ssiisiiffiisssiiiiiiiii", gensym("#X"),gensym("obj"),
                (int)x->x_gui.x_obj.te_xpix, (int)x->x_gui.x_obj.te_ypix,
                gensym("svsl"), x->x_gui.x_w, x->x_gui.x_h,
                (float)x->x_min, (float)x->x_max,
                x->x_lin0_log1, iem_symargstoint(&x->x_gui.x_isa),
                srl[0], srl[1], srl[2],
                x->x_gui.x_ldx, x->x_gui.x_ldy,
                iem_fstyletoint(&x->x_gui.x_fsf), x->x_gui.x_fontsize,
                bflcol[0], bflcol[1], bflcol[2],
                x->x_val, x->x_steady);
    binbuf_addv(b, ";");
}

void svsl_check_height(t_svsl *x, int h)
{
    if(h < IEM_SL_MINSIZE)
        h = IEM_SL_MINSIZE;
    x->x_gui.x_h = h;
    if(x->x_val > (x->x_gui.x_h*100 - 100))
    {
        x->x_pos = x->x_gui.x_h*100 - 100;
        x->x_val = x->x_pos;
    }
    if(x->x_lin0_log1)
        x->x_k = log(x->x_max/x->x_min)/(double)(x->x_gui.x_h - 1);
    else
        x->x_k = (x->x_max - x->x_min)/(double)(x->x_gui.x_h - 1);
}

void svsl_check_minmax(t_svsl *x, double min, double max)
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
        x->x_k = log(x->x_max/x->x_min)/(double)(x->x_gui.x_h - 1);
    else
        x->x_k = (x->x_max - x->x_min)/(double)(x->x_gui.x_h - 1);
}

static void svsl_properties(t_gobj *z, t_glist *owner)
{
    t_svsl *x = (t_svsl *)z;
    char buf[800];
    t_symbol *srl[3];

    iemgui_properties(&x->x_gui, srl);

    sprintf(buf, "pdtk_iemgui_dialog %%s |vsl| \
            --------dimensions(pix)(pix):-------- %d %d width: %d %d height: \
            -----------output-range:----------- %g bottom: %g top: %d \
            %d lin log %d %d empty %d \
            %s %s \
            %s %d %d \
            %d %d \
            %d %d %d\n",
            x->x_gui.x_w, IEM_GUI_MINSIZE, x->x_gui.x_h, IEM_SL_MINSIZE,
            x->x_min, x->x_max, 0,/*no_schedule*/
            x->x_lin0_log1, x->x_gui.x_isa.x_loadinit, x->x_steady, -1,/*no multi, but iem-characteristic*/
            srl[0]->s_name, srl[1]->s_name,
            srl[2]->s_name, x->x_gui.x_ldx, x->x_gui.x_ldy,
            x->x_gui.x_fsf.x_font_style, x->x_gui.x_fontsize,
            0xffffff & x->x_gui.x_bcol, 0xffffff & x->x_gui.x_fcol, 0xffffff & x->x_gui.x_lcol);
    gfxstub_new(&x->x_gui.x_obj.ob_pd, x, buf);
}

static void svsl_bang(t_svsl *x)
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

static void svsl_dialog(t_svsl *x, t_symbol *s, int argc, t_atom *argv)
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
    x->x_gui.x_w = iemgui_clip_size(w);
    svsl_check_height(x, h);
    svsl_check_minmax(x, min, max);
    (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_CONFIG);
    (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_IO + sr_flags);
    (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_MOVE);
    canvas_fixlinesfor(glist_getcanvas(x->x_gui.x_glist), (t_text*)x);
}

static void svsl_motion(t_svsl *x, t_floatarg dx, t_floatarg dy)
{
    int old = x->x_val;

    if(x->x_gui.x_fsf.x_finemoved)
        x->x_pos -= (int)dy;
    else
        x->x_pos -= 100*(int)dy;
    x->x_val = x->x_pos;
    if(x->x_val > (100*x->x_gui.x_h - 100))
    {
        x->x_val = 100*x->x_gui.x_h - 100;
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
        svsl_bang(x);
    }
}

static void svsl_click(t_svsl *x, t_floatarg xpos, t_floatarg ypos,
                          t_floatarg shift, t_floatarg ctrl, t_floatarg alt)
{
    if(!x->x_steady)
        x->x_val = (int)(100.0 * (x->x_gui.x_h + text_ypix(&x->x_gui.x_obj, x->x_gui.x_glist) - ypos));
    if(x->x_val > (100*x->x_gui.x_h - 100))
        x->x_val = 100*x->x_gui.x_h - 100;
    if(x->x_val < 0)
        x->x_val = 0;
    x->x_pos = x->x_val;
    (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
    svsl_bang(x);
    glist_grab(x->x_gui.x_glist, &x->x_gui.x_obj.te_g,
        (t_glistmotionfn)svsl_motion, 0, xpos, ypos);
}

static int svsl_newclick(t_gobj *z, struct _glist *glist,
                            int xpix, int ypix, int shift, int alt, int dbl, int doit)
{
    t_svsl* x = (t_svsl *)z;

    if(doit)
    {
        svsl_click( x, (t_floatarg)xpix, (t_floatarg)ypix, (t_floatarg)shift,
                       0, (t_floatarg)alt);
        if(shift)
            x->x_gui.x_fsf.x_finemoved = 1;
        else
            x->x_gui.x_fsf.x_finemoved = 0;
    }
    return (1);
}

static void svsl_set(t_svsl *x, t_floatarg f)
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

static void svsl_float(t_svsl *x, t_floatarg f)
{
    svsl_set(x, f);
    if(x->x_gui.x_fsf.x_put_in2out)
        svsl_bang(x);
}

static void svsl_size(t_svsl *x, t_symbol *s, int ac, t_atom *av)
{
    x->x_gui.x_w = iemgui_clip_size((int)atom_getintarg(0, ac, av));
    if(ac > 1)
        svsl_check_height(x, (int)atom_getintarg(1, ac, av));
    iemgui_size((void *)x, &x->x_gui);
}

static void svsl_delta(t_svsl *x, t_symbol *s, int ac, t_atom *av)
{iemgui_delta((void *)x, &x->x_gui, s, ac, av);}

static void svsl_pos(t_svsl *x, t_symbol *s, int ac, t_atom *av)
{iemgui_pos((void *)x, &x->x_gui, s, ac, av);}

static void svsl_range(t_svsl *x, t_symbol *s, int ac, t_atom *av)
{
    svsl_check_minmax(x, (double)atom_getfloatarg(0, ac, av),
                         (double)atom_getfloatarg(1, ac, av));
}

static void svsl_color(t_svsl *x, t_symbol *s, int ac, t_atom *av)
{iemgui_color((void *)x, &x->x_gui, s, ac, av);}

static void svsl_send(t_svsl *x, t_symbol *s)
{iemgui_send(x, &x->x_gui, s);}

static void svsl_receive(t_svsl *x, t_symbol *s)
{iemgui_receive(x, &x->x_gui, s);}

static void svsl_label(t_svsl *x, t_symbol *s)
{iemgui_label((void *)x, &x->x_gui, s);}

static void svsl_label_pos(t_svsl *x, t_symbol *s, int ac, t_atom *av)
{iemgui_label_pos((void *)x, &x->x_gui, s, ac, av);}

static void svsl_label_font(t_svsl *x, t_symbol *s, int ac, t_atom *av)
{iemgui_label_font((void *)x, &x->x_gui, s, ac, av);}

static void svsl_log(t_svsl *x)
{
    x->x_lin0_log1 = 1;
    svsl_check_minmax(x, x->x_min, x->x_max);
}

static void svsl_lin(t_svsl *x)
{
    x->x_lin0_log1 = 0;
    x->x_k = (x->x_max - x->x_min)/(double)(x->x_gui.x_h - 1);
}

static void svsl_init(t_svsl *x, t_floatarg f)
{
    x->x_gui.x_isa.x_loadinit = (f==0.0)?0:1;
}

static void svsl_steady(t_svsl *x, t_floatarg f)
{
    x->x_steady = (f==0.0)?0:1;
}

static void svsl_loadbang(t_svsl *x)
{
    if(!sys_noloadbang && x->x_gui.x_isa.x_loadinit)
    {
        (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
        svsl_bang(x);
    }
}

static void *svsl_new(t_symbol *s, int argc, t_atom *argv)
{
    t_svsl *x = (t_svsl *)pd_new(svsl_class);
    int bflcol[]={-262144, -1, -1};
    int w=IEM_GUI_DEFAULTSIZE, h=IEM_SL_DEFAULTSIZE;
    int lilo=0, f=0, ldx=0, ldy=-9;
    int fs=10, v=0, steady=1;
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
    x->x_gui.x_draw = (t_iemfunptr)svsl_draw;
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
    if(!strcmp(x->x_gui.x_snd->s_name, "empty")) x->x_gui.x_fsf.x_snd_able = 0;
    if(!strcmp(x->x_gui.x_rcv->s_name, "empty")) x->x_gui.x_fsf.x_rcv_able = 0;
    if(x->x_gui.x_fsf.x_font_style == 1) strcpy(x->x_gui.x_font, "helvetica");
    else if(x->x_gui.x_fsf.x_font_style == 2) strcpy(x->x_gui.x_font, "times");
    else { x->x_gui.x_fsf.x_font_style = 0;
        strcpy(x->x_gui.x_font, sys_font); }
    if(x->x_gui.x_fsf.x_rcv_able) pd_bind(&x->x_gui.x_obj.ob_pd, x->x_gui.x_rcv);
    x->x_gui.x_ldx = ldx;
    x->x_gui.x_ldy = ldy;
    if(fs < 4)
        fs = 4;
    x->x_gui.x_fontsize = fs;
    x->x_gui.x_w = iemgui_clip_size(w);
    svsl_check_height(x, h);
    svsl_check_minmax(x, min, max);
    iemgui_all_colfromload(&x->x_gui, bflcol);
    iemgui_verify_snd_ne_rcv(&x->x_gui);
    outlet_new(&x->x_gui.x_obj, &s_float);
    return (x);
}

static void svsl_free(t_svsl *x)
{
    if(x->x_gui.x_fsf.x_rcv_able)
        pd_unbind(&x->x_gui.x_obj.ob_pd, x->x_gui.x_rcv);
    gfxstub_deleteforkey(x);
}

void svsl_setup(void)
{
    svsl_class = class_new(gensym("vsl"), (t_newmethod)svsl_new,
                              (t_method)svsl_free, sizeof(t_svsl), 0, A_GIMME, 0);

#ifndef GGEE_HSLIDER_COMPATIBLE
    class_addcreator((t_newmethod)svsl_new, gensym("svsl"), A_GIMME, 0);
#endif

    class_addbang(svsl_class,svsl_bang);
    class_addfloat(svsl_class,svsl_float);
    class_addmethod(svsl_class, (t_method)svsl_click, gensym("click"),
                    A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, 0);
    class_addmethod(svsl_class, (t_method)svsl_motion, gensym("motion"),
                    A_FLOAT, A_FLOAT, 0);
    class_addmethod(svsl_class, (t_method)svsl_dialog, gensym("dialog"),
                    A_GIMME, 0);
    class_addmethod(svsl_class, (t_method)svsl_loadbang, gensym("loadbang"), 0);
    class_addmethod(svsl_class, (t_method)svsl_set, gensym("set"), A_FLOAT, 0);
    class_addmethod(svsl_class, (t_method)svsl_size, gensym("size"), A_GIMME, 0);
    class_addmethod(svsl_class, (t_method)svsl_delta, gensym("delta"), A_GIMME, 0);
    class_addmethod(svsl_class, (t_method)svsl_pos, gensym("pos"), A_GIMME, 0);
    class_addmethod(svsl_class, (t_method)svsl_range, gensym("range"), A_GIMME, 0);
    class_addmethod(svsl_class, (t_method)svsl_color, gensym("color"), A_GIMME, 0);
    class_addmethod(svsl_class, (t_method)svsl_send, gensym("send"), A_DEFSYM, 0);
    class_addmethod(svsl_class, (t_method)svsl_receive, gensym("receive"), A_DEFSYM, 0);
    class_addmethod(svsl_class, (t_method)svsl_label, gensym("label"), A_DEFSYM, 0);
    class_addmethod(svsl_class, (t_method)svsl_label_pos, gensym("label_pos"), A_GIMME, 0);
    class_addmethod(svsl_class, (t_method)svsl_label_font, gensym("label_font"), A_GIMME, 0);
    class_addmethod(svsl_class, (t_method)svsl_log, gensym("log"), 0);
    class_addmethod(svsl_class, (t_method)svsl_lin, gensym("lin"), 0);
    class_addmethod(svsl_class, (t_method)svsl_init, gensym("init"), A_FLOAT, 0);
    class_addmethod(svsl_class, (t_method)svsl_steady, gensym("steady"), A_FLOAT, 0);
    svsl_widgetbehavior.w_getrectfn =    svsl_getrect;
    svsl_widgetbehavior.w_displacefn =   iemgui_displace;
    svsl_widgetbehavior.w_selectfn =     iemgui_select;
    svsl_widgetbehavior.w_activatefn =   NULL;
    svsl_widgetbehavior.w_deletefn =     iemgui_delete;
    svsl_widgetbehavior.w_visfn =        iemgui_vis;
    svsl_widgetbehavior.w_clickfn =      svsl_newclick;
    class_setwidget(svsl_class, &svsl_widgetbehavior);
    class_sethelpsymbol(svsl_class, gensym("svsl"));
    class_setsavefn(svsl_class, svsl_save);
    class_setpropertiesfn(svsl_class, svsl_properties);
}
