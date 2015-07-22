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
//
// Class DaoxOutline - implementation.
//
// Initially the rendering algorithm was designed by David Turner and the 
// other authors of the FreeType library - see the above notice. I nearly 
// created a similar renderer, but still I was far from David's work. 
// I completely redesigned the original code and adapted it for Anti-Grain 
// ideas. Two functions - render_line and render_scanline are the core of 
// the algorithm - they calculate the exact coverage of each pixel cell
// of the polygon. I left these functions almost as is, because there's 
// no way to improve the perfection - hats off to David and his group!
//
// All other code is very different from the original. 
// 
//----------------------------------------------------------------------------
*/


#include <math.h>
#include <stdlib.h>
#include "dao_rasterizer.h"


typedef struct DaoxScanlineIterator DaoxScanlineIterator;


void DaoxPixCell_Set(DaoxPixCell *self, int x, int y, int c, int a);
void DaoxPixCell_SetCoord(DaoxPixCell *self, int x, int y);
void DaoxPixCell_SetCover(DaoxPixCell *self, int c, int a);
void DaoxPixCell_AddCover(DaoxPixCell *self, int c, int a);



DaoxScanline* DaoxScanline_New()
{
	DaoxScanline *self = (DaoxScanline*) dao_calloc( 1, sizeof(DaoxScanline) );
	self->last_x = self->last_y = 0x7FFF;
	return self;
}
void DaoxScanline_Delete( DaoxScanline *self )
{
	if( self->counts ) dao_free( self->counts );
	if( self->start_ptrs ) dao_free( self->start_ptrs );
	if( self->covers ) dao_free( self->covers );
	dao_free( self );
}

static void DaoxScanline_Reset( DaoxScanline *self, int min_x, int max_x, int dx, int dy)
{
    uint_t max_len = max_x - min_x + 2;
    if(max_len > self->max_len)
    {
		if( self->counts ) dao_free( self->counts );
		if( self->start_ptrs ) dao_free( self->start_ptrs );
		if( self->covers ) dao_free( self->covers );
        self->covers     = (uchar_t  *) dao_malloc( max_len*sizeof(uchar_t  ) );
        self->start_ptrs = (uchar_t* *) dao_malloc( max_len*sizeof(uchar_t* ) );
        self->counts     = (ushort_t*) dao_malloc( max_len*sizeof(ushort_t) );
        self->max_len    = max_len;
    }
    self->dx            = dx;
    self->dy            = dy;
    self->last_x        = 0x7FFF;
    self->last_y        = 0x7FFF;
    self->min_x         = min_x;
    self->cur_count     = self->counts;
    self->cur_start_ptr = self->start_ptrs;
    self->num_spans     = 0;
}


static void DaoxScanline_AddSpan( DaoxScanline *self, int x, int y, uint_t num, uint_t cover)
{
    x -= self->min_x;

    memset(self->covers + x, cover, num);
    if(x == self->last_x+1)
    {
        (*self->cur_count) += (ushort_t)num;
    }
    else
    {
        *++self->cur_count = (ushort_t)num;
        *++self->cur_start_ptr = self->covers + x;
        self->num_spans++;
    }
    self->last_x = x + num - 1;
    self->last_y = y;
}

static void DaoxScanline_ResetSpans( DaoxScanline *self )
{
    self->last_x        = 0x7FFF;
    self->last_y        = 0x7FFF;
    self->cur_count     = self->counts;
    self->cur_start_ptr = self->start_ptrs;
    self->num_spans     = 0;
}


static void DaoxScanline_AddCell( DaoxScanline *self, int x, int y, uint_t cover)
{
    x -= self->min_x;
    self->covers[x] = (uchar_t)cover;
    if(x == self->last_x+1)
    {
        (*self->cur_count)++;
    }
    else
    {
        *++self->cur_count = 1;
        *++self->cur_start_ptr = self->covers + x;
        self->num_spans++;
    }
    self->last_x = x;
    self->last_y = y;
}
static int DaoxScanline_IsReady( DaoxScanline *self, int y)
{
    return self->num_spans && (y ^ self->last_y);
}
static int  DaoxScanline_BaseX( const DaoxScanline *self )
{
	return self->min_x + self->dx;
}
static int  DaoxScanline_Y( const DaoxScanline *self )
{
	return self->last_y + self->dy;
}
static uint_t DaoxScanline_SpanCount( const DaoxScanline *self )
{
	return self->num_spans;
}


struct DaoxScanlineIterator
{
	uchar_t*  covers;
	ushort_t* cur_count;
	uchar_t** cur_start_ptr;
};

static DaoxScanlineIterator DaoxScanlineIterator_Init( DaoxScanline *sl )
{
	DaoxScanlineIterator iter;
	iter.covers = sl->covers;
	iter.cur_count = sl->counts;
	iter.cur_start_ptr = sl->start_ptrs;
	return iter;
}

static int DaoxScanlineIterator_Next( DaoxScanlineIterator *self )
{
	++self->cur_count;
	++self->cur_start_ptr;
	return (int)(*self->cur_start_ptr - self->covers);
}

static int DaoxScanlineIterator_PixCount( DaoxScanlineIterator *self )
{
	return (int)(*self->cur_count);
}
static const uchar_t* DaoxScanlineIterator_Covers( DaoxScanlineIterator *self )
{
	return *self->cur_start_ptr;
}




const uchar_t rasterizer_default_gamma[] = 
{
      0,  0,  1,  1,  2,  2,  3,  4,  4,  5,  5,  6,  7,  7,  8,  8,
      9, 10, 10, 11, 11, 12, 13, 13, 14, 14, 15, 16, 16, 17, 18, 18,
     19, 19, 20, 21, 21, 22, 22, 23, 24, 24, 25, 25, 26, 27, 27, 28,
     29, 29, 30, 30, 31, 32, 32, 33, 34, 34, 35, 36, 36, 37, 37, 38,
     39, 39, 40, 41, 41, 42, 43, 43, 44, 45, 45, 46, 47, 47, 48, 49,
     49, 50, 51, 51, 52, 53, 53, 54, 55, 55, 56, 57, 57, 58, 59, 60,
     60, 61, 62, 62, 63, 64, 65, 65, 66, 67, 68, 68, 69, 70, 71, 71,
     72, 73, 74, 74, 75, 76, 77, 78, 78, 79, 80, 81, 82, 83, 83, 84,
     85, 86, 87, 88, 89, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99,
    100,101,101,102,103,104,105,106,107,108,109,110,111,112,114,115,
    116,117,118,119,120,121,122,123,124,126,127,128,129,130,131,132,
    134,135,136,137,139,140,141,142,144,145,146,147,149,150,151,153,
    154,155,157,158,159,161,162,164,165,166,168,169,171,172,174,175,
    177,178,180,181,183,184,186,188,189,191,192,194,195,197,199,200,
    202,204,205,207,209,210,212,214,215,217,219,220,222,224,225,227,
    229,230,232,234,236,237,239,241,242,244,246,248,249,251,253,255
};





enum
{
    not_closed    = 1,
    sort_required = 2
};

void DaoxPixCell_SetCover(DaoxPixCell *self, int c, int a)
{
    self->cover = c;
    self->area = a;
}

void DaoxPixCell_AddCover(DaoxPixCell *self, int c, int a)
{
    self->cover += c;
    self->area += a;
}

void DaoxPixCell_SetCoord(DaoxPixCell *self, int cx, int cy)
{
    self->x = (short)cx;
    self->y = (short)cy;
    self->packed_coord = (cy << 16) + cx;
}

void DaoxPixCell_Set(DaoxPixCell *self, int cx, int cy, int c, int a)
{
    self->x = (short)cx;
    self->y = (short)cy;
    self->packed_coord = (cy << 16) + cx;
    self->cover = c;
    self->area = a;
}



DaoxOutline* DaoxOutline_New()
{
	DaoxOutline *self = (DaoxOutline*) dao_calloc( 1, sizeof(DaoxOutline) );
    self->min_x = 0x7FFFFFFF;
    self->min_y = 0x7FFFFFFF;
    self->max_x = -0x7FFFFFFF;
    self->max_y = -0x7FFFFFFF;
    self->flags = sort_required;
    DaoxPixCell_Set( &self->cur_cell, 0x7FFF, 0x7FFF, 0, 0);
	return self;
}

void DaoxOutline_Delete( DaoxOutline *self )
{
    if( self->sorted_cells ) dao_free( self->sorted_cells );
    if(self->num_blocks)
    {
        DaoxPixCell** ptr = self->cells + self->num_blocks - 1;
        while(self->num_blocks--)
        {
            dao_free( *ptr );
            ptr--;
        }
        dao_free( self->cells );
    }
	dao_free( self );
}

void DaoxOutline_Reset( DaoxOutline *self )
{ 
    self->num_cells = 0; 
    self->cur_block = 0;
    DaoxPixCell_Set( &self->cur_cell, 0x7FFF, 0x7FFF, 0, 0);
    self->flags |= sort_required;
    self->flags &= ~not_closed;
    self->min_x =  0x7FFFFFFF;
    self->min_y =  0x7FFFFFFF;
    self->max_x = -0x7FFFFFFF;
    self->max_y = -0x7FFFFFFF;
}



void DaoxOutline_AllocateBlock( DaoxOutline *self )
{
    if(self->cur_block >= self->num_blocks)
    {
        if(self->num_blocks >= self->max_blocks)
        {
			int count = self->max_blocks + DAOX_CELL_BLOCK_POOL;
            DaoxPixCell** new_cells = (DaoxPixCell* *) dao_malloc( count*sizeof(DaoxPixCell* ) );
            if(self->cells)
            {
                memcpy(new_cells, self->cells, self->max_blocks * sizeof(DaoxPixCell*));
                dao_free( self->cells );
            }
            self->cells = new_cells;
            self->max_blocks += DAOX_CELL_BLOCK_POOL;
        }
        self->cells[self->num_blocks++] = (DaoxPixCell*) dao_malloc( DAOX_CELL_BLOCK_SIZE*sizeof(DaoxPixCell) );
    }
    self->cur_cell_ptr = self->cells[self->cur_block++];
}


void DaoxOutline_AddCurrentCell( DaoxOutline *self )
{
    if(self->cur_cell.area | self->cur_cell.cover)
    {
        if((self->num_cells & DAOX_CELL_BLOCK_MASK) == 0)
        {
            if(self->num_blocks >= DAOX_CELL_BLOCK_LIMIT) return;
            DaoxOutline_AllocateBlock( self );
        }
        *self->cur_cell_ptr++ = self->cur_cell;
        self->num_cells++;
    }
}



void DaoxOutline_SetCurrentCell( DaoxOutline *self, int x, int y)
{
    if(self->cur_cell.packed_coord != (y << 16) + x)
    {
        DaoxOutline_AddCurrentCell( self );
        DaoxPixCell_Set( &self->cur_cell, x, y, 0, 0);
    }
}



void DaoxOutline_RenderDaoxScanline( DaoxOutline *self, int ey, int x1, int y1, int x2, int y2)
{
    int ex1 = x1 >> DAOX_POLY_BASE_SHIFT;
    int ex2 = x2 >> DAOX_POLY_BASE_SHIFT;
    int fx1 = x1 & DAOX_POLY_BASE_MASK;
    int fx2 = x2 & DAOX_POLY_BASE_MASK;

    int delta, p, first, dx;
    int incr, lift, mod, rem;

    //trivial case. Happens often
    if(y1 == y2)
    {
        DaoxOutline_SetCurrentCell( self, ex2, ey);
        return;
    }

    //everything is located in a single cell.  That is easy!
    if(ex1 == ex2)
    {
        delta = y2 - y1;
        DaoxPixCell_AddCover( &self->cur_cell, delta, (fx1 + fx2) * delta);
        return;
    }

    //ok, we'll have to render a run of adjacent cells on the same
    //scanline...
    p     = (DAOX_POLY_BASE_SIZE - fx1) * (y2 - y1);
    first = DAOX_POLY_BASE_SIZE;
    incr  = 1;

    dx = x2 - x1;

    if(dx < 0)
    {
        p     = fx1 * (y2 - y1);
        first = 0;
        incr  = -1;
        dx    = -dx;
    }

    delta = p / dx;
    mod   = p % dx;

    if(mod < 0)
    {
        delta--;
        mod += dx;
    }

    DaoxPixCell_AddCover( &self->cur_cell, delta, (fx1 + first) * delta);

    ex1 += incr;
    DaoxOutline_SetCurrentCell( self, ex1, ey);
    y1  += delta;

    if(ex1 != ex2)
    {
        p     = DAOX_POLY_BASE_SIZE * (y2 - y1 + delta);
        lift  = p / dx;
        rem   = p % dx;

        if (rem < 0)
        {
            lift--;
            rem += dx;
        }

        mod -= dx;

        while (ex1 != ex2)
        {
            delta = lift;
            mod  += rem;
            if(mod >= 0)
            {
                mod -= dx;
                delta++;
            }

            DaoxPixCell_AddCover( &self->cur_cell, delta, (DAOX_POLY_BASE_SIZE) * delta);
            y1  += delta;
            ex1 += incr;
            DaoxOutline_SetCurrentCell( self, ex1, ey);
        }
    }
    delta = y2 - y1;
    DaoxPixCell_AddCover( &self->cur_cell, delta, (fx2 + DAOX_POLY_BASE_SIZE - first) * delta);
}




void DaoxOutline_RenderLine( DaoxOutline *self, int x1, int y1, int x2, int y2)
{
    int ey1 = y1 >> DAOX_POLY_BASE_SHIFT;
    int ey2 = y2 >> DAOX_POLY_BASE_SHIFT;
    int fy1 = y1 & DAOX_POLY_BASE_MASK;
    int fy2 = y2 & DAOX_POLY_BASE_MASK;

    int dx, dy, x_from, x_to;
    int p, rem, mod, lift, delta, first, incr;

    if(ey1   < self->min_y) self->min_y = ey1;
    if(ey1+1 > self->max_y) self->max_y = ey1+1;
    if(ey2   < self->min_y) self->min_y = ey2;
    if(ey2+1 > self->max_y) self->max_y = ey2+1;

    dx = x2 - x1;
    dy = y2 - y1;

    //everything is on a single scanline
    if(ey1 == ey2)
    {
        DaoxOutline_RenderDaoxScanline( self, ey1, x1, fy1, x2, fy2);
        return;
    }

    //Vertical line - we have to calculate start and end cells,
    //and then - the common values of the area and coverage for
    //all cells of the line. We know exactly there's only one 
    //cell, so, we don't have to call DaoxOutline_RenderDaoxScanline( self ).
    incr  = 1;
    if(dx == 0)
    {
        int ex = x1 >> DAOX_POLY_BASE_SHIFT;
        int two_fx = (x1 - (ex << DAOX_POLY_BASE_SHIFT)) << 1;
        int area;

        first = DAOX_POLY_BASE_SIZE;
        if(dy < 0)
        {
            first = 0;
            incr  = -1;
        }

        x_from = x1;

        //render_scanline(ey1, x_from, fy1, x_from, first);
        delta = first - fy1;
        DaoxPixCell_AddCover( &self->cur_cell, delta, two_fx * delta);

        ey1 += incr;
        DaoxOutline_SetCurrentCell( self, ex, ey1);

        delta = first + first - DAOX_POLY_BASE_SIZE;
        area = two_fx * delta;
        while(ey1 != ey2)
        {
            //render_scanline(ey1, x_from, DAOX_POLY_BASE_SIZE - first, x_from, first);
            DaoxPixCell_SetCover( &self->cur_cell, delta, area);
            ey1 += incr;
            DaoxOutline_SetCurrentCell( self, ex, ey1);
        }
        //render_scanline(ey1, x_from, DAOX_POLY_BASE_SIZE - first, x_from, fy2);
        delta = fy2 - DAOX_POLY_BASE_SIZE + first;
        DaoxPixCell_AddCover( &self->cur_cell, delta, two_fx * delta);
        return;
    }

    //ok, we have to render several scanlines
    p     = (DAOX_POLY_BASE_SIZE - fy1) * dx;
    first = DAOX_POLY_BASE_SIZE;

    if(dy < 0)
    {
        p     = fy1 * dx;
        first = 0;
        incr  = -1;
        dy    = -dy;
    }

    delta = p / dy;
    mod   = p % dy;

    if(mod < 0)
    {
        delta--;
        mod += dy;
    }

    x_from = x1 + delta;
    DaoxOutline_RenderDaoxScanline( self, ey1, x1, fy1, x_from, first);

    ey1 += incr;
    DaoxOutline_SetCurrentCell( self, x_from >> DAOX_POLY_BASE_SHIFT, ey1);

    if(ey1 != ey2)
    {
        p     = DAOX_POLY_BASE_SIZE * dx;
        lift  = p / dy;
        rem   = p % dy;

        if(rem < 0)
        {
            lift--;
            rem += dy;
        }
        mod -= dy;

        while(ey1 != ey2)
        {
            delta = lift;
            mod  += rem;
            if (mod >= 0)
            {
                mod -= dy;
                delta++;
            }

            x_to = x_from + delta;
            DaoxOutline_RenderDaoxScanline( self, ey1, x_from, DAOX_POLY_BASE_SIZE - first, x_to, first);
            x_from = x_to;

            ey1 += incr;
            DaoxOutline_SetCurrentCell( self, x_from >> DAOX_POLY_BASE_SHIFT, ey1);
        }
    }
    DaoxOutline_RenderDaoxScanline( self, ey1, x_from, DAOX_POLY_BASE_SIZE - first, x2, fy2);
}


void DaoxOutline_MoveTo( DaoxOutline *self, int x, int y)
{
    if((self->flags & sort_required) == 0) DaoxOutline_Reset( self );
    if(self->flags & not_closed) DaoxOutline_LineTo( self, self->close_x, self->close_y);
    DaoxOutline_SetCurrentCell( self, x >> DAOX_POLY_BASE_SHIFT, y >> DAOX_POLY_BASE_SHIFT);
    self->close_x = self->cur_x = x;
    self->close_y = self->cur_y = y;
}



void DaoxOutline_LineTo( DaoxOutline *self, int x, int y)
{
    if((self->flags & sort_required) && ((self->cur_x ^ x) | (self->cur_y ^ y)))
    {
        int c;

        c = self->cur_x >> DAOX_POLY_BASE_SHIFT;
        if(c < self->min_x) self->min_x = c;
        ++c;
        if(c > self->max_x) self->max_x = c;

        c = x >> DAOX_POLY_BASE_SHIFT;
        if(c < self->min_x) self->min_x = c;
        ++c;
        if(c > self->max_x) self->max_x = c;

        DaoxOutline_RenderLine( self, self->cur_x, self->cur_y, x, y);
        self->cur_x = x;
        self->cur_y = y;
        self->flags |= not_closed;
    }
}


enum
{
    qsort_threshold = 9
};

static void swap_cells(DaoxPixCell** a, DaoxPixCell** b)
{
    DaoxPixCell *temp = *a;
    *a = *b;
    *b = temp;
}

static int less_than(DaoxPixCell** a, DaoxPixCell** b)
{
    return (*a)->packed_coord < (*b)->packed_coord;
}

void DaoxOutline_QSortCells( DaoxPixCell** start, uint_t num)
{
    DaoxPixCell**  stack[80];
    DaoxPixCell*** top; 
    DaoxPixCell**  limit;
    DaoxPixCell**  base;

    limit = start + num;
    base  = start;
    top   = stack;

    for (;;)
    {
        int len = (int)(limit - base);

        DaoxPixCell** i;
        DaoxPixCell** j;
        DaoxPixCell** pivot;

        if(len > qsort_threshold)
        {
            // we use base + len/2 as the pivot
            pivot = base + len / 2;
            swap_cells(base, pivot);

            i = base + 1;
            j = limit - 1;

            // now ensure that *i <= *base <= *j 
            if(less_than(j, i))
            {
                swap_cells(i, j);
            }

            if(less_than(base, i))
            {
                swap_cells(base, i);
            }

            if(less_than(j, base))
            {
                swap_cells(base, j);
            }

            for(;;)
            {
                do i++; while( less_than(i, base) );
                do j--; while( less_than(base, j) );

                if ( i > j )
                {
                    break;
                }

                swap_cells(i, j);
            }

            swap_cells(base, j);

            // now, push the largest sub-array
            if(j - base > limit - i)
            {
                top[0] = base;
                top[1] = j;
                base   = i;
            }
            else
            {
                top[0] = i;
                top[1] = limit;
                limit  = j;
            }
            top += 2;
        }
        else
        {
            // the sub-array is small, perform insertion sort
            j = base;
            i = j + 1;

            for(; i < limit; j = i, i++)
            {
                for(; less_than(j + 1, j); j--)
                {
                    swap_cells(j + 1, j);
                    if (j == base)
                    {
                        break;
                    }
                }
            }
            if(top > stack)
            {
                top  -= 2;
                base  = top[0];
                limit = top[1];
            }
            else
            {
                break;
            }
        }
    }
}

static void DaoxOutline_SortCells( DaoxOutline *self )
{
	DaoxPixCell** sorted_ptr;
	DaoxPixCell** block_ptr;
	DaoxPixCell*  cell_ptr;
	int i, nb;

    if(self->num_cells == 0) return;
    if(self->num_cells > self->sorted_size)
    {
		if( self->sorted_cells ) dao_free( self->sorted_cells );
        self->sorted_size = self->num_cells;
        self->sorted_cells = (DaoxPixCell**) dao_malloc( (self->num_cells + 1)*sizeof(DaoxPixCell*) );
    }

    sorted_ptr = self->sorted_cells;
    block_ptr = self->cells;

    nb = self->num_cells >> DAOX_CELL_BLOCK_SHIFT;
    while(nb--)
    {
        cell_ptr = *block_ptr++;
        i = DAOX_CELL_BLOCK_SIZE;
        while(i--) 
        {
            *sorted_ptr++ = cell_ptr++;
        }
    }

    cell_ptr = *block_ptr++;
    i = self->num_cells & DAOX_CELL_BLOCK_MASK;
    while(i--) 
    {
        *sorted_ptr++ = cell_ptr++;
    }
    self->sorted_cells[self->num_cells] = 0;
    DaoxOutline_QSortCells(self->sorted_cells, self->num_cells);
}

static DaoxPixCell** DaoxOutline_Cells( DaoxOutline *self )
{
    if(self->flags & not_closed)
    {
        DaoxOutline_LineTo( self, self->close_x, self->close_y);
        self->flags &= ~not_closed;
    }

    //Perform sort only the first time.
    if(self->flags & sort_required)
    {
        DaoxOutline_AddCurrentCell( self );
        if(self->num_cells == 0) return 0;
        DaoxOutline_SortCells( self );
        self->flags &= ~sort_required;
    }
    return self->sorted_cells;
}





DaoxRenderer* DaoxRenderer_New( DaoImage *rbuf )
{
	DaoxRenderer *self = (DaoxRenderer*) dao_malloc( sizeof(DaoxRenderer) );
	self->rbuf = rbuf;
	//self->span = SpanRGB24();
	switch( rbuf->depth ){
	case DAOX_IMAGE_BIT24 : self->span = SpanRGB24();  break;
	case DAOX_IMAGE_BIT32 : self->span = SpanRGBA32(); break;
	}
	return self;
}
void DaoxRenderer_Delete( DaoxRenderer *self )
{
	dao_free( self );
}

void DaoxRenderer_Clear( DaoxRenderer *self, DaoxColor c)
{
	uint_t y;
	for(y = 0; y < self->rbuf->height; y++)
	{
		uchar_t *row = self->rbuf->buffer.data.uchars + y * self->rbuf->stride;
		self->span.hline(row, 0, self->rbuf->width, c);
	}
}

static int DaoImage_InBox( DaoImage *self, int x, int y )
{
	return x >= 0 && y >= 0 && x < (int)self->width && y < (int)self->height;
}
void DaoxRenderer_SetPixel( DaoxRenderer *self, int x, int y, DaoxColor c)
{
	if( DaoImage_InBox( self->rbuf, x, y) )
	{
		uchar_t *row = self->rbuf->buffer.data.uchars + y * self->rbuf->stride;
		self->span.hline(row, x, 1, c);
	}
}


void DaoxRenderer_Render(DaoxRenderer *self, DaoxScanline *sl, DaoxPixBrush *b )
{
	DaoxScanlineIterator span;
	uchar_t *row;
	uint_t num_spans;
	int sly, base_x;
	
	sly = DaoxScanline_Y( sl );
	if( sly < 0 || sly >= (int)(self->rbuf->height))
	{
		return;
	}

	num_spans = DaoxScanline_SpanCount( sl );
	base_x = DaoxScanline_BaseX( sl );
	row = self->rbuf->buffer.data.uchars + sly * self->rbuf->stride;
	span = DaoxScanlineIterator_Init( sl );

	do
	{
		int x = DaoxScanlineIterator_Next( &span ) + base_x;
		const uchar_t* covers = DaoxScanlineIterator_Covers( & span );
		int num_pix = DaoxScanlineIterator_PixCount( & span );
		if(x < 0)
		{
			num_pix += x;
			if(num_pix <= 0) continue;
			covers -= x;
			x = 0;
		}
		if(x + num_pix >= (int)(self->rbuf->width))
		{
			num_pix = self->rbuf->width - x;
			if(num_pix <= 0) continue;
		}
		if( b->gradient ){
			self->span.render2(row, x, sly, num_pix, covers, b);
		}else{
			self->span.render(row, x, num_pix, covers, b->color);
		}
	}
	while(--num_spans);
}



DaoxRasterizer* DaoxRasterizer_New()
{
	DaoxRasterizer *self = (DaoxRasterizer*) dao_calloc( 1, sizeof(DaoxRasterizer) );
	self->outline = DaoxOutline_New();
	self->scanline = DaoxScanline_New();
	self->filling_rule = DAOX_FILL_NON_ZERO;
	memcpy(self->gamma, rasterizer_default_gamma, sizeof(self->gamma));
	return self;
}
void DaoxRasterizer_Delete( DaoxRasterizer *self )
{
	DaoxOutline_Delete( self->outline );
	DaoxScanline_Delete( self->scanline );
	dao_free( self );
}
void DaoxRasterizer_Reset( DaoxRasterizer *self )
{
	DaoxOutline_Reset( self->outline );
}

void DaoxRasterizer_MoveTo( DaoxRasterizer *self, int x, int y)
{
	DaoxOutline_MoveTo( self->outline, x, y);
}
void DaoxRasterizer_LineTo( DaoxRasterizer *self, int x, int y)
{
	DaoxOutline_LineTo( self->outline, x, y);
}

static int poly_coord(double c)
{
    return (int)(c * DAOX_POLY_BASE_SIZE);
}
void DaoxRasterizer_MoveToD( DaoxRasterizer *self, double x, double y)
{
	DaoxOutline_MoveTo( self->outline, poly_coord(x), poly_coord(y));
}
void DaoxRasterizer_LineToD( DaoxRasterizer *self, double x, double y)
{
	DaoxOutline_LineTo( self->outline, poly_coord(x), poly_coord(y));
}

void DaoxRasterizer_SetGamma( DaoxRasterizer *self, double g)
{
    uint_t i;
    for(i = 0; i < 256; i++)
    {
        self->gamma[i] = (uchar_t)(pow((double)i / 255.0, g) * 255.0);
    }
}


static void DaoxRasterizer_SetGamma2( DaoxRasterizer *self, const uchar_t* g)
{
    memcpy(self->gamma, g, sizeof(self->gamma));
}

static uint_t DaoxRasterizer_CalculateAlpha( DaoxRasterizer *self, int area) 
{
	int cover = area >> (DAOX_POLY_BASE_SHIFT*2 + 1 - DAOX_AA_SHIFT);

	if(cover < 0) cover = -cover;
	if(self->filling_rule == DAOX_FILL_EVEN_ODD)
	{
		cover &= DAOX_AA_MASK2;
		if(cover > DAOX_AA_NUM)
		{
			cover = DAOX_AA_NUM2 - cover;
		}
	}
	if(cover > DAOX_AA_MASK) cover = DAOX_AA_MASK;
	return cover;
}

void DaoxRasterizer_Render( DaoxRasterizer *self, DaoxRenderer *r, DaoxPixBrush *b )
{
	DaoxPixCell** cells = DaoxOutline_Cells( self->outline );
	const DaoxPixCell* cur_cell = *cells++;
	int x, y;
	int cover;
	int alpha;
	int area;

	if(self->outline->num_cells == 0) return;


	DaoxScanline_Reset( self->scanline, self->outline->min_x, self->outline->max_x, 0, 0);

	cover = 0;
	for(;;)
	{
		const DaoxPixCell* start_cell = cur_cell;

		int coord  = cur_cell->packed_coord;
		x = cur_cell->x;
		y = cur_cell->y;

		area   = start_cell->area;
		cover += start_cell->cover;

		//accumulate all start cells
		while((cur_cell = *cells++) != 0)
		{
			if(cur_cell->packed_coord != coord) break;
			area  += cur_cell->area;
			cover += cur_cell->cover;
		}

		if(area)
		{
			alpha = DaoxRasterizer_CalculateAlpha( self, (cover << (DAOX_POLY_BASE_SHIFT + 1)) - area);
			if(alpha)
			{
				if( DaoxScanline_IsReady( self->scanline, y) )
				{
					DaoxRenderer_Render( r, self->scanline, b );
					DaoxScanline_ResetSpans( self->scanline );
				}
				DaoxScanline_AddCell( self->scanline, x, y, self->gamma[alpha] );
			}
			x++;
		}

		if(!cur_cell) break;

		if(cur_cell->x > x)
		{
			alpha = DaoxRasterizer_CalculateAlpha( self, cover << (DAOX_POLY_BASE_SHIFT + 1));
			if(alpha)
			{
				if( DaoxScanline_IsReady( self->scanline, y) )
				{
					DaoxRenderer_Render( r, self->scanline, b );
					DaoxScanline_ResetSpans( self->scanline );
				}
				DaoxScanline_AddSpan( self->scanline, x, y, 
						cur_cell->x - x, 
						self->gamma[alpha]);
			}
		}
	} 

	if( DaoxScanline_SpanCount( self->scanline ))
	{
		DaoxRenderer_Render( r, self->scanline, b );
	}
}

int DaoxRasterizer_HitTest( DaoxRasterizer *self, int tx, int ty)
{
    DaoxPixCell** cells = DaoxOutline_Cells( self->outline );
    const DaoxPixCell* cur_cell = *cells++;
    int x, y;
    int cover;
    int alpha;
    int area;

    if(self->outline->num_cells == 0) return 0;

    cover = 0;
    for(;;)
    {
        const DaoxPixCell* start_cell = cur_cell;

        int coord  = cur_cell->packed_coord;
        x = cur_cell->x;
        y = cur_cell->y;

        if(y > ty) return 0;

        area   = start_cell->area;
        cover += start_cell->cover;

        while((cur_cell = *cells++) != 0)
        {
            if(cur_cell->packed_coord != coord) break;
            area  += cur_cell->area;
            cover += cur_cell->cover;
        }

        if(area)
        {
            alpha = DaoxRasterizer_CalculateAlpha( self, (cover << (DAOX_POLY_BASE_SHIFT + 1)) - area);
            if(alpha)
            {
                if(tx == x && ty == y) return 1;
            }
            x++;
        }

        if(!cur_cell) break;

        if(cur_cell->x > x)
        {
            alpha = DaoxRasterizer_CalculateAlpha( self, cover << (DAOX_POLY_BASE_SHIFT + 1));
            if(alpha)
            {
                if(ty == y && tx >= x && tx <= cur_cell->x) return 1;
            }
        }
    }
    return 0;
}



static void SpanRGB24_Render(uchar_t* ptr, int x, uint_t count, const uchar_t* covers, DaoxColor c)
{
	uchar_t red   = 255 * c.red;
	uchar_t green = 255 * c.green;
	uchar_t blue  = 255 * c.blue;
	uchar_t alpha = 255 * c.alpha;
	uchar_t* p = ptr + x + x + x;
	do
	{
		int a = (*covers++) * alpha;
		int r = p[0];
		int g = p[1];
		int b = p[2];
		*p++ = (((red   - r) * a) + (r << 16)) >> 16;
		*p++ = (((green - g) * a) + (g << 16)) >> 16;
		*p++ = (((blue  - b) * a) + (b << 16)) >> 16;
	}
	while(--count);
}

static void SpanRGB24_HLine(uchar_t* ptr, int x, uint_t count, DaoxColor c)
{
	uchar_t red   = 255 * c.red;
	uchar_t green = 255 * c.green;
	uchar_t blue  = 255 * c.blue;
	uchar_t* p = ptr + x + x + x;
	do { *p++ = red; *p++ = green; *p++ = blue; } while(--count);
}

DaoxSpanTinter SpanRGB24()
{
	DaoxSpanTinter span;
	span.render = SpanRGB24_Render;
	span.hline = SpanRGB24_HLine;
	return span;
}




static void SpanRGBA32_Render(uchar_t* ptr, int x, uint_t count, const uchar_t* covers, DaoxColor c)
{
	uchar_t red   = 255 * c.red;
	uchar_t green = 255 * c.green;
	uchar_t blue  = 255 * c.blue;
	uchar_t alpha = 255 * c.alpha;
	uchar_t* p = ptr + (x << 2);
	do
	{
		int a2 = (*covers++) * alpha;
		int r = p[0];
		int g = p[1];
		int b = p[2];
		int a = p[3];
		*p++ = (((red   - r) * a2) + (r << 16)) >> 16;
		*p++ = (((green - g) * a2) + (g << 16)) >> 16;
		*p++ = (((blue  - b) * a2) + (b << 16)) >> 16;
		*p++ = (((alpha - a) * a2) + (a << 16)) >> 16;
	}
	while(--count);
}

static void SpanRGBA32_Render2(uchar_t* ptr, int x, int y, uint_t count, const uchar_t* covers, DaoxPixBrush *brush )
{
	uchar_t* p = ptr + (x << 2);
	do
	{
		DaoxVector2D point = DaoxVector2D_XY( x++, y );
		DaoxColor c = DaoxGradient_ComputeColor( brush->gradient, point );
		uchar_t red   = 255 * c.red;
		uchar_t green = 255 * c.green;
		uchar_t blue  = 255 * c.blue;
		uchar_t alpha = 255 * c.alpha;
		int a2 = (*covers++) * alpha;
		int r = p[0];
		int g = p[1];
		int b = p[2];
		int a = p[3];
		*p++ = (((red   - r) * a2) + (r << 16)) >> 16;
		*p++ = (((green - g) * a2) + (g << 16)) >> 16;
		*p++ = (((blue  - b) * a2) + (b << 16)) >> 16;
		*p++ = (((alpha - a) * a2) + (a << 16)) >> 16;
	}
	while(--count);
}

static void SpanRGBA32_HLine(uchar_t* ptr, int x, uint_t count, DaoxColor c)
{
	uchar_t red   = 255 * c.red;
	uchar_t green = 255 * c.green;
	uchar_t blue  = 255 * c.blue;
	uchar_t alpha = 255 * c.alpha;
	uchar_t* p = ptr + (x << 2);
	do { *p++ = red; *p++ = green; *p++ = blue; *p++ = alpha; } while(--count);
}

static void SpanRGBA32_HLine2(uchar_t* ptr, int x, int y, uint_t count, DaoxPixBrush *brush )
{
	uchar_t* p = ptr + (x << 2);
	do {
		DaoxVector2D point = DaoxVector2D_XY( x++, y );
		DaoxColor c = DaoxGradient_ComputeColor( brush->gradient, point );
		uchar_t red   = 255 * c.red;
		uchar_t green = 255 * c.green;
		uchar_t blue  = 255 * c.blue;
		uchar_t alpha = 255 * c.alpha;
		*p++ = red; *p++ = green; *p++ = blue; *p++ = alpha;
	} while(--count);
}

DaoxSpanTinter SpanRGBA32()
{
	DaoxSpanTinter span;
	span.render = SpanRGBA32_Render;
	span.render2 = SpanRGBA32_Render2;
	span.hline  = SpanRGBA32_HLine;
	span.hline2 = SpanRGBA32_HLine2;
	return span;
}




