/*
// Dao Graphics Engine
// http://www.daovm.net
//
// Copyright (c) 2012-2017, Limin Fu
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


#include "stdlib.h"
#include "math.h"
#include "assert.h"
#include "dao_path.h"


typedef struct DaoxMatrix2D  DaoxMatrix2D;

struct DaoxMatrix2D
{
	float  A;
	float  B;
	float  C;
	float  D;
};



float daox_graphics_device_width = 300.0;
float daox_graphics_device_height = 200.0;

const DaoxColor daox_black_color = { 0.0, 0.0, 0.0, 1.0 };
const DaoxColor daox_white_color = { 1.0, 1.0, 1.0, 1.0 };
const DaoxColor daox_red_color = { 1.0, 0.0, 0.0, 1.0 };
const DaoxColor daox_green_color = { 0.0, 1.0, 0.0, 1.0 };
const DaoxColor daox_blue_color = { 0.0, 0.0, 1.0, 1.0 };
const DaoxColor daox_gray_color = { 0.5, 0.5, 0.5, 1.0 };


DaoxColor DaoxColor_Darker( const DaoxColor *self, float factor )
{
	DaoxColor color = *self;
	if( factor < 0.0 ) factor = 0.0;
	if( factor > 1.0 ) factor = 1.0;
	color.red = (1.0 - factor) * color.red;
	color.green = (1.0 - factor) * color.green;
	color.blue = (1.0 - factor) * color.blue;
	return color;
}
DaoxColor DaoxColor_Lighter( const DaoxColor *self, float factor )
{
	DaoxColor color = *self;
	if( factor < 0.0 ) factor = 0.0;
	if( factor > 1.0 ) factor = 1.0;
	color.red = (1.0 - factor) * color.red + factor;
	color.green = (1.0 - factor) * color.green + factor;
	color.blue = (1.0 - factor) * color.blue + factor;
	return color;
}



float DaoxMatrix2D_Trace( DaoxMatrix2D *self )
{
	return self->A + self->D;
}
float DaoxMatrix2D_Determinant( DaoxMatrix2D *self )
{
	return self->A * self->D - self->B * self->C;
}
DaoxVector2D DaoxMatrix2D_EigenValues( DaoxMatrix2D *self )
{
	DaoxVector2D egenvalues;
	float trace = DaoxMatrix2D_Trace( self );
	float det = DaoxMatrix2D_Determinant( self );
	float S = sqrt( trace * trace - 4.0 * det );
	egenvalues.x = fabs( 0.5 * (trace + S) );
	egenvalues.y = fabs( 0.5 * (trace - S) );
	if( egenvalues.x < egenvalues.y ){
		S = egenvalues.x;
		egenvalues.x = egenvalues.y;
		egenvalues.y = S;
	}
	return egenvalues;
}
DaoxMatrix2D DaoxMatrix2D_EigenVectors( DaoxMatrix2D *self, DaoxVector2D eigenvalues )
{
	DaoxMatrix2D vectors;
	float N1, N2;
	vectors.A = eigenvalues.x + self->B - self->D;
	vectors.B = eigenvalues.x + self->C - self->A;
	vectors.C = eigenvalues.y + self->B - self->D;
	vectors.D = eigenvalues.y + self->C - self->A;
	N1 = sqrt( vectors.A * vectors.A + vectors.B * vectors.B );
	N2 = sqrt( vectors.C * vectors.C + vectors.D * vectors.D );
	vectors.A /= N1;
	vectors.B /= N1;
	vectors.C /= N2;
	vectors.D /= N2;
	return vectors;
}







DaoxVector2D DaoxVector2D_XY( float x, float y )
{
	DaoxVector2D point;
	point.x = x;
	point.y = y;
	return point;
}
DaoxVector2D DaoxVector2D_Vector3D( DaoxVector3D x )
{
	DaoxVector2D point;
	point.x = x.x;
	point.y = x.y;
	return point;
}
DaoxVector2D DaoxVector2D_Interpolate( DaoxVector2D A, DaoxVector2D B, float t )
{
	DaoxVector2D point;
	point.x = (1.0 - t) * A.x + t * B.x;
	point.y = (1.0 - t) * A.y + t * B.y;
	return point;
}
DaoxVector2D  DaoxVector2D_Add( DaoxVector2D *self, DaoxVector2D *other )
{
	DaoxVector2D res;
	res.x = self->x + other->x;
	res.y = self->y + other->y;
	return res;
}
DaoxVector2D  DaoxVector2D_Sub( DaoxVector2D *self, DaoxVector2D *other )
{
	DaoxVector2D res;
	res.x = self->x - other->x;
	res.y = self->y - other->y;
	return res;
}
DaoxVector2D  DaoxVector2D_Scale( DaoxVector2D *self, double scale )
{
	DaoxVector2D res;
	res.x = self->x * scale;
	res.y = self->y * scale;
	return res;
}
DaoxVector2D  DaoxVector2D_Normalize( DaoxVector2D *self )
{
	double norm2 = sqrt( DaoxVector2D_Norm2( self ) );
	return DaoxVector2D_Scale( self, 1.0/(norm2+EPSILON) );
}
#if 0
double DaoxVector2D_Dist2( DaoxVector2D *self, DaoxVector2D *other )
{
	double res = (self->x - other->x) * (self->x - other->x);
	res += (self->y - other->y) * (self->y - other->y);
	return res;
}
#endif
double DaoxVector2D_Dot( DaoxVector2D *self, DaoxVector2D *other )
{
	return self->x * other->x + self->y * other->y;
}
double DaoxVector2D_Norm2( DaoxVector2D *self )
{
	return self->x * self->x + self->y * self->y;
}
double DaoxVector2D_Dist( DaoxVector2D start, DaoxVector2D end )
{
	double x1 = start.x, x2 = end.x;
	double y1 = start.y, y2 = end.y;
	return sqrt( (x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1) );
}
double DaoxVector2D_Dist2( DaoxVector2D start, DaoxVector2D end )
{
	double x1 = start.x, x2 = end.x;
	double y1 = start.y, y2 = end.y;
	return (x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1);
}
double DaoxTriangle_Area( DaoxVector2D A, DaoxVector2D B, DaoxVector2D C )
{
	return 0.5 * ((A.x - C.x)*(B.y - A.y) - (A.x - B.x)*(C.y - A.y));
}
double DaoxTriangle_AreaBySideLength( double A, double B, double C )
{
	double M = 0.5 * (A + B + C);
	return sqrt( M * (M - A) * (M - B) * (M - C) );
}
double DaoxTriangle_AngleCosine( DaoxVector2D C, DaoxVector2D A, DaoxVector2D B )
{
	double CA = DaoxVector2D_Dist2( C, A );
	double CB = DaoxVector2D_Dist2( C, B );
	double AB = DaoxVector2D_Dist2( A, B );
	return (CA + CB - AB) / (2.0 * sqrt(CA * CB) );
}
int DaoxTriangle_Contain( DaoxVector2D A, DaoxVector2D B, DaoxVector2D C, DaoxVector2D P )
{
	int AB = 2*(DaoxTriangle_Area( P, A, B ) >= 0.0) - 1;
	int BC = 2*(DaoxTriangle_Area( P, B, C ) >= 0.0) - 1;
	int CA = 2*(DaoxTriangle_Area( P, C, A ) >= 0.0) - 1;
	return (AB*BC > 0) && (BC*CA > 0) && (CA*AB > 0);
}

void DaoxVector2D_Print( DaoxVector2D *self )
{
	printf( "%15.9f %15.9f\n", self->x, self->y );
}



DaoxVector3D  DaoxVector3D_XYZ( float x, float y, float z )
{
	DaoxVector3D res;
	res.x = x;
	res.y = y;
	res.z = z;
	return res;
}
double DaoxVector3D_Norm2( DaoxVector3D *self )
{
	double norm2 = self->x * self->x;
	norm2 += self->y * self->y;
	norm2 += self->z * self->z;
	return norm2;
}
DaoxVector3D DaoxVector3D_Mean( DaoxVector3D A, DaoxVector3D B )
{
	DaoxVector3D point;
	point.x = 0.5 * (A.x + B.x);
	point.y = 0.5 * (A.y + B.y);
	point.z = 0.5 * (A.z + B.z);
	return point;
}
DaoxVector3D DaoxVector3D_Interpolate( DaoxVector3D A, DaoxVector3D B, float t )
{
	DaoxVector3D point;
	point.x = (1.0 - t) * A.x + t * B.x;
	point.y = (1.0 - t) * A.y + t * B.y;
	point.z = (1.0 - t) * A.z + t * B.z;
	return point;
}
DaoxVector3D  DaoxVector3D_Add( DaoxVector3D *self, DaoxVector3D *other )
{
	DaoxVector3D res;
	res.x = self->x + other->x;
	res.y = self->y + other->y;
	res.z = self->z + other->z;
	return res;
}
DaoxVector3D  DaoxVector3D_Sub( DaoxVector3D *self, DaoxVector3D *other )
{
	DaoxVector3D res;
	res.x = self->x - other->x;
	res.y = self->y - other->y;
	res.z = self->z - other->z;
	return res;
}
DaoxVector3D  DaoxVector3D_Mul( DaoxVector3D *self, DaoxVector3D *other )
{
	DaoxVector3D res;
	res.x = self->x * other->x;
	res.y = self->y * other->y;
	res.z = self->z * other->z;
	return res;
}
DaoxVector3D  DaoxVector3D_Cross( DaoxVector3D *self, DaoxVector3D *other )
{
	DaoxVector3D res;
	res.x = self->y * other->z - self->z * other->y;
	res.y = self->z * other->x - self->x * other->z;
	res.z = self->x * other->y - self->y * other->x;
	return res;
}
DaoxVector3D  DaoxVector3D_Scale( DaoxVector3D *self, double scale )
{
	DaoxVector3D res;
	res.x = self->x * scale;
	res.y = self->y * scale;
	res.z = self->z * scale;
	return res;
}
DaoxVector3D  DaoxVector3D_Normalize( DaoxVector3D *self )
{
	double scale = 1.0f / (sqrt( DaoxVector3D_Norm2( self ) ) + EPSILON);
	return DaoxVector3D_Scale( self, scale );
}
double DaoxVector3D_Dot( DaoxVector3D *self, DaoxVector3D *other )
{
	double res = self->x * other->x;
	res += self->y * other->y;
	res += self->z * other->z;
	return res;
}
double DaoxVector3D_Angle( DaoxVector3D *self, DaoxVector3D *other )
{
	double norm, res = self->x * other->x;
	res += self->y * other->y;
	res += self->z * other->z;
	norm = sqrt( DaoxVector3D_Norm2( self ) * DaoxVector3D_Norm2( other ) );
	res /= (norm + EPSILON);
	if( res > (1-EPSILON) ) return 0.0;
	if( res < (-1+EPSILON) ) return M_PI;
	return acos( res );
}
double DaoxVector3D_DotSqrt( DaoxVector3D *self, DaoxVector3D *other )
{
	double dot = DaoxVector3D_Dot( self, other );
	double dot2 = sqrt( fabs(dot) );
	return dot > 0 ? dot2 : -dot2;
}
double DaoxVector3D_Dist2( DaoxVector3D *self, DaoxVector3D *other )
{
	double res = (self->x - other->x) * (self->x - other->x);
	res += (self->y - other->y) * (self->y - other->y);
	res += (self->z - other->z) * (self->z - other->z);
	return res;
}
double DaoxVector3D_Dist( DaoxVector3D *self, DaoxVector3D *other )
{
	return sqrt( DaoxVector3D_Dist2( self, other ) );
}
double DaoxVector3D_Difference( DaoxVector3D *self, DaoxVector3D *other )
{
	double diff, max = 0.0;
	if( (diff = fabs( self->x - other->x )) > max ) max = diff;
	if( (diff = fabs( self->y - other->y )) > max ) max = diff;
	if( (diff = fabs( self->z - other->z )) > max ) max = diff;
	return max;
}
void DaoxVector3D_Print( DaoxVector3D *self )
{
	printf( "%9.3f", self->x );
	printf( "    %9.3f", self->y );
	printf( "    %9.3f\n", self->z );
}
DaoxVector3D DaoxTriangle_Normal( DaoxVector3D *A, DaoxVector3D *B, DaoxVector3D *C )
{
	DaoxVector3D AB = DaoxVector3D_Sub( B, A );
	DaoxVector3D BC = DaoxVector3D_Sub( C, B );
	DaoxVector3D CA = DaoxVector3D_Sub( A, C );
	DaoxVector3D N1 = DaoxVector3D_Cross( & AB, & BC );
	DaoxVector3D N2 = DaoxVector3D_Cross( & BC, & CA );
	DaoxVector3D N3 = DaoxVector3D_Cross( & CA, & AB );
	DaoxVector3D N = DaoxVector3D_Add( & N1, & N2 );
	N = DaoxVector3D_Add( & N, & N3 );
	return DaoxVector3D_Normalize( & N );
}
DaoxVector3D DaoxPlaneLineIntersect( DaoxVector3D O, DaoxVector3D N, DaoxVector3D P, DaoxVector3D Q )
{
	DaoxVector3D OP = DaoxVector3D_Sub( & O, & P );
	DaoxVector3D QP = DaoxVector3D_Sub( & Q, & P );
	double OPN = DaoxVector3D_Dot( & OP, & N );
	double QPN = DaoxVector3D_Dot( & QP, & N );
	return DaoxVector3D_Interpolate( P, Q, OPN / (QPN + (QPN > 0.0 ? EPSILON : -EPSILON)) );
}
DaoxVector3D DaoxPlaneLineIntersect2( DaoxVector3D O, DaoxVector3D N, DaoxVector3D P, DaoxVector3D D )
{
	DaoxVector3D res;
	DaoxVector3D OP = DaoxVector3D_Sub( & O, & P );
	double OPN = DaoxVector3D_Dot( & OP, & N );
	double DN = DaoxVector3D_Dot( & D, & N );
	double factor = OPN / (DN + (DN > 0.0 ? EPSILON : -EPSILON));
	res.x = P.x + factor * D.x;
	res.y = P.y + factor * D.y;
	res.z = P.z + factor * D.z;
	return res;
}
DaoxVector3D DaoxVector3D_ProjectToPlane( DaoxVector3D *self, DaoxVector3D *planeNorm )
{
	DaoxVector3D projection = { 0.0, 0.0, 0.0 };
	DaoxVector3D norm = DaoxVector3D_Normalize( planeNorm );
	double dot = DaoxVector3D_Dot( self, & norm );
	if( fabs(dot - 1.0) < 1E-16 ) return projection;
	projection = DaoxVector3D_Scale( & norm, dot );
	return DaoxVector3D_Sub( self, & projection );
}






DaoxMatrix3D  DaoxMatrix3D_Identity()
{
	DaoxMatrix3D res;
	memset( & res, 0, sizeof(DaoxMatrix3D) );
	res.A11 = res.A22 = 1.0;
	return res;
}
DaoxMatrix3D  DaoxMatrix3D_PointRotation( DaoxVector2D point, float alpha )
{
	DaoxMatrix3D identity = DaoxMatrix3D_Identity();
	DaoxMatrix3D addpoint = identity;
	DaoxMatrix3D subpoint = identity;
	DaoxMatrix3D rotate = identity;
	DaoxMatrix3D res;
	addpoint.B1 = point.x;
	addpoint.B2 = point.y;
	addpoint.B1 = - point.x;
	addpoint.B2 = - point.y;
	rotate.A11 = rotate.A22 = cos( alpha );
	rotate.A21 = sin( alpha );
	rotate.A12 = - rotate.A21;
	res = DaoxMatrix3D_MulMatrix( & addpoint, & rotate );
	res = DaoxMatrix3D_MulMatrix( & res, & subpoint );
	return res;
}
DaoxMatrix3D  DaoxMatrix3D_MulMatrix( DaoxMatrix3D *self, DaoxMatrix3D *other )
{
	DaoxMatrix3D res;
	res.A11 = self->A11 * other->A11 + self->A12 * other->A21;
	res.A12 = self->A11 * other->A12 + self->A12 * other->A22;
	res.B1  = self->A11 * other->B1  + self->A12 * other->B2  + self->B1;
	res.A21 = self->A21 * other->A11 + self->A22 * other->A21;
	res.A22 = self->A21 * other->A12 + self->A22 * other->A22;
	res.B2  = self->A21 * other->B1  + self->A22 * other->B2  + self->B2;
	return res;
}
DaoxVector2D  DaoxMatrix3D_MulVector( DaoxMatrix3D *self, DaoxVector2D *vector, float w )
{
	DaoxVector2D res;
	res.x = self->A11 * vector->x + self->A12 * vector->y + w * self->B1;
	res.y = self->A21 * vector->x + self->A22 * vector->y + w * self->B2;
	return res;
}
DaoxVector2D  DaoxMatrix3D_RotateVector( DaoxVector2D vector, float alpha )
{
	DaoxVector2D origin = { 0, 0 };
	DaoxMatrix3D rotation = DaoxMatrix3D_PointRotation( origin, alpha );
	return DaoxMatrix3D_MulVector( & rotation, & vector, 0 );
}

void DaoxMatrix3D_Set( DaoxMatrix3D *self, float *mat, int n )
{
	if( n != 4 && n != 6 ) return;
	self->A11 = mat[0];
	self->A21 = mat[1];
	self->A12 = mat[2];
	self->A22 = mat[3];
	if( n == 6 ){
		self->B1 = mat[4];
		self->B2 = mat[5];
	}
}
void DaoxMatrix3D_RotateXAxisTo( DaoxMatrix3D *self, float x, float y )
{
	float r = sqrt( x*x + y*y );
	self->A11 =   x / r;
	self->A12 = - y / r;
	self->A21 =   y / r;
	self->A22 =   x / r;
}
void DaoxMatrix3D_RotateYAxisTo( DaoxMatrix3D *self, float x, float y )
{
	float r = sqrt( x*x + y*y );
	self->A11 =   y / r;
	self->A12 =   x / r;
	self->A21 = - x / r;
	self->A22 =   y / r;
}
void DaoxMatrix3D_SetScale( DaoxMatrix3D *self, float x, float y )
{
	self->A11 *= x;
	self->A12 *= x;
	self->A21 *= y;
	self->A22 *= y;
}
void DaoxMatrix3D_Multiply( DaoxMatrix3D *self, DaoxMatrix3D other )
{
	float A11 = self->A11 * other.A11 + self->A12 * other.A21;
	float A12 = self->A11 * other.A12 + self->A12 * other.A22;
	float A21 = self->A21 * other.A11 + self->A22 * other.A21;
	float A22 = self->A21 * other.A12 + self->A22 * other.A22;
	float B1 = self->A11 * other.B1 + self->A12 * other.B2 + self->B1;
	float B2 = self->A21 * other.B1 + self->A22 * other.B2 + self->B2;
	self->A11 = A11;
	self->A12 = A12;
	self->A21 = A21;
	self->A22 = A22;
	self->B1 = B1;
	self->B2 = B2;
}
DaoxVector2D DaoxMatrix3D_TransformXY( DaoxMatrix3D *self, float x, float y )
{
	DaoxVector2D pt;
	pt.x = self->A11 * x + self->A12 * y + self->B1;
	pt.y = self->A21 * x + self->A22 * y + self->B2;
	return pt;
}
DaoxVector2D DaoxMatrix3D_Transform( DaoxMatrix3D *self, DaoxVector2D point )
{
	return DaoxMatrix3D_TransformXY( self, point.x, point.y );
}
DaoxMatrix3D DaoxMatrix3D_Inverse( DaoxMatrix3D *self )
{
	DaoxMatrix3D inverse;
	double det = self->A11 * self->A22 - self->A12 * self->A21;
	inverse.A11 =   self->A22 / det;
	inverse.A12 = - self->A12 / det;
	inverse.A21 = - self->A21 / det;
	inverse.A22 =   self->A11 / det;
	inverse.B1 = - inverse.A11 * self->B1 - inverse.A12 * self->B2;
	inverse.B2 = - inverse.A21 * self->B1 - inverse.A22 * self->B2;
	return inverse;
}
void DaoxMatrix3D_Print( DaoxMatrix3D *self )
{
	printf( "DaoxMatrix3D: %9.3f %9.3f %9.3f %9.3f %9.3f %9.3f\n",
			self->A11, self->A12, self->A21, self->A22, self->B1, self->B2 );
}





DaoxOBBox2D DaoxOBBox2D_InitRect( float left, float right, float top, float bottom )
{
	DaoxOBBox2D obbox;
	obbox.O.x = left;
	obbox.O.y = bottom;
	obbox.X.x = right;
	obbox.X.y = bottom;
	obbox.Y.x = left;
	obbox.Y.y = top;
	return obbox;
}
int DaoxOBBox2D_Contain( DaoxOBBox2D *self, DaoxVector2D point )
{
	DaoxVector2D X = DaoxVector2D_Sub( & self->X, & self->O );
	DaoxVector2D Y = DaoxVector2D_Sub( & self->Y, & self->O );
	double x, dx = DaoxVector2D_Norm2( & X );
	double y, dy = DaoxVector2D_Norm2( & Y );
	point = DaoxVector2D_Sub( & point, & self->O );
	x = DaoxVector2D_Dot( & X, & point );
	y = DaoxVector2D_Dot( & Y, & point );
	if( !((x >= 0.0 && x <= dx) && (y >= 0.0 && y <= dy)) ){
		printf( "%15.9f %15.9f; %15.9f %15.9f\n", x, dx, y, dy );
	}
	return (x >= 0.0 && x <= dx) && (y >= 0.0 && y <= dy);
}
void DaoxOBBox2D_ResetBox( DaoxOBBox2D *self, DaoxVector2D points[], int count )
{
	DaoxVector2D xaxis1, xaxis2, yaxis1, yaxis2;
	DaoxVector2D first, second, third, zero = {0.0,0.0};
	DaoxVector2D xaxis, yaxis;
	double xmin, xmax, ymin, ymax, margin = EPSILON;
	double max = -1.0, max1 = -1.0, max2 = -1.0;
	daoint i, j;

	self->O = self->X = self->Y = zero;
	if( count == 0 ) return;

	first = second = third = points[0];
	self->O = self->X = self->Y = first;
	for(i=0; i<count; ++i){
		DaoxVector2D point = points[i];
		double dist2 = DaoxVector2D_Dist2( point, second );
		if( dist2 > max ){
			max = dist2;
			first = point;
		}
	}
	if( max == 0.0 ) return;
	/* Find the vertex that is furthest from the "first": */
	for(i=0; i<count; ++i){
		DaoxVector2D point = points[i];
		double dist = DaoxVector2D_Dist2( first, point );
		if( dist >= max1 ){
			xaxis = point;
			max1 = dist;
		}
	}
	xaxis = DaoxVector2D_Sub( & xaxis, & first );
	max1 = sqrt( max1 );
	xaxis.x /= max1;
	xaxis.y /= max1;
	yaxis.x = - xaxis.y;
	yaxis.y = xaxis.x;

	//printf( "dot: %9f\n", DaoxVector2D_Dot( & xaxis, & yaxis ) );

	/* Construct the bounding box aligned to the new "xaxis", "yaxis" and "zaxis": */
	xmin = xmax = ymin = ymax = 0.0;
	for(i=0; i<count; ++i){
		DaoxVector2D point = points[i];
		DaoxVector2D point2 = DaoxVector2D_Sub( & point, & first );
		double dotx = DaoxVector2D_Dot( & point2, & xaxis );
		double doty = DaoxVector2D_Dot( & point2, & yaxis );
		if( dotx <= xmin ) xmin = dotx;
		if( doty <= ymin ) ymin = doty;
		if( dotx >= xmax ) xmax = dotx;
		if( doty >= ymax ) ymax = doty;
	}
	xmin -= margin;
	ymin -= margin;
	xmax += margin;
	ymax += margin;
	xaxis1 = DaoxVector2D_Scale( & xaxis, xmin );
	yaxis1 = DaoxVector2D_Scale( & yaxis, ymin );
	xaxis2 = DaoxVector2D_Scale( & xaxis, xmax - xmin );
	yaxis2 = DaoxVector2D_Scale( & yaxis, ymax - ymin );
	self->O = first;
	self->O = DaoxVector2D_Add( & self->O, & xaxis1 );
	self->O = DaoxVector2D_Add( & self->O, & yaxis1 );
	self->X = DaoxVector2D_Add( & self->O, & xaxis2 );
	self->Y = DaoxVector2D_Add( & self->O, & yaxis2 );

	return;
	printf( "testing...... %i\n", count );
	printf( "max: %15.9f %15.9f %20.15f\n", max1, max2, DaoxVector2D_Dot( & xaxis, & yaxis ) );
	DaoxVector2D_Print( & self->O );
	DaoxVector2D_Print( & self->X );
	DaoxVector2D_Print( & self->Y );
	for(i=0; i<count; ++i){
		DaoxVector2D point = points[i];
		if( DaoxOBBox2D_Contain( self, point ) == 0 ){
			printf( "not contained: %5i\n", i );
			DaoxVector2D_Print( & point );
		}
	}
}
/*
// This function take a plane (passing "point" with normal "norm"),
// and a line segment (connecting P1 and P2) as parameter. It returns:
// -1, if the line segment P1P2 lies in the negative side of the plane;
// 0,  if the line segment cross the plane;
// 1,  if the line segment lies in the positive side of the plane;
*/
static int CheckLine( DaoxVector2D point, DaoxVector2D norm, DaoxVector2D P1, DaoxVector2D P2 )
{
	DaoxVector2D Q1 = DaoxVector2D_Sub( & P1, & point );
	DaoxVector2D Q2 = DaoxVector2D_Sub( & P2, & point );
	double dot1 = DaoxVector2D_Dot( & Q1, & norm );
	double dot2 = DaoxVector2D_Dot( & Q2, & norm );
	if( dot1 * dot2 <= 0.0 ) return 0;
	if( dot1 < 0.0 ) return -1;
	return 1;
}
static int CheckBox( DaoxVector2D point, DaoxVector2D norm, DaoxOBBox2D *box )
{
	DaoxVector2D DX, XY;
	int ch, check = CheckLine( point, norm, box->X, box->Y );

	if( check == 0 ) return 0;

	DX = DaoxVector2D_Sub( & box->X, & box->O );
	XY = DaoxVector2D_Add( & box->Y, & DX );

	ch = CheckLine( point, norm, box->O, XY );
	if( (check * ch) <= 0 ) return 0;

	return check;
}
enum CheckCode
{
	INSIDE ,  /* containded; */
	OUTSIDE , /* separated by edge; */
	CROSS ,   /* crossing each other; */
	MAYBE
};
static int CheckBox2( DaoxOBBox2D *self, DaoxOBBox2D *other )
{
	DaoxVector2D ynorm, xnorm = DaoxVector2D_Sub( & self->X, & self->O );
	int left, top, bottom, right = CheckBox( self->X, xnorm, other );

	if( right > 0 ) return OUTSIDE;

	left = - CheckBox( self->O, xnorm, other );
	if( left > 0 ) return OUTSIDE;

	ynorm = DaoxVector2D_Sub( & self->Y, & self->O );
	top = CheckBox( self->Y, ynorm, other );
	if( top > 0 ) return OUTSIDE;

	bottom = - CheckBox( self->O, ynorm, other );
	if( bottom > 0 ) return OUTSIDE;

	if( right < 0 && left < 0 && top < 0 && bottom < 0 ) return INSIDE;

	if( right < 0 && left < 0 ) return CROSS;
	if( top < 0 && bottom < 0 ) return CROSS;

	return MAYBE;
}
int DaoxOBBox2D_Intersect( DaoxOBBox2D *self, DaoxOBBox2D *other )
{
	DaoxVector2D C1 = DaoxVector2D_Interpolate( self->X, self->Y, 0.5 );
	DaoxVector2D C2 = DaoxVector2D_Interpolate( other->X, other->Y, 0.5 );
	double R1 = DaoxVector2D_Dist( self->X, C1 );
	double R2 = DaoxVector2D_Dist( other->X, C2 );

	if( DaoxVector2D_Dist( C1, C2 ) > (R1 + R2 + EPSILON) ) return -1;

	int check = CheckBox2( self, other );
	if( check == INSIDE ) return 1;
	if( check == OUTSIDE ) return -1;
	check = CheckBox2( other, self );
	if( check == INSIDE ) return 1;
	if( check == OUTSIDE ) return -1;
	return 0;
}
double DaoxOBBox2D_Area( DaoxOBBox2D *self )
{
	DaoxVector2D X = DaoxVector2D_Sub( & self->X, & self->O );
	DaoxVector2D Y = DaoxVector2D_Sub( & self->Y, & self->O );
	double W = sqrt( DaoxVector2D_Norm2( & X ) );
	double H = sqrt( DaoxVector2D_Norm2( & Y ) );
	//printf( "DaoxOBBox2D_Area: %25.20f %25.20f; %25.20f\n", W, H, W*H );
	return W*H;
}
/* TODO: invariable center */
DaoxOBBox2D DaoxOBBox2D_Scale( DaoxOBBox2D *self, float scale )
{
	DaoxOBBox2D obbox;
	obbox.O = DaoxVector2D_Scale( & self->O, scale );
	obbox.X = DaoxVector2D_Scale( & self->X, scale );
	obbox.Y = DaoxVector2D_Scale( & self->Y, scale );
	return obbox;
}
DaoxOBBox2D DaoxOBBox2D_Transform( DaoxOBBox2D *self, DaoxMatrix3D *transfrom )
{
	DaoxOBBox2D obbox;
	obbox.O = DaoxMatrix3D_MulVector( transfrom, & self->O, 1 );
	obbox.X = DaoxMatrix3D_MulVector( transfrom, & self->X, 1 );
	obbox.Y = DaoxMatrix3D_MulVector( transfrom, & self->Y, 1 );
	return obbox;
}
DaoxOBBox2D DaoxOBBox2D_CopyWithMargin( DaoxOBBox2D *self, double margin )
{
	DaoxOBBox2D res;
	DaoxVector2D dX, X = DaoxVector2D_Sub( & self->X, & self->O );
	DaoxVector2D dY, Y = DaoxVector2D_Sub( & self->Y, & self->O );
	double W = sqrt( DaoxVector2D_Norm2( & X ) ) + EPSILON;
	double H = sqrt( DaoxVector2D_Norm2( & Y ) ) + EPSILON;
	if( margin < -0.5*W ) margin = -0.5*W;
	if( margin < -0.5*H ) margin = -0.5*H;
	dX = DaoxVector2D_Scale( & X, margin / W );
	dY = DaoxVector2D_Scale( & Y, margin / H );
	res.O = DaoxVector2D_Add( & self->O, & dX );
	res.O = DaoxVector2D_Add( & res.O, & dY );
	X = DaoxVector2D_Scale( & X, (W+2*margin) / W );
	Y = DaoxVector2D_Scale( & Y, (H+2*margin) / H );
	res.X = DaoxVector2D_Add( & res.O, & X );
	res.Y = DaoxVector2D_Add( & res.O, & Y );
	//DaoxOBBox2D_Area( self );
	//DaoxOBBox2D_Area( & res );
	return res;
}
int DaoxOBBox2D_Intersect2( DaoxOBBox2D *self, DaoxOBBox2D *other, double tolerance )
{
	DaoxOBBox2D box1 = DaoxOBBox2D_CopyWithMargin( self, -0.5*tolerance );
	DaoxOBBox2D box2 = DaoxOBBox2D_CopyWithMargin( other, -0.5*tolerance );
	return DaoxOBBox2D_Intersect( & box1, & box2 );
}







void DaoxAABBox2D_AddMargin( DaoxAABBox2D *self, float margin )
{
	self->left -= margin;
	self->bottom -= margin;
	self->right += margin;
	self->top += margin;
}
void DaoxAABBox2D_InitXY( DaoxAABBox2D *self, float x, float y )
{
	self->left = self->right = x;
	self->bottom = self->top = y;
}
void DaoxAABBox2D_Init( DaoxAABBox2D *self, DaoxVector2D point )
{
	self->left = self->right = point.x;
	self->bottom = self->top = point.y;
}
void DaoxAABBox2D_UpdateXY( DaoxAABBox2D *self, float x, float y )
{
	if( x < self->left ) self->left = x;
	if( x > self->right ) self->right = x;
	if( y < self->bottom ) self->bottom = y;
	if( y > self->top ) self->top = y;
}
void DaoxAABBox2D_Update( DaoxAABBox2D *self, DaoxVector2D point )
{
	DaoxAABBox2D_UpdateXY( self, point.x, point.y );
}
DaoxAABBox2D DaoxAABBox2D_Transform( DaoxAABBox2D *self, DaoxMatrix3D *transform )
{
	DaoxAABBox2D box;
	DaoxVector2D P1 = DaoxMatrix3D_TransformXY( transform, self->left, self->bottom );
	DaoxVector2D P2 = DaoxMatrix3D_TransformXY( transform, self->left, self->top );
	DaoxVector2D P3 = DaoxMatrix3D_TransformXY( transform, self->right, self->top );
	DaoxVector2D P4 = DaoxMatrix3D_TransformXY( transform, self->right, self->bottom );
	DaoxAABBox2D_Init( & box, P1 );
	DaoxAABBox2D_Update( & box, P2 );
	DaoxAABBox2D_Update( & box, P3 );
	DaoxAABBox2D_Update( & box, P4 );
	return box;
}
void DaoxAABBox2D_Print( DaoxAABBox2D *self )
{
	printf( "DaoxAABBox2D: %9.3f %9.3f %9.3f %9.3f\n",
			self->left, self->right, self->bottom, self->top );
}




DaoxVector2D* DArray_PushVector2D( DArray *self, DaoxVector2D *vector2d )
{
	DaoxVector2D *res = (DaoxVector2D*) DArray_Push( self );
	if( vector2d ) *res = *vector2d;
	return res;
}
DaoxVector3D* DArray_PushVector3D( DArray *self, DaoxVector3D *vector3d )
{
	DaoxVector3D *res = (DaoxVector3D*) DArray_Push( self );
	if( vector3d ) *res = *vector3d;
	return res;
}

DaoxVector2D* DArray_PushVectorXY( DArray *self, float x, float y )
{
	DaoxVector2D *item = (DaoxVector2D*) DArray_Push( self );
	item->x = x;
	item->y = y;
	return item;
}
DaoxVector3D* DArray_PushVectorXYZ( DArray *self, float x, float y, float z )
{
	DaoxVector3D *item = (DaoxVector3D*) DArray_Push( self );
	item->x = x;
	item->y = y;
	item->z = z;
	return item;
}
DaoxIndexFloat* DArray_PushIndexFloat( DArray *self, int index, float value )
{
	DaoxIndexFloat *item = (DaoxIndexFloat*) DArray_Push( self );
	item->index = index;
	item->value = value;
	return item;
}

void DaoxIndexFloats_PartialQuickSort( DaoxIndexFloat *data, int first, int last, int part )
{
	int lower=first+1, upper=last;
	float pivot;
	DaoxIndexFloat val;

	if( first >= last ) return;
	val = data[first];
	data[first] = data[ (first+last)/2 ];
	data[ (first+last)/2 ] = val;
	pivot = data[ first ].value;

	while( lower <= upper ){
		while( lower <= last && data[lower].value < pivot ) lower ++;
		while( pivot < data[upper].value ) upper --;
		if( lower < upper ){
			val = data[lower];
			data[lower] = data[upper];
			data[upper] = val;
			upper --;
		}
		lower ++;
	}
	val = data[first];
	data[first] = data[upper];
	data[upper] = val;
	if( first < upper-1 ) DaoxIndexFloats_PartialQuickSort( data, first, upper-1, part );
	if( upper >= part ) return;
	if( upper+1 < last ) DaoxIndexFloats_PartialQuickSort( data, upper+1, last, part );
}
void DArray_SortIndexFloats( DArray *self )
{
	if( self->size <= 1 ) return;
	DaoxIndexFloats_PartialQuickSort( self->data.indexfloats, 0, self->size-1, self->size );
}

void DList_PartialQuickSort( void *data[], int first, int last, int part, DList_CompareItem cmpfunc )
{
	int lower=first+1, upper=last;
	void *pivot;
	void *val;

	if( first >= last ) return;
	val = data[first];
	data[first] = data[ (first+last)/2 ];
	data[ (first+last)/2 ] = val;
	pivot = data[ first ];

	while( lower <= upper ){
		while( lower <= last && (*cmpfunc)( data[lower], pivot ) < 0 ) lower ++;
		while( (*cmpfunc)( pivot, data[upper] ) < 0 ) upper --;
		if( lower < upper ){
			val = data[lower];
			data[lower] = data[upper];
			data[upper] = val;
			upper --;
		}
		lower ++;
	}
	val = data[first];
	data[first] = data[upper];
	data[upper] = val;
	if( first < upper-1 ) DList_PartialQuickSort( data, first, upper-1, part, cmpfunc );
	if( upper >= part ) return;
	if( upper+1 < last ) DList_PartialQuickSort( data, upper+1, last, part, cmpfunc );
}
void DList_Sort( DList *self, DList_CompareItem cmpfunc )
{
	DList_PartialQuickSort( self->items.pVoid, 0, self->size-1, self->size, cmpfunc );
}


double DaoxMath_Clamp( double value, double min, double max )
{
	if( value < min ) value = min;
	if( value > max ) value = max;
	return value;
}
