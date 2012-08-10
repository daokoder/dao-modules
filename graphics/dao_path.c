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
#include <assert.h>
#include "daoStdtype.h"
#include "dao_path.h"
#include "dao_graphics.h"



DaoxIntArray* DaoxIntArray_New()
{
	DaoxIntArray *self = (DaoxIntArray*) calloc(1,sizeof(DaoxIntArray));
	return self;
}
void DaoxIntArray_Delete( DaoxIntArray *self )
{
	if( self->values ) free( self->values );
	free( self );
}
void DaoxIntArray_Reset( DaoxIntArray *self )
{
	self->count = 0;
}
void DaoxIntArray_Push( DaoxIntArray *self, int value )
{
	if( self->count >= self->capacity ){
		self->capacity += 0.2 * self->capacity + 1;
		self->values = (int*) dao_realloc( self->values, self->capacity * sizeof(int) );
	}
	self->values[ self->count ] = value;
	self->count += 1;
}




DaoxPathSegment* DaoxPathSegment_New( DaoxPathComponent *component )
{
	DaoxPathSegment* self = (DaoxPathSegment*) calloc(1,sizeof(DaoxPathSegment));
	self->start = 0.0;
	self->end = 1.0;
	self->component = component;
	self->maxlen = self->maxdiff = 1E16;
	return self;
}

DaoxPathComponent* DaoxPathComponent_New( DaoxPath *path )
{
	DaoxPathComponent* self = (DaoxPathComponent*) calloc(1,sizeof(DaoxPathComponent));
	self->path = path;
	self->maxlen = self->maxdiff = 1E16;
	self->first = self->last = DaoxPathSegment_New( self );
	return self;
}
DaoxPathSegment* DaoxPathComponent_PushSegment( DaoxPathComponent *self )
{
	DaoxPathSegment *segment;
	if( self->first->bezier == 0 ) return self->first;
	segment = DaoxPathSegment_New( self );
	self->last->next = segment;
	self->last = segment;
	return segment;
}

DaoxPath* DaoxPath_New()
{
	DaoxPath *self = (DaoxPath*) calloc(1,sizeof(DaoxPath));
	self->first = self->last = DaoxPathComponent_New( self );
	self->points = DaoxPointArray_New();
	self->triangles = DaoxIntArray_New();
	return self;
}
void DaoxPath_Delete( DaoxPath *self )
{
	// TODO:
}


DaoxPathComponent* DaoxPath_PushComponent( DaoxPath *self )
{
	DaoxPathComponent *com;
	if( self->first->first->bezier == 0 ) return self->first;
	com = DaoxPathComponent_New( self );
	self->last->next = com;
	self->last = com;
	return com;
}
void DaoxPath_MoveTo( DaoxPath *self, double x, double y )
{
	DaoxPathComponent *com;
	if( self->last->last->bezier == 0 ){
		self->last->last->P1.x = x;
		self->last->last->P1.y = y;
		return;
	}
	com = DaoxPathComponent_New( self );
	com->last->P1.x = x;
	com->last->P1.y = y;
	self->last->next = com;
	self->last = com;
}
/* If MoveTo() is not called, line from (0,0). */
void DaoxPath_LineTo( DaoxPath *self, double x, double y )
{
	DaoxPoint start;
	DaoxPathSegment *segment = NULL;
	if( self->last->last->bezier == 0 ){
		start = self->last->last->P1;
		segment = self->last->last;
	}else{
		start = self->last->last->P2;
		segment = DaoxPathComponent_PushSegment( self->last );
		segment->P1 = start;
	}
	segment->bezier = 1;
	segment->P2.x = start.x + x;
	segment->P2.y = start.y + y;
}
/*
// Quoted from: http://en.wikipedia.org/wiki/Bezier_spline
// We may compose a circle of radius R from an arbitrary number of cubic Bezier curves.
// Let the arc start at point A and end at point B, placed at equal distances above 
// and below the x-axis, spanning an arc of angle theta=2*phi:
//     AX = cos(phi)
//     AY = sin(phi)
//     BX =  AX
//     BY = -AY
// The control points may be written as:
//     CAX = (4-AX)/3
//     CAY = (1-AX)*(3-AX)/(3*AY)
//     CBX =  CAX
//     CBY = -CAY
*/
void DaoxPathSegment_MakeArc( DaoxPathSegment *self, double dx, double dy, double R, double degree )
{
	double angle = 0.0;
	double AX = cos( 0.5 * degree );
	double AY = - sin( 0.5 * degree );
	double BX = AX, BY = - AY;
	double CAX = (4.0 - AX) / 3.0;
	double CAY = (1.0 - AX)*(3.0 - AX) / (3.0 * AY);
	double CBX = CAX, CBY = - CAY;
	double CAX2 = CAX - AX;  /* control point relative to A; */
	double CAY2 = CAY - AY;
	double CBX2 = CBX - BX;  /* control point relative to B; */
	double CBY2 = CBY - BY;
	double sine, cosine;

	self->bezier = 3;
	//self->count = 1;
	self->P2.x = self->P1.x + dx;
	self->P2.y = self->P1.y + dy;

	printf( ")))))))))))))))))))))))))) %15f %15f\n", CAX2, CAY2 );
	printf( ")))))))))))))))))))))))))) %15f %15f\n", CBX2, CBY2 );
	printf( "%15f: %15f %15f, %15f %15f\n", R, dx, dy, self->P2.x, self->P2.y );

	if( fabs( dx ) < 1E-16 ){
		angle = dy >= 0.0 ? 0.5*M_PI : 1.5*M_PI;
	}else{
		angle = atan( dy / fabs( dx ) );
		if( dx < 0.0 ) angle = M_PI - angle;
	}
	sine = R * sin( angle - 0.5 * M_PI );
	cosine = R * cos( angle - 0.5 * M_PI );
	printf( ")))))))))))))))))))))))))) %15f %15f\n", (CBX2 * cosine - CBY2 * sine)/R, (CBX2 * sine + CBY2 * cosine)/R );
	printf( ")))))))))))))))))))))))))) %15f %15f\n", (CAX2 * cosine - CAY2 * sine)/R, (CAX2 * sine + CAY2 * cosine)/R );
	self->C1.x = self->P1.x + CAX2 * cosine - CAY2 * sine;
	self->C1.y = self->P1.y + CAX2 * sine + CAY2 * cosine;
	self->C2.x = self->P2.x + CBX2 * cosine - CBY2 * sine;
	self->C2.y = self->P2.y + CBX2 * sine + CBY2 * cosine;
	//self->C1.x = 0.5 * (self->C1.x + self->C2.x);
	//self->C1.y = 0.5 * (self->C1.y + self->C2.y);
	printf( "%15f %15f, %15f %15f\n", self->C1.x, self->C1.y, self->C2.x, self->C2.y );
}
void DaoxPath_ArcTo2( DaoxPath *self, double x, double y, double degrees, double deg2 )
{
	DaoxPoint point, start, next, end, center; /* A: start; B: end; C: center; */
	DaoxPathSegment *segment = NULL;
	double degrees2 = M_PI * degrees / 180.0;
	double t = tan( 0.5 * degrees2 ) + 1E-12;
	double dx, dy, R, dA, sine, cosine, dL;
	int i, K;

	if( self->last->last->bezier == 0 ){
		start = self->last->last->P1;
		segment = self->last->last;
	}else{
		start = self->last->last->P2;
		segment = DaoxPathComponent_PushSegment( self->last );
		segment->P1 = start;
	}

	center.x = 0.5 * x;
	center.y = 0.5 * y;
	dx = - center.x;
	dy = - center.y;
	if( degrees < 0.0 ){
		center.x += - dy / t;
		center.y += + dx / t;
	}else{
		center.x += + dy / t;
		center.y += - dx / t;
	}
	/* Now "center" has coordinates relative to the start point; */
	R = sqrt( center.x * center.x + center.y * center.y ); /* distance to the start; */
	if( degrees < deg2 ){
		DaoxPathSegment_MakeArc( segment, x, y, R, degrees2 );
		return;
	}

	/* Now make the coordinates of "center" absolute, and all others relative to "center": */
	center.x += start.x;
	center.y += start.y;
	end.x = start.x + x - center.x;
	end.y = start.y + y - center.y;
	start.x -= center.x;
	start.y -= center.y;
	point = start;

	K = 1 + fabs( degrees ) / deg2;
	dA = degrees2 / K;
	dL = 1.0 / (double)K;
	for(i=0; i<K; ++i){
		if( segment == NULL ){
			segment = DaoxPathComponent_PushSegment( self->last );
			segment->P1.x = point.x + center.x;
			segment->P1.y = point.y + center.y;
		}
		segment->start = i * dL;
		segment->end = (i + 1) * dL;
		if( i == (K-1) ) segment->end = 1.0;
		sine = sin( (i + 1) * dA );
		cosine = cos( (i + 1) * dA );
		next.x = start.x * cosine - start.y * sine;
		next.y = start.x * sine + start.y * cosine;
		DaoxPathSegment_MakeArc( segment, next.x - point.x, next.y - point.y, R, dA );
		point = next;
		segment = NULL;
	}

}
void DaoxPath_ArcTo( DaoxPath *self, double x, double y, double degrees )
{
	DaoxPath_ArcTo2( self, x, y, degrees, 30.0 );
}
void DaoxPath_QuadTo( DaoxPath *self, double cx, double cy, double x, double y )
{
	DaoxPathSegment *segment = NULL;
	DaoxPoint start = {0.0,0.0};

	if( self->last->last->bezier == 0 ){
		start = self->last->last->P1;
		segment = self->last->last;
	}else{
		start = self->last->last->P2;
		segment = DaoxPathComponent_PushSegment( self->last );
		segment->P1 = start;
	}
	segment->bezier = 2;
	segment->C1.x = start.x + cx;
	segment->C1.y = start.y + cy;
	segment->C2 = segment->C1;
	segment->P2.x = start.x + x;
	segment->P2.y = start.y + y;
}
void DaoxPath_CubicTo( DaoxPath *self, double cx, double cy, double x, double y )
{
	DaoxPathSegment *segment = NULL;
	DaoxPoint control = self->last->last->C2;
	DaoxPoint start = self->last->last->P2;

	assert( self->last->last->bezier );
	control.x = 2.0 * start.x - control.x;
	control.y = 2.0 * start.y - control.y;
	segment = DaoxPathComponent_PushSegment( self->last );
	segment->bezier = 3;
	segment->P1 = start;
	segment->C1 = control;
	segment->C2.x = start.x + cx;
	segment->C2.y = start.y + cy;
	segment->P2.x = start.x + x;
	segment->P2.y = start.y + y;
}
void DaoxPath_CubicTo2( DaoxPath *self, double cx1, double cy1, double cx2, double cy2, double x2, double y2 )
{
	DaoxPathSegment *segment = NULL;
	DaoxPoint start = {0.0,0.0};

	if( self->last->last->bezier == 0 ){
		start = self->last->last->P1;
		segment = self->last->last;
	}else{
		start = self->last->last->P2;
		segment = DaoxPathComponent_PushSegment( self->last );
		segment->P1 = start;
	}
	segment->bezier = 3;
	segment->C1.x = start.x + cx1;
	segment->C1.y = start.y + cy1;
	segment->C2.x = start.x + x2 + cx2;
	segment->C2.y = start.y + y2 + cy2;
	segment->P2.x = start.x + x2;
	segment->P2.y = start.y + y2;
}
void DaoxPath_Close( DaoxPath *self )
{
	DaoxPathSegment *last = self->last->last;
	DaoxPathSegment *segment = NULL;
	if( self->last->first == self->last->last ){
		if( self->last->first->bezier == 0 ) return;  /* no component data; */
	}
	if( DaoxDistance( self->last->last->P2, self->last->first->P1 ) < 1E-16 ){
		self->last->last->next = self->last->first;
		return;
	}
	segment = DaoxPathComponent_PushSegment( self->last );
	segment->bezier = 1;
	segment->P1 = last->P2;
	segment->P2 = self->last->first->P1;
	segment->next = self->last->first;
}






/*
// 1. Subdivide path/border segments until all of them are locally convex or concave;
// 2. Tiny subdivision close to junctions;
// 3. Make junctions;
// 4. Store points, and borders:
//    for locally convex segment, store P1 and P2 as triangulation points,
//       and the segment as border;
//    for locally concave segment, store P1,C1 (and C2), P2 as triangulation points,
//       and store the segement as border;
// 5. Triangulate the polygon formed by the points from the path segments;
// 6. Check each triangle with an edge on the border:
// 7. Store other triangles without edges on borders.
//
// Implementation:
// 1. Subdivide until the length of P1C1,C1C2,C2P2 is only a small percent longer
//    than P1P2;
// 2. And subdivide until all the segments are locally convex or concave;
// 3. And possibly subdivide until no bounding triangle or quad intersects;
*/


void DaoxPathSegment_SetPoints( DaoxPathSegment *self, DaoxPoint P1, DaoxPoint P2, DaoxPoint C1, DaoxPoint C2 )
{
	self->P1 = P1;
	self->P2 = P2;
	self->C1 = C1;
	self->C2 = C2;
	self->count = 1;
	self->start = 0.0;
	self->end = 1.0;
}
void DaoxPathSegment_InitSubSegments( DaoxPathSegment *self )
{
	if( self->first == NULL ) self->first = DaoxPathSegment_New( self->component );
	if( self->second == NULL ) self->second = DaoxPathSegment_New( self->component );
	self->first->bezier = self->second->bezier = self->bezier;
	self->first->convexness = self->second->convexness = self->convexness;
	self->first->count = self->second->count = 1;
	self->count = 2;
}
void DaoxPathSegment_DivideLinear( DaoxPathSegment *self, double at )
{
	DaoxPathSegment_InitSubSegments( self );

	self->first->P1 = self->P1;
	self->first->P2.x = (1.0 - at)*self->P1.x + at*self->P2.x;
	self->first->P2.y = (1.0 - at)*self->P1.y + at*self->P2.y;
	self->second->P1 = self->first->P2;
	self->second->P2 = self->P2;

	self->first->start = self->start;
	self->second->end = self->end;
	self->first->end = self->second->start = (1.0 - at)*self->start + at*self->end;
}
void DaoxPathSegment_DivideQuadratic( DaoxPathSegment *self, double at )
{
	DaoxPoint Q1;

	DaoxPathSegment_InitSubSegments( self );

	self->first->P1 = self->P1;
	self->first->C1.x = (1.0 - at)*self->P1.x + at*self->C1.x;
	self->first->C1.y = (1.0 - at)*self->P1.y + at*self->C1.y;

	self->second->P2 = self->P2;
	self->second->C1.x = (1.0 - at)*self->C1.x + at*self->P2.x;
	self->second->C1.y = (1.0 - at)*self->C1.y + at*self->P2.y;

	Q1.x = (1.0 - at)*self->first->C1.x + at*self->second->C1.x;
	Q1.y = (1.0 - at)*self->first->C1.y + at*self->second->C1.y;
	self->first->P2 = Q1;
	self->second->P1 = Q1;

	self->first->start = self->start;
	self->second->end = self->end;
	self->first->end = self->second->start = (1.0 - at)*self->start + at*self->end;
}
void DaoxPathSegment_DivideCubic( DaoxPathSegment *self, double at )
{
	DaoxPoint Q1;

	DaoxPathSegment_InitSubSegments( self );

	Q1.x = (1.0 - at)*self->C1.x + at*self->C2.x;
	Q1.y = (1.0 - at)*self->C1.y + at*self->C2.y;

	self->first->P1 = self->P1;
	self->first->C1.x = (1.0 - at)*self->P1.x + at*self->C1.x;
	self->first->C1.y = (1.0 - at)*self->P1.y + at*self->C1.y;
	self->first->C2.x = (1.0 - at)*self->first->C1.x + at*Q1.x;
	self->first->C2.y = (1.0 - at)*self->first->C1.y + at*Q1.y;

	self->second->P2 = self->P2;
	self->second->C2.x = (1.0 - at)*self->C2.x + at*self->P2.x;
	self->second->C2.y = (1.0 - at)*self->C2.y + at*self->P2.y;
	self->second->C1.x = (1.0 - at)*Q1.x + at*self->second->C2.x;
	self->second->C1.y = (1.0 - at)*Q1.y + at*self->second->C2.y;

	Q1.x = (1.0 - at)*self->first->C2.x + at*self->second->C1.x;
	Q1.y = (1.0 - at)*self->first->C2.y + at*self->second->C1.y;
	self->first->P2 = Q1;
	self->second->P1 = Q1;

	self->first->start = self->start;
	self->second->end = self->end;
	self->first->end = self->second->start = (1.0 - at)*self->start + at*self->end;
}
void DaoxPathSegment_Divide( DaoxPathSegment *self, double at )
{
	if( self->bezier == 2 ){
		DaoxPathSegment_DivideQuadratic( self, at );
	}else if( self->bezier == 3 ){
		DaoxPathSegment_DivideCubic( self, at );
	}else{
		DaoxPathSegment_DivideLinear( self, at );
	}
}


void DaoxPathSegment_RefineQuadratic( DaoxPathSegment *self, double maxlen, double maxdiff )
{
	double PC, CP, PP;

	if( self->count > 1 ){
		DaoxPathSegment_RefineQuadratic( self->first, maxlen, maxdiff );
		DaoxPathSegment_RefineQuadratic( self->second, maxlen, maxdiff );
		self->count = self->first->count + self->second->count;
		return;
	}

	PC = DaoxDistance( self->P1, self->C1 );
	CP = DaoxDistance( self->C1, self->P2 );
	PP = DaoxDistance( self->P1, self->P2 );

	self->maxlen = PP;
	self->maxdiff = (PC + CP - PP) / PP;
	if( PP < maxlen && ((PC + CP - PP) < maxdiff*PP || PP < 0.01*maxlen) ){
		self->count = 1;
		if( self->first ) self->first->count = 0;
		if( self->second ) self->second->count = 0;
		if( self->maxlen > self->component->maxlen ) self->component->maxlen = self->maxlen;
		if( self->maxdiff > self->component->maxdiff ) self->component->maxdiff = self->maxdiff;
		return;
	}
	DaoxPathSegment_DivideQuadratic( self, PC / (PC + CP) );

	DaoxPathSegment_RefineQuadratic( self->first, maxlen, maxdiff );
	DaoxPathSegment_RefineQuadratic( self->second, maxlen, maxdiff );
	self->count = self->first->count + self->second->count;
}
void DaoxPathSegment_RefineCubic( DaoxPathSegment *self, double maxlen, double maxdiff )
{
	double PC, CC, CP, PP;

	if( self->count > 1 ){
		DaoxPathSegment_RefineCubic( self->first, maxlen, maxdiff );
		DaoxPathSegment_RefineCubic( self->second, maxlen, maxdiff );
		self->count = self->first->count + self->second->count;
		return;
	}

	PC = DaoxDistance( self->P1, self->C1 );
	CC = DaoxDistance( self->C1, self->C2 );
	CP = DaoxDistance( self->C2, self->P2 );
	PP = DaoxDistance( self->P1, self->P2 );

	self->maxlen = PP;
	self->maxdiff = (PC + CC + CP - PP) / PP;
	if( PP < maxlen && ((PC + CC + CP - PP) < maxdiff*PP || PP < 0.01*maxlen) ){
		self->count = 1;
		if( self->first ) self->first->count = 0;
		if( self->second ) self->second->count = 0;
		if( self->maxlen > self->component->maxlen ) self->component->maxlen = self->maxlen;
		if( self->maxdiff > self->component->maxdiff ) self->component->maxdiff = self->maxdiff;
		return;
	}
	DaoxPathSegment_DivideCubic( self, PC / (PC + CP) );

	DaoxPathSegment_RefineCubic( self->first, maxlen, maxdiff );
	DaoxPathSegment_RefineCubic( self->second, maxlen, maxdiff );
	self->count = self->first->count + self->second->count;
}

void DaoxPathSegment_Refine( DaoxPathSegment *self, double maxlen, double maxdiff )
{
	if( self->bezier == 1 ){
		self->count = 1;
	}else if( self->bezier == 2 ){
		DaoxPathSegment_RefineQuadratic( self, maxlen, maxdiff );
	}else if( self->bezier == 3 ){
		DaoxPathSegment_RefineCubic( self, maxlen, maxdiff );
	}
}

void DaoxPathComponent_Refine( DaoxPathComponent *self, double maxlen, double maxdiff )
{
	DaoxPathSegment *first = self->refined.first ? self->refined.first : self->first;
	DaoxPathSegment *segment = first;
	do {
		DaoxPathSegment_Refine( segment, maxlen, maxdiff );
		if( segment->count == 1 ) DaoxPathSegment_Divide( segment, 0.5 );
		segment = segment->next;
	} while( segment && segment != first );
}
void DaoxPathComponent_RetrieveSegment( DaoxPathComponent *self, DaoxPathSegment *segment )
{
	if( segment->count == 1 ){
		segment->next = NULL;
		if( self->refined.first == NULL ){
			self->refined.first = self->refined.last = segment;
		}else{
			self->refined.last->next = segment;
			self->refined.last = segment;
		}
	}else if( segment->count ){
		DaoxPathComponent_RetrieveSegment( self, segment->first );
		DaoxPathComponent_RetrieveSegment( self, segment->second );
	}
}
void DaoxPathComponent_RetrieveRefined( DaoxPathComponent *self )
{
	DaoxPathSegment *segment = self->first;
	self->refined.first = self->refined.last = NULL;
	do {
		DaoxPathComponent_RetrieveSegment( self, segment );
		segment = segment->next;
	} while( segment && segment != self->first );
	if( self->last->next == self->first ) self->refined.last->next = self->refined.first;
}
int DaoxPathSegment_CheckConvexness2( DaoxPathSegment *self, DaoxPoint point )
{
	double area1, area2, epsilon = 0.0;
	if( self->bezier == 1 ) return 0;
	area1 = DaoxTriangle_Area( self->P1, self->P2, point );
	area2 = DaoxTriangle_Area( self->P1, self->P2, self->C1 );
	if( area1 < epsilon ) area2 = - area2;
	if( area2 > epsilon ) return -1;
	if( area2 < epsilon ) return 1;
	return 0;
}
void DaoxPathSegment_SetConvexness( DaoxPathSegment *self, int convexness )
{
	self->convexness = convexness;
	if( self->count > 1 ){
		DaoxPathSegment_SetConvexness( self->first, convexness );
		DaoxPathSegment_SetConvexness( self->second, convexness );
	}
}
void DaoxPathSegment_CheckConvexness( DaoxPathSegment *self, DaoxPoint point )
{
	int convexness = DaoxPathSegment_CheckConvexness2( self, point );
	DaoxPathSegment_SetConvexness( self, convexness );
}
/*
// For locally concave path segment, add smaller triangles that are fully
// contained in the region enclosed by the path:
*/
void DaoxPathSegment_AddInnerTriangles( DaoxPathSegment *self, int start, int end, int p3 )
{
	DaoxPath *path = self->component->path;
	int n = path->points->count;
	if( self->bezier != 2 && self->bezier != 3 ) return;
	DaoxPointArray_Push( path->points, self->C1 );
	DaoxIntArray_Push( path->triangles, start );
	DaoxIntArray_Push( path->triangles, n );
	DaoxIntArray_Push( path->triangles, p3 );
	if( self->bezier == 2 ){
		DaoxIntArray_Push( path->triangles, n );
		DaoxIntArray_Push( path->triangles, end );
		DaoxIntArray_Push( path->triangles, p3 );
	}else{
		DaoxPointArray_Push( path->points, self->C2 );
		DaoxIntArray_Push( path->triangles, n );
		DaoxIntArray_Push( path->triangles, n+1 );
		DaoxIntArray_Push( path->triangles, p3 );
		DaoxIntArray_Push( path->triangles, n+1 );
		DaoxIntArray_Push( path->triangles, end );
		DaoxIntArray_Push( path->triangles, p3 );
	}
}

DaoxPoint DaoxPoint_Transform( DaoxPoint self, double transform[6] )
{
	DaoxPoint point;
	point.x = transform[0] * self.x + transform[2] * self.y + transform[4];
	point.y = transform[1] * self.x + transform[3] * self.y + transform[5];
	return point;
}

void DaoxPath_ImportPath( DaoxPath *self, DaoxPath *path, double transform[6] )
{
	DaoxPathComponent *com, *com2;
	DaoxPathSegment *seg, *seg2;
	for(com=path->first; com; com=com->next){
		if( com->first->bezier == 0 ) continue;
		com2 = DaoxPath_PushComponent( self );
		seg = com->first;
		do {
			seg2 = DaoxPathComponent_PushSegment( com2 );
			seg2->count = 1;
			seg2->bezier = seg->bezier;
			seg2->start = seg->start;
			seg2->end = seg->end;
			seg2->P1 = DaoxPoint_Transform( seg->P1, transform );
			seg2->P2 = DaoxPoint_Transform( seg->P2, transform );
			seg2->C1 = DaoxPoint_Transform( seg->C1, transform );
			seg2->C2 = DaoxPoint_Transform( seg->C2, transform );
			seg = seg->next;
		} while( seg && seg != com->first );
		if( com->last->next == com->first ) com2->last->next = com2->first;
	}
}

void DaoxPath_Refine( DaoxPath *self, double maxlen, double maxdiff )
{
	DaoxPathComponent *com;
	printf( "DaoxPath_Refine: %15f %15f\n", maxlen, maxdiff );
	for(com=self->first; com; com=com->next){
		if( com->first->bezier == 0 ) continue;
		printf( "1: maxlen = %15f;  maxdiff = %15f\n", com->maxlen, com->maxdiff );
		if( maxlen > com->maxlen && maxdiff > com->maxdiff ) continue;
		com->maxlen = com->maxdiff = 0.0;
		DaoxPathComponent_Refine( com, maxlen, maxdiff );
		printf( "2: maxlen = %15f;  maxdiff = %15f\n", com->maxlen, com->maxdiff );
	}
}

void DaoxPath_Preprocess( DaoxPath *self, DaoxPathBuffer *buffer )
{
	DArray *segments = DArray_New(0);
	DaoxTriangulator *triangulator = buffer->triangulator;
	DaoxPathComponent *com;
	DaoxPathSegment *seg;
	daoint i, count = 0;

#if 0
	DaoxPath_MoveTo( self, 3000, 1000 );
	DaoxPath_ArcTo2( self, 0, 4000, 180, 180 );
	DaoxPath_ArcTo( self, 0, -4000, 180 );
	DaoxPath_Close( self );
#endif

	printf( "DaoxPath_Segment 1: %p\n", triangulator );
	DaoxTriangulator_Reset( triangulator );
	DaoxPath_Refine( self, 1E16, 0.1 );
	for(com=self->first; com; com=com->next){
		if( com->first->bezier == 0 ) continue;
		// TODO: refine for junctions;
		DaoxPathComponent_RetrieveRefined( com );
		if( com->refined.last == NULL || com->refined.last->next == NULL ) continue;
		seg = com->refined.first;
		do {
			DaoxTriangulator_PushPoint( triangulator, seg->P1.x, seg->P1.y );
			DArray_PushBack( segments, seg );
			seg = seg->next;
		} while( seg && seg != com->refined.first );
		DaoxTriangulator_CloseContour( triangulator );
	}
	printf( "DaoxPath_Segment 2: %i\n", (int) triangulator->vertices->size );
	DaoxTriangulator_Triangulate( triangulator );
	printf( "DaoxPath_Segment 2: %i\n", (int) triangulator->triangles->size );
	self->points->count = 0;
	DaoxPointArray_PushPoints( self->points, triangulator->points );
	for(i=0; i<triangulator->triangles->size; i+=3){
		int C = triangulator->triangles->items.pInt[i];
		int A = triangulator->triangles->items.pInt[i+1];
		int B = triangulator->triangles->items.pInt[i+2];
		int m = 0;
		DaoxPathSegment *SC = (DaoxPathSegment*) segments->items.pVoid[C];
		DaoxPathSegment *SA = (DaoxPathSegment*) segments->items.pVoid[A];
		DaoxPathSegment *SB = (DaoxPathSegment*) segments->items.pVoid[B];
#if 0
		printf( "A:  %15p  %15f  %15f\n", SA, SA->P1.x, SA->P1.y );
		printf( "B:  %15p  %15f  %15f\n", SB, SB->P1.x, SB->P1.y );
		printf( "C:  %15p  %15f  %15f\n", SC, SC->P1.x, SC->P1.y );
#endif
		if( SC->next != SA && SA->next != SB && SB->next != SC ) goto PushTriangle;
		if( SC->next == SA ) DaoxPathSegment_CheckConvexness( SC, SB->P1 );
		if( SA->next == SB ) DaoxPathSegment_CheckConvexness( SA, SC->P1 );
		if( SB->next == SC ) DaoxPathSegment_CheckConvexness( SB, SA->P1 );
		if( SA->convexness >= 0 && SB->convexness >= 0 && SC->convexness >= 0 ) goto PushTriangle;
		if( SC->next == SA && SC->convexness < 0 ){
			DaoxPathSegment_AddInnerTriangles( SC, C, A, B );
			m += 1;
		}
		if( SA->next == SB && SA->convexness < 0 ){
			DaoxPathSegment_AddInnerTriangles( SA, A, B, C );
			m += 1;
		}
		if( SB->next == SC && SB->convexness < 0 ){
			DaoxPathSegment_AddInnerTriangles( SB, B, C, A );
			m += 1;
		}
		if( m ) continue;
PushTriangle:
		DaoxIntArray_Push( self->triangles, C );
		DaoxIntArray_Push( self->triangles, A );
		DaoxIntArray_Push( self->triangles, B );
	}
	
	DArray_Delete( segments );
}

void DaoxPathSegment_GetRefined( DaoxPathSegment *self, DaoxGraphicsData *gdata )
{
	DaoxPolygonArray *fills = gdata->fillPolygons;
	double maxlen = gdata->maxlen;
	double maxdiff = gdata->maxdiff;

	if( maxlen < 1E-16 ) maxlen = 10;
	if( maxdiff < 1E-16 ) maxdiff = 0.001;

	if( self->bezier == 1 || self->convexness == 0 || self->count == 1 ) return;
	if( self->maxlen <= maxlen && self->maxdiff <= maxdiff ) return;
	if( self->bezier == 2 ){
		DaoxPolygonArray_PushPolygon( fills );
		if( self->convexness > 0 ){
			DaoxPolygonArray_PushPoint( fills, self->first->P2 );
			DaoxPolygonArray_PushPoint( fills, self->P1 );
			DaoxPolygonArray_PushPoint( fills, self->P2 );
		}else{
			DaoxPolygonArray_PushPoint( fills, self->C1 );
			DaoxPolygonArray_PushPoint( fills, self->first->C1 );
			DaoxPolygonArray_PushPoint( fills, self->second->C1 );
		}
	}else{
		DaoxPolygonArray_PushPolygon( fills );
		if( self->convexness > 0 ){
			DaoxPolygonArray_PushPoint( fills, self->first->P2 );
			DaoxPolygonArray_PushPoint( fills, self->P1 );
			DaoxPolygonArray_PushPoint( fills, self->P2 );
		}else{
			double at = (self->first->end - self->start) / (self->end - self->start);
			DaoxPoint Q;
			Q.x = (1.0 - at) * self->C1.x + at * self->C2.x;
			Q.y = (1.0 - at) * self->C1.y + at * self->C2.y;
			DaoxPolygonArray_PushPoint( fills, self->C1 );
			DaoxPolygonArray_PushPoint( fills, self->first->C1 );
			DaoxPolygonArray_PushPoint( fills, Q );
			DaoxPolygonArray_PushPolygon( fills );
			DaoxPolygonArray_PushPoint( fills, self->C1 );
			DaoxPolygonArray_PushPoint( fills, Q );
			DaoxPolygonArray_PushPoint( fills, self->second->C2 );
			DaoxPolygonArray_PushPolygon( fills );
			DaoxPolygonArray_PushPoint( fills, Q );
			DaoxPolygonArray_PushPoint( fills, self->first->C2 );
			DaoxPolygonArray_PushPoint( fills, self->second->C1 );
		}
	}
	DaoxPathSegment_GetRefined( self->first, gdata );
	DaoxPathSegment_GetRefined( self->second, gdata );
}
void DaoxPath_ExportGraphicsData( DaoxPath *self, DaoxGraphicsData *gdata )
{
	DaoxPoint *points;
	DaoxPathSegment *seg;
	DaoxPathComponent *com;
	DaoxPolygonArray *fills = gdata->fillPolygons;
	double *transform = gdata->transform;
	double maxlen = gdata->maxlen;
	double maxdiff = gdata->maxdiff;
	double A = gdata->transform[0];
	double B = gdata->transform[1];
	double C = gdata->transform[2];
	double D = gdata->transform[3];
	int fillPointCount = fills->points->count;
	int i, j, count;

	if( maxlen < 1E-16 ) maxlen = 10;
	if( maxdiff < 1E-16 ) maxdiff = 0.001;

	DaoxPath_Refine( self, maxlen, maxdiff );
	for(i=0; i<self->triangles->count; i+=3){
		DaoxPoint *points = self->points->points;
		int *ids = self->triangles->values + i;
		DaoxPolygonArray_PushPolygon( fills );
		for(j=0; j<3; ++j) DaoxPolygonArray_PushPoint( fills, points[ids[j]] );
	}
	for(com=self->first; com; com=com->next){
		if( com->first->bezier == 0 ) continue;
		if( com->refined.last == NULL || com->refined.last->next == NULL ) continue;
		//printf( "component: %p\n", com );
		seg = com->refined.first;
		do {
			//printf( "convexness: %15p %15f %i\n", seg, seg->start, seg->convexness );
			DaoxPathSegment_GetRefined( seg, gdata );
			seg = seg->next;
		} while( seg && seg != com->refined.first );
	}
	if( fabs( A*D - B*C ) < 1E-16 ) return;
	
	points = fills->points->points + fillPointCount;
	for(i=0,count=fills->points->count-fillPointCount; i<count; ++i){
		DaoxPoint *point = points + i;
		double x = point->x;
		double y = point->y;
		point->x = transform[0] * x + transform[2] * y + transform[4];
		point->y = transform[1] * x + transform[3] * y + transform[5];
	}
}
