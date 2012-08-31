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


#if defined(__APPLE__)
#  include <OpenGL/gl.h>
#else
#  include <GL/gl.h>
#endif

#include <string.h>
#include "dao_opengl.h"


#define DAOX_VBO_COUNT   3

#define DAOX_VBO_VERTEX  0
#define DAOX_VBO_STROKE  1
#define DAOX_VBO_FILL    2


#define USE_STENCIL


void DaoxGraphics_glSetColor( DaoxColor color, double alpha )
{
	glColor4f( color.red, color.green, color.blue, color.alpha * alpha );
}

void DaoxGraphics_glTriangle( DaoxPoint *pts, DaoxColor *cls, int i, int j, int k, double alpha )
{
	if( cls ) glColor4f( cls[i].red, cls[i].green, cls[i].blue, cls[i].alpha * alpha );
	glVertex2f( pts[i].x, pts[i].y );
	if( cls ) glColor4f( cls[j].red, cls[j].green, cls[j].blue, cls[j].alpha * alpha );
	glVertex2f( pts[j].x, pts[j].y );
	if( cls ) glColor4f( cls[k].red, cls[k].green, cls[k].blue, cls[k].alpha * alpha );
	glVertex2f( pts[k].x, pts[k].y );
}
void DaoxGraphics_glDrawTriangles( DaoxPointArray *points, DaoxIntArray *triangles, DaoxColorArray *colors, double alpha )
{
	DaoxColor *cls = colors && colors->count ? colors->colors : NULL;
	DaoxPoint *pts = points->points;
	int i, *ids = triangles->values;
	for(i=0; i<triangles->count; i+=3){
		glBegin( GL_LINE_LOOP );
		DaoxGraphics_glTriangle( pts, cls, ids[i+0], ids[i+1], ids[i+2], alpha );
		glEnd();
	}
}
void DaoxGraphics_glFillTriangles( DaoxPointArray *points, DaoxIntArray *triangles, DaoxColorArray *colors )
{
	DaoxColor *cls = colors && colors->count ? colors->colors : NULL;
	DaoxPoint *pts = points->points;
	int i, *ids = triangles->values;
	//DaoxGraphics_glDrawTriangles( points, triangles, colors, 1.0 ); return;
	glBegin( GL_TRIANGLES );
	for(i=0; i<triangles->count; i+=3){
		DaoxGraphics_glTriangle( pts, cls, ids[i+0], ids[i+1], ids[i+2], 1.0 );
	}
	glEnd();
}
void DaoxGraphics_TransfromMatrix( DaoxTransform transform, GLdouble matrix[16] )
{
	memset( matrix, 0, 16*sizeof(GLdouble) );
	matrix[0] = transform.Axx;
	matrix[4] = transform.Axy;
	matrix[12] = transform.Bx;
	matrix[1] = transform.Ayx;
	matrix[5] = transform.Ayy;
	matrix[13] = transform.By;
	matrix[15] = 1.0;
}
void DaoxGraphics_glDrawItem( DaoxGraphicsItem *item, DaoxTransform transform )
{
	DaoxBounds bounds;
	DaoxTransform inverse;
	DaoxGraphicsData *gd = item->gdata;
	DaoxGraphicsScene *scene = item->scene;
	GLdouble matrix[16] = {0};
	double scale = DaoxGraphicsScene_Scale( scene );
	double stroke = item->state->strokeWidth / (scale + 1E-16);
	int n = item->children ? item->children->size : 0;
	int k = stroke >= 1.0;
	int m = stroke >= 1E-3;
	int i;

	DaoxTransform_Multiply( & transform, item->state->transform );

	if( n == 0 && (item->bounds.right > item->bounds.left + 1E-6) ){
		DaoxBounds box = DaoxBounds_Transform( & item->bounds, & transform );

		if( box.left > scene->viewport.right + 1 ) return;
		if( box.right < scene->viewport.left - 1 ) return;
		if( box.bottom > scene->viewport.top + 1 ) return;
		if( box.top < scene->viewport.bottom - 1 ) return;
	}
#if 0
#endif

	inverse = DaoxTransform_Inverse( & transform );
	bounds = DaoxBounds_Transform( & item->scene->viewport, & inverse );
	//DaoxBounds_AddMargin( & bounds, 0.1 * (bounds.right - bounds.left) );
	DaoxBounds_AddMargin( & bounds, item->state->strokeWidth + 1 );
	//DaoxBounds_Print( & bounds );
	if( DaoxBounds_Contain( & gd->bounds, item->bounds ) == 0 ){
		if( DaoxBounds_Contain( & gd->bounds, bounds ) == 0 )
			DaoxGraphicsData_Reset( gd );
	}
	gd->bounds = bounds;
	DaoxGraphicsItem_UpdateData( item, item->scene );

	//if( gd->strokeTriangles->count + gd->fillTriangles->count == 0 && n == 0 ) return;

	printf( "strokeColors = %6i\n", gd->strokeColors->count );
	printf( "strokePoints = %6i\n", gd->strokePoints->count );
	printf( "strokeTriangles = %6i\n", gd->strokeTriangles->count );
	printf( "fillTriangles   = %6i\n", gd->fillTriangles->count );
#if 0
#endif
	
	DaoxGraphics_TransfromMatrix( item->state->transform, matrix );

	glPushMatrix();
	glMultMatrixd( matrix );

#ifdef USE_STENCIL
	glEnable( GL_STENCIL_TEST );
	glStencilFunc( GL_NOTEQUAL, 0x01, 0x01);
	glStencilOp( GL_REPLACE, GL_REPLACE, GL_REPLACE );
#endif
	if( gd->strokeTriangles->count && m && k ){
		if( gd->strokeColors->count < gd->strokePoints->count )
			DaoxGraphics_glSetColor( item->state->strokeColor, 1.0 );
		DaoxGraphics_glFillTriangles( gd->strokePoints, gd->strokeTriangles, gd->strokeColors );
	}

#ifdef USE_STENCIL
	glStencilFunc( GL_NOTEQUAL, 0x01, 0x01);
	glStencilOp( GL_REPLACE, GL_REPLACE, GL_REPLACE );
#endif
	if( gd->fillTriangles->count ){
		if( gd->fillColors->count < gd->fillPoints->count )
			DaoxGraphics_glSetColor( item->state->fillColor, 1.0 );
		DaoxGraphics_glFillTriangles( gd->fillPoints, gd->fillTriangles, gd->fillColors );
	}
	if( gd->strokeTriangles->count && m && k == 0 ){
		if( gd->strokeColors->count < gd->strokePoints->count )
			DaoxGraphics_glSetColor( item->state->strokeColor, stroke );
		DaoxGraphics_glDrawTriangles( gd->strokePoints, gd->strokeTriangles, gd->strokeColors, stroke );
	}
#ifdef USE_STENCIL
	glStencilFunc( GL_ALWAYS, 0x0, 0x01);
	glStencilOp( GL_REPLACE, GL_REPLACE, GL_REPLACE );
	glColor4f( 0.0, 0.0, 0.0, 0.0 );
	if( gd->strokeTriangles->count && m && k ){
		DaoxGraphics_glFillTriangles( gd->strokePoints, gd->strokeTriangles, NULL );
	}
	if( gd->fillTriangles->count ){
		DaoxGraphics_glFillTriangles( gd->fillPoints, gd->fillTriangles, NULL );
	}
	if( gd->strokeTriangles->count && m && k == 0 ){
		DaoxGraphics_glDrawTriangles( gd->strokePoints, gd->strokeTriangles, gd->strokeColors, stroke );
	}
	glDisable( GL_STENCIL_TEST );;
#endif

	if( gd->texture == 0 && item->image ){
		uchar_t *data = item->image->imageData;
		int width = item->image->width;
		int height = item->image->height;
		GLuint tid = 0;
		glGenTextures( 1, & tid ); /* TODO: delete */
		gd->texture = tid;
		glBindTexture(GL_TEXTURE_2D, tid);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		if( item->image->depth == DAOX_IMAGE_BIT24 ){
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		}else if( item->image->depth == DAOX_IMAGE_BIT32 ){
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		}
	}
	if( gd->texture && item->image ){
		int x = item->points->points[0].x;
		int y = item->points->points[0].y;
		int w = item->image->width;
		int h = item->image->height;
		GLuint tid = gd->texture;
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D,tid);
		glBegin(GL_QUADS);
		glColor3f(1,1,1);
		glTexCoord2d(0,0);  glVertex3f(x, y, 0);
		glTexCoord2d(1,0);  glVertex3f(x+w, y, 0);
		glTexCoord2d(1,1);  glVertex3f(x+w, y+h, 0);
		glTexCoord2d(0,1);  glVertex3f(x, y+h, 0);
		glEnd();
		glDisable(GL_TEXTURE_2D);
	}

	for(i=0; i<n; i++){
		DaoxGraphicsItem *it = (DaoxGraphicsItem*) item->children->items.pVoid[i];
		DaoxGraphics_glDrawItem( it, transform );
	}
	glPopMatrix();
}
void DaoxGraphics_glDrawScene( DaoxGraphicsScene *scene, double left, double right, double bottom, double top )
{
	DaoxColor bgcolor = scene->background;
	GLdouble matrix[16] = {0};
	int i, n = scene->items->size;

	glMatrixMode (GL_PROJECTION);
	glLoadIdentity ();
	glOrtho(left, right, bottom, top, 0, 1);
	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity ();

	/* Displacement trick for exact pixelization: */
	glTranslatef(0.375, 0.375, 0);

	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable( GL_LIGHTING );
	glClearColor( bgcolor.red, bgcolor.green, bgcolor.blue, bgcolor.alpha );

	DaoxGraphics_TransfromMatrix( scene->transform, matrix );

	glPushMatrix();
	glMultMatrixd( matrix );

#ifdef USE_STENCIL
	glClearStencil(0);
	glClear(GL_STENCIL_BUFFER_BIT);
#endif

	for(i=0; i<n; i++){
		DaoxGraphicsItem *it = (DaoxGraphicsItem*) scene->items->items.pVoid[i];
		DaoxGraphics_glDrawItem( it, scene->transform );
	}
	glPopMatrix();
}

void DaoxGraphics_glDrawSceneImage( DaoxGraphicsScene *scene, double left, double right, double bottom, double top, DaoxImage *image, int width, int height )
{
}
