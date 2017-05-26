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

/*
// Vector Graphics Frontend:
//
//
// TODO:
// * type for color and transformation matrix?
// * locating item by point;
*/

#ifndef __DAO_CANVAS_H__
#define __DAO_CANVAS_H__

#include "dao_path.h"
#include "dao_font.h"
#include "daoStdtype.h"
#include "daoValue.h"

#define DAO_HAS_IMAGE
#include"dao_api.h"

#ifdef __cplusplus
extern "C"{
#endif


#define DAOX_MAX_GRADIENT_STOPS  64

typedef struct DaoxBrush      DaoxBrush;
typedef struct DaoxGradient   DaoxGradient;
typedef struct DaoxCanvasNode DaoxCanvasNode;
typedef struct DaoxCanvas     DaoxCanvas;


enum DaoxGradientTypes
{
	DAOX_GRADIENT_BASE ,
	DAOX_GRADIENT_LINEAR ,
	DAOX_GRADIENT_RADIAL 
};

struct DaoxGradient
{
	DAO_CSTRUCT_COMMON;

	int           gradient;
	float         radius;
	DaoxVector2D  points[2];

	DArray  *stops;  /* <float> */
	DArray  *colors; /* <DaoxColor> */

};
DAO_CANVAS_DLL DaoType *daox_type_gradient;
DAO_CANVAS_DLL DaoType *daox_type_linear_gradient;
DAO_CANVAS_DLL DaoType *daox_type_radial_gradient;
DAO_CANVAS_DLL DaoType *daox_type_path_gradient;

DAO_CANVAS_DLL DaoxGradient* DaoxGradient_New( DaoVmSpace *vmspace, int type );
DAO_CANVAS_DLL void DaoxGradient_Delete( DaoxGradient *self );
DAO_CANVAS_DLL void DaoxGradient_Add( DaoxGradient *self, float stop, DaoxColor color );
DAO_CANVAS_DLL void DaoxGradient_Copy( DaoxGradient *self, DaoxGradient *other );
DAO_CANVAS_DLL DaoxColor DaoxGradient_ComputeColor( DaoxGradient *self, DaoxVector2D point );




struct DaoxBrush
{
	DAO_CSTRUCT_COMMON;

	DaoxPathStyle  strokeStyle;
	DaoxColor      strokeColor;
	DaoxColor      fillColor;
	DaoxGradient  *strokeGradient;
	DaoxGradient  *fillGradient;
	DaoImage      *image;
	DaoxFont      *font;
	float          fontSize;
};
DAO_CANVAS_DLL DaoType *daox_type_brush;

DAO_CANVAS_DLL DaoxBrush* DaoxBrush_New( DaoVmSpace *vmspace );
DAO_CANVAS_DLL void DaoxBrush_Delete( DaoxBrush *self );
DAO_CANVAS_DLL void DaoxBrush_Copy( DaoxBrush *self, DaoxBrush *other );

DAO_CANVAS_DLL void DaoxBrush_SetStrokeWidth( DaoxBrush *self, float width );
DAO_CANVAS_DLL void DaoxBrush_SetStrokeColor( DaoxBrush *self, DaoxColor color );
DAO_CANVAS_DLL void DaoxBrush_SetFillColor( DaoxBrush *self, DaoxColor color );
DAO_CANVAS_DLL void DaoxBrush_SetDashPattern( DaoxBrush *self, float pat[], int n );
DAO_CANVAS_DLL void DaoxBrush_SetFont( DaoxBrush *self, DaoxFont *font, float size );





/*
// Base for all canvas item types:
*/
struct DaoxCanvasNode
{
	DAO_CSTRUCT_COMMON;

	uchar_t  visible;  /* visibility; */
	uchar_t  changed;  /* geometry changed; */
	uchar_t  moved;    /* orientation changed; */
	float    scale;    /* path scale; */

	DaoxOBBox2D   obbox;        /* local space; */
	DaoxVector2D  rotation;     /* local space (cos,sin); */
	DaoxVector2D  translation;  /* parent space; */

	DaoxBrush     *brush;   /* brush for the node; */
	DaoxPath      *path;    /* path for the canvas node; */
	DaoxPathMesh  *mesh;    /* tessellation for the path; */

	DaoxCanvasNode  *parent;    /* parent node; */
	DList           *children;  /* children nodes; */
};

DAO_CANVAS_DLL DaoxCanvasNode* DaoxCanvasNode_New( DaoType *type );
DAO_CANVAS_DLL void DaoxCanvasNode_Delete( DaoxCanvasNode *self );

DAO_CANVAS_DLL void DaoxCanvasNode_MarkDataChanged( DaoxCanvasNode *self );
DAO_CANVAS_DLL void DaoxCanvasNode_MarkStateChanged( DaoxCanvasNode *self );
DAO_CANVAS_DLL void DaoxCanvasNode_Update( DaoxCanvasNode *self, DaoxCanvas *canvas );
DAO_CANVAS_DLL DaoxMatrix3D DaoxCanvasNode_GetLocalTransform( DaoxCanvasNode *self );



/*
// A line is always defined locally by (0,0)-(1,0).
// Its actual size and orientation are determined by its transformations.
*/
typedef  DaoxCanvasNode  DaoxCanvasLine;

/*
// A rectable is always defined locally by (0,0)-(1,0)-(1,y)-(0,y).
// Its actual size and orientation are determined by its transformations.
*/
typedef  DaoxCanvasNode  DaoxCanvasRect;

/*
// A circle is always defined locally as unit circle located at (0,0).
// Its actual size and orientation are determined by its transformations.
*/
typedef  DaoxCanvasNode  DaoxCanvasCircle;

/*
// An ellipse is always defined locally as unit circle located at (0,0).
// Its actual size and orientation are determined by its transformations.
*/
typedef  DaoxCanvasNode  DaoxCanvasEllipse;


typedef DaoxCanvasNode  DaoxCanvasPath;
typedef DaoxCanvasNode  DaoxCanvasText;
typedef DaoxCanvasNode  DaoxCanvasImage;


DAO_CANVAS_DLL DaoType *daox_type_canvas_node;
DAO_CANVAS_DLL DaoType *daox_type_canvas_line;
DAO_CANVAS_DLL DaoType *daox_type_canvas_rect;
DAO_CANVAS_DLL DaoType *daox_type_canvas_circle;
DAO_CANVAS_DLL DaoType *daox_type_canvas_ellipse;
DAO_CANVAS_DLL DaoType *daox_type_canvas_path;
DAO_CANVAS_DLL DaoType *daox_type_canvas_text;
DAO_CANVAS_DLL DaoType *daox_type_canvas_image;





struct DaoxCanvas
{
	DAO_CSTRUCT_COMMON;

	DaoxAABBox2D  viewport;
	DaoxMatrix3D  transform;

	DaoxColor  background;

	DList  *nodes;
	DList  *actives;
	DList  *brushes;

	DaoxPath      *auxPath;
	DaoxPathCache *pathCache;
};
DAO_CANVAS_DLL DaoType *daox_type_canvas;


DAO_CANVAS_DLL DaoxCanvas* DaoxCanvas_New( DaoVmSpace *vmspace );
DAO_CANVAS_DLL void DaoxCanvas_Delete( DaoxCanvas *self );

DAO_CANVAS_DLL void DaoxCanvas_SetViewport( DaoxCanvas *self, float left, float right, float bottom, float top );
DAO_CANVAS_DLL float DaoxCanvas_Scale( DaoxCanvas *self );
DAO_CANVAS_DLL void DaoxCanvas_SetBackground( DaoxCanvas *self, DaoxColor color );

DAO_CANVAS_DLL DaoxBrush* DaoxCanvas_PushBrush( DaoxCanvas *self, int index );
DAO_CANVAS_DLL void DaoxCanvas_PopBrush( DaoxCanvas *self );

DAO_CANVAS_DLL DaoxCanvasNode* DaoxCanvas_AddGroup( DaoxCanvas *self );

DAO_CANVAS_DLL DaoxCanvasLine* DaoxCanvas_AddLine( DaoxCanvas *self, float x1, float y1, float x2, float y2 );

DAO_CANVAS_DLL DaoxCanvasRect* DaoxCanvas_AddRect( DaoxCanvas *self, float x1, float y1, float x2, float y2, float rx, float ry );

DAO_CANVAS_DLL DaoxCanvasCircle* DaoxCanvas_AddCircle( DaoxCanvas *self, float x, float y, float r );

DAO_CANVAS_DLL DaoxCanvasEllipse* DaoxCanvas_AddEllipse( DaoxCanvas *self, float x, float y, float rx, float ry );

DAO_CANVAS_DLL DaoxCanvasPath* DaoxCanvas_AddPath( DaoxCanvas *self, DaoxPath *path );

DAO_CANVAS_DLL DaoxCanvasText* DaoxCanvas_AddText( DaoxCanvas *self, const char *text, float x, float y, float degrees );

DAO_CANVAS_DLL DaoxCanvasText* DaoxCanvas_AddPathText( DaoxCanvas *self, const char *text, DaoxPath *path, float degrees );

DAO_CANVAS_DLL DaoxCanvasImage* DaoxCanvas_AddImage( DaoxCanvas *self, DaoImage *image, float x, float y, float w );




#ifdef __cplusplus
}
#endif

#endif
