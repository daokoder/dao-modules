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
	self->convexness = 1; /* need for refinement of open path; */
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
	self->cmdRelative = 0;
	self->first = self->last = NULL;
	DaoxPath_PushComponent( self );
}
void DaoxPath_SetRelativeMode( DaoxPath *self, int relative )
{
	self->cmdRelative = relative;
}
void DaoxPath_MoveTo( DaoxPath *self, float x, float y )
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
void DaoxPath_LineTo( DaoxPath *self, float x, float y )
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
	if( self->cmdRelative ){
		x += start.x;
		y += start.y;
	}
	segment->bezier = 1;
	segment->P2.x = x;
	segment->P2.y = y;
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
void DaoxPathSegment_MakeArc( DaoxPathSegment *self, float dx, float dy, float R, float degree )
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
	if( degree > 0.0 ){
		sine = R * sin( angle - 0.5 * M_PI );
		cosine = R * cos( angle - 0.5 * M_PI );
	}else{
		sine = R * sin( angle + 0.5 * M_PI );
		cosine = R * cos( angle + 0.5 * M_PI );
	}
	self->C1.x = self->P1.x + CAX2 * cosine - CAY2 * sine;
	self->C1.y = self->P1.y + CAX2 * sine + CAY2 * cosine;
	self->C2.x = self->P2.x + CBX2 * cosine - CBY2 * sine;
	self->C2.y = self->P2.y + CBX2 * sine + CBY2 * cosine;
}
void DaoxPath_ArcBy2( DaoxPath *self, float cx, float cy, float degrees, float deg2 )
{
	DaoxPoint point, start, next, end, center; /* A: start; B: end; C: center; */
	DaoxPathSegment *segment = NULL;
	double degrees2 = M_PI * degrees / 180.0;
	double dx, dy, R, dA, sine, cosine, dL, dR;
	int i, K;

	if( self->last->last->bezier == 0 ){
		start = self->last->last->P1;
		segment = self->last->last;
	}else{
		start = self->last->last->P2;
		segment = DaoxPathComponent_PushSegment( self->last );
		segment->P1 = start;
	}
	if( self->cmdRelative == 0 ){
		cx -= start.x;
		cy -= start.y;
	}

	R = sqrt( cx * cx + cy * cy );
	if( fabs( degrees ) < deg2 ){
		cosine = cos( degrees2 );
		sine = sin( degrees2 );
		dx = - sine * (-cy);
		dy = cosine * (-cx);
		DaoxPathSegment_MakeArc( segment, dx, dy, R, degrees2 );
		return;
	}
	center.x = start.x + cx;
	center.y = start.y + cy;

	printf( "degrees = %15f:  %15f  %15f\n", degrees, start.x, start.y );
	printf( "degrees = %15f:  %15f  %15f\n", degrees, center.x, center.y );

	/* Make start relative to the center: */
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
		printf( "%3i: %15f\n", i, (i + 1) * dA );
		sine = sin( (i + 1) * dA );
		cosine = cos( (i + 1) * dA );
		next.x = start.x * cosine - start.y * sine;
		next.y = start.x * sine + start.y * cosine;
		DaoxPathSegment_MakeArc( segment, next.x - point.x, next.y - point.y, R, dA );
		point = next;
		segment = NULL;
	}
}
void DaoxPath_ArcTo2( DaoxPath *self, float x, float y, float degrees, float deg2 )
{
	DaoxPoint start;
	double degrees2 = M_PI * degrees / 180.0;
	double cx, cy, dx, dy, R, dR, dL;

	if( self->last->last->bezier == 0 ){
		start = self->last->last->P1;
	}else{
		start = self->last->last->P2;
	}
	if( self->cmdRelative == 0 ){
		x -= start.x;
		y -= start.y;
	}

	cx = 0.5 * x;
	cy = 0.5 * y;
	dx = - cx;
	dy = - cy;
	dL = x * x + y * y;
	R = 0.5 * dL / (1.0 - cos(degrees2) );
	dR = sqrt( R - 0.25 * dL );
	R = sqrt( R );
	dL = 0.5 * sqrt( dL );
	if( degrees > 180.0 ){
		cx += - dR * dy / dL;
		cy += + dR * dx / dL;
	}else if( degrees < - 180.0 ){
		cx += + dR * dy / dL;
		cy += - dR * dx / dL;
	}else if( degrees > 0.0 ){
		cx += + dR * dy / dL;
		cy += - dR * dx / dL;
	}else{
		cx += - dR * dy / dL;
		cy += + dR * dx / dL;
	}
	if( self->cmdRelative == 0 ){
		cx += start.x;
		cy += start.y;
	}

	DaoxPath_ArcBy2( self, cx, cy, degrees, deg2 );
}
void DaoxPath_ArcTo( DaoxPath *self, float x, float y, float degrees )
{
	DaoxPath_ArcTo2( self, x, y, degrees, 30.0 );
}
void DaoxPath_ArcBy( DaoxPath *self, float cx, float cy, float degrees )
{
	DaoxPath_ArcBy2( self, cx, cy, degrees, 30.0 );
}
void DaoxPath_QuadTo( DaoxPath *self, float cx, float cy, float x, float y )
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
	if( self->cmdRelative ){
		cx += start.x;
		cy += start.y;
		x += start.x;
		y += start.y;
	}
	segment->bezier = 2;
	segment->C1.x = cx;
	segment->C1.y = cy;
	segment->P2.x = x;
	segment->P2.y = y;
	segment->C2 = segment->C1;
}
void DaoxPath_CubicTo( DaoxPath *self, float cx, float cy, float x, float y )
{
	DaoxPathSegment *segment = NULL;
	DaoxPoint control = self->last->last->C2;
	DaoxPoint start = self->last->last->P2;

	assert( self->last->last->bezier >= 2 );
	control.x = 2.0 * start.x - control.x;
	control.y = 2.0 * start.y - control.y;
	if( self->cmdRelative ){
		cx += start.x + x;
		cy += start.y + y;
		x += start.x;
		y += start.y;
	}
	segment = DaoxPathComponent_PushSegment( self->last );
	segment->bezier = 3;
	segment->P1 = start;
	segment->C1 = control;
	segment->C2.x = cx;
	segment->C2.y = cy;
	segment->P2.x = x;
	segment->P2.y = y;
}
void DaoxPath_CubicTo2( DaoxPath *self, float cx1, float cy1, float cx2, float cy2, float x2, float y2 )
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
	if( self->cmdRelative ){
		cx1 += start.x;
		cy1 += start.y;
		cx2 += start.x + x2;
		cy2 += start.y + y2;
		x2 += start.x;
		y2 += start.y;
	}
	segment->bezier = 3;
	segment->C1.x = cx1;
	segment->C1.y = cy1;
	segment->C2.x = cx2;
	segment->C2.y = cy2;
	segment->P2.x = x2;
	segment->P2.y = y2;
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
void DaoxPathSegment_DivideLinear( DaoxPathSegment *self, float at )
{
	DaoxPathSegment_InitSubSegments( self );

	self->first->P2.x = (1.0 - at)*self->P1.x + at*self->P2.x;
	self->first->P2.y = (1.0 - at)*self->P1.y + at*self->P2.y;
	self->second->P1 = self->first->P2;

	self->first->end = self->second->start = (1.0 - at)*self->start + at*self->end;
}
void DaoxPathSegment_DivideQuadratic( DaoxPathSegment *self, float at )
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
void DaoxPathSegment_DivideCubic( DaoxPathSegment *self, float at )
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
void DaoxPathSegment_Divide( DaoxPathSegment *self, float at )
{
	if( self->bezier == 2 ){
		DaoxPathSegment_DivideQuadratic( self, at );
	}else if( self->bezier == 3 ){
		DaoxPathSegment_DivideCubic( self, at );
	}else{
		DaoxPathSegment_DivideLinear( self, at );
	}
}

void DaoxPathSegment_ComputeLengthAndDelta( DaoxPathSegment *self )
{
	float PP = DaoxDistance( self->P1, self->P2 );
	if( self->bezier >= 2 ){
		float PC = DaoxDistance( self->P1, self->C1 );
		float CP = DaoxDistance( self->bezier == 2 ? self->C1 : self->C2, self->P2 );
		if( self->bezier >= 3 ){
			float CC = DaoxDistance( self->C1, self->C2 );
			self->length = 0.5 * (PC + CC + CP + PP);
			self->delta = (PC + CC + CP - PP) / (PP + 1E-16);
		}else{
			self->length = 0.5 * (PC + CP + PP);
			self->delta = (PC + CP - PP) / (PP + 1E-16);
		}
	}else{
		self->length = PP;
		self->delta = 0.0;
	}
}

int DaoxPathSegment_TryDivideQuadratic( DaoxPathSegment *self, float maxlen, float maxdiff )
{
	float PC = DaoxDistance( self->P1, self->C1 );
	float CP = DaoxDistance( self->C1, self->P2 );
	float PP = DaoxDistance( self->P1, self->P2 );

	self->length = 0.5 * (PC + CP + PP);
	self->delta = (PC + CP - PP) / (PP + 1E-16);
	if( self->length < maxlen && (self->delta < maxdiff || self->length*self->delta < 1) ){
		self->count = 1;
		if( self->first ) self->first->count = 0;
		if( self->second ) self->second->count = 0;
		if( self->length > self->component->maxlen ) self->component->maxlen = self->length;
		if( self->delta > self->component->maxdiff ) self->component->maxdiff = self->delta;
		return 0;
	}
	/* Interpolate between 0.5 and PC/(PC+CP), in case that PC/(PC+CP) become 0 or 1: */
	DaoxPathSegment_DivideQuadratic( self, 0.1 * 0.5 + 0.9 * PC / (PC + CP) );
	return 1;
}
int DaoxPathSegment_TryDivideCubic( DaoxPathSegment *self, float maxlen, float maxdiff )
{
	float PC = DaoxDistance( self->P1, self->C1 );
	float CC = DaoxDistance( self->C1, self->C2 );
	float CP = DaoxDistance( self->C2, self->P2 );
	float PP = DaoxDistance( self->P1, self->P2 );

	self->length = 0.5 * (PC + CC + CP + PP);
	self->delta = (PC + CC + CP - PP) / (PP + 1E-16);
	if( self->length < maxlen && (self->delta < maxdiff || self->length*self->delta < 1) ){
		self->count = 1;
		if( self->first ) self->first->count = 0;
		if( self->second ) self->second->count = 0;
		if( self->length > self->component->maxlen ) self->component->maxlen = self->length;
		if( self->delta > self->component->maxdiff ) self->component->maxdiff = self->delta;
		return 0;
	}
	/* Interpolate between 0.5 and PC/(PC+CP), in case that PC/(PC+CP) become 0 or 1: */
	DaoxPathSegment_DivideCubic( self, 0.1 * 0.5 + 0.9 * PC / (PC + CP) );
	return 1;
}
void DaoxPathSegment_RefineLinear( DaoxPathSegment *self, float maxlen, float maxdiff )
{
	self->length = DaoxDistance( self->P1, self->P2 );
	if( self->count > 1 ){
		DaoxPathSegment_RefineLinear( self->first, maxlen, maxdiff );
		DaoxPathSegment_RefineLinear( self->second, maxlen, maxdiff );
		self->count = self->first->count + self->second->count;
		return;
	}
}
void DaoxPathSegment_RefineQuadratic( DaoxPathSegment *self, float maxlen, float maxdiff )
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
void DaoxPathSegment_RefineCubic( DaoxPathSegment *self, float maxlen, float maxdiff )
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

void DaoxPathSegment_Refine( DaoxPathSegment *self, float maxlen, float maxdiff )
{
	if( self->bezier == 1 ){
		DaoxPathSegment_RefineLinear( self, maxlen, maxdiff );
	}else if( self->bezier == 2 ){
		DaoxPathSegment_RefineQuadratic( self, maxlen, maxdiff );
	}else if( self->bezier == 3 ){
		DaoxPathSegment_RefineCubic( self, maxlen, maxdiff );
	}
}

void DaoxPathComponent_Refine( DaoxPathComponent *self, float maxlen, float maxdiff )
{
	DaoxPathSegment *first = self->refined.first ? self->refined.first : self->first;
	DaoxPathSegment *segment = first;
	self->length = 0.0;
	do {
		DaoxPathSegment_Refine( segment, maxlen, maxdiff );
		if( segment->count == 1 ){
			DaoxPathSegment_Divide( segment, 0.5 );
			segment->first->length = DaoxDistance( segment->first->P1, segment->first->P2 );
			segment->second->length = DaoxDistance( segment->second->P1, segment->second->P2 );
		}
		self->length += segment->length;
		segment = segment->next;
	} while( segment && segment != first );
}
float DaoxPathComponent_MaxLineLength( DaoxPathComponent *self )
{
	float len, max = 0.0;
	DaoxPathSegment *first = self->first;
	DaoxPathSegment *segment = first;
	do {
		len = segment->length;
		if( len > max ) max = len;
		segment = segment->next;
	} while( segment && segment != first );
	return max;
}


DaoxPathSegment* DaoxPathSegment_LocateByDistance( DaoxPathSegment *self, float distance, float offset, float *p )
{
	if( distance < offset || distance > (offset + self->length) ) return NULL;
	if( p ) *p = (distance - offset) / self->length;
	if( self->count <= 1 ) return self;
	if( distance <= (offset + self->first->length) ){
		return DaoxPathSegment_LocateByDistance( self->first, distance, offset, p );
	}
	return DaoxPathSegment_LocateByDistance( self->second, distance, offset + self->first->length, p );
}

DaoxPathSegment* DaoxPathComponent_LocateByDistance( DaoxPathComponent *self, float distance, float offset, float *p )
{
	DaoxPathSegment *first = self->first;
	DaoxPathSegment *segment = first;
	do {
		DaoxPathSegment *seg = DaoxPathSegment_LocateByDistance( segment, distance, offset, p );
		if( seg ) return seg;
		offset += segment->length;
		segment = segment->next;
	} while( segment && segment != first );
	return NULL;
}

DaoxPathSegment* DaoxPath_LocateByDistance( DaoxPath *self, float distance, float *p )
{
	float offset = 0.0;
	DaoxPathSegment *seg;
	DaoxPathComponent *com;
	if( distance < 0.0 ) return NULL;
	for(com=self->first; com; com=com->next){
		if( com->first->bezier == 0 ) continue;
		seg = DaoxPathComponent_LocateByDistance( com, distance, offset, p );
		if( seg ) return seg;
		offset += com->length;
	}
	return NULL;
}
DaoxPathSegment* DaoxPath_LocateByPercentage( DaoxPath *self, float percentage, float *p )
{
}


DaoxBounds DaoxPathSegment_GetBounds( DaoxPathSegment *self )
{
	DaoxBounds bounds;
	DaoxBounds_Init( & bounds, self->P1 );
	DaoxBounds_Update( & bounds, self->P2 );
	if( self->bezier >= 2 ) DaoxBounds_Update( & bounds, self->C1 );
	if( self->bezier >= 3 ) DaoxBounds_Update( & bounds, self->C2 );
	return bounds;
}
int DaoxPathSegment_MayIntersect( DaoxPathSegment *self, DaoxPathSegment *other )
{
	DaoxBounds B1 = DaoxPathSegment_GetBounds( self );
	DaoxBounds B2 = DaoxPathSegment_GetBounds( other );
	if( self->bezier == 0 || other->bezier == 0 ) return 0;
	if( B1.left > B2.right || B1.right < B2.left ) return 0;
	if( B1.bottom > B2.top || B1.top < B2.bottom ) return 0;
	
	/* Check for adjacent segements: */
	if( self->end == other->start && DaoxDistance( self->P2, other->P1 ) == 0.0 ){
		DaoxBounds_Init( & B1, self->P1 );
		if( self->bezier >= 2 ) DaoxBounds_Update( & B1, self->C1 );
		if( self->bezier >= 3 ) DaoxBounds_Update( & B1, self->C2 );
	}else if( self->start == other->end && DaoxDistance( self->P1, other->P2 ) == 0.0 ){
		DaoxBounds_Init( & B1, self->P2 );
		if( self->bezier >= 2 ) DaoxBounds_Update( & B1, self->C1 );
		if( self->bezier >= 3 ) DaoxBounds_Update( & B1, self->C2 );
	}
	if( B1.left > B2.right || B1.right < B2.left ) return 0;
	if( B1.bottom > B2.top || B1.top < B2.bottom ) return 0;
	return 1;
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
	float area1, area2, epsilon = 0.0;
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

void DaoxPath_Refine( DaoxPath *self, float maxlen, float maxdiff )
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

void DaoxPath_Preprocess( DaoxPath *self, DaoxTriangulator *triangulator )
{
	DArray *segments = DArray_New(0);
	DaoxPathComponent *com;
	DaoxPathSegment *seg;
	daoint i, j, count = 0;
	float len, maxlen = 0.0;
	float maxdiff = 0.1;

	if( self->first->first->bezier == 0 ) return;

	printf( "\nDaoxPath_Segment 1: %p\n", triangulator );
	DaoxTriangulator_Reset( triangulator );
	DaoxPath_Refine( self, 1E16, 1 ); /* setup length; */
	for(com=self->first; com; com=com->next){
		len = DaoxPathComponent_MaxLineLength( com );
		if( len > maxlen ) maxlen = len;
	}
	printf( "maxlen = %f\n", maxlen );
	maxlen *= 0.5;
	if( maxlen < 1E-1 ) maxlen = 0.1;
	DaoxPath_Refine( self, maxlen, maxdiff );
	for(com=self->first; com; com=com->next){
		if( com->first->bezier == 0 ) continue;
		DaoxPathComponent_RetrieveRefined( com );
		if( com->refined.last == NULL || com->refined.last->next == NULL ) continue;
		seg = com->refined.first;
		do {
			DArray_PushBack( segments, seg );
			seg = seg->next;
		} while( seg && seg != com->refined.first );
	}
	printf( "Before intersection refinement: %i\n", (int)segments->size );
	for(i=0; i<segments->size; ++i){
		DaoxPathSegment *S1 = (DaoxPathSegment*) segments->items.pVoid[i];
		if( S1 == NULL ) continue;
		/* Do not use S1->delta here, line segments always have delta equal to 0: */
		if( S1->length < 1.0 ) continue;
		for(j=i+1; j<segments->size; ++j){
			DaoxPathSegment *S2 = (DaoxPathSegment*) segments->items.pVoid[j];
			if( S2 == NULL ) continue;
			if( S2->length < 1.0 ) continue;
			if( DaoxPathSegment_MayIntersect( S1, S2 ) ){
				float S, T, R1 = 0.5, R2 = 0.5;
				if( DaoxLine_Intersect( S1->P1, S1->P2, S2->P1, S2->P2, &S, &T ) ){
					R1 = 0.1*R1 + 0.9*S;
					R2 = 0.1*R2 + 0.9*T;
				}
				DaoxPathSegment_Divide( S1, R1 );
				DaoxPathSegment_Divide( S2, R2 );
				DaoxPathSegment_ComputeLengthAndDelta( S1->first );
				DaoxPathSegment_ComputeLengthAndDelta( S2->first );
				DaoxPathSegment_ComputeLengthAndDelta( S1->second );
				DaoxPathSegment_ComputeLengthAndDelta( S2->second );
				DArray_Append( segments, S1->first );
				DArray_Append( segments, S1->second );
				DArray_Append( segments, S2->first );
				DArray_Append( segments, S2->second );
				segments->items.pVoid[i] = NULL;
				segments->items.pVoid[j] = NULL;
				break;
			}
		}
		if( segments->items.pVoid[i] == NULL ) continue;
	}
	printf( "After intersection refinement: %i\n", (int)segments->size );
	segments->size = 0;
	for(com=self->first; com; com=com->next){
		if( com->first->bezier == 0 ) continue;
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
	DaoxTriangulator_Reset( triangulator );
	for(com=self->first; com; com=com->next){
		if( com->first->bezier == 0 ) continue;
		if( com->refined.last == NULL || com->refined.last->next == NULL ) continue;
		seg = com->refined.first;
		do {
			DaoxTriangulator_PushPoint( triangulator, seg->P1.x, seg->P1.y );
			if( seg->convexness < 0 && seg->bezier >= 2 ){
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









typedef struct DaoxPathSegmentPair  DaoxPathSegmentPair;
struct DaoxPathSegmentPair
{
	DaoxPathSegment  *first;
	DaoxPathSegment  *second;
};


void DaoxPathSegment_GetRefinedStroke( DaoxPathSegment *self, DaoxGraphicsData *gdata )
{
	DaoxQuad quad;
	DaoxPoint PM;
	DaoxColor color;
	DaoxPoint P1 = DaoxPoint_Transform( self->P1, gdata->transform );
	DaoxPoint P2 = DaoxPoint_Transform( self->P2, gdata->transform );
	DaoxColorGradient *strokeGradient = gdata->item->state->strokeGradient;
	int m, hasGradient = strokeGradient != NULL && strokeGradient->stops->count;
	float at, pos, len, width = gdata->strokeWidth;
	float offset = gdata->currentOffset;

	gdata->currentOffset += self->length;

	if( width < 1E-6 ) return;

	if( gdata->item->state->dash == 0 ){
		quad = DaoxLine2Quad( P1, P2, width );
		m = DaoxGraphicsData_PushStrokeQuad( gdata, quad );
		if( hasGradient && m ) DaoxGraphicsData_PushStrokeQuadColors( gdata, offset, self->length );
		return;
	}
	DaoxGraphicsData_MakeDashStroke( gdata, P1, P2, offset );
	return;
	len = DaoxDistance( P1, P2 );
	while( len > 0.0 ){
		if( len < gdata->dashLength ){
			if( (gdata->dashState&1) == 0 ){
				quad = DaoxLine2Quad( P1, P2, width );
				m = DaoxGraphicsData_PushStrokeQuad( gdata, quad );
				if( hasGradient && m ) DaoxGraphicsData_PushStrokeQuadColors( gdata, offset, len );
			}
			DaoxGraphicsData_UpdateDashState( gdata, len );
			return;
		}
		at = gdata->dashLength / len;
		PM.x = (1.0 - at) * P1.x + at * P2.x;
		PM.y = (1.0 - at) * P1.y + at * P2.y;
		if( (gdata->dashState&1) == 0 ){
			quad = DaoxLine2Quad( P1, PM, width );
			m = DaoxGraphicsData_PushStrokeQuad( gdata, quad );
			if( hasGradient && m ) DaoxGraphicsData_PushStrokeQuadColors( gdata, offset, gdata->dashLength );
		}
		len -= gdata->dashLength;
		offset += gdata->dashLength;
		DaoxGraphicsData_UpdateDashState( gdata, gdata->dashLength );
		P1 = PM;
	}
}


DaoxPathSegmentPair DaoxPathSegment_GetRefined( DaoxPathSegment *self, DaoxGraphicsData *gdata )
{
	DaoxBounds bounds;
	DaoxPoint P1, P2, P3;
	DaoxQuad quad, prev, next;
	DaoxPathSegmentPair sp1, sp2, segs = {NULL, NULL};
	DaoxColorGradient *strokeGradient = gdata->item->state->strokeGradient;
	int m = 0, hasGradient = strokeGradient != NULL && strokeGradient->stops->count;
	int finer = self->start < 1E-16 || self->end > (1.0-1E-16);
	float width = gdata->strokeWidth;
	float maxlen = gdata->maxlen;
	float maxdiff = gdata->maxdiff;
	float len, offset, nojunction = 0;

	if( maxlen < 1E-16 ) maxlen = 10;
	if( maxdiff < 1E-16 ) maxdiff = 0.001;

	DaoxBounds_Init( & bounds, DaoxPoint_Transform( self->P1, gdata->transform ) );
	DaoxBounds_Update( & bounds, DaoxPoint_Transform( self->P2, gdata->transform ) );
	if( self->bezier > 1 )
		DaoxBounds_Update( & bounds, DaoxPoint_Transform( self->C1, gdata->transform ) );
	if( self->bezier > 2 )
		DaoxBounds_Update( & bounds, DaoxPoint_Transform( self->C2, gdata->transform ) );

	segs.first = segs.second = self;
	if( DaoxBounds_Intersect( & gdata->bounds, bounds ) == 0 ){
		gdata->currentOffset += self->length;
		DaoxGraphicsData_UpdateDashState( gdata, self->length );
		return segs;
	}

	if( finer && self->count == 1 && self->delta > 1E-6 ){
		DaoxPathSegment_Divide( self, (self->start < 0.5) ? 0.01 : 0.99 );
		DaoxPathSegment_ComputeLengthAndDelta( self );
	}

	if( self->bezier == 1 || self->count == 1 ){
		DaoxPathSegment_GetRefinedStroke( self, gdata );
		return segs;
	}
	if( finer == 0 && self->length <= maxlen && self->delta <= maxdiff ){
		DaoxPathSegment_GetRefinedStroke( self, gdata );
		return segs;
	}
	if( finer == 0 && self->length < gdata->scale ){
		DaoxPathSegment_GetRefinedStroke( self, gdata );
		return segs;
	}
	if( self->component->last->next == self->component->first ){
		if( self->bezier == 2 ){
			if( self->convexness >= 0 ){
				DaoxGraphicsData_PushFilling( gdata, self->first->P2, self->P1, self->P2 );
			}else{
				DaoxGraphicsData_PushFilling( gdata, self->C1, self->first->C1, self->second->C1 );
			}
		}else{
			if( self->convexness >= 0 ){
				DaoxGraphicsData_PushFilling( gdata, self->first->P2, self->P1, self->P2 );
			}else{
				float at = (self->first->end - self->start) / (self->end - self->start);
				DaoxPoint Q;
				Q.x = (1.0 - at) * self->C1.x + at * self->C2.x;
				Q.y = (1.0 - at) * self->C1.y + at * self->C2.y;
				DaoxGraphicsData_PushFilling( gdata, self->C1, self->first->C1, Q );
				DaoxGraphicsData_PushFilling( gdata, self->C2, Q, self->second->C2 );
				DaoxGraphicsData_PushFilling( gdata, Q, self->first->C2, self->second->C1 );
			}
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
		m += DaoxGraphicsData_PushStrokeTriangle( gdata, prev.D, next.A, P2 );
		if( gdata->item->state->dash ){
			m += DaoxGraphicsData_PushStrokeTriangle( gdata, P2, prev.C, next.B );
		}
	}else{
		m += DaoxGraphicsData_PushStrokeTriangle( gdata, prev.C, next.B, P2 );
		if( gdata->item->state->dash ){
			m += DaoxGraphicsData_PushStrokeTriangle( gdata, P2, prev.D, next.A );
		}
	}
	if( hasGradient ){
		float pos = offset / gdata->currentLength;
		DaoxColor color = DaoxColorGradient_InterpolateColor( strokeGradient, pos );
		while( (m--) > 0 ) DaoxColorArray_Push( gdata->strokeColors, color );
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
	DaoxGraphicsScene *scene = gdata->item->scene;
	DaoxBounds box = scene->viewport;
	float scale = DaoxGraphicsScene_Scale( scene );
	float width = gdata->strokeWidth;
	float maxlen = gdata->maxlen;
	float maxdiff = gdata->maxdiff;
	float pos;
	int i, j, count, jt, jt2;

	if( self->first->first->bezier == 0 ) return;

	printf( "scale: %15f\n", scale );

	maxlen *= scale;
	maxdiff *= scale;

	if( maxlen < 1E-1 ) maxlen = 0.1;
	if( maxdiff < 1E-3 ) maxdiff = 0.001;

	gdata->scale = scale;
	gdata->maxlen = maxlen;
	gdata->maxdiff = maxdiff;

	DaoxPath_Refine( self, maxlen, maxdiff );
	gdata->currentLength = self->length;
	printf( "gdata->currentLength = %15f\n", self->length );
	if( 1 ){
		DaoxPoint *points = self->points->points;
		int *ids = self->triangles->values;
		int offset = gdata->fillPoints->count;
		for(i=0; i<self->points->count; i++){
			DaoxPoint point = DaoxPoint_Transform( points[i], gdata->transform );
			DaoxPointArray_Push( gdata->fillPoints, point );
		}
		for(i=0; i<self->triangles->count; i+=3){
			DaoxPoint A = points[ids[i]];
			DaoxPoint B = points[ids[i+1]];
			DaoxPoint C = points[ids[i+2]];
			DaoxPoint TA = DaoxPoint_Transform( A, gdata->transform );
			DaoxPoint TB = DaoxPoint_Transform( B, gdata->transform );
			DaoxPoint TC = DaoxPoint_Transform( C, gdata->transform );
			if( DaoxBounds_CheckTriangle( & gdata->bounds, TA, TB, TC ) == 0 ) continue;
			DaoxIntArray_Push( gdata->fillTriangles, ids[i] + offset );
			DaoxIntArray_Push( gdata->fillTriangles, ids[i+1] + offset );
			DaoxIntArray_Push( gdata->fillTriangles, ids[i+2] + offset );
		}
	}
	for(com=self->first; com; com=com->next){
		if( com->first->bezier == 0 ) continue;
		if( com->refined.last == NULL ) continue;
		//printf( "component: %p\n", com );
		if( com != self->first ){ /* Reset dash state for different contour? */
			gdata->dashState = 0;
			gdata->dashLength = gdata->item->state->dashPattern[0];
		}
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

		if( com->refined.last->next == NULL ) continue;
		if( open.first != NULL && cur.first != NULL && width > 1E-6 ){
			int userjunction = cur.second->end > 0.8 && open.first->start < 0.2;
			jt2 = gdata->item->state->dash == 0 || (gdata->dashState & 1) == 0;
			jt = userjunction ? gdata->junction : DAOX_JUNCTION_FLAT;
			if( jt && jt2 ) DaoxGraphicsData_MakeJunction( gdata, cur.second->P1, cur.second->P2, open.first->P2, 1.0, jt );
		}
	}
	printf( "strokeTriangles = %6i\n", gdata->strokeTriangles->count );
	printf( "fillTriangles   = %6i\n", gdata->fillTriangles->count );
}
