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





void DaoxTransform_Set( DaoxTransform *self, float *mat, int n )
{
	if( n != 4 && n != 6 ) return;
	self->Axx = mat[0];
	self->Ayx = mat[1];
	self->Axy = mat[2];
	self->Ayy = mat[3];
	if( n == 6 ){
		self->Bx = mat[4];
		self->By = mat[5];
	}
}
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


DaoxPoint DaoxPoint_Transform( DaoxPoint self, DaoxTransform *transform )
{
	if( transform == NULL ) return self;
	return DaoxTransform_Transform( transform, self );
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
	return (CA + CB - AB) / (2.0 * sqrt(CA * CB) );
}
int DaoxTriangle_Contain( DaoxPoint A, DaoxPoint B, DaoxPoint C, DaoxPoint P )
{
	int AB = 2*(DaoxTriangle_Area( P, A, B ) >= 0.0) - 1;
	int BC = 2*(DaoxTriangle_Area( P, B, C ) >= 0.0) - 1;
	int CA = 2*(DaoxTriangle_Area( P, C, A ) >= 0.0) - 1;
	return (AB*BC > 0) && (BC*CA > 0) && (CA*AB > 0);
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


