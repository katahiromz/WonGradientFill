/* WonGradientFill.c --- GradientFill API clone by katahiromz */
/* Copyright (C) 2017 Katayama Hirofumi MZ. License: CC0 */
#include <windows.h>
#include <assert.h>

/*
 * Operation of WonGradientFill function will be done by linear interpolation,
 * using fixed point numbers.
 */

/* Some C compilers don't support inline keyword. Use __inline instread if so. */
#ifndef INLINE
    #ifdef __cplusplus
        #define INLINE inline
    #else
        #define INLINE __inline
    #endif
#endif

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
    const BYTE value2 = s_bayer_64[(y & 7) * 8 + 7 ^ (x & 7)];
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
    return ((s_bayer_64[(y & 7) * 8 + (x & 7)] <= b) ? 255 : 0);
}

typedef struct XYMINMAX
{
    LONG xMin, yMin, xMax, yMax;
} XYMINMAX;

/* Calculate the extent of the verteces */
static INLINE void
GetXYMinMax(XYMINMAX *pXYMinMax, TRIVERTEX *pTriVertex, ULONG dwNumVertex)
{
    ULONG i;
    LONG x, y, xMin, yMin, xMax, yMax;

    /* first vertex */
    xMin = xMax = pTriVertex[0].x;
    yMin = yMax = pTriVertex[0].y;

    /* get min/max */
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

    /* store it */
    pXYMinMax->xMin = xMin;
    pXYMinMax->yMin = yMin;
    pXYMinMax->xMax = xMax;
    pXYMinMax->yMax = yMax;
}

#define ADD_DELTAS() \
    r1 += dr; \
    g1 += dg; \
    b1 += db;
#define ADD_DELTAS_ALPHA() \
    r1 += dr; \
    g1 += dg; \
    b1 += db; \
    a1 += da;
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

static void WINAPI
MeshFillRectH(LPBYTE pbBits, ULONG cx, ULONG cy, TRIVERTEX *pTriVertex,
              GRADIENT_RECT *rect, INT xMin, INT yMin, BOOL bDither, BOOL bLow)
{
    COLOR16 r1, g1, b1, a1;
    COLOR16 dr, dg, db, da;
    LONG dx, dy, x1, y1;
    ULONG stride;
    LPBYTE pb;
    TRIVERTEX *v1, *v2, *tmp;

    /* sort v1, v2 by x */
    v1 = pTriVertex + rect->UpperLeft;
    v2 = pTriVertex + rect->LowerRight;
    if (v1->x > v2->x)
    {
        tmp = v2;
        v2 = v1;
        v1 = tmp;
    }
    assert(v1->x <= v2->x);

    r1 = v1->Red;
    g1 = v1->Green;
    b1 = v1->Blue;
    a1 = v1->Alpha;

    /* calculate delta's */
    dx = v2->x - v1->x;
    dy = v2->y - v1->y;
    if (dx != 0)
    {
        dr = (v2->Red - v1->Red) / dx;
        dg = (v2->Green - v1->Green) / dx;
        db = (v2->Blue - v1->Blue) / dx;
        da = (v2->Alpha - v1->Alpha) / dx;
    }

    /* calculate the first position */
    if (dy < 0)
    {
        pb = &GET_BYTE(v1->x - xMin, v2->y - yMin);
        dy = -dy;
    }
    else
    {
        pb = &GET_BYTE(v1->x - xMin, v1->y - yMin);
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
              GRADIENT_RECT *rect, INT xMin, INT yMin, BOOL bDither, BOOL bLow)
{
    COLOR16 r1, g1, b1, a1;
    COLOR16 dr, dg, db, da;
    LONG dx, dy, x1, y1;
    ULONG stride;
    LPBYTE pb;
    TRIVERTEX *v1, *v2, *tmp;

    /* sort v1, v2 by y */
    v1 = pTriVertex + rect->UpperLeft;
    v2 = pTriVertex + rect->LowerRight;
    if (v1->y > v2->y)
    {
        tmp = v2;
        v2 = v1;
        v1 = tmp;
    }
    assert(v1->y <= v2->y);

    r1 = v1->Red;
    g1 = v1->Green;
    b1 = v1->Blue;
    a1 = v1->Alpha;

    /* calculate delta's */
    dx = v2->x - v1->x;
    dy = v2->y - v1->y;
    if (dy != 0)
    {
        dr = (v2->Red - v1->Red) / dy;
        dg = (v2->Green - v1->Green) / dy;
        db = (v2->Blue - v1->Blue) / dy;
        da = (v2->Alpha - v1->Alpha) / dy;
    }

    /* calculate the first position */
    if (dx < 0)
    {
        pb = &GET_BYTE(v2->x - xMin, v1->y - yMin);
        dx = -dx;
    }
    else
    {
        pb = &GET_BYTE(v1->x - xMin, v1->y - yMin);
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
#define DO_RENDER(DO_P,ADD_D,CALC_E,CALC_D) \
    for (y1 = v1->y; y1 < v2->y; y1++) \
    { \
        if (v2->x < v3->x) \
        { \
            CALC_E(v1, v2, v1, v3); \
            CALC_D(); \
            pb = &GET_BYTE(x1 - xMin, y1 - yMin); \
            DO_LINE(DO_P, ADD_D); \
        } \
        else \
        { \
            CALC_E(v1, v3, v1, v2); \
            CALC_D(); \
            pb = &GET_BYTE(x1 - xMin, y1 - yMin); \
            DO_LINE(DO_P, ADD_D); \
        } \
    } \
    for (y1 = v2->y; y1 < v3->y; y1++) \
    { \
        if (v2->x < v3->x) \
        { \
            CALC_E(v2, v3, v1, v3); \
            CALC_D(); \
            pb = &GET_BYTE(x1 - xMin, y1 - yMin); \
            DO_LINE(DO_P, ADD_D); \
        } \
        else \
        { \
            CALC_E(v1, v3, v2, v3); \
            CALC_D(); \
            pb = &GET_BYTE(x1 - xMin, y1 - yMin); \
            DO_LINE(DO_P, ADD_D); \
        } \
    }

static void WINAPI
MeshFillTriangle(LPBYTE pbBits, ULONG cx, ULONG cy,
                 TRIVERTEX *v1, TRIVERTEX *v2, TRIVERTEX *v3,
                 GRADIENT_TRIANGLE *triangle,
                 INT xMin, INT yMin, BOOL bDither, BOOL bLow)
{
    COLOR16 r1, g1, b1, a1, r2, g2, b2, a2;
    COLOR16 dr, dg, db, da;
    LONG dx, x1, y1, x2;
    LPBYTE pb;
    ULONG stride = cx * 4;    /* one row width */

    assert(v1->y <= v2->y && v2->y <= v3->y);

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
    BOOL bDither, bLow;
    ULONG i;
    GRADIENT_RECT *rect = (GRADIENT_RECT *)pMesh;

    /* get the extent */
    GetXYMinMax(&xyminmax, pTriVertex, dwNumVertex);
    xMin = xyminmax.xMin;
    yMin = xyminmax.yMin;
    xMax = xyminmax.xMax;
    yMax = xyminmax.yMax;

    /* create a memory DC */
    hMemDC = CreateCompatibleDC(NULL);
    if (hMemDC == NULL)
        return FALSE;

    /* create a 32-bpp DIB section */
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

    /* will we do dithering? */
    {
        BITMAP bm;
        HBITMAP hbm = (HBITMAP)GetCurrentObject(hDC, OBJ_BITMAP);
        if (hbm && GetObject(hbm, sizeof(BITMAP), &bm))
        {
            bDither = bm.bmBitsPixel < 24 || GetDeviceCaps(hDC, BITSPIXEL) < 24;
            bLow = bm.bmBitsPixel <= 4 || GetDeviceCaps(hDC, BITSPIXEL) <= 4;
        }
        else
        {
            bDither = GetDeviceCaps(hDC, BITSPIXEL) < 24;
            bLow = GetDeviceCaps(hDC, BITSPIXEL) <= 4;
        }
    }

    /* transfer to hMemDC */
    hbmMemOld = SelectObject(hMemDC, hbmMem);
    BitBlt(hMemDC, 0, 0, cx, cy, hDC, xMin, yMin, SRCCOPY);
    SelectObject(hMemDC, hbmMemOld);

    /* select hbmMem */
    hbmMemOld = SelectObject(hMemDC, hbmMem);

    /* do main process */
    if (bVertical)
    {
        /* vertical */
        for (i = 0; i < dwNumMesh; ++i, ++rect)
        {
            MeshFillRectV(pbBits, cx, cy, pTriVertex, rect, xMin, yMin, bDither, bLow);
        }
    }
    else
    {
        /* horizontal */
        for (i = 0; i < dwNumMesh; ++i, ++rect)
        {
            MeshFillRectH(pbBits, cx, cy, pTriVertex, rect, xMin, yMin, bDither, bLow);
        }
    }

    /* transfer to hDC */
    SetDIBitsToDevice(hDC, xMin, yMin, cx, cy, 0, 0, 0, cy, pbBits, &bmi, DIB_RGB_COLORS);

    /* deselect hbmMem */
    SelectObject(hMemDC, hbmMemOld);

    /* clean up */
    DeleteObject(hbmMem);
    DeleteDC(hMemDC);

    return TRUE;    /* success */
}

static BOOL WINAPI
GFillTriangle(HDC hDC, TRIVERTEX *pTriVertex, ULONG dwNumVertex,
              VOID *pMesh, ULONG dwNumMesh)
{
    XYMINMAX xyminmax;
    LONG xMin, yMin, xMax, yMax, cx, cy;
    BITMAPINFO bmi = { { sizeof(BITMAPINFOHEADER) } };
    BOOL bDither, bLow;
    HDC hMemDC;
    HBITMAP hbmMem;
    HGDIOBJ hbmMemOld;
    LPBYTE pbBits;
    ULONG i;
    GRADIENT_TRIANGLE *triangle = (GRADIENT_TRIANGLE *)pMesh;
    TRIVERTEX *v1, *v2, *v3;

    /* get the extent */
    GetXYMinMax(&xyminmax, pTriVertex, dwNumVertex);
    xMin = xyminmax.xMin;
    yMin = xyminmax.yMin;
    xMax = xyminmax.xMax;
    yMax = xyminmax.yMax;

    /* create a memory DC */
    hMemDC = CreateCompatibleDC(NULL);
    if (hMemDC == NULL)
        return FALSE;

    /* create a 32-bpp DIB section */
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

    /* will we do dithering? */
    {
        BITMAP bm;
        HBITMAP hbm = (HBITMAP)GetCurrentObject(hDC, OBJ_BITMAP);
        if (hbm && GetObject(hbm, sizeof(BITMAP), &bm))
        {
            bDither = bm.bmBitsPixel < 24 || GetDeviceCaps(hDC, BITSPIXEL) < 24;
            bLow = bm.bmBitsPixel <= 4 || GetDeviceCaps(hDC, BITSPIXEL) <= 4;
        }
        else
        {
            bDither = GetDeviceCaps(hDC, BITSPIXEL) < 24;
            bLow = GetDeviceCaps(hDC, BITSPIXEL) <= 4;
        }
    }

    /* transfer to hMemDC */
    hbmMemOld = SelectObject(hMemDC, hbmMem);
    BitBlt(hMemDC, 0, 0, cx, cy, hDC, xMin, yMin, SRCCOPY);
    SelectObject(hMemDC, hbmMemOld);

    /* select hbmMem */
    hbmMemOld = SelectObject(hMemDC, hbmMem);

    /* do main process */
    for (i = 0; i < dwNumMesh; ++i, ++triangle)
    {
        v1 = pTriVertex + triangle->Vertex1;
        v2 = pTriVertex + triangle->Vertex2;
        v3 = pTriVertex + triangle->Vertex3;

        /* sort v1, v2, v3 */
        if (v1->y > v2->y)
        {
            TRIVERTEX *tmp = v1;
            v1 = v2;
            v2 = tmp;
        }
        if (v2->y > v3->y)
        {
            TRIVERTEX *tmp = v2;
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

        MeshFillTriangle(pbBits, cx, cy, v1, v2, v3, triangle, xMin, yMin, bDither, bLow);
    }

    /* transfer to hDC */
    SetDIBitsToDevice(hDC, xMin, yMin, cx, cy, 0, 0, 0, cy, pbBits, &bmi, DIB_RGB_COLORS);

    /* deselect hbmMem */
    SelectObject(hMemDC, hbmMemOld);

    /* clean up */
    DeleteObject(hbmMem);
    DeleteDC(hMemDC);

    return TRUE;    /* success */
}

#ifdef __cplusplus
extern "C"
#endif
BOOL WINAPI
WonGradientFill(HDC hDC, TRIVERTEX *pTriVertex, ULONG dwNumVertex,
                VOID *pMesh, ULONG dwNumMesh, ULONG dwMode)
{
    if (dwNumVertex == 0 || dwNumMesh == 0)
        return FALSE;   /* nothing to do */

    switch (dwMode)
    {
    case GRADIENT_FILL_RECT_H:
        return GFillRect(hDC, pTriVertex, dwNumVertex, pMesh, dwNumMesh, FALSE);
    case GRADIENT_FILL_RECT_V:
        return GFillRect(hDC, pTriVertex, dwNumVertex, pMesh, dwNumMesh, TRUE);
    case GRADIENT_FILL_TRIANGLE:
        return GFillTriangle(hDC, pTriVertex, dwNumVertex, pMesh, dwNumMesh);
    default:
        return FALSE;   /* invalid parameter */
    }
}
