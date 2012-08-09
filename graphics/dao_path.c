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


#include <assert.h>
#include "daoStdtype.h"
#include "dao_path.h"



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




DaoxPathSegment* DaoxPathSegment_New( DaoxPath *path )
{
	DaoxPathSegment* self = (DaoxPathSegment*) calloc(1,sizeof(DaoxPathSegment));
	self->start = 0.0;
	self->end = 1.0;
	self->path = path;
	return self;
}

DaoxPathComponent* DaoxPathComponent_New( DaoxPath *path )
{
	DaoxPathComponent* self = (DaoxPathComponent*) calloc(1,sizeof(DaoxPathComponent));
	self->first = self->last = DaoxPathSegment_New( path );
	self->path = path;
	return self;
}
DaoxPathSegment* DaoxPathComponent_PushSegment( DaoxPathComponent *self )
{
	DaoxPathSegment *segment;
	if( self->first->bezier == 0 ) return self->first;
	segment = DaoxPathSegment_New( self->path );
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
	DaoxPoint point;
	DaoxPathSegment *segment = NULL;
	if( self->last->last->bezier == 0 ){
		point = self->last->last->P1;
		segment = self->last->last;
	}else{
		point = self->last->last->P2;
		segment = DaoxPathComponent_PushSegment( self->last );
		segment->P1 = point;
	}
	segment->bezier = 1;
	segment->P2.x = point.x + x;
	segment->P2.y = point.y + y;
}
void DaoxPath_ArcTo( DaoxPath *self, double x, double y, double degrees, int clockwise )
{
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
	if( self->first == NULL ) self->first = DaoxPathSegment_New( self->path );
	if( self->second == NULL ) self->second = DaoxPathSegment_New( self->path );
	self->first->count = 1;
	self->second->count = 1;
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
	double PC = DaoxDistance( self->P1, self->C1 );
	double CP = DaoxDistance( self->C1, self->P2 );
	double PP = DaoxDistance( self->P1, self->P2 );

	if( PP < maxlen && ((PC + CP - PP) < maxdiff*PP || PP < 0.01*maxlen) ){
		self->count = 1;
		if( self->first ) self->first->count = 0;
		if( self->second ) self->second->count = 0;
		return;
	}
	DaoxPathSegment_DivideQuadratic( self, PC / (PC + CP) );

	DaoxPathSegment_RefineQuadratic( self->first, maxlen, maxdiff );
	DaoxPathSegment_RefineQuadratic( self->second, maxlen, maxdiff );
	self->count = self->first->count + self->second->count;
}
void DaoxPathSegment_RefineCubic( DaoxPathSegment *self, double maxlen, double maxdiff )
{
	double PC = DaoxDistance( self->P1, self->C1 );
	double CC = DaoxDistance( self->C1, self->C2 );
	double CP = DaoxDistance( self->C2, self->P2 );
	double PP = DaoxDistance( self->P1, self->P2 );

	if( PP < maxlen && ((PC + CC + CP - PP) < maxdiff*PP || PP < 0.01*maxlen) ){
		self->count = 1;
		if( self->first ) self->first->count = 0;
		if( self->second ) self->second->count = 0;
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
	DaoxPathSegment *segment = self->first;
	do {
		DaoxPathSegment_Refine( segment, maxlen, maxdiff );
		if( segment->count == 1 ) DaoxPathSegment_Divide( segment, 0.5 );
		segment = segment->next;
	} while( segment && segment != self->first );
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
int DaoxPathSegment_CheckConvexness( DaoxPathSegment *self, DaoxPoint point )
{
	double area1, area2, m;
	if( self->bezier == 1 ) return 0;
	area1 = DaoxTriangle_Area( self->P1, self->P2, point );
	area2 = DaoxTriangle_Area( self->P1, self->P2, self->C1 );
	m = area1 * area2;
	if( m > 1E-9 ) return -1;
	if( m < -1E-9 ) return 1;
	return 0;
}
/*
// For locally concave path segment, add smaller triangles that are fully
// contained in the region enclosed by the path:
*/
void DaoxPathSegment_AddInnerTriangles( DaoxPathSegment *self, int start, int end, int p3 )
{
	int n = self->path->points->count;
	if( self->bezier != 2 && self->bezier != 3 ) return;
	DaoxPointArray_Push( self->path->points, self->C1 );
	DaoxIntArray_Push( self->path->triangles, start );
	DaoxIntArray_Push( self->path->triangles, n );
	DaoxIntArray_Push( self->path->triangles, p3 );
	if( self->bezier == 1 ){
		DaoxIntArray_Push( self->path->triangles, n );
		DaoxIntArray_Push( self->path->triangles, end );
		DaoxIntArray_Push( self->path->triangles, p3 );
	}else{
		DaoxPointArray_Push( self->path->points, self->C2 );
		DaoxIntArray_Push( self->path->triangles, n );
		DaoxIntArray_Push( self->path->triangles, n+1 );
		DaoxIntArray_Push( self->path->triangles, p3 );
		DaoxIntArray_Push( self->path->triangles, n+1 );
		DaoxIntArray_Push( self->path->triangles, end );
		DaoxIntArray_Push( self->path->triangles, p3 );
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

void DaoxPath_Segment( DaoxPath *self, DaoxPathBuffer *buffer )
{
	DArray *segments = DArray_New(0);
	DaoxTriangulator *triangulator = buffer->triangulator;
	DaoxPathComponent *com;
	DaoxPathSegment *seg;
	daoint i, count = 0;

	printf( "DaoxPath_Segment 1: %p\n", triangulator );
	DaoxTriangulator_Reset( triangulator );
	for(com=self->first; com; com=com->next){
		if( com->first->bezier == 0 ) continue;
		DaoxPathComponent_Refine( com, 1E6, 0.1 );
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
		DaoxPathSegment *SC = (DaoxPathSegment*) segments->items.pVoid[C];
		DaoxPathSegment *SA = (DaoxPathSegment*) segments->items.pVoid[A];
		DaoxPathSegment *SB = (DaoxPathSegment*) segments->items.pVoid[B];
		goto PushTriangle;
		if( SC->next != SA && SA->next != SB ) goto PushTriangle;
		if( SC->next == SA ){
			if( DaoxPathSegment_CheckConvexness( SC, SB->P1 ) >= 0 ) goto PushTriangle;
			DaoxPathSegment_AddInnerTriangles( SC, C, A, B );
		}
		if( SA->next == SB ){
			if( DaoxPathSegment_CheckConvexness( SA, SC->P1 ) >= 0 ) goto PushTriangle;
			DaoxPathSegment_AddInnerTriangles( SA, A, B, C );
		}
		continue;
PushTriangle:
		DaoxIntArray_Push( self->triangles, C );
		DaoxIntArray_Push( self->triangles, A );
		DaoxIntArray_Push( self->triangles, B );
	}
	
	DArray_Delete( segments );
}

