/*
Standard Improved BSD License

Copyright (c) 2013, Sebastian Shader, Miller Puckette, and others. All rights
reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above  
   copyright notice, this list of conditions and the following 
   disclaimer in the documentation and/or other materials provided
   with the distribution.
3. The name of the author may not be used to endorse or promote
   products derived from this software without specific prior 
   written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
*/

#include "m_pd.h"

static t_class *numargs_class;

typedef struct _numargs {
	t_object thisun;
	t_float numbr;
} t_numargs;

void *numargs_new(void){
	t_numargs *thisclass = (t_numargs *)pd_new(numargs_class);
	
	outlet_new(&thisclass->thisun, &s_float);
	
	int num;
	t_atom *dum;
	canvas_getargs(&num, &dum);
	thisclass->numbr = (t_float)(num);
	
	return (void *)thisclass;
}

void numargs_bang(t_numargs *thisclass){
	outlet_float(thisclass->thisun.ob_outlet, thisclass->numbr);
}

void numargs_setup(void){
	numargs_class = class_new(gensym("numargs"), (t_newmethod)numargs_new,
		0, sizeof(t_numargs), 0, 0);
	
	class_addbang(numargs_class, numargs_bang);
}
