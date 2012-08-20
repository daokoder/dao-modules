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





void DaoxTransform_RotateXAxisTo( DaoxTransform *self, float x, float y )
{
	float r = sqrt( x*x + y*y );
	self->Axx =   x / r;
	self->Axy = - y / r;
	self->Ayx =   y / r;
	self->Ayy =   x / r;
}
void DaoxTransform_RotateYAxisTo( DaoxTransform *self, float x, float y )
{
	float r = sqrt( x*x + y*y );
	self->Axx =   y / r;
	self->Axy =   x / r;
	self->Ayx = - x / r;
	self->Ayy =   y / r;
}
void DaoxTransform_SetScale( DaoxTransform *self, float x, float y )
{
	self->Axx *= x;
	self->Axy *= x;
	self->Ayx *= y;
	self->Ayy *= y;
}
void DaoxTransform_Multiply( DaoxTransform *self, DaoxTransform other )
{
	float Axx = self->Axx * other.Axx + self->Axy * other.Ayx;
	float Axy = self->Axx * other.Axy + self->Axy * other.Ayy;
	float Ayx = self->Ayx * other.Axx + self->Ayy * other.Ayx;
	float Ayy = self->Ayx * other.Axy + self->Ayy * other.Ayy;
	float Bx = self->Axx * other.Bx + self->Axy * other.By + self->Bx;
	float By = self->Ayx * other.Bx + self->Ayy * other.By + self->By;
	self->Axx = Axx;
	self->Axy = Axy;
	self->Ayx = Ayx;
	self->Ayy = Ayy;
	self->Bx = Bx;
	self->By = By;
}
DaoxPoint DaoxTransform_TransformXY( DaoxTransform *self, float x, float y )
{
	DaoxPoint pt;
	pt.x = self->Axx * x + self->Axy * y + self->Bx;
	pt.y = self->Ayx * x + self->Ayy * y + self->By;
	return pt;
}
DaoxPoint DaoxTransform_Transform( DaoxTransform *self, DaoxPoint point )
{
	return DaoxTransform_TransformXY( self, point.x, point.y );
}
DaoxTransform DaoxTransform_Inverse( DaoxTransform *self )
{
	DaoxTransform inverse;
	double det = self->Axx * self->Ayy - self->Axy * self->Ayx;
	inverse.Axx =   self->Ayy / det;
	inverse.Axy = - self->Axy / det;
	inverse.Ayx = - self->Ayx / det;
	inverse.Ayy =   self->Axx / det;
	inverse.Bx = - inverse.Axx * self->Bx - inverse.Axy * self->By;
	inverse.By = - inverse.Ayx * self->Bx - inverse.Ayy * self->By;
	return inverse;
}
void DaoxTransform_Print( DaoxTransform *self )
{
	printf( "DaoxTransform: %9.3f %9.3f %9.3f %9.3f %9.3f %9.3f\n",
			self->Axx, self->Axy, self->Ayx, self->Ayy, self->Bx, self->By );
}




void DaoxBounds_AddMargin( DaoxBounds *self, float margin )
{
	self->left -= margin;
	self->bottom -= margin;
	self->right += margin;
	self->top += margin;
}
void DaoxBounds_InitXY( DaoxBounds *self, float x, float y )
{
	self->left = self->right = x;
	self->bottom = self->top = y;
}
void DaoxBounds_Init( DaoxBounds *self, DaoxPoint point )
{
	self->left = self->right = point.x;
	self->bottom = self->top = point.y;
}
void DaoxBounds_UpdateXY( DaoxBounds *self, float x, float y )
{
	if( x < self->left ) self->left = x;
	if( x > self->right ) self->right = x;
	if( y < self->bottom ) self->bottom = y;
	if( y > self->top ) self->top = y;
}
void DaoxBounds_Update( DaoxBounds *self, DaoxPoint point )
{
	DaoxBounds_UpdateXY( self, point.x, point.y );
}
DaoxBounds DaoxBounds_Transform( DaoxBounds *self, DaoxTransform *transform )
{
	DaoxBounds box;
	DaoxPoint P1 = DaoxTransform_TransformXY( transform, self->left, self->bottom );
	DaoxPoint P2 = DaoxTransform_TransformXY( transform, self->left, self->top );
	DaoxPoint P3 = DaoxTransform_TransformXY( transform, self->right, self->top );
	DaoxPoint P4 = DaoxTransform_TransformXY( transform, self->right, self->bottom );
	DaoxBounds_Init( & box, P1 );
	DaoxBounds_Update( & box, P2 );
	DaoxBounds_Update( & box, P3 );
	DaoxBounds_Update( & box, P4 );
	return box;
}
DaoxBounds DaoxBounds_FromTriangle( DaoxPoint A, DaoxPoint B, DaoxPoint C )
{
	DaoxBounds box;
	DaoxBounds_Init( & box, A );
	DaoxBounds_Update( & box, B );
	DaoxBounds_Update( & box, C );
	return box;
}
int DaoxBounds_Contain( DaoxBounds *self, DaoxBounds other )
{
	if( other.left < self->left ) return 0;
	if( other.bottom < self->bottom ) return 0;
	if( other.right > self->right ) return 0;
	if( other.top > self->top ) return 0;
	return 1;
}
int DaoxBounds_Intersect( DaoxBounds *self, DaoxBounds other )
{
	if( other.left > self->right ) return 0;
	if( other.bottom > self->top ) return 0;
	if( other.right < self->left ) return 0;
	if( other.top < self->bottom ) return 0;
	return 1;
}
int DaoxBounds_CheckTriangle( DaoxBounds *self, DaoxPoint A, DaoxPoint B, DaoxPoint C )
{
	DaoxBounds bounds = DaoxBounds_FromTriangle( A, B, C );
	return DaoxBounds_Intersect( self, bounds );
}
int DaoxBounds_CheckQuad( DaoxBounds *self, DaoxQuad quad )
{
	DaoxBounds bounds;
	DaoxBounds_Init( & bounds, quad.A );
	DaoxBounds_Update( & bounds, quad.B );
	DaoxBounds_Update( & bounds, quad.C );
	DaoxBounds_Update( & bounds, quad.D );
	return DaoxBounds_Intersect( self, bounds );
}
void DaoxBounds_Print( DaoxBounds *self )
{
	printf( "DaoxBounds: %9.3f %9.3f %9.3f %9.3f\n",
			self->left, self->right, self->bottom, self->top );
}




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
void DaoxBezierSegment_DivideQuadratic( DaoxBezierSegment *self, float at )
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
void DaoxBezierSegment_DivideCubic( DaoxBezierSegment *self, float at )
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
void DaoxBezierSegment_RefineQuadratic( DaoxBezierSegment *self, float maxlen )
{
	float D1 = DaoxDistance( self->P0, self->P1 );
	float D2 = DaoxDistance( self->P1, self->P3 );
	float D3 = DaoxDistance( self->P0, self->P3 );

	if( D3 < maxlen && ((D1 + D2 - D3) < 0.01*D3 || D3 < 0.001) ){
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
void DaoxBezierSegment_RefineCubic( DaoxBezierSegment *self, float maxlen )
{
	float D01 = DaoxDistance( self->P0, self->P1 );
	float D12 = DaoxDistance( self->P1, self->P2 );
	float D23 = DaoxDistance( self->P2, self->P3 );
	float D03 = DaoxDistance( self->P0, self->P3 );

	if( D03 < maxlen && ((D01 + D12 + D23 - D03) < 0.01*D03 || D03 < 0.001) ){
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
DaoxBezierSegment* DaoxBezierSegment_GetSegment( DaoxBezierSegment *self, float parloc )
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






DaoxFloatArray* DaoxFloatArray_New()
{
	DaoxFloatArray *self = (DaoxFloatArray*) dao_calloc(1,sizeof(DaoxFloatArray));
	return self;
}
void DaoxFloatArray_Delete( DaoxFloatArray *self )
{
	if( self->values ) dao_free( self->values );
	dao_free( self );
}
void DaoxFloatArray_Reset( DaoxFloatArray *self )
{
	self->count = 0;
}
void DaoxPointArray_Resize( DaoxPointArray *self, int count )
{
	if( self->capacity != count ){
		self->capacity = count;
		self->points = (DaoxPoint*) dao_realloc( self->points, self->capacity*sizeof(DaoxPoint) );
	}
	self->count = count;
}
void DaoxFloatArray_Push( DaoxFloatArray *self, float value )
{
	if( self->count >= self->capacity ){
		self->capacity += 0.2 * self->capacity + 1;
		self->values = (float*) dao_realloc( self->values, self->capacity * sizeof(float) );
	}
	self->values[ self->count ] = value;
	self->count += 1;
}
void DaoxFloatArray_QuickSort( float *values, int first, int last )
{
	float pivot, tmp;
	int lower = first+1, upper = last;

	if( first >= last ) return;
	tmp = values[first];
	values[first] = values[ (first+last)/2 ];
	values[ (first+last)/2 ] = tmp;
	pivot = values[ first ];

	while( lower <= upper ){
		while( lower < last && values[lower] < pivot ) lower ++;
		while( upper > first && pivot < values[upper] ) upper --;
		if( lower < upper ){
			tmp = values[lower];
			values[lower] = values[upper];
			values[upper] = tmp;
			upper --;
		}
		lower ++;
	}
	tmp = values[first];
	values[first] = values[upper];
	values[upper] = tmp;
	if( first+1 < upper ) DaoxFloatArray_QuickSort( values, first, upper-1 );
	if( upper+1 < last ) DaoxFloatArray_QuickSort( values, upper+1, last );
}
void DaoxFloatArray_Sort( DaoxFloatArray *self )
{
	if( self->count <= 1 ) return;
	DaoxFloatArray_QuickSort( self->values, 0, self->count - 1 );
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
void DaoxPointArray_Reset( DaoxPointArray *self )
{
	self->count = 0;
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
void DaoxPointArray_PushPoints( DaoxPointArray *self, DaoxPointArray *points )
{
	if( points->count == 0 ) return;
	if( (self->count + points->count) >= self->capacity ){
		self->capacity += 0.2 * self->capacity + points->count;
		self->points = (DaoxPoint*) dao_realloc( self->points, self->capacity * sizeof(DaoxPoint) );
	}
	memcpy( self->points, points->points, points->count * sizeof(DaoxPoint) );
	self->count += points->count;
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
void DaoxPolygonArray_PushPointXY( DaoxPolygonArray *self, float x, float y )
{
	DaoxPointArray_PushXY( self->points, x, y );
	self->polygons->slices[ self->polygons->count - 1 ].count += 1;
}
void DaoxPolygonArray_PushPoint( DaoxPolygonArray *self, DaoxPoint point )
{
	DaoxPolygonArray_PushPointXY( self, point.x, point.y );
}
void DaoxPolygonArray_PushPoints( DaoxPolygonArray *self, DaoxPointArray *points )
{
	DaoxPointArray_PushPoints( self->points, points );
	self->polygons->slices[ self->polygons->count - 1 ].count += points->count;
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





DaoxSimplePath* DaoxSimplePath_New()
{
	DaoxSimplePath *self = (DaoxSimplePath*) dao_malloc( sizeof(DaoxSimplePath) );
	self->points = DaoxPointArray_New();
	self->commands = DaoxByteArray_New();
	return self;
}
void DaoxSimplePath_Delete( DaoxSimplePath *self )
{
	DaoxPointArray_Delete( self->points );
	DaoxByteArray_Delete( self->commands );
	dao_free( self );
}
void DaoxSimplePath_Reset( DaoxSimplePath *self )
{
	self->points->count = 0;
	self->commands->count = 0;
}
void DaoxSimplePath_MoveTo( DaoxSimplePath *self, float x, float y )
{
	DaoxPointArray_PushXY( self->points, x, y );
	DaoxByteArray_Push( self->commands, DAOX_PATH_MOVE_TO );
}
void DaoxSimplePath_LineTo( DaoxSimplePath *self, float x, float y )
{
	DaoxPoint point;
	assert( self->points->count > 0 );
	point = self->points->points[ self->points->count - 1 ];
	DaoxPointArray_PushXY( self->points, point.x + x, point.y + y );
	DaoxByteArray_Push( self->commands, DAOX_PATH_LINE_TO );
}
void DaoxSimplePath_ArcTo( DaoxSimplePath *self, float x, float y, float degrees, int clockwise )
{
	DaoxPoint point, center;
	float t = tan( 0.5 * degrees * M_PI / 180.0 ) + 1E-12;
	float d, d2, dx, dy;
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
void DaoxSimplePath_QuadTo( DaoxSimplePath *self, float cx, float cy, float x, float y )
{
	DaoxPoint current;
	assert( self->points->count > 0 );
	current = self->points->points[ self->points->count - 1 ];
	DaoxPointArray_PushXY( self->points, current.x + cx, current.y + cy );
	DaoxPointArray_PushXY( self->points, current.x + x, current.y + y );
	DaoxByteArray_Push( self->commands, DAOX_PATH_QUAD_TO );
}
void DaoxSimplePath_CubicTo( DaoxSimplePath *self, float cx, float cy, float x, float y )
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
void DaoxSimplePath_CubicTo2( DaoxSimplePath *self, float cx1, float cy1, float cx2, float cy2, float x2, float y2 )
{
	DaoxPoint point;
	assert( self->points->count > 0 );
	point = self->points->points[ self->points->count - 1 ];
	DaoxPointArray_PushXY( self->points, point.x + cx1, point.y + cy1 );
	DaoxPointArray_PushXY( self->points, point.x + x2 + cx2, point.y + y2 + cy2 );
	DaoxPointArray_PushXY( self->points, point.x + x2, point.y + y2 );
	DaoxByteArray_Push( self->commands, DAOX_PATH_CUBIC_TO );
}
void DaoxSimplePath_Close( DaoxSimplePath *self )
{
	DaoxByteArray_Push( self->commands, DAOX_PATH_CLOSE );
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
/*
//        B------------------------C
//        |                        |
// line:  start------------------end
//        |                        |
//        A------------------------D
*/
DaoxQuad DaoxLine2Quad( DaoxPoint start, DaoxPoint end, float width )
{
	float x1 = start.x, x2 = end.x;
	float y1 = start.y, y2 = end.y;
	float dist = sqrt( (x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1) ) + 1E-12;
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
float DaoxTriangle_Area( DaoxPoint A, DaoxPoint B, DaoxPoint C )
{
	return 0.5 * ((A.x - C.x)*(B.y - A.y) - (A.x - B.x)*(C.y - A.y));
}
float DaoxTriangle_AreaBySideLength( float A, float B, float C )
{
	float M = 0.5 * (A + B + C);
	return sqrt( M * (M - A) * (M - B) * (M - C) );
}
float DaoxTriangle_PointCloseness( DaoxPoint A, DaoxPoint B, DaoxPoint C, DaoxPoint P )
{
	float AB = DaoxTriangle_Area( P, A, B );
	float BC = DaoxTriangle_Area( P, B, C );
	float CA = DaoxTriangle_Area( P, C, A );
	float min = AB < BC ? AB : BC;
	return (CA < min) ? CA : min;
}
float DaoxTriangle_AngleCosine( DaoxPoint C, DaoxPoint A, DaoxPoint B )
{
	float CA = DaoxDistance2( C, A );
	float CB = DaoxDistance2( C, B );
	float AB = DaoxDistance2( A, B );
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

int DaoxLine_Intersect( DaoxPoint A, DaoxPoint B, DaoxPoint C, DaoxPoint D, float *S, float *T )
{
	float BxAx = B.x - A.x;
	float ByAy = B.y - A.y;
	float CxAx = C.x - A.x;
	float CyAy = C.y - A.y;
	float DxCx = D.x - C.x;
	float DyCy = D.y - C.y;
	float K = BxAx * DyCy - ByAy * DxCx;

	if( K == 0.0 ) return 0;

	*S = (CxAx * DyCy - CyAy * DxCx) / K;
	*T = (CxAx * ByAy - CyAy * BxAx) / K;

	if( *S < 0 || *S > 1.0 ) return 0;
	if( *T < 0 || *T > 1.0 ) return 0;

	return 1;
}
/*
// Return  1, if the junction should connect first.D and second.A;
// Return -1, if the junction should connect first.C and second.B;
// Return 0, otherwise;
// Output parameter: tip, the tip point for a sharp junction.
//
// TODO: in case of parallel.
*/
int DaoxLineQuad_Junction( DaoxQuad first, DaoxQuad second, DaoxPoint *tip )
{
	float DAS1 = 0.0, DAS2 = 0.0, BCS1 = 0.0, BCS2 = 0.0;
	int DA = DaoxLine_Intersect( first.A, first.D, second.A, second.D, & DAS1, & DAS2 );
	int BC = DaoxLine_Intersect( first.B, first.C, second.B, second.C, & BCS1, & BCS2 );
	if( tip ){
		if( DAS1 > 1.0 ){
			tip->x = (1.0 - DAS1) * first.A.x + DAS1 * first.D.x;
			tip->y = (1.0 - DAS1) * first.A.y + DAS1 * first.D.y;
		}else{
			tip->x = (1.0 - BCS1) * first.B.x + BCS1 * first.C.x;
			tip->y = (1.0 - BCS1) * first.B.y + BCS1 * first.C.y;
		}
	}
	return (DAS1 > 1.0) ? 1 : -1;
}









DaoxPathBuffer* DaoxPathBuffer_New()
{
	DaoxPathBuffer *self = (DaoxPathBuffer*) dao_calloc(1,sizeof(DaoxPathBuffer));
	self->points = DaoxPointArray_New();
	self->junctions = DaoxByteArray_New();
	self->bezier = DaoxBezierSegment_New();
	self->triangulator = DaoxTriangulator_New();
	return self;
}
void DaoxPathBuffer_Delete( DaoxPathBuffer *self )
{
	DaoxPointArray_Delete( self->points );
	DaoxByteArray_Delete( self->junctions );
	DaoxBezierSegment_Delete( self->bezier );
	DaoxTriangulator_Delete( self->triangulator );
	dao_free( self );
}
