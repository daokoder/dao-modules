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
#include "daoStdtype.h"
#include "dao_geometry.h"
#include "dao_triangulator.h"






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
	self->start = 0.0;
	self->end = 1.0;
}
void DaoxBezierSegment_DivideQuadratic( DaoxBezierSegment *self, double at )
{
	DaoxPoint Q1;

	if( self->first == NULL ) self->first = DaoxBezierSegment_New();
	if( self->second == NULL ) self->second = DaoxBezierSegment_New();
	self->first->count = 1;
	self->second->count = 1;
	self->count = 2;

	self->first->P0 = self->P0;
	self->first->P1.x = (1.0 - at)*self->P0.x + at*self->P1.x;
	self->first->P1.y = (1.0 - at)*self->P0.y + at*self->P1.y;

	self->second->P3 = self->P3;
	self->second->P1.x = (1.0 - at)*self->P1.x + at*self->P3.x;
	self->second->P1.y = (1.0 - at)*self->P1.y + at*self->P3.y;

	Q1.x = (1.0 - at)*self->first->P1.x + at*self->second->P1.x;
	Q1.y = (1.0 - at)*self->first->P1.y + at*self->second->P1.y;
	self->first->P3 = Q1;
	self->second->P0 = Q1;

	self->first->start = self->start;
	self->second->end = self->end;
	self->first->end = self->second->start = (1.0 - at)*self->start + at*self->end;
}
void DaoxBezierSegment_DivideCubic( DaoxBezierSegment *self, double at )
{
	DaoxPoint Q1;

	if( self->first == NULL ) self->first = DaoxBezierSegment_New();
	if( self->second == NULL ) self->second = DaoxBezierSegment_New();
	self->first->count = 1;
	self->second->count = 1;
	self->count = 2;

	Q1.x = (1.0 - at)*self->P1.x + at*self->P2.x;
	Q1.y = (1.0 - at)*self->P1.y + at*self->P2.y;

	self->first->P0 = self->P0;
	self->first->P1.x = (1.0 - at)*self->P0.x + at*self->P1.x;
	self->first->P1.y = (1.0 - at)*self->P0.y + at*self->P1.y;
	self->first->P2.x = (1.0 - at)*self->first->P1.x + at*Q1.x;
	self->first->P2.y = (1.0 - at)*self->first->P1.y + at*Q1.y;

	self->second->P3 = self->P3;
	self->second->P2.x = (1.0 - at)*self->P2.x + at*self->P3.x;
	self->second->P2.y = (1.0 - at)*self->P2.y + at*self->P3.y;
	self->second->P1.x = (1.0 - at)*Q1.x + at*self->second->P2.x;
	self->second->P1.y = (1.0 - at)*Q1.y + at*self->second->P2.y;

	Q1.x = (1.0 - at)*self->first->P2.x + at*self->second->P1.x;
	Q1.y = (1.0 - at)*self->first->P2.y + at*self->second->P1.y;
	self->first->P3 = Q1;
	self->second->P0 = Q1;

	self->first->start = self->start;
	self->second->end = self->end;
	self->first->end = self->second->start = (1.0 - at)*self->start + at*self->end;
}
void DaoxBezierSegment_RefineQuadratic( DaoxBezierSegment *self, double maxlen )
{
	double D1 = DaoxDistance( self->P0, self->P1 );
	double D2 = DaoxDistance( self->P1, self->P3 );
	double D3 = DaoxDistance( self->P0, self->P3 );

	if( D3 < maxlen && ((D1 + D2 - D3) < 0.001*D3 || D3 < 0.001) ){
		self->count = 1;
		if( self->first ) self->first->count = 0;
		if( self->second ) self->second->count = 0;
		return;
	}
	DaoxBezierSegment_DivideQuadratic( self, 0.5 );

	DaoxBezierSegment_RefineQuadratic( self->first, maxlen );
	DaoxBezierSegment_RefineQuadratic( self->second, maxlen );
	self->count = self->first->count + self->second->count;
}
void DaoxBezierSegment_RefineCubic( DaoxBezierSegment *self, double maxlen )
{
	double D01 = DaoxDistance( self->P0, self->P1 );
	double D12 = DaoxDistance( self->P1, self->P2 );
	double D23 = DaoxDistance( self->P2, self->P3 );
	double D03 = DaoxDistance( self->P0, self->P3 );

	if( D03 < maxlen && ((D01 + D12 + D23 - D03) < 0.005*D03 || D03 < 0.001) ){
		self->count = 1;
		if( self->first ) self->first->count = 0;
		if( self->second ) self->second->count = 0;
		return;
	}
	DaoxBezierSegment_DivideCubic( self, 0.5 );

	DaoxBezierSegment_RefineCubic( self->first, maxlen );
	DaoxBezierSegment_RefineCubic( self->second, maxlen );
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
DaoxBezierSegment* DaoxBezierSegment_GetSegment( DaoxBezierSegment *self, double parloc )
{
	if( self->count == 1 ){
		if( self->start <= parloc && parloc <= self->end ) return self;
	}else if( self->count ){
		if( self->first->end >= parloc ){
			return DaoxBezierSegment_GetSegment( self->first, parloc );
		}else if( self->second->start <= parloc ){
			return DaoxBezierSegment_GetSegment( self->second, parloc );
		}
	}
	return NULL;
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
void DaoxPointArray_PushXY( DaoxPointArray *self, double x, double y )
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
	DaoxPointArray_Delete( self->points );
	DaoxSliceArray_Delete( self->polygons );
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
void DaoxPolygonArray_PushPointXY( DaoxPolygonArray *self, double x, double y )
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
void DaoxPolygonArray_PushTriangle( DaoxPolygonArray *self, DaoxPoint A, DaoxPoint B, DaoxPoint C )
{
	DaoxPolygonArray_PushPolygon( self );
	DaoxPolygonArray_PushPoint( self, A );
	DaoxPolygonArray_PushPoint( self, B );
	DaoxPolygonArray_PushPoint( self, C );
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
void DaoxPath_MoveTo( DaoxPath *self, double x, double y )
{
	DaoxPointArray_PushXY( self->points, x, y );
	DaoxByteArray_Push( self->commands, DAOX_PATH_MOVE_TO );
}
void DaoxPath_LineTo( DaoxPath *self, double x, double y )
{
	DaoxPoint point;
	assert( self->points->count > 0 );
	point = self->points->points[ self->points->count - 1 ];
	DaoxPointArray_PushXY( self->points, point.x + x, point.y + y );
	DaoxByteArray_Push( self->commands, DAOX_PATH_LINE_TO );
}
void DaoxPath_ArcTo( DaoxPath *self, double x, double y, double degrees, int clockwise )
{
	DaoxPoint point, center;
	double t = tan( 0.5 * degrees * M_PI / 180.0 ) + 1E-12;
	double d, d2, dx, dy;
	assert( self->points->count > 0 );
	point = self->points->points[ self->points->count - 1 ];
	center.x = 0.5 * x;
	center.y = 0.5 * y;
	d = 0.25 * sqrt( x*x + y*y );
	d2 = d / (tan( 0.5 * degrees * M_PI / 180.0 ) + 1E-12);
	dx = - center.x;
	dy = - center.y;
	if( clockwise ){
		center.x += - dy / t;
		center.y += + dx / t;
	}else{
		center.x += + dy / t;
		center.y += - dx / t;
	}
	DaoxPointArray_PushXY( self->points, point.x + center.x, point.y + center.y );
	DaoxPointArray_PushXY( self->points, point.x + x, point.y + y );
	DaoxByteArray_Push( self->commands, DAOX_PATH_ARCR_TO + (clockwise != 0) );
}
void DaoxPath_QuadTo( DaoxPath *self, double x, double y, double cx, double cy )
{
	DaoxPoint current;
	assert( self->points->count > 0 );
	current = self->points->points[ self->points->count - 1 ];
	DaoxPointArray_PushXY( self->points, current.x + cx, current.y + cy );
	DaoxPointArray_PushXY( self->points, current.x + x, current.y + y );
	DaoxByteArray_Push( self->commands, DAOX_PATH_QUAD_TO );
}
void DaoxPath_CubicTo( DaoxPath *self, double x, double y, double cx, double cy )
{
	DaoxPoint control, start;
	assert( self->commands->count > 0 );
	assert( self->commands->bytes[ self->commands->count - 1 ] == DAOX_PATH_CUBIC_TO );
	control = self->points->points[ self->points->count - 2 ];
	start = self->points->points[ self->points->count - 1 ];
	control.x = 2.0 * start.x - control.x;
	control.y = 2.0 * start.y - control.y;
	DaoxPointArray_Push( self->points, control );
	DaoxPointArray_PushXY( self->points, start.x + x + cx, start.y + x + cy );
	DaoxPointArray_PushXY( self->points, start.x + x, start.y + y );
	DaoxByteArray_Push( self->commands, DAOX_PATH_CUBIC_TO );
}
void DaoxPath_CubicTo2( DaoxPath *self, double cx0, double cy0, double x, double y, double cx, double cy )
{
	DaoxPoint point;
	assert( self->points->count > 0 );
	point = self->points->points[ self->points->count - 1 ];
	DaoxPointArray_PushXY( self->points, point.x + cx0, point.y + cy0 );
	DaoxPointArray_PushXY( self->points, point.x + x + cx, point.y + y + cy );
	DaoxPointArray_PushXY( self->points, point.x + x, point.y + y );
	DaoxByteArray_Push( self->commands, DAOX_PATH_CUBIC_TO );
}
void DaoxPath_Close( DaoxPath *self )
{
	DaoxByteArray_Push( self->commands, DAOX_PATH_CLOSE );
}





DaoxQuad DaoxQuad_FromRect( double left, double bottom, double right, double top )
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
double DaoxDistance( DaoxPoint start, DaoxPoint end )
{
	double x1 = start.x, x2 = end.x;
	double y1 = start.y, y2 = end.y;
	return sqrt( (x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1) );
}
double DaoxDistance2( DaoxPoint start, DaoxPoint end )
{
	double x1 = start.x, x2 = end.x;
	double y1 = start.y, y2 = end.y;
	return (x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1);
}
DaoxQuad DaoxLine2Quad( DaoxPoint start, DaoxPoint end, double width )
{
	double x1 = start.x, x2 = end.x;
	double y1 = start.y, y2 = end.y;
	double dist = sqrt( (x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1) ) + 1E-12;
	double x = 0.5 * width * (x2 - x1) / dist;
	double y = 0.5 * width * (y2 - y1) / dist;
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
double DaoxTriangle_Area( DaoxPoint A, DaoxPoint B, DaoxPoint C )
{
	return 0.5 * ((A.x - C.x)*(B.y - A.y) - (A.x - B.x)*(C.y - A.y));
}
double DaoxTriangle_AreaBySideLength( double A, double B, double C )
{
	double M = 0.5 * (A + B + C);
	return sqrt( M * (M - A) * (M - B) * (M - C) );
}
double DaoxTriangle_PointCloseness( DaoxPoint A, DaoxPoint B, DaoxPoint C, DaoxPoint P )
{
	double AB = DaoxTriangle_Area( P, A, B );
	double BC = DaoxTriangle_Area( P, B, C );
	double CA = DaoxTriangle_Area( P, C, A );
	double min = AB < BC ? AB : BC;
	return (CA < min) ? CA : min;
}
double DaoxTriangle_AngleCosine( DaoxPoint C, DaoxPoint A, DaoxPoint B )
{
	double CA = DaoxDistance2( C, A );
	double CB = DaoxDistance2( C, B );
	double AB = DaoxDistance2( A, B );
	return (CA + CB - AB) / (2.0 * sqrt(CA + CB) );
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
DaoxLine DaoxQuadJunctionMajor( DaoxQuad *first, DaoxQuad *second, DaoxPoint c, double width )
{
	DaoxLine junction;
	double D1 = 2.0 * DaoxDistance2( first->C, second->A ) + 1;
	double D2 = 2.0 * DaoxDistance2( first->C, second->B ) + 1;
	double dx, dy, W = width * width + 1;
	junction.start = junction.end = c;

	dx = first->C.x - second->A.x;
	dy = first->C.y - second->A.y;
	junction.start.x += dx * W / D1;
	junction.start.y += dy * W / D1;
	junction.end.x -= dx * W / D1;
	junction.end.y -= dy * W / D1;
	return junction;
}
DaoxQuad DaoxLineJunctionMinor( DaoxPoint p1, DaoxPoint p2, DaoxPoint p3, double width )
{
	DaoxQuad first = DaoxLine2Quad( p1, p2, width );
	DaoxQuad second = DaoxLine2Quad( p2, p3, width );
	return DaoxQuadJunctionMinor( & first, & second );
}
DaoxLine DaoxLineJunctionMajor( DaoxPoint p1, DaoxPoint p2, DaoxPoint p3, double width )
{
	DaoxQuad first = DaoxLine2Quad( p1, p2, width );
	DaoxQuad second = DaoxLine2Quad( p2, p3, width );
	return DaoxQuadJunctionMajor( & first, & second, p2, width );
}


#define DELTA  0.1

static void DaoxPointArray_PushArcPoint( DaoxPointArray *self, double CX, double CY, double R, double A )
{
	DaoxPointArray_PushXY( self, CX + R * cos( A ), CY + R * sin( A ) );
}

int DaoxPointArray_SegmentArc( DaoxPointArray *self, DaoxPoint start, DaoxPoint end, DaoxPoint center, int clockwise )
{
	double CX = center.x;
	double CY = center.y;
	double R = DaoxDistance( start, center ) + 1E-9;
	double C = 2.0 * M_PI * R;
	double dA, A1, A2, A12 = 0.0;
	double AA = 0.0, ddA = DELTA / R;
	int i, k, M, K = C / 8;

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

	M = self->count;
	DaoxPointArray_Push( self, start );
	if( clockwise ){
		A12 = 2*M_PI + A1 - A2;
		if( A12 > 2*M_PI ) A12 -= 2*M_PI;
		K = 1 + (int)(A12 / dA);
		dA = A12 / K;

		/* Add minor line segments at the start: */
		for(i=1; i<=3; ++i){
			k = (i*(i+1)) / 2;
			if( dA > (2*k+i)*ddA ) DaoxPointArray_PushArcPoint( self, CX, CY, R, A1 - k*ddA );
		}

		for(i=1; i<K; ++i) DaoxPointArray_PushArcPoint( self, CX, CY, R, A1 - i*dA );
		A1 -= K * dA;

		/* Add minor line segments at the end: */
		for(i=3; i>=1; --i){
			k = (i*(i+1)) / 2;
			if( dA > (2*k+i)*ddA ) DaoxPointArray_PushArcPoint( self, CX, CY, R, A1 + k*ddA );
		}
	}else{
		A12 = 2*M_PI + A2 - A1;
		if( A12 > 2*M_PI ) A12 -= 2*M_PI;
		K = 1 + (int)(A12 / dA);
		dA = A12 / K;
		if( 15.0*ddA >= dA ) ddA = dA / 15.0;

		/* Add minor line segments at the start: */
		for(i=1; i<=3; ++i){
			k = (i*(i+1)) / 2;
			if( dA > (2*k+i)*ddA ) DaoxPointArray_PushArcPoint( self, CX, CY, R, A1 + k*ddA );
		}

		for(i=1; i<K; ++i) DaoxPointArray_PushArcPoint( self, CX, CY, R, A1 + i*dA );
		A1 += K * dA;

		/* Add minor line segments at the end: */
		for(i=3; i>=1; --i){
			k = (i*(i+1)) / 2;
			if( dA > (2*k+i)*ddA ) DaoxPointArray_PushArcPoint( self, CX, CY, R, A1 - k*ddA );
		}
	}
	DaoxPointArray_Push( self, end );
	return self->count - M;
}

void DaoxPolygonArray_MakeJunction( DaoxPolygonArray *self, DaoxQuad prev, DaoxPoint cur, DaoxQuad next, int junction, int strokeWidth )
{
	DaoxSliceArray *polygons;
	DaoxLine line;
	DaoxQuad quad;
	int count = self->points->count;
	switch( junction ){
	case DAOX_JUNCTION_NONE :
		break;
	case DAOX_JUNCTION_SHARP :
		line = DaoxQuadJunctionMajor( & prev, & next, cur, strokeWidth );
		quad.A = line.start;
		quad.B = line.end;
		quad.C = next.A;
		quad.D = prev.C;
		DaoxPolygonArray_PushQuad( self, quad );
		quad.A = line.start;
		quad.B = next.B;
		quad.C = line.end;
		quad.D = prev.D;
		DaoxPolygonArray_PushQuad( self, quad );
		break;
	case DAOX_JUNCTION_FLAT  :
		quad = DaoxQuadJunctionMinor( & prev, & next );
		DaoxPolygonArray_PushQuad( self, quad );
		break;
	case DAOX_JUNCTION_ROUND :
		DaoxPolygonArray_PushPolygon( self );
		count = DaoxPointArray_SegmentArc( self->points, prev.D, next.A, cur, 0 );
		polygons = self->polygons;
		polygons->slices[ polygons->count - 1 ].count = count;
		DaoxPolygonArray_PushPoint( self, cur );
		break;
	}
}

void DaoxPolygonArray_MakeLines( DaoxPolygonArray *self, DaoxPointArray *points,
		DaoxByteArray *junctions, double width, int junction, int close )
{
	DaoxQuad quad, second;
	int i, J, M = points->count - 1;
	assert( points->count >= 2 );
	quad = second = DaoxLine2Quad( points->points[0], points->points[1], width );
	DaoxPolygonArray_PushQuad( self, quad );
	for(i=1; i<M; ++i){
		DaoxPoint prev = points->points[i-1];
		DaoxPoint cur = points->points[i];
		DaoxPoint next = points->points[i+1];
		J = junctions ? junctions->bytes[i] : junction;
		second = DaoxLine2Quad( cur, next, width );
		DaoxPolygonArray_MakeJunction( self, quad, cur, second, J, width );
		DaoxPolygonArray_PushQuad( self, second );
		quad = second;
	}
	if( close == 0 ) return;
	J = junctions ? junctions->bytes[M] : junction;
	quad = DaoxLine2Quad( points->points[M], points->points[0], width );
	DaoxPolygonArray_MakeJunction( self, second, points->points[M], quad, J, width );
	second = DaoxLine2Quad( points->points[0], points->points[1], width );
	DaoxPolygonArray_PushQuad( self, quad );
	J = junctions ? junctions->bytes[0] : junction;
	DaoxPolygonArray_MakeJunction( self, quad, points->points[0], second, J, width );
}

void DaoxPolygonArray_TriangulatePolygon( DaoxPolygonArray *self, DaoxPointArray *points, DaoxTriangulator *triangulator )
{
	int i;
	DaoxTriangulator_Reset( triangulator );
	for(i=0; i<points->count; ++i){
		DaoxPoint point = points->points[i];
		DaoxTriangulator_PushPoint( triangulator, point.x, point.y );
	}
	DaoxTriangulator_CloseContour( triangulator );
	DaoxTriangulator_Triangulate( triangulator );
	for(i=0; i<triangulator->triangles->size; i+=3){
		daoint *ids = triangulator->triangles->items.pInt + i;
		DaoxPoint A = points->points[ids[0]];
		DaoxPoint B = points->points[ids[1]];
		DaoxPoint C = points->points[ids[2]];
		DaoxPolygonArray_PushTriangle( self, A, B, C );
	}
	printf( "triangles: %i\n", self->polygons->count );
}

static void DaoxPointArray_PushMiddlePoint( DaoxPointArray *self, DaoxPoint P0, DaoxPoint P1, double at )
{
	DaoxPoint PM;
	PM.x = (1.0 - at) * P0.x + at * P1.x;
	PM.y = (1.0 - at) * P0.y + at * P1.y;
	DaoxPointArray_Push( self, PM );
}

/*
// Minor line segments are added at the start and end of each path segment,
// to improve the triangulation of intersecting path segments!
*/
void DaoxPath_MakePolygons( DaoxPath *self, double width, int junction,
		DaoxPolygonArray *strokes, DaoxPolygonArray *fills, DaoxPathBuffer *buffer )
{
	DaoxTriangulator *triangulator = buffer->triangulator;
	DaoxPointArray *lines = buffer->points;
	DaoxByteArray *junctions = buffer->junctions;
	DaoxBezierSegment *bez1, *bez2, *bezier = buffer->bezier;
	DaoxPoint P0, P1, PM, *points = self->points->points;
	int bezjt = width < 100 ? DAOX_JUNCTION_FLAT : DAOX_JUNCTION_ROUND;
	int i, j, k, K = 0;
	double mlen, ratio;

	lines->count = 0;
	junctions->count = 0;
	DaoxTriangulator_Reset( triangulator );
	for(i=0; i<self->commands->count; ++i){
		uchar_t command = self->commands->bytes[i];
		//printf( "%3i %3i, %15f %15f\n", i, (int)command, points[K].x, points[K].y );
		if( command == DAOX_PATH_MOVE_TO ){
			if( lines->count ){
				DaoxPolygonArray_MakeLines( strokes, lines, junctions, width, 0, 0 );
				lines->count = 0;
			}
			DaoxPointArray_Push( lines, points[ K++ ] );
			DaoxByteArray_Push( junctions, junction );
		}else if( command == DAOX_PATH_LINE_TO ){
			P0 = points[K-1];
			P1 = points[K];
			mlen = DaoxDistance( P0, P1 );
			for(j=1; j<=3; ++j){
				k = (j*(j+1)) / 2;
				if( mlen > (2*k+j)*DELTA ){
					DaoxPointArray_PushMiddlePoint( lines, P0, P1, (k*DELTA) / mlen );
					DaoxByteArray_Push( junctions, DAOX_JUNCTION_NONE );
				}
			}
			for(j=3; j>=1; --j){
				k = (j*(j+1)) / 2;
				if( mlen > (2*k+j)*DELTA ){
					DaoxPointArray_PushMiddlePoint( lines, P0, P1, 1.0 - (k*DELTA) / mlen );
					DaoxByteArray_Push( junctions, DAOX_JUNCTION_NONE );
				}
			}
			DaoxPointArray_Push( lines, points[ K++ ] );
			DaoxByteArray_Push( junctions, junction );
		}else if( command == DAOX_PATH_ARCR_TO || command == DAOX_PATH_ARCL_TO ){
			int clockwise = command == DAOX_PATH_ARCL_TO;
			lines->count -= 1;
			DaoxPointArray_SegmentArc( lines, points[K-1], points[K+1], points[K], clockwise );
			DaoxByteArray_Resize( junctions, lines->count, bezjt );
			junctions->bytes[ junctions->count - 1 ] = junction;
			K += 2;
		}else if( command == DAOX_PATH_QUAD_TO ){
			DaoxBezierSegment_SetPoints( bezier, points[K-1], points[K], points[K+1], points[K+1] );
			DaoxBezierSegment_RefineQuadratic( bezier, 20 );
			bez1 = DaoxBezierSegment_GetSegment( bezier, 0.0 );
			bez2 = DaoxBezierSegment_GetSegment( bezier, 1.0 );
			for(j=0; j<5; ++j){
				bez1 = DaoxBezierSegment_GetSegment( bez1, 0.0 );
				bez2 = DaoxBezierSegment_GetSegment( bez2, 1.0 );
				if( DaoxDistance( bez1->P0, bez1->P3 ) > DELTA )
					DaoxBezierSegment_DivideQuadratic( bez1, 0.1 );
				if( DaoxDistance( bez2->P0, bez2->P3 ) > DELTA )
					DaoxBezierSegment_DivideQuadratic( bez2, 0.9 );
			}
			DaoxBezierSegment_ExportEndPoints( bezier, lines );
			DaoxByteArray_Resize( junctions, lines->count, bezjt );
			junctions->bytes[ junctions->count - 1 ] = junction;
			K += 2;
		}else if( command == DAOX_PATH_CUBIC_TO ){
			DaoxBezierSegment_SetPoints( bezier, points[K-1], points[K], points[K+1], points[K+2] );
			DaoxBezierSegment_RefineCubic( bezier, 20 );
			bez1 = DaoxBezierSegment_GetSegment( bezier, 0.0 );
			bez2 = DaoxBezierSegment_GetSegment( bezier, 1.0 );
			for(j=0; j<5; ++j){
				bez1 = DaoxBezierSegment_GetSegment( bez1, 0.0 );
				bez2 = DaoxBezierSegment_GetSegment( bez2, 1.0 );
				if( DaoxDistance( bez1->P0, bez1->P3 ) > DELTA )
					DaoxBezierSegment_DivideQuadratic( bez1, 0.1 );
				if( DaoxDistance( bez2->P0, bez2->P3 ) > DELTA )
					DaoxBezierSegment_DivideQuadratic( bez2, 0.9 );
			}
			DaoxBezierSegment_ExportEndPoints( bezier, lines );
			DaoxByteArray_Resize( junctions, lines->count, bezjt );
			junctions->bytes[ junctions->count - 1 ] = junction;
			K += 3;
		}else if( command == DAOX_PATH_CLOSE ){
			DaoxPolygonArray_MakeLines( strokes, lines, junctions, width, 0, 1 );
			if( fills != NULL && triangulator != NULL ){
				for(j=0; j<lines->count; ++j){
					DaoxPoint point = lines->points[j];
					DaoxTriangulator_PushPoint( triangulator, point.x, point.y );
				}
				DaoxTriangulator_CloseContour( triangulator );
			}
			lines->count = 0;
		}
	}
	printf( "stroke triangles: %i\n", strokes->polygons->count );
	if( lines->count ) DaoxPolygonArray_MakeLines( strokes, lines, junctions, width, 0, 0 );
	if( triangulator->points->count == 0 ) return;
	DaoxTriangulator_Triangulate( triangulator );
	for(i=0; i<triangulator->triangles->size; i+=3){
		daoint *ids = triangulator->triangles->items.pInt + i;
		DaoxPoint A = triangulator->points->points[ids[0]];
		DaoxPoint B = triangulator->points->points[ids[1]];
		DaoxPoint C = triangulator->points->points[ids[2]];
		DaoxPolygonArray_PushTriangle( fills, A, B, C );
	}
	printf( "filling triangles: %i\n", fills->polygons->count );
}
