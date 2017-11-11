// WonGradientFill.c --- GradientFill API clone by katahiromz
// Copyright (C) 2017 Katayama Hirofumi MZ.
#include <windows.h>
#include <assert.h>

#ifndef INLINE
    #ifdef __cplusplus
        #define INLINE inline
    #else
        #define INLINE __inline
    #endif
#endif

/* https://en.wikipedia.org/wiki/Ordered_dithering */
static INLINE BYTE BayerDithering(ULONG x, ULONG y, BYTE b)
{
#if 1
    #define BV(x) (x * 255 / 64)
    static const BYTE s_bayer_64[] =
    {
        BV(0),  BV(48), BV(12), BV(60), BV(3),  BV(51), BV(15), BV(63),
        BV(32), BV(16), BV(44), BV(28), BV(35), BV(19), BV(47), BV(31),
        BV(8),  BV(56), BV(4),  BV(52), BV(11), BV(59), BV(7),  BV(55),
        BV(40), BV(24), BV(36), BV(20), BV(43), BV(27), BV(39), BV(23),
        BV(2),  BV(50), BV(14), BV(62), BV(1),  BV(49), BV(13), BV(61),
        BV(34), BV(18), BV(46), BV(30), BV(33), BV(17), BV(45), BV(29),
        BV(10), BV(58), BV(6),  BV(54), BV(9),  BV(57), BV(5),  BV(53),
        BV(42), BV(26), BV(38), BV(22), BV(41), BV(25), BV(37), BV(21)
    };
    return ((s_bayer_64[(y & 7) * 8 + (x & 7)] <= b) ? 255 : 0);
#else
    #define BV(x) (x * 255 / 16)
    static const BYTE s_bayer_16[] =
    {
        BV(0), BV(8), BV(2), BV(10),
        BV(12), BV(4), BV(14), BV(6),
        BV(3), BV(11), BV(1), BV(9),
        BV(15), BV(7), BV(13), BV(5)
    };
    return ((s_bayer_16[(y & 3) * 4 + (x & 3)] <= b) ? 255 : 0);
#endif
#undef BV
}

typedef struct XYMINMAX
{
    LONG xMin, yMin, xMax, yMax;
} XYMINMAX;

static INLINE void
GetXYMinMax(XYMINMAX *pXYMinMax, TRIVERTEX *pTriVertex, ULONG dwNumVertex)
{
    ULONG i;
    LONG x, y, xMin, yMin, xMax, yMax;

    xMin = xMax = pTriVertex[0].x;
    yMin = yMax = pTriVertex[0].y;

    for (i = 1; i < dwNumVertex; ++i)
    {
        x = pTriVertex[i].x;
        if (x < xMin)
            xMin = x;
        if (x > xMax)
            xMax = x;
        y = pTriVertex[i].y;
        if (y < yMin)
            yMin = y;
        if (y > yMax)
            yMax = y;
    }

    pXYMinMax->xMin = xMin;
    pXYMinMax->yMin = yMin;
    pXYMinMax->xMax = xMax;
    pXYMinMax->yMax = yMax;
}

static void WINAPI
MeshFillRectH(LPBYTE pbBits, ULONG cx, ULONG cy, const TRIVERTEX *pTriVertex,
              const GRADIENT_RECT *rect, INT xMin, INT yMin, BOOL bDither)
{
    COLOR16 r1, g1, b1, a1;
    COLOR16 dr, dg, db, da;
    INT dx, dy, x1, y1;
    ULONG stride;
    LPBYTE pb;

    const TRIVERTEX *v1 = pTriVertex + rect->UpperLeft;
    const TRIVERTEX *v2 = pTriVertex + rect->LowerRight;
    if (v1->x > v2->x)
    {
        const TRIVERTEX *tmp = v2;
        v2 = v1;
        v1 = tmp;
    }
    assert(v1->x <= v2->x);

    dx = v2->x - v1->x;
    dy = v2->y - v1->y;
    r1 = v1->Red;
    g1 = v1->Green;
    b1 = v1->Blue;
    a1 = v1->Alpha;

    if (dx != 0)
    {
        dr = (v2->Red - v1->Red) / dx;
        dg = (v2->Green - v1->Green) / dx;
        db = (v2->Blue - v1->Blue) / dx;
        da = (v2->Alpha - v1->Alpha) / dx;
    }

    if (dy < 0)
    {
        pb = pbBits + ((v2->y - yMin) * cx + (v1->x - xMin)) * 4;
        dy = -dy;
    }
    else
    {
        pb = pbBits + ((v1->y - yMin) * cx + (v1->x - xMin)) * 4;
    }

    stride = cx * 4;
    if (bDither)
    {
        for (x1 = 0; x1 < dx; x1++)
        {
            for (y1 = 0; y1 < dy; y1++)
            {
                *pb++ = BayerDithering(x1, y1, b1 >> 8);
                *pb++ = BayerDithering(x1, y1, g1 >> 8);
                *pb++ = BayerDithering(x1, y1, r1 >> 8);
                pb++;
                pb += stride - 4;
            }
            r1 += dr;
            g1 += dg;
            b1 += db;
            pb -= dy * stride - 4;
        }
    }
    else
    {
        for (x1 = 0; x1 < dx; x1++)
        {
            for (y1 = 0; y1 < dy; y1++)
            {
                *pb++ = b1 >> 8;
                *pb++ = g1 >> 8;
                *pb++ = r1 >> 8;
                *pb++ = a1 >> 8;
                pb += stride - 4;
            }
            r1 += dr;
            g1 += dg;
            b1 += db;
            a1 += da;
            pb -= dy * stride - 4;
        }
    }
}

static void WINAPI
MeshFillRectV(LPBYTE pbBits, ULONG cx, ULONG cy, const TRIVERTEX *pTriVertex,
              const GRADIENT_RECT *rect, INT xMin, INT yMin, BOOL bDither)
{
    COLOR16 r1, g1, b1, a1;
    COLOR16 dr, dg, db, da;
    INT dx, dy, x1, y1;
    ULONG stride;
    LPBYTE pb;

    const TRIVERTEX *v1 = pTriVertex + rect->UpperLeft;
    const TRIVERTEX *v2 = pTriVertex + rect->LowerRight;
    if (v1->y > v2->y)
    {
        const TRIVERTEX *tmp = v2;
        v2 = v1;
        v1 = tmp;
    }
    assert(v1->y <= v2->y);

    dx = v2->x - v1->x;
    dy = v2->y - v1->y;
    r1 = v1->Red;
    g1 = v1->Green;
    b1 = v1->Blue;
    a1 = v1->Alpha;

    if (dy != 0)
    {
        dr = (v2->Red - v1->Red) / dy;
        dg = (v2->Green - v1->Green) / dy;
        db = (v2->Blue - v1->Blue) / dy;
        da = (v2->Alpha - v1->Alpha) / dy;
    }

    if (dx < 0)
    {
        pb = pbBits + ((v1->y - yMin) * cx + (v2->x - xMin)) * 4;
        dx = -dx;
    }
    else
    {
        pb = pbBits + ((v1->y - yMin) * cx + (v1->x - xMin)) * 4;
    }

    stride = cx * 4;
    if (bDither)
    {
        for (y1 = 0; y1 < dy; y1++)
        {
            for (x1 = 0; x1 < dx; x1++)
            {
                *pb++ = BayerDithering(x1, y1, b1 >> 8);
                *pb++ = BayerDithering(x1, y1, g1 >> 8);
                *pb++ = BayerDithering(x1, y1, r1 >> 8);
                pb++;
            }
            r1 += dr;
            g1 += dg;
            b1 += db;
            pb -= dx * 4;
            pb += stride;
        }
    }
    else
    {
        for (y1 = 0; y1 < dy; y1++)
        {
            for (x1 = 0; x1 < dx; x1++)
            {
                *pb++ = b1 >> 8;
                *pb++ = g1 >> 8;
                *pb++ = r1 >> 8;
                *pb++ = a1 >> 8;
            }
            r1 += dr;
            g1 += dg;
            b1 += db;
            a1 += da;
            pb -= dx * 4;
            pb += stride;
        }
    }
}

static void WINAPI
MeshFillTriangle(LPBYTE pbBits, ULONG cx, ULONG cy,
                 const TRIVERTEX *v1,
                 const TRIVERTEX *v2,
                 const TRIVERTEX *v3,
                 const GRADIENT_TRIANGLE *triangle,
                 INT xMin, INT yMin, BOOL bDither)
{
    COLOR16 r1, g1, b1, a1, r2, g2, b2, a2;
    COLOR16 dr, dg, db, da;
    INT dx, x1, y1, x2;
    LPBYTE pb;
    ULONG stride = cx * 4;

    if (bDither)
    {
        for (y1 = v1->y; y1 < v2->y; y1++)
        {
            if (v2->x < v3->x)
            {
                x1 = v1->x + (v2->x - v1->x) * (y1 - v1->y) / (v2->y - v1->y);
                r1 = (COLOR16)(v1->Red   + (v2->Red   - v1->Red)   * (y1 - v1->y) / (v2->y - v1->y));
                g1 = (COLOR16)(v1->Green + (v2->Green - v1->Green) * (y1 - v1->y) / (v2->y - v1->y));
                b1 = (COLOR16)(v1->Blue  + (v2->Blue  - v1->Blue)  * (y1 - v1->y) / (v2->y - v1->y));
                x2 = v1->x + (v3->x - v1->x) * (y1 - v1->y) / (v3->y - v1->y);
                r2 = (COLOR16)(v1->Red   + (v3->Red   - v1->Red)   * (y1 - v1->y) / (v3->y - v1->y));
                g2 = (COLOR16)(v1->Green + (v3->Green - v1->Green) * (y1 - v1->y) / (v3->y - v1->y));
                b2 = (COLOR16)(v1->Blue  + (v3->Blue  - v1->Blue)  * (y1 - v1->y) / (v3->y - v1->y));
                dx = x2 - x1;
                if (dx != 0)
                {
                    db = (b2 - b1) / dx;
                    dg = (g2 - g1) / dx;
                    dr = (r2 - r1) / dx;
                }
                pb = pbBits + (x1 - xMin) * 4 + (y1 - yMin) * stride;
                for ( ; x1 < x2; x1++)
                {
                    *pb++ = BayerDithering(x1, y1, b1 >> 8);
                    *pb++ = BayerDithering(x1, y1, g1 >> 8);
                    *pb++ = BayerDithering(x1, y1, r1 >> 8);
                    pb++;
                    b1 += db;
                    g1 += dg;
                    r1 += dr;
                }
            }
            else
            {
                x1 = v1->x + (v3->x - v1->x) * (y1 - v1->y) / (v3->y - v1->y);
                r1 = (COLOR16)(v1->Red   + (v3->Red   - v1->Red)   * (y1 - v1->y) / (v3->y - v1->y));
                g1 = (COLOR16)(v1->Green + (v3->Green - v1->Green) * (y1 - v1->y) / (v3->y - v1->y));
                b1 = (COLOR16)(v1->Blue  + (v3->Blue  - v1->Blue)  * (y1 - v1->y) / (v3->y - v1->y));
                x2 = v1->x + (v2->x - v1->x) * (y1 - v1->y) / (v2->y - v1->y);
                r2 = (COLOR16)(v1->Red   + (v2->Red   - v1->Red)   * (y1 - v1->y) / (v2->y - v1->y));
                g2 = (COLOR16)(v1->Green + (v2->Green - v1->Green) * (y1 - v1->y) / (v2->y - v1->y));
                b2 = (COLOR16)(v1->Blue  + (v2->Blue  - v1->Blue)  * (y1 - v1->y) / (v2->y - v1->y));
                dx = x2 - x1;
                if (dx != 0)
                {
                    db = (b2 - b1) / dx;
                    dg = (g2 - g1) / dx;
                    dr = (r2 - r1) / dx;
                }
                pb = pbBits + (x1 - xMin) * 4 + (y1 - yMin) * stride;
                for ( ; x1 < x2; x1++)
                {
                    *pb++ = BayerDithering(x1, y1, b1 >> 8);
                    *pb++ = BayerDithering(x1, y1, g1 >> 8);
                    *pb++ = BayerDithering(x1, y1, r1 >> 8);
                    pb++;
                    b1 += db;
                    g1 += dg;
                    r1 += dr;
                }
            }
        }
        for (y1 = v2->y; y1 < v3->y; y1++)
        {
            if (v2->x < v3->x)
            {
                x1 = v2->x + (v3->x - v2->x) * (y1 - v2->y) / (v3->y - v2->y);
                r1 = (COLOR16)(v2->Red   + (v3->Red   - v2->Red)   * (y1 - v2->y) / (v3->y - v2->y));
                g1 = (COLOR16)(v2->Green + (v3->Green - v2->Green) * (y1 - v2->y) / (v3->y - v2->y));
                b1 = (COLOR16)(v2->Blue  + (v3->Blue  - v2->Blue)  * (y1 - v2->y) / (v3->y - v2->y));
                x2 = v1->x + (v3->x - v1->x) * (y1 - v1->y) / (v3->y - v1->y);
                r2 = (COLOR16)(v1->Red   + (v3->Red   - v1->Red)   * (y1 - v1->y) / (v3->y - v1->y));
                g2 = (COLOR16)(v1->Green + (v3->Green - v1->Green) * (y1 - v1->y) / (v3->y - v1->y));
                b2 = (COLOR16)(v1->Blue  + (v3->Blue  - v1->Blue)  * (y1 - v1->y) / (v3->y - v1->y));
                dx = x2 - x1;
                if (dx != 0)
                {
                    db = (b2 - b1) / dx;
                    dg = (g2 - g1) / dx;
                    dr = (r2 - r1) / dx;
                }
                pb = pbBits + (x1 - xMin) * 4 + (y1 - yMin) * stride;
                for ( ; x1 < x2; x1++)
                {
                    *pb++ = BayerDithering(x1, y1, b1 >> 8);
                    *pb++ = BayerDithering(x1, y1, g1 >> 8);
                    *pb++ = BayerDithering(x1, y1, r1 >> 8);
                    pb++;
                    b1 += db;
                    g1 += dg;
                    r1 += dr;
                }
            }
            else
            {
                x1 = v1->x + (v3->x - v1->x) * (y1 - v1->y) / (v3->y - v1->y);
                r1 = (COLOR16)(v1->Red   + (v3->Red   - v1->Red)   * (y1 - v1->y) / (v3->y - v1->y));
                g1 = (COLOR16)(v1->Green + (v3->Green - v1->Green) * (y1 - v1->y) / (v3->y - v1->y));
                b1 = (COLOR16)(v1->Blue  + (v3->Blue  - v1->Blue)  * (y1 - v1->y) / (v3->y - v1->y));
                x2 = v2->x + (v3->x - v2->x) * (y1 - v2->y) / (v3->y - v2->y);
                r2 = (COLOR16)(v2->Red   + (v3->Red   - v2->Red)   * (y1 - v2->y) / (v3->y - v2->y));
                g2 = (COLOR16)(v2->Green + (v3->Green - v2->Green) * (y1 - v2->y) / (v3->y - v2->y));
                b2 = (COLOR16)(v2->Blue  + (v3->Blue  - v2->Blue)  * (y1 - v2->y) / (v3->y - v2->y));
                dx = x2 - x1;
                if (dx != 0)
                {
                    db = (b2 - b1) / dx;
                    dg = (g2 - g1) / dx;
                    dr = (r2 - r1) / dx;
                }
                pb = pbBits + (x1 - xMin) * 4 + (y1 - yMin) * stride;
                for ( ; x1 < x2; x1++)
                {
                    *pb++ = BayerDithering(x1, y1, b1 >> 8);
                    *pb++ = BayerDithering(x1, y1, g1 >> 8);
                    *pb++ = BayerDithering(x1, y1, r1 >> 8);
                    pb++;
                    b1 += db;
                    g1 += dg;
                    r1 += dr;
                }
            }
        }
    }
    else
    {
        for (y1 = v1->y; y1 < v2->y; y1++)
        {
            if (v2->x < v3->x)
            {
                x1 = v1->x + (v2->x - v1->x) * (y1 - v1->y) / (v2->y - v1->y);
                r1 = (COLOR16)(v1->Red   + (v2->Red   - v1->Red)   * (y1 - v1->y) / (v2->y - v1->y));
                g1 = (COLOR16)(v1->Green + (v2->Green - v1->Green) * (y1 - v1->y) / (v2->y - v1->y));
                b1 = (COLOR16)(v1->Blue  + (v2->Blue  - v1->Blue)  * (y1 - v1->y) / (v2->y - v1->y));
                a1 = (COLOR16)(v1->Alpha + (v2->Alpha - v1->Alpha) * (y1 - v1->y) / (v2->y - v1->y));
                x2 = v1->x + (v3->x - v1->x) * (y1 - v1->y) / (v3->y - v1->y);
                r2 = (COLOR16)(v1->Red   + (v3->Red   - v1->Red)   * (y1 - v1->y) / (v3->y - v1->y));
                g2 = (COLOR16)(v1->Green + (v3->Green - v1->Green) * (y1 - v1->y) / (v3->y - v1->y));
                b2 = (COLOR16)(v1->Blue  + (v3->Blue  - v1->Blue)  * (y1 - v1->y) / (v3->y - v1->y));
                a2 = (COLOR16)(v1->Alpha + (v3->Alpha - v1->Alpha) * (y1 - v1->y) / (v3->y - v1->y));
                dx = x2 - x1;
                if (dx != 0)
                {
                    db = (b2 - b1) / dx;
                    dg = (g2 - g1) / dx;
                    dr = (r2 - r1) / dx;
                    da = (a2 - a1) / dx;
                }
                pb = pbBits + (x1 - xMin) * 4 + (y1 - yMin) * stride;
                for ( ; x1 < x2; x1++)
                {
                    *pb++ = b1 >> 8;
                    *pb++ = g1 >> 8;
                    *pb++ = r1 >> 8;
                    *pb++ = a1 >> 8;
                    b1 += db;
                    g1 += dg;
                    r1 += dr;
                    a1 += da;
                }
            }
            else
            {
                x1 = v1->x + (v3->x - v1->x) * (y1 - v1->y) / (v3->y - v1->y);
                r1 = (COLOR16)(v1->Red   + (v3->Red   - v1->Red)   * (y1 - v1->y) / (v3->y - v1->y));
                g1 = (COLOR16)(v1->Green + (v3->Green - v1->Green) * (y1 - v1->y) / (v3->y - v1->y));
                b1 = (COLOR16)(v1->Blue  + (v3->Blue  - v1->Blue)  * (y1 - v1->y) / (v3->y - v1->y));
                a1 = (COLOR16)(v1->Alpha + (v3->Alpha - v1->Alpha) * (y1 - v1->y) / (v3->y - v1->y));
                x2 = v1->x + (v2->x - v1->x) * (y1 - v1->y) / (v2->y - v1->y);
                r2 = (COLOR16)(v1->Red   + (v2->Red   - v1->Red)   * (y1 - v1->y) / (v2->y - v1->y));
                g2 = (COLOR16)(v1->Green + (v2->Green - v1->Green) * (y1 - v1->y) / (v2->y - v1->y));
                b2 = (COLOR16)(v1->Blue  + (v2->Blue  - v1->Blue)  * (y1 - v1->y) / (v2->y - v1->y));
                a2 = (COLOR16)(v1->Alpha + (v2->Alpha - v1->Alpha) * (y1 - v1->y) / (v2->y - v1->y));
                dx = x2 - x1;
                if (dx != 0)
                {
                    db = (b2 - b1) / dx;
                    dg = (g2 - g1) / dx;
                    dr = (r2 - r1) / dx;
                    da = (a2 - a1) / dx;
                }
                pb = pbBits + (x1 - xMin) * 4 + (y1 - yMin) * stride;
                for ( ; x1 < x2; x1++)
                {
                    *pb++ = b1 >> 8;
                    *pb++ = g1 >> 8;
                    *pb++ = r1 >> 8;
                    *pb++ = a1 >> 8;
                    b1 += db;
                    g1 += dg;
                    r1 += dr;
                    a1 += da;
                }
            }
        }
        for (y1 = v2->y; y1 < v3->y; y1++)
        {
            if (v2->x < v3->x)
            {
                x1 = v2->x + (v3->x - v2->x) * (y1 - v2->y) / (v3->y - v2->y);
                r1 = (COLOR16)(v2->Red   + (v3->Red   - v2->Red)   * (y1 - v2->y) / (v3->y - v2->y));
                g1 = (COLOR16)(v2->Green + (v3->Green - v2->Green) * (y1 - v2->y) / (v3->y - v2->y));
                b1 = (COLOR16)(v2->Blue  + (v3->Blue  - v2->Blue)  * (y1 - v2->y) / (v3->y - v2->y));
                a1 = (COLOR16)(v2->Alpha + (v3->Alpha - v2->Alpha) * (y1 - v2->y) / (v3->y - v2->y));
                x2 = v1->x + (v3->x - v1->x) * (y1 - v1->y) / (v3->y - v1->y);
                r2 = (COLOR16)(v1->Red   + (v3->Red   - v1->Red)   * (y1 - v1->y) / (v3->y - v1->y));
                g2 = (COLOR16)(v1->Green + (v3->Green - v1->Green) * (y1 - v1->y) / (v3->y - v1->y));
                b2 = (COLOR16)(v1->Blue  + (v3->Blue  - v1->Blue)  * (y1 - v1->y) / (v3->y - v1->y));
                a2 = (COLOR16)(v1->Alpha + (v3->Alpha - v1->Alpha) * (y1 - v1->y) / (v3->y - v1->y));
                dx = x2 - x1;
                if (dx != 0)
                {
                    db = (b2 - b1) / dx;
                    dg = (g2 - g1) / dx;
                    dr = (r2 - r1) / dx;
                    da = (a2 - a1) / dx;
                }
                pb = pbBits + (x1 - xMin) * 4 + (y1 - yMin) * stride;
                for ( ; x1 < x2; x1++)
                {
                    *pb++ = b1 >> 8;
                    *pb++ = g1 >> 8;
                    *pb++ = r1 >> 8;
                    *pb++ = a1 >> 8;
                    b1 += db;
                    g1 += dg;
                    r1 += dr;
                    a1 += da;
                }
            }
            else
            {
                x1 = v1->x + (v3->x - v1->x) * (y1 - v1->y) / (v3->y - v1->y);
                r1 = (COLOR16)(v1->Red   + (v3->Red   - v1->Red)   * (y1 - v1->y) / (v3->y - v1->y));
                g1 = (COLOR16)(v1->Green + (v3->Green - v1->Green) * (y1 - v1->y) / (v3->y - v1->y));
                b1 = (COLOR16)(v1->Blue  + (v3->Blue  - v1->Blue)  * (y1 - v1->y) / (v3->y - v1->y));
                a1 = (COLOR16)(v1->Alpha + (v3->Alpha - v1->Alpha) * (y1 - v1->y) / (v3->y - v1->y));
                x2 = v2->x + (v3->x - v2->x) * (y1 - v2->y) / (v3->y - v2->y);
                r2 = (COLOR16)(v2->Red   + (v3->Red   - v2->Red)   * (y1 - v2->y) / (v3->y - v2->y));
                g2 = (COLOR16)(v2->Green + (v3->Green - v2->Green) * (y1 - v2->y) / (v3->y - v2->y));
                b2 = (COLOR16)(v2->Blue  + (v3->Blue  - v2->Blue)  * (y1 - v2->y) / (v3->y - v2->y));
                a2 = (COLOR16)(v2->Alpha + (v3->Alpha - v2->Alpha) * (y1 - v2->y) / (v3->y - v2->y));
                dx = x2 - x1;
                if (dx != 0)
                {
                    db = (b2 - b1) / dx;
                    dg = (g2 - g1) / dx;
                    dr = (r2 - r1) / dx;
                    da = (a2 - a1) / dx;
                }
                pb = pbBits + (x1 - xMin) * 4 + (y1 - yMin) * stride;
                for ( ; x1 < x2; x1++)
                {
                    *pb++ = b1 >> 8;
                    *pb++ = g1 >> 8;
                    *pb++ = r1 >> 8;
                    *pb++ = a1 >> 8;
                    b1 += db;
                    g1 += dg;
                    r1 += dr;
                    a1 += da;
                }
            }
        }
    }
}

static BOOL WINAPI
GFillRect(HDC hDC, TRIVERTEX *pTriVertex, ULONG dwNumVertex,
          VOID *pMesh, ULONG dwNumMesh, BOOL bVertical)
{
    XYMINMAX xyminmax;
    LONG xMin, yMin, xMax, yMax, cx, cy;
    HDC hMemDC;
    HBITMAP hbmMem;
    HGDIOBJ hbmMemOld;
    LPBYTE pbBits;
    BITMAPINFO bmi = { { sizeof(BITMAPINFOHEADER) } };
    BOOL bDither;
    ULONG i;
    GRADIENT_RECT *rect;

    GetXYMinMax(&xyminmax, pTriVertex, dwNumVertex);
    xMin = xyminmax.xMin;
    yMin = xyminmax.yMin;
    xMax = xyminmax.xMax;
    yMax = xyminmax.yMax;

    hMemDC = CreateCompatibleDC(hDC);
    if (hMemDC == NULL)
        return FALSE;

    cx = xMax - xMin;
    cy = yMax - yMin;
    bmi.bmiHeader.biWidth = cx;
    bmi.bmiHeader.biHeight = -cy;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    hbmMem = CreateDIBSection(hMemDC, &bmi, DIB_RGB_COLORS, (VOID **)&pbBits, NULL, 0);
    if (hbmMem == NULL)
    {
        DeleteDC(hMemDC);
        return FALSE;
    }

    if (GetDeviceCaps(hDC, BITSPIXEL) < 24)
    {
        bDither = TRUE;
    }
    else
    {
        BITMAP bm;
        HBITMAP hbm = (HBITMAP)GetCurrentObject(hDC, OBJ_BITMAP);
        bDither = (hbm && GetObject(hbm, sizeof(BITMAP), &bm) && bm.bmBitsPixel < 24);
    }

    hbmMemOld = SelectObject(hMemDC, hbmMem);
    BitBlt(hMemDC, 0, 0, cx, cy, hDC, xMin, yMin, SRCCOPY);
    SelectObject(hMemDC, hbmMemOld);

    hbmMemOld = SelectObject(hMemDC, hbmMem);
    if (bVertical)
    {
        for (i = 0; i < dwNumMesh; ++i)
        {
            rect = (GRADIENT_RECT *)pMesh + i;
            MeshFillRectV(pbBits, cx, cy, pTriVertex, rect, xMin, yMin, bDither);
        }
    }
    else
    {
        for (i = 0; i < dwNumMesh; ++i)
        {
            rect = (GRADIENT_RECT *)pMesh + i;
            MeshFillRectH(pbBits, cx, cy, pTriVertex, rect, xMin, yMin, bDither);
        }
    }
    SetDIBitsToDevice(hDC, xMin, yMin, cx, cy, 0, 0, 0, cy, pbBits, &bmi, DIB_RGB_COLORS);
    SelectObject(hMemDC, hbmMemOld);

    DeleteObject(hbmMem);
    DeleteDC(hMemDC);
    return TRUE;
}

static BOOL WINAPI
GFillTriangle(HDC hDC, TRIVERTEX *pTriVertex, ULONG dwNumVertex,
              VOID *pMesh, ULONG dwNumMesh)
{
    XYMINMAX xyminmax;
    LONG xMin, yMin, xMax, yMax, cx, cy;
    BITMAPINFO bmi = { { sizeof(BITMAPINFOHEADER) } };
    BOOL bDither;
    HDC hMemDC;
    HBITMAP hbmMem;
    HGDIOBJ hbmMemOld;
    LPBYTE pbBits;
    ULONG i;
    GRADIENT_TRIANGLE *triangle;
	const TRIVERTEX *v1;
	const TRIVERTEX *v2;
	const TRIVERTEX *v3;

    GetXYMinMax(&xyminmax, pTriVertex, dwNumVertex);
    xMin = xyminmax.xMin;
    yMin = xyminmax.yMin;
    xMax = xyminmax.xMax;
    yMax = xyminmax.yMax;

    hMemDC = CreateCompatibleDC(hDC);
    if (hMemDC == NULL)
        return FALSE;

    cx = xMax - xMin;
    cy = yMax - yMin;
    bmi.bmiHeader.biWidth = cx;
    bmi.bmiHeader.biHeight = -cy;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    hbmMem = CreateDIBSection(hMemDC, &bmi, DIB_RGB_COLORS, (VOID **)&pbBits, NULL, 0);
    if (hbmMem == NULL)
    {
        DeleteDC(hMemDC);
        return FALSE;
    }

    if (GetDeviceCaps(hDC, BITSPIXEL) < 24)
    {
        bDither = TRUE;
    }
    else
    {
        BITMAP bm;
        HBITMAP hbm = (HBITMAP)GetCurrentObject(hDC, OBJ_BITMAP);
        bDither = (hbm && GetObject(hbm, sizeof(BITMAP), &bm) && bm.bmBitsPixel < 24);
    }

    hbmMemOld = SelectObject(hMemDC, hbmMem);
    BitBlt(hMemDC, 0, 0, cx, cy, hDC, xMin, yMin, SRCCOPY);
    SelectObject(hMemDC, hbmMemOld);

    hbmMemOld = SelectObject(hMemDC, hbmMem);
    for (i = 0; i < dwNumMesh; ++i)
    {
        triangle = (GRADIENT_TRIANGLE *)pMesh + i;
        v1 = pTriVertex + triangle->Vertex1;
        v2 = pTriVertex + triangle->Vertex2;
        v3 = pTriVertex + triangle->Vertex3;
        if (v1->y > v2->y)
        {
            const TRIVERTEX *tmp = v1;
            v1 = v2;
            v2 = tmp;
        }
        if (v2->y > v3->y)
        {
            const TRIVERTEX *tmp = v2;
            v2 = v3;
            v3 = tmp;
            if (v1->y > v2->y)
            {
                tmp = v1;
                v1 = v2;
                v2 = tmp;
            }
        }
        assert(v1->y <= v2->y && v2->y <= v3->y);
        MeshFillTriangle(pbBits, cx, cy, v1, v2, v3, triangle, xMin, yMin, bDither);
    }
    SetDIBitsToDevice(hDC, xMin, yMin, cx, cy, 0, 0, 0, cy, pbBits, &bmi, DIB_RGB_COLORS);
    SelectObject(hMemDC, hbmMemOld);

    DeleteObject(hbmMem);
    DeleteDC(hMemDC);
    return TRUE;
}

#ifdef __cplusplus
extern "C"
#endif
BOOL WINAPI
WonGradientFill(HDC hDC, TRIVERTEX *pTriVertex, ULONG dwNumVertex,
                VOID *pMesh, ULONG dwNumMesh, ULONG dwMode)
{
    if (dwNumVertex == 0 || dwNumMesh == 0)
        return FALSE;

    switch (dwMode)
    {
    case GRADIENT_FILL_RECT_H:
        return GFillRect(hDC, pTriVertex, dwNumVertex, pMesh, dwNumMesh, FALSE);
    case GRADIENT_FILL_RECT_V:
        return GFillRect(hDC, pTriVertex, dwNumVertex, pMesh, dwNumMesh, TRUE);
    case GRADIENT_FILL_TRIANGLE:
        return GFillTriangle(hDC, pTriVertex, dwNumVertex, pMesh, dwNumMesh);
    default:
        return FALSE;
    }
}
