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



double DaoxGraphicsScene_Scale( DaoxGraphicsScene *self );



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
	double BxAx = B.x - A.x;
	double ByAy = B.y - A.y;
	double CxAx = C.x - A.x;
	double CyAy = C.y - A.y;
	double t = (CxAx * BxAx + CyAy * ByAy) / (BxAx * BxAx + ByAy * ByAy);
	return DaoxColorGradient_InterpolateColor( self, t );
}
DaoxColor DaoxColorGradient_ComputeRadialColor( DaoxColorGradient *self, DaoxPoint point )
{
	DaoxPoint C = self->points[0];
	DaoxPoint F = self->points[1];
	DaoxPoint G = point;
	double R = self->radius;
	double GxFx = G.x - F.x;
	double GyFy = G.y - F.y;
	double FxCx = F.x - C.x;
	double FyCy = F.y - C.y;
	double a = GxFx * GxFx + GyFy * GyFy;
	double b = 2.0 * (GxFx * FxCx + GyFy * FyCy);
	double c = FxCx * FxCx + FyCy * FyCy - R * R;
	double t = (- b + sqrt(b * b - 4.0 * a * c)) / (2.0 * a);
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


static void GRAD_AddStop( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxColorGradient *self = (DaoxColorGradient*) p[0];
	DaoxColor color = {0.0,0.0,0.0,0.0};
	DaoValue **tvalues = p[2]->xTuple.items;
	color.red = tvalues[0]->xFloat.value;
	color.green = tvalues[1]->xFloat.value;
	color.blue = tvalues[2]->xFloat.value;
	color.alpha = tvalues[3]->xFloat.value;
	DaoxColorGradient_Add( self, p[1]->xFloat.value, color );
}

static DaoFuncItem DaoxColorGradientMeths[]=
{
	{ GRAD_AddStop,  "AddStop( self : ColorGradient, at : float, color :tuple<red:float,green:float,blue:float,alpha:float> ) => ColorGradient" },
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
	self->points[0].x = p[1]->xDouble.value;
	self->points[0].y = p[2]->xDouble.value;
}
static void LGRAD_SetEnd( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxColorGradient *self = (DaoxColorGradient*) p[0];
	self->points[1].x = p[1]->xDouble.value;
	self->points[1].y = p[2]->xDouble.value;
}

static DaoFuncItem DaoxLinearGradientMeths[]=
{
	{ LGRAD_SetStart,  "SetStart( self : LinearGradient, x : double, y : double )" },
	{ LGRAD_SetEnd,    "SetEnd( self : LinearGradient, x : double, y : double )" },
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
	self->radius = p[1]->xDouble.value;
}

static DaoFuncItem DaoxRadialGradientMeths[]=
{
	{ LGRAD_SetStart,  "SetCenter( self : RadialGradient, x : double, y : double )" },
	{ LGRAD_SetEnd,    "SetFocus( self : RadialGradient, x : double, y : double )" },
	{ RGRAD_SetRadius, "SetRadius( self : RadialGradient, r : double )" },
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
	self->dashLength = 0.0;
	self->currentOffset = 0.0;
	self->currentLength = 0.0;
	self->junction = item->state->junction;
	self->strokeWidth = item->state->strokeWidth;
	self->item = item;
}
void DaoxGraphicsData_PushStrokeTriangle( DaoxGraphicsData *self, DaoxPoint A, DaoxPoint B, DaoxPoint C )
{
	int index = self->strokePoints->count;
	DaoxPointArray_Push( self->strokePoints, A );
	DaoxPointArray_Push( self->strokePoints, B );
	DaoxPointArray_Push( self->strokePoints, C );
	DaoxIntArray_Push( self->strokeTriangles, index );
	DaoxIntArray_Push( self->strokeTriangles, index+1 );
	DaoxIntArray_Push( self->strokeTriangles, index+2 );
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
	DaoxIntArray_Push( triangles, index+2 );
	DaoxIntArray_Push( triangles, index+3 );
	DaoxIntArray_Push( triangles, index );
}
void DaoxGraphicsData_PushStrokeQuad( DaoxGraphicsData *self, DaoxQuad quad )
{
	DaoxGraphicsData_PushQuad( self->strokePoints, self->strokeTriangles, quad );
}
void DaoxGraphicsData_PushFillQuad( DaoxGraphicsData *self, DaoxQuad quad )
{
	DaoxGraphicsData_PushQuad( self->fillPoints, self->fillTriangles, quad );
}
void DaoxIntArray_PushTriangle( DaoxIntArray *triangles, int A, int B, int C )
{
	DaoxIntArray_Push( triangles, A );
	DaoxIntArray_Push( triangles, B );
	DaoxIntArray_Push( triangles, C );
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
	double scale = DaoxGraphicsScene_Scale( self->item->scene );
	int i, k, m, n;

	if( gradient == NULL ) return;
	for(i=colors->count,n=points->count; i<n; ++i){
		DaoxColor C = DaoxColorGradient_ComputeColor( gradient, points->points[i] );
		DaoxColorArray_Push( colors, C );
	}
	printf( "before: %i\n", triangles->count );
	for(i=0,k=0; i<triangles->count; i+=3){
		double dAB, dBC, dCA, dmax = 0.0, diff = 0.0;
		int IA = triangles->values[i];
		int IB = triangles->values[i+1];
		int IC = triangles->values[i+2];
		DaoxPoint PA = points->points[IA];
		DaoxPoint PB = points->points[IB];
		DaoxPoint PC = points->points[IC];
		DaoxPoint PmAB, PmBC, PmCA;
		DaoxColor CA, CB, CC, CmAB, CmBC, CmCA;
		DaoxColor CtAB, CtBC, CtCA;

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
		if( dmax < 1.0 * scale ){
			triangles->values[k] = IA;
			triangles->values[k+1] = IB;
			triangles->values[k+2] = IC;
			k += 3;
			continue;
		}
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

		if( diff > 2E-3 || dmax > (gradient->gradient == DAOX_GRADIENT_LINEAR ? 50 : 20) ){
			int IBC = points->count;
			DaoxPointArray_Push( points, PmBC );
			DaoxColorArray_Push( colors, CtBC );
			DaoxIntArray_PushTriangle( triangles, IA, IB, IBC );
			DaoxIntArray_PushTriangle( triangles, IC, IA, IBC );
		}else{
			triangles->values[k] = IA;
			triangles->values[k+1] = IB;
			triangles->values[k+2] = IC;
			k += 3;
		}
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
	memcpy( self->dashPattern, other->dashPattern, other->dash*sizeof(double) );
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




void DaoxGraphicsLine_Set( DaoxGraphicsLine *self, double x1, double y1, double x2, double y2 )
{
	assert( self->ctype == daox_type_graphics_line );
	self->P1.x = x1;
	self->P1.y = y1;
	self->P2.x = x2;
	self->P2.y = y2;
}

void DaoxGraphicsRect_Set( DaoxGraphicsRect *self, double x1, double y1, double x2, double y2 )
{
	assert( self->ctype == daox_type_graphics_rect );
	self->P1.x = x1;
	self->P1.y = y1;
	self->P2.x = x2;
	self->P2.y = y2;
}

void DaoxGraphicsCircle_Set( DaoxGraphicsCircle *self, double x, double y, double r )
{
	assert( self->ctype == daox_type_graphics_circle );
	self->P1.x = x;
	self->P1.y = y;
	self->P2.x = r;
	self->P2.y = r;
}

void DaoxGraphicsEllipse_Set( DaoxGraphicsEllipse *self, double x, double y, double rx, double ry )
{
	assert( self->ctype == daox_type_graphics_ellipse );
	self->P1.x = x;
	self->P1.y = y;
	self->P2.x = rx;
	self->P2.y = ry;
}



void DaoxGraphicsPolyLine_Add( DaoxGraphicsPolyLine *self, double x1, double y1, double x2, double y2 )
{
	assert( self->ctype == daox_type_graphics_polyline );
	DaoxPointArray_PushXY( self->points, x1, y1 );
	DaoxPointArray_PushXY( self->points, x2, y2 );
}

void DaoxGraphicsPolygon_Add( DaoxGraphicsPolygon *self, double x, double y )
{
	assert( self->ctype == daox_type_graphics_polygon );
	DaoxPointArray_PushXY( self->points, x, y );
}

void DaoxGraphicsPath_SetRelativeMode( DaoxGraphicsPath *self, int relative )
{
	DaoxPath_SetRelativeMode( self->path, relative );
}
void DaoxGraphicsPath_MoveTo( DaoxGraphicsPath *self, double x, double y )
{
	assert( self->ctype == daox_type_graphics_path );
	DaoxPath_MoveTo( self->path, x, y );
}
void DaoxGraphicsPath_LineTo( DaoxGraphicsPath *self, double x, double y )
{
	assert( self->ctype == daox_type_graphics_path );
	DaoxPath_LineTo( self->path, x, y );
}
void DaoxGraphicsPath_ArcTo( DaoxGraphicsPath *self, double x, double y, double degrees )
{
	assert( self->ctype == daox_type_graphics_path );
	DaoxPath_ArcTo( self->path, x, y, degrees );
}
void DaoxGraphicsPath_ArcBy( DaoxGraphicsPath *self, double cx, double cy, double degrees )
{
	assert( self->ctype == daox_type_graphics_path );
	DaoxPath_ArcBy( self->path, cx, cy, degrees );
}
void DaoxGraphicsPath_QuadTo( DaoxGraphicsPath *self, double cx, double cy, double x, double y )
{
	assert( self->ctype == daox_type_graphics_path );
	DaoxPath_QuadTo( self->path, cx, cy, x, y );
}
void DaoxGraphicsPath_CubicTo( DaoxGraphicsPath *self, double cx, double cy, double x, double y )
{
	assert( self->ctype == daox_type_graphics_path );
	DaoxPath_CubicTo( self->path, cx, cy, x, y );
}
void DaoxGraphicsPath_CubicTo2( DaoxGraphicsLine *self, double cx0, double cy0, double cx, double cy, double x, double y )
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

/* TODO: dash pattern. */
void DaoxGraphicsLine_UpdateData( DaoxGraphicsLine *self, DaoxGraphicsScene *scene )
{
	DaoxQuad quad = DaoxLine2Quad( self->P1, self->P2, self->state->strokeWidth );

	DaoxGraphicsiItem_ResetData( self );
	DaoxGraphicsData_PushStrokeQuad( self->gdata, quad );
}
void DaoxGraphicsRect_UpdateData( DaoxGraphicsRect *self, DaoxGraphicsScene *scene )
{
	DaoxQuad centerQuad, bottomQuad, topQuad, leftQuad, rightQuad;
	double left = self->P1.x;
	double bottom = self->P1.y;
	double right = self->P2.x;
	double top = self->P2.y;
	double tmp, W2 = 0.5 * self->state->strokeWidth;

	if( left > right ) tmp = left, left = right, right = tmp;
	if( bottom > top ) tmp = bottom, bottom = top, top = tmp;

	centerQuad = DaoxQuad_FromRect( left + W2, bottom + W2, right - W2, top - W2 );
	bottomQuad = DaoxQuad_FromRect( left - W2, bottom - W2, right + W2, bottom + W2 );
	topQuad    = DaoxQuad_FromRect( left - W2, top - W2, right + W2, top + W2 );
	leftQuad   = DaoxQuad_FromRect( left - W2, bottom + W2, left + W2, top - W2 );
	rightQuad  = DaoxQuad_FromRect( right - W2, bottom + W2, right + W2, top - W2 );

	DaoxGraphicsiItem_ResetData( self );

	DaoxGraphicsData_PushStrokeQuad( self->gdata, bottomQuad );
	DaoxGraphicsData_PushStrokeQuad( self->gdata, topQuad );
	DaoxGraphicsData_PushStrokeQuad( self->gdata, leftQuad );
	DaoxGraphicsData_PushStrokeQuad( self->gdata, rightQuad );

	if( self->state->fillColor.alpha < 1E-9 ) return;
	DaoxGraphicsData_PushFillQuad( self->gdata, centerQuad );
}
void DaoxGraphicsEllipse_UpdateData( DaoxGraphicsEllipse *self, DaoxGraphicsScene *scene )
{
	DaoxTransform transform = {0.0,0.0,0.0,0.0,0.0,0.0};
	double CX = self->P1.x;
	double CY = self->P1.y;
	double RX = self->P2.x;
	double RY = self->P2.y;
	double W = self->state->strokeWidth;

	DaoxGraphicsData_Init( self->gdata, self );
	self->gdata->junction = DAOX_JUNCTION_FLAT;
	self->gdata->maxlen = 3.5 * log(RX + RY + W + 1.0) / log(2.0);
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
void DaoxGraphicsPolyLine_UpdateData( DaoxGraphicsPolyLine *self, DaoxGraphicsScene *scene )
{
	int i;
	DaoxGraphicsiItem_ResetData( self );

	for(i=0; i<self->points->count; i+=2){
		DaoxPoint start = self->points->points[i];
		DaoxPoint end = self->points->points[i+1];
		DaoxQuad quad = DaoxLine2Quad( start, end, self->state->strokeWidth );
		DaoxGraphicsData_PushStrokeQuad( self->gdata, quad );
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
		DaoxPath_Preprocess( self->path, scene->buffer );
	}
	DaoxGraphicsData_Init( self->gdata, self );
	DaoxPath_ExportGraphicsData( self->path, self->gdata );
	printf( "DaoxGraphicsPolygon_UpdateData: %i\n", self->gdata->strokeTriangles->count );
}
void DaoxGraphicsPath_UpdateData( DaoxGraphicsPath *self, DaoxGraphicsScene *scene )
{
	DaoxGraphicsiItem_ResetData( self );
#if 0
	DaoxPathGraph_Reset( scene->graph );
	DaoxPathGraph_Import( scene->graph, self->path );
	DaoxPathGraph_IntersectEdges( scene->graph );
	DaoxPathGraph_Export( scene->graph, self->path );
#endif
	if( self->path->first->refined.first == NULL )
		DaoxPath_Preprocess( self->path, scene->buffer );
	DaoxGraphicsData_Init( self->gdata, self );
	DaoxPath_ExportGraphicsData( self->path, self->gdata );
}
void DaoxGraphicsText_UpdateData( DaoxGraphicsText *self, DaoxGraphicsScene *scene )
{
	int i, j, jt = DAOX_JUNCTION_FLAT;
	double scale, offset, maxlen, maxdiff;
	double gscale = DaoxGraphicsScene_Scale( scene );
	double width = self->state->strokeWidth;
	double size = self->state->fontSize;
	DaoxFont *font = self->state->font;
	DaoxGlyph *glyph;

	DaoxGraphicsiItem_ResetData( self );

	if( self->codepoint == 0 ) return;
	if( font == NULL ) return;
	
	scale = size / (double)font->fontHeight;
	maxlen = 8.0 * font->fontHeight / size; 
	maxdiff = 2.0 / size;

	DaoxGraphicsData_Init( self->gdata, self );
	self->gdata->junction = DAOX_JUNCTION_FLAT;
	self->gdata->maxlen = maxlen;
	self->gdata->maxdiff = maxdiff;

	printf( ">>>>>>>>>> self->codepoint: %lc %p\n", self->codepoint, self->gdata->transform );

	glyph = DaoxFont_GetCharGlyph( font, self->codepoint );
	DaoxPath_ExportGraphicsData( glyph->shape, self->gdata );
}

int DaoxGraphicsItem_UpdateData( DaoxGraphicsItem *self, DaoxGraphicsScene *scene )
{
	double scale = DaoxGraphicsScene_Scale( scene );
	daoint i;

	if( self->visible == 0 ){
		DaoxGraphicsiItem_ResetData( self );
		self->gdata->scale = scale;
		return 1;
	}
	if( scale > 2*self->gdata->scale || 2*scale < self->gdata->scale ){
		printf( "here %15f %15f\n", scale, self->gdata->scale );
		DaoxGraphicsiItem_ResetData( self );
	}
	if( self->gdata->strokePoints->count || self->gdata->fillPoints->count ) return 0;
	switch( self->shape ){
	case DAOX_GS_LINE     : DaoxGraphicsLine_UpdateData( self, scene );     break;
	case DAOX_GS_RECT     : DaoxGraphicsRect_UpdateData( self, scene );     break;
	case DAOX_GS_CIRCLE   : DaoxGraphicsCircle_UpdateData( self, scene );   break;
	case DAOX_GS_ELLIPSE  : DaoxGraphicsEllipse_UpdateData( self, scene );  break;
	case DAOX_GS_POLYLINE : DaoxGraphicsPolyLine_UpdateData( self, scene ); break;
	case DAOX_GS_POLYGON  : DaoxGraphicsPolygon_UpdateData( self, scene );  break;
	case DAOX_GS_PATH     : DaoxGraphicsPath_UpdateData( self, scene );     break;
	case DAOX_GS_TEXT     : DaoxGraphicsText_UpdateData( self, scene );     break;
	}
	DaoxGraphicsData_MakeFillGradient( self->gdata );
	self->gdata->scale = scale;
	if( self->gdata->strokePoints->count ){
		DaoxBoundingBox_Init( & self->bounds, self->gdata->strokePoints->points[0] );
	}else if( self->gdata->fillPoints->count ){
		DaoxBoundingBox_Init( & self->bounds, self->gdata->fillPoints->points[0] );
	}
	for(i=0; i<self->gdata->strokePoints->count; ++i){
		DaoxBoundingBox_Update( & self->bounds, self->gdata->strokePoints->points[i] );
	}
	for(i=0; i<self->gdata->fillPoints->count; ++i){
		DaoxBoundingBox_Update( & self->bounds, self->gdata->fillPoints->points[i] );
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
	self->buffer = DaoxPathBuffer_New();
	self->graph = DaoxPathGraph_New();
	self->smallCircle = DaoxPath_New();
	self->largeCircle = DaoxPath_New();
	self->wideEllipse = DaoxPath_New();
	self->narrowEllipse = DaoxPath_New();

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

	DaoxPath_Preprocess( self->smallCircle, self->buffer );
	DaoxPath_Preprocess( self->largeCircle, self->buffer );
	DaoxPath_Preprocess( self->wideEllipse, self->buffer );
	DaoxPath_Preprocess( self->narrowEllipse, self->buffer );

	for(i=0; i<DAOX_ARCS; i++){
		double angle2 = (i+1.0) * 180 / (double)DAOX_ARCS;
		double angle = (i+1.0) * M_PI / (double)DAOX_ARCS;
		double cosine = cos( 0.5 * angle );
		double sine = sin( 0.5 * angle );
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
		DaoxPath_Preprocess( self->smallArcs[i], self->buffer );
		DaoxPath_Preprocess( self->largeArcs[i], self->buffer );
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
	DaoxPathBuffer_Delete( self->buffer );
	DaoxPathGraph_Delete( self->graph );
	DaoxPath_Delete( self->smallCircle );
	DaoxPath_Delete( self->largeCircle );
	DaoxPath_Delete( self->wideEllipse );
	DaoxPath_Delete( self->narrowEllipse );
	dao_free( self );
}


void DaoxGraphicsScene_SetViewport( DaoxGraphicsScene *self, double left, double right, double bottom, double top )
{
	self->viewport.left = left;
	self->viewport.right = right;
	self->viewport.bottom = bottom;
	self->viewport.top = top;
}
double DaoxGraphicsScene_Scale( DaoxGraphicsScene *self )
{
	DaoxBoundingBox box = self->viewport;
	double xscale = fabs( box.right - box.left ) / (self->defaultWidth + 1);
	double yscale = fabs( box.top - box.bottom ) / (self->defaultHeight + 1);
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
void DaoxGraphicsScene_PushState( DaoxGraphicsScene *self )
{
	DaoxGraphicsState *prev = DaoxGraphicsScene_GetCurrentState( self );
	DaoxGraphicsState *state = DaoxGraphicsState_New();
	if( prev ) DaoxGraphicsState_Copy( state, prev );
	DArray_PushBack( self->states, state );
}
void DaoxGraphicsScene_PopState( DaoxGraphicsScene *self )
{
	DArray_PopBack( self->states );
}

void DaoxGraphicsScene_SetStrokeWidth( DaoxGraphicsScene *self, double width )
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
void DaoxGraphicsScene_SetDashPattern( DaoxGraphicsScene *self, double pat[], int n )
{
	DaoxGraphicsState *state = DaoxGraphicsScene_GetOrPushState( self );
	if( n > 10 ) n = 10;
	state->dash = n;
	memcpy( state->dashPattern, pat, n*sizeof(double) );
}
void DaoxGraphicsScene_SetFont( DaoxGraphicsScene *self, DaoxFont *font, double size )
{
	DaoxGraphicsState *state = DaoxGraphicsScene_GetOrPushState( self );
	DaoGC_ShiftRC( (DaoValue*) font, (DaoValue*) state->font );
	state->font = font;
	state->fontSize = size;
}


DaoxGraphicsLine* DaoxGraphicsScene_AddLine( DaoxGraphicsScene *self, double x1, double y1, double x2, double y2 )
{
	DaoxGraphicsLine *item = DaoxGraphicsItem_New( self, DAOX_GS_LINE );
	DaoxGraphicsLine_Set( item, x1, y1, x2, y2 );
	DArray_PushBack( self->items, item );
	return item;
}

DaoxGraphicsRect* DaoxGraphicsScene_AddRect( DaoxGraphicsScene *self, double x1, double y1, double x2, double y2 )
{
	DaoxGraphicsRect *item = DaoxGraphicsItem_New( self, DAOX_GS_RECT );
	DaoxGraphicsRect_Set( item, x1, y1, x2, y2 );
	DArray_PushBack( self->items, item );
	return item;
}

DaoxGraphicsCircle* DaoxGraphicsScene_AddCircle( DaoxGraphicsScene *self, double x, double y, double r )
{
	DaoxGraphicsCircle *item = DaoxGraphicsItem_New( self, DAOX_GS_CIRCLE );
	DaoxGraphicsCircle_Set( item, x, y, r );
	DArray_PushBack( self->items, item );
	return item;
}

DaoxGraphicsEllipse* DaoxGraphicsScene_AddEllipse( DaoxGraphicsScene *self, double x, double y, double rx, double ry )
{
	DaoxGraphicsEllipse *item = DaoxGraphicsItem_New( self, DAOX_GS_ELLIPSE );
	DaoxGraphicsEllipse_Set( item, x, y, rx, ry );
	DArray_PushBack( self->items, item );
	return item;
}

DaoxGraphicsPolyLine* DaoxGraphicsScene_AddPolyLine( DaoxGraphicsScene *self )
{
	DaoxGraphicsPolyLine *item = DaoxGraphicsItem_New( self, DAOX_GS_POLYLINE );
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
void DaoxGraphicsText_AddCharItems( DaoxGraphicsText *self, const wchar_t *text )
{
	DaoxGlyph *glyph;
	DaoxGraphicsText *chitem;
	DaoxFont *font = self->state->font;
	DaoxTransform transform = {0.0,0.0,0.0,0.0,0.0,0.0};
	double width = self->state->strokeWidth;
	double size = self->state->fontSize;
	double scale = size / (double)font->fontHeight;
	double offset;

	transform.Axx = transform.Ayy = scale;
	transform.Bx = self->P1.x;
	transform.By = self->P1.y;

	if( self->path ) DaoxPath_Refine( self->path, 8.0/size, 2.0/size );

	offset = self->P1.x;
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
			double p = 0.0, adv = 0.5 * (scale * glyph->advanceWidth + width);
			DaoxPathSegment *seg1 = DaoxPath_LocateByDistance( self->path, offset+adv, &p );
			DaoxPathSegment *seg2 = DaoxPath_LocateByDistance( self->path, offset, &p );
			if( seg1 == NULL ) seg1 = seg2;
			if( seg2 ){
				double dx = seg1->P2.x - seg1->P1.x;
				double dy = seg1->P2.y - seg1->P1.y;
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
DaoxGraphicsText* DaoxGraphicsScene_AddText( DaoxGraphicsScene *self, const wchar_t *text, double x, double y )
{
	DaoxGraphicsPath *item;
	DaoxGraphicsState *state;
	if( self->states->size == 0 ) return NULL;
	state = DaoxGraphicsScene_GetOrPushState( self );
	if( state->font == NULL ) return NULL;

	item = DaoxGraphicsItem_New( self, DAOX_GS_TEXT );
	DArray_PushBack( self->items, item );
	item->P1.x = x;
	item->P1.y = y;
	DaoxGraphicsText_AddCharItems( item, text );
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
	DaoxPath_Preprocess( item->path, self->buffer );
	DaoxGraphicsText_AddCharItems( item, text );
	return item;
}








static void ITEM_SetVisible( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsItem *self = (DaoxGraphicsItem*) p[0];
	self->visible = p[1]->xEnum.value;
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
	{ ITEM_SetVisible,  "SetVisible( self : GraphicsItem, visible :enum<false,true> )" },
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
	double x1 = p[0]->xDouble.value;
	double y1 = p[1]->xDouble.value;
	double x2 = p[2]->xDouble.value;
	double y2 = p[3]->xDouble.value;
	DaoxGraphicsLine_Set( self, x1, y1, x2, y2 );
}
static void LINE_New( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsLine *self = DaoxGraphicsItem_New( NULL, DAOX_GS_LINE );
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
	{ LINE_New,     "GraphicsLine( x1 = 0.0D, y1 = 0.0D, x2 = 1.0D, y2 = 1.0D )" },
	{ LINE_Set,     "Set( self : GraphicsLine, x1 = 0.0D, y1 = 0.0D, x2 = 1.0D, y2 = 1.0D ) => GraphicsLine" },
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
	double x1 = p[0]->xDouble.value;
	double y1 = p[1]->xDouble.value;
	double x2 = p[2]->xDouble.value;
	double y2 = p[3]->xDouble.value;
	DaoxGraphicsRect_Set( self, x1, y1, x2, y2 );
}
static void RECT_New( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsRect *self = DaoxGraphicsItem_New( NULL, DAOX_GS_RECT );
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
	{ RECT_New,     "GraphicsRect( x1 = 0.0D, y1 = 0.0D, x2 = 1.0D, y2 = 1.0D )" },
	{ RECT_Set,     "Set( self : GraphicsRect, x1 = 0.0D, y1 = 0.0D, x2 = 1.0D, y2 = 1.0D ) => GraphicsRect" },
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
	double x = p[0]->xDouble.value;
	double y = p[1]->xDouble.value;
	double r = p[2]->xDouble.value;
	DaoxGraphicsCircle_Set( self, x, y, r );
}
static void CIRCLE_New( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsCircle *self = DaoxGraphicsItem_New( NULL, DAOX_GS_CIRCLE );
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
	{ CIRCLE_New,     "GraphicsCircle( cx = 0.0D, cy = 0.0D, r = 1.0D )" },
	{ CIRCLE_Set,     "Set( self : GraphicsCircle, cx = 0.0D, cy = 0.0D, r = 1.0D ) => GraphicsCircle" },
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
	double x1 = p[0]->xDouble.value;
	double y1 = p[1]->xDouble.value;
	double x2 = p[2]->xDouble.value;
	double y2 = p[3]->xDouble.value;
	DaoxGraphicsEllipse_Set( self, x1, y1, x2, y2 );
}
static void ELLIPSE_New( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsEllipse *self = DaoxGraphicsItem_New( NULL, DAOX_GS_ELLIPSE );
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
	{ ELLIPSE_New,  "GraphicsEllipse( cx = 0.0D, cy = 0.0D, rx = 1.0D, ry = 1.0D )" },
	{ ELLIPSE_Set,  "Set( self : GraphicsEllipse, cx = 0.0D, cy = 0.0D, rx = 1.0D, ry = 1.0D ) => GraphicsEllipse" },
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
	DaoxGraphicsPolyLine *self = DaoxGraphicsItem_New( NULL, DAOX_GS_POLYLINE );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void POLYLINE_Add( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsPolyLine *self = (DaoxGraphicsPolyLine*) p[0];
	double x1 = p[1]->xDouble.value;
	double y1 = p[2]->xDouble.value;
	double x2 = p[3]->xDouble.value;
	double y2 = p[4]->xDouble.value;
	DaoxGraphicsPolyLine_Add( self, x1, y1, x2, y2 );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static DaoFuncItem DaoxGraphicsPolyLineMeths[]=
{
	{ POLYLINE_New,   "GraphicsPolyLine()" },
	{ POLYLINE_Add,   "Add( self : GraphicsPolyLine, x1 = 0.0D, y1 = 0.0D, x2 = 1.0D, y2 = 1.0D ) => GraphicsPolyLine" },
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
	DaoxGraphicsPolygon *self = DaoxGraphicsItem_New( NULL, DAOX_GS_POLYGON );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void POLYGON_Add( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsPolygon *self = (DaoxGraphicsPolygon*) p[0];
	double x = p[1]->xDouble.value;
	double y = p[2]->xDouble.value;
	DaoxGraphicsPolygon_Add( self, x, y );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static DaoFuncItem DaoxGraphicsPolygonMeths[]=
{
	{ POLYGON_New,   "GraphicsPolygon()" },
	{ POLYGON_Add,   "Add( self : GraphicsPolygon, x : double, y : double ) => GraphicsPolygon" },
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
	DaoxGraphicsPath *self = DaoxGraphicsItem_New( NULL, DAOX_GS_PATH );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void PATH_MoveTo( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsPath *self = (DaoxGraphicsPath*) p[0];
	double x = p[1]->xDouble.value;
	double y = p[2]->xDouble.value;
	DaoxGraphicsPath_MoveTo( self, x, y );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void PATH_LineTo( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsPath *self = (DaoxGraphicsPath*) p[0];
	double x = p[1]->xDouble.value;
	double y = p[2]->xDouble.value;
	DaoxGraphicsPath_LineTo( self, x, y );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void PATH_ArcTo( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsPath *self = (DaoxGraphicsPath*) p[0];
	double x = p[1]->xDouble.value;
	double y = p[2]->xDouble.value;
	double d = p[3]->xDouble.value;
	DaoxGraphicsPath_ArcTo( self, x, y, d );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void PATH_ArcBy( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsPath *self = (DaoxGraphicsPath*) p[0];
	double x = p[1]->xDouble.value;
	double y = p[2]->xDouble.value;
	double d = p[3]->xDouble.value;
	DaoxGraphicsPath_ArcBy( self, x, y, d );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void PATH_QuadTo( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsPath *self = (DaoxGraphicsPath*) p[0];
	double cx = p[1]->xDouble.value;
	double cy = p[2]->xDouble.value;
	double x = p[3]->xDouble.value;
	double y = p[4]->xDouble.value;
	DaoxGraphicsPath_QuadTo( self, cx, cy, x, y );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void PATH_CubicTo( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsPath *self = (DaoxGraphicsPath*) p[0];
	double cx = p[1]->xDouble.value;
	double cy = p[2]->xDouble.value;
	double x = p[3]->xDouble.value;
	double y = p[4]->xDouble.value;
	DaoxGraphicsPath_CubicTo( self, cx, cy, x, y );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void PATH_CubicTo2( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsPath *self = (DaoxGraphicsPath*) p[0];
	double cx0 = p[1]->xDouble.value;
	double cy0 = p[2]->xDouble.value;
	double cx = p[3]->xDouble.value;
	double cy = p[4]->xDouble.value;
	double x = p[5]->xDouble.value;
	double y = p[6]->xDouble.value;
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
	{ PATH_New,       "GraphicsPath()" },
	{ PATH_MoveTo,    "MoveTo( self : GraphicsPath, x : double, y : double ) => GraphicsPath" },
	{ PATH_LineRelTo,    "LineTo( self : GraphicsPath, x : double, y : double ) => GraphicsPath" },
	{ PATH_ArcRelTo,     "ArcTo( self : GraphicsPath, x : double, y : double, degrees : double ) => GraphicsPath" },
	{ PATH_ArcRelBy,     "ArcBy( self : GraphicsPath, cx : double, cy : double, degrees : double ) => GraphicsPath" },
	{ PATH_QuadRelTo,   "QuadTo( self : GraphicsPath, cx : double, cy : double, x : double, y : double ) => GraphicsPath" },
	{ PATH_CubicRelTo,   "CubicTo( self : GraphicsPath, cx : double, cy : double, x : double, y : double ) => GraphicsPath" },
	{ PATH_CubicRelTo2,  "CubicTo( self : GraphicsPath, cx0 : double, cy0 : double, cx : double, cy : double, x : double, y : double ) => GraphicsPath" },
	{ PATH_LineAbsTo,    "LineAbsTo( self : GraphicsPath, x : double, y : double ) => GraphicsPath" },
	{ PATH_ArcAbsTo,     "ArcAbsTo( self : GraphicsPath, x : double, y : double, degrees : double ) => GraphicsPath" },
	{ PATH_ArcAbsBy,     "ArcAbsBy( self : GraphicsPath, cx : double, cy : double, degrees : double ) => GraphicsPath" },
	{ PATH_QuadAbsTo,   "QuadAbsTo( self : GraphicsPath, cx : double, cy : double, x : double, y : double ) => GraphicsPath" },
	{ PATH_CubicAbsTo,   "CubicAbsTo( self : GraphicsPath, cx : double, cy : double, x : double, y : double ) => GraphicsPath" },
	{ PATH_CubicAbsTo2,  "CubicAbsTo( self : GraphicsPath, cx0 : double, cy0 : double, cx : double, cy : double, x : double, y : double ) => GraphicsPath" },
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
	self->viewport.left = p[1]->xDouble.value;
	self->viewport.right = p[2]->xDouble.value;
	self->viewport.bottom = p[3]->xDouble.value;
	self->viewport.top = p[4]->xDouble.value;
}
static void SCENE_GetViewport( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsScene *self = (DaoxGraphicsScene*) p[0];
	DaoProcess_NewDouble( proc, self->viewport.left );
	DaoProcess_NewDouble( proc, self->viewport.right );
	DaoProcess_NewDouble( proc, self->viewport.bottom );
	DaoProcess_NewDouble( proc, self->viewport.top );
	DaoProcess_PutTuple( proc, -4 );
}
static void SCENE_SetBackground( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxColor color;
	DaoxGraphicsScene *self = (DaoxGraphicsScene*) p[0];
	color.red   = p[1]->xTuple.items[0]->xFloat.value;
	color.green = p[1]->xTuple.items[1]->xFloat.value;
	color.blue  = p[1]->xTuple.items[2]->xFloat.value;
	color.alpha = p[2]->xFloat.value;
	DaoxGraphicsScene_SetBackground( self, color );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void SCENE_AddLine( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsScene *self = (DaoxGraphicsScene*) p[0];
	double x1 = p[1]->xDouble.value, y1 = p[2]->xDouble.value;
	double x2 = p[3]->xDouble.value, y2 = p[4]->xDouble.value;
	DaoxGraphicsLine *item = DaoxGraphicsScene_AddLine( self, x1, y1, x2, y2 );
	DaoProcess_PutValue( proc, (DaoValue*) item );
}
static void SCENE_AddRect( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsScene *self = (DaoxGraphicsScene*) p[0];
	double x1 = p[1]->xDouble.value, y1 = p[2]->xDouble.value;
	double x2 = p[3]->xDouble.value, y2 = p[4]->xDouble.value;
	DaoxGraphicsRect *item = DaoxGraphicsScene_AddRect( self, x1, y1, x2, y2 );
	DaoProcess_PutValue( proc, (DaoValue*) item );
}
static void SCENE_AddCircle( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsScene *self = (DaoxGraphicsScene*) p[0];
	double x = p[1]->xDouble.value, y = p[2]->xDouble.value;
	double r = p[3]->xDouble.value;
	DaoxGraphicsCircle *item = DaoxGraphicsScene_AddCircle( self, x, y, r );
	DaoProcess_PutValue( proc, (DaoValue*) item );
}
static void SCENE_AddEllipse( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsScene *self = (DaoxGraphicsScene*) p[0];
	double x = p[1]->xDouble.value, y = p[2]->xDouble.value;
	double rx = p[3]->xDouble.value, ry = p[4]->xDouble.value;
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
static void SCENE_AddText( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsScene *self = (DaoxGraphicsScene*) p[0];
	DString *text = DaoValue_TryGetString( p[1] );
	double x = p[2]->xDouble.value;
	double y = p[3]->xDouble.value;
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
	DaoxGraphicsScene_PushState( self );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void SCENE_PopState( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsScene *self = (DaoxGraphicsScene*) p[0];
	DaoxGraphicsScene_PopState( self );
}
static void SCENE_SetStrokeWidth( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsScene *self = (DaoxGraphicsScene*) p[0];
	DaoxGraphicsScene_SetStrokeWidth( self, p[1]->xDouble.value );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void SCENE_SetStrokeColor( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxColor color;
	DaoxGraphicsScene *self = (DaoxGraphicsScene*) p[0];
	color.red   = p[1]->xTuple.items[0]->xFloat.value;
	color.green = p[1]->xTuple.items[1]->xFloat.value;
	color.blue  = p[1]->xTuple.items[2]->xFloat.value;
	color.alpha = p[2]->xFloat.value;
	DaoxGraphicsScene_SetStrokeColor( self, color );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void SCENE_SetFillColor( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxColor color;
	DaoxGraphicsScene *self = (DaoxGraphicsScene*) p[0];
	color.red   = p[1]->xTuple.items[0]->xFloat.value;
	color.green = p[1]->xTuple.items[1]->xFloat.value;
	color.blue  = p[1]->xTuple.items[2]->xFloat.value;
	color.alpha = p[2]->xFloat.value;
	DaoxGraphicsScene_SetFillColor( self, color );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void SCENE_SetJunction( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsScene *self = (DaoxGraphicsScene*) p[0];
	DaoxGraphicsState *state = DaoxGraphicsScene_GetOrPushState( self );
	state->junction = p[1]->xEnum.value;
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void SCENE_SetDash( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsScene *self = (DaoxGraphicsScene*) p[0];
	DaoArray *array = (DaoArray*) p[1];
	DaoxGraphicsScene_SetDashPattern( self, array->data.d, array->size );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void SCENE_SetTransform( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsScene *self = (DaoxGraphicsScene*) p[0];
	DaoxGraphicsState *state = DaoxGraphicsScene_GetOrPushState( self );
	DaoArray *array = (DaoArray*) p[1];
	daoint n = array->size;
	if( n != 4 && n != 6 ){
		DaoProcess_RaiseException( proc, DAO_ERROR_PARAM, "need matrix with 4 or 6 elements" );
		return;
	}
	state->transform.Axx = array->data.d[0];
	state->transform.Ayx = array->data.d[1];
	state->transform.Axy = array->data.d[2];
	state->transform.Ayy = array->data.d[3];
	if( n == 6 ){
		state->transform.Bx = array->data.d[4];
		state->transform.By = array->data.d[5];
	}
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void SCENE_SetStrokeGradient( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsScene *self = (DaoxGraphicsScene*) p[0];
	DaoxGraphicsState *state = DaoxGraphicsScene_GetOrPushState( self );
	if( state->strokeGradient == NULL ){
		state->strokeGradient = DaoxColorGradient_New( DAOX_GRADIENT_BASE );
		DaoGC_IncRC( (DaoValue*) state->strokeGradient );
	}
	DaoProcess_PutValue( proc, (DaoValue*) state->strokeGradient );
}
static void SCENE_SetFillGradient( DaoProcess *proc, DaoValue *p[], int N, int type )
{
	DaoxGraphicsScene *self = (DaoxGraphicsScene*) p[0];
	DaoxGraphicsState *state = DaoxGraphicsScene_GetOrPushState( self );
	if( state->fillGradient == NULL ){
		state->fillGradient = DaoxColorGradient_New( type );
		DaoGC_IncRC( (DaoValue*) state->fillGradient );
	}
	DaoProcess_PutValue( proc, (DaoValue*) state->fillGradient );
}
static void SCENE_SetLinearGradient( DaoProcess *proc, DaoValue *p[], int N )
{
	SCENE_SetFillGradient( proc, p, N, DAOX_GRADIENT_LINEAR );
}
static void SCENE_SetRadialGradient( DaoProcess *proc, DaoValue *p[], int N )
{
	SCENE_SetFillGradient( proc, p, N, DAOX_GRADIENT_RADIAL );
}
static void SCENE_SetPathGradient( DaoProcess *proc, DaoValue *p[], int N )
{
	SCENE_SetFillGradient( proc, p, N, DAOX_GRADIENT_PATH );
}
static void SCENE_SetFont( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsScene *self = (DaoxGraphicsScene*) p[0];
	DaoxFont *font = (DaoxFont*) p[1];
	DaoxGraphicsScene_SetFont( self, font, p[2]->xDouble.value );
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
	{ SCENE_SetViewport,   "SetViewport( self: GraphicsScene, left: double, right: double, bottom: double, top: double )" },
	{ SCENE_GetViewport,   "GetViewport( self: GraphicsScene ) => tuple<left:double,right:double,bottom:double,top:double>" },
	{ SCENE_SetBackground,  "SetBackground( self : GraphicsScene, color : tuple<red:float,green:float,blue:float>, alpha = 1.0 ) => GraphicsScene" },

	{ SCENE_PushState,   "PushState( self: GraphicsScene ) => GraphicsScene" },
	{ SCENE_PopState,    "PopState( self: GraphicsScene )" },
	{ SCENE_SetStrokeWidth,  "SetStrokeWidth( self : GraphicsScene, width = 1.0D ) => GraphicsScene" },
	{ SCENE_SetStrokeColor,  "SetStrokeColor( self : GraphicsScene, color : tuple<red:float,green:float,blue:float>, alpha = 1.0 ) => GraphicsScene" },
	{ SCENE_SetFillColor,  "SetFillColor( self : GraphicsScene, color : tuple<red:float,green:float,blue:float>, alpha = 1.0 ) => GraphicsScene" },
	{ SCENE_SetJunction, "SetJunction( self : GraphicsScene, junction: enum<none,sharp,flat,round> = $sharp ) => GraphicsScene" },
	{ SCENE_SetDash, "SetDashPattern( self : GraphicsScene, pattern = [3D,2D] ) => GraphicsScene" },
	{ SCENE_SetTransform, "SetTransform( self : GraphicsScene, transform : array<double> ) => GraphicsScene" },
	{ SCENE_SetStrokeGradient, "SetStrokeGradient( self : GraphicsScene ) => ColorGradient" },
	{ SCENE_SetLinearGradient, "SetLinearGradient( self : GraphicsScene ) => LinearGradient" },
	{ SCENE_SetRadialGradient, "SetRadialGradient( self : GraphicsScene ) => RadialGradient" },
	//{ SCENE_SetPathGradient,   "SetPathGradient( self : GraphicsScene ) => PathGradient" },

	{ SCENE_AddLine,   "AddLine( self: GraphicsScene, x1: double, y1: double, x2: double, y2: double ) => GraphicsLine" },
	{ SCENE_AddRect,   "AddRect( self: GraphicsScene, x1: double, y1: double, x2: double, y2: double ) => GraphicsRect" },
	{ SCENE_AddCircle,    "AddCircle( self: GraphicsScene, x: double, y: double, r: double ) => GraphicsCircle" },
	{ SCENE_AddEllipse,   "AddEllipse( self: GraphicsScene, x: double, y: double, rx: double, ry: double ) => GraphicsEllipse" },
	{ SCENE_AddPolyLine,  "AddPolyLine( self: GraphicsScene ) => GraphicsPolyLine" },
	{ SCENE_AddPolygon,   "AddPolygon( self: GraphicsScene ) => GraphicsPolygon" },
	{ SCENE_AddPath,      "AddPath( self: GraphicsScene ) => GraphicsPath" },
	{ SCENE_AddText,      "AddText( self: GraphicsScene, text : string, x :double, y :double ) => GraphicsText" },
	{ SCENE_AddText2,     "AddText( self: GraphicsScene, text : string, path :GraphicsPath ) => GraphicsText" },
	{ SCENE_SetFont,      "SetFont( self: GraphicsScene, font : Font, size = 12D )" },
	{ SCENE_Test,         "Test( self: GraphicsScene ) => GraphicsPath" },
	{ NULL, NULL }
};

DaoTypeBase DaoxGraphicsScene_Typer =
{
	"GraphicsScene", NULL, NULL, (DaoFuncItem*) DaoxGraphicsSceneMeths, {0}, {0},
	(FuncPtrDel)DaoxGraphicsScene_Delete, DaoxGraphicsScene_GetGCFields
};

DaoTypeBase DaoxGraphicsState_Typer =
{
	"GraphicsState", NULL, NULL, NULL, {0}, {0},
	(FuncPtrDel)DaoxGraphicsState_Delete, NULL 
};





DAO_DLL int DaoTriangulator_OnLoad( DaoVmSpace *vmSpace, DaoNamespace *ns );
DAO_DLL int DaoFont_OnLoad( DaoVmSpace *vmSpace, DaoNamespace *ns );
DAO_DLL int DaoGLUT_OnLoad( DaoVmSpace *vmSpace, DaoNamespace *ns );

DAO_DLL int DaoOnLoad( DaoVmSpace *vmSpace, DaoNamespace *ns )
{
	DaoFont_OnLoad( vmSpace, ns );

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
	daox_type_graphics_polyline = DaoNamespace_WrapType( ns, & DaoxGraphicsPolyLine_Typer, 0 );
	daox_type_graphics_polygon = DaoNamespace_WrapType( ns, & DaoxGraphicsPolygon_Typer, 0 );
	daox_type_graphics_path = DaoNamespace_WrapType( ns, & DaoxGraphicsPath_Typer, 0 );
	daox_type_graphics_text = DaoNamespace_WrapType( ns, & DaoxGraphicsText_Typer, 0 );

	DaoTriangulator_OnLoad( vmSpace, ns );
	DaoGLUT_OnLoad( vmSpace, ns );
	return 0;
}
