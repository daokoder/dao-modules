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


#include <gl.h>
#include <string.h>
#include "dao_opengl.h"


#define DAOX_VBO_COUNT   3

#define DAOX_VBO_VERTEX  0
#define DAOX_VBO_STROKE  1
#define DAOX_VBO_FILL    2


void DaoxGraphics_glSetColor( DaoxColor color )
{
	glColor4f( color.red, color.green, color.blue, color.alpha );
}

void DaoxGraphics_glFillRect( DaoxPoint lb, DaoxPoint rt )
{
	glBegin( GL_QUADS );
	{
		glVertex2f( lb.x, lb.y );
		glVertex2f( rt.x, lb.y );
		glVertex2f( rt.x, rt.y );
		glVertex2f( lb.x, rt.y );
	}
	glEnd();
}
void DaoxGraphics_glFillTriangle( DaoxPoint A, DaoxPoint B, DaoxPoint C )
{
	glBegin( GL_TRIANGLES );
	{
		glVertex2f( A.x, A.y );
		glVertex2f( B.x, B.y );
		glVertex2f( C.x, C.y );
	}
	glEnd();
}
void DaoxGraphics_glFillQuad( DaoxPoint *points, int count )
{
	int i;
	glBegin( GL_QUADS );
	{
		for(i=0; i<count; ++i) glVertex2f( points[i].x, points[i].y );
	}
	glEnd();
}
void DaoxGraphics_glFillPolygons( DaoxPolygonArray *polygons )
{
	int i;
	for(i=0; i<polygons->polygons->count; ++i){
		DaoxSlice polygon = polygons->polygons->slices[i];
		DaoxPoint *points = polygons->points->points + polygon.offset;
		if( polygon.count == 2 ){
			DaoxGraphics_glFillRect( points[0], points[1] );
		}else if( polygon.count == 3 ){
			DaoxGraphics_glFillTriangle( points[0], points[1], points[2] );
		}else if( polygon.count == 4 ){
			DaoxGraphics_glFillQuad( points, polygon.count );
		}
	}
}
void DaoxGraphics_glFillTriangles( DaoxPointArray *points, DaoxIntArray *triangles, DaoxColorArray *colors )
{
	DaoxColor *cls = colors && colors->count ? colors->colors : NULL;
	DaoxPoint *pts = points->points;
	int i, k1, k2, k3, *ids = triangles->values;
	glBegin( GL_TRIANGLES );
	{
		for(i=0; i<triangles->count; i+=3){
			k1 = ids[i+0];
			k2 = ids[i+1];
			k3 = ids[i+2];
			//glBegin( GL_LINE_LOOP );
			if( cls ) glColor4f( cls[k1].red, cls[k1].green, cls[k1].blue, cls[k1].alpha );
			glVertex2f( pts[k1].x, pts[k1].y );
			if( cls ) glColor4f( cls[k2].red, cls[k2].green, cls[k2].blue, cls[k2].alpha );
			glVertex2f( pts[k2].x, pts[k2].y );
			if( cls ) glColor4f( cls[k3].red, cls[k3].green, cls[k3].blue, cls[k3].alpha );
			glVertex2f( pts[k3].x, pts[k3].y );
			//glEnd();
		}
	}
	glEnd();
}
void DaoxGraphics_glDrawItem( DaoxGraphicsItem *item, DaoxTransform transform )
{
	DaoxGraphicsData *gd = item->gdata;
	DaoxGraphicsScene *scene = item->scene;
	GLdouble matrix[16] = {0};
	int i, n = item->children ? item->children->size : 0;

	DaoxTransform_Multiply( & transform, item->state->transform );

	if( n == 0 && (item->bounds.right > item->bounds.left + 1E-6) ){
		DaoxBoundingBox box = item->bounds;
		DaoxPoint P1 = DaoxTransform_TransformXY( & transform, box.left, box.bottom );
		DaoxPoint P2 = DaoxTransform_TransformXY( & transform, box.left, box.top );
		DaoxPoint P3 = DaoxTransform_TransformXY( & transform, box.right, box.top );
		DaoxPoint P4 = DaoxTransform_TransformXY( & transform, box.right, box.bottom );
		DaoxBoundingBox_Init( & box, P1 );
		DaoxBoundingBox_Update( & box, P2 );
		DaoxBoundingBox_Update( & box, P3 );
		DaoxBoundingBox_Update( & box, P4 );

		//printf( "%15f %15f %15f %15f\n", box.left, box.right, box.bottom, box.top );
		if( box.left > scene->viewport.right ) return;
		if( box.right < scene->viewport.left ) return;
		if( box.bottom > scene->viewport.top ) return;
		if( box.top < scene->viewport.bottom ) return;
	}
#if 0
#endif

	DaoxGraphicsItem_UpdateData( item, item->scene );

	if( gd->strokeTriangles->count + gd->fillTriangles->count == 0 && n == 0 ) return;

#if 0
	printf( "strokeColors = %6i\n", gd->strokeColors->count );
	printf( "strokePoints = %6i\n", gd->strokePoints->count );
	printf( "strokeTriangles = %6i\n", gd->strokeTriangles->count );
	printf( "fillTriangles   = %6i\n", gd->fillTriangles->count );
#endif
	
	memset( matrix, 0, 16*sizeof(GLdouble) );

	matrix[0] = item->state->transform.Axx;
	matrix[4] = item->state->transform.Axy;
	matrix[12] = item->state->transform.Bx;
	matrix[1] = item->state->transform.Ayx;
	matrix[5] = item->state->transform.Ayy;
	matrix[13] = item->state->transform.By;
	matrix[15] = 1.0;

	glPushMatrix();
	glMultMatrixd( matrix );

#define USE_STENCIL

#ifdef USE_STENCIL
	if( item->shape >= DAOX_GS_CIRCLE ){
		glClearStencil(0);
		glClear(GL_STENCIL_BUFFER_BIT);
		glEnable( GL_STENCIL_TEST );
		glStencilFunc( GL_NOTEQUAL, 0x01, 0x01);
		glStencilOp( GL_REPLACE, GL_REPLACE, GL_REPLACE );
	}
#endif
	if( gd->strokeTriangles->count ){
		if( gd->strokeColors->count < gd->strokePoints->count )
			DaoxGraphics_glSetColor( item->state->strokeColor );
		DaoxGraphics_glFillTriangles( gd->strokePoints, gd->strokeTriangles, gd->strokeColors );
	}

#ifdef USE_STENCIL
	if( item->shape >= DAOX_GS_CIRCLE ){
		glStencilFunc( GL_NOTEQUAL, 0x01, 0x01);
		glStencilOp( GL_REPLACE, GL_REPLACE, GL_REPLACE );
	}
#endif
	if( gd->fillTriangles->count ){
		if( gd->fillColors->count < gd->fillPoints->count )
			DaoxGraphics_glSetColor( item->state->fillColor );
		DaoxGraphics_glFillTriangles( gd->fillPoints, gd->fillTriangles, gd->fillColors );
	}
#ifdef USE_STENCIL
	if( item->shape >= DAOX_GS_CIRCLE ) glDisable( GL_STENCIL_TEST );;
#endif

	for(i=0; i<n; i++){
		DaoxGraphicsItem *it = (DaoxGraphicsItem*) item->children->items.pVoid[i];
		DaoxGraphics_glDrawItem( it, transform );
	}
	glPopMatrix();
}
void DaoxGraphics_glDrawScene( DaoxGraphicsScene *scene, double left, double right, double bottom, double top )
{
	DaoxTransform transform = { 1.0, 0.0, 0.0, 1.0, 0.0, 0.0 };
	DaoxColor bgcolor = scene->background;
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

	for(i=0; i<n; i++){
		DaoxGraphicsItem *it = (DaoxGraphicsItem*) scene->items->items.pVoid[i];
		DaoxGraphics_glDrawItem( it, transform );
	}
}
