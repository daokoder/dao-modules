/*
// Polygon rasterizer, adapted from AGG2-Lite.
//
// Copyright (c) 2015, Limin Fu
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
// THIS SOFTWARE IS PROVIDED  BY THE COPYRIGHT HOLDERS AND  CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED  WARRANTIES,  INCLUDING,  BUT NOT LIMITED TO,  THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL  THE COPYRIGHT HOLDER OR CONTRIBUTORS  BE LIABLE FOR ANY DIRECT,
// INDIRECT,  INCIDENTAL, SPECIAL,  EXEMPLARY,  OR CONSEQUENTIAL  DAMAGES (INCLUDING,
// BUT NOT LIMITED TO,  PROCUREMENT OF  SUBSTITUTE  GOODS OR  SERVICES;  LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION)  HOWEVER CAUSED  AND ON ANY THEORY OF
// LIABILITY,  WHETHER IN CONTRACT,  STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------
// Anti-Grain Geometry - Version 2.1 Lite 
// Copyright (C) 2002-2003 Maxim Shemanarev (McSeem)
//
// Permission to copy, use, modify, sell and distribute this software 
// is granted provided this copyright notice appears in all copies. 
// This software is provided "as is" without express or implied
// warranty, and with no claim as to its suitability for any purpose.
//
// The author gratefully acknowleges the support of David Turner, 
// Robert Wilhelm, and Werner Lemberg - the authors of the FreeType 
// libray - in producing this work. See http://www.freetype.org for details.
//
// Contact: mcseem@antigrain.com
//          mcseemagg@yahoo.com
//          http://www.antigrain.com
//----------------------------------------------------------------------------
*/

#ifndef __DAO_RASTERIZER_H__
#define __DAO_RASTERIZER_H__

#include <string.h>
#include "dao_canvas.h"


typedef struct DaoxPixBrush    DaoxPixBrush;
typedef struct DaoxPixCell     DaoxPixCell;
typedef struct DaoxOutline     DaoxOutline;
typedef struct DaoxScanline    DaoxScanline;
typedef struct DaoxSpanTinter  DaoxSpanTinter;
typedef struct DaoxRenderer    DaoxRenderer;
typedef struct DaoxRasterizer  DaoxRasterizer;


struct DaoxPixBrush
{
	DaoxColor      color;
	DaoxGradient  *gradient;
	DaoxPathStyle *strokeStyle;  /* Only for dash pattern; */
};



//========================================================================
//
// This struct is used to transfer data from struct DaoxOutline (or a similar one)
// to the rendering buffer. It's organized very simple. The struct stores 
// information of horizontal spans to render it into a pixel-map buffer. 
// Each span has initial X, length, and an array of bytes that determine the 
// alpha-values for each pixel. So, the restriction of using this struct is 256 
// levels of Anti-Aliasing, which is quite enough for any practical purpose.
// Before using this struct you should know the minimal and maximal pixel 
// coordinates of your scanline. The protocol of using is:
// 1. reset(min_x, max_x)
// 2. add_cell() / add_span() - accumulate scanline. You pass Y-coordinate 
//    into these functions in order to make scanline know the last Y. Before 
//    calling add_cell() / add_span() you should check with method is_ready(y)
//    if the last Y has changed. It also checks if the scanline is not empty. 
//    When forming one scanline the next X coordinate must be always greater
//    than the last stored one, i.e. it works only with ordered coordinates.
// 3. If the current scanline is_ready() you should render it and then call 
//    reset_spans() before adding new cells/spans.
//    
// 4. Rendering:
// 
// DaoxScanline provides an iterator struct that allows you to extract
// the spans and the cover values for each pixel. Be aware that clipping
// has not been done yet, so you should perform it yourself.
// Use scanline::iterator to render spans:
//-------------------------------------------------------------------------
//
// int base_x = sl.base_x();          // base X. Should be added to the span's X
//                                    // "sl" is a const reference to the 
//                                    // scanline passed in.
//
// int y = sl.y();                    // Y-coordinate of the scanline
//
// ************************************
// ...Perform vertical clipping here...
// ************************************
//
// scanline::iterator span(sl);
// 
// uchar_t* row = m_rbuf->row(y); // The the address of the beginning 
//                                      // of the current row
// 
// unsigned num_spans = sl.num_spans(); // Number of spans. It's guaranteed that
//                                      // num_spans is always greater than 0.
//
// do
// {
//     int x = span.next() + base_x;        // The beginning X of the span
//
//     const uchar_t covers* = span.covers(); // The array of the cover values
//
//     int num_pix = span.num_pix();        // Number of pixels of the span.
//                                          // Always greater than 0, still we
//                                          // shoud use "int" instead of 
//                                          // "unsigned" because it's more
//                                          // convenient for clipping
//
//     **************************************
//     ...Perform horizontal clipping here...
//     ...you have x, covers, and pix_count..
//     **************************************
//
//     uchar_t* dst = row + x;  // Calculate the start address of the row.
//                                    // In this case we assume a simple 
//                                    // grayscale image 1-byte per pixel.
//     do
//     {
//         *dst++ = *covers++;        // Hypotetical rendering. 
//     }
//     while(--num_pix);
// } 
// while(--num_spans);  // num_spans cannot be 0, so this loop is quite safe
//------------------------------------------------------------------------
//
// The question is: why should we accumulate the whole scanline when we
// could render just separate spans when they're ready?
// That's because using the scanline is in general faster. When is consists 
// of more than one span the conditions for the processor cash system
// are better, because switching between two different areas of memory 
// (that can be large ones) occures less frequently.
//------------------------------------------------------------------------
struct DaoxScanline
{
    int        min_x;
    uint_t     max_len;
    int        dx;
    int        dy;
    int        last_x;
    int        last_y;

    uchar_t*   covers;
    uchar_t**  start_ptrs;
    ushort_t*  counts;
    uint_t     num_spans;

    uchar_t**  cur_start_ptr;
    ushort_t*  cur_count;
};

enum DaoxAntiAliasParams
{
	DAOX_AA_SHIFT = 8,
	DAOX_AA_NUM   = 1 << DAOX_AA_SHIFT,
	DAOX_AA_MASK  = DAOX_AA_NUM - 1,
	DAOX_AA_NUM2  = DAOX_AA_NUM * 2,
	DAOX_AA_MASK2 = DAOX_AA_NUM2 - 1
};

DaoxScanline* DaoxScanline_New();
void DaoxScanline_Delete( DaoxScanline *self );



struct DaoxSpanTinter
{
    void (*render)( uchar_t* ptr, int x, uint_t count, const uchar_t* covers, DaoxColor c );
    void (*render2)( uchar_t* ptr, int x, int y, uint_t count, const uchar_t* covers, DaoxPixBrush *b );
    void (*hline)( uchar_t* ptr, int x, uint_t count, DaoxColor c );
    void (*hline2)( uchar_t* ptr, int x, int y, uint_t count, DaoxPixBrush *b );
};

//========================================================================
// This struct template is used basically for rendering scanlines. 
// The 'DaoxSpanTinter' argument is one of the span renderers, such as DaoxSpanTinterRGB24 
// and others.
// 
// Usage:
// 
//     // Creation
//     agg::RenderingBuffer rbuf(ptr, w, h, stride);
//     agg::DaoxRenderer<agg::DaoxSpanTinterRGB24> ren(rbuf);
//     agg::DaoxRasterizer ras;
//
//     // Clear the frame buffer
//     ren.clear(agg::Color(0,0,0));
//
//     // Making polygon
//     // ras.MoveTo(. . .);
//     // ras.LineTo(. . .);
//     // . . .
//
//     // Rendering
//     ras.render(ren, agg::Color(200, 100, 80));
//  
//------------------------------------------------------------------------
struct DaoxRenderer
{
    DaoxImage    *rbuf;
    DaoxSpanTinter  span;
};

DaoxRenderer* DaoxRenderer_New( DaoxImage *rbuf );
void DaoxRenderer_Delete( DaoxRenderer *self );

void DaoxRenderer_Clear( DaoxRenderer *self, DaoxColor c);
void DaoxRenderer_SetPixel( DaoxRenderer *self, int x, int y, DaoxColor c);

void DaoxRenderer_Render(DaoxRenderer *self, DaoxScanline *sl, DaoxPixBrush *b);



//------------------------------------------------------------------------
// These constants determine the subpixel accuracy, to be more precise, 
// the number of bits of the fractional part of the coordinates. 
// The possible coordinate capacity in bits can be calculated by formula:
// sizeof(int) * 8 - DAOX_POLY_BASE_SHIFT * 2, i.e, for 32-bit integers and
// 8-bits fractional part the capacity is 16 bits or [-32768...32767].
enum DaoxPolyParams
{
    DAOX_POLY_BASE_SHIFT = 8,
    DAOX_POLY_BASE_SIZE  = 1 << DAOX_POLY_BASE_SHIFT,
    DAOX_POLY_BASE_MASK  = DAOX_POLY_BASE_SIZE - 1
};



enum DaoxCellParams
{
	DAOX_CELL_BLOCK_SHIFT = 12,
	DAOX_CELL_BLOCK_SIZE  = 1 << DAOX_CELL_BLOCK_SHIFT,
	DAOX_CELL_BLOCK_MASK  = DAOX_CELL_BLOCK_SIZE - 1,
	DAOX_CELL_BLOCK_POOL  = 256,
	DAOX_CELL_BLOCK_LIMIT = 1024
};

//------------------------------------------------------------------------
// A pixel cell. There're no constructors defined and it was done 
// intentionally in order to avoid extra overhead when allocating an 
// array of cells.
struct DaoxPixCell
{
	short x;
	short y;
    int   packed_coord;
    int   cover;
    int   area;
};

//------------------------------------------------------------------------
// An internal struct that implements the main rasterization algorithm.
// Used in the rasterizer. Should not be used direcly.
struct DaoxOutline
{
    uint_t  num_blocks;
    uint_t  max_blocks;
    uint_t  cur_block;
    uint_t  num_cells;

    DaoxPixCell**  cells;
    DaoxPixCell*   cur_cell_ptr;
    DaoxPixCell**  sorted_cells;
    uint_t         sorted_size;
    DaoxPixCell    cur_cell;

    int       cur_x;
    int       cur_y;
    int       close_x;
    int       close_y;
    int       min_x;
    int       min_y;
    int       max_x;
    int       max_y;
    uint_t    flags;
};

DaoxOutline* DaoxOutline_New();
void DaoxOutline_Delete( DaoxOutline *self );
void DaoxOutline_Reset( DaoxOutline *self );
void DaoxOutline_MoveTo( DaoxOutline *self, int x, int y);
void DaoxOutline_LineTo( DaoxOutline *self, int x, int y);



enum DaoxFillingRule
{
    DAOX_FILL_NON_ZERO,
    DAOX_FILL_EVEN_ODD
};


//========================================================================
// Polygon rasterizer that is used to render filled polygons with 
// high-quality Anti-Aliasing. Internally, by default, the struct uses 
// integer coordinates in format 24.8, i.e. 24 bits for integer part 
// and 8 bits for fractional - see DAOX_POLY_BASE_SHIFT. This struct can be 
// used in the following  way:
//
// 1. filling_rule(FillingRule ft) - optional.
//
// 2. gamma() - optional.
//
// 3. reset()
//
// 4. MoveTo(x, y) / LineTo(x, y) - make the polygon. One can create 
//    more than one contour, but each contour must consist of at least 3
//    vertices, i.e. MoveTo(x1, y1); LineTo(x2, y2); LineTo(x3, y3);
//    is the absolute minimum of vertices that define a triangle.
//    The algorithm does not check either the number of vertices nor
//    coincidence of their coordinates, but in the worst case it just 
//    won't draw anything.
//    The orger of the vertices (clockwise or counterclockwise) 
//    is important when using the non-zero filling rule (DAOX_FILL_NON_ZERO).
//    In this case the vertex order of all the contours must be the same
//    if you want your intersecting polygons to be without "holes".
//    You actually can use different vertices order. If the contours do not 
//    intersect each other the order is not important anyway. If they do, 
//    contours with the same vertex order will be rendered without "holes" 
//    while the intersecting contours with different orders will have "holes".
//
// filling_rule() and gamma() can be called anytime before "sweeping".
//------------------------------------------------------------------------
struct DaoxRasterizer
{
    DaoxOutline    *outline;
    DaoxScanline   *scanline;
    uchar_t         filling_rule;
    uchar_t         gamma[256];

};

DaoxRasterizer* DaoxRasterizer_New();
void DaoxRasterizer_Delete( DaoxRasterizer *self );

void DaoxRasterizer_Reset( DaoxRasterizer *self );
void DaoxRasterizer_MoveTo( DaoxRasterizer *self, int x, int y);
void DaoxRasterizer_LineTo( DaoxRasterizer *self, int x, int y);
void DaoxRasterizer_MoveToD( DaoxRasterizer *self, double x, double y);
void DaoxRasterizer_LineToD( DaoxRasterizer *self, double x, double y);
void DaoxRasterizer_SetGamma( DaoxRasterizer *self, double g);

void DaoxRasterizer_Render( DaoxRasterizer *self, DaoxRenderer *r, DaoxPixBrush *b );
int  DaoxRasterizer_HitTest( DaoxRasterizer *self, int tx, int ty);


DaoxSpanTinter SpanRGB24();
DaoxSpanTinter SpanRGBA32();


#endif

