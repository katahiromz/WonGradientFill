/* WonGradientFill.h --- Wonders GradientFill */
/**************************************************************************/

#ifndef WONGRADIENTFILL_H_
#define WONGRADIENTFILL_H_      1   /* Version 1 */

#ifndef _INC_WINDOWS
    #include <windows.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************/

BOOL WINAPI
WonGradientFill(HDC hDC, TRIVERTEX *pTriVertex, ULONG dwNumVertex,
                void *pMesh, ULONG dwNumMesh, ULONG dwMode);

/**************************************************************************/

#ifdef __cplusplus
} // extern "C"
#endif

#endif  /* ndef WONGRADIENTFILL_H_ */
