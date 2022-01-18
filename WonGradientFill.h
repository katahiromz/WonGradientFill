/* WonGradientFill.h --- Wonders API GradientFill */
/* Copyright (C) 2017 Katayama Hirofumi MZ. License: CC0 */
/**************************************************************************/

#ifndef WONGRADIENTFILL_H_
#define WONGRADIENTFILL_H_      5   /* Version 5 */

#ifndef _INC_WINDOWS
    #include <windows.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************/

#ifndef GRADIENT_FILL_RECT_H
    #define GRADIENT_FILL_RECT_H 0x00000000
    #define GRADIENT_FILL_RECT_V 0x00000001
    #define GRADIENT_FILL_TRIANGLE 0x00000002
    #define GRADIENT_FILL_OP_FLAG 0x000000ff

    typedef USHORT COLOR16;

    typedef struct {
        LONG x;
        LONG y;
        COLOR16 Red;
        COLOR16 Green;
        COLOR16 Blue;
        COLOR16 Alpha;
    } TRIVERTEX;

    typedef struct {
        ULONG Vertex1;
        ULONG Vertex2;
        ULONG Vertex3;
    } GRADIENT_TRIANGLE;

    typedef struct {
        ULONG UpperLeft;
        ULONG LowerRight;
    } GRADIENT_RECT;
#endif  /* ndef GRADIENT_FILL_RECT_H */

/**************************************************************************/

BOOL WINAPI
WonGradientFill(HDC hDC, TRIVERTEX *pTriVertex, ULONG dwNumVertex,
                void *pMesh, ULONG dwNumMesh, ULONG dwMode);

/**************************************************************************/

#ifdef __cplusplus
} // extern "C"
#endif

/**************************************************************************/

#if defined(_WONVER) && (_WONVER < 0x0410)
    #define GradientFill WonGradientFill
    #define GdiGradientFill WonGradientFill
#endif

/**************************************************************************/

#endif  /* ndef WONGRADIENTFILL_H_ */
