/* Copyright (c) 1997-1999 Miller Puckette.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution. */

/* g_7_guis.c written by Thomas Musil (c) IEM KUG Graz Austria 2000-2001 */
/* thanks to Miller Puckette, Guenther Geiger and Krzystof Czaja */

/* 'sknb' gui object by Star Morin, Based on Frank Barknecht's knob */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "m_pd.h"

#ifndef PD_MAJOR_VERSION
#include "s_stuff.h"
#else 
#include "m_imp.h"
#endif

#include "g_canvas.h"
#include "t_tk.h"
#include "g_all_guis.h"
#include <math.h>

#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#define IEM_KNOB_DEFAULTSIZE 32

/* ------------ sknb gui ----------------------- */

t_widgetbehavior sknb_widgetbehavior;
static t_class *sknb_class;
t_symbol *iemgui_key_sym=0;		/* taken from g_all_guis.c */

typedef struct _sknb			/* taken from Frank's modyfied g_all_guis.h */
{
    t_iemgui x_gui;
    int      x_pos;
    int      x_val;
    int      x_lin0_log1;
    int      x_steady;
    double   x_min;
    double   x_max;
    double   x_k;
    double   x_ex;
} t_sknb;

/* widget helper functions */

static void sknb_draw_update(t_sknb *x, t_glist *glist)
{
    if (glist_isvisible(glist))
    {

      t_canvas *canvas=glist_getcanvas(glist);
      float scale = 360 / (float)(x->x_gui.x_h * 100);
      float extent = ((x->x_val * scale) * -1) - 1;

	    sys_vgui(".x%x.c itemconfigure %xKNOB -start -90 -extent %.5f -fill #%6.6x\n", 
        glist_getcanvas(glist), x, extent, x->x_gui.x_fcol);

      sys_vgui(".x%x.c itemconfigure %xBASECIRC -fill #%6.6x\n", canvas,
       x, x->x_gui.x_bcol);

    }
}

static void sknb_draw_new(t_sknb *x, t_glist *glist)
{
    int xpos=text_xpix(&x->x_gui.x_obj, glist);
    int ypos=text_ypix(&x->x_gui.x_obj, glist);

    float scale = 360 / (float)(x->x_gui.x_h * 100);
    float extent = ((x->x_val * scale) * -1) - 1;

    t_canvas *canvas=glist_getcanvas(glist);
    
    // BASECIRC
    sys_vgui(".x%x.c create oval %d %d %d %d  -fill #%6.6x -tags %xBASECIRC\n",
      canvas,
      xpos, ypos,
      xpos + x->x_gui.x_h, ypos + x->x_gui.x_h,
      x->x_gui.x_bcol, x
    );

    // KNOB
    sys_vgui(".x%x.c create arc %d %d %d %d -style pieslice -start -90 -extent %.5f -fill #%6.6x -tags %xKNOB\n",
      canvas,
      xpos + 1, ypos + 1,
      (xpos + x->x_gui.x_h) - 1, (ypos + x->x_gui.x_h) - 1,
      extent,
      x->x_gui.x_fcol, x
    );

    sys_vgui(".x%x.c create text %d %d -text {%s} -anchor w -font {{%s} %d %s} -fill #%6.6x -tags %xLABEL\n",
	     canvas, xpos+x->x_gui.x_ldx, ypos+x->x_gui.x_ldy,
	     strcmp(x->x_gui.x_lab->s_name, "empty")?x->x_gui.x_lab->s_name:"",
	     x->x_gui.x_font, x->x_gui.x_fontsize, sys_fontweight,x->x_gui.x_lcol, x);

    if(!x->x_gui.x_fsf.x_snd_able)
      sys_vgui(".x%x.c create rectangle %d %d %d %d -tags %xOUT%d\n",
	      canvas,
	      xpos, ypos + x->x_gui.x_h+2,
	      xpos+7, ypos + x->x_gui.x_h+3,
	      x, 0);

    if(!x->x_gui.x_fsf.x_rcv_able)
	    sys_vgui(".x%x.c create rectangle %d %d %d %d -tags %xIN%d\n",
	      canvas,
	      xpos, ypos-2,
	      xpos+7, ypos-1,
	      x, 0);

}

static void sknb_draw_move(t_sknb *x, t_glist *glist)
{
    int xpos=text_xpix(&x->x_gui.x_obj, glist);
    int ypos=text_ypix(&x->x_gui.x_obj, glist);
    t_canvas *canvas=glist_getcanvas(glist);

    sys_vgui(".x%x.c coords %xBASECIRC %d %d %d %d\n",
	     canvas, x, 
	     xpos,  /* x1 */     
	     ypos,  /* y1 */  
	     xpos + x->x_gui.x_h,  /* x2 */
	     ypos + x->x_gui.x_h  /* y2 */
	     );
    
    sys_vgui(".x%x.c coords %xKNOB %d %d %d %d\n", 
      canvas, x,
      xpos + 1,  /* x1 */
      ypos + 1,  /* y1 */
      xpos + x->x_gui.x_h - 1,  /* x2 */
      ypos + x->x_gui.x_h - 1  /* y2 */
    );

    sys_vgui(".x%lx.c coords %lxLABEL %d %d\n",
             canvas, x, xpos+x->x_gui.x_ldx, ypos+x->x_gui.x_ldy);

    if(!x->x_gui.x_fsf.x_snd_able)
      sys_vgui(".x%x.c coords %xOUT%d %d %d %d %d\n",
	      canvas, x, 0,
	      xpos, ypos + x->x_gui.x_h+2,
	      xpos+7, ypos + x->x_gui.x_h+3);

    if(!x->x_gui.x_fsf.x_rcv_able)
	    sys_vgui(".x%x.c coords %xIN%d %d %d %d %d\n",
	      canvas, x, 0,
	      xpos, ypos-2,
	      xpos+7, ypos-1);

}

static void sknb_draw_erase(t_sknb* x,t_glist* glist)
{
    t_canvas *canvas=glist_getcanvas(glist);

    sys_vgui(".x%x.c delete %xBASECIRC\n", canvas, x);
    sys_vgui(".x%x.c delete %xKNOB\n", canvas, x);
    sys_vgui(".x%x.c delete %xLABEL\n", canvas, x);
    if(!x->x_gui.x_fsf.x_snd_able)
        sys_vgui(".x%x.c delete %xOUT%d\n", canvas, x, 0);
    if(!x->x_gui.x_fsf.x_rcv_able)
	      sys_vgui(".x%x.c delete %xIN%d\n", canvas, x, 0);
}

static void sknb_draw_config(t_sknb* x,t_glist* glist)
{
    t_canvas *canvas=glist_getcanvas(glist);

    sys_vgui(".x%x.c itemconfigure %xLABEL -font {{%s} %d %s} -fill #%6.6x -text {%s} \n",
	     canvas, x, x->x_gui.x_font, x->x_gui.x_fontsize, sys_fontweight,
	     x->x_gui.x_fsf.x_selected?IEM_GUI_COLOR_SELECTED:x->x_gui.x_lcol,
	     strcmp(x->x_gui.x_lab->s_name, "empty")?x->x_gui.x_lab->s_name:"");
    sys_vgui(".x%x.c itemconfigure %xBASECIRC -fill #%6.6x\n", canvas,
	     x, x->x_gui.x_bcol);
    sys_vgui(".x%x.c itemconfigure %xKNOB -fill #%6.6x\n", canvas,
	     x, x->x_gui.x_fcol);
}

static void sknb_draw_io(t_sknb* x,t_glist* glist, int old_snd_rcv_flags)
{
    int xpos=text_xpix(&x->x_gui.x_obj, glist);
    int ypos=text_ypix(&x->x_gui.x_obj, glist);
    t_canvas *canvas=glist_getcanvas(glist);

    if((old_snd_rcv_flags & IEM_GUI_OLD_SND_FLAG) && !x->x_gui.x_fsf.x_snd_able)
        sys_vgui(".x%x.c create rectangle %d %d %d %d -tags %xOUT%d\n",
	     canvas,
	     xpos, ypos + x->x_gui.x_h+2,
	     xpos+7, ypos + x->x_gui.x_h+3,
	     x, 0);
    if(!(old_snd_rcv_flags & IEM_GUI_OLD_SND_FLAG) && x->x_gui.x_fsf.x_snd_able)
        sys_vgui(".x%x.c delete %xOUT%d\n", canvas, x, 0);
    if((old_snd_rcv_flags & IEM_GUI_OLD_RCV_FLAG) && !x->x_gui.x_fsf.x_rcv_able)
        sys_vgui(".x%x.c create rectangle %d %d %d %d -tags %xIN%d\n",
	     canvas,
	     xpos, ypos-2,
	     xpos+7, ypos-1,
	     x, 0);
    if(!(old_snd_rcv_flags & IEM_GUI_OLD_RCV_FLAG) && x->x_gui.x_fsf.x_rcv_able)
        sys_vgui(".x%x.c delete %xIN%d\n", canvas, x, 0);
}

static void sknb_draw_select(t_sknb *x, t_glist *glist)
{
    t_canvas *canvas=glist_getcanvas(glist);

    if(x->x_gui.x_fsf.x_selected)
    {
	    sys_vgui(".x%x.c itemconfigure %xBASECIRC -outline #%6.6x\n", canvas, x, IEM_GUI_COLOR_SELECTED);
	    sys_vgui(".x%x.c itemconfigure %xLABEL -fill #%6.6x\n", canvas, x, IEM_GUI_COLOR_SELECTED);
    }
    else
    {
	    sys_vgui(".x%x.c itemconfigure %xBASECIRC -outline #%6.6x\n", canvas, x, IEM_GUI_COLOR_NORMAL);
	    sys_vgui(".x%x.c itemconfigure %xLABEL -fill #%6.6x\n", canvas, x, x->x_gui.x_lcol);
    }
}

void sknb_draw(t_sknb *x, t_glist *glist, int mode)
{
  if(mode == IEM_GUI_DRAW_MODE_UPDATE)
	  sknb_draw_update(x, glist);
  else if(mode == IEM_GUI_DRAW_MODE_MOVE)
	  sknb_draw_move(x, glist);
  else if(mode == IEM_GUI_DRAW_MODE_NEW)
	  sknb_draw_new(x, glist);
  else if(mode == IEM_GUI_DRAW_MODE_SELECT)
	  sknb_draw_select(x, glist);
  else if(mode == IEM_GUI_DRAW_MODE_ERASE)
	  sknb_draw_erase(x, glist);
  else if(mode == IEM_GUI_DRAW_MODE_CONFIG)
	  sknb_draw_config(x, glist);
  else if(mode >= IEM_GUI_DRAW_MODE_IO)
	  sknb_draw_io(x, glist, mode - IEM_GUI_DRAW_MODE_IO);
}

/* ------------------------ sknb widgetbehaviour----------------------------- */


static void sknb_getrect(t_gobj *z, t_glist *glist,
			    int *xp1, int *yp1, int *xp2, int *yp2)
{
    t_sknb* x = (t_sknb*)z;

    *xp1 = text_xpix(&x->x_gui.x_obj, glist);
    *yp1 = text_ypix(&x->x_gui.x_obj, glist) - 2;
    *xp2 = *xp1 + x->x_gui.x_h;
    *yp2 = *yp1 + x->x_gui.x_h + 5;
}

static void sknb_save(t_gobj *z, t_binbuf *b)
{
    t_sknb *x = (t_sknb *)z;
    int bflcol[3];
    t_symbol *srl[3];

    iemgui_save(&x->x_gui, srl, bflcol);
    binbuf_addv(b, "ssiisiiffiisssiiiiiiiii", gensym("#X"),gensym("obj"),
		(t_int)x->x_gui.x_obj.te_xpix, (t_int)x->x_gui.x_obj.te_ypix,
		atom_getsymbol(binbuf_getvec(x->x_gui.x_obj.te_binbuf)),
        x->x_gui.x_h, x->x_gui.x_h,
		(float)x->x_min, (float)x->x_max,
		x->x_lin0_log1, iem_symargstoint(&x->x_gui.x_isa),
		srl[0], srl[1], srl[2],
		x->x_gui.x_ldx, x->x_gui.x_ldy,
		iem_fstyletoint(&x->x_gui.x_fsf), x->x_gui.x_fontsize,
		bflcol[0], bflcol[1], bflcol[2],
		x->x_val, x->x_steady);
    binbuf_addv(b, ";");
}

void sknb_check_height(t_sknb *x, int h)
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

void sknb_check_minmax(t_sknb *x, double min, double max)
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

static void sknb_properties(t_gobj *z, t_glist *owner)
{
    t_sknb *x = (t_sknb *)z;
    char buf[800];
    t_symbol *srl[3];

    iemgui_properties(&x->x_gui, srl);
    
    sprintf(buf, "pdtk_iemgui_dialog %%s KNOB \
	    --------dimensions(pix)(pix):-------- %d %d NONE: %d %d height: \
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

static void sknb_bang(t_sknb *x)
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

static void sknb_dialog(t_sknb *x, t_symbol *s, int argc, t_atom *argv)
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
    x->x_gui.x_h = iemgui_clip_size(w);
    sknb_check_height(x, h);
    sknb_check_minmax(x, min, max);
    (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_CONFIG);
    (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_IO + sr_flags);
    (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_MOVE);
    canvas_fixlinesfor(glist_getcanvas(x->x_gui.x_glist), (t_text*)x);
}

static void sknb_motion(t_sknb *x, t_floatarg dx, t_floatarg dy)
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
	sknb_bang(x);
    }
}

static void sknb_click(t_sknb *x, t_floatarg xpos, t_floatarg ypos,
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
    sknb_bang(x);
    glist_grab(x->x_gui.x_glist, &x->x_gui.x_obj.te_g, (t_glistmotionfn)sknb_motion,
	       0, xpos, ypos);
}

static int sknb_newclick(t_gobj *z, struct _glist *glist,
			    int xpix, int ypix, int shift, int alt, int dbl, int doit)
{
    t_sknb* x = (t_sknb *)z;

    if(doit)
    {
	sknb_click( x, (t_floatarg)xpix, (t_floatarg)ypix, (t_floatarg)shift,
		       0, (t_floatarg)alt);
	if(shift)
	    x->x_gui.x_fsf.x_finemoved = 1;
	else
	    x->x_gui.x_fsf.x_finemoved = 0;
    }
    return (1);
}

static void sknb_set(t_sknb *x, t_floatarg f)
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

static void sknb_float(t_sknb *x, t_floatarg f)
{
    sknb_set(x, f);
    if(x->x_gui.x_fsf.x_put_in2out)
	sknb_bang(x);
}

static void sknb_size(t_sknb *x, t_symbol *s, int ac, t_atom *av)
{
    x->x_gui.x_h = iemgui_clip_size((int)atom_getintarg(0, ac, av));
    if(ac > 1)
	sknb_check_height(x, (int)atom_getintarg(1, ac, av));
    iemgui_size((void *)x, &x->x_gui);
}

static void sknb_delta(t_sknb *x, t_symbol *s, int ac, t_atom *av)
{iemgui_delta((void *)x, &x->x_gui, s, ac, av);}

static void sknb_pos(t_sknb *x, t_symbol *s, int ac, t_atom *av)
{iemgui_pos((void *)x, &x->x_gui, s, ac, av);}

static void sknb_range(t_sknb *x, t_symbol *s, int ac, t_atom *av)
{
    sknb_check_minmax(x, (double)atom_getfloatarg(0, ac, av),
			 (double)atom_getfloatarg(1, ac, av));
}

static void sknb_color(t_sknb *x, t_symbol *s, int ac, t_atom *av)
{iemgui_color((void *)x, &x->x_gui, s, ac, av);}

static void sknb_send(t_sknb *x, t_symbol *s)
{iemgui_send(x, &x->x_gui, s);}

static void sknb_receive(t_sknb *x, t_symbol *s)
{iemgui_receive(x, &x->x_gui, s);}

static void sknb_label(t_sknb *x, t_symbol *s)
{iemgui_label((void *)x, &x->x_gui, s);}

static void sknb_label_pos(t_sknb *x, t_symbol *s, int ac, t_atom *av)
{iemgui_label_pos((void *)x, &x->x_gui, s, ac, av);}

static void sknb_label_font(t_sknb *x, t_symbol *s, int ac, t_atom *av)
{iemgui_label_font((void *)x, &x->x_gui, s, ac, av);}

static void sknb_log(t_sknb *x)
{
    x->x_lin0_log1 = 1;
    sknb_check_minmax(x, x->x_min, x->x_max);
}

static void sknb_lin(t_sknb *x)
{
    x->x_lin0_log1 = 0;
    x->x_k = (x->x_max - x->x_min)/(double)(x->x_gui.x_h - 1);
}

static void sknb_init(t_sknb *x, t_floatarg f)
{
    x->x_gui.x_isa.x_loadinit = (f==0.0)?0:1;
}

static void sknb_steady(t_sknb *x, t_floatarg f)
{
    x->x_steady = (f==0.0)?0:1;
}

static void sknb_loadbang(t_sknb *x)
{
  /* WARNING: this is a kludge to get this object building on
     Windows. Currently, the linker fails on the symbol
     "sys_noloadbang".  <hans@at.or.at>
   */
#ifdef _WIN32
    if(x->x_gui.x_isa.x_loadinit)
#else
    if(!sys_noloadbang && x->x_gui.x_isa.x_loadinit)
#endif
    {
	(*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
	sknb_bang(x);
    } 
}

/*
static void sknb_list(t_sknb *x, t_symbol *s, int ac, t_atom *av)
{
    int l=iemgui_list((void *)x, &x->x_gui, s, ac, av);

    if(l < 0)
    {
	if(IS_A_FLOAT(av,0))
	    sknb_float(x, atom_getfloatarg(0, ac, av));
    }
    else if(l > 0)
    {
	(*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_MOVE);
	canvas_fixlinesfor(glist_getcanvas(x->x_gui.x_glist), (t_text*)x);
    }
}
*/

static void *sknb_new(t_symbol *s, int argc, t_atom *argv)
{
    t_sknb *x = (t_sknb *)pd_new(sknb_class);
    int bflcol[]={-262144, -1, -1};
    t_symbol *srl[3];
    int w=IEM_KNOB_DEFAULTSIZE, h=IEM_KNOB_DEFAULTSIZE;
    int lilo=0, ldx=0, ldy=-8;
    int fs=8, v=0, steady=1;
    double min=0.0, max=(double)(IEM_SL_DEFAULTSIZE-1);
    char str[144];

    //srl[0] = gensym("empty");
    //srl[1] = gensym("empty");
    //srl[2] = gensym("empty");

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

    x->x_gui.x_draw = (t_iemfunptr)sknb_draw;

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
        strcpy(x->x_gui.x_font, "courier"); }
    if(x->x_gui.x_fsf.x_rcv_able)
        pd_bind(&x->x_gui.x_obj.ob_pd, x->x_gui.x_rcv);
    x->x_gui.x_ldx = ldx;
    x->x_gui.x_ldy = ldy;
    if(fs < 4)
        fs = 4;
    x->x_gui.x_fontsize = fs;
    x->x_gui.x_h = iemgui_clip_size(h);
    sknb_check_height(x, w);
    sknb_check_minmax(x, min, max);
    iemgui_all_colfromload(&x->x_gui, bflcol);
    //x->x_thick = 0;
    iemgui_verify_snd_ne_rcv(&x->x_gui);
    outlet_new(&x->x_gui.x_obj, &s_float);
    return (x);
}



static void sknb_free(t_sknb *x)
{
    if(x->x_gui.x_fsf.x_selected)
	pd_unbind(&x->x_gui.x_obj.ob_pd, iemgui_key_sym);
    if(x->x_gui.x_fsf.x_rcv_able)
	pd_unbind(&x->x_gui.x_obj.ob_pd, x->x_gui.x_rcv);
    gfxstub_deleteforkey(x);
}

void sknb_setup(void)
{
    sknb_class = class_new(gensym("sknb"), (t_newmethod)sknb_new,
			      (t_method)sknb_free, sizeof(t_sknb), 0, A_GIMME, 0);
    class_addcreator((t_newmethod)sknb_new, gensym("sknb"), A_GIMME, 0);
    class_addbang(sknb_class,sknb_bang);
    class_addfloat(sknb_class,sknb_float);

    /*    class_addlist(sknb_class, sknb_list); */
    class_addmethod(sknb_class, (t_method)sknb_click, gensym("click"),
		    A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, 0);
    class_addmethod(sknb_class, (t_method)sknb_motion, gensym("motion"),
		    A_FLOAT, A_FLOAT, 0);
    class_addmethod(sknb_class, (t_method)sknb_dialog, gensym("dialog"),
		    A_GIMME, 0);
    class_addmethod(sknb_class, (t_method)sknb_loadbang, gensym("loadbang"), 0);
    class_addmethod(sknb_class, (t_method)sknb_set, gensym("set"), A_FLOAT, 0);
    class_addmethod(sknb_class, (t_method)sknb_size, gensym("size"), A_GIMME, 0);
    class_addmethod(sknb_class, (t_method)sknb_delta, gensym("delta"), A_GIMME, 0);
    class_addmethod(sknb_class, (t_method)sknb_pos, gensym("pos"), A_GIMME, 0);
    class_addmethod(sknb_class, (t_method)sknb_range, gensym("range"), A_GIMME, 0);
    class_addmethod(sknb_class, (t_method)sknb_color, gensym("color"), A_GIMME, 0);
    class_addmethod(sknb_class, (t_method)sknb_send, gensym("send"), A_DEFSYM, 0);
    class_addmethod(sknb_class, (t_method)sknb_receive, gensym("receive"), A_DEFSYM, 0);
    class_addmethod(sknb_class, (t_method)sknb_label, gensym("label"), A_DEFSYM, 0);
    class_addmethod(sknb_class, (t_method)sknb_label_pos, gensym("label_pos"), A_GIMME, 0);
    class_addmethod(sknb_class, (t_method)sknb_label_font, gensym("label_font"), A_GIMME, 0);
    class_addmethod(sknb_class, (t_method)sknb_log, gensym("log"), 0);
    class_addmethod(sknb_class, (t_method)sknb_lin, gensym("lin"), 0);
    class_addmethod(sknb_class, (t_method)sknb_init, gensym("init"), A_FLOAT, 0);
    class_addmethod(sknb_class, (t_method)sknb_steady, gensym("steady"), A_FLOAT, 0);
    if(!iemgui_key_sym)
 	iemgui_key_sym = gensym("#keyname");
    sknb_widgetbehavior.w_getrectfn =    sknb_getrect;
    sknb_widgetbehavior.w_displacefn =   iemgui_displace;
    sknb_widgetbehavior.w_selectfn =     iemgui_select;
    sknb_widgetbehavior.w_activatefn =   NULL;
    sknb_widgetbehavior.w_deletefn =     iemgui_delete;
    sknb_widgetbehavior.w_visfn =        iemgui_vis;
    sknb_widgetbehavior.w_clickfn =      sknb_newclick;
#if PD_MINOR_VERSION < 37 /* TODO: remove old behaviour in exactly 2 months from now */
	sknb_widgetbehavior.w_propertiesfn = sknb_properties;;
    sknb_widgetbehavior.w_savefn =       sknb_save;
#else
	class_setpropertiesfn(sknb_class, &sknb_properties);
	class_setsavefn(sknb_class, &sknb_save);
#endif
	class_setwidget(sknb_class, &sknb_widgetbehavior);
    class_sethelpsymbol(sknb_class, gensym("sknb"));
}
