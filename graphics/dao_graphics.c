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

DaoType *daox_type_color_gradient = NULL;
DaoType *daox_type_linear_gradient = NULL;
DaoType *daox_type_radial_gradient = NULL;
DaoType *daox_type_path_gradient = NULL;
DaoType *daox_type_graphics_state = NULL;
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



float DaoxGraphicsScene_Scale( DaoxGraphicsScene *self );



DaoxColor DaoxColor_Interpolate( DaoxColor C1, DaoxColor C2, float start, float mid, float end )
{
	DaoxColor color;
	float at = (mid - start) / (end - start + 1E-9);
	color.red = (1.0 - at) * C1.red + at * C2.red;
	color.green = (1.0 - at) * C1.green + at * C2.green;
	color.blue = (1.0 - at) * C1.blue + at * C2.blue;
	color.alpha = (1.0 - at) * C1.alpha + at * C2.alpha;
	return color;
}




DaoxColorArray* DaoxColorArray_New()
{
	DaoxColorArray *self = (DaoxColorArray*) dao_calloc( 1, sizeof(DaoxColorArray) );
	return self;
}
void DaoxColorArray_Clear( DaoxColorArray *self )
{
	if( self->colors ) dao_free( self->colors );
	self->colors = NULL;
	self->count = 0;
}
void DaoxColorArray_Delete( DaoxColorArray *self )
{
	DaoxColorArray_Clear( self );
	dao_free( self );
}
void DaoxColorArray_Reset( DaoxColorArray *self )
{
	self->count = 0;
}
void DaoxColorArray_PushRGBA( DaoxColorArray *self, float r, float g, float b, float a )
{
	DaoxColor *color;
	if( self->count >= self->capacity ){
		self->capacity += 0.2 * self->capacity + 1;
		self->colors = (DaoxColor*) dao_realloc( self->colors, self->capacity * sizeof(DaoxColor) );
	}
	color = self->colors + self->count;
	color->red = r;
	color->green = g;
	color->blue = b;
	color->alpha = a;
	self->count += 1;
}
void DaoxColorArray_Push( DaoxColorArray *self, DaoxColor color )
{
	DaoxColorArray_PushRGBA( self, color.red, color.green, color.blue, color.alpha );
}




DaoxColorGradient* DaoxColorGradient_New( int type )
{
	DaoxColorGradient *self = (DaoxColorGradient*) dao_calloc(1,sizeof(DaoxColorGradient));
	DaoType *ctype = daox_type_color_gradient;
	switch( type ){
	case DAOX_GRADIENT_BASE : ctype = daox_type_color_gradient; break;
	case DAOX_GRADIENT_LINEAR : ctype = daox_type_linear_gradient; break;
	case DAOX_GRADIENT_RADIAL : ctype = daox_type_radial_gradient; break;
	case DAOX_GRADIENT_PATH : ctype = daox_type_path_gradient; break;
	}
	DaoCdata_InitCommon( (DaoCdata*)self, ctype );
	self->stops = DaoxFloatArray_New();
	self->colors = DaoxColorArray_New();
	self->gradient = type;
	return self;
}
void DaoxColorGradient_Delete( DaoxColorGradient *self )
{
	DaoxFloatArray_Delete( self->stops );
	DaoxColorArray_Delete( self->colors );
	DaoCdata_FreeCommon( (DaoCdata*) self );
	dao_free( self );
}
void DaoxColorGradient_Add( DaoxColorGradient *self, float stop, DaoxColor color )
{
	DaoxFloatArray_Push( self->stops, stop );
	DaoxColorArray_Push( self->colors, color );
}
DaoxColor DaoxColorGradient_InterpolateColor( DaoxColorGradient *self, float at )
{
	int i, n = self->stops->count;
	float start, end, *stops = self->stops->values;
	DaoxColor *colors = self->colors->colors;
	DaoxColor color = {0.0,0.0,0.0,0.0};

	if( self->stops->count ) color = self->colors->colors[0];
	if( self->stops->count <= 1 ) return color;
	if( at < 0.0 ) at = 0.0;
	if( at > 1.0 ) at = 1.0;
	if( at < stops[0] || at > stops[n-1] ){
		start = stops[n-1];
		end = stops[0] + 1.0;
		if( at < stops[0] ) at += 1.0;
		return DaoxColor_Interpolate( colors[n-1], colors[0], start, at, end );
	}
	for(i=1; i<n; ++i){
		float start = stops[i-1];
		float end = stops[i];
		if( at >= start && at <= end ){
			color = DaoxColor_Interpolate( colors[i-1], colors[i], start, at, end );
			break;
		}
	}
	return color;
}
DaoxColor DaoxColorGradient_ComputeLinearColor( DaoxColorGradient *self, DaoxPoint point )
{
	DaoxPoint A = self->points[0];
	DaoxPoint B = self->points[1];
	DaoxPoint C = point;
	float BxAx = B.x - A.x;
	float ByAy = B.y - A.y;
	float CxAx = C.x - A.x;
	float CyAy = C.y - A.y;
	float t = (CxAx * BxAx + CyAy * ByAy) / (BxAx * BxAx + ByAy * ByAy);
	return DaoxColorGradient_InterpolateColor( self, t );
}
DaoxColor DaoxColorGradient_ComputeRadialColor( DaoxColorGradient *self, DaoxPoint point )
{
	DaoxPoint C = self->points[0];
	DaoxPoint F = self->points[1];
	DaoxPoint G = point;
	float R = self->radius;
	float GxFx = G.x - F.x;
	float GyFy = G.y - F.y;
	float FxCx = F.x - C.x;
	float FyCy = F.y - C.y;
	float a = GxFx * GxFx + GyFy * GyFy;
	float b = 2.0 * (GxFx * FxCx + GyFy * FyCy);
	float c = FxCx * FxCx + FyCy * FyCy - R * R;
	float t = (- b + sqrt(b * b - 4.0 * a * c)) / (2.0 * a);
	if( t < 1.0 ){
		t = 1.0;
	}else{
		t = 1.0 / t;
	}
	return DaoxColorGradient_InterpolateColor( self, t );
}
DaoxColor DaoxColorGradient_ComputePathColor( DaoxColorGradient *self, DaoxPoint point )
{
}
DaoxColor DaoxColorGradient_ComputeColor( DaoxColorGradient *self, DaoxPoint point )
{
	DaoxColor color = {0.0,0.0,0.0,0.0};
	switch( self->gradient ){
	case DAOX_GRADIENT_LINEAR : return DaoxColorGradient_ComputeLinearColor( self, point );
	case DAOX_GRADIENT_RADIAL : return DaoxColorGradient_ComputeRadialColor( self, point );
	case DAOX_GRADIENT_PATH :   return DaoxColorGradient_ComputePathColor( self, point );
	}
	return color;
}


void DaoxColor_FromDaoValues( DaoxColor *self, DaoValue *values[] )
{
	self->red = values[0]->xFloat.value;
	self->green = values[1]->xFloat.value;
	self->blue = values[2]->xFloat.value;
	self->alpha = values[3]->xFloat.value;
}

static void GRAD_AddStop( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxColorGradient *self = (DaoxColorGradient*) p[0];
	DaoxColor color = {0.0,0.0,0.0,0.0};
	DaoxColor_FromDaoValues( & color, p[2]->xTuple.items );
	DaoxColorGradient_Add( self, p[1]->xFloat.value, color );
}
static void GRAD_AddStops( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxColorGradient *self = (DaoxColorGradient*) p[0];
	DaoList *stops = (DaoList*) p[1];
	int i, n = DaoList_Size( stops );
	for(i=0; i<n; ++i){
		DaoTuple *item = (DaoTuple*) DaoList_GetItem( stops, i );
		DaoxColor color = {0.0,0.0,0.0,0.0};
		DaoxColor_FromDaoValues( & color, item->items[1]->xTuple.items );
		DaoxColorGradient_Add( self, item->items[0]->xFloat.value, color );
	}
}

static DaoFuncItem DaoxColorGradientMeths[]=
{
	{ GRAD_AddStop,  "AddStop( self : ColorGradient, at: float, color: Color ) => ColorGradient" },
	{ GRAD_AddStops,  "AddStops( self : ColorGradient, stops: list<tuple<at:float,color:Color>> ) => ColorGradient" },
	{ NULL, NULL }
};

DaoTypeBase DaoxColorGradient_Typer =
{
	"ColorGradient", NULL, NULL, (DaoFuncItem*) DaoxColorGradientMeths, {0}, {0},
	(FuncPtrDel)DaoxColorGradient_Delete, NULL
};


static void LGRAD_SetStart( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxColorGradient *self = (DaoxColorGradient*) p[0];
	self->points[0].x = p[1]->xFloat.value;
	self->points[0].y = p[2]->xFloat.value;
}
static void LGRAD_SetEnd( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxColorGradient *self = (DaoxColorGradient*) p[0];
	self->points[1].x = p[1]->xFloat.value;
	self->points[1].y = p[2]->xFloat.value;
}

static DaoFuncItem DaoxLinearGradientMeths[]=
{
	{ LGRAD_SetStart,  "SetStart( self : LinearGradient, x : float, y : float )" },
	{ LGRAD_SetEnd,    "SetEnd( self : LinearGradient, x : float, y : float )" },
	{ NULL, NULL }
};

DaoTypeBase DaoxLinearGradient_Typer =
{
	"LinearGradient", NULL, NULL, (DaoFuncItem*) DaoxLinearGradientMeths,
	{ & DaoxColorGradient_Typer, 0}, {0},
	(FuncPtrDel)DaoxColorGradient_Delete, NULL
};


static void RGRAD_SetRadius( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxColorGradient *self = (DaoxColorGradient*) p[0];
	self->radius = p[1]->xFloat.value;
}

static DaoFuncItem DaoxRadialGradientMeths[]=
{
	{ LGRAD_SetStart,  "SetCenter( self : RadialGradient, x : float, y : float )" },
	{ LGRAD_SetEnd,    "SetFocus( self : RadialGradient, x : float, y : float )" },
	{ RGRAD_SetRadius, "SetRadius( self : RadialGradient, r : float )" },
	{ NULL, NULL }
};

DaoTypeBase DaoxRadialGradient_Typer =
{
	"RadialGradient", NULL, NULL, (DaoFuncItem*) DaoxRadialGradientMeths,
	{ & DaoxColorGradient_Typer, 0}, {0},
	(FuncPtrDel)DaoxColorGradient_Delete, NULL
};


static void PGRAD_AddStop( DaoProcess *proc, DaoValue *p[], int N )
{
}

static DaoFuncItem DaoxPathGradientMeths[]=
{
	{ NULL, NULL }
};

DaoTypeBase DaoxPathGradient_Typer =
{
	"PathGradient", NULL, NULL, (DaoFuncItem*) DaoxPathGradientMeths,
	{ & DaoxColorGradient_Typer, 0}, {0},
	(FuncPtrDel)DaoxColorGradient_Delete, NULL
};








DaoxGraphicsData* DaoxGraphicsData_New( DaoxGraphicsItem *item )
{
	DaoxGraphicsData *self = (DaoxGraphicsData*)dao_calloc(1,sizeof(DaoxGraphicsData));
	self->strokeColors = DaoxColorArray_New();
	self->strokePoints = DaoxPointArray_New();
	self->strokeTriangles = DaoxIntArray_New();
	self->fillColors = DaoxColorArray_New();
	self->fillPoints = DaoxPointArray_New();
	self->fillTriangles = DaoxIntArray_New();
	self->item = item;
	return self;
}
void DaoxGraphicsData_Delete( DaoxGraphicsData *self )
{
	DaoxColorArray_Delete( self->strokeColors );
	DaoxPointArray_Delete( self->strokePoints );
	DaoxIntArray_Delete( self->strokeTriangles );
	DaoxColorArray_Delete( self->fillColors );
	DaoxPointArray_Delete( self->fillPoints );
	DaoxIntArray_Delete( self->fillTriangles );
	dao_free( self );
}
void DaoxGraphicsData_Reset( DaoxGraphicsData *self )
{
	DaoxColorArray_Reset( self->strokeColors );
	DaoxPointArray_Reset( self->strokePoints );
	DaoxIntArray_Reset( self->strokeTriangles );
	DaoxColorArray_Reset( self->fillColors );
	DaoxPointArray_Reset( self->fillPoints );
	DaoxIntArray_Reset( self->fillTriangles );
}
void DaoxGraphicsData_Init( DaoxGraphicsData *self, DaoxGraphicsItem *item )
{
	self->transform = NULL;
	self->maxlen = 10;
	self->maxdiff = 0.001;
	self->dashState = 0;
	self->dashLength = item->state->dashPattern[0];
	self->currentOffset = 0.0;
	self->currentLength = -1.0;
	self->junction = item->state->junction;
	self->strokeWidth = item->state->strokeWidth;
	self->item = item;
	DaoxGraphicsData_Reset( self );
}
void DaoxGraphicsData_UpdateDashState2( DaoxGraphicsData *self, float len )
{
	if( self->item->state->dash == 0 ) return;
	self->dashLength -= len;
	if( self->dashLength <= 1E-16 ){
		self->dashState = (self->dashState + 1) % self->item->state->dash;
		self->dashLength = self->item->state->dashPattern[ self->dashState ];
	}
}
void DaoxGraphicsData_UpdateDashState( DaoxGraphicsData *self, float len )
{
	if( self->item->state->dash == 0 ) return;
	while( len >= self->dashLength ){
		len -= self->dashLength;
		DaoxGraphicsData_UpdateDashState2( self, self->dashLength );
	}
	if( len > 0.0 ) DaoxGraphicsData_UpdateDashState2( self, len );
}

void DaoxIntArray_PushTriangle( DaoxIntArray *triangles, int A, int B, int C )
{
	DaoxIntArray_Push( triangles, A );
	DaoxIntArray_Push( triangles, B );
	DaoxIntArray_Push( triangles, C );
}
void DaoxGraphicsData_PushStrokeColor( DaoxGraphicsData *self, DaoxColor color, int times )
{
	while( (times--) > 0 ) DaoxColorArray_Push( self->strokeColors, color );
}
void DaoxGraphicsData_PushStrokeQuadColors( DaoxGraphicsData *self, float offset, float len )
{
	DaoxColorGradient *strokeGradient = self->item->state->strokeGradient;
	DaoxColor color;
	float pos;
	pos = offset / self->currentLength;
	color = DaoxColorGradient_InterpolateColor( strokeGradient, pos );
	DaoxGraphicsData_PushStrokeColor( self, color, 2 );
	pos = (offset + len) / self->currentLength;
	color = DaoxColorGradient_InterpolateColor( strokeGradient, pos );
	DaoxGraphicsData_PushStrokeColor( self, color, 2 );
}
int DaoxGraphicsData_PushStrokeTriangle( DaoxGraphicsData *self, DaoxPoint A, DaoxPoint B, DaoxPoint C )
{
	int index = self->strokePoints->count;
	if( DaoxBounds_CheckTriangle( & self->bounds, A, B, C ) == 0 ) return 0;
	DaoxPointArray_Push( self->strokePoints, A );
	DaoxPointArray_Push( self->strokePoints, B );
	DaoxPointArray_Push( self->strokePoints, C );
	DaoxIntArray_Push( self->strokeTriangles, index );
	DaoxIntArray_Push( self->strokeTriangles, index+1 );
	DaoxIntArray_Push( self->strokeTriangles, index+2 );
	return 3;
}
void DaoxGraphicsData_PushQuad( DaoxPointArray *points, DaoxIntArray *triangles, DaoxQuad quad )
{
	int index = points->count;
	DaoxPointArray_Push( points, quad.A );
	DaoxPointArray_Push( points, quad.B );
	DaoxPointArray_Push( points, quad.C );
	DaoxPointArray_Push( points, quad.D );
	DaoxIntArray_Push( triangles, index );
	DaoxIntArray_Push( triangles, index+1 );
	DaoxIntArray_Push( triangles, index+2 );
	DaoxIntArray_Push( triangles, index );
	DaoxIntArray_Push( triangles, index+2 );
	DaoxIntArray_Push( triangles, index+3 );
}
int DaoxGraphicsData_PushStrokeQuad( DaoxGraphicsData *self, DaoxQuad quad )
{
	if( DaoxBounds_CheckQuad( & self->bounds, quad ) == 0 ) return 0;
	DaoxGraphicsData_PushQuad( self->strokePoints, self->strokeTriangles, quad );
	return 4;
}
int DaoxGraphicsData_PushStrokeQuadNoCheck( DaoxGraphicsData *self, DaoxQuad quad )
{
	if( DaoxBounds_CheckQuad( & self->bounds, quad ) == 0 ) return 0;
	DaoxGraphicsData_PushQuad( self->strokePoints, self->strokeTriangles, quad );
	return 4;
}
void DaoxGraphicsData_PushFillQuad( DaoxGraphicsData *self, DaoxQuad quad )
{
	if( DaoxBounds_CheckQuad( & self->bounds, quad ) == 0 ) return;
	DaoxGraphicsData_PushQuad( self->fillPoints, self->fillTriangles, quad );
}
void DaoxGraphicsData_PushFilling( DaoxGraphicsData *self, DaoxPoint A, DaoxPoint B, DaoxPoint C )
{
	int index = self->fillPoints->count;
	A = DaoxPoint_Transform( A, self->transform );
	B = DaoxPoint_Transform( B, self->transform );
	C = DaoxPoint_Transform( C, self->transform );
	if( DaoxBounds_CheckTriangle( & self->bounds, A, B, C ) == 0 ) return;
	DaoxPointArray_Push( self->fillPoints, A );
	DaoxPointArray_Push( self->fillPoints, B );
	DaoxPointArray_Push( self->fillPoints, C );
	DaoxIntArray_Push( self->fillTriangles, index );
	DaoxIntArray_Push( self->fillTriangles, index+1 );
	DaoxIntArray_Push( self->fillTriangles, index+2 );
}
void DaoxGraphicsData_MakeJunction( DaoxGraphicsData *self, DaoxPoint A, DaoxPoint B, DaoxPoint C, float pos, int junction )
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
	float scale, dist, cosine, sine, angle, W2 = 0.5 * self->strokeWidth;
	int m = 0, k = DaoxLineQuad_Junction( Q1, Q2, & P3 );

	if( self->strokeWidth < 1E-6 ) return;

	if( hasGradient ) color = DaoxColorGradient_InterpolateColor( strokeGradient, pos );

	if( k > 0 ){
		if( junction != DAOX_JUNCTION_ROUND ){
			m = DaoxGraphicsData_PushStrokeTriangle( self, Q2.A, Q1.D, P2 );
			if( hasGradient ) DaoxGraphicsData_PushStrokeColor( self, color, m );
		}
		if( junction == DAOX_JUNCTION_SHARP ){
			m = DaoxGraphicsData_PushStrokeTriangle( self, Q1.D, Q2.A, P3 );
			if( hasGradient ) DaoxGraphicsData_PushStrokeColor( self, color, m );
		}
	}else{
		if( junction != DAOX_JUNCTION_ROUND ){
			m = DaoxGraphicsData_PushStrokeTriangle( self, Q1.C, Q2.B, P2 );
			if( hasGradient ) DaoxGraphicsData_PushStrokeColor( self, color, m );
		}
		if( junction == DAOX_JUNCTION_SHARP ){
			m = DaoxGraphicsData_PushStrokeTriangle( self, Q2.B, Q1.C, P3 );
			if( hasGradient ) DaoxGraphicsData_PushStrokeColor( self, color, m );
		}
	}
	if( self->item->state->dash ){
		if( k > 0 ){
			m = DaoxGraphicsData_PushStrokeTriangle( self, P2, Q2.A, Q1.D );
		}else{
			m = DaoxGraphicsData_PushStrokeTriangle( self, P2, Q1.C, Q2.B );
		}
		if( hasGradient ) DaoxGraphicsData_PushStrokeColor( self, color, m );
	}
	if( junction != DAOX_JUNCTION_ROUND ) return;

	scale = 0.5 * self->strokeWidth / 100.0;
	dist = DaoxDistance( P2, P3 );
	cosine = (P3.x - P2.x) / dist;
	sine = (P3.y - P2.y) / dist;
	dist = DaoxDistance( Q1.C, Q2.B );
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
void DaoxGraphicsData_MakeDashStroke( DaoxGraphicsData *self, DaoxPoint P1, DaoxPoint P2, float offset )
{
	DaoxPoint PM;
	DaoxQuad quad;
	DaoxColorGradient *strokeGradient = self->item->state->strokeGradient;
	int m, hasGradient = strokeGradient != NULL && strokeGradient->stops->count;
	float at, pos, width = self->strokeWidth;
	float len = DaoxDistance( P1, P2 );
	while( len > 0.0 ){
		if( len < self->dashLength ){
			if( (self->dashState&1) == 0 ){
				quad = DaoxLine2Quad( P1, P2, width );
				m = DaoxGraphicsData_PushStrokeQuad( self, quad );
				if( hasGradient && m ) DaoxGraphicsData_PushStrokeQuadColors( self, offset, len );
			}
			DaoxGraphicsData_UpdateDashState( self, len );
			return;
		}
		at = self->dashLength / len;
		PM.x = (1.0 - at) * P1.x + at * P2.x;
		PM.y = (1.0 - at) * P1.y + at * P2.y;
		if( (self->dashState&1) == 0 ){
			quad = DaoxLine2Quad( P1, PM, width );
			m = DaoxGraphicsData_PushStrokeQuad( self, quad );
			if( hasGradient && m ) DaoxGraphicsData_PushStrokeQuadColors( self, offset, self->dashLength );
		}
		len -= self->dashLength;
		offset += self->dashLength;
		DaoxGraphicsData_UpdateDashState( self, self->dashLength );
		P1 = PM;
	}
}
void DaoxGraphicsData_MakeLine( DaoxGraphicsData *self, DaoxPoint P1, DaoxPoint P2 )
{
	float len = DaoxDistance( P1, P2 );
	float offset = self->currentOffset;
	DaoxQuad quad = DaoxLine2Quad( P1, P2, self->strokeWidth );
	DaoxBounds bounds;

	DaoxBounds_Init( & bounds, quad.A );
	DaoxBounds_Update( & bounds, quad.B );
	DaoxBounds_Update( & bounds, quad.C );
	DaoxBounds_Update( & bounds, quad.D );

	if( DaoxBounds_Intersect( & self->bounds, bounds ) == 0 ){
		self->currentOffset += len;
		DaoxGraphicsData_UpdateDashState( self, len );
		return;
	}

	if( self->item->state->dash ){
		DaoxGraphicsData_MakeDashStroke( self, P1, P2, self->currentOffset );
	}else{
		DaoxColorGradient *strokeGradient = self->item->state->strokeGradient;
		int m, hasGradient = strokeGradient != NULL && strokeGradient->stops->count;
		DaoxQuad quad = DaoxLine2Quad( P1, P2, self->strokeWidth );
		m = DaoxGraphicsData_PushStrokeQuad( self, quad );
		if( hasGradient && m ) DaoxGraphicsData_PushStrokeQuadColors( self, offset, len );
	}
	self->currentOffset += len;
}
void DaoxGraphicsData_MakeRoundCorner( DaoxGraphicsData *self )
{
	DaoxGraphicsData backup;
	DaoxPath_ExportGraphicsData( self->item->scene->quarterArc, self );
	backup = *self;
	if( self->item->state->fillColor.alpha > 1E-9 ){
		self->strokeWidth = 0.0;
		DaoxPath_ExportGraphicsData( self->item->scene->quarterCircle, self );
	}
	*self = backup;
}
void DaoxGraphicsData_MakeRect( DaoxGraphicsData *self, DaoxPoint P1, DaoxPoint P2, float rx, float ry )
{
	DaoxBounds bounds;
	DaoxPoint LB, RB, RT, LT, point;
	DaoxQuad quad0, quad1, quad2, quad3, quad4;
	DaoxTransform transform = {0.0,0.0,0.0,0.0,0.0,0.0};
	DaoxColorGradient *strokeGradient = self->item->state->strokeGradient;
	int hasGradient = strokeGradient != NULL && strokeGradient->stops->count;
	float extra = 0.1 * DaoxGraphicsScene_Scale( self->item->scene );
	float arclen = 0.0, dashLength = 0.0;
	float tmp, W2 = 0.5 * self->strokeWidth;
	float left = P1.x, right = P2.x;
	float bottom = P1.y, top = P2.y;
	float width, height;
	int m, roundCorner = (rx > 1E-24) && (ry > 1E-24);
	uchar_t dashState = 0;

	if( left > right ) tmp = left, left = right, right = tmp;
	if( bottom > top ) tmp = bottom, bottom = top, top = tmp;
	width = right - left;
	height = top - bottom;

	bounds.left = left - W2;
	bounds.right = right + W2;
	bounds.bottom = bottom - W2;
	bounds.top = top + W2;
	if( DaoxBounds_Intersect( & self->bounds, bounds ) == 0 ) return;

	LB.x = left;
	LB.y = bottom;
	RB.x = right;
	RB.y = bottom;
	RT.x = right;
	RT.y = top;
	LT.x = left;
	LT.y = top;
	tmp = (rx - ry) / (rx + ry);
	tmp = 3.0 * tmp * tmp;
	arclen = M_PI * (rx + ry) * (1.0 + tmp / (10.0 + sqrt(4.0 - tmp)));
	self->currentLength = 2 * (width + height) + arclen;
	self->currentLength -= 4 * (rx + ry);
	if( roundCorner == 0 && self->item->state->dash == 0 && hasGradient == 0 ){
		int id1 = self->strokePoints->count;
		int id2 = id1 + 4;
		int id3 = id2 + 4;
		quad1 = DaoxQuad_FromRect( left - W2, bottom - W2, right + W2, bottom + W2 );
		quad3 = DaoxQuad_FromRect( left - W2, top - W2, right + W2, top + W2 );

		DaoxGraphicsData_PushStrokeQuadNoCheck( self, quad1 );
		DaoxGraphicsData_PushStrokeQuadNoCheck( self, quad3 );
		DaoxPointArray_Push( self->strokePoints, DaoxPoint_FromXY( left+W2, bottom+W2 ) );
		DaoxPointArray_Push( self->strokePoints, DaoxPoint_FromXY( left+W2, top-W2 ) );
		DaoxPointArray_Push( self->strokePoints, DaoxPoint_FromXY( right-W2, bottom+W2 ) );
		DaoxPointArray_Push( self->strokePoints, DaoxPoint_FromXY( right-W2, top-W2 ) );
		DaoxIntArray_PushTriangle( self->strokeTriangles, id1+3, id3, id3+1 );
		DaoxIntArray_PushTriangle( self->strokeTriangles, id1+3, id3+1, id2 );
		DaoxIntArray_PushTriangle( self->strokeTriangles, id3+2, id1+2, id2+1 );
		DaoxIntArray_PushTriangle( self->strokeTriangles, id3+2, id2+1, id3+3 );

		if( self->item->state->fillColor.alpha < 1E-9 ) return;
		quad0 = DaoxQuad_FromRect( left + W2, bottom + W2, right - W2, top - W2 );
		DaoxGraphicsData_PushFillQuad( self, quad0 );
		return;
	}else if( roundCorner == 0 && self->item->state->dash == 0 ){
		int id1 = self->strokePoints->count;
		int id2 = id1 + 4;
		int id3 = id2 + 4;
		int id4 = id3 + 4;
		int id5 = id4 + 4;
		self->currentLength = 2.0 * (width + height);
		quad1 = DaoxQuad_FromRectLeftFirst( left + W2, bottom - W2, right - W2, bottom + W2 );
		quad2 = DaoxQuad_FromRectBottomFirst( right - W2, bottom + W2, right + W2, top - W2 );
		quad3 = DaoxQuad_FromRectRightFirst( left + W2, top - W2, right - W2, top + W2 );
		quad4 = DaoxQuad_FromRectTopFirst( left - W2, bottom + W2, left + W2, top - W2 );

		DaoxGraphicsData_PushStrokeQuadNoCheck( self, quad1 );
		DaoxGraphicsData_PushStrokeQuadNoCheck( self, quad2 );
		DaoxGraphicsData_PushStrokeQuadNoCheck( self, quad3 );
		DaoxGraphicsData_PushStrokeQuadNoCheck( self, quad4 );
		DaoxPointArray_Push( self->strokePoints, DaoxPoint_FromXY( left-W2, bottom-W2 ) );
		DaoxPointArray_Push( self->strokePoints, DaoxPoint_FromXY( right+W2, bottom-W2 ) );
		DaoxPointArray_Push( self->strokePoints, DaoxPoint_FromXY( right+W2, top+W2 ) );
		DaoxPointArray_Push( self->strokePoints, DaoxPoint_FromXY( left-W2, top+W2 ) );
		DaoxIntArray_PushTriangle( self->strokeTriangles, id5, id1+1, id1 );
		DaoxIntArray_PushTriangle( self->strokeTriangles, id5, id4+3, id4+2 );
		DaoxIntArray_PushTriangle( self->strokeTriangles, id5+1, id1+3, id1+2 );
		DaoxIntArray_PushTriangle( self->strokeTriangles, id5+1, id2+1, id2 );
		DaoxIntArray_PushTriangle( self->strokeTriangles, id5+2, id2+3, id2+2 );
		DaoxIntArray_PushTriangle( self->strokeTriangles, id5+2, id3+1, id3 );
		DaoxIntArray_PushTriangle( self->strokeTriangles, id5+3, id3+3, id3+2 );
		DaoxIntArray_PushTriangle( self->strokeTriangles, id5+3, id4+1, id4 );

		DaoxGraphicsData_PushStrokeQuadColors( self, 0, width );
		DaoxGraphicsData_PushStrokeQuadColors( self, width, height );
		DaoxGraphicsData_PushStrokeQuadColors( self, width + height, width );
		DaoxGraphicsData_PushStrokeQuadColors( self, 2*width + height, height );
		DaoxGraphicsData_PushStrokeColor( self, self->strokeColors->colors[id1], 1 );
		DaoxGraphicsData_PushStrokeColor( self, self->strokeColors->colors[id2], 1 );
		DaoxGraphicsData_PushStrokeColor( self, self->strokeColors->colors[id3], 1 );
		DaoxGraphicsData_PushStrokeColor( self, self->strokeColors->colors[id4], 1 );

		if( self->item->state->fillColor.alpha < 1E-9 ) return;
		quad0 = DaoxQuad_FromRect( left + W2, bottom + W2, right - W2, top - W2 );
		DaoxGraphicsData_PushFillQuad( self, quad0 );
		return;
	}else if( roundCorner == 0 ){
		DaoxColorGradient *strokeGradient = self->item->state->strokeGradient;
		DaoxColor color1, color2, color3, color4;
		float len = 2.0 * (width + height);

		quad1 = DaoxQuad_FromRect( left - W2, bottom - W2, left, bottom );
		quad2 = DaoxQuad_FromRect( right, bottom - W2, right + W2, bottom );
		quad3 = DaoxQuad_FromRect( right, top, right + W2, top + W2 );
		quad4 = DaoxQuad_FromRect( left - W2, top, left, top + W2 );

		color1 = DaoxColorGradient_InterpolateColor( strokeGradient, 0.0 );
		color2 = DaoxColorGradient_InterpolateColor( strokeGradient, width / len );
		color3 = DaoxColorGradient_InterpolateColor( strokeGradient, (width+height)/len );
		color4 = DaoxColorGradient_InterpolateColor( strokeGradient, (2*width+height)/len );

		self->currentLength = len;
		DaoxGraphicsData_MakeLine( self, LB, RB );
		DaoxGraphicsData_MakeLine( self, RB, RT );
		DaoxGraphicsData_MakeLine( self, RT, LT );
		DaoxGraphicsData_MakeLine( self, LT, LB );

		DaoxGraphicsData_PushStrokeQuadNoCheck( self, quad1 );
		DaoxGraphicsData_PushStrokeQuadNoCheck( self, quad2 );
		DaoxGraphicsData_PushStrokeQuadNoCheck( self, quad3 );
		DaoxGraphicsData_PushStrokeQuadNoCheck( self, quad4 );

		DaoxGraphicsData_PushStrokeColor( self, color1, 4 );
		DaoxGraphicsData_PushStrokeColor( self, color2, 4 );
		DaoxGraphicsData_PushStrokeColor( self, color3, 4 );
		DaoxGraphicsData_PushStrokeColor( self, color4, 4 );

		if( self->item->state->fillColor.alpha < 1E-9 ) return;
		quad0 = DaoxQuad_FromRect( left, bottom, right, top );
		DaoxGraphicsData_PushFillQuad( self, quad0 );
		return;
	}

	if( self->item->state->fillColor.alpha > 1E-9 ){
		quad0 = DaoxQuad_FromRect( left+rx, bottom+ry, right-rx, top-ry );
		DaoxGraphicsData_PushFillQuad( self, quad0 );
		if( self->item->state->dash ){
			quad1 = DaoxQuad_FromRect( left + rx, bottom, right - rx, bottom + ry );
			quad2 = DaoxQuad_FromRect( right - rx, bottom + ry, right, top - ry );
			quad3 = DaoxQuad_FromRect( left + rx, top - ry, right - rx, top );
			quad4 = DaoxQuad_FromRect( left, bottom + ry, left + rx, top - ry );
		}else{
			quad1 = DaoxQuad_FromRect( left + rx, bottom + W2, right - rx, bottom + ry );
			quad2 = DaoxQuad_FromRect( right - rx, bottom + ry, right - W2, top - ry );
			quad3 = DaoxQuad_FromRect( left + rx, top - ry, right - rx, top - W2 );
			quad4 = DaoxQuad_FromRect( left + W2, bottom + ry, left + rx, top - ry );
		}
		DaoxGraphicsData_PushFillQuad( self, quad1 );
		DaoxGraphicsData_PushFillQuad( self, quad2 );
		DaoxGraphicsData_PushFillQuad( self, quad3 );
		DaoxGraphicsData_PushFillQuad( self, quad4 );
	}

	m = self->strokePoints->count;
	self->junction = DAOX_JUNCTION_FLAT;
	self->transform = & transform;
	transform.Axx = - rx / 100.0;
	transform.Ayy = - ry / 100.0 ;
	transform.Bx = left + rx;
	transform.By = bottom + ry;
	DaoxGraphicsData_MakeRoundCorner( self );
	if( self->item->state->strokeWidth > 0.001 * self->scale ){
		DaoxPoint start = LB, end = RB;
		start.x += rx - extra;
		end.x -= rx - extra;
		DaoxGraphicsData_MakeLine( self, start, end );
	}

	transform.Axx = transform.Ayy = 0.0;
	transform.Axy = + rx / 100.0;
	transform.Ayx = - ry / 100.0 ;
	transform.Bx = right - rx;
	transform.By = bottom + ry;
	DaoxGraphicsData_MakeRoundCorner( self );
	if( self->item->state->strokeWidth > 0.001 * self->scale ){
		DaoxPoint start = RB, end = RT;
		start.y += ry - extra;
		end.y -= ry - extra;
		DaoxGraphicsData_MakeLine( self, start, end );
	}

	transform.Axy = transform.Ayx = 0.0;
	transform.Axx = + rx / 100.0;
	transform.Ayy = + ry / 100.0 ;
	transform.Bx = right - rx;
	transform.By = top - ry;
	DaoxGraphicsData_MakeRoundCorner( self );
	if( self->item->state->strokeWidth > 0.001 * self->scale ){
		DaoxPoint start = RT, end = LT;
		start.x -= rx - extra;
		end.x += rx - extra;
		DaoxGraphicsData_MakeLine( self, start, end );
	}

	transform.Axx = transform.Ayy = 0.0;
	transform.Axy = - rx / 100.0;
	transform.Ayx = + ry / 100.0 ;
	transform.Bx = left + rx;
	transform.By = top - ry;
	DaoxGraphicsData_MakeRoundCorner( self );
	if( self->item->state->strokeWidth > 0.001 * self->scale ){
		DaoxPoint start = LT, end = LB;
		start.y -= ry - extra;
		end.y += ry - extra;
		DaoxGraphicsData_MakeLine( self, start, end );
	}
}







DaoxPoint DaoxPoint_Mean( DaoxPoint A, DaoxPoint B )
{
	DaoxPoint M;
	M.x = 0.5 * (A.x + B.x);
	M.y = 0.5 * (A.y + B.y);
	return M;
}
DaoxColor DaoxColor_Mean( DaoxColor A, DaoxColor B )
{
	DaoxColor M;
	M.red = 0.5 * (A.red + B.red);
	M.green = 0.5 * (A.green + B.green);
	M.blue = 0.5 * (A.blue + B.blue);
	M.alpha = 0.5 * (A.alpha + B.alpha);
	return M;
}
float DaoxColor_MaxDiff( DaoxColor A, DaoxColor B, float maxdiff )
{
	float red = fabs( A.red - B.red );
	float green = fabs( A.green - B.green );
	float blue = fabs( A.blue - B.blue );
	float alpha = fabs( A.alpha - B.alpha );
	if( red > maxdiff ) maxdiff = red;
	if( green > maxdiff ) maxdiff = green;
	if( blue > maxdiff ) maxdiff = blue;
	if( green > maxdiff ) maxdiff = green;
	return maxdiff;
}
void DaoxGraphicsData_MakeFillGradient( DaoxGraphicsData *self )
{
	DaoxColorGradient *gradient = self->item->state->fillGradient;
	DaoxColorArray *colors = self->fillColors;
	DaoxPointArray *points = self->fillPoints;
	DaoxIntArray *triangles = self->fillTriangles;
	DaoxColor mean, correct;
	float maxdist, scale = DaoxGraphicsScene_Scale( self->item->scene );
	int i, k, m, n;

	if( gradient == NULL ) return;
	maxdist = (gradient->gradient == DAOX_GRADIENT_LINEAR ? 50 : 30);
	for(i=colors->count,n=points->count; i<n; ++i){
		DaoxColor C = DaoxColorGradient_ComputeColor( gradient, points->points[i] );
		DaoxColorArray_Push( colors, C );
	}
	printf( "before: %i\n", triangles->count );
	for(i=0,k=0; i<triangles->count; i+=3){
		float dAB, dBC, dCA, dmax = 0.0, diff = 0.0;
		int IA = triangles->values[i];
		int IB = triangles->values[i+1];
		int IC = triangles->values[i+2];
		DaoxPoint PA = points->points[IA];
		DaoxPoint PB = points->points[IB];
		DaoxPoint PC = points->points[IC];
		DaoxPoint PmAB, PmBC, PmCA;
		DaoxColor CA, CB, CC, CmAB, CmBC, CmCA;
		DaoxColor CtAB, CtBC, CtCA;

		if( DaoxBounds_CheckTriangle( & self->bounds, PA, PB, PC ) == 0 ) continue;

		dAB = DaoxDistance( PA, PB );
		if( dAB > dmax ) dmax = dAB;
		dBC = DaoxDistance( PB, PC );
		if( dBC > dmax ) dmax = dBC;
		dCA = DaoxDistance( PC, PA );
		if( dCA > dmax ) dmax = dCA;
		/* Rotate variables until BC become the longest edge: */
		if( (dmax - dAB) < 1E-16 ){
			m = IA;
			IA = IC;
			IC = IB;
			IB = m;
		}else if( (dmax - dCA) < 1E-16 ){
			m = IA;
			IA = IB;
			IB = IC;
			IC = m;
		}
		PA = points->points[IA];
		PB = points->points[IB];
		PC = points->points[IC];
		if( dmax < 1.0 * scale ) goto NoSubdivision;

		CA = colors->colors[IA];
		CB = colors->colors[IB];
		CC = colors->colors[IC];

		PmAB = DaoxPoint_Mean( PA, PB );
		PmBC = DaoxPoint_Mean( PB, PC );
		PmCA = DaoxPoint_Mean( PC, PA );
		CmAB = DaoxColor_Mean( CA, CB );
		CmBC = DaoxColor_Mean( CB, CC );
		CmCA = DaoxColor_Mean( CC, CA );

		CtAB= DaoxColorGradient_ComputeColor( gradient, PmAB );
		CtBC= DaoxColorGradient_ComputeColor( gradient, PmBC );
		CtCA= DaoxColorGradient_ComputeColor( gradient, PmCA );
		diff = DaoxColor_MaxDiff( CmAB, CtAB, diff );
		diff = DaoxColor_MaxDiff( CmBC, CtBC, diff );
		diff = DaoxColor_MaxDiff( CmCA, CtCA, diff );

		if( dmax * diff < 0.1 * scale && dmax < 2*maxdist ) goto NoSubdivision;

		if( diff > 2E-3 || dmax > maxdist ){
			int IBC = points->count;
			DaoxPointArray_Push( points, PmBC );
			DaoxColorArray_Push( colors, CtBC );
			DaoxIntArray_PushTriangle( triangles, IA, IB, IBC );
			DaoxIntArray_PushTriangle( triangles, IC, IA, IBC );
			continue;
		}
NoSubdivision:
		triangles->values[k] = IA;
		triangles->values[k+1] = IB;
		triangles->values[k+2] = IC;
		k += 3;
	}
	triangles->count = k;
	printf( "after: %i\n", triangles->count );
}






DaoxGraphicsState* DaoxGraphicsState_New()
{
	DaoxGraphicsState *self = (DaoxGraphicsState*) dao_calloc(1,sizeof(DaoxGraphicsState));
	DaoCdata_InitCommon( (DaoCdata*)self, daox_type_graphics_state );
	self->junction = DAOX_JUNCTION_FLAT;
	self->transform.Axx = 1.0;
	self->transform.Ayy = 1.0;
	self->strokeWidth = 1.0;
	self->strokeColor.alpha = 1.0;
	self->fontSize = 12.0;
	return self;
}
void DaoxGraphicsState_Delete( DaoxGraphicsState *self )
{
	if( self->strokeGradient ) DaoxColorGradient_Delete( self->strokeGradient );
	if( self->font ) DaoGC_DecRC( (DaoValue*) self->font );
	DaoCdata_FreeCommon( (DaoCdata*) self );
	dao_free( self );
}
void DaoxGraphicsState_Copy( DaoxGraphicsState *self, DaoxGraphicsState *other )
{
	DaoGC_ShiftRC( (DaoValue*) other->font, (DaoValue*) self->font );
	self->font = other->font;
	self->dash = other->dash;
	self->junction = other->junction;
	self->fontSize = other->fontSize;
	self->strokeWidth = other->strokeWidth;
	self->transform = other->transform;
	self->strokeColor = other->strokeColor;
	self->fillColor = other->fillColor;
	memcpy( self->dashPattern, other->dashPattern, other->dash*sizeof(float) );
}
void DaoxGraphicsState_SetDashPattern( DaoxGraphicsState *self, float pat[], int n )
{
	if( n > 10 ) n = 10;
	self->dash = n;
	memcpy( self->dashPattern, pat, n*sizeof(float) );
}
void DaoxGraphicsState_SetFont( DaoxGraphicsState *self, DaoxFont *font, float size )
{
	DaoGC_ShiftRC( (DaoValue*) font, (DaoValue*) self->font );
	self->font = font;
	self->fontSize = size;
}

DaoxGraphicsState* DaoxGraphicsScene_GetOrPushState( DaoxGraphicsScene *self );



DaoxGraphicsItem* DaoxGraphicsItem_New( DaoxGraphicsScene *scene, int shape )
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
	self->visible = 1;
	self->scene = scene;
	self->shape = shape;
	self->points = DaoxPointArray_New();
	self->gdata = DaoxGraphicsData_New( self );
	self->state = DaoxGraphicsScene_GetOrPushState( scene );
	if( shape == DAOX_GS_POLYGON || shape == DAOX_GS_PATH ) self->path = DaoxPath_New();
	return self;
}
void DaoxGraphicsItem_Delete( DaoxGraphicsItem *self )
{
	if( self->children ) DArray_Delete( self->children );
	if( self->path ) DaoxPath_Delete( self->path );
	DaoxGraphicsData_Delete( self->gdata );
	DaoCdata_FreeCommon( (DaoCdata*) self );
	dao_free( self );
}




void DaoxGraphicsLine_Set( DaoxGraphicsLine *self, float x1, float y1, float x2, float y2 )
{
	assert( self->ctype == daox_type_graphics_line );
	DaoxPointArray_Resize( self->points, 2 );
	self->points->points[0].x = x1;
	self->points->points[0].y = y1;
	self->points->points[1].x = x2;
	self->points->points[1].y = y2;
}

void DaoxGraphicsRect_Set( DaoxGraphicsRect *self, float x1, float y1, float x2, float y2, float rx, float ry )
{
	self->roundCorner = (rx > 1E-24) && (ry > 1E-24);
	assert( self->ctype == daox_type_graphics_rect );
	DaoxPointArray_Resize( self->points, 2 + self->roundCorner );
	self->points->points[0].x = x1;
	self->points->points[0].y = y1;
	self->points->points[1].x = x2;
	self->points->points[1].y = y2;
	self->points->points[2].x = rx;
	self->points->points[2].y = ry;
}

void DaoxGraphicsCircle_Set( DaoxGraphicsCircle *self, float x, float y, float r )
{
	assert( self->ctype == daox_type_graphics_circle );
	DaoxPointArray_Resize( self->points, 2 );
	self->points->points[0].x = x;
	self->points->points[0].y = y;
	self->points->points[1].x = r;
	self->points->points[1].y = r;
}

void DaoxGraphicsEllipse_Set( DaoxGraphicsEllipse *self, float x, float y, float rx, float ry )
{
	assert( self->ctype == daox_type_graphics_ellipse );
	DaoxPointArray_Resize( self->points, 2 );
	self->points->points[0].x = x;
	self->points->points[0].y = y;
	self->points->points[1].x = rx;
	self->points->points[1].y = ry;
}



void DaoxGraphicsPolyline_Add( DaoxGraphicsPolyline *self, float x, float y )
{
	assert( self->ctype == daox_type_graphics_polyline );
	DaoxPointArray_PushXY( self->points, x, y );
}

void DaoxGraphicsPolygon_Add( DaoxGraphicsPolygon *self, float x, float y )
{
	assert( self->ctype == daox_type_graphics_polygon );
	DaoxPointArray_PushXY( self->points, x, y );
}

void DaoxGraphicsPath_SetRelativeMode( DaoxGraphicsPath *self, int relative )
{
	DaoxPath_SetRelativeMode( self->path, relative );
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
void DaoxGraphicsPath_ArcTo( DaoxGraphicsPath *self, float x, float y, float degrees )
{
	assert( self->ctype == daox_type_graphics_path );
	DaoxPath_ArcTo( self->path, x, y, degrees );
}
void DaoxGraphicsPath_ArcBy( DaoxGraphicsPath *self, float cx, float cy, float degrees )
{
	assert( self->ctype == daox_type_graphics_path );
	DaoxPath_ArcBy( self->path, cx, cy, degrees );
}
void DaoxGraphicsPath_QuadTo( DaoxGraphicsPath *self, float cx, float cy, float x, float y )
{
	assert( self->ctype == daox_type_graphics_path );
	DaoxPath_QuadTo( self->path, cx, cy, x, y );
}
void DaoxGraphicsPath_CubicTo( DaoxGraphicsPath *self, float cx, float cy, float x, float y )
{
	assert( self->ctype == daox_type_graphics_path );
	DaoxPath_CubicTo( self->path, cx, cy, x, y );
}
void DaoxGraphicsPath_CubicTo2( DaoxGraphicsLine *self, float cx0, float cy0, float cx, float cy, float x, float y )
{
	assert( self->ctype == daox_type_graphics_path );
	DaoxPath_CubicTo2( self->path, cx0, cy0, cx, cy, x, y );
}
void DaoxGraphicsPath_Close( DaoxGraphicsPath *self )
{
	assert( self->ctype == daox_type_graphics_path );
	DaoxPath_Close( self->path );
}





static void DaoxGraphicsiItem_ResetData( DaoxGraphicsItem *self )
{
	DaoxGraphicsData_Reset( self->gdata );
}

void DaoxGraphicsLine_UpdateData( DaoxGraphicsLine *self, DaoxGraphicsScene *scene )
{
	int i, n = self->points->count;
	DaoxGraphicsData_Init( self->gdata, self );
	for(i=0; i<n; i+=2){
		DaoxPoint P1 = self->points->points[i];
		DaoxPoint P2 = self->points->points[i+1];
		self->gdata->dashState = 0;
		self->gdata->dashLength = self->state->dashPattern[0];
		self->gdata->currentOffset = 0.0;
		self->gdata->currentLength = DaoxDistance( P1, P2 );
		DaoxGraphicsData_MakeLine( self->gdata, P1, P2 );
	}
}
void DaoxGraphicsRect_UpdateData( DaoxGraphicsRect *self, DaoxGraphicsScene *scene )
{
	int i, n = self->points->count;
	DaoxGraphicsData_Init( self->gdata, self );
	for(i=0; i<n; i+=(2+self->roundCorner)){
		DaoxPoint P1 = self->points->points[i];
		DaoxPoint P2 = self->points->points[i+1];
		float rx = 0.0, ry = 0.0;
		if( self->roundCorner ){
			DaoxPoint P3 = self->points->points[i+2];
			rx = P3.x;
			ry = P3.y;
		}
		DaoxGraphicsData_MakeRect( self->gdata, P1, P2, rx, ry );
	}
}
void DaoxGraphicsEllipse_UpdateData( DaoxGraphicsEllipse *self, DaoxGraphicsScene *scene )
{
	DaoxTransform transform = {0.0,0.0,0.0,0.0,0.0,0.0};
	float CX = self->points->points[0].x;
	float CY = self->points->points[0].y;
	float RX = self->points->points[1].x;
	float RY = self->points->points[1].y;
	float W = self->state->strokeWidth;

	DaoxGraphicsData_Init( self->gdata, self );
	self->gdata->junction = DAOX_JUNCTION_FLAT;
	self->gdata->maxlen = 3.0 * log(RX + RY + W + 1.0) / log(2.0);
	self->gdata->maxdiff = 0.5 / (RX + RY + W + 1.0);

	transform.Bx = CX;
	transform.By = CY;
	self->gdata->transform = & transform;

	if( fabs( RX - RY ) < 1E-16 ){
		if( RX <= 10 ){
			self->gdata->maxlen = 1.0;
			transform.Axx = transform.Ayy = RX / 10.0;
			DaoxPath_ExportGraphicsData( scene->smallCircle, self->gdata );
		}else{
			transform.Axx = transform.Ayy = RX / 100.0;
			DaoxPath_ExportGraphicsData( scene->largeCircle, self->gdata );
		}
		self->gdata->transform = NULL;
		return;
	}
	if( RX > RY ){
		if( RX < 3.0 * RY ){
			transform.Axx = RX / 200.0;
			transform.Ayy = RY / 100.0;
			DaoxPath_ExportGraphicsData( scene->wideEllipse, self->gdata );
		}else{
			transform.Axx = RX / 400.0;
			transform.Ayy = RY / 100.0;
			DaoxPath_ExportGraphicsData( scene->narrowEllipse, self->gdata );
		}
	}else{
		transform.Axx = transform.Ayy = 0.0;
		if( RY < 3.0 * RX ){
			transform.Axy = RX / 100.0;
			transform.Ayx = RY / 200.0;
			DaoxPath_ExportGraphicsData( scene->wideEllipse, self->gdata );
		}else{
			transform.Axy = RX / 100.0;
			transform.Ayx = RY / 400.0;
			DaoxPath_ExportGraphicsData( scene->narrowEllipse, self->gdata );
		}
	}
	self->gdata->transform = NULL;
}
void DaoxGraphicsCircle_UpdateData( DaoxGraphicsCircle *self, DaoxGraphicsScene *scene )
{
	DaoxGraphicsEllipse_UpdateData( self, scene );
}
void DaoxGraphicsPolyline_UpdateData( DaoxGraphicsPolyline *self, DaoxGraphicsScene *scene )
{
	int i, jt = self->state->junction;

	DaoxGraphicsiItem_ResetData( self );
	DaoxGraphicsData_Init( self->gdata, self );
	self->gdata->dashState = 0;
	self->gdata->dashLength = self->state->dashPattern[0];
	self->gdata->currentOffset = 0.0;
	self->gdata->currentLength = 0.0;
	for(i=1; i<self->points->count; ++i){
		DaoxPoint start = self->points->points[i-1];
		DaoxPoint end = self->points->points[i];
		self->gdata->currentLength += DaoxDistance( start, end );
	}
	for(i=1; i<self->points->count; ++i){
		float pos = self->gdata->currentOffset;
		DaoxPoint start = self->points->points[i-1];
		DaoxPoint end = self->points->points[i];
		DaoxGraphicsData_MakeLine( self->gdata, start, end );
		if( i >= 2 ){
			DaoxPoint prev = self->points->points[i-2];
			DaoxGraphicsData_MakeJunction( self->gdata, prev, start, end, pos, jt );
		}
	}
}
void DaoxGraphicsPolygon_UpdateData( DaoxGraphicsPolygon *self, DaoxGraphicsScene *scene )
{
	DaoxGraphicsiItem_ResetData( self );
	if( self->points->count == 0 ) return;
	if( self->path->first->refined.first == NULL ){
		DaoxPoint point, *points = self->points->points;
		int i, n = self->points->count;
		DaoxPath_Reset( self->path );
		DaoxPath_MoveTo( self->path, points[0].x, points[0].y );
		point = points[0];
		for(i=1; i<n; ++i){
			DaoxPath_LineTo( self->path, points[i].x - point.x, points[i].y - point.y );
			point = points[i];
		}
		DaoxPath_Close( self->path );
		DaoxPath_Preprocess( self->path, scene->triangulator );
	}
	DaoxGraphicsData_Init( self->gdata, self );
	DaoxPath_ExportGraphicsData( self->path, self->gdata );
	printf( "DaoxGraphicsPolygon_UpdateData: %i\n", self->gdata->strokeTriangles->count );
}
void DaoxGraphicsPath_UpdateData( DaoxGraphicsPath *self, DaoxGraphicsScene *scene )
{
	DaoxGraphicsiItem_ResetData( self );
	if( self->path->first->refined.first == NULL )
		DaoxPath_Preprocess( self->path, scene->triangulator );
	DaoxGraphicsData_Init( self->gdata, self );
	DaoxPath_ExportGraphicsData( self->path, self->gdata );
}
void DaoxGraphicsText_UpdateData( DaoxGraphicsText *self, DaoxGraphicsScene *scene )
{
	int i, j, jt = DAOX_JUNCTION_FLAT;
	float scale, offset, maxlen, maxdiff;
	float gscale = DaoxGraphicsScene_Scale( scene );
	float width = self->state->strokeWidth;
	float size = self->state->fontSize;
	DaoxFont *font = self->state->font;
	DaoxGlyph *glyph;

	DaoxGraphicsiItem_ResetData( self );

	if( self->codepoint == 0 ) return;
	if( font == NULL ) return;
	
	scale = size / (float)font->fontHeight;
	maxlen = 8.0 * font->fontHeight / size; 
	maxdiff = 2.0 / size;

	DaoxGraphicsData_Init( self->gdata, self );
	self->gdata->junction = DAOX_JUNCTION_FLAT;
	self->gdata->maxlen = maxlen;
	self->gdata->maxdiff = maxdiff;

	glyph = DaoxFont_GetCharGlyph( font, self->codepoint );
	DaoxPath_ExportGraphicsData( glyph->shape, self->gdata );
}

int DaoxGraphicsItem_UpdateData( DaoxGraphicsItem *self, DaoxGraphicsScene *scene )
{
	float scale = DaoxGraphicsScene_Scale( scene );
	daoint i;

	if( self->visible == 0 ){
		DaoxGraphicsiItem_ResetData( self );
		self->gdata->scale = scale;
		return 1;
	}
	if( scale > 2*self->gdata->scale || 2*scale < self->gdata->scale ){
		DaoxGraphicsiItem_ResetData( self );
	}
	if( self->gdata->strokePoints->count || self->gdata->fillPoints->count ) return 0;
	switch( self->shape ){
	case DAOX_GS_LINE     : DaoxGraphicsLine_UpdateData( self, scene );     break;
	case DAOX_GS_RECT     : DaoxGraphicsRect_UpdateData( self, scene );     break;
	case DAOX_GS_CIRCLE   : DaoxGraphicsCircle_UpdateData( self, scene );   break;
	case DAOX_GS_ELLIPSE  : DaoxGraphicsEllipse_UpdateData( self, scene );  break;
	case DAOX_GS_POLYLINE : DaoxGraphicsPolyline_UpdateData( self, scene ); break;
	case DAOX_GS_POLYGON  : DaoxGraphicsPolygon_UpdateData( self, scene );  break;
	case DAOX_GS_PATH     : DaoxGraphicsPath_UpdateData( self, scene );     break;
	case DAOX_GS_TEXT     : DaoxGraphicsText_UpdateData( self, scene );     break;
	}
	DaoxGraphicsData_MakeFillGradient( self->gdata );
	self->gdata->scale = scale;
	
	/* TODO better handling: */
	if( self->bounds.right > self->bounds.left + 1 ) return 1;
	if( self->gdata->strokePoints->count ){
		DaoxBounds_Init( & self->bounds, self->gdata->strokePoints->points[0] );
	}else if( self->gdata->fillPoints->count ){
		DaoxBounds_Init( & self->bounds, self->gdata->fillPoints->points[0] );
	}
	for(i=0; i<self->gdata->strokePoints->count; ++i){
		DaoxBounds_Update( & self->bounds, self->gdata->strokePoints->points[i] );
	}
	for(i=0; i<self->gdata->fillPoints->count; ++i){
		DaoxBounds_Update( & self->bounds, self->gdata->fillPoints->points[i] );
	}
	if( self->children == NULL ) return 1;
	for(i=0; i<self->children->size; ++i){
		DaoxGraphicsItem *item = (DaoxGraphicsItem*) self->children->items.pVoid[i];
		DaoxGraphicsItem_UpdateData( item, scene );
		if( item->bounds.left < self->bounds.left ) self->bounds.left = item->bounds.left;
		if( item->bounds.right > self->bounds.right ) self->bounds.right = item->bounds.right;
		if( item->bounds.bottom < self->bounds.bottom ) self->bounds.bottom = item->bounds.bottom;
		if( item->bounds.top > self->bounds.top ) self->bounds.top = item->bounds.top;
	}
	return 1;
}








DaoxGraphicsScene* DaoxGraphicsScene_New()
{
	int i;
	DaoxTransform X2 = { 2.0, 0.0, 0.0, 1.0, 0.0, 0.0 };
	DaoxTransform X4 = { 4.0, 0.0, 0.0, 1.0, 0.0, 0.0 };

	DaoxGraphicsScene *self = (DaoxGraphicsScene*) dao_calloc( 1, sizeof(DaoxGraphicsScene) );
	DaoCdata_InitCommon( (DaoCdata*) self, daox_type_graphics_scene );
	self->items = DArray_New(D_VALUE);
	self->states = DArray_New(D_VALUE);
	self->quarterArc = DaoxPath_New();
	self->quarterCircle = DaoxPath_New();
	self->smallCircle = DaoxPath_New();
	self->largeCircle = DaoxPath_New();
	self->wideEllipse = DaoxPath_New();
	self->narrowEllipse = DaoxPath_New();
	self->transform = X2;
	self->transform.Axx = 1.0;
	self->triangulator = DaoxTriangulator_New();

	DaoxPath_MoveTo( self->quarterArc,  100.0, 0.0 );
	DaoxPath_ArcBy( self->quarterArc,  0.0, 0.0, 90 );

	DaoxPath_MoveTo( self->quarterCircle, 0, 0 );
	DaoxPath_LineTo( self->quarterCircle,  100.0, 0.0 );
	DaoxPath_ArcTo( self->quarterCircle,  0.0, 100.0, 90 );
	DaoxPath_Close( self->quarterCircle );

	/* less accurate approximation for small circle: */
	DaoxPath_MoveTo( self->smallCircle, -10.0, 0 );
	DaoxPath_ArcTo2( self->smallCircle,  10.0, 0, 180, 180 );
	DaoxPath_ArcTo2( self->smallCircle, -10.0, 0, 180, 180 );
	DaoxPath_Close( self->smallCircle );

	DaoxPath_MoveTo( self->largeCircle, -100, 0 );
	DaoxPath_ArcTo( self->largeCircle,  100.0, 0, 180 );
	DaoxPath_ArcTo( self->largeCircle, -100.0, 0, 180 );
	DaoxPath_Close( self->largeCircle );

	DaoxPath_ImportPath( self->wideEllipse, self->largeCircle, & X2 );
	DaoxPath_ImportPath( self->narrowEllipse, self->largeCircle, & X4 );

	DaoxPath_Preprocess( self->quarterArc, self->triangulator );
	DaoxPath_Preprocess( self->quarterCircle, self->triangulator );
	DaoxPath_Preprocess( self->smallCircle, self->triangulator );
	DaoxPath_Preprocess( self->largeCircle, self->triangulator );
	DaoxPath_Preprocess( self->wideEllipse, self->triangulator );
	DaoxPath_Preprocess( self->narrowEllipse, self->triangulator );

	for(i=0; i<=DAOX_ARCS; i++){
		float angle2 = (i+1.0) * 180 / (float)DAOX_ARCS;
		float angle = (i+1.0) * M_PI / (float)DAOX_ARCS;
		float cosine = cos( 0.5 * angle );
		float sine = sin( 0.5 * angle );
		self->smallArcs[i] = DaoxPath_New();
		self->largeArcs[i] = DaoxPath_New();
		DaoxPath_MoveTo( self->smallArcs[i], 0.0, 0.0 );
		DaoxPath_MoveTo( self->largeArcs[i], 0.0, 0.0 );
		DaoxPath_LineTo( self->smallArcs[i], 10.0*cosine, -10.0*sine );
		DaoxPath_LineTo( self->largeArcs[i], 100.0*cosine, -100.0*sine );
		DaoxPath_ArcTo2( self->smallArcs[i], 10.0*cosine, 10.0*sine, angle2, 180.0 );
		DaoxPath_ArcTo( self->largeArcs[i],  100.0*cosine, 100.0*sine, angle2 );
		DaoxPath_Close( self->smallArcs[i] );
		DaoxPath_Close( self->largeArcs[i] );
		DaoxPath_Preprocess( self->smallArcs[i], self->triangulator );
		DaoxPath_Preprocess( self->largeArcs[i], self->triangulator );
	}
	return self;
}
void DaoxGraphicsScene_Delete( DaoxGraphicsScene *self )
{
	int i;
	for(i=0; i<DAOX_ARCS; i++){
		DaoxPath_Delete( self->smallArcs[i] );
		DaoxPath_Delete( self->largeArcs[i] );
	}
	DaoCdata_FreeCommon( (DaoCdata*) self );
	DArray_Delete( self->items );
	DArray_Delete( self->states );
	DaoxPath_Delete( self->quarterArc );
	DaoxPath_Delete( self->quarterCircle );
	DaoxPath_Delete( self->smallCircle );
	DaoxPath_Delete( self->largeCircle );
	DaoxPath_Delete( self->wideEllipse );
	DaoxPath_Delete( self->narrowEllipse );
	DaoxTriangulator_Delete( self->triangulator );
	dao_free( self );
}


void DaoxGraphicsScene_SetViewport( DaoxGraphicsScene *self, float left, float right, float bottom, float top )
{
	self->viewport.left = left;
	self->viewport.right = right;
	self->viewport.bottom = bottom;
	self->viewport.top = top;
}
float DaoxGraphicsScene_Scale( DaoxGraphicsScene *self )
{
	DaoxBounds box = self->viewport;
	float xscale = fabs( box.right - box.left ) / (self->defaultWidth + 1);
	float yscale = fabs( box.top - box.bottom ) / (self->defaultHeight + 1);
	return 0.5 * (xscale + yscale);
}
void DaoxGraphicsScene_SetBackground( DaoxGraphicsScene *self, DaoxColor color )
{
	self->background = color;
}
DaoxGraphicsState* DaoxGraphicsScene_GetCurrentState( DaoxGraphicsScene *self )
{
	if( self->states->size == 0 ) return NULL;
	return (DaoxGraphicsState*) self->states->items.pVoid[self->states->size-1];
}
DaoxGraphicsState* DaoxGraphicsScene_GetOrPushState( DaoxGraphicsScene *self )
{
	if( self->states->size == 0 ) DaoxGraphicsScene_PushState( self );
	return DaoxGraphicsScene_GetCurrentState( self );
}
DaoxGraphicsState* DaoxGraphicsScene_PushState( DaoxGraphicsScene *self )
{
	DaoxGraphicsState *prev = DaoxGraphicsScene_GetCurrentState( self );
	DaoxGraphicsState *state = DaoxGraphicsState_New();
	if( prev ) DaoxGraphicsState_Copy( state, prev );
	DArray_PushBack( self->states, state );
	return state;
}
void DaoxGraphicsScene_PopState( DaoxGraphicsScene *self )
{
	DArray_PopBack( self->states );
}

void DaoxGraphicsScene_SetStrokeWidth( DaoxGraphicsScene *self, float width )
{
	DaoxGraphicsState *state = DaoxGraphicsScene_GetOrPushState( self );
	state->strokeWidth = width;
}
void DaoxGraphicsScene_SetStrokeColor( DaoxGraphicsScene *self, DaoxColor color )
{
	DaoxGraphicsState *state = DaoxGraphicsScene_GetOrPushState( self );
	state->strokeColor = color;
}
void DaoxGraphicsScene_SetFillColor( DaoxGraphicsScene *self, DaoxColor color )
{
	DaoxGraphicsState *state = DaoxGraphicsScene_GetOrPushState( self );
	state->fillColor = color;
}
void DaoxGraphicsScene_SetDashPattern( DaoxGraphicsScene *self, float pat[], int n )
{
	DaoxGraphicsState *state = DaoxGraphicsScene_GetOrPushState( self );
	if( n > 10 ) n = 10;
	state->dash = n;
	memcpy( state->dashPattern, pat, n*sizeof(float) );
}
void DaoxGraphicsScene_SetFont( DaoxGraphicsScene *self, DaoxFont *font, float size )
{
	DaoxGraphicsState *state = DaoxGraphicsScene_GetOrPushState( self );
	DaoGC_ShiftRC( (DaoValue*) font, (DaoValue*) state->font );
	state->font = font;
	state->fontSize = size;
}


DaoxGraphicsLine* DaoxGraphicsScene_AddLine( DaoxGraphicsScene *self, float x1, float y1, float x2, float y2 )
{
	DaoxGraphicsLine *item = DaoxGraphicsItem_New( self, DAOX_GS_LINE );
	DaoxGraphicsLine_Set( item, x1, y1, x2, y2 );
	DArray_PushBack( self->items, item );
	return item;
}

DaoxGraphicsRect* DaoxGraphicsScene_AddRect( DaoxGraphicsScene *self, float x1, float y1, float x2, float y2, float rx, float ry )
{
	DaoxGraphicsRect *item = DaoxGraphicsItem_New( self, DAOX_GS_RECT );
	DaoxGraphicsRect_Set( item, x1, y1, x2, y2, rx, ry );
	DArray_PushBack( self->items, item );
	return item;
}

DaoxGraphicsCircle* DaoxGraphicsScene_AddCircle( DaoxGraphicsScene *self, float x, float y, float r )
{
	DaoxGraphicsCircle *item = DaoxGraphicsItem_New( self, DAOX_GS_CIRCLE );
	DaoxGraphicsCircle_Set( item, x, y, r );
	DArray_PushBack( self->items, item );
	return item;
}

DaoxGraphicsEllipse* DaoxGraphicsScene_AddEllipse( DaoxGraphicsScene *self, float x, float y, float rx, float ry )
{
	DaoxGraphicsEllipse *item = DaoxGraphicsItem_New( self, DAOX_GS_ELLIPSE );
	DaoxGraphicsEllipse_Set( item, x, y, rx, ry );
	DArray_PushBack( self->items, item );
	return item;
}

DaoxGraphicsPolyline* DaoxGraphicsScene_AddPolyline( DaoxGraphicsScene *self )
{
	DaoxGraphicsPolyline *item = DaoxGraphicsItem_New( self, DAOX_GS_POLYLINE );
	DArray_PushBack( self->items, item );
	return item;
}

DaoxGraphicsPolygon* DaoxGraphicsScene_AddPolygon( DaoxGraphicsScene *self )
{
	DaoxGraphicsPolygon *item = DaoxGraphicsItem_New( self, DAOX_GS_POLYGON );
	DArray_PushBack( self->items, item );
	return item;
}

DaoxGraphicsPath* DaoxGraphicsScene_AddPath( DaoxGraphicsScene *self )
{
	DaoxGraphicsPath *item = DaoxGraphicsItem_New( self, DAOX_GS_PATH );
	DArray_PushBack( self->items, item );
	return item;
}
void DaoxGraphicsText_AddCharItems( DaoxGraphicsText *self, const wchar_t *text, double x, double y )
{
	DaoxGlyph *glyph;
	DaoxGraphicsText *chitem;
	DaoxFont *font = self->state->font;
	DaoxTransform transform = {0.0,0.0,0.0,0.0,0.0,0.0};
	float width = self->state->strokeWidth;
	float size = self->state->fontSize;
	float scale = size / (float)font->fontHeight;
	float offset;

	transform.Axx = transform.Ayy = scale;
	transform.Bx = x;
	transform.By = y;

	if( self->path ) DaoxPath_Refine( self->path, 8.0/size, 2.0/size );

	offset = x;
	while( *text ){
		wchar_t ch = *text++;
		glyph = DaoxFont_GetCharGlyph( font, ch );
		if( glyph == NULL ) break;

		if( self->children == NULL ) self->children = DArray_New(D_VALUE);
		DaoxGraphicsScene_PushState( self->scene );
		chitem = DaoxGraphicsItem_New( self->scene, DAOX_GS_TEXT );
		DArray_PushBack( self->children, chitem );
		DaoxGraphicsScene_PopState( self->scene );
		/* Set codepoint to zero if the glyph is empty: */
		chitem->codepoint = glyph->shape->triangles->count ? ch : 0;

		transform.Bx = offset;
		if( self->path ){
			float p = 0.0, adv = 0.5 * (scale * glyph->advanceWidth + width);
			DaoxPathSegment *seg1 = DaoxPath_LocateByDistance( self->path, offset+adv, &p );
			DaoxPathSegment *seg2 = DaoxPath_LocateByDistance( self->path, offset, &p );
			if( seg1 == NULL ) seg1 = seg2;
			if( seg2 ){
				float dx = seg1->P2.x - seg1->P1.x;
				float dy = seg1->P2.y - seg1->P1.y;
				DaoxTransform_RotateXAxisTo( & transform, dx, dy );
				DaoxTransform_SetScale( & transform, scale, scale );
				transform.Bx = (1.0 - p) * seg2->P1.x + p * seg2->P2.x;
				transform.By = (1.0 - p) * seg2->P1.y + p * seg2->P2.y;
			}
		}
		chitem->state->transform = transform;
		offset += scale * glyph->advanceWidth + width;
		chitem = NULL;
	}
}
DaoxGraphicsText* DaoxGraphicsScene_AddText( DaoxGraphicsScene *self, const wchar_t *text, float x, float y )
{
	DaoxGraphicsPath *item;
	DaoxGraphicsState *state;
	if( self->states->size == 0 ) return NULL;
	state = DaoxGraphicsScene_GetOrPushState( self );
	if( state->font == NULL ) return NULL;

	item = DaoxGraphicsItem_New( self, DAOX_GS_TEXT );
	DArray_PushBack( self->items, item );
	DaoxGraphicsText_AddCharItems( item, text, x, y );
	return item;
}
DaoxGraphicsText* DaoxGraphicsScene_AddPathText( DaoxGraphicsScene *self, const wchar_t *text, DaoxPath *path )
{
	DaoxGraphicsPath *item;
	DaoxGraphicsState *state;
	if( self->states->size == 0 ) return NULL;
	state = DaoxGraphicsScene_GetOrPushState( self );
	if( state->font == NULL ) return NULL;

	item = DaoxGraphicsItem_New( self, DAOX_GS_TEXT );
	DArray_PushBack( self->items, item );
	if( item->path == NULL ) item->path = DaoxPath_New();
	DaoxPath_Reset( item->path );
	DaoxPath_ImportPath( item->path, path, NULL );
	DaoxPath_Preprocess( item->path, self->triangulator );
	DaoxGraphicsText_AddCharItems( item, text, 0, 0 );
	return item;
}








static void ITEM_SetVisible( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsItem *self = (DaoxGraphicsItem*) p[0];
	self->visible = p[1]->xEnum.value;
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
	{ ITEM_SetVisible,  "SetVisible( self : GraphicsItem, visible :enum<false,true> )" },
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
static void LINE_Set( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsLine *self = (DaoxGraphicsLine*) p[0];
	LINE_SetData( self, p + 1 );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static DaoFuncItem DaoxGraphicsLineMeths[]=
{
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
	float rx = p[4]->xFloat.value;
	float ry = p[5]->xFloat.value;
	DaoxGraphicsRect_Set( self, x1, y1, x2, y2, rx, ry );
}
static void RECT_Set( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsRect *self = (DaoxGraphicsRect*) p[0];
	RECT_SetData( self, p + 1 );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static DaoFuncItem DaoxGraphicsRectMeths[]=
{
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
static void CIRCLE_Set( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsCircle *self = (DaoxGraphicsCircle*) p[0];
	CIRCLE_SetData( self, p + 1 );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static DaoFuncItem DaoxGraphicsCircleMeths[]=
{
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
static void ELLIPSE_Set( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsEllipse *self = (DaoxGraphicsEllipse*) p[0];
	ELLIPSE_SetData( self, p + 1 );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static DaoFuncItem DaoxGraphicsEllipseMeths[]=
{
	{ ELLIPSE_Set,  "Set( self : GraphicsEllipse, cx = 0.0, cy = 0.0, rx = 1.0, ry = 1.0 ) => GraphicsEllipse" },
	{ NULL, NULL }
};

DaoTypeBase DaoxGraphicsEllipse_Typer =
{
	"GraphicsEllipse", NULL, NULL, (DaoFuncItem*) DaoxGraphicsEllipseMeths,
	{ & DaoxGraphicsItem_Typer, NULL }, { NULL },
	(FuncPtrDel)DaoxGraphicsItem_Delete, DaoxGraphicsItem_GetGCFields
};





static void POLYLINE_Add( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsPolyline *self = (DaoxGraphicsPolyline*) p[0];
	float x = p[1]->xFloat.value;
	float y = p[2]->xFloat.value;
	DaoxGraphicsPolyline_Add( self, x, y );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static DaoFuncItem DaoxGraphicsPolylineMeths[]=
{
	{ POLYLINE_Add,  "Add( self : GraphicsPolyline, x: float, y: float ) => GraphicsPolyline" },
	{ NULL, NULL }
};

DaoTypeBase DaoxGraphicsPolyline_Typer =
{
	"GraphicsPolyline", NULL, NULL, (DaoFuncItem*) DaoxGraphicsPolylineMeths,
	{ & DaoxGraphicsItem_Typer, NULL }, { NULL },
	(FuncPtrDel)DaoxGraphicsItem_Delete, DaoxGraphicsItem_GetGCFields
};





static void POLYGON_Add( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsPolygon *self = (DaoxGraphicsPolygon*) p[0];
	float x = p[1]->xFloat.value;
	float y = p[2]->xFloat.value;
	DaoxGraphicsPolygon_Add( self, x, y );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static DaoFuncItem DaoxGraphicsPolygonMeths[]=
{
	{ POLYGON_Add,   "Add( self : GraphicsPolygon, x : float, y : float ) => GraphicsPolygon" },
	{ NULL, NULL }
};

DaoTypeBase DaoxGraphicsPolygon_Typer =
{
	"GraphicsPolygon", NULL, NULL, (DaoFuncItem*) DaoxGraphicsPolygonMeths,
	{ & DaoxGraphicsItem_Typer, NULL }, { NULL },
	(FuncPtrDel)DaoxGraphicsItem_Delete, DaoxGraphicsItem_GetGCFields
};






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
	DaoxGraphicsPath_ArcTo( self, x, y, d );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void PATH_ArcBy( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsPath *self = (DaoxGraphicsPath*) p[0];
	float x = p[1]->xFloat.value;
	float y = p[2]->xFloat.value;
	float d = p[3]->xFloat.value;
	DaoxGraphicsPath_ArcBy( self, x, y, d );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void PATH_QuadTo( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsPath *self = (DaoxGraphicsPath*) p[0];
	float cx = p[1]->xFloat.value;
	float cy = p[2]->xFloat.value;
	float x = p[3]->xFloat.value;
	float y = p[4]->xFloat.value;
	DaoxGraphicsPath_QuadTo( self, cx, cy, x, y );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void PATH_CubicTo( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsPath *self = (DaoxGraphicsPath*) p[0];
	float cx = p[1]->xFloat.value;
	float cy = p[2]->xFloat.value;
	float x = p[3]->xFloat.value;
	float y = p[4]->xFloat.value;
	DaoxGraphicsPath_CubicTo( self, cx, cy, x, y );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void PATH_CubicTo2( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsPath *self = (DaoxGraphicsPath*) p[0];
	float cx0 = p[1]->xFloat.value;
	float cy0 = p[2]->xFloat.value;
	float cx = p[3]->xFloat.value;
	float cy = p[4]->xFloat.value;
	float x = p[5]->xFloat.value;
	float y = p[6]->xFloat.value;
	DaoxGraphicsPath_CubicTo2( self, cx0, cy0, cx, cy, x, y );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void PATH_Close( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsPath *self = (DaoxGraphicsPath*) p[0];
	DaoxGraphicsPath_Close( self );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}

static void PATH_LineRelTo( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsPath *self = (DaoxGraphicsPath*) p[0];
	DaoxGraphicsPath_SetRelativeMode( self, 1 );
	PATH_LineTo( proc, p, N );
}
static void PATH_ArcRelTo( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsPath *self = (DaoxGraphicsPath*) p[0];
	DaoxGraphicsPath_SetRelativeMode( self, 1 );
	PATH_ArcTo( proc, p, N );
}
static void PATH_ArcRelBy( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsPath *self = (DaoxGraphicsPath*) p[0];
	DaoxGraphicsPath_SetRelativeMode( self, 1 );
	PATH_ArcBy( proc, p, N );
}
static void PATH_QuadRelTo( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsPath *self = (DaoxGraphicsPath*) p[0];
	DaoxGraphicsPath_SetRelativeMode( self, 1 );
	PATH_QuadTo( proc, p, N );
}
static void PATH_CubicRelTo( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsPath *self = (DaoxGraphicsPath*) p[0];
	DaoxGraphicsPath_SetRelativeMode( self, 1 );
	PATH_CubicTo( proc, p, N );
}
static void PATH_CubicRelTo2( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsPath *self = (DaoxGraphicsPath*) p[0];
	DaoxGraphicsPath_SetRelativeMode( self, 1 );
	PATH_CubicTo2( proc, p, N );
}
static void PATH_LineAbsTo( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsPath *self = (DaoxGraphicsPath*) p[0];
	DaoxGraphicsPath_SetRelativeMode( self, 0 );
	PATH_LineTo( proc, p, N );
}
static void PATH_ArcAbsTo( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsPath *self = (DaoxGraphicsPath*) p[0];
	DaoxGraphicsPath_SetRelativeMode( self, 0 );
	PATH_ArcTo( proc, p, N );
}
static void PATH_ArcAbsBy( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsPath *self = (DaoxGraphicsPath*) p[0];
	DaoxGraphicsPath_SetRelativeMode( self, 0 );
	PATH_ArcBy( proc, p, N );
}
static void PATH_QuadAbsTo( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsPath *self = (DaoxGraphicsPath*) p[0];
	DaoxGraphicsPath_SetRelativeMode( self, 0 );
	PATH_QuadTo( proc, p, N );
}
static void PATH_CubicAbsTo( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsPath *self = (DaoxGraphicsPath*) p[0];
	DaoxGraphicsPath_SetRelativeMode( self, 0 );
	PATH_CubicTo( proc, p, N );
}
static void PATH_CubicAbsTo2( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsPath *self = (DaoxGraphicsPath*) p[0];
	DaoxGraphicsPath_SetRelativeMode( self, 0 );
	PATH_CubicTo2( proc, p, N );
}
static DaoFuncItem DaoxGraphicsPathMeths[]=
{
	{ PATH_MoveTo,    "MoveTo( self : GraphicsPath, x : float, y : float ) => GraphicsPath" },

	{ PATH_LineRelTo,    "LineTo( self : GraphicsPath, x : float, y : float ) => GraphicsPath" },

	{ PATH_ArcRelTo,     "ArcTo( self : GraphicsPath, x : float, y : float, degrees : float ) => GraphicsPath" },

	{ PATH_ArcRelBy,     "ArcBy( self : GraphicsPath, cx : float, cy : float, degrees : float ) => GraphicsPath" },

	{ PATH_QuadRelTo,   "QuadTo( self : GraphicsPath, cx : float, cy : float, x : float, y : float ) => GraphicsPath" },

	{ PATH_CubicRelTo,   "CubicTo( self : GraphicsPath, cx : float, cy : float, x : float, y : float ) => GraphicsPath" },

	{ PATH_CubicRelTo2,  "CubicTo( self : GraphicsPath, cx0 : float, cy0 : float, cx : float, cy : float, x : float, y : float ) => GraphicsPath" },

	{ PATH_LineAbsTo,    "LineAbsTo( self : GraphicsPath, x : float, y : float ) => GraphicsPath" },

	{ PATH_ArcAbsTo,     "ArcAbsTo( self : GraphicsPath, x : float, y : float, degrees : float ) => GraphicsPath" },

	{ PATH_ArcAbsBy,     "ArcAbsBy( self : GraphicsPath, cx : float, cy : float, degrees : float ) => GraphicsPath" },

	{ PATH_QuadAbsTo,   "QuadAbsTo( self : GraphicsPath, cx : float, cy : float, x : float, y : float ) => GraphicsPath" },

	{ PATH_CubicAbsTo,   "CubicAbsTo( self : GraphicsPath, cx : float, cy : float, x : float, y : float ) => GraphicsPath" },

	{ PATH_CubicAbsTo2,  "CubicAbsTo( self : GraphicsPath, cx0 : float, cy0 : float, cx : float, cy : float, x : float, y : float ) => GraphicsPath" },

	{ PATH_Close,     "Close( self : GraphicsPath ) => GraphicsPath" },
	{ NULL, NULL }
};

DaoTypeBase DaoxGraphicsPath_Typer =
{
	"GraphicsPath", NULL, NULL, (DaoFuncItem*) DaoxGraphicsPathMeths,
	{ & DaoxGraphicsItem_Typer, NULL }, { NULL },
	(FuncPtrDel)DaoxGraphicsItem_Delete, DaoxGraphicsItem_GetGCFields
};




static DaoFuncItem DaoxGraphicsTextMeths[]=
{
	{ NULL, NULL }
};

DaoTypeBase DaoxGraphicsText_Typer =
{
	"GraphicsText", NULL, NULL, (DaoFuncItem*) DaoxGraphicsTextMeths,
	{ & DaoxGraphicsItem_Typer, NULL }, { NULL },
	(FuncPtrDel)DaoxGraphicsItem_Delete, DaoxGraphicsItem_GetGCFields
};




static void SCENE_New( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsScene *self = DaoxGraphicsScene_New();
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void SCENE_SetViewport( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsScene *self = (DaoxGraphicsScene*) p[0];
	self->viewport.left = p[1]->xFloat.value;
	self->viewport.right = p[2]->xFloat.value;
	self->viewport.bottom = p[3]->xFloat.value;
	self->viewport.top = p[4]->xFloat.value;
}
static void SCENE_GetViewport( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsScene *self = (DaoxGraphicsScene*) p[0];
	DaoProcess_NewFloat( proc, self->viewport.left );
	DaoProcess_NewFloat( proc, self->viewport.right );
	DaoProcess_NewFloat( proc, self->viewport.bottom );
	DaoProcess_NewFloat( proc, self->viewport.top );
	DaoProcess_PutTuple( proc, -4 );
}
static void SCENE_SetBackground( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxColor color;
	DaoxGraphicsScene *self = (DaoxGraphicsScene*) p[0];
	color.red   = p[1]->xFloat.value;
	color.green = p[2]->xFloat.value;
	color.blue  = p[3]->xFloat.value;
	color.alpha = p[4]->xFloat.value;
	DaoxGraphicsScene_SetBackground( self, color );
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
	float rx = p[5]->xFloat.value, ry = p[6]->xFloat.value;
	DaoxGraphicsRect *item = DaoxGraphicsScene_AddRect( self, x1, y1, x2, y2, rx, ry );
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
static void SCENE_AddPolyline( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsScene *self = (DaoxGraphicsScene*) p[0];
	DaoxGraphicsPolyline *item = DaoxGraphicsScene_AddPolyline( self );
	float x1 = p[1]->xFloat.value, y1 = p[2]->xFloat.value;
	float x2 = p[3]->xFloat.value, y2 = p[4]->xFloat.value;
	DaoxGraphicsPolyline_Add( item, x1, y1 );
	DaoxGraphicsPolyline_Add( item, x2, y2 );
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
static void SCENE_AddText( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsScene *self = (DaoxGraphicsScene*) p[0];
	DString *text = DaoValue_TryGetString( p[1] );
	float x = p[2]->xFloat.value;
	float y = p[3]->xFloat.value;
	DaoxGraphicsText *item = DaoxGraphicsScene_AddText( self, DString_GetWCS( text ), x, y );
	if( item == NULL ){
		DaoProcess_RaiseException( proc, DAO_ERROR, "no font is set" );
		return;
	}
	DaoProcess_PutValue( proc, (DaoValue*) item );
}
static void SCENE_AddText2( DaoProcess *proc, DaoValue *p[], int N )
{
	wchar_t *text = DaoValue_TryGetWCString( p[1] );
	DaoxGraphicsScene *self = (DaoxGraphicsScene*) p[0];
	DaoxGraphicsPath *path = (DaoxGraphicsPath*) p[2];
	DaoxGraphicsText *item = DaoxGraphicsScene_AddPathText( self, text, path->path );
	if( item == NULL ){
		DaoProcess_RaiseException( proc, DAO_ERROR, "no font is set" );
		return;
	}
	DaoProcess_PutValue( proc, (DaoValue*) item );
}
static void SCENE_PushState( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsScene *self = (DaoxGraphicsScene*) p[0];
	DaoxGraphicsState *state = DaoxGraphicsScene_PushState( self );
	DaoProcess_PutValue( proc, (DaoValue*) state );
}
static void SCENE_PopState( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsScene *self = (DaoxGraphicsScene*) p[0];
	DaoxGraphicsScene_PopState( self );
}



static void STATE_SetStrokeWidth( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsState *self = (DaoxGraphicsState*) p[0];
	self->strokeWidth = p[1]->xFloat.value;
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void STATE_SetStrokeColor( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsState *self = (DaoxGraphicsState*) p[0];
	self->strokeColor.red   = p[1]->xFloat.value;
	self->strokeColor.green = p[2]->xFloat.value;
	self->strokeColor.blue  = p[3]->xFloat.value;
	self->strokeColor.alpha = p[4]->xFloat.value;
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void STATE_SetFillColor( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsState *self = (DaoxGraphicsState*) p[0];
	self->fillColor.red   = p[1]->xFloat.value;
	self->fillColor.green = p[2]->xFloat.value;
	self->fillColor.blue  = p[3]->xFloat.value;
	self->fillColor.alpha = p[4]->xFloat.value;
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void STATE_SetJunction( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsState *self = (DaoxGraphicsState*) p[0];
	self->junction = p[1]->xEnum.value;
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void STATE_SetDash( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsState *self = (DaoxGraphicsState*) p[0];
	DaoArray *array = (DaoArray*) p[1];
	DaoxGraphicsState_SetDashPattern( self, array->data.f, array->size );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void STATE_SetTransform( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsState *self = (DaoxGraphicsState*) p[0];
	DaoArray *array = (DaoArray*) p[1];
	daoint n = array->size;
	if( n != 4 && n != 6 ){
		DaoProcess_RaiseException( proc, DAO_ERROR_PARAM, "need matrix with 4 or 6 elements" );
		return;
	}
	DaoxTransform_Set( & self->transform, array->data.f, n );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void STATE_MulTransform( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxTransform transform;
	DaoxGraphicsState *self = (DaoxGraphicsState*) p[0];
	DaoArray *array = (DaoArray*) p[1];
	daoint n = array->size;
	if( n != 4 && n != 6 ){
		DaoProcess_RaiseException( proc, DAO_ERROR_PARAM, "need matrix with 4 or 6 elements" );
		return;
	}
	DaoxTransform_Set( & transform, array->data.f, n );
	DaoxTransform_Multiply( & self->transform, transform );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void STATE_SetStrokeGradient( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsState *self = (DaoxGraphicsState*) p[0];
	if( self->strokeGradient == NULL ){
		self->strokeGradient = DaoxColorGradient_New( DAOX_GRADIENT_BASE );
		DaoGC_IncRC( (DaoValue*) self->strokeGradient );
	}
	DaoProcess_PutValue( proc, (DaoValue*) self->strokeGradient );
}
static void STATE_SetFillGradient( DaoProcess *proc, DaoValue *p[], int N, int type )
{
	DaoxGraphicsState *self = (DaoxGraphicsState*) p[0];
	if( self->fillGradient == NULL ){
		self->fillGradient = DaoxColorGradient_New( type );
		DaoGC_IncRC( (DaoValue*) self->fillGradient );
	}
	DaoProcess_PutValue( proc, (DaoValue*) self->fillGradient );
}
static void STATE_SetLinearGradient( DaoProcess *proc, DaoValue *p[], int N )
{
	STATE_SetFillGradient( proc, p, N, DAOX_GRADIENT_LINEAR );
}
static void STATE_SetRadialGradient( DaoProcess *proc, DaoValue *p[], int N )
{
	STATE_SetFillGradient( proc, p, N, DAOX_GRADIENT_RADIAL );
}
static void STATE_SetPathGradient( DaoProcess *proc, DaoValue *p[], int N )
{
	STATE_SetFillGradient( proc, p, N, DAOX_GRADIENT_PATH );
}
static void STATE_SetFont( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsState *self = (DaoxGraphicsState*) p[0];
	DaoxFont *font = (DaoxFont*) p[1];
	DaoxGraphicsState_SetFont( self, font, p[2]->xFloat.value );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void SCENE_Test( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsScene *self = (DaoxGraphicsScene*) p[0];
	int i, n = self->items->size;
	for(i=0; i<n; i++){
		DaoxGraphicsPath *item = (DaoxGraphicsItem*) self->items->items.pVoid[i];
		DaoxGraphicsItem_UpdateData( item, self );
	}
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
	{ SCENE_New,         "GraphicsScene()" },

	{ SCENE_SetViewport,   "SetViewport( self: GraphicsScene, left: float, right: float, bottom: float, top: float )" },

	{ SCENE_GetViewport,   "GetViewport( self: GraphicsScene ) => tuple<left:float,right:float,bottom:float,top:float>" },

	{ SCENE_SetBackground,  "SetBackground( self : GraphicsScene, red: float, green: float, blue: float, alpha = 1.0 ) => GraphicsScene" },

	{ SCENE_PushState,   "PushState( self: GraphicsScene ) => GraphicsState" },

	{ SCENE_PopState,    "PopState( self: GraphicsScene )" },

	{ SCENE_AddLine,   "AddLine( self: GraphicsScene, x1: float, y1: float, x2: float, y2: float ) => GraphicsLine" },

	{ SCENE_AddRect,   "AddRect( self: GraphicsScene, x1: float, y1: float, x2: float, y2: float, rx = 0.0, ry = 0.0 ) => GraphicsRect" },

	{ SCENE_AddCircle,    "AddCircle( self: GraphicsScene, x: float, y: float, r: float ) => GraphicsCircle" },

	{ SCENE_AddEllipse,   "AddEllipse( self: GraphicsScene, x: float, y: float, rx: float, ry: float ) => GraphicsEllipse" },

	{ SCENE_AddPolyline,  "AddPolyline( self: GraphicsScene, x1: float, y1: float, x2: float, y2: float ) => GraphicsPolyline" },

	{ SCENE_AddPolygon,   "AddPolygon( self: GraphicsScene ) => GraphicsPolygon" },

	{ SCENE_AddPath,      "AddPath( self: GraphicsScene ) => GraphicsPath" },

	{ SCENE_AddText,      "AddText( self: GraphicsScene, text : string, x :float, y :float ) => GraphicsText" },

	{ SCENE_AddText2,     "AddText( self: GraphicsScene, text : string, path :GraphicsPath ) => GraphicsText" },

	{ SCENE_Test,         "Test( self: GraphicsScene )" },
	{ NULL, NULL }
};

DaoTypeBase DaoxGraphicsScene_Typer =
{
	"GraphicsScene", NULL, NULL, (DaoFuncItem*) DaoxGraphicsSceneMeths, {0}, {0},
	(FuncPtrDel)DaoxGraphicsScene_Delete, DaoxGraphicsScene_GetGCFields
};


static DaoFuncItem DaoxGraphicsStateMeths[]=
{
	{ STATE_SetStrokeWidth,  "SetStrokeWidth( self : GraphicsState, width = 1.0 ) => GraphicsState" },

	{ STATE_SetStrokeColor,  "SetStrokeColor( self : GraphicsState, red: float, green: float, blue: float, alpha = 1.0 ) => GraphicsState" },

	{ STATE_SetFillColor,  "SetFillColor( self : GraphicsState, red: float, green: float, blue: float, alpha = 1.0 ) => GraphicsState" },

	{ STATE_SetJunction, "SetJunction( self : GraphicsState, junction: enum<none,sharp,flat,round> = $sharp ) => GraphicsState" },

	{ STATE_SetDash, "SetDashPattern( self : GraphicsState, pattern = [3.0,2.0] ) => GraphicsState" },

	{ STATE_SetTransform, "SetTransform( self : GraphicsState, transform : array<float> ) => GraphicsState" },
	{ STATE_MulTransform, "MulTransform( self : GraphicsState, transform : array<float> ) => GraphicsState" },

	{ STATE_SetStrokeGradient, "SetStrokeGradient( self : GraphicsState ) => ColorGradient" },

	{ STATE_SetLinearGradient, "SetLinearGradient( self : GraphicsState ) => LinearGradient" },

	{ STATE_SetRadialGradient, "SetRadialGradient( self : GraphicsState ) => RadialGradient" },

	//{ STATE_SetPathGradient,   "SetPathGradient( self : GraphicsState ) => PathGradient" },

	{ STATE_SetFont,      "SetFont( self: GraphicsState, font : Font, size = 12.0 ) => GraphicsState" },
	{ NULL, NULL }
};


DaoTypeBase DaoxGraphicsState_Typer =
{
	"GraphicsState", NULL, NULL, (DaoFuncItem*) DaoxGraphicsStateMeths, {0}, {0},
	(FuncPtrDel)DaoxGraphicsState_Delete, NULL 
};





DAO_DLL int DaoTriangulator_OnLoad( DaoVmSpace *vmSpace, DaoNamespace *ns );
DAO_DLL int DaoFont_OnLoad( DaoVmSpace *vmSpace, DaoNamespace *ns );
DAO_DLL int DaoGLUT_OnLoad( DaoVmSpace *vmSpace, DaoNamespace *ns );

DAO_DLL int DaoOnLoad( DaoVmSpace *vmSpace, DaoNamespace *ns )
{
	DaoFont_OnLoad( vmSpace, ns );

	DaoNamespace_TypeDefine( ns, "tuple<red:float,green:float,blue:float,alpha:float>", "Color" );

	daox_type_color_gradient = DaoNamespace_WrapType( ns, & DaoxColorGradient_Typer, 0 );
	daox_type_linear_gradient = DaoNamespace_WrapType( ns, & DaoxLinearGradient_Typer, 0 );
	daox_type_radial_gradient = DaoNamespace_WrapType( ns, & DaoxRadialGradient_Typer, 0 );
	daox_type_path_gradient = DaoNamespace_WrapType( ns, & DaoxPathGradient_Typer, 0 );

	daox_type_graphics_state = DaoNamespace_WrapType( ns, & DaoxGraphicsState_Typer, 0 );
	daox_type_graphics_scene = DaoNamespace_WrapType( ns, & DaoxGraphicsScene_Typer, 0 );
	daox_type_graphics_item = DaoNamespace_WrapType( ns, & DaoxGraphicsItem_Typer, 0 );
	daox_type_graphics_line = DaoNamespace_WrapType( ns, & DaoxGraphicsLine_Typer, 0 );
	daox_type_graphics_rect = DaoNamespace_WrapType( ns, & DaoxGraphicsRect_Typer, 0 );
	daox_type_graphics_circle = DaoNamespace_WrapType( ns, & DaoxGraphicsCircle_Typer, 0 );
	daox_type_graphics_ellipse = DaoNamespace_WrapType( ns, & DaoxGraphicsEllipse_Typer, 0 );
	daox_type_graphics_polyline = DaoNamespace_WrapType( ns, & DaoxGraphicsPolyline_Typer, 0 );
	daox_type_graphics_polygon = DaoNamespace_WrapType( ns, & DaoxGraphicsPolygon_Typer, 0 );
	daox_type_graphics_path = DaoNamespace_WrapType( ns, & DaoxGraphicsPath_Typer, 0 );
	daox_type_graphics_text = DaoNamespace_WrapType( ns, & DaoxGraphicsText_Typer, 0 );

	DaoTriangulator_OnLoad( vmSpace, ns );
	DaoGLUT_OnLoad( vmSpace, ns );
	return 0;
}
