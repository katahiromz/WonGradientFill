// MyNotepad.cpp --- A Win32 application                -*- C++ -*-
//////////////////////////////////////////////////////////////////////////////

#include "MWindowBase.hpp"
#include <cmath>

#define INTERVAL 100

#ifndef M_PI
    static const double M_PI = 3.14159265358979323846;
#endif

static const double deg2rad = M_PI / 180;

//////////////////////////////////////////////////////////////////////////////

typedef struct BITMAPINFODX
{
    BITMAPINFOHEADER bmiHeader;
    RGBQUAD bmiColors[256];
} BITMAPINFODX;

extern "C"
BOOL WINAPI
WonGradientFill(HDC hDC, TRIVERTEX *pTriVertex, ULONG dwNumVertex,
                VOID *pMesh, ULONG dwNumMesh, ULONG dwMode);

//////////////////////////////////////////////////////////////////////////////

// the Win32 application
struct MGradientFillTest : public MWindowBase
{
    HINSTANCE   m_hInst;        // the instance handle
    INT         m_nMode;
    HBITMAP     m_hbm;
    SIZE        m_siz;
    BOOL        m_bUseMSIMG32;

    // constructors
    MGradientFillTest(HINSTANCE hInst)
        : m_hInst(hInst), m_nMode(0), m_hbm(NULL), m_bUseMSIMG32(FALSE)
    {
    }

    virtual void ModifyWndClassDx(WNDCLASSEX& wcx)
    {
        wcx.hIconSm = wcx.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    }

    BOOL OnEraseBkgnd(HWND hwnd, HDC hdc)
    {
        return TRUE;
    }

    void ReCreateBitmap(HWND hwnd)
    {
        if (m_hbm)
        {
            DeleteObject(m_hbm);
            m_hbm = NULL;
        }

        BITMAPINFODX bmi = {{ sizeof(BITMAPINFOHEADER) }};
        bmi.bmiHeader.biWidth = m_siz.cx;
        bmi.bmiHeader.biHeight = m_siz.cy;
        bmi.bmiHeader.biPlanes = 1;

        m_nMode = (m_nMode + 1) % 3;
        switch (m_nMode)
        {
        case 0:
            bmi.bmiHeader.biBitCount = 24;
            break;
        case 1:
            bmi.bmiHeader.biBitCount = 8;
            {
                HDC hDC = CreateCompatibleDC(NULL);
                PALETTEENTRY entries[256];
                GetSystemPaletteEntries(hDC, 0, 256, entries);
                for (size_t i = 0; i < 256; ++i)
                {
                    bmi.bmiColors[i].rgbBlue = entries[i].peBlue;
                    bmi.bmiColors[i].rgbGreen = entries[i].peGreen;
                    bmi.bmiColors[i].rgbRed = entries[i].peRed;
                    bmi.bmiColors[i].rgbReserved = 0;
                }
            }
            break;
        case 2:
            bmi.bmiHeader.biBitCount = 1;
            bmi.bmiColors[0].rgbBlue = 255;
            bmi.bmiColors[0].rgbGreen = 255;
            bmi.bmiColors[0].rgbRed = 255;
            bmi.bmiColors[0].rgbReserved = 0;
            bmi.bmiColors[1].rgbBlue = 0;
            bmi.bmiColors[1].rgbGreen = 0;
            bmi.bmiColors[1].rgbRed = 0;
            bmi.bmiColors[1].rgbReserved = 0;
            break;
        }

        LPVOID pvBits;
        m_hbm = CreateDIBSection(NULL, (BITMAPINFO *)&bmi, DIB_RGB_COLORS,
                                 &pvBits, NULL, 0);
    }

    void OnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
    {
        ReCreateBitmap(hwnd);
    }

    void OnRButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
    {
        m_bUseMSIMG32 = !m_bUseMSIMG32;
    }

    BOOL OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
    {
        RECT rc;
        ::GetClientRect(hwnd, &rc);
        m_siz = SizeFromRectDx(&rc);
        ReCreateBitmap(hwnd);
        SetTimer(hwnd, 999, INTERVAL, NULL);
        return TRUE;
    }

    void OnPaint(HWND hwnd)
    {
        static INT s_i = 0;
        HDC hMemDC = CreateCompatibleDC(NULL);
        HGDIOBJ hbmOld = SelectObject(hMemDC, m_hbm);

        INT cx = m_siz.cx / 2, cy = m_siz.cy / 2;
        INT c;
        if (m_siz.cx < m_siz.cy)
            c = cx;
        else
            c = cy;

        {
            GRADIENT_RECT rect = { 0, 1 };
            TRIVERTEX tri[] =
            {
                { 0, 0, 0xFF00, 0, 0 },
                { m_siz.cx, m_siz.cy, 0, 0xFF00, 0 },
            };
            if (m_bUseMSIMG32)
            {
                GradientFill(hMemDC, tri, 2, &rect, 1, GRADIENT_FILL_RECT_H);
            }
            else
            {
                WonGradientFill(hMemDC, tri, 2, &rect, 1, GRADIENT_FILL_RECT_H);
            }
        }
        {
            GRADIENT_TRIANGLE triangle =
            {
                0, 1, 2
            };
            TRIVERTEX tri[] =
            {
                {
                    LONG(cx + c * cos(s_i * deg2rad)),
                    LONG(cy + c * sin(s_i * deg2rad)),
                    0xFF00, 0, 0
                },
                {
                    LONG(cx + c * cos((s_i + 120) * deg2rad)),
                    LONG(cy + c * sin((s_i + 120) * deg2rad)),
                    0, 0xFF00, 0
                },
                {
                    LONG(cx + c * cos((s_i + 240) * deg2rad)),
                    LONG(cy + c * sin((s_i + 240) * deg2rad)),
                    0, 0, 0xFF00
                }
            };
            if (m_bUseMSIMG32)
            {
                GradientFill(hMemDC, tri, 3,
                             &triangle, 1, GRADIENT_FILL_TRIANGLE);
            }
            else
            {
                WonGradientFill(hMemDC, tri, 3,
                                &triangle, 1, GRADIENT_FILL_TRIANGLE);
            }
        }

        PAINTSTRUCT ps;
        HDC hDC = BeginPaint(hwnd, &ps);
        if (hDC)
        {
            SetStretchBltMode(hDC, STRETCH_DELETESCANS);
            BitBlt(hDC, 0, 0, m_siz.cx, m_siz.cy, hMemDC, 0, 0, SRCCOPY);

            char szBuf[32];
            static const char *modes[] = {"24", "8", "1"};
            if (m_bUseMSIMG32)
                wsprintfA(szBuf, "MS:%s", modes[m_nMode]);
            else
                wsprintfA(szBuf, "MR:%s", modes[m_nMode]);
            TextOutA(hDC, 0, 0, szBuf, lstrlenA(szBuf));

            EndPaint(hwnd, &ps);
        }
        SelectObject(hMemDC, hbmOld);

        ++s_i;
    }

    void OnTimer(HWND hwnd, UINT id)
    {
        KillTimer(hwnd, 999);
        InvalidateRect(hwnd, NULL, TRUE);
        SetTimer(hwnd, 999, INTERVAL, NULL);
    }

    virtual LRESULT CALLBACK
    WindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
            DO_MSG(WM_CREATE, OnCreate);
            DO_MSG(WM_LBUTTONDOWN, OnLButtonDown);
            DO_MSG(WM_RBUTTONDOWN, OnRButtonDown);
            DO_MSG(WM_ERASEBKGND, OnEraseBkgnd);
            DO_MSG(WM_PAINT, OnPaint);
            DO_MSG(WM_SIZE, OnSize);
            DO_MSG(WM_TIMER, OnTimer);
            DO_MSG(WM_DESTROY, OnDestroy);
        default:
            return DefaultProcDx(hwnd, uMsg, wParam, lParam);
        }
        return 0;
    }

    BOOL StartDx(INT nCmdShow)
    {
        if (!CreateWindowDx(NULL, LoadStringDx(1)))
        {
            ErrorBoxDx(TEXT("failure of CreateWindow"));
            return FALSE;
        }

        ::ShowWindow(*this, nCmdShow);
        ::UpdateWindow(*this);

        return TRUE;
    }

    // message loop
    INT RunDx()
    {
        MSG msg;
        while (::GetMessage(&msg, NULL, 0, 0))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
        }
        return INT(msg.wParam);
    }

    void OnDestroy(HWND hwnd)
    {
        ::PostQuitMessage(0);
    }

    void OnSize(HWND hwnd, UINT state, int cx, int cy)
    {
        RECT rc;
        ::GetClientRect(hwnd, &rc);
        m_siz = SizeFromRectDx(&rc);
        ReCreateBitmap(hwnd);
    }
};

//////////////////////////////////////////////////////////////////////////////
// Win32 App main function

extern "C"
INT APIENTRY _tWinMain(
    HINSTANCE   hInstance,
    HINSTANCE   hPrevInstance,
    LPTSTR      lpCmdLine,
    INT         nCmdShow)
{
    int ret;

    {
        MGradientFillTest app(hInstance);

        if (app.StartDx(nCmdShow))
        {
            ret = app.RunDx();
        }
        else
        {
            ret = 2;
        }
    }

#if (WINVER >= 0x0500)
    HANDLE hProcess = GetCurrentProcess();
    DebugPrintDx(TEXT("Count of GDI objects: %ld\n"),
                 GetGuiResources(hProcess, GR_GDIOBJECTS));
    DebugPrintDx(TEXT("Count of USER objects: %ld\n"),
                 GetGuiResources(hProcess, GR_USEROBJECTS));
#endif

#if defined(_MSC_VER) && !defined(NDEBUG)
    // for detecting memory leak (MSVC only)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    return ret;
}

//////////////////////////////////////////////////////////////////////////////
