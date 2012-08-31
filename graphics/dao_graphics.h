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

/*
// Graphics Frontend:
//
// Currently only for 2D vector graphics.
//
// TODO: SVG Tiny backend;
//
// TODO:
// * multiple line, rect and ellipse in one item;
// * type for color and transformation matrix?
// * text on path, alignment direction (x-axis, y-axis, -x-axis, -y-axis, arbitrary angle);
// * locating item by point;
// * render to image (bmp?);
*/

#ifndef __DAO_GRAPHICS_H__
#define __DAO_GRAPHICS_H__

#include "daoStdtype.h"
#include "daoValue.h"

#include "dao_geometry.h"
#include "dao_triangulator.h"
#include "dao_font.h"
#include "dao_image.h"


#define DAOX_ARCS 18


enum DaoxGraphicsShapes
{
	DAOX_GS_LINE ,
	DAOX_GS_RECT ,
	DAOX_GS_CIRCLE ,
	DAOX_GS_ELLIPSE ,
	DAOX_GS_POLYLINE ,
	DAOX_GS_POLYGON ,
	DAOX_GS_PATH ,
	DAOX_GS_TEXT ,
	DAOX_GS_IMAGE
};

typedef struct DaoxColor             DaoxColor;
typedef struct DaoxColorArray        DaoxColorArray;

typedef struct DaoxGraphicsData      DaoxGraphicsData;

typedef struct DaoxGraphicsState     DaoxGraphicsState;
typedef struct DaoxColorGradient  DaoxColorGradient;

typedef struct DaoxGraphicsScene     DaoxGraphicsScene;
typedef struct DaoxGraphicsItem      DaoxGraphicsItem;


typedef  DaoxGraphicsItem  DaoxGraphicsLine;
typedef  DaoxGraphicsItem  DaoxGraphicsRect;
typedef  DaoxGraphicsItem  DaoxGraphicsCircle;
typedef  DaoxGraphicsItem  DaoxGraphicsEllipse;
typedef  DaoxGraphicsItem  DaoxGraphicsPolyline;
typedef  DaoxGraphicsItem  DaoxGraphicsPolygon;
typedef  DaoxGraphicsItem  DaoxGraphicsPath;
typedef  DaoxGraphicsItem  DaoxGraphicsText;
typedef  DaoxGraphicsItem  DaoxGraphicsImage;



struct DaoxColor
{
	float  red;
	float  green;
	float  blue;
	float  alpha;
};



struct DaoxColorArray
{
	DaoxColor  *colors;

	int  count;
	int  capacity;
};






/*
// Polygons converted from the stroking and filling areas of the item.
//
// These polygons are potentially overlapping, due to the fact that
// this frontend needs to be light and efficient, so that it can take
// the advantage of hardware acceleration.
//
// They should be filled using stencil buffer or other techniques to
// avoid multiple drawing in the overlapping areas.
//
// Note: two-point polygons are used to represent rectangles by pairs
// of points (left,bottom) and (right,top).
*/
struct DaoxGraphicsData
{
	uchar_t  linecap;
	uchar_t  junction;
	uchar_t  dashState;
	float    dashLength;
	float    currentOffset;
	float    currentLength;
	float    strokeWidth;
	float    maxlen;
	float    maxdiff;
	float    scale;
	int      texture;

	DaoxBounds  bounds;

	DaoxTransform  *transform;  /* for path only; */

	DaoxColorArray  *strokeColors;
	DaoxPointArray  *strokePoints;
	DaoxIntArray    *strokeTriangles;

	DaoxColorArray  *fillColors;
	DaoxPointArray  *fillPoints;
	DaoxIntArray    *fillTriangles;

	DaoxGraphicsItem  *item; 
};




enum DaoxGradientTypes
{
	DAOX_GRADIENT_BASE ,
	DAOX_GRADIENT_LINEAR ,
	DAOX_GRADIENT_RADIAL ,
	DAOX_GRADIENT_PATH
};

struct DaoxColorGradient
{
	DAO_CDATA_COMMON;

	int  gradient;

	DaoxFloatArray  *stops;
	DaoxColorArray  *colors;

	DaoxPoint  points[2];
	float     radius;
};
DAO_DLL extern DaoType *daox_type_color_gradient;
DAO_DLL extern DaoType *daox_type_linear_gradient;
DAO_DLL extern DaoType *daox_type_radial_gradient;
DAO_DLL extern DaoType *daox_type_path_gradient;




struct DaoxGraphicsState
{
	DAO_CDATA_COMMON;

	uchar_t dash;
	uchar_t linecap;
	uchar_t junction;
	float   fontSize;
	float   strokeWidth;
	float   dashPattern[10];

	DaoxTransform  transform;

	DaoxColor  strokeColor;  /* stroke color: RGBA; */
	DaoxColor  fillColor;    /* filling color: RGBA; */

	DaoxColorGradient  *strokeGradient;
	DaoxColorGradient  *fillGradient;

	DaoxFont  *font;
};
DAO_DLL extern DaoType *daox_type_graphics_state;



struct DaoxGraphicsItem
{
	DAO_CDATA_COMMON;

	uchar_t  shape;  /* shape type; */
	uchar_t  visible;
	uchar_t  roundCorner; /* for rect; */
	wchar_t  codepoint;

	DaoxBounds  bounds;

	DaoxGraphicsState  *state;

	DaoxPointArray  *points;
	DaoxPath        *path;

	DaoxImage  *image;

	DaoxGraphicsData  *gdata;
	DaoxGraphicsScene *scene;

	DArray  *children;  /* child items; */
};
DAO_DLL extern DaoType *daox_type_graphics_item;
DAO_DLL extern DaoType *daox_type_graphics_line;
DAO_DLL extern DaoType *daox_type_graphics_rect;
DAO_DLL extern DaoType *daox_type_graphics_circle;
DAO_DLL extern DaoType *daox_type_graphics_ellipse;
DAO_DLL extern DaoType *daox_type_graphics_polyline;
DAO_DLL extern DaoType *daox_type_graphics_polygon;
DAO_DLL extern DaoType *daox_type_graphics_path;
DAO_DLL extern DaoType *daox_type_graphics_text;
DAO_DLL extern DaoType *daox_type_graphics_image;


struct DaoxGraphicsScene
{
	DAO_CDATA_COMMON;

	float  defaultWidth;
	float  defaultHeight;

	DaoxBounds  viewport;

	DaoxTransform  transform;

	DaoxColor  background;

	DArray  *items;
	DArray  *states;

	DaoxPath  *quarterArc; 
	DaoxPath  *quarterCircle; 

	DaoxPath  *smallCircle; 
	DaoxPath  *largeCircle;

	DaoxPath  *wideEllipse; 
	DaoxPath  *narrowEllipse; 

	DaoxPath  *smallArcs[DAOX_ARCS];
	DaoxPath  *largeArcs[DAOX_ARCS];

	DaoxTriangulator  *triangulator;
};
DAO_DLL extern DaoType *daox_type_graphics_scene;




#ifdef __cplusplus
extern "C"{
#endif



DaoxColorArray* DaoxColorArray_New();
void DaoxColorArray_Clear( DaoxColorArray *self );
void DaoxColorArray_Delete( DaoxColorArray *self );
void DaoxColorArray_Reset( DaoxColorArray *self );
void DaoxColorArray_PushRGBA( DaoxColorArray *self, float r, float g, float b, float a );
void DaoxColorArray_Push( DaoxColorArray *self, DaoxColor color );


DaoxColorGradient* DaoxColorGradient_New( int type );
void DaoxColorGradient_Delete( DaoxColorGradient *self );
void DaoxColorGradient_Add( DaoxColorGradient *self, float stop, DaoxColor color );
DaoxColor DaoxColorGradient_InterpolateColor( DaoxColorGradient *self, float at );


DaoxGraphicsData* DaoxGraphicsData_New( DaoxGraphicsItem *item );
void DaoxGraphicsData_Delete( DaoxGraphicsData *self );
void DaoxGraphicsData_Reset( DaoxGraphicsData *self );
void DaoxGraphicsData_Init( DaoxGraphicsData *self, DaoxGraphicsItem *item );
void DaoxGraphicsData_UpdateDashState( DaoxGraphicsData *self, float len );
void DaoxGraphicsData_PushStrokeQuadColors( DaoxGraphicsData *self, float offset, float len );
int DaoxGraphicsData_PushStrokeTriangle( DaoxGraphicsData *self, DaoxPoint A, DaoxPoint B, DaoxPoint C );
int DaoxGraphicsData_PushStrokeQuad( DaoxGraphicsData *self, DaoxQuad quad );
void DaoxGraphicsData_PushFilling( DaoxGraphicsData *self, DaoxPoint A, DaoxPoint B, DaoxPoint C );
void DaoxGraphicsData_MakeLineCap( DaoxGraphicsData *self, DaoxPoint A, DaoxPoint B, int cap, int head );
void DaoxGraphicsData_MakeJunction( DaoxGraphicsData *self, DaoxPoint A, DaoxPoint B, DaoxPoint C, float pos, int junction );
void DaoxGraphicsData_MakeDashStroke( DaoxGraphicsData *self, DaoxPoint P1, DaoxPoint P2, float offset );



DaoxGraphicsState* DaoxGraphicsState_New();
void DaoxGraphicsState_Delete( DaoxGraphicsState *self );
void DaoxGraphicsState_Copy( DaoxGraphicsState *self, DaoxGraphicsState *other );



DAO_DLL DaoxGraphicsItem* DaoxGraphicsItem_New( DaoxGraphicsScene *scene, int shape );
DAO_DLL void DaoxGraphicsItem_Delete( DaoxGraphicsItem *self );


DAO_DLL void DaoxGraphicsLine_Set( DaoxGraphicsLine *self, float x1, float y1, float x2, float y2 );

DAO_DLL void DaoxGraphicsRect_Set( DaoxGraphicsRect *self, float x1, float y1, float x2, float y2, float rx, float ry );

DAO_DLL void DaoxGraphicsCircle_Set( DaoxGraphicsCircle *self, float x, float y, float r );

DAO_DLL void DaoxGraphicsEllipse_Set( DaoxGraphicsEllipse *self, float x, float y, float rx, float ry );

DAO_DLL void DaoxGraphicsPolyline_Add( DaoxGraphicsPolyline *self, float x, float y );

DAO_DLL void DaoxGraphicsPolygon_Add( DaoxGraphicsPolygon *self, float x, float y );

DAO_DLL void DaoxGraphicsPath_SetRelativeMode( DaoxGraphicsPath *self, int relative );
DAO_DLL void DaoxGraphicsPath_MoveTo( DaoxGraphicsPath *self, float x, float y );
DAO_DLL void DaoxGraphicsPath_LineTo( DaoxGraphicsPath *self, float x, float y );
DAO_DLL void DaoxGraphicsPath_ArcTo( DaoxGraphicsPath *self, float x, float y, float degrees );
DAO_DLL void DaoxGraphicsPath_ArcBy( DaoxGraphicsPath *self, float cx, float cy, float degrees );
DAO_DLL void DaoxGraphicsPath_QuadTo( DaoxGraphicsPath *self, float x, float y, float cx, float cy );
DAO_DLL void DaoxGraphicsPath_CubicTo( DaoxGraphicsPath *self, float x, float y, float cx, float cy );
DAO_DLL void DaoxGraphicsPath_CubicTo2( DaoxGraphicsPath *self, float cx0, float cy0, float x, float y, float cx, float cy );
DAO_DLL void DaoxGraphicsPath_Close( DaoxGraphicsPath *self );


DAO_DLL int DaoxGraphicsItem_UpdateData( DaoxGraphicsItem *self, DaoxGraphicsScene *scene );






DAO_DLL DaoxGraphicsScene* DaoxGraphicsScene_New();
DAO_DLL void DaoxGraphicsScene_Delete( DaoxGraphicsScene *self );

DAO_DLL void DaoxGraphicsScene_SetViewport( DaoxGraphicsScene *self, float left, float right, float bottom, float top );
DAO_DLL float DaoxGraphicsScene_Scale( DaoxGraphicsScene *self );
DAO_DLL void DaoxGraphicsScene_SetBackground( DaoxGraphicsScene *self, DaoxColor color );

DAO_DLL DaoxGraphicsState* DaoxGraphicsScene_PushState( DaoxGraphicsScene *self );
DAO_DLL void DaoxGraphicsScene_PopState( DaoxGraphicsScene *self );

DAO_DLL void DaoxGraphicsScene_SetStrokeWidth( DaoxGraphicsScene *self, float width );
DAO_DLL void DaoxGraphicsScene_SetStrokeColor( DaoxGraphicsScene *self, DaoxColor color );
DAO_DLL void DaoxGraphicsScene_SetFillColor( DaoxGraphicsScene *self, DaoxColor color );
DAO_DLL void DaoxGraphicsScene_SetDashPattern( DaoxGraphicsScene *self, float pat[], int n );
DAO_DLL void DaoxGraphicsScene_SetFont( DaoxGraphicsScene *self, DaoxFont *font, float size );



DAO_DLL DaoxGraphicsLine* DaoxGraphicsScene_AddLine( DaoxGraphicsScene *self, float x1, float y1, float x2, float y2 );

DAO_DLL DaoxGraphicsRect* DaoxGraphicsScene_AddRect( DaoxGraphicsScene *self, float x1, float y1, float x2, float y2, float rx, float ry );

DAO_DLL DaoxGraphicsCircle* DaoxGraphicsScene_AddCircle( DaoxGraphicsScene *self, float x, float y, float r );

DAO_DLL DaoxGraphicsEllipse* DaoxGraphicsScene_AddEllipse( DaoxGraphicsScene *self, float x, float y, float rx, float ry );

DAO_DLL DaoxGraphicsPolyline* DaoxGraphicsScene_AddPolyline( DaoxGraphicsScene *self );

DAO_DLL DaoxGraphicsPolygon* DaoxGraphicsScene_AddPolygon( DaoxGraphicsScene *self );

DAO_DLL DaoxGraphicsPath* DaoxGraphicsScene_AddPath( DaoxGraphicsScene *self );

DAO_DLL DaoxGraphicsText* DaoxGraphicsScene_AddText( DaoxGraphicsScene *self, const wchar_t *text, float x, float y );
DAO_DLL DaoxGraphicsImage* DaoxGraphicsScene_AddImage( DaoxGraphicsScene *self, DaoxImage *image, float x, float y );




#ifdef __cplusplus
}
#endif

#endif
