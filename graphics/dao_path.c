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
	self->count = 1;
	self->component = component;
	return self;
}
void DaoxPathSegment_Delete( DaoxPathSegment *self )
{
	if( self->first ) DaoxPathSegment_Delete( self->first );
	if( self->second ) DaoxPathSegment_Delete( self->second );
	dao_free( self );
}

DaoxPathComponent* DaoxPathComponent_New( DaoxPath *path )
{
	DaoxPathComponent* self = (DaoxPathComponent*) calloc(1,sizeof(DaoxPathComponent));
	self->first = self->last = DaoxPathSegment_New( self );
	self->maxlen = self->maxdiff = 1E16;
	self->path = path;
	return self;
}
DaoxPathSegment* DaoxPathComponent_PushSegment( DaoxPathComponent *self )
{
	DaoxPathSegment *segment = NULL;
	if( self->last && self->last->bezier == 0 ) return self->last;
	if( self->cache ){
		segment = self->cache;
		self->cache = segment->next;
		segment->bezier = 0;
		segment->count = 1;
		segment->start = 0.0;
		segment->end = 1.0;
	}else{
		segment = DaoxPathSegment_New( self );
	}
	if( self->first == NULL ){
		self->first = self->last = segment;
		return segment;
	}
	self->last->next = segment;
	self->last = segment;
	return segment;
}
void DaoxPathComponent_Reset( DaoxPathComponent *self )
{
	DaoxPathSegment *segment = self->first;
	self->maxlen = self->maxdiff = 1E16;
	do {
		segment->next = self->cache;
		self->cache = segment;
		segment = segment->next;
	} while( segment && segment != self->first );
	self->first = self->last = NULL;
	self->refined.first = self->refined.last = NULL;
	DaoxPathComponent_PushSegment( self );
}
void DaoxPathComponent_Delete( DaoxPathComponent *self )
{
	DaoxPathComponent_Reset( self );
	while( self->cache ){
		DaoxPathSegment *segment = self->cache;
		self->cache = self->cache->next;
		DaoxPathSegment_Delete( segment );
	}
	dao_free( self );
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
	DaoxPath_Reset( self );
	while( self->cache ){
		DaoxPathComponent *com = self->cache;
		self->cache = self->cache->next;
		DaoxPathComponent_Delete( com );
	}
	DaoxPointArray_Delete( self->points );
	DaoxIntArray_Delete( self->triangles );
	free( self );
}
DaoxPathComponent* DaoxPath_PushComponent( DaoxPath *self )
{
	DaoxPathComponent *com = NULL;
	if( self->last && self->last->first->bezier == 0 ) return self->last;
	if( self->cache ){
		com = self->cache;
		self->cache = com->next;
	}else{
		com = DaoxPathComponent_New( self );
	}
	if( self->first == NULL ){
		self->first = self->last = com;
		return com;
	}
	self->last->next = com;
	self->last = com;
	return com;
}
void DaoxPath_Reset( DaoxPath *self )
{
	DaoxPathComponent *com;
	for(com=self->first; com; com=com->next){
		DaoxPathComponent_Reset( com );
		com->next = self->cache;
		self->cache = com;
	}
	self->first = self->last = NULL;
	DaoxPath_PushComponent( self );
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
	self->count = 1;
	self->P2.x = self->P1.x + dx;
	self->P2.y = self->P1.y + dy;

	if( fabs( dx ) < 1E-16 ){
		angle = dy >= 0.0 ? 0.5*M_PI : 1.5*M_PI;
	}else{
		angle = atan( dy / fabs( dx ) );
		if( dx < 0.0 ) angle = M_PI - angle;
	}
	sine = R * sin( angle - 0.5 * M_PI );
	cosine = R * cos( angle - 0.5 * M_PI );
	self->C1.x = self->P1.x + CAX2 * cosine - CAY2 * sine;
	self->C1.y = self->P1.y + CAX2 * sine + CAY2 * cosine;
	self->C2.x = self->P2.x + CBX2 * cosine - CBY2 * sine;
	self->C2.y = self->P2.y + CBX2 * sine + CBY2 * cosine;
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

	assert( self->last->last->bezier >= 2 );
	control.x = 2.0 * start.x - control.x;
	control.y = 2.0 * start.y - control.y;
	segment = DaoxPathComponent_PushSegment( self->last );
	segment->bezier = 3;
	segment->P1 = start;
	segment->C1 = control;
	segment->C2.x = start.x + x + cx;
	segment->C2.y = start.y + y + cy;
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







void DaoxPathSegment_SetPoints( DaoxPathSegment *self, DaoxPoint P1, DaoxPoint P2, DaoxPoint C1, DaoxPoint C2 )
{
	self->P1 = P1;
	self->P2 = P2;
	self->C1 = C1;
	self->C2 = C2;
	self->count = 1;
	self->start = 0.0;
	self->end = 1.0;
	self->length = 0.0;
}
void DaoxPathSegment_InitSubSegments( DaoxPathSegment *self )
{
	if( self->first == NULL ) self->first = DaoxPathSegment_New( self->component );
	if( self->second == NULL ) self->second = DaoxPathSegment_New( self->component );
	self->first->bezier = self->second->bezier = self->bezier;
	self->first->convexness = self->second->convexness = self->convexness;
	self->first->count = self->second->count = 1;
	self->count = 2;

	self->first->P1 = self->P1;
	self->second->P2 = self->P2;

	self->first->start = self->start;
	self->second->end = self->end;
}
void DaoxPathSegment_DivideLinear( DaoxPathSegment *self, double at )
{
	DaoxPathSegment_InitSubSegments( self );

	self->first->P2.x = (1.0 - at)*self->P1.x + at*self->P2.x;
	self->first->P2.y = (1.0 - at)*self->P1.y + at*self->P2.y;
	self->second->P1 = self->first->P2;

	self->first->end = self->second->start = (1.0 - at)*self->start + at*self->end;
}
void DaoxPathSegment_DivideQuadratic( DaoxPathSegment *self, double at )
{
	DaoxPoint Q1;

	DaoxPathSegment_InitSubSegments( self );

	self->first->C1.x = (1.0 - at)*self->P1.x + at*self->C1.x;
	self->first->C1.y = (1.0 - at)*self->P1.y + at*self->C1.y;

	self->second->C1.x = (1.0 - at)*self->C1.x + at*self->P2.x;
	self->second->C1.y = (1.0 - at)*self->C1.y + at*self->P2.y;

	Q1.x = (1.0 - at)*self->first->C1.x + at*self->second->C1.x;
	Q1.y = (1.0 - at)*self->first->C1.y + at*self->second->C1.y;
	self->first->P2 = Q1;
	self->second->P1 = Q1;

	self->first->end = self->second->start = (1.0 - at)*self->start + at*self->end;
}
void DaoxPathSegment_DivideCubic( DaoxPathSegment *self, double at )
{
	DaoxPoint Q1;

	DaoxPathSegment_InitSubSegments( self );

	Q1.x = (1.0 - at)*self->C1.x + at*self->C2.x;
	Q1.y = (1.0 - at)*self->C1.y + at*self->C2.y;

	self->first->C1.x = (1.0 - at)*self->P1.x + at*self->C1.x;
	self->first->C1.y = (1.0 - at)*self->P1.y + at*self->C1.y;
	self->first->C2.x = (1.0 - at)*self->first->C1.x + at*Q1.x;
	self->first->C2.y = (1.0 - at)*self->first->C1.y + at*Q1.y;

	self->second->C2.x = (1.0 - at)*self->C2.x + at*self->P2.x;
	self->second->C2.y = (1.0 - at)*self->C2.y + at*self->P2.y;
	self->second->C1.x = (1.0 - at)*Q1.x + at*self->second->C2.x;
	self->second->C1.y = (1.0 - at)*Q1.y + at*self->second->C2.y;

	Q1.x = (1.0 - at)*self->first->C2.x + at*self->second->C1.x;
	Q1.y = (1.0 - at)*self->first->C2.y + at*self->second->C1.y;
	self->first->P2 = Q1;
	self->second->P1 = Q1;

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


int DaoxPathSegment_TryDivideQuadratic( DaoxPathSegment *self, double maxlen, double maxdiff )
{
	double PC = DaoxDistance( self->P1, self->C1 );
	double CP = DaoxDistance( self->C1, self->P2 );
	double PP = DaoxDistance( self->P1, self->P2 );

	self->length = 0.5 * (PC + CP + PP);
	self->delta = (PC + CP - PP) / PP;
	if( self->length < maxlen && (self->delta < maxdiff || self->length < 0.001*maxlen) ){
		self->count = 1;
		if( self->first ) self->first->count = 0;
		if( self->second ) self->second->count = 0;
		if( self->length > self->component->maxlen ) self->component->maxlen = self->length;
		if( self->delta > self->component->maxdiff ) self->component->maxdiff = self->delta;
		return 0;
	}
	DaoxPathSegment_DivideQuadratic( self, PC / (PC + CP) );
	return 1;
}
int DaoxPathSegment_TryDivideCubic( DaoxPathSegment *self, double maxlen, double maxdiff )
{
	double PC = DaoxDistance( self->P1, self->C1 );
	double CC = DaoxDistance( self->C1, self->C2 );
	double CP = DaoxDistance( self->C2, self->P2 );
	double PP = DaoxDistance( self->P1, self->P2 );

	self->length = 0.5 * (PC + CC + CP + PP);
	self->delta = (PC + CC + CP - PP) / PP;
	if( self->length < maxlen && (self->delta < maxdiff || self->length < 0.001*maxlen) ){
		self->count = 1;
		if( self->first ) self->first->count = 0;
		if( self->second ) self->second->count = 0;
		if( self->length > self->component->maxlen ) self->component->maxlen = self->length;
		if( self->delta > self->component->maxdiff ) self->component->maxdiff = self->delta;
		return 0;
	}
	DaoxPathSegment_DivideCubic( self, PC / (PC + CP) );
	return 1;
}
void DaoxPathSegment_RefineLinear( DaoxPathSegment *self, double maxlen, double maxdiff )
{
	if( self->count > 1 ){
		DaoxPathSegment_RefineLinear( self->first, maxlen, maxdiff );
		DaoxPathSegment_RefineLinear( self->second, maxlen, maxdiff );
		self->count = self->first->count + self->second->count;
		return;
	}
	self->length = DaoxDistance( self->P1, self->P2 );
}
void DaoxPathSegment_RefineQuadratic( DaoxPathSegment *self, double maxlen, double maxdiff )
{
	if( self->count > 1 ){
		DaoxPathSegment_RefineQuadratic( self->first, maxlen, maxdiff );
		DaoxPathSegment_RefineQuadratic( self->second, maxlen, maxdiff );
		self->count = self->first->count + self->second->count;
		self->length = self->first->length + self->second->length;
		return;
	}
	if( DaoxPathSegment_TryDivideQuadratic( self, maxlen, maxdiff ) == 0 ) return;

	DaoxPathSegment_RefineQuadratic( self->first, maxlen, maxdiff );
	DaoxPathSegment_RefineQuadratic( self->second, maxlen, maxdiff );
	self->count = self->first->count + self->second->count;
	self->length = self->first->length + self->second->length;
}
void DaoxPathSegment_RefineCubic( DaoxPathSegment *self, double maxlen, double maxdiff )
{
	if( self->count > 1 ){
		DaoxPathSegment_RefineCubic( self->first, maxlen, maxdiff );
		DaoxPathSegment_RefineCubic( self->second, maxlen, maxdiff );
		self->count = self->first->count + self->second->count;
		self->length = self->first->length + self->second->length;
		return;
	}
	if( DaoxPathSegment_TryDivideCubic( self, maxlen, maxdiff ) == 0 ) return;

	DaoxPathSegment_RefineCubic( self->first, maxlen, maxdiff );
	DaoxPathSegment_RefineCubic( self->second, maxlen, maxdiff );
	self->count = self->first->count + self->second->count;
	self->length = self->first->length + self->second->length;
}

void DaoxPathSegment_Refine( DaoxPathSegment *self, double maxlen, double maxdiff )
{
	if( self->bezier == 1 ){
		DaoxPathSegment_RefineLinear( self, maxlen, maxdiff );
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
	self->length = 0.0;
	do {
		DaoxPathSegment_Refine( segment, maxlen, maxdiff );
		if( segment->count == 1 ) DaoxPathSegment_Divide( segment, 0.5 );
		printf( "%15f\n", segment->length );
		self->length += segment->length;
		segment = segment->next;
	} while( segment && segment != first );
}
double DaoxPathComponent_MaxLineLength( DaoxPathComponent *self )
{
	double len, max = 0.0;
	DaoxPathSegment *first = self->first;
	DaoxPathSegment *segment = first;
	do {
		len = segment->length;
		if( len > max ) max = len;
		segment = segment->next;
	} while( segment && segment != first );
	return max;
}
void DaoxPathComponent_RetrieveSegment( DaoxPathComponent *self, DaoxPathSegment *segment )
{
	printf( "%3i %3i\n", segment->bezier, segment->count );
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

DaoxPoint DaoxPoint_Transform( DaoxPoint self, DaoxTransform *transform )
{
	DaoxPoint point;
	if( transform == NULL ) return self;
	point.x = transform->Axx * self.x + transform->Axy * self.y + transform->Bx;
	point.y = transform->Ayx * self.x + transform->Ayy * self.y + transform->By;
	return point;
}

void DaoxPath_ImportPath( DaoxPath *self, DaoxPath *path, DaoxTransform *transform )
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
	printf( "DaoxPath_Refine: %15f %15f %i\n", maxlen, maxdiff, self->first->first->bezier );
	self->length = 0.0;
	for(com=self->first; com; com=com->next){
		if( com->first->bezier == 0 ) continue;
		if( maxlen > com->maxlen && maxdiff > com->maxdiff ){
			self->length += com->length;
			continue;
		}
		com->maxlen = com->maxdiff = 0.0;
		DaoxPathComponent_Refine( com, maxlen, maxdiff );
		self->length += com->length;
	}
}

void DaoxPath_Preprocess( DaoxPath *self, DaoxPathBuffer *buffer )
{
	DArray *segments = DArray_New(0);
	DaoxTriangulator *triangulator = buffer->triangulator;
	DaoxPathComponent *com;
	DaoxPathSegment *seg;
	daoint i, count = 0;
	double len, maxlen = 0.0;
	double maxdiff = 0.1;

	printf( "DaoxPath_Segment 1: %p\n", triangulator );
	DaoxTriangulator_Reset( triangulator );
	DaoxPath_Refine( self, 1E16, 1 ); /* setup length; */
	for(com=self->first; com; com=com->next){
		len = DaoxPathComponent_MaxLineLength( com );
		if( len > maxlen ) maxlen = len;
	}
	printf( "maxlen = %f\n", maxlen );
	maxlen *= 0.5;
	DaoxPath_Refine( self, maxlen, maxdiff );
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
	/* Check local convexness with triangulation to handle possible presence of holes: */
	for(i=0; i<triangulator->triangles->size; i+=3){
		int C = triangulator->triangles->items.pInt[i];
		int A = triangulator->triangles->items.pInt[i+1];
		int B = triangulator->triangles->items.pInt[i+2];
		DaoxPathSegment *SC = (DaoxPathSegment*) segments->items.pVoid[C];
		DaoxPathSegment *SA = (DaoxPathSegment*) segments->items.pVoid[A];
		DaoxPathSegment *SB = (DaoxPathSegment*) segments->items.pVoid[B];
		if( SC->next == SA ) DaoxPathSegment_CheckConvexness( SC, SB->P1 );
		if( SA->next == SB ) DaoxPathSegment_CheckConvexness( SA, SC->P1 );
		if( SB->next == SC ) DaoxPathSegment_CheckConvexness( SB, SA->P1 );
	}
	for(com=self->first; com; com=com->next){
		if( com->first->bezier == 0 ) continue;
		if( com->refined.last == NULL || com->refined.last->next == NULL ) continue;
		seg = com->refined.first;
		do {
			DaoxPathSegment_Refine( seg, 0.25*maxlen, 0.25*maxdiff );
			seg = seg->next;
		} while( seg && seg != com->refined.first );
		DaoxPathComponent_RetrieveRefined( com );
	}
	DaoxTriangulator_Reset( triangulator );
	for(com=self->first; com; com=com->next){
		if( com->first->bezier == 0 ) continue;
		if( com->refined.last == NULL || com->refined.last->next == NULL ) continue;
		seg = com->refined.first;
		do {
			DaoxTriangulator_PushPoint( triangulator, seg->P1.x, seg->P1.y );
			if( seg->convexness <= 0 && seg->bezier >= 2 ){
				/* Push control points for locally concave bezier curves: */
				DaoxTriangulator_PushPoint( triangulator, seg->C1.x, seg->C1.y );
				if( seg->bezier == 3 ){
					DaoxTriangulator_PushPoint( triangulator, seg->C2.x, seg->C2.y );
				}
			}
			seg = seg->next;
		} while( seg && seg != com->refined.first );
		DaoxTriangulator_CloseContour( triangulator );
	}
	DaoxTriangulator_Triangulate( triangulator );
	self->points->count = 0;
	DaoxPointArray_PushPoints( self->points, triangulator->points );
	for(i=0; i<triangulator->triangles->size; i+=1){
		int C = triangulator->triangles->items.pInt[i];
		DaoxIntArray_Push( self->triangles, C );
	}
	
	DArray_Delete( segments );
}







void DaoxGraphicsData_PushFilling( DaoxGraphicsData *self, DaoxPoint A, DaoxPoint B, DaoxPoint C )
{
	int index = self->fillPoints->count;
	A = DaoxPoint_Transform( A, self->transform );
	B = DaoxPoint_Transform( B, self->transform );
	C = DaoxPoint_Transform( C, self->transform );
	DaoxPointArray_Push( self->fillPoints, A );
	DaoxPointArray_Push( self->fillPoints, B );
	DaoxPointArray_Push( self->fillPoints, C );
	DaoxIntArray_Push( self->fillTriangles, index );
	DaoxIntArray_Push( self->fillTriangles, index+1 );
	DaoxIntArray_Push( self->fillTriangles, index+2 );
}
void DaoxGraphicsData_PushStrokeColor( DaoxGraphicsData *self, DaoxColor color, int times )
{
	while( (times--) > 0 ) DaoxColorArray_Push( self->strokeColors, color );
}
void DaoxGraphicsData_MakeJunction( DaoxGraphicsData *self, DaoxPoint A, DaoxPoint B, DaoxPoint C, double pos, int junction )
{
	DaoxColor color = {0.0};
	DaoxGraphicsData round = *self;
	DaoxTransform transform = {0.0,0.0,0.0,0.0,0.0,0.0};
	DaoxPoint P1 = DaoxPoint_Transform( A, self->transform );
	DaoxPoint P2 = DaoxPoint_Transform( B, self->transform );
	DaoxPoint P3 = DaoxPoint_Transform( C, self->transform );
	DaoxQuad Q1 = DaoxLine2Quad( P1, P2, self->strokeWidth );
	DaoxQuad Q2 = DaoxLine2Quad( P2, P3, self->strokeWidth );
	DaoxColorGradient *strokeGradient = self->item->state->strokeGradient;
	int hasGradient = strokeGradient != NULL && strokeGradient->stops->count;
	double scale, dist, cosine, sine, angle, W2 = 0.5 * self->strokeWidth;
	int m, k = DaoxLineQuad_Junction( Q1, Q2, & P3 );

	if( self->strokeWidth < 1E-6 ) return;

	if( hasGradient ) color = DaoxColorGradient_InterpolateColor( strokeGradient, pos );

	if( k > 0 ){
		if( junction != DAOX_JUNCTION_ROUND ){
			DaoxGraphicsData_PushStrokeTriangle( self, Q1.D, Q2.A, P2 );
			if( hasGradient ) DaoxGraphicsData_PushStrokeColor( self, color, 3 );
		}
		if( junction == DAOX_JUNCTION_SHARP ){
			DaoxGraphicsData_PushStrokeTriangle( self, Q1.D, Q2.A, P3 );
			if( hasGradient ) DaoxGraphicsData_PushStrokeColor( self, color, 3 );
		}
	}else{
		if( junction != DAOX_JUNCTION_ROUND ){
			DaoxGraphicsData_PushStrokeTriangle( self, Q1.C, Q2.B, P2 );
			if( hasGradient ) DaoxGraphicsData_PushStrokeColor( self, color, 3 );
		}
		if( junction == DAOX_JUNCTION_SHARP ){
			DaoxGraphicsData_PushStrokeTriangle( self, Q1.C, Q2.B, P3 );
			if( hasGradient ) DaoxGraphicsData_PushStrokeColor( self, color, 3 );
		}
	}
	if( self->item->state->dash ){
		if( hasGradient ) DaoxGraphicsData_PushStrokeColor( self, color, 3 );
		if( k > 0 ){
			DaoxGraphicsData_PushStrokeTriangle( self, P2, Q1.C, Q2.B );
		}else{
			DaoxGraphicsData_PushStrokeTriangle( self, P2, Q1.D, Q2.A );
		}

	}
	if( junction != DAOX_JUNCTION_ROUND ) return;

	scale = 0.5 * self->strokeWidth / 100.0;
	dist = DaoxDistance( P2, P3 );
	cosine = (P3.x - P2.x) / dist;
	sine = (P3.y - P2.y) / dist;
	dist = DaoxDistance( Q1.D, Q2.A );
	angle = acos( 1.0 - dist*dist / (2.0*W2*W2) );

	m = self->strokePoints->count;
	round.strokeWidth = 0.0;
	round.junction = DAOX_JUNCTION_FLAT;
	round.strokePoints = self->strokePoints;
	round.fillPoints = self->strokePoints;
	round.strokeTriangles = self->strokeTriangles;
	round.fillTriangles = self->strokeTriangles;
	round.transform = & transform;
	transform.Axx = scale * cosine;
	transform.Axy = - scale * sine;
	transform.Ayx = + scale * sine;
	transform.Ayy = scale * cosine;
	transform.Bx = P2.x;
	transform.By = P2.y;
	k = DAOX_ARCS * angle / M_PI;
	DaoxPath_ExportGraphicsData( self->item->scene->largeArcs[k], & round );
	if( hasGradient ) DaoxGraphicsData_PushStrokeColor( self, color, self->strokePoints->count - m );
}


typedef struct DaoxPathSegmentPair  DaoxPathSegmentPair;
struct DaoxPathSegmentPair
{
	DaoxPathSegment  *first;
	DaoxPathSegment  *second;
};

void DaoxGraphicsData_UpdateDashState( DaoxGraphicsData *self, double len )
{
	if( self->item->state->dash == 0 ) return;
	self->dashLength -= len;
	if( self->dashLength <= 1E-16 ){
		self->dashState = (self->dashState + 1) % self->item->state->dash;
		self->dashLength = self->item->state->dashPattern[ self->dashState ];
	}
}

void DaoxGraphicsData_PushStrokeQuadColors( DaoxGraphicsData *self, double offset, double len )
{
	DaoxColorGradient *strokeGradient = self->item->state->strokeGradient;
	DaoxColor color;
	double pos;
	pos = offset / self->currentLength;
	color = DaoxColorGradient_InterpolateColor( strokeGradient, pos );
	DaoxGraphicsData_PushStrokeColor( self, color, 2 );
	pos = (offset + len) / self->currentLength;
	color = DaoxColorGradient_InterpolateColor( strokeGradient, pos );
	DaoxGraphicsData_PushStrokeColor( self, color, 2 );
}

void DaoxPathSegment_GetRefinedStroke( DaoxPathSegment *self, DaoxGraphicsData *gdata )
{
	DaoxQuad quad;
	DaoxPoint PM;
	DaoxColor color;
	DaoxPoint P1 = DaoxPoint_Transform( self->P1, gdata->transform );
	DaoxPoint P2 = DaoxPoint_Transform( self->P2, gdata->transform );
	DaoxColorGradient *strokeGradient = gdata->item->state->strokeGradient;
	int hasGradient = strokeGradient != NULL && strokeGradient->stops->count;
	double at, pos, len, width = gdata->strokeWidth;
	double offset = gdata->currentOffset;

	gdata->currentOffset += self->length;

	if( width < 1E-6 ) return;


	if( gdata->item->state->dash == 0 ){
		quad = DaoxLine2Quad( P1, P2, width );
		DaoxGraphicsData_PushStrokeQuad( gdata, quad );
		if( hasGradient ) DaoxGraphicsData_PushStrokeQuadColors( gdata, offset, self->length );
		return;
	}
	PM = DaoxPoint_Transform( self->C1, gdata->transform );
	len = DaoxDistance( P1, P2 );
	while( len > 0.0 ){
		if( len < gdata->dashLength ){
			if( (gdata->dashState&1) == 0 ){
				quad = DaoxLine2Quad( P1, P2, width );
				DaoxGraphicsData_PushStrokeQuad( gdata, quad );
				if( hasGradient ) DaoxGraphicsData_PushStrokeQuadColors( gdata, offset, len );
			}
			DaoxGraphicsData_UpdateDashState( gdata, len );
			return;
		}
		at = gdata->dashLength / len;
		PM.x = (1.0 - at) * P1.x + at * P2.x;
		PM.y = (1.0 - at) * P1.y + at * P2.y;
		if( (gdata->dashState&1) == 0 ){
			quad = DaoxLine2Quad( P1, PM, width );
			DaoxGraphicsData_PushStrokeQuad( gdata, quad );
			if( hasGradient ) DaoxGraphicsData_PushStrokeQuadColors( gdata, offset, gdata->dashLength );
		}
		len -= gdata->dashLength;
		offset += gdata->dashLength;
		DaoxGraphicsData_UpdateDashState( gdata, gdata->dashLength );
		P1 = PM;
	}
}


DaoxPathSegmentPair DaoxPathSegment_GetRefined( DaoxPathSegment *self, DaoxGraphicsData *gdata )
{
	DaoxPoint P1, P2, P3;
	DaoxQuad quad, prev, next;
	DaoxPathSegmentPair sp1, sp2, segs = {NULL, NULL};
	DaoxColorGradient *strokeGradient = gdata->item->state->strokeGradient;
	int hasGradient = strokeGradient != NULL && strokeGradient->stops->count;
	double width = gdata->strokeWidth;
	double maxlen = gdata->maxlen;
	double maxdiff = gdata->maxdiff;
	double offset, nojunction = 0;

	if( maxlen < 1E-16 ) maxlen = 10;
	if( maxdiff < 1E-16 ) maxdiff = 0.001;

	segs.first = segs.second = self;

	if( self->bezier == 1 || self->convexness == 0 || self->count == 1 ){
		DaoxPathSegment_GetRefinedStroke( self, gdata );
		return segs;
	}
	if( self->length <= maxlen && self->delta <= maxdiff ){
		DaoxPathSegment_GetRefinedStroke( self, gdata );
		return segs;
	}
	if( self->bezier == 2 ){
		if( self->convexness > 0 ){
			DaoxGraphicsData_PushFilling( gdata, self->first->P2, self->P1, self->P2 );
		}else{
			DaoxGraphicsData_PushFilling( gdata, self->C1, self->first->C1, self->second->C1 );
		}
	}else{
		if( self->convexness > 0 ){
			DaoxGraphicsData_PushFilling( gdata, self->first->P2, self->P1, self->P2 );
		}else{
			double at = (self->first->end - self->start) / (self->end - self->start);
			DaoxPoint Q;
			Q.x = (1.0 - at) * self->C1.x + at * self->C2.x;
			Q.y = (1.0 - at) * self->C1.y + at * self->C2.y;
			DaoxGraphicsData_PushFilling( gdata, self->C1, self->first->C1, Q );
			DaoxGraphicsData_PushFilling( gdata, self->C2, Q, self->second->C2 );
			DaoxGraphicsData_PushFilling( gdata, Q, self->first->C2, self->second->C1 );
		}
	}
	sp1 = DaoxPathSegment_GetRefined( self->first, gdata );
	offset = gdata->currentOffset;
	if( gdata->item->state->dash && (gdata->dashState & 1) ) nojunction = 1;
	sp2 = DaoxPathSegment_GetRefined( self->second, gdata );
	segs.first = sp1.first;
	segs.second = sp2.second;

	if( width < 1E-6 || nojunction ) return segs;

	P1 = DaoxPoint_Transform( sp1.second->P1, gdata->transform );
	P2 = DaoxPoint_Transform( sp1.second->P2, gdata->transform );
	P3 = DaoxPoint_Transform( sp2.first->P2, gdata->transform );
	prev = DaoxLine2Quad( P1, P2, width );
	next = DaoxLine2Quad( P2, P3, width );
	if( DaoxLineQuad_Junction( prev, next, NULL ) > 0 ){
		DaoxGraphicsData_PushStrokeTriangle( gdata, prev.D, next.A, P2 );
		if( gdata->item->state->dash ){
			DaoxGraphicsData_PushStrokeTriangle( gdata, P2, prev.C, next.B );
		}
	}else{
		DaoxGraphicsData_PushStrokeTriangle( gdata, prev.C, next.B, P2 );
		if( gdata->item->state->dash ){
			DaoxGraphicsData_PushStrokeTriangle( gdata, P2, prev.D, next.A );
		}
	}
	if( hasGradient ){
		int i = gdata->item->state->dash ? 6 : 3;
		double pos = offset / gdata->currentLength;
		DaoxColor color = DaoxColorGradient_InterpolateColor( strokeGradient, pos );
		while( (i--) > 0 ) DaoxColorArray_Push( gdata->strokeColors, color );
	}
	return segs;
}
void DaoxPath_ExportGraphicsData( DaoxPath *self, DaoxGraphicsData *gdata )
{
	DaoxPoint *points;
	DaoxPathSegment *seg;
	DaoxPathComponent *com;
	DaoxPathSegmentPair open, cur, prev = {NULL,NULL};
	DaoxTransform *transform = gdata->transform;
	double width = gdata->strokeWidth;
	double maxlen = gdata->maxlen;
	double maxdiff = gdata->maxdiff;
	double pos;
	int i, j, count, jt, jt2;

	if( maxlen < 1E-16 ) maxlen = 10;
	if( maxdiff < 1E-16 ) maxdiff = 0.001;

	DaoxPath_Refine( self, maxlen, maxdiff );
	gdata->currentLength = self->length;
	gdata->currentOffset = 0.0;
	printf( "gdata->currentLength = %15f\n", self->length );
	if( 1 ){
		DaoxPoint *points = self->points->points;
		int *ids = self->triangles->values;
		int offset = gdata->fillPoints->count;
		for(i=0; i<self->points->count; i++){
			DaoxPoint point = DaoxPoint_Transform( points[i], gdata->transform );
			DaoxPointArray_Push( gdata->fillPoints, point );
		}
		for(i=0; i<self->triangles->count; i++){
			DaoxIntArray_Push( gdata->fillTriangles, ids[i] + offset );
		}
	}
	if( width < 1E-16 ) return;
	for(com=self->first; com; com=com->next){
		if( com->first->bezier == 0 ) continue;
		if( com->refined.last == NULL || com->refined.last->next == NULL ) continue;
		//printf( "component: %p\n", com );
		gdata->dashState = 0;
		gdata->dashLength = gdata->item->state->dashPattern[0];
		prev.first = prev.second = NULL;
		seg = com->refined.first;
		pos = 0.0;
		do {
			//printf( "convexness: %15p %15f %i\n", seg, seg->start, seg->convexness );
			jt2 = gdata->item->state->dash == 0 || (gdata->dashState & 1) == 0;
			cur = DaoxPathSegment_GetRefined( seg, gdata );
			if( prev.first != NULL && width > 1E-6 ){
				int userjunction = prev.second->end > 0.8 && cur.first->start < 0.2;
				jt = userjunction ? gdata->junction : DAOX_JUNCTION_FLAT;
				if( jt && jt2 ) DaoxGraphicsData_MakeJunction( gdata, prev.second->P1, prev.second->P2, cur.first->P2, pos, jt );
			}else if( prev.first == NULL ){
				open = cur;
			}
			pos = gdata->currentOffset / gdata->currentLength;
			prev = cur;
			seg = seg->next;
		} while( seg && seg != com->refined.first );

		if( open.first != NULL && cur.first != NULL && width > 1E-6 ){
			int userjunction = cur.second->end > 0.8 && open.first->start < 0.2;
			jt2 = gdata->item->state->dash == 0 || (gdata->dashState & 1) == 0;
			jt = userjunction ? gdata->junction : DAOX_JUNCTION_FLAT;
			if( jt && jt2 ) DaoxGraphicsData_MakeJunction( gdata, cur.second->P1, cur.second->P2, open.first->P2, 1.0, jt );
		}
	}
}
