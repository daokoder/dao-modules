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

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "dao_graphics.h"

DaoType *daox_type_graphics_scene = NULL;
DaoType *daox_type_graphics_item = NULL;
DaoType *daox_type_graphics_line = NULL;
DaoType *daox_type_graphics_rect = NULL;
DaoType *daox_type_graphics_circle = NULL;
DaoType *daox_type_graphics_ellipse = NULL;
DaoType *daox_type_graphics_polyline = NULL;
DaoType *daox_type_graphics_polygon = NULL;
DaoType *daox_type_graphics_path = NULL;
DaoType *daox_type_graphics_text = NULL;



DaoxBezierSegment* DaoxBezierSegment_New()
{
	DaoxBezierSegment *self = (DaoxBezierSegment*) dao_calloc( 1, sizeof(DaoxBezierSegment) );
	return self;
}
void DaoxBezierSegment_Delete( DaoxBezierSegment *self )
{
	if( self->first ) DaoxBezierSegment_Delete( self->first );
	if( self->second ) DaoxBezierSegment_Delete( self->second );
	dao_free( self );
}
void DaoxBezierSegment_SetPoints( DaoxBezierSegment *self, DaoxPoint P0, DaoxPoint P1, DaoxPoint P2, DaoxPoint P3 )
{
	self->P0 = P0;
	self->P1 = P1;
	self->P2 = P2;
	self->P3 = P3;
	self->count = 1;
}
void DaoxBezierSegment_Refine( DaoxBezierSegment *self, float threshold )
{
	float D1, D2, D3 = DaoxDistance( self->P0, self->P3 );
	DaoxPoint Q1;

	Q1.x = 0.5 * (self->P1.x + self->P2.x);
	Q1.y = 0.5 * (self->P1.y + self->P2.y);
	D1 = DaoxDistance( Q1, self->P0 );
	D2 = DaoxDistance( Q1, self->P3 );
	if( (D1 + D2) < (D3 + 0.01) && D3 < 20 ){
		self->count = 1;
		if( self->first ) self->first->count = 0;
		if( self->second ) self->second->count = 0;
		return;
	}
	if( self->first == NULL ) self->first = DaoxBezierSegment_New();
	if( self->second == NULL ) self->second = DaoxBezierSegment_New();

	self->first->P0 = self->P0;
	self->first->P1.x = 0.5 * (self->P0.x + self->P1.x);
	self->first->P1.y = 0.5 * (self->P0.y + self->P1.y);
	self->first->P2.x = 0.5 * (self->first->P1.x + Q1.x);
	self->first->P2.y = 0.5 * (self->first->P1.y + Q1.y);

	self->second->P3 = self->P3;
	self->second->P2.x = 0.5 * (self->P2.x + self->P3.x);
	self->second->P2.y = 0.5 * (self->P2.y + self->P3.y);
	self->second->P1.x = 0.5 * (Q1.x + self->second->P2.x);
	self->second->P1.y = 0.5 * (Q1.y + self->second->P2.y);

	Q1.x = 0.5 * (self->first->P2.x + self->second->P1.x);
	Q1.y = 0.5 * (self->first->P2.y + self->second->P1.y);
	self->first->P3 = Q1;
	self->second->P0 = Q1;

	DaoxBezierSegment_Refine( self->first, threshold );
	DaoxBezierSegment_Refine( self->second, threshold );
	self->count = self->first->count + self->second->count;
}
void DaoxBezierSegment_ExportEndPoints( DaoxBezierSegment *self, DaoxPointArray *points )
{
	if( self->count == 1 ){
		DaoxPointArray_Push( points, self->P3 );
	}else if( self->count ){
		DaoxBezierSegment_ExportEndPoints( self->first, points );
		DaoxBezierSegment_ExportEndPoints( self->second, points );
	}
}





DaoxByteArray* DaoxByteArray_New()
{
	DaoxByteArray *self = (DaoxByteArray*) dao_calloc( 1, sizeof(DaoxByteArray) );
	return self;
}
void DaoxByteArray_Clear( DaoxByteArray *self )
{
	if( self->bytes ) dao_free( self->bytes );
	self->bytes = NULL;
	self->count = 0;
}
void DaoxByteArray_Delete( DaoxByteArray *self )
{
	DaoxByteArray_Clear( self );
	dao_free( self );
}
void DaoxByteArray_Push( DaoxByteArray *self, uchar_t byte )
{
	if( self->count >= self->capacity ){
		self->capacity += 0.2 * self->capacity + 1;
		self->bytes = (uchar_t*) dao_realloc( self->bytes, self->capacity * sizeof(uchar_t) );
	}
	self->bytes[ self->count ] = byte;
	self->count += 1;
}
void DaoxByteArray_Resize( DaoxByteArray *self, int count, uchar_t byte )
{
	if( count < 0 ) return;
	if( count < self->count ){
		self->count = count;
		return;
	}
	if( count >= self->capacity ){
		self->capacity = count;
		self->bytes = (uchar_t*) dao_realloc( self->bytes, self->capacity * sizeof(uchar_t) );
	}
	memset( self->bytes + self->count, byte, (count - self->count)*sizeof(uchar_t) );
	self->count = count;
}





DaoxPointArray* DaoxPointArray_New()
{
	DaoxPointArray *self = (DaoxPointArray*) dao_calloc( 1, sizeof(DaoxPointArray) );
	return self;
}
void DaoxPointArray_Clear( DaoxPointArray *self )
{
	if( self->points ) dao_free( self->points );
	self->points = NULL;
	self->count = 0;
}
void DaoxPointArray_Delete( DaoxPointArray *self )
{
	DaoxPointArray_Clear( self );
	dao_free( self );
}
void DaoxPointArray_PushXY( DaoxPointArray *self, float x, float y )
{
	DaoxPoint *point;
	if( self->count >= self->capacity ){
		self->capacity += 0.2 * self->capacity + 1;
		self->points = (DaoxPoint*) dao_realloc( self->points, self->capacity * sizeof(DaoxPoint) );
	}
	point = self->points + self->count;
	point->x = x;
	point->y = y;
	self->count += 1;
}
void DaoxPointArray_Push( DaoxPointArray *self, DaoxPoint point )
{
	DaoxPointArray_PushXY( self, point.x, point.y );
}





DaoxSliceArray* DaoxSliceArray_New()
{
	DaoxSliceArray *self = (DaoxSliceArray*) dao_calloc( 1, sizeof(DaoxSliceArray) );
	return self;
}
void DaoxSliceArray_Delete( DaoxSliceArray *self )
{
	if( self->slices ) dao_free( self->slices );
	dao_free( self );
}
void DaoxSliceArray_Push( DaoxSliceArray *self, int offset, int count )
{
	DaoxSlice *slice;
	if( self->count >= self->capacity ){
		self->capacity += 0.2 * self->capacity + 1;
		self->slices = (DaoxSlice*) dao_realloc( self->slices, self->capacity * sizeof(DaoxSlice) );
	}
	slice = self->slices + self->count;
	slice->offset = offset;
	slice->count = count;
	self->count += 1;
}





DaoxPolygonArray* DaoxPolygonArray_New()
{
	DaoxPolygonArray *self = (DaoxPolygonArray*) dao_calloc( 1, sizeof(DaoxPolygonArray) );
	self->points = DaoxPointArray_New();
	self->polygons = DaoxSliceArray_New();
	return self;
}
void DaoxPolygonArray_Delete( DaoxPolygonArray *self )
{
	if( self->points ) DaoxPointArray_Delete( self->points );
	if( self->polygons ) DaoxSliceArray_Delete( self->polygons );
	dao_free( self );
}
void DaoxPolygonArray_Reset( DaoxPolygonArray *self )
{
	self->points->count = 0;
	self->polygons->count = 0;
}
void DaoxPolygonArray_PushPolygon( DaoxPolygonArray *self )
{
	DaoxSliceArray_Push( self->polygons, self->points->count, 0 );
}
void DaoxPolygonArray_PushPointXY( DaoxPolygonArray *self, float x, float y )
{
	DaoxPointArray_PushXY( self->points, x, y );
	self->polygons->slices[ self->polygons->count - 1 ].count += 1;
}
void DaoxPolygonArray_PushPoint( DaoxPolygonArray *self, DaoxPoint point )
{
	DaoxPolygonArray_PushPointXY( self, point.x, point.y );
}
void DaoxPolygonArray_PushRect( DaoxPolygonArray *self, DaoxPoint lb, DaoxPoint rt )
{
	DaoxPolygonArray_PushPolygon( self );
	DaoxPolygonArray_PushPoint( self, lb );
	DaoxPolygonArray_PushPoint( self, rt );
}
void DaoxPolygonArray_PushQuad( DaoxPolygonArray *self, DaoxQuad quad )
{
	DaoxPolygonArray_PushPolygon( self );
	DaoxPolygonArray_PushPoint( self, quad.A );
	DaoxPolygonArray_PushPoint( self, quad.B );
	DaoxPolygonArray_PushPoint( self, quad.C );
	DaoxPolygonArray_PushPoint( self, quad.D );
}





DaoxPath* DaoxPath_New()
{
	DaoxPath *self = (DaoxPath*) dao_malloc( sizeof(DaoxPath) );
	self->points = DaoxPointArray_New();
	self->commands = DaoxByteArray_New();
	return self;
}
void DaoxPath_Delete( DaoxPath *self )
{
	DaoxPointArray_Delete( self->points );
	DaoxByteArray_Delete( self->commands );
	dao_free( self );
}
void DaoxPath_Reset( DaoxPath *self )
{
	self->points->count = 0;
	self->commands->count = 0;
}
void DaoxPath_MoveTo( DaoxPath *self, float x, float y )
{
	DaoxPointArray_PushXY( self->points, x, y );
	DaoxByteArray_Push( self->commands, DAOX_PATH_MOVE_TO );
}
void DaoxPath_LineTo( DaoxPath *self, float x, float y )
{
	DaoxPointArray_PushXY( self->points, x, y );
	DaoxByteArray_Push( self->commands, DAOX_PATH_LINE_TO );
}
void DaoxPath_ArcTo( DaoxPath *self, float x, float y, float degrees, int clockwise )
{
	DaoxPoint point, center;
	float t = tan( 0.5 * degrees * M_PI / 180.0 ) + 1E-9;
	float d, d2, dx, dy;
	assert( self->points->count > 0 );
	point = self->points->points[ self->points->count - 1 ];
	center.x = 0.5 * (x + point.x);
	center.y = 0.5 * (y + point.y);
	d = DaoxDistance( center, point );
	d2 = d / (tan( 0.5 * degrees * M_PI / 180.0 ) + 1E-9);
	dx = point.x - center.x;
	dy = point.y - center.y;
	if( clockwise ){
		center.x += - dy / t;
		center.y += + dx / t;
	}else{
		center.x += + dy / t;
		center.y += - dx / t;
	}
	DaoxPointArray_Push( self->points, center );
	DaoxPointArray_PushXY( self->points, x, y );
	DaoxByteArray_Push( self->commands, DAOX_PATH_ARCR_TO + (clockwise != 0) );
}
void DaoxPath_CubicTo( DaoxPath *self, float x, float y, float cx, float cy )
{
	DaoxPoint control, start;
	assert( self->commands->count > 0 );
	assert( self->commands->bytes[ self->commands->count - 1 ] == DAOX_PATH_CUBIC_TO );
	control = self->points->points[ self->points->count - 2 ];
	start = self->points->points[ self->points->count - 1 ];
	control.x = 2.0 * start.x - control.x;
	control.y = 2.0 * start.y - control.y;
	DaoxPointArray_Push( self->points, control );
	DaoxPointArray_PushXY( self->points, cx, cy );
	DaoxPointArray_PushXY( self->points, x, y );
	DaoxByteArray_Push( self->commands, DAOX_PATH_CUBIC_TO );
}
void DaoxPath_CubicTo2( DaoxPath *self, float cx0, float cy0, float x, float y, float cx, float cy )
{
	DaoxPointArray_PushXY( self->points, cx0, cy0 );
	DaoxPointArray_PushXY( self->points, cx, cy );
	DaoxPointArray_PushXY( self->points, x, y );
	DaoxByteArray_Push( self->commands, DAOX_PATH_CUBIC_TO );
}
void DaoxPath_Close( DaoxPath *self )
{
	DaoxByteArray_Push( self->commands, DAOX_PATH_CLOSE );
}







DaoxGraphicsItem* DaoxGraphicsItem_New( int shape )
{
	DaoxGraphicsItem *self = (DaoxGraphicsItem*) dao_calloc( 1, sizeof(DaoxGraphicsItem) );
	DaoType *ctype = NULL;
	switch( shape ){
	case DAOX_GS_LINE     : ctype = daox_type_graphics_line;     break;
	case DAOX_GS_RECT     : ctype = daox_type_graphics_rect;     break;
	case DAOX_GS_CIRCLE   : ctype = daox_type_graphics_circle;   break;
	case DAOX_GS_ELLIPSE  : ctype = daox_type_graphics_ellipse;  break;
	case DAOX_GS_POLYLINE : ctype = daox_type_graphics_polyline; break;
	case DAOX_GS_POLYGON  : ctype = daox_type_graphics_polygon;  break;
	case DAOX_GS_PATH     : ctype = daox_type_graphics_path;     break;
	case DAOX_GS_TEXT     : ctype = daox_type_graphics_text;     break;
	}
	assert( ctype != NULL );
	DaoCdata_InitCommon( (DaoCdata*)self, ctype );
	self->shape = shape;
	self->junction = DAOX_JUNCTION_SHARP;
	self->transform.A = 1;
	self->transform.C = 1;
	self->strokeWidth = 1;
	self->strokeColor.alpha = 1;
	self->path = DaoxPath_New();
	self->dashMasks = DaoxPolygonArray_New();
	self->strokePolygons = DaoxPolygonArray_New();
	self->fillPolygons = DaoxPolygonArray_New();
	return self;
}
void DaoxGraphicsItem_Delete( DaoxGraphicsItem *self )
{
	if( self->text ) DString_Delete( self->text );
	if( self->font ) DString_Delete( self->font );
	if( self->children ) DArray_Delete( self->children );
	DaoxPolygonArray_Delete( self->dashMasks );
	DaoxPolygonArray_Delete( self->strokePolygons );
	DaoxPolygonArray_Delete( self->fillPolygons );
	DaoxPath_Delete( self->path );
	DaoCdata_FreeCommon( (DaoCdata*) self );
	dao_free( self );
}


void DaoxGraphicsItem_SetStrokeWidth( DaoxGraphicsItem *self, float w )
{
	self->strokeWidth = w;
}

void DaoxGraphicsItem_SetStrokeColor( DaoxGraphicsItem *self, float r, float g, float b, float a )
{
	self->strokeColor.red = r;
	self->strokeColor.green = g;
	self->strokeColor.blue = b;
	self->strokeColor.alpha = a;
}

void DaoxGraphicsItem_SetFillColor( DaoxGraphicsItem *self, float r, float g, float b, float a )
{
	self->fillColor.red = r;
	self->fillColor.green = g;
	self->fillColor.blue = b;
	self->fillColor.alpha = a;
}


void DaoxGraphicsLine_Set( DaoxGraphicsLine *self, float x1, float y1, float x2, float y2 )
{
	assert( self->ctype == daox_type_graphics_line );
	self->position.x = x1;
	self->position.y = y1;
	self->scale.x = x2 - x1;
	self->scale.y = y2 - y1;
}

void DaoxGraphicsRect_Set( DaoxGraphicsRect *self, float x1, float y1, float x2, float y2 )
{
	assert( self->ctype == daox_type_graphics_rect );
	self->position.x = x1;
	self->position.y = y1;
	self->scale.x = x2 - x1;
	self->scale.y = y2 - y1;
}

void DaoxGraphicsCircle_Set( DaoxGraphicsCircle *self, float x, float y, float r )
{
	assert( self->ctype == daox_type_graphics_circle );
	self->position.x = x;
	self->position.y = y;
	self->scale.x = r;
	self->scale.y = r;
}

void DaoxGraphicsEllipse_Set( DaoxGraphicsEllipse *self, float x, float y, float rx, float ry )
{
	assert( self->ctype == daox_type_graphics_ellipse );
	self->position.x = x;
	self->position.y = y;
	self->scale.x = rx;
	self->scale.y = ry;
}



void DaoxGraphicsPolyLine_Add( DaoxGraphicsPolyLine *self, float x1, float y1, float x2, float y2 )
{
	assert( self->ctype == daox_type_graphics_polyline );
	DaoxPointArray_PushXY( self->path->points, x1, y1 );
	DaoxPointArray_PushXY( self->path->points, x2, y2 );
}

void DaoxGraphicsPolygon_Add( DaoxGraphicsPolygon *self, float x, float y )
{
	assert( self->ctype == daox_type_graphics_polygon );
	DaoxPointArray_PushXY( self->path->points, x, y );
}

void DaoxGraphicsPath_MoveTo( DaoxGraphicsPath *self, float x, float y )
{
	assert( self->ctype == daox_type_graphics_path );
	DaoxPath_MoveTo( self->path, x, y );
}
void DaoxGraphicsPath_LineTo( DaoxGraphicsPath *self, float x, float y )
{
	assert( self->ctype == daox_type_graphics_path );
	DaoxPath_LineTo( self->path, x, y );
}
void DaoxGraphicsPath_ArcTo( DaoxGraphicsPath *self, float x, float y, float degrees, int clockwise )
{
	assert( self->ctype == daox_type_graphics_path );
	DaoxPath_ArcTo( self->path, x, y, degrees, clockwise );
}
void DaoxGraphicsPath_CubicTo( DaoxGraphicsPath *self, float x, float y, float cx, float cy )
{
	assert( self->ctype == daox_type_graphics_path );
	DaoxPath_CubicTo( self->path, x, y, cx, cy );
}
void DaoxGraphicsPath_CubicTo2( DaoxGraphicsLine *self, float cx0, float cy0, float x, float y, float cx, float cy )
{
	assert( self->ctype == daox_type_graphics_path );
	DaoxPath_CubicTo2( self->path, cx0, cy0, x, y, cx, cy );
}
void DaoxGraphicsPath_Close( DaoxGraphicsPath *self )
{
	assert( self->ctype == daox_type_graphics_path );
	DaoxPath_Close( self->path );
}





static void DaoxGraphicsiItem_ResetPolygons( DaoxGraphicsItem *self )
{
	DaoxPolygonArray_Reset( self->dashMasks );
	DaoxPolygonArray_Reset( self->strokePolygons );
	DaoxPolygonArray_Reset( self->fillPolygons );
}


int DaoxGraphics_ArcToLines( DaoxPointArray *lines, DaoxPoint start, DaoxPoint end, DaoxPoint center, int clockwise )
{
	float CX = center.x;
	float CY = center.y;
	float R = DaoxDistance( start, center ) + 1E-6;
	float C = 2.0 * M_PI * R;
	float dA, A1, A2, A12 = 0.0;
	int i, K = C / 8;

	if( K < 32 ){
		K = 32;
	}else if( K > 256 ){
		K = 256;
	}
	dA = 2.0 * M_PI / K;
	A1 = acos( (start.x - center.x) / R );
	A2 = acos( (end.x - center.x) / R );
	if( start.y < center.y ) A1 = 2 * M_PI - A1;
	if( end.y < center.y ) A2 = 2 * M_PI - A2;

	K = lines->count;
	DaoxPointArray_Push( lines, start );
	if( clockwise ){
		A12 = 2*M_PI + A1 - A2;
		if( A12 > 2*M_PI ) A12 -= 2*M_PI;
		for(A1-=dA; A12>1.5*dA; A1-=dA, A12-=dA){
			DaoxPointArray_PushXY( lines, CX + R * cos( A1 ), CY + R * sin( A1 ) );
		}
	}else{
		A12 = 2*M_PI + A2 - A1;
		if( A12 > 2*M_PI ) A12 -= 2*M_PI;
		for(A1+=dA; A12>1.5*dA; A1+=dA, A12-=dA){
			DaoxPointArray_PushXY( lines, CX + R * cos( A1 ), CY + R * sin( A1 ) );
		}
	}
	DaoxPointArray_Push( lines, end );
	return lines->count - K;
}

void DaoxGraphicsItem_AddPolygonFromJunction( DaoxGraphicsItem *self, DaoxQuad prev, DaoxPoint cur, DaoxQuad next, int junction )
{
	DaoxSliceArray *polygons;
	DaoxLine line;
	DaoxQuad quad;
	int count = self->strokePolygons->points->count;
	switch( junction ){
	case DAOX_JUNCTION_NONE :
		break;
	case DAOX_JUNCTION_SHARP :
		line = DaoxQuadJunctionMajor( & prev, & next, cur, self->strokeWidth );
		quad.A = line.start;
		quad.B = line.end;
		quad.C = next.A;
		quad.D = prev.C;
		DaoxPolygonArray_PushQuad( self->strokePolygons, quad );
		quad.A = line.start;
		quad.B = next.B;
		quad.C = line.end;
		quad.D = prev.D;
		DaoxPolygonArray_PushQuad( self->strokePolygons, quad );
		break;
	case DAOX_JUNCTION_FLAT  :
		quad = DaoxQuadJunctionMinor( & prev, & next );
		DaoxPolygonArray_PushQuad( self->strokePolygons, quad );
		break;
	case DAOX_JUNCTION_ROUND :
		DaoxPolygonArray_PushPolygon( self->strokePolygons );
		count = DaoxGraphics_ArcToLines( self->strokePolygons->points, prev.D, next.A, cur, 0 );
		polygons = self->strokePolygons->polygons;
		polygons->slices[ polygons->count - 1 ].count = count;
		DaoxPolygonArray_PushPoint( self->strokePolygons, cur );
		break;
	}
}

void DaoxGraphicsItem_AddPolygonsFromLines( DaoxGraphicsItem *self,
		DaoxPointArray *points, DaoxByteArray *junctions,
		int junction, int close, DaoxGraphicsScene *scene )
{
	DaoxQuad quad, second;
	int i, J, M = points->count - 1;
	assert( points->count >= 2 );
	quad = second = DaoxLine2Quad( points->points[0], points->points[1], self->strokeWidth );
	DaoxPolygonArray_PushQuad( self->strokePolygons, quad );
	for(i=1; i<M; ++i){
		DaoxPoint prev = points->points[i-1];
		DaoxPoint cur = points->points[i];
		DaoxPoint next = points->points[i+1];
		J = junctions ? junctions->bytes[i] : junction;
		second = DaoxLine2Quad( cur, next, self->strokeWidth );
		DaoxGraphicsItem_AddPolygonFromJunction( self, quad, cur, second, J );
		DaoxPolygonArray_PushQuad( self->strokePolygons, second );
		quad = second;
	}
	if( close == 0 ) return;
	J = junctions ? junctions->bytes[M] : junction;
	quad = DaoxLine2Quad( points->points[M], points->points[0], self->strokeWidth );
	DaoxGraphicsItem_AddPolygonFromJunction( self, second, points->points[M], quad, J );
	second = DaoxLine2Quad( points->points[0], points->points[1], self->strokeWidth );
	DaoxPolygonArray_PushQuad( self->strokePolygons, quad );
	J = junctions ? junctions->bytes[0] : junction;
	DaoxGraphicsItem_AddPolygonFromJunction( self, quad, points->points[0], second, J );

	DaoxPolygonArray_PushPolygon( self->fillPolygons );
	for(i=0; i<=M; ++i) DaoxPolygonArray_PushPoint( self->fillPolygons, points->points[i] );
}

void DaoxGraphicsItem_AddPolygonsFromPath( DaoxGraphicsItem *self, DaoxPath *path, DaoxGraphicsScene *scene )
{
	DaoxPointArray *lines = scene->points;
	DaoxByteArray *junctions = scene->junctions;
	DaoxPoint *points = path->points->points;
	int i, K = 0;
	lines->count = 0;
	junctions->count = 0;
	for(i=0; i<path->commands->count; ++i){
		uchar_t command = path->commands->bytes[i];
		if( command == DAOX_PATH_MOVE_TO ){
			if( lines->count ){
				int close = path->commands->bytes[i-1] == DAOX_PATH_CLOSE;
				DaoxGraphicsItem_AddPolygonsFromLines( self, lines, junctions, 0, close, scene );
				lines->count = 0;
			}
			DaoxPointArray_Push( lines, points[ K++ ] );
			DaoxByteArray_Push( junctions, self->junction );
		}else if( command == DAOX_PATH_LINE_TO ){
			DaoxPointArray_Push( lines, points[ K++ ] );
			DaoxByteArray_Push( junctions, self->junction );
		}else if( command == DAOX_PATH_ARCR_TO ){
			lines->count -= 1;
			DaoxGraphics_ArcToLines( lines, points[K-1], points[K+1], points[K], 0 );
			DaoxByteArray_Resize( junctions, lines->count, DAOX_JUNCTION_ROUND );
			junctions->bytes[ junctions->count - 1 ] = self->junction;
			K += 2;
		}else if( command == DAOX_PATH_ARCL_TO ){
			lines->count -= 1;
			DaoxGraphics_ArcToLines( lines, points[K-1], points[K+1], points[K], 1 );
			DaoxByteArray_Resize( junctions, lines->count, DAOX_JUNCTION_ROUND );
			junctions->bytes[ junctions->count - 1 ] = self->junction;
			K += 2;
		}else if( command == DAOX_PATH_CUBIC_TO ){
			DaoxBezierSegment_SetPoints( scene->bezier, points[K-1], points[K], points[K+1], points[K+2] );
			DaoxBezierSegment_Refine( scene->bezier, 8 );
			DaoxBezierSegment_ExportEndPoints( scene->bezier, lines );
			DaoxByteArray_Resize( junctions, lines->count, DAOX_JUNCTION_ROUND );
			junctions->bytes[ junctions->count - 1 ] = self->junction;
			K += 3;
		}
	}
	printf( "line segment count: %i\n", lines->count );
	if( lines->count ){
		int close = path->commands->bytes[i-1] == DAOX_PATH_CLOSE;
		DaoxGraphicsItem_AddPolygonsFromLines( self, lines, junctions, 0, close, scene );
	}
}


void DaoxGraphicsLine_UpdatePolygons( DaoxGraphicsLine *self, DaoxGraphicsScene *scene )
{
	DaoxPoint end;
	DaoxQuad quad;

	end.x = self->position.x + self->scale.x;
	end.y = self->position.y + self->scale.y;
	quad = DaoxLine2Quad( self->position, end, self->strokeWidth );

	DaoxGraphicsiItem_ResetPolygons( self );
	DaoxPolygonArray_PushQuad( self->strokePolygons, quad );
}
void DaoxGraphicsRect_UpdatePolygons( DaoxGraphicsRect *self, DaoxGraphicsScene *scene )
{
	DaoxQuad centerQuad, bottomQuad, topQuad, leftQuad, rightQuad;
	float left = self->position.x;
	float bottom = self->position.y;
	float right = self->position.x + self->scale.x;
	float top = self->position.y + self->scale.y;
	float tmp, W2 = 0.5 * self->strokeWidth;

	if( left > right ) tmp = left, left = right, right = tmp;
	if( bottom > top ) tmp = bottom, bottom = top, top = tmp;

	centerQuad = DaoxQuad_FromRect( left + W2, bottom + W2, right - W2, top - W2 );
	bottomQuad = DaoxQuad_FromRect( left - W2, bottom - W2, right + W2, bottom + W2 );
	topQuad    = DaoxQuad_FromRect( left - W2, top - W2, right + W2, top + W2 );
	leftQuad   = DaoxQuad_FromRect( left - W2, bottom + W2, left + W2, top - W2 );
	rightQuad  = DaoxQuad_FromRect( right - W2, bottom + W2, right + W2, top - W2 );

	DaoxGraphicsiItem_ResetPolygons( self );

	DaoxPolygonArray_PushRect( self->fillPolygons, centerQuad.A, centerQuad.C );
	DaoxPolygonArray_PushRect( self->strokePolygons, bottomQuad.A, bottomQuad.C );
	DaoxPolygonArray_PushRect( self->strokePolygons, topQuad.A, topQuad.C );
	DaoxPolygonArray_PushRect( self->strokePolygons, leftQuad.A, leftQuad.C );
	DaoxPolygonArray_PushRect( self->strokePolygons, rightQuad.A, rightQuad.C );
}
void DaoxGraphicsEllipse_UpdatePolygons( DaoxGraphicsEllipse *self, DaoxGraphicsScene *scene )
{
	DaoxPoint innerPoints[256];
	DaoxPoint outerPoints[256];
	float CX = self->position.x;
	float CY = self->position.y;
	float RX = self->scale.x;
	float RY = self->scale.y;
	float W = self->strokeWidth;
	float W2 = 0.5 * W;
	float abm3 = 3.0 * (RX + RY + W);
	float a3b = 3.0 * RX + RY + 2.0 * W;
	float ab3 = RX + 3.0 * RY + 2.0 * W;
	float clen = M_PI * (abm3 - sqrt( a3b * ab3 ));
	int i, seg = clen / 8;

	if( seg < 32 ){
		seg = 32;
	}else if( seg > 256 ){
		seg = 256;
	}
	self->path->points->count = 0;
	for(i=0; i<seg; ++i){
		double ang = 2.0 * M_PI * i / (double) seg;
		DaoxPointArray_PushXY( scene->path->points, CX + RX * cos( ang ), CY + RY * sin( ang ) );
	}
	DaoxGraphicsiItem_ResetPolygons( self );
	DaoxGraphicsItem_AddPolygonsFromLines( self, scene->path->points, NULL, DAOX_JUNCTION_FLAT, 1, scene );
}
void DaoxGraphicsCircle_UpdatePolygons( DaoxGraphicsCircle *self, DaoxGraphicsScene *scene )
{
	DaoxGraphicsEllipse_UpdatePolygons( self, scene );
}
void DaoxGraphicsPolyLine_UpdatePolygons( DaoxGraphicsPolyLine *self, DaoxGraphicsScene *scene )
{
	int i;

	DaoxGraphicsiItem_ResetPolygons( self );

	for(i=0; i<self->path->points->count; i+=2){
		DaoxPoint start = self->path->points->points[i];
		DaoxPoint end = self->path->points->points[i+1];
		DaoxQuad quad = DaoxLine2Quad( start, end, self->strokeWidth );
		DaoxPolygonArray_PushQuad( self->strokePolygons, quad );
	}
}
void DaoxGraphicsPolygon_UpdatePolygons( DaoxGraphicsPolygon *self, DaoxGraphicsScene *scene )
{
	DaoxGraphicsiItem_ResetPolygons( self );
	DaoxGraphicsItem_AddPolygonsFromLines( self, self->path->points, NULL, self->junction, 1, scene );
}
void DaoxGraphicsPath_UpdatePolygons( DaoxGraphicsPath *self, DaoxGraphicsScene *scene )
{
	DaoxGraphicsiItem_ResetPolygons( self );
	DaoxGraphicsItem_AddPolygonsFromPath( self, self->path, scene );
}
void DaoxGraphicsText_UpdatePolygons( DaoxGraphicsText *self, DaoxGraphicsScene *scene )
{
	DaoxGraphicsiItem_ResetPolygons( self );
}

void DaoxGraphicsItem_UpdatePolygons( DaoxGraphicsItem *self, DaoxGraphicsScene *scene )
{
	if( self->strokePolygons->points->count || self->fillPolygons->points->count ) return;
	switch( self->shape ){
	case DAOX_GS_LINE     : DaoxGraphicsLine_UpdatePolygons( self, scene );     break;
	case DAOX_GS_RECT     : DaoxGraphicsRect_UpdatePolygons( self, scene );     break;
	case DAOX_GS_CIRCLE   : DaoxGraphicsCircle_UpdatePolygons( self, scene );   break;
	case DAOX_GS_ELLIPSE  : DaoxGraphicsEllipse_UpdatePolygons( self, scene );  break;
	case DAOX_GS_POLYLINE : DaoxGraphicsPolyLine_UpdatePolygons( self, scene ); break;
	case DAOX_GS_POLYGON  : DaoxGraphicsPolygon_UpdatePolygons( self, scene );  break;
	case DAOX_GS_PATH     : DaoxGraphicsPath_UpdatePolygons( self, scene );     break;
	case DAOX_GS_TEXT     : DaoxGraphicsText_UpdatePolygons( self, scene );     break;
	}
}







DaoxGraphicsScene* DaoxGraphicsScene_New()
{
	DaoxGraphicsScene *self = (DaoxGraphicsScene*) dao_calloc( 1, sizeof(DaoxGraphicsScene) );
	DaoCdata_InitCommon( (DaoCdata*) self, daox_type_graphics_scene );
	self->items = DArray_New(D_VALUE);
	self->path = DaoxPath_New();
	self->points = DaoxPointArray_New();
	self->junctions = DaoxByteArray_New();
	self->bezier = DaoxBezierSegment_New();
	return self;
}
void DaoxGraphicsScene_Delete( DaoxGraphicsScene *self )
{
	DaoCdata_FreeCommon( (DaoCdata*) self );
	DArray_Delete( self->items );
	DaoxPath_Delete( self->path );
	DaoxPointArray_Delete( self->points );
	DaoxByteArray_Delete( self->junctions );
	DaoxBezierSegment_Delete( self->bezier );
	dao_free( self );
}


DaoxGraphicsLine* DaoxGraphicsScene_AddLine( DaoxGraphicsScene *self, float x1, float y1, float x2, float y2 )
{
	DaoxGraphicsLine *item = DaoxGraphicsItem_New( DAOX_GS_LINE );
	DaoxGraphicsLine_Set( item, x1, y1, x2, y2 );
	DArray_PushBack( self->items, item );
	return item;
}

DaoxGraphicsRect* DaoxGraphicsScene_AddRect( DaoxGraphicsScene *self, float x1, float y1, float x2, float y2 )
{
	DaoxGraphicsRect *item = DaoxGraphicsItem_New( DAOX_GS_RECT );
	DaoxGraphicsRect_Set( item, x1, y1, x2, y2 );
	DArray_PushBack( self->items, item );
	return item;
}

DaoxGraphicsCircle* DaoxGraphicsScene_AddCircle( DaoxGraphicsScene *self, float x, float y, float r )
{
	DaoxGraphicsCircle *item = DaoxGraphicsItem_New( DAOX_GS_CIRCLE );
	DaoxGraphicsCircle_Set( item, x, y, r );
	DArray_PushBack( self->items, item );
	return item;
}

DaoxGraphicsEllipse* DaoxGraphicsScene_AddEllipse( DaoxGraphicsScene *self, float x, float y, float rx, float ry )
{
	DaoxGraphicsEllipse *item = DaoxGraphicsItem_New( DAOX_GS_ELLIPSE );
	DaoxGraphicsEllipse_Set( item, x, y, rx, ry );
	DArray_PushBack( self->items, item );
	return item;
}

DaoxGraphicsPolyLine* DaoxGraphicsScene_AddPolyLine( DaoxGraphicsScene *self )
{
	DaoxGraphicsPolyLine *item = DaoxGraphicsItem_New( DAOX_GS_POLYLINE );
	DArray_PushBack( self->items, item );
	return item;
}

DaoxGraphicsPolygon* DaoxGraphicsScene_AddPolygon( DaoxGraphicsScene *self )
{
	DaoxGraphicsPolygon *item = DaoxGraphicsItem_New( DAOX_GS_POLYGON );
	DArray_PushBack( self->items, item );
	return item;
}

DaoxGraphicsPath* DaoxGraphicsScene_AddPath( DaoxGraphicsScene *self )
{
	DaoxGraphicsPath *item = DaoxGraphicsItem_New( DAOX_GS_PATH );
	DArray_PushBack( self->items, item );
	return item;
}




DaoxQuad DaoxQuad_FromRect( float left, float bottom, float right, float top )
{
	DaoxQuad quad;
	quad.A.x = left;
	quad.A.y = bottom;
	quad.B.x = right;
	quad.B.y = bottom;
	quad.C.x = right;
	quad.C.y = top;
	quad.D.x = left;
	quad.D.y = top;
	return quad;
}
float DaoxDistance( DaoxPoint start, DaoxPoint end )
{
	float x1 = start.x, x2 = end.x;
	float y1 = start.y, y2 = end.y;
	return sqrt( (x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1) );
}
float DaoxDistance2( DaoxPoint start, DaoxPoint end )
{
	float x1 = start.x, x2 = end.x;
	float y1 = start.y, y2 = end.y;
	return (x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1);
}
DaoxQuad DaoxLine2Quad( DaoxPoint start, DaoxPoint end, float width )
{
	float x1 = start.x, x2 = end.x;
	float y1 = start.y, y2 = end.y;
	float dist = sqrt( (x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1) ) + 1E-6;
	float x = 0.5 * width * (x2 - x1) / dist;
	float y = 0.5 * width * (y2 - y1) / dist;
	DaoxQuad quad;
	quad.A.x = x1 + y;
	quad.A.y = y1 - x;
	quad.B.x = x1 - y;
	quad.B.y = y1 + x;
	x = -x; y = -y;
	quad.C.x = x2 + y;
	quad.C.y = y2 - x;
	quad.D.x = x2 - y;
	quad.D.y = y2 + x;
	return quad;
}
DaoxQuad DaoxQuadJunctionMinor( DaoxQuad *first, DaoxQuad *second )
{
	DaoxQuad junction;
	junction.A = first->C;
	junction.B = second->A;
	junction.C = first->D;
	junction.D = second->B;
	return junction;
}
DaoxLine DaoxQuadJunctionMajor( DaoxQuad *first, DaoxQuad *second, DaoxPoint c, float width )
{
	DaoxLine junction;
	float D1 = 2.0 * DaoxDistance2( first->C, second->A ) + 1;
	float D2 = 2.0 * DaoxDistance2( first->C, second->B ) + 1;
	float dx, dy, W = width * width + 1;
	junction.start = junction.end = c;

	dx = first->C.x - second->A.x;
	dy = first->C.y - second->A.y;
	junction.start.x += dx * W / D1;
	junction.start.y += dy * W / D1;
	junction.end.x -= dx * W / D1;
	junction.end.y -= dy * W / D1;
	return junction;
}
DaoxQuad DaoxLineJunctionMinor( DaoxPoint p1, DaoxPoint p2, DaoxPoint p3, float width )
{
	DaoxQuad first = DaoxLine2Quad( p1, p2, width );
	DaoxQuad second = DaoxLine2Quad( p2, p3, width );
	return DaoxQuadJunctionMinor( & first, & second );
}
DaoxLine DaoxLineJunctionMajor( DaoxPoint p1, DaoxPoint p2, DaoxPoint p3, float width )
{
	DaoxQuad first = DaoxLine2Quad( p1, p2, width );
	DaoxQuad second = DaoxLine2Quad( p2, p3, width );
	return DaoxQuadJunctionMajor( & first, & second, p2, width );
}






static void ITEM_SetStrokeWidth( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsItem *self = (DaoxGraphicsItem*) p[0];
	DaoxGraphicsItem_SetStrokeWidth( self, p[1]->xFloat.value );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void ITEM_SetStrokeColor( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsItem *self = (DaoxGraphicsItem*) p[0];
	float red   = p[1]->xTuple.items[0]->xFloat.value;
	float green = p[1]->xTuple.items[1]->xFloat.value;
	float blue  = p[1]->xTuple.items[2]->xFloat.value;
	float alpha = p[2]->xFloat.value;
	DaoxGraphicsItem_SetStrokeColor( self, red, green, blue, alpha );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void ITEM_SetFillColor( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsItem *self = (DaoxGraphicsItem*) p[0];
	float red   = p[1]->xTuple.items[0]->xFloat.value;
	float green = p[1]->xTuple.items[1]->xFloat.value;
	float blue  = p[1]->xTuple.items[2]->xFloat.value;
	float alpha = p[2]->xFloat.value;
	DaoxGraphicsItem_SetFillColor( self, red, green, blue, alpha );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void ITEM_AddChild( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsItem *self = (DaoxGraphicsItem*) p[0];
	DaoxGraphicsItem *child = (DaoxGraphicsItem*) p[1];
	if( self->children == NULL ) self->children = DArray_New(D_VALUE);
	DArray_PushBack( self->children, child );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}

static void DaoxGraphicsItem_GetGCFields( void *p, DArray *values, DArray *arrays, DArray *maps, int remove )
{
	daoint i, n;
	DaoxGraphicsItem *self = (DaoxGraphicsItem*) p;
	if( self->children == NULL ) return;
	DArray_Append( arrays, self->children );
}

static DaoFuncItem DaoxGraphicsItemMeths[]=
{
	{ ITEM_SetStrokeWidth,  "SetStrokeWidth( self : GraphicsItem, width = 1.0 ) => GraphicsItem" },
	{ ITEM_SetStrokeColor,  "SetStrokeColor( self : GraphicsItem, color : tuple<red:float,green:float,blue:float>, alpha = 1.0 ) => GraphicsItem" },
	{ ITEM_SetFillColor,  "SetFillColor( self : GraphicsItem, color : tuple<red:float,green:float,blue:float>, alpha = 1.0 ) => GraphicsItem" },

	{ ITEM_AddChild,  "AddChild( self : GraphicsItem, child : GraphicsItem ) => GraphicsItem" },
	{ NULL, NULL }
};

DaoTypeBase DaoxGraphicsItem_Typer =
{
	"GraphicsItem", NULL, NULL, (DaoFuncItem*) DaoxGraphicsItemMeths, {0}, {0},
	(FuncPtrDel)DaoxGraphicsItem_Delete, DaoxGraphicsItem_GetGCFields
};




static void LINE_SetData( DaoxGraphicsLine *self, DaoValue *p[] )
{
	float x1 = p[0]->xFloat.value;
	float y1 = p[1]->xFloat.value;
	float x2 = p[2]->xFloat.value;
	float y2 = p[3]->xFloat.value;
	DaoxGraphicsLine_Set( self, x1, y1, x2, y2 );
}
static void LINE_New( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsLine *self = DaoxGraphicsItem_New( DAOX_GS_LINE );
	DaoProcess_PutValue( proc, (DaoValue*) self );
	LINE_SetData( self, p );
}
static void LINE_Set( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsLine *self = (DaoxGraphicsLine*) p[0];
	LINE_SetData( self, p + 1 );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static DaoFuncItem DaoxGraphicsLineMeths[]=
{
	{ LINE_New,     "GraphicsLine( x1 = 0.0, y1 = 0.0, x2 = 1.0, y2 = 1.0 )" },
	{ LINE_Set,     "Set( self : GraphicsLine, x1 = 0.0, y1 = 0.0, x2 = 1.0, y2 = 1.0 ) => GraphicsLine" },
	{ NULL, NULL }
};

DaoTypeBase DaoxGraphicsLine_Typer =
{
	"GraphicsLine", NULL, NULL, (DaoFuncItem*) DaoxGraphicsLineMeths,
	{ & DaoxGraphicsItem_Typer, NULL }, { NULL },
	(FuncPtrDel)DaoxGraphicsItem_Delete, DaoxGraphicsItem_GetGCFields
};





static void RECT_SetData( DaoxGraphicsRect *self, DaoValue *p[] )
{
	float x1 = p[0]->xFloat.value;
	float y1 = p[1]->xFloat.value;
	float x2 = p[2]->xFloat.value;
	float y2 = p[3]->xFloat.value;
	DaoxGraphicsRect_Set( self, x1, y1, x2, y2 );
}
static void RECT_New( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsRect *self = DaoxGraphicsItem_New( DAOX_GS_RECT );
	DaoProcess_PutValue( proc, (DaoValue*) self );
	RECT_SetData( self, p );
}
static void RECT_Set( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsRect *self = (DaoxGraphicsRect*) p[0];
	RECT_SetData( self, p + 1 );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static DaoFuncItem DaoxGraphicsRectMeths[]=
{
	{ RECT_New,     "GraphicsRect( x1 = 0.0, y1 = 0.0, x2 = 1.0, y2 = 1.0 )" },
	{ RECT_Set,     "Set( self : GraphicsRect, x1 = 0.0, y1 = 0.0, x2 = 1.0, y2 = 1.0 ) => GraphicsRect" },
	{ NULL, NULL }
};

DaoTypeBase DaoxGraphicsRect_Typer =
{
	"GraphicsRect", NULL, NULL, (DaoFuncItem*) DaoxGraphicsRectMeths,
	{ & DaoxGraphicsItem_Typer, NULL }, { NULL },
	(FuncPtrDel)DaoxGraphicsItem_Delete, DaoxGraphicsItem_GetGCFields
};






static void CIRCLE_SetData( DaoxGraphicsCircle *self, DaoValue *p[] )
{
	float x = p[0]->xFloat.value;
	float y = p[1]->xFloat.value;
	float r = p[2]->xFloat.value;
	DaoxGraphicsCircle_Set( self, x, y, r );
}
static void CIRCLE_New( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsCircle *self = DaoxGraphicsItem_New( DAOX_GS_CIRCLE );
	DaoProcess_PutValue( proc, (DaoValue*) self );
	CIRCLE_SetData( self, p );
}
static void CIRCLE_Set( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsCircle *self = (DaoxGraphicsCircle*) p[0];
	CIRCLE_SetData( self, p + 1 );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static DaoFuncItem DaoxGraphicsCircleMeths[]=
{
	{ CIRCLE_New,     "GraphicsCircle( cx = 0.0, cy = 0.0, r = 1.0 )" },
	{ CIRCLE_Set,     "Set( self : GraphicsCircle, cx = 0.0, cy = 0.0, r = 1.0 ) => GraphicsCircle" },
	{ NULL, NULL }
};

DaoTypeBase DaoxGraphicsCircle_Typer =
{
	"GraphicsCircle", NULL, NULL, (DaoFuncItem*) DaoxGraphicsCircleMeths,
	{ & DaoxGraphicsItem_Typer, NULL }, { NULL },
	(FuncPtrDel)DaoxGraphicsItem_Delete, DaoxGraphicsItem_GetGCFields
};






static void ELLIPSE_SetData( DaoxGraphicsEllipse *self, DaoValue *p[] )
{
	float x1 = p[0]->xFloat.value;
	float y1 = p[1]->xFloat.value;
	float x2 = p[2]->xFloat.value;
	float y2 = p[3]->xFloat.value;
	DaoxGraphicsEllipse_Set( self, x1, y1, x2, y2 );
}
static void ELLIPSE_New( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsEllipse *self = DaoxGraphicsItem_New( DAOX_GS_ELLIPSE );
	DaoProcess_PutValue( proc, (DaoValue*) self );
	ELLIPSE_SetData( self, p );
}
static void ELLIPSE_Set( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsEllipse *self = (DaoxGraphicsEllipse*) p[0];
	ELLIPSE_SetData( self, p + 1 );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static DaoFuncItem DaoxGraphicsEllipseMeths[]=
{
	{ ELLIPSE_New,  "GraphicsEllipse( cx = 0.0, cy = 0.0, rx = 1.0, ry = 1.0 )" },
	{ ELLIPSE_Set,  "Set( self : GraphicsEllipse, cx = 0.0, cy = 0.0, rx = 1.0, ry = 1.0 ) => GraphicsEllipse" },
	{ NULL, NULL }
};

DaoTypeBase DaoxGraphicsEllipse_Typer =
{
	"GraphicsEllipse", NULL, NULL, (DaoFuncItem*) DaoxGraphicsEllipseMeths,
	{ & DaoxGraphicsItem_Typer, NULL }, { NULL },
	(FuncPtrDel)DaoxGraphicsItem_Delete, DaoxGraphicsItem_GetGCFields
};





static void POLYLINE_New( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsPolyLine *self = DaoxGraphicsItem_New( DAOX_GS_POLYLINE );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void POLYLINE_Add( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsPolyLine *self = (DaoxGraphicsPolyLine*) p[0];
	float x1 = p[1]->xFloat.value;
	float y1 = p[2]->xFloat.value;
	float x2 = p[3]->xFloat.value;
	float y2 = p[4]->xFloat.value;
	DaoxGraphicsPolyLine_Add( self, x1, y1, x2, y2 );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static DaoFuncItem DaoxGraphicsPolyLineMeths[]=
{
	{ POLYLINE_New,   "GraphicsPolyLine()" },
	{ POLYLINE_Add,   "Add( self : GraphicsPolyLine, x1 = 0.0, y1 = 0.0, x2 = 1.0, y2 = 1.0 ) => GraphicsPolyLine" },
	{ NULL, NULL }
};

DaoTypeBase DaoxGraphicsPolyLine_Typer =
{
	"GraphicsPolyLine", NULL, NULL, (DaoFuncItem*) DaoxGraphicsPolyLineMeths,
	{ & DaoxGraphicsItem_Typer, NULL }, { NULL },
	(FuncPtrDel)DaoxGraphicsItem_Delete, DaoxGraphicsItem_GetGCFields
};





static void POLYGON_New( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsPolygon *self = DaoxGraphicsItem_New( DAOX_GS_POLYGON );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void POLYGON_Add( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsPolygon *self = (DaoxGraphicsPolygon*) p[0];
	float x = p[1]->xFloat.value;
	float y = p[2]->xFloat.value;
	DaoxGraphicsPolygon_Add( self, x, y );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void POLYGON_SetJunction( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsPolygon *self = (DaoxGraphicsPolygon*) p[0];
	self->junction = p[1]->xEnum.value;
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static DaoFuncItem DaoxGraphicsPolygonMeths[]=
{
	{ POLYGON_New,   "GraphicsPolygon()" },
	{ POLYGON_Add,   "Add( self : GraphicsPolygon, x = 0.0, y = 0.0 ) => GraphicsPolygon" },
	{ POLYGON_SetJunction, 
		"SetJunction( self : GraphicsPolygon, junction: enum<none,sharp,flat,round> = $sharp ) => GraphicsPolygon" },
	{ NULL, NULL }
};

DaoTypeBase DaoxGraphicsPolygon_Typer =
{
	"GraphicsPolygon", NULL, NULL, (DaoFuncItem*) DaoxGraphicsPolygonMeths,
	{ & DaoxGraphicsItem_Typer, NULL }, { NULL },
	(FuncPtrDel)DaoxGraphicsItem_Delete, DaoxGraphicsItem_GetGCFields
};






static void PATH_New( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsPath *self = DaoxGraphicsItem_New( DAOX_GS_PATH );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void PATH_MoveTo( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsPath *self = (DaoxGraphicsPath*) p[0];
	float x = p[1]->xFloat.value;
	float y = p[2]->xFloat.value;
	DaoxGraphicsPath_MoveTo( self, x, y );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void PATH_LineTo( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsPath *self = (DaoxGraphicsPath*) p[0];
	float x = p[1]->xFloat.value;
	float y = p[2]->xFloat.value;
	DaoxGraphicsPath_LineTo( self, x, y );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void PATH_ArcTo( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsPath *self = (DaoxGraphicsPath*) p[0];
	float x = p[1]->xFloat.value;
	float y = p[2]->xFloat.value;
	float d = p[3]->xFloat.value;
	DaoxGraphicsPath_ArcTo( self, x, y, d, p[4]->xInteger.value );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void PATH_CubicTo( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsPath *self = (DaoxGraphicsPath*) p[0];
	float x = p[1]->xFloat.value;
	float y = p[2]->xFloat.value;
	float cx = p[3]->xFloat.value;
	float cy = p[4]->xFloat.value;
	DaoxGraphicsPath_CubicTo( self, x, y, cx, cy );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void PATH_CubicTo2( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsPath *self = (DaoxGraphicsPath*) p[0];
	float cx0 = p[1]->xFloat.value;
	float cy0 = p[2]->xFloat.value;
	float x = p[3]->xFloat.value;
	float y = p[4]->xFloat.value;
	float cx = p[5]->xFloat.value;
	float cy = p[6]->xFloat.value;
	DaoxGraphicsPath_CubicTo2( self, cx0, cy0, x, y, cx, cy );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void PATH_Close( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsPath *self = (DaoxGraphicsPath*) p[0];
	DaoxGraphicsPath_Close( self );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static DaoFuncItem DaoxGraphicsPathMeths[]=
{
	{ PATH_New,       "GraphicsPath()" },
	{ PATH_MoveTo,    "MoveTo( self : GraphicsPath, x : float, y : float ) => GraphicsPath" },
	{ PATH_LineTo,    "LineTo( self : GraphicsPath, x : float, y : float ) => GraphicsPath" },
	{ PATH_ArcTo,     "ArcTo( self : GraphicsPath, x : float, y : float, degrees : float, clockwise = 0 ) => GraphicsPath" },
	{ PATH_CubicTo,   "CubicTo( self : GraphicsPath, x : float, y : float, cx : float, cy : float ) => GraphicsPath" },
	{ PATH_CubicTo2,  "CubicTo( self : GraphicsPath, cx0 : float, cy0 : float, x : float, y : float, cx : float, cy : float ) => GraphicsPath" },
	{ PATH_Close,     "Close( self : GraphicsPath ) => GraphicsPath" },
	{ POLYGON_SetJunction, 
		"SetJunction( self : GraphicsPath, junction: enum<none,sharp,flat,round> = $sharp ) => GraphicsPolygon" },
	{ NULL, NULL }
};

DaoTypeBase DaoxGraphicsPath_Typer =
{
	"GraphicsPath", NULL, NULL, (DaoFuncItem*) DaoxGraphicsPathMeths,
	{ & DaoxGraphicsItem_Typer, NULL }, { NULL },
	(FuncPtrDel)DaoxGraphicsItem_Delete, DaoxGraphicsItem_GetGCFields
};





static void SCENE_New( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsScene *self = DaoxGraphicsScene_New();
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void SCENE_AddLine( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsScene *self = (DaoxGraphicsScene*) p[0];
	float x1 = p[1]->xFloat.value, y1 = p[2]->xFloat.value;
	float x2 = p[3]->xFloat.value, y2 = p[4]->xFloat.value;
	DaoxGraphicsLine *item = DaoxGraphicsScene_AddLine( self, x1, y1, x2, y2 );
	DaoProcess_PutValue( proc, (DaoValue*) item );
}
static void SCENE_AddRect( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsScene *self = (DaoxGraphicsScene*) p[0];
	float x1 = p[1]->xFloat.value, y1 = p[2]->xFloat.value;
	float x2 = p[3]->xFloat.value, y2 = p[4]->xFloat.value;
	DaoxGraphicsRect *item = DaoxGraphicsScene_AddRect( self, x1, y1, x2, y2 );
	DaoProcess_PutValue( proc, (DaoValue*) item );
}
static void SCENE_AddCircle( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsScene *self = (DaoxGraphicsScene*) p[0];
	float x = p[1]->xFloat.value, y = p[2]->xFloat.value;
	float r = p[3]->xFloat.value;
	DaoxGraphicsCircle *item = DaoxGraphicsScene_AddCircle( self, x, y, r );
	DaoProcess_PutValue( proc, (DaoValue*) item );
}
static void SCENE_AddEllipse( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsScene *self = (DaoxGraphicsScene*) p[0];
	float x = p[1]->xFloat.value, y = p[2]->xFloat.value;
	float rx = p[3]->xFloat.value, ry = p[4]->xFloat.value;
	DaoxGraphicsEllipse *item = DaoxGraphicsScene_AddEllipse( self, x, y, rx, ry );
	DaoProcess_PutValue( proc, (DaoValue*) item );
}
static void SCENE_AddPolyLine( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsScene *self = (DaoxGraphicsScene*) p[0];
	DaoxGraphicsPolyLine *item = DaoxGraphicsScene_AddPolyLine( self );
	DaoProcess_PutValue( proc, (DaoValue*) item );
}
static void SCENE_AddPolygon( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsScene *self = (DaoxGraphicsScene*) p[0];
	DaoxGraphicsPolygon *item = DaoxGraphicsScene_AddPolygon( self );
	DaoProcess_PutValue( proc, (DaoValue*) item );
}
static void SCENE_AddPath( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsScene *self = (DaoxGraphicsScene*) p[0];
	DaoxGraphicsPath *item = DaoxGraphicsScene_AddPath( self );
	DaoProcess_PutValue( proc, (DaoValue*) item );
}

static void DaoxGraphicsScene_GetGCFields( void *p, DArray *values, DArray *arrays, DArray *maps, int remove )
{
	daoint i, n;
	DaoxGraphicsScene *self = (DaoxGraphicsScene*) p;
	if( self->items == NULL ) return;
	DArray_Append( arrays, self->items );
}

static DaoFuncItem DaoxGraphicsSceneMeths[]=
{
	{ SCENE_New,       "GraphicsScene()" },
	{ SCENE_AddLine,   "AddLine( self: GraphicsScene, x1: float, y1: float, x2: float, y2: float ) => GraphicsLine" },
	{ SCENE_AddRect,   "AddRect( self: GraphicsScene, x1: float, y1: float, x2: float, y2: float ) => GraphicsRect" },
	{ SCENE_AddCircle,    "AddCircle( self: GraphicsScene, x: float, y: float, r: float ) => GraphicsCircle" },
	{ SCENE_AddEllipse,   "AddEllipse( self: GraphicsScene, x: float, y: float, rx: float, ry: float ) => GraphicsEllipse" },
	{ SCENE_AddPolyLine,  "AddPolyLine( self: GraphicsScene ) => GraphicsPolyLine" },
	{ SCENE_AddPolygon,   "AddPolygon( self: GraphicsScene ) => GraphicsPolygon" },
	{ SCENE_AddPath,      "AddPath( self: GraphicsScene ) => GraphicsPath" },
	{ NULL, NULL }
};

DaoTypeBase DaoxGraphicsScene_Typer =
{
	"GraphicsScene", NULL, NULL, (DaoFuncItem*) DaoxGraphicsSceneMeths, {0}, {0},
	(FuncPtrDel)DaoxGraphicsScene_Delete, DaoxGraphicsScene_GetGCFields
};






DAO_DLL int DaoOnLoad( DaoVmSpace *vmSpace, DaoNamespace *ns )
{
	daox_type_graphics_scene = DaoNamespace_WrapType( ns, & DaoxGraphicsScene_Typer, 0 );
	daox_type_graphics_item = DaoNamespace_WrapType( ns, & DaoxGraphicsItem_Typer, 0 );
	daox_type_graphics_line = DaoNamespace_WrapType( ns, & DaoxGraphicsLine_Typer, 0 );
	daox_type_graphics_rect = DaoNamespace_WrapType( ns, & DaoxGraphicsRect_Typer, 0 );
	daox_type_graphics_circle = DaoNamespace_WrapType( ns, & DaoxGraphicsCircle_Typer, 0 );
	daox_type_graphics_ellipse = DaoNamespace_WrapType( ns, & DaoxGraphicsEllipse_Typer, 0 );
	daox_type_graphics_polyline = DaoNamespace_WrapType( ns, & DaoxGraphicsPolyLine_Typer, 0 );
	daox_type_graphics_polygon = DaoNamespace_WrapType( ns, & DaoxGraphicsPolygon_Typer, 0 );
	daox_type_graphics_path = DaoNamespace_WrapType( ns, & DaoxGraphicsPath_Typer, 0 );
	return 0;
}
