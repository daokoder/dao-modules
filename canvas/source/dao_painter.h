/*
// Dao Canvas Module
// http://www.daovm.net
//
// Copyright (c) 2015-2017, Limin Fu
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

#ifndef __DAO_PAINTER_H__
#define __DAO_PAINTER_H__

#include "dao_canvas.h"
#include "dao_rasterizer.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef struct DaoxPainter     DaoxPainter;


struct DaoxPainter
{
	DAO_CSTRUCT_COMMON;

	DaoxOBBox2D      obbox;
	DaoImage        *buffer;
	DaoImage        *target;
	DaoxRenderer    *renderer;
	DaoxRasterizer  *rasterizer;
	DaoxGradient    *gradient;
};
DAO_CANVAS_DLL DaoType *daox_type_painter;

DAO_CANVAS_DLL DaoxPainter* DaoxPainter_New( DaoVmSpace *vmspace );
DAO_CANVAS_DLL void DaoxPainter_Delete( DaoxPainter *self );


DAO_CANVAS_DLL void DaoxPainter_Paint( DaoxPainter *self, DaoxCanvas *canvas, DaoxAABBox2D viewport );

DAO_CANVAS_DLL void DaoxPainter_PaintCanvasImage( DaoxPainter *self, DaoxCanvas *canvas, DaoxAABBox2D viewport, DaoImage *image, int width, int height );



#ifdef __cplusplus
}
#endif

#endif
