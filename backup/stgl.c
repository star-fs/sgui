/* Copyright (c) 1997-1999 Miller Puckette.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution. */

/* g_7_guis.c written by Thomas Musil (c) IEM KUG Graz Austria 2000-2001 */
/* thanks to Miller Puckette, Guenther Geiger and Krzystof Czaja */

/* 'sknb' gui object by Star Morin, based on g_toggle.c */

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

/* --------------- stgl rectangular resizable toggle ------------------- */

t_widgetbehavior stgl_widgetbehavior;
static t_class *stgl_class;

typedef struct _stgl
{
    t_iemgui x_gui;
    float    x_on;
    float    x_nonzero;
} t_stgl;


/* widget helper functions */

void stgl_draw_update(t_stgl *x, t_glist *glist)
{
    if(glist_isvisible(glist))
    {
        t_canvas *canvas=glist_getcanvas(glist);

        sys_vgui(".x%lx.c itemconfigure %lxTGL -fill #%6.6x\n", canvas, x,
          (x->x_on!=0.0)?x->x_gui.x_fcol:x->x_gui.x_bcol);

    }
}

void stgl_draw_new(t_stgl *x, t_glist *glist)
{
    t_canvas *canvas=glist_getcanvas(glist);
    int xx=text_xpix(&x->x_gui.x_obj, glist), yy=text_ypix(&x->x_gui.x_obj, glist);

    sys_vgui(".x%lx.c create rectangle %d %d %d %d -fill #%6.6x -tags %lxBASE\n",
             canvas, xx, yy, xx + x->x_gui.x_w, yy + x->x_gui.x_h,
             x->x_gui.x_bcol, x);

    sys_vgui(".x%lx.c create rectangle %d %d %d %d -fill #%6.6x -tags %lxTGL\n",
      canvas, xx+2, yy+2, xx + x->x_gui.x_w-2, yy + x->x_gui.x_h-2,
      (x->x_on!=0.0)?x->x_gui.x_fcol:x->x_gui.x_bcol, x);

    sys_vgui(".x%lx.c create text %d %d -text {%s} -anchor w \
             -font {{%s} %d %s} -fill #%6.6x -tags %lxLABEL\n",
             canvas, xx+x->x_gui.x_ldx,
             yy+x->x_gui.x_ldy,
             strcmp(x->x_gui.x_lab->s_name, "empty")?x->x_gui.x_lab->s_name:"",
             x->x_gui.x_font, x->x_gui.x_fontsize, sys_fontweight,
			 x->x_gui.x_lcol, x);
    if(!x->x_gui.x_fsf.x_snd_able)
        sys_vgui(".x%lx.c create rectangle %d %d %d %d -tags %lxOUT%d\n",
             canvas, xx, yy + x->x_gui.x_h-1, xx + IOWIDTH, yy + x->x_gui.x_h, x, 0);
    if(!x->x_gui.x_fsf.x_rcv_able)
        sys_vgui(".x%lx.c create rectangle %d %d %d %d -tags %lxIN%d\n",
             canvas, xx, yy, xx + IOWIDTH, yy+1, x, 0);
}

void stgl_draw_move(t_stgl *x, t_glist *glist)
{
    t_canvas *canvas=glist_getcanvas(glist);
    int xx=text_xpix(&x->x_gui.x_obj, glist), yy=text_ypix(&x->x_gui.x_obj, glist);

    sys_vgui(".x%lx.c coords %lxBASE %d %d %d %d\n",
             canvas, x, xx, yy, xx + x->x_gui.x_w, yy + x->x_gui.x_h);
    sys_vgui(".x%lx.c coords %lxTGL %d %d %d %d\n",
             canvas, x, xx+2, yy+2, xx + x->x_gui.x_w-2, yy + x->x_gui.x_h-2);
    sys_vgui(".x%lx.c coords %lxLABEL %d %d\n",
             canvas, x, xx+x->x_gui.x_ldx, yy+x->x_gui.x_ldy);
    if(!x->x_gui.x_fsf.x_snd_able)
        sys_vgui(".x%lx.c coords %lxOUT%d %d %d %d %d\n",
             canvas, x, 0, xx, yy + x->x_gui.x_h-1, xx + IOWIDTH, yy + x->x_gui.x_h);
    if(!x->x_gui.x_fsf.x_rcv_able)
        sys_vgui(".x%lx.c coords %lxIN%d %d %d %d %d\n",
             canvas, x, 0, xx, yy, xx + IOWIDTH, yy+1);
}

void stgl_draw_erase(t_stgl* x, t_glist* glist)
{
    t_canvas *canvas=glist_getcanvas(glist);

    sys_vgui(".x%lx.c delete %lxBASE\n", canvas, x);
    sys_vgui(".x%lx.c delete %lxTGL\n", canvas, x);
    sys_vgui(".x%lx.c delete %lxLABEL\n", canvas, x);
    if(!x->x_gui.x_fsf.x_snd_able)
        sys_vgui(".x%lx.c delete %lxOUT%d\n", canvas, x, 0);
    if(!x->x_gui.x_fsf.x_rcv_able)
        sys_vgui(".x%lx.c delete %lxIN%d\n", canvas, x, 0);
}

void stgl_draw_config(t_stgl* x, t_glist* glist)
{
    t_canvas *canvas=glist_getcanvas(glist);

    sys_vgui(".x%lx.c itemconfigure %lxLABEL -font {{%s} %d %s} -fill #%6.6x -text {%s} \n",
             canvas, x, x->x_gui.x_font, x->x_gui.x_fontsize, sys_fontweight,
             x->x_gui.x_fsf.x_selected?IEM_GUI_COLOR_SELECTED:x->x_gui.x_lcol,
             strcmp(x->x_gui.x_lab->s_name, "empty")?x->x_gui.x_lab->s_name:"");
    sys_vgui(".x%lx.c itemconfigure %lxBASE -fill #%6.6x\n", canvas, x,
             x->x_gui.x_bcol);
    sys_vgui(".x%lx.c itemconfigure %lxTGL -fill #%6.6x\n", canvas, x,
             x->x_on?x->x_gui.x_fcol:x->x_gui.x_bcol);
}

void stgl_draw_io(t_stgl* x, t_glist* glist, int old_snd_rcv_flags)
{
    int xpos=text_xpix(&x->x_gui.x_obj, glist);
    int ypos=text_ypix(&x->x_gui.x_obj, glist);
    t_canvas *canvas=glist_getcanvas(glist);

    if((old_snd_rcv_flags & IEM_GUI_OLD_SND_FLAG) && !x->x_gui.x_fsf.x_snd_able)
        sys_vgui(".x%lx.c create rectangle %d %d %d %d -tags %lxOUT%d\n",
             canvas, xpos,
             ypos + x->x_gui.x_h-1, xpos + IOWIDTH,
             ypos + x->x_gui.x_h, x, 0);
    if(!(old_snd_rcv_flags & IEM_GUI_OLD_SND_FLAG) && x->x_gui.x_fsf.x_snd_able)
        sys_vgui(".x%lx.c delete %lxOUT%d\n", canvas, x, 0);
    if((old_snd_rcv_flags & IEM_GUI_OLD_RCV_FLAG) && !x->x_gui.x_fsf.x_rcv_able)
        sys_vgui(".x%lx.c create rectangle %d %d %d %d -tags %lxIN%d\n",
             canvas, xpos, ypos,
             xpos + IOWIDTH, ypos+1, x, 0);
    if(!(old_snd_rcv_flags & IEM_GUI_OLD_RCV_FLAG) && x->x_gui.x_fsf.x_rcv_able)
        sys_vgui(".x%lx.c delete %lxIN%d\n", canvas, x, 0);
}

void stgl_draw_select(t_stgl* x, t_glist* glist)
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

void stgl_draw(t_stgl *x, t_glist *glist, int mode)
{
    if(mode == IEM_GUI_DRAW_MODE_UPDATE)
        stgl_draw_update(x, glist);
    else if(mode == IEM_GUI_DRAW_MODE_MOVE)
        stgl_draw_move(x, glist);
    else if(mode == IEM_GUI_DRAW_MODE_NEW)
        stgl_draw_new(x, glist);
    else if(mode == IEM_GUI_DRAW_MODE_SELECT)
        stgl_draw_select(x, glist);
    else if(mode == IEM_GUI_DRAW_MODE_ERASE)
        stgl_draw_erase(x, glist);
    else if(mode == IEM_GUI_DRAW_MODE_CONFIG)
        stgl_draw_config(x, glist);
    else if(mode >= IEM_GUI_DRAW_MODE_IO)
        stgl_draw_io(x, glist, mode - IEM_GUI_DRAW_MODE_IO);
}

/* ------------------------ tgl widgetbehaviour----------------------------- */

static void stgl_getrect(t_gobj *z, t_glist *glist, int *xp1, int *yp1, int *xp2, int *yp2)
{
    t_stgl *x = (t_stgl *)z;

    *xp1 = text_xpix(&x->x_gui.x_obj, glist);
    *yp1 = text_ypix(&x->x_gui.x_obj, glist);
    *xp2 = *xp1 + x->x_gui.x_w;
    *yp2 = *yp1 + x->x_gui.x_h;
}

static void stgl_save(t_gobj *z, t_binbuf *b)
{
    t_stgl *x = (t_stgl *)z;
    int bflcol[3];
    t_symbol *srl[3];

    iemgui_save(&x->x_gui, srl, bflcol);
    binbuf_addv(b, "ssiisiiisssiiiiiiiff", gensym("#X"),gensym("obj"),
                (int)x->x_gui.x_obj.te_xpix,
                (int)x->x_gui.x_obj.te_ypix,
                gensym("stgl"), x->x_gui.x_w, x->x_gui.x_h,
                iem_symargstoint(&x->x_gui.x_isa),
                srl[0], srl[1], srl[2],
                x->x_gui.x_ldx, x->x_gui.x_ldy,
                iem_fstyletoint(&x->x_gui.x_fsf), x->x_gui.x_fontsize,
                bflcol[0], bflcol[1], bflcol[2], x->x_on, x->x_nonzero);
    binbuf_addv(b, ";");
}

static void stgl_properties(t_gobj *z, t_glist *owner)
{
    t_stgl *x = (t_stgl *)z;
    char buf[800];
    t_symbol *srl[3];

    iemgui_properties(&x->x_gui, srl);
    sprintf(buf, "pdtk_iemgui_dialog %%s |tgl| \
            --------dimensions(pix)(pix):-------- %d %d width: %d %d height: \
            -----------non-zero-value:----------- %g value: 0.0 empty %g \
            -1 lin log %d %d empty %d \
            %s %s \
            %s %d %d \
            %d %d \
            %d %d %d\n",
            x->x_gui.x_w, IEM_GUI_MINSIZE,
            x->x_gui.x_h, IEM_GUI_MINSIZE,
            x->x_nonzero, 1.0,/*non_zero-schedule*/
            x->x_gui.x_isa.x_loadinit, -1, -1,/*no multi*/
            srl[0]->s_name, srl[1]->s_name,
            srl[2]->s_name, x->x_gui.x_ldx, x->x_gui.x_ldy,
            x->x_gui.x_fsf.x_font_style, x->x_gui.x_fontsize,
            0xffffff & x->x_gui.x_bcol, 0xffffff & x->x_gui.x_fcol, 0xffffff & x->x_gui.x_lcol);
    gfxstub_new(&x->x_gui.x_obj.ob_pd, x, buf);
}

static void stgl_bang(t_stgl *x)
{
    x->x_on = (x->x_on==0.0)?x->x_nonzero:0.0;
    (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
    outlet_float(x->x_gui.x_obj.ob_outlet, x->x_on);
    if(x->x_gui.x_fsf.x_snd_able && x->x_gui.x_snd->s_thing)
        pd_float(x->x_gui.x_snd->s_thing, x->x_on);
}

static void stgl_dialog(t_stgl *x, t_symbol *s, int argc, t_atom *argv)
{
    t_symbol *srl[3];
    int w = (int)atom_getintarg(0, argc, argv);
    int h = (int)atom_getintarg(1, argc, argv);
    float nonzero = (float)atom_getfloatarg(2, argc, argv);
    int sr_flags;

    if(nonzero == 0.0)
        nonzero = 1.0;
    x->x_nonzero = nonzero;
    if(x->x_on != 0.0)
        x->x_on = x->x_nonzero;
    sr_flags = iemgui_dialog(&x->x_gui, srl, argc, argv);
    x->x_gui.x_w = w;
    x->x_gui.x_h = h;
    (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_CONFIG);
    (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_IO + sr_flags);
    (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_MOVE);
    canvas_fixlinesfor(glist_getcanvas(x->x_gui.x_glist), (t_text*)x);
}

static void stgl_click(t_stgl *x, t_floatarg xpos, t_floatarg ypos, t_floatarg shift, t_floatarg ctrl, t_floatarg alt)
{stgl_bang(x);}

static int stgl_newclick(t_gobj *z, struct _glist *glist, int xpix, int ypix, int shift, int alt, int dbl, int doit)
{
    if(doit)
        stgl_click((t_stgl *)z, (t_floatarg)xpix, (t_floatarg)ypix, (t_floatarg)shift, 0, (t_floatarg)alt);
    return (1);
}

static void stgl_set(t_stgl *x, t_floatarg f)
{
    x->x_on = f;
    if(f != 0.0)
        x->x_nonzero = f;
    (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
}

static void stgl_float(t_stgl *x, t_floatarg f)
{
    stgl_set(x, f);
    if(x->x_gui.x_fsf.x_put_in2out)
    {
        outlet_float(x->x_gui.x_obj.ob_outlet, x->x_on);
        if(x->x_gui.x_fsf.x_snd_able && x->x_gui.x_snd->s_thing)
            pd_float(x->x_gui.x_snd->s_thing, x->x_on);
    }
}

static void stgl_fout(t_stgl *x, t_floatarg f)
{
    stgl_set(x, f);
    outlet_float(x->x_gui.x_obj.ob_outlet, x->x_on);
    if(x->x_gui.x_fsf.x_snd_able && x->x_gui.x_snd->s_thing)
        pd_float(x->x_gui.x_snd->s_thing, x->x_on);
}

static void stgl_loadbang(t_stgl *x)
{
    if(!sys_noloadbang && x->x_gui.x_isa.x_loadinit)
        stgl_fout(x, (float)x->x_on);
}

static void stgl_size(t_stgl *x, t_symbol *s, int ac, t_atom *av)
{
    x->x_gui.x_w = iemgui_clip_size((int)atom_getintarg(0, ac, av));
    x->x_gui.x_h = x->x_gui.x_w;
    iemgui_size((void *)x, &x->x_gui);
}

static void stgl_delta(t_stgl *x, t_symbol *s, int ac, t_atom *av)
{iemgui_delta((void *)x, &x->x_gui, s, ac, av);}

static void stgl_pos(t_stgl *x, t_symbol *s, int ac, t_atom *av)
{iemgui_pos((void *)x, &x->x_gui, s, ac, av);}

static void stgl_color(t_stgl *x, t_symbol *s, int ac, t_atom *av)
{iemgui_color((void *)x, &x->x_gui, s, ac, av);}

static void stgl_send(t_stgl *x, t_symbol *s)
{iemgui_send(x, &x->x_gui, s);}

static void stgl_receive(t_stgl *x, t_symbol *s)
{iemgui_receive(x, &x->x_gui, s);}

static void stgl_label(t_stgl *x, t_symbol *s)
{iemgui_label((void *)x, &x->x_gui, s);}

static void stgl_label_font(t_stgl *x, t_symbol *s, int ac, t_atom *av)
{iemgui_label_font((void *)x, &x->x_gui, s, ac, av);}

static void stgl_label_pos(t_stgl *x, t_symbol *s, int ac, t_atom *av)
{iemgui_label_pos((void *)x, &x->x_gui, s, ac, av);}

static void stgl_init(t_stgl *x, t_floatarg f)
{
    x->x_gui.x_isa.x_loadinit = (f==0.0)?0:1;
}

static void stgl_nonzero(t_stgl *x, t_floatarg f)
{
    if(f != 0.0)
        x->x_nonzero = f;
}

static void *stgl_new(t_symbol *s, int argc, t_atom *argv)
{
    t_stgl *x = (t_stgl *)pd_new(stgl_class);
    int bflcol[]={-262144, -1, -1};
    int w=IEM_GUI_DEFAULTSIZE;
    int h=IEM_GUI_DEFAULTSIZE;
    int f=0;
    int ldx=17, ldy=7;
    int fs=10;
    float on=0.0, nonzero=1.0;
    char str[144];

    iem_inttosymargs(&x->x_gui.x_isa, 0);
    iem_inttofstyle(&x->x_gui.x_fsf, 0);

    if(((argc == 14)||(argc == 15))&&IS_A_FLOAT(argv,0)
       &&IS_A_FLOAT(argv,1)
       &&IS_A_FLOAT(argv,2)
       &&(IS_A_SYMBOL(argv,3)||IS_A_FLOAT(argv,3))
       &&(IS_A_SYMBOL(argv,4)||IS_A_FLOAT(argv,4))
       &&(IS_A_SYMBOL(argv,5)||IS_A_FLOAT(argv,5))
       &&IS_A_FLOAT(argv,6)&&IS_A_FLOAT(argv,7)
       &&IS_A_FLOAT(argv,8)&&IS_A_FLOAT(argv,9)&&IS_A_FLOAT(argv,10)
       &&IS_A_FLOAT(argv,11)&&IS_A_FLOAT(argv,12)&&IS_A_FLOAT(argv,13))
    {
        post("in stgl load, w=%d, h=%d", (int)atom_getintarg(0, argc, argv), (int)atom_getintarg(1, argc, argv));
        w = (int)atom_getintarg(0, argc, argv);
        h = (int)atom_getintarg(1, argc, argv);
        iem_inttosymargs(&x->x_gui.x_isa, atom_getintarg(2, argc, argv));
        iemgui_new_getnames(&x->x_gui, 3, argv);
        ldx = (int)atom_getintarg(6, argc, argv);
        ldy = (int)atom_getintarg(7, argc, argv);
        iem_inttofstyle(&x->x_gui.x_fsf, atom_getintarg(8, argc, argv));
        fs = (int)atom_getintarg(9, argc, argv);
        bflcol[0] = (int)atom_getintarg(10, argc, argv);
        bflcol[1] = (int)atom_getintarg(11, argc, argv);
        bflcol[2] = (int)atom_getintarg(12, argc, argv);
        on = (float)atom_getfloatarg(13, argc, argv);
    }
    else iemgui_new_getnames(&x->x_gui, 2, 0);
    if((argc == 15)&&IS_A_FLOAT(argv,14))
        nonzero = (float)atom_getfloatarg(14, argc, argv);
    x->x_gui.x_draw = (t_iemfunptr)stgl_draw;

    x->x_gui.x_fsf.x_snd_able = 1;
    x->x_gui.x_fsf.x_rcv_able = 1;
    x->x_gui.x_glist = (t_glist *)canvas_getcurrent();
    if (!strcmp(x->x_gui.x_snd->s_name, "empty"))
        x->x_gui.x_fsf.x_snd_able = 0;
    if (!strcmp(x->x_gui.x_rcv->s_name, "empty"))
        x->x_gui.x_fsf.x_rcv_able = 0;
    if(x->x_gui.x_fsf.x_font_style == 1) strcpy(x->x_gui.x_font, "helvetica");
    else if(x->x_gui.x_fsf.x_font_style == 2) strcpy(x->x_gui.x_font, "times");
    else { x->x_gui.x_fsf.x_font_style = 0;
        strcpy(x->x_gui.x_font, sys_font); }
    x->x_nonzero = (nonzero!=0.0)?nonzero:1.0;
    if(x->x_gui.x_isa.x_loadinit)
        x->x_on = (on!=0.0)?nonzero:0.0;
    else
        x->x_on = 0.0;
    if (x->x_gui.x_fsf.x_rcv_able)
        pd_bind(&x->x_gui.x_obj.ob_pd, x->x_gui.x_rcv);
    x->x_gui.x_ldx = ldx;
    x->x_gui.x_ldy = ldy;

    if(fs < 4)
        fs = 4;
    x->x_gui.x_fontsize = fs;
    x->x_gui.x_w = w;
    x->x_gui.x_h = h;
    iemgui_all_colfromload(&x->x_gui, bflcol);
    iemgui_verify_snd_ne_rcv(&x->x_gui);
    outlet_new(&x->x_gui.x_obj, &s_float);
    return (x);
}

static void stgl_ff(t_stgl *x)
{
    if(x->x_gui.x_fsf.x_rcv_able)
        pd_unbind(&x->x_gui.x_obj.ob_pd, x->x_gui.x_rcv);
    gfxstub_deleteforkey(x);
}

void stgl_setup(void)
{
    stgl_class = class_new(gensym("tgl"), (t_newmethod)stgl_new,
                             (t_method)stgl_ff, sizeof(t_stgl), 0, A_GIMME, 0);
    class_addcreator((t_newmethod)stgl_new, gensym("stgl"), A_GIMME, 0);
    class_addbang(stgl_class, stgl_bang);
    class_addfloat(stgl_class, stgl_float);
    class_addmethod(stgl_class, (t_method)stgl_click, gensym("click"),
                    A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, 0);
    class_addmethod(stgl_class, (t_method)stgl_dialog, gensym("dialog"),
                    A_GIMME, 0);
    class_addmethod(stgl_class, (t_method)stgl_loadbang, gensym("loadbang"), 0);
    class_addmethod(stgl_class, (t_method)stgl_set, gensym("set"), A_FLOAT, 0);
    class_addmethod(stgl_class, (t_method)stgl_size, gensym("size"), A_GIMME, 0);
    class_addmethod(stgl_class, (t_method)stgl_delta, gensym("delta"), A_GIMME, 0);
    class_addmethod(stgl_class, (t_method)stgl_pos, gensym("pos"), A_GIMME, 0);
    class_addmethod(stgl_class, (t_method)stgl_color, gensym("color"), A_GIMME, 0);
    class_addmethod(stgl_class, (t_method)stgl_send, gensym("send"), A_DEFSYM, 0);
    class_addmethod(stgl_class, (t_method)stgl_receive, gensym("receive"), A_DEFSYM, 0);
    class_addmethod(stgl_class, (t_method)stgl_label, gensym("label"), A_DEFSYM, 0);
    class_addmethod(stgl_class, (t_method)stgl_label_pos, gensym("label_pos"), A_GIMME, 0);
    class_addmethod(stgl_class, (t_method)stgl_label_font, gensym("label_font"), A_GIMME, 0);
    class_addmethod(stgl_class, (t_method)stgl_init, gensym("init"), A_FLOAT, 0);
    class_addmethod(stgl_class, (t_method)stgl_nonzero, gensym("nonzero"), A_FLOAT, 0);
    stgl_widgetbehavior.w_getrectfn = stgl_getrect;
    stgl_widgetbehavior.w_displacefn = iemgui_displace;
    stgl_widgetbehavior.w_selectfn = iemgui_select;
    stgl_widgetbehavior.w_activatefn = NULL;
    stgl_widgetbehavior.w_deletefn = iemgui_delete;
    stgl_widgetbehavior.w_visfn = iemgui_vis;
    stgl_widgetbehavior.w_clickfn = stgl_newclick;
    class_setwidget(stgl_class, &stgl_widgetbehavior);
    class_sethelpsymbol(stgl_class, gensym("stgl"));
    class_setsavefn(stgl_class, stgl_save);
    class_setpropertiesfn(stgl_class, stgl_properties);
}
