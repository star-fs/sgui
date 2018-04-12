#include "m_pd.h"

typedef struct _sgui
{
     t_object x_obj;
} t_sgui;

static t_class* sgui_class;



static void* sgui_new(t_symbol* s) {
    t_sgui *x = (t_sgui *)pd_new( sgui_class);
    return (x);
}

 void sgui_setup(void) 
{
    sgui_class = class_new(gensym("sgui"), (t_newmethod)sgui_new, 0,
    	sizeof(t_sgui), 0, (t_atomtype)0);

    sbng_setup();
	  svsl_setup();
	  shsl_setup();
	  stgl_setup();
	  skng_setup();
     post("sgui by star morin");
     post("Contact: star@autospkr.org");
     post("sgui: version:  0.1 ");
     post("sgui: compiled: "__DATE__);
	 post("");
}
