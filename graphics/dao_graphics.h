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
// TODO: hardware accelerated backend;
// TODO: rasterization backend? base it on the OpenVG RI?
// TODO: other library based backend; 
*/

#ifndef __DAO_GRAPHICS_H__
#define __DAO_GRAPHICS_H__

#include "daoStdtype.h"
#include "daoValue.h"

#include "dao_geometry.h"
#include "dao_path.h"
#include "dao_triangulator.h"


enum DaoxGraphicsShapes
{
	DAOX_GS_LINE ,
	DAOX_GS_RECT ,
	DAOX_GS_CIRCLE ,
	DAOX_GS_ELLIPSE ,
	DAOX_GS_POLYLINE ,
	DAOX_GS_POLYGON ,
	DAOX_GS_PATH ,
	DAOX_GS_TEXT
};

typedef struct DaoxColor             DaoxColor;

typedef struct DaoxGraphicsScene     DaoxGraphicsScene;
typedef struct DaoxGraphicsItem      DaoxGraphicsItem;


typedef  DaoxGraphicsItem  DaoxGraphicsLine;
typedef  DaoxGraphicsItem  DaoxGraphicsRect;
typedef  DaoxGraphicsItem  DaoxGraphicsCircle;
typedef  DaoxGraphicsItem  DaoxGraphicsEllipse;
typedef  DaoxGraphicsItem  DaoxGraphicsPolyLine;
typedef  DaoxGraphicsItem  DaoxGraphicsPolygon;
typedef  DaoxGraphicsItem  DaoxGraphicsPath;
typedef  DaoxGraphicsItem  DaoxGraphicsText;



struct DaoxColor
{
	double  red;
	double  green;
	double  blue;
	double  alpha;
};




struct DaoxGraphicsItem
{
	DAO_CDATA_COMMON;

	uchar_t  shape;     /* shape type; */
	uchar_t  stroke;    /* stroke type; */
	uchar_t  junction;  /* junction type; */

	DaoxPoint  position;     /* X, Y; */
	DaoxPoint  scale;        /* scales (width,height; dx,dy; rx,ry etc.); */

	DaoxTransform  transform;     /* [A, B; C, D] */

	double      strokeWidth;     /* stroke width; */
	DaoxColor  strokeColor;  /* stroke color: RGBA; */
	DaoxColor  fillColor;    /* filling color: RGBA; */

	DaoxPath  *path;  /* path, or points for polylines and polygons; */

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
	DaoxPolygonArray  *dashMasks;
	DaoxPolygonArray  *strokePolygons;
	DaoxPolygonArray  *fillPolygons;

	DString  *text;
	DString  *font;

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


struct DaoxGraphicsScene
{
	DAO_CDATA_COMMON;

	DArray  *items;

	DaoxPathBuffer  buffer;

	DaoxPathGraph  *graph;
};
DAO_DLL extern DaoType *daox_type_graphics_scene;




#ifdef __cplusplus
extern "C"{
#endif



DAO_DLL DaoxGraphicsItem* DaoxGraphicsItem_New( int shape );
DAO_DLL void DaoxGraphicsItem_Delete( DaoxGraphicsItem *self );


DAO_DLL void DaoxGraphicsItem_SetStrokeWidth( DaoxGraphicsItem *self, double w );

DAO_DLL void DaoxGraphicsItem_SetStrokeColor( DaoxGraphicsItem *self, double r, double g, double b, double a );

DAO_DLL void DaoxGraphicsItem_SetFillColor( DaoxGraphicsItem *self, double r, double g, double b, double a );


DAO_DLL void DaoxGraphicsLine_Set( DaoxGraphicsLine *self, double x1, double y1, double x2, double y2 );

DAO_DLL void DaoxGraphicsRect_Set( DaoxGraphicsRect *self, double x1, double y1, double x2, double y2 );

DAO_DLL void DaoxGraphicsCircle_Set( DaoxGraphicsCircle *self, double x, double y, double r );

DAO_DLL void DaoxGraphicsEllipse_Set( DaoxGraphicsEllipse *self, double x, double y, double rx, double ry );

DAO_DLL void DaoxGraphicsPolyLine_Add( DaoxGraphicsPolyLine *self, double x1, double y1, double x2, double y2 );

DAO_DLL void DaoxGraphicsPolygon_Add( DaoxGraphicsPolygon *self, double x, double y );

DAO_DLL void DaoxGraphicsPath_MoveTo( DaoxGraphicsPath *self, double x, double y );
DAO_DLL void DaoxGraphicsPath_LineTo( DaoxGraphicsPath *self, double x, double y );
DAO_DLL void DaoxGraphicsPath_ArcTo( DaoxGraphicsPath *self, double x, double y, double degrees, int clockwise );
DAO_DLL void DaoxGraphicsPath_QuadTo( DaoxGraphicsPath *self, double x, double y, double cx, double cy );
DAO_DLL void DaoxGraphicsPath_CubicTo( DaoxGraphicsPath *self, double x, double y, double cx, double cy );
DAO_DLL void DaoxGraphicsPath_CubicTo2( DaoxGraphicsPath *self, double cx0, double cy0, double x, double y, double cx, double cy );
DAO_DLL void DaoxGraphicsPath_Close( DaoxGraphicsPath *self );


DAO_DLL void DaoxGraphicsItem_UpdatePolygons( DaoxGraphicsItem *self, DaoxGraphicsScene *scene );






DAO_DLL DaoxGraphicsScene* DaoxGraphicsScene_New();
DAO_DLL void DaoxGraphicsScene_Delete( DaoxGraphicsScene *self );


DAO_DLL DaoxGraphicsLine* DaoxGraphicsScene_AddLine( DaoxGraphicsScene *self, double x1, double y1, double x2, double y2 );

DAO_DLL DaoxGraphicsRect* DaoxGraphicsScene_AddRect( DaoxGraphicsScene *self, double x1, double y1, double x2, double y2 );

DAO_DLL DaoxGraphicsCircle* DaoxGraphicsScene_AddCircle( DaoxGraphicsScene *self, double x, double y, double r );

DAO_DLL DaoxGraphicsEllipse* DaoxGraphicsScene_AddEllipse( DaoxGraphicsScene *self, double x, double y, double rx, double ry );

DAO_DLL DaoxGraphicsPolyLine* DaoxGraphicsScene_AddPolyLine( DaoxGraphicsScene *self );

DAO_DLL DaoxGraphicsPolygon* DaoxGraphicsScene_AddPolygon( DaoxGraphicsScene *self );

DAO_DLL DaoxGraphicsPath* DaoxGraphicsScene_AddPath( DaoxGraphicsScene *self );




#ifdef __cplusplus
}
#endif

#endif
