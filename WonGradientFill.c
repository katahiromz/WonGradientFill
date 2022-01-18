/* WonGradientFill.c --- Wonders API GradientFill */
/* Copyright (C) 2017 Katayama Hirofumi MZ. License: CC0 */
/**************************************************************************/

#include "WonGradientFill.h"

#ifdef __cplusplus
    #include <cassert>
#else
    #include <assert.h>
#endif

/*
 * Operation of WonGradientFill function will be done by linear interpolation,
 * using fixed point numbers.
 */

/* Some C compilers don't support inline keyword. Use __inline instread if so. */
#ifndef INLINE
    #ifdef __cplusplus
        #define INLINE inline
    #elif (__STDC_VERSION__ >= 199901L)
        #define INLINE inline
    #else
        #define INLINE __inline
    #endif
#endif

#define BV(x) (x * 255 / 64 + 4)
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
#undef BV

/* Bayer Dithering */
/* https://en.wikipedia.org/wiki/Ordered_dithering */
static INLINE BYTE BayerDithering(ULONG x, ULONG y, BYTE b)
{
    return ((s_bayer_64[(y & 7) * 8 + (x & 7)] <= b) ? 255 : 0);
}

static INLINE BYTE BayerDitheringHigh(ULONG x, ULONG y, BYTE b)
{
    const BYTE value1 = s_bayer_64[(y & 7) * 8 + (x & 7)];
    const BYTE value2 = s_bayer_64[((y + 3) & 7) * 8 + ((x + 2) & 7)];
    if (value1 <= b)
    {
        if (value2 <= b)
            return 255;
        return 127;
    }
    else
    {
        if (value2 <= b)
            return 127;
        return 0;
    }
}

#define SWAP(v1,v2,tmp) \
    tmp = v1; \
    v1 = v2; \
    v2 = tmp;

/* Calculate the extent of the verteces */
#define GET_XY_MINMAX(i,x,y,pTriVertex,dwNumVertex) do { \
    xMin = xMax = pTriVertex[0].x; \
    yMin = yMax = pTriVertex[0].y; \
    for (i = 1; i < dwNumVertex; ++i) \
    { \
        x = pTriVertex[i].x; \
        if (x < xMin) \
            xMin = x; \
        if (x > xMax) \
            xMax = x; \
        y = pTriVertex[i].y; \
        if (y < yMin) \
            yMin = y; \
        if (y > yMax) \
            yMax = y; \
    } \
} while (0)

#define ADD_DELTAS() \
    r1 += (COLOR16)dr; \
    g1 += (COLOR16)dg; \
    b1 += (COLOR16)db;
#define ADD_DELTAS_ALPHA() \
    r1 += (COLOR16)dr; \
    g1 += (COLOR16)dg; \
    b1 += (COLOR16)db; \
    a1 += (COLOR16)da;
#define CALC_DELTAS() \
    dx = x2 - x1; \
    if (dx != 0) \
    { \
        db = (b2 - b1) / dx; \
        dg = (g2 - g1) / dx; \
        dr = (r2 - r1) / dx; \
    }
#define CALC_DELTAS_ALPHA() \
    dx = x2 - x1; \
    if (dx != 0) \
    { \
        db = (b2 - b1) / dx; \
        dg = (g2 - g1) / dx; \
        dr = (r2 - r1) / dx; \
        da = (a2 - a1) / dx; \
    }
#define GET_BYTE(x,y) pbBits[((y) * cx + (x)) * 4]

#define DO_PIXEL_LOW() \
    *pb++ = BayerDithering(x1, y1, b1 >> 8); \
    *pb++ = BayerDithering(x1, y1, g1 >> 8); \
    *pb++ = BayerDithering(x1, y1, r1 >> 8); \
    pb++;   /* skip alpha value */
#define DO_PIXEL_HIGH() \
    *pb++ = BayerDitheringHigh(x1, y1, b1 >> 8); \
    *pb++ = BayerDitheringHigh(x1, y1, g1 >> 8); \
    *pb++ = BayerDitheringHigh(x1, y1, r1 >> 8); \
    pb++;   /* skip alpha value */
#define DO_PIXEL_ALPHA() \
    *pb++ = b1 >> 8; \
    *pb++ = g1 >> 8; \
    *pb++ = r1 >> 8; \
    *pb++ = a1 >> 8;

#define DO_RECT_H(DO_P,ADD_D) \
    for (x1 = 0; x1 < dx; x1++) \
    { \
        for (y1 = 0; y1 < dy; y1++) \
        { \
            DO_P(); \
            pb += stride - 4; \
        } \
        ADD_D(); \
        pb -= dy * stride - 4; \
    }
#define DO_RECT_V(DO_P,ADD_D) \
    for (y1 = 0; y1 < dy; y1++) \
    { \
        for (x1 = 0; x1 < dx; x1++) \
        { \
            DO_P(); \
        } \
        ADD_D(); \
        pb -= dx * 4; \
        pb += stride; \
    }
#define INIT_DELTA(delta) \
    dx = v2->x - v1->x; \
    dy = v2->y - v1->y; \
    if (delta != 0) \
    { \
        dr = (v2->Red - v1->Red) / delta; \
        dg = (v2->Green - v1->Green) / delta; \
        db = (v2->Blue - v1->Blue) / delta; \
        da = (v2->Alpha - v1->Alpha) / delta; \
    }

static void WINAPI
MeshFillRectH(LPBYTE pbBits, ULONG cx, ULONG cy, TRIVERTEX *pTriVertex,
              GRADIENT_RECT *rect, BOOL bDither, BOOL bLow)
{
    COLOR16 r1, g1, b1, a1;
    LONG dr, dg, db, da;
    LONG dx, dy, x1, y1, x0, y0;
    ULONG stride;
    LPBYTE pb;
    TRIVERTEX *v1, *v2, *tmp;

    /* sort v1, v2 by x */
    v1 = &pTriVertex[rect->UpperLeft];
    v2 = &pTriVertex[rect->LowerRight];
    if (v1->x > v2->x)
    {
        SWAP(v1, v2, tmp);
    }
    assert(v1->x <= v2->x);
    x0 = v1->x;
    y0 = ((v1->y < v2->y) ? v1->y : v2->y);

    r1 = v1->Red;
    g1 = v1->Green;
    b1 = v1->Blue;
    a1 = v1->Alpha;

    /* calculate delta's */
    INIT_DELTA(dx);

    /* calculate the first position */
    if (dy < 0)
    {
        pb = &GET_BYTE(v1->x - x0, v2->y - y0);
        dy = -dy;
    }
    else
    {
        pb = &GET_BYTE(v1->x - x0, v1->y - y0);
    }

    stride = cx * 4;    /* one row width */

    if (bDither)
    {
        /* fill rectangle horizontally (dithering) */
        if (bLow)
        {
            DO_RECT_H(DO_PIXEL_LOW, ADD_DELTAS);
        }
        else
        {
            DO_RECT_H(DO_PIXEL_HIGH, ADD_DELTAS);
        }
    }
    else
    {
        /* fill rectangle horizontally (w/o dithering) */
        DO_RECT_H(DO_PIXEL_ALPHA, ADD_DELTAS_ALPHA);
    }
}

static void WINAPI
MeshFillRectV(LPBYTE pbBits, ULONG cx, ULONG cy, TRIVERTEX *pTriVertex,
              GRADIENT_RECT *rect, BOOL bDither, BOOL bLow)
{
    COLOR16 r1, g1, b1, a1;
    LONG dr, dg, db, da;
    LONG dx, dy, x1, y1, x0, y0;
    ULONG stride;
    LPBYTE pb;
    TRIVERTEX *v1, *v2, *tmp;

    /* sort v1, v2 by y */
    v1 = &pTriVertex[rect->UpperLeft];
    v2 = &pTriVertex[rect->LowerRight];
    if (v1->y > v2->y)
    {
        SWAP(v1, v2, tmp);
    }
    assert(v1->y <= v2->y);
    y0 = v1->y;
    x0 = ((v1->x < v2->x) ? v1->x : v2->x);

    r1 = v1->Red;
    g1 = v1->Green;
    b1 = v1->Blue;
    a1 = v1->Alpha;

    /* calculate delta's */
    INIT_DELTA(dy);

    /* calculate the first position */
    if (dx < 0)
    {
        pb = &GET_BYTE(v2->x - x0, v1->y - y0);
        dx = -dx;
    }
    else
    {
        pb = &GET_BYTE(v1->x - x0, v1->y - y0);
    }

    stride = cx * 4;    /* one row width */

    if (bDither)
    {
        /* fill rectangle vertically (dithering) */
        if (bLow)
        {
            DO_RECT_V(DO_PIXEL_LOW, ADD_DELTAS);
        }
        else
        {
            DO_RECT_V(DO_PIXEL_HIGH, ADD_DELTAS);
        }
    }
    else
    {
        /* fill rectangle vertically (w/o dithering) */
        DO_RECT_V(DO_PIXEL_ALPHA, ADD_DELTAS_ALPHA);
    }
}

#define CALC_EDGE(v1,v2,w1,w2) \
    x1 = v1->x + (v2->x - v1->x) * (y1 - v1->y) / (v2->y - v1->y); \
    r1 = (COLOR16)(v1->Red   + (v2->Red   - v1->Red)   * (y1 - v1->y) / (v2->y - v1->y)); \
    g1 = (COLOR16)(v1->Green + (v2->Green - v1->Green) * (y1 - v1->y) / (v2->y - v1->y)); \
    b1 = (COLOR16)(v1->Blue  + (v2->Blue  - v1->Blue)  * (y1 - v1->y) / (v2->y - v1->y)); \
    x2 = w1->x + (w2->x - w1->x) * (y1 - w1->y) / (w2->y - w1->y); \
    r2 = (COLOR16)(w1->Red   + (w2->Red   - w1->Red)   * (y1 - w1->y) / (w2->y - w1->y)); \
    g2 = (COLOR16)(w1->Green + (w2->Green - w1->Green) * (y1 - w1->y) / (w2->y - w1->y)); \
    b2 = (COLOR16)(w1->Blue  + (w2->Blue  - w1->Blue)  * (y1 - w1->y) / (w2->y - w1->y));
#define CALC_EDGE_ALPHA(v1,v2,w1,w2) \
    x1 = v1->x + (v2->x - v1->x) * (y1 - v1->y) / (v2->y - v1->y); \
    r1 = (COLOR16)(v1->Red   + (v2->Red   - v1->Red)   * (y1 - v1->y) / (v2->y - v1->y)); \
    g1 = (COLOR16)(v1->Green + (v2->Green - v1->Green) * (y1 - v1->y) / (v2->y - v1->y)); \
    b1 = (COLOR16)(v1->Blue  + (v2->Blue  - v1->Blue)  * (y1 - v1->y) / (v2->y - v1->y)); \
    a1 = (COLOR16)(v1->Alpha + (v2->Alpha - v1->Alpha) * (y1 - v1->y) / (v2->y - v1->y)); \
    x2 = w1->x + (w2->x - w1->x) * (y1 - w1->y) / (w2->y - w1->y); \
    r2 = (COLOR16)(w1->Red   + (w2->Red   - w1->Red)   * (y1 - w1->y) / (w2->y - w1->y)); \
    g2 = (COLOR16)(w1->Green + (w2->Green - w1->Green) * (y1 - w1->y) / (w2->y - w1->y)); \
    b2 = (COLOR16)(w1->Blue  + (w2->Blue  - w1->Blue)  * (y1 - w1->y) / (w2->y - w1->y)); \
    a2 = (COLOR16)(w1->Alpha + (w2->Alpha - w1->Alpha) * (y1 - w1->y) / (w2->y - w1->y));
#define DO_LINE(DO_P,ADD_D) \
    for ( ; x1 < x2; ++x1) \
    { \
        DO_P(); \
        ADD_D(); \
    }
#define DO_LINE2(a1,a2,a3,a4,DO_P,ADD_D,CALC_E,CALC_D) \
    CALC_E(a1, a2, a3, a4); \
    CALC_D(); \
    pb = &GET_BYTE(x1 - x0, y1 - y0); \
    DO_LINE(DO_P, ADD_D);
#define DO_RENDER(DO_P,ADD_D,CALC_E,CALC_D) \
    for (y1 = v1->y; y1 < v2->y; y1++) \
    { \
        if (v2->x < v3->x) \
        { \
            DO_LINE2(v1, v2, v1, v3, DO_P, ADD_D, CALC_E, CALC_D); \
        } \
        else \
        { \
            DO_LINE2(v1, v3, v1, v2, DO_P, ADD_D, CALC_E, CALC_D); \
        } \
    } \
    for (y1 = v2->y; y1 < v3->y; y1++) \
    { \
        if (v2->x < v3->x) \
        { \
            DO_LINE2(v2, v3, v1, v3, DO_P, ADD_D, CALC_E, CALC_D); \
        } \
        else \
        { \
            DO_LINE2(v1, v3, v2, v3, DO_P, ADD_D, CALC_E, CALC_D); \
        } \
    }

static void WINAPI
MeshFillTriangle(LPBYTE pbBits, ULONG cx, ULONG cy,
                 TRIVERTEX *v1, TRIVERTEX *v2, TRIVERTEX *v3,
                 GRADIENT_TRIANGLE *triangle,
                 BOOL bDither, BOOL bLow)
{
    COLOR16 r1, g1, b1, a1, r2, g2, b2, a2;
    LONG dr, dg, db, da;
    LONG dx, x1, y1, x2, x0, y0;
    LPBYTE pb;

    assert(v1->y <= v2->y && v2->y <= v3->y);

    y0 = v1->y;
    x0 = v1->x;
    if (x0 > v2->x)
        x0 = v2->x;
    if (x0 > v3->x)
        x0 = v3->x;

    if (bDither)
    {
        if (bLow)
        {
            DO_RENDER(DO_PIXEL_LOW, ADD_DELTAS, CALC_EDGE, CALC_DELTAS);
        }
        else
        {
            DO_RENDER(DO_PIXEL_HIGH, ADD_DELTAS, CALC_EDGE, CALC_DELTAS);
        }
    }
    else
    {
        DO_RENDER(DO_PIXEL_ALPHA, ADD_DELTAS_ALPHA, CALC_EDGE_ALPHA, CALC_DELTAS_ALPHA);
    }
}

#define CREATE_MEM_BMP() \
    bmi.bmiHeader.biWidth = cx; \
    bmi.bmiHeader.biHeight = -cy; \
    bmi.bmiHeader.biPlanes = 1; \
    bmi.bmiHeader.biBitCount = 32; \
    hbmMem = CreateDIBSection(hMemDC, &bmi, DIB_RGB_COLORS, (void **)&pbBits, NULL, 0); \
    if (hbmMem == NULL) \
    { \
        DeleteDC(hMemDC); \
        return FALSE; \
    }

#define CHECK_BITSPIXEL() \
    hbm = (HBITMAP)GetCurrentObject(hDC, OBJ_BITMAP); \
    if (hbm && GetObject(hbm, sizeof(BITMAP), &bm)) \
    { \
        bDither = bm.bmBitsPixel < 24 || GetDeviceCaps(hDC, BITSPIXEL) < 24; \
        bLow = bm.bmBitsPixel <= 4 || GetDeviceCaps(hDC, BITSPIXEL) <= 4; \
    } \
    else \
    { \
        bDither = GetDeviceCaps(hDC, BITSPIXEL) < 24; \
        bLow = GetDeviceCaps(hDC, BITSPIXEL) <= 4; \
    }

static BOOL WINAPI
IntGradFillRect(HDC hDC, TRIVERTEX *pTriVertex, ULONG dwNumVertex,
          void *pMesh, ULONG dwNumMesh, BOOL bVertical)
{
    LONG x, y, xMin, yMin, xMax, yMax, cx, cy;
    HDC hMemDC;
    HBITMAP hbmMem;
    HGDIOBJ hbm, hbmMemOld;
    LPBYTE pbBits;
    BITMAPINFO bmi = { { sizeof(BITMAPINFOHEADER) } };
    BOOL bDither, bLow;
    ULONG i;
    BITMAP bm;
    GRADIENT_RECT *rect = (GRADIENT_RECT *)pMesh;

    /* get the extent */
    GET_XY_MINMAX(i, x, y, pTriVertex, dwNumVertex);

    /* create a memory DC */
    hMemDC = CreateCompatibleDC(NULL);
    if (hMemDC == NULL)
        return FALSE;

    /* create a 32-bpp DIB section */
    cx = xMax - xMin;
    cy = yMax - yMin;
    CREATE_MEM_BMP();

    /* will we do dithering? */
    CHECK_BITSPIXEL();

    /* select hbmMem */
    hbmMemOld = SelectObject(hMemDC, hbmMem);

    /* transfer to hMemDC */
    BitBlt(hMemDC, 0, 0, cx, cy, hDC, xMin, yMin, SRCCOPY);

    /* do main process */
    if (bVertical)
    {
        /* vertical */
        for (i = 0; i < dwNumMesh; ++i, ++rect)
        {
            MeshFillRectV(pbBits, cx, cy, pTriVertex, rect, bDither, bLow);
        }
    }
    else
    {
        /* horizontal */
        for (i = 0; i < dwNumMesh; ++i, ++rect)
        {
            MeshFillRectH(pbBits, cx, cy, pTriVertex, rect, bDither, bLow);
        }
    }

    /* transfer to hDC */
    SetDIBitsToDevice(hDC, xMin, yMin, cx, cy, 0, 0, 0, cy, pbBits, &bmi, DIB_RGB_COLORS);

    /* deselect hbmMem */
    SelectObject(hMemDC, hbmMemOld);

    /* clean up */
    DeleteDC(hMemDC);
    DeleteObject(hbmMem);

    return TRUE;    /* success */
}

static BOOL WINAPI
IntGradFillTriangle(HDC hDC, TRIVERTEX *pTriVertex, ULONG dwNumVertex,
              void *pMesh, ULONG dwNumMesh)
{
    LONG x, y, xMin, yMin, xMax, yMax, cx, cy;
    BITMAPINFO bmi = { { sizeof(BITMAPINFOHEADER) } };
    BOOL bDither, bLow;
    HDC hMemDC;
    HBITMAP hbm, hbmMem;
    HGDIOBJ hbmMemOld;
    LPBYTE pbBits;
    ULONG i;
    BITMAP bm;
    GRADIENT_TRIANGLE *triangle = (GRADIENT_TRIANGLE *)pMesh;
    TRIVERTEX *v1, *v2, *v3, *tmp;

    /* get the extent */
    GET_XY_MINMAX(i, x, y, pTriVertex, dwNumVertex);

    /* create a memory DC */
    hMemDC = CreateCompatibleDC(NULL);
    if (hMemDC == NULL)
        return FALSE;

    /* create a 32-bpp DIB section */
    cx = xMax - xMin;
    cy = yMax - yMin;
    CREATE_MEM_BMP();

    /* will we do dithering? */
    CHECK_BITSPIXEL();

    /* select hbmMem */
    hbmMemOld = SelectObject(hMemDC, hbmMem);

    /* transfer to hMemDC */
    BitBlt(hMemDC, 0, 0, cx, cy, hDC, xMin, yMin, SRCCOPY);

    /* do main process */
    for (i = 0; i < dwNumMesh; ++i, ++triangle)
    {
        v1 = &pTriVertex[triangle->Vertex1];
        v2 = &pTriVertex[triangle->Vertex2];
        v3 = &pTriVertex[triangle->Vertex3];

        /* sort v1, v2, v3 by y */
        if (v1->y > v2->y)
        {
            SWAP(v1, v2, tmp);
        }
        if (v2->y > v3->y)
        {
            SWAP(v2, v3, tmp);
            if (v1->y > v2->y)
            {
                SWAP(v1, v2, tmp);
            }
        }
        assert(v1->y <= v2->y && v2->y <= v3->y);

        MeshFillTriangle(pbBits, cx, cy, v1, v2, v3, triangle, bDither, bLow);
    }

    /* transfer to hDC */
    SetDIBitsToDevice(hDC, xMin, yMin, cx, cy, 0, 0, 0, cy, pbBits, &bmi, DIB_RGB_COLORS);

    /* deselect hbmMem */
    SelectObject(hMemDC, hbmMemOld);

    /* clean up */
    DeleteDC(hMemDC);
    DeleteObject(hbmMem);

    return TRUE;    /* success */
}

#ifdef __cplusplus
extern "C"
#endif
BOOL WINAPI
WonGradientFill(HDC hDC, TRIVERTEX *pTriVertex, ULONG dwNumVertex,
                void *pMesh, ULONG dwNumMesh, ULONG dwMode)
{
    if (dwNumVertex == 0 || dwNumMesh == 0)
        return FALSE;   /* nothing to do */

    switch (dwMode)
    {
    case GRADIENT_FILL_RECT_H:
        return IntGradFillRect(hDC, pTriVertex, dwNumVertex, pMesh, dwNumMesh, FALSE);
    case GRADIENT_FILL_RECT_V:
        return IntGradFillRect(hDC, pTriVertex, dwNumVertex, pMesh, dwNumMesh, TRUE);
    case GRADIENT_FILL_TRIANGLE:
        return IntGradFillTriangle(hDC, pTriVertex, dwNumVertex, pMesh, dwNumMesh);
    default:
        return FALSE;   /* invalid parameter */
    }
}
