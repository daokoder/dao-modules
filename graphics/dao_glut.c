/*
// Dao Standard Modules
// http://www.daovm.net
//
// Copyright (c) 2012, Limin Fu
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
// 
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
// OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
// SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
// OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <glut.h>
#include <stdio.h>
#include "dao_opengl.h"


static double window_width = 300.0;
static double window_height = 200.0;
static double fps_limit = 10.0;

static int fps_count = 0;
static int last_update = 0;
static int count_reset = 0;
static int test_fps = 0;

DaoxGraphicsScene *daox_current_scene = NULL;

void DaoxGraphics_glutDisplay(void)
{
  int now, interval;
  
  if( daox_current_scene )
	  DaoxGraphics_glDrawScene( daox_current_scene, 0, 0, window_width, window_height );

  glutSwapBuffers();

  now = glutGet(GLUT_ELAPSED_TIME);
  interval = now - last_update;
  if( interval < 1000.0 / fps_limit ) usleep( 2000 * (1000.0 / fps_limit - interval) );
  last_update = now;

  if( test_fps == 0 ) return;
  
  fps_count += 1;
  
  if( last_update ) printf( "FPS: %9.1f\n", 1000.0*fps_count / (float)(now - count_reset) );
  if( (now - count_reset) > 5000 ){
	  fps_count = (int)(1000.0 * fps_count / (float)(now - count_reset));
	  count_reset = now - 1000;
  }
}
void DaoxGraphics_glutInit(int width, int height, const char *title)
{
  int i, argc = 1;
  char *argv = "Dao Graphics";

  glutInit( &argc, &argv );
  
  glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_ALPHA | GLUT_STENCIL | GLUT_MULTISAMPLE);
  
  glutInitWindowPosition(0,0);
  glutInitWindowSize(width,height);
  glutCreateWindow(title);
  
  glutDisplayFunc(DaoxGraphics_glutDisplay);
  glutIdleFunc(DaoxGraphics_glutDisplay);
  
  glDisable(GL_DEPTH_TEST);
  glEnable(GL_LINE_SMOOTH);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glHint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);
  glClearColor(1.0, 1.0, 1.0, 0.0);
}



static void GLUT_Init( DaoProcess *proc, DaoValue *p[], int N )
{
	char *title = DaoValue_TryGetMBString( p[2] );
	window_width = DaoValue_TryGetInteger( p[0] );
	window_height = DaoValue_TryGetInteger( p[1] );
	fps_limit = DaoValue_TryGetInteger( p[3] );
	test_fps = DaoValue_TryGetInteger( p[4] );
	DaoxGraphics_glutInit( window_width, window_height, title );
}
static void GLUT_Show( DaoProcess *proc, DaoValue *p[], int N )
{
	daox_current_scene = (DaoxGraphicsScene*) p[0];

	glutMainLoop();
}
static DaoFuncItem DaoxGLUTMeths[]=
{
	{ GLUT_Init,     "glutInit( width = 300, height = 200, title = '', fps=10, test_fps=0 )" },
	{ GLUT_Show,     "glutShow( scene : GraphicsScene )" },
	{ NULL, NULL }
};


DAO_DLL int DaoGLUT_OnLoad( DaoVmSpace *vmSpace, DaoNamespace *ns )
{
	DaoNamespace_WrapFunctions( ns, DaoxGLUTMeths );
	return 0;
}
