#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#include <windows.h>
#include <windowsx.h>
#include "resource.h"

#define DEFAULT_DPI 96
#define SCALEX(X) MulDiv(X, uDpiX, DEFAULT_DPI)
#define SCALEY(Y) MulDiv(Y, uDpiY, DEFAULT_DPI)
#define POINT2PIXEL(PT) MulDiv(PT, uDpiY, 72)

TCHAR szClassName[] = TEXT("Window");

BOOL GetScaling(HWND hWnd, UINT* pnX, UINT* pnY)
{
	BOOL bSetScaling = FALSE;
	const HMONITOR hMonitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
	if (hMonitor)
	{
		HMODULE hShcore = LoadLibrary(TEXT("SHCORE"));
		if (hShcore)
		{
			typedef HRESULT __stdcall GetDpiForMonitor(HMONITOR, int, UINT*, UINT*);
			GetDpiForMonitor* fnGetDpiForMonitor = reinterpret_cast<GetDpiForMonitor*>(GetProcAddress(hShcore, "GetDpiForMonitor"));
			if (fnGetDpiForMonitor)
			{
				UINT uDpiX, uDpiY;
				if (SUCCEEDED(fnGetDpiForMonitor(hMonitor, 0, &uDpiX, &uDpiY)) && uDpiX > 0 && uDpiY > 0)
				{
					*pnX = uDpiX;
					*pnY = uDpiY;
					bSetScaling = TRUE;
				}
			}
			FreeLibrary(hShcore);
		}
	}
	if (!bSetScaling)
	{
		HDC hdc = GetDC(NULL);
		if (hdc)
		{
			*pnX = GetDeviceCaps(hdc, LOGPIXELSX);
			*pnY = GetDeviceCaps(hdc, LOGPIXELSY);
			ReleaseDC(NULL, hdc);
			bSetScaling = TRUE;
		}
	}
	if (!bSetScaling)
	{
		*pnX = DEFAULT_DPI;
		*pnY = DEFAULT_DPI;
		bSetScaling = TRUE;
	}
	return bSetScaling;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static HWND hButton;
	static HWND hEdit;
	static HFONT hFont;
	static UINT uDpiX = DEFAULT_DPI, uDpiY = DEFAULT_DPI;
	static HBRUSH hBrush;
	static BOOL bDrag;
	static HCURSOR hCursor;
	switch (msg)
	{
	case WM_CREATE:
		hCursor = LoadCursor(((LPCREATESTRUCT)lParam)->hInstance, MAKEINTRESOURCE(IDC_CURSOR1));
		hBrush = CreateSolidBrush(RGB(255, 255, 255));
		SetClassLongPtr(hWnd, GCLP_HBRBACKGROUND, (LONG_PTR)hBrush);
		hEdit = CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("EDIT"), 0, WS_VISIBLE | WS_CHILD | ES_MULTILINE | ES_AUTOHSCROLL | ES_AUTOVSCROLL | ES_READONLY, 0, 0, 0, 0, hWnd, 0, ((LPCREATESTRUCT)lParam)->hInstance, 0);
		SendMessage(hWnd, WM_DPICHANGED, 0, 0);
		break;
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hWnd, &ps);
			if (!bDrag)
			{
				DrawIcon(hdc, POINT2PIXEL(10), POINT2PIXEL(10), hCursor);
			}
			EndPaint(hWnd, &ps);
		}
		break;
	case WM_LBUTTONDOWN:
		bDrag = TRUE;
		SetCapture(hWnd);
		break;
	case WM_MOUSEMOVE:
		if (bDrag)
		{
			SetCursor(hCursor);
			POINT point = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
			ClientToScreen(hWnd, &point);
			HWND hDesktop = GetDesktopWindow();
			HDC hdc = GetDC(hDesktop);
			DeleteObject(hBrush);
			COLORREF color = GetPixel(hdc, point.x, point.y);
			{
				SetWindowText(hEdit, 0);
				WCHAR szText[256];
				wsprintf(szText, L"#%02x%02x%02x\r\n", GetRValue(color), GetGValue(color), GetBValue(color));
				SendMessage(hEdit, EM_REPLACESEL, (WPARAM)FALSE, (LPARAM)szText);
				wsprintf(szText, L"%d, %d, %d\r\n", GetRValue(color), GetGValue(color), GetBValue(color));
				SendMessage(hEdit, EM_REPLACESEL, (WPARAM)FALSE, (LPARAM)szText);

			}
			hBrush = CreateSolidBrush(color);
			SetClassLongPtr(hWnd, GCLP_HBRBACKGROUND, (LONG_PTR)hBrush);
			InvalidateRect(hWnd, 0, TRUE);
			ReleaseDC(hDesktop, hdc);
		}
		break;
	case WM_LBUTTONUP:
		if (bDrag) {
			ReleaseCapture();
			InvalidateRect(hWnd, 0, TRUE);
			bDrag = FALSE;
		}
		break;
	case WM_SIZE:
		MoveWindow(hEdit, POINT2PIXEL(10 + 32 + 10), POINT2PIXEL(10), LOWORD(lParam) - POINT2PIXEL(20 + 32 + 10), HIWORD(lParam) - POINT2PIXEL(20), TRUE);
		break;
	case WM_NCCREATE:
		{
			const HMODULE hModUser32 = GetModuleHandle(TEXT("user32.dll"));
			if (hModUser32)
			{
				typedef BOOL(WINAPI*fnTypeEnableNCScaling)(HWND);
				const fnTypeEnableNCScaling fnEnableNCScaling = (fnTypeEnableNCScaling)GetProcAddress(hModUser32, "EnableNonClientDpiScaling");
				if (fnEnableNCScaling)
				{
					fnEnableNCScaling(hWnd);
				}
			}
		}
		return DefWindowProc(hWnd, msg, wParam, lParam);
	case WM_DPICHANGED:
		GetScaling(hWnd, &uDpiX, &uDpiY);
		DeleteObject(hFont);
		hFont = CreateFontW(-POINT2PIXEL(10), 0, 0, 0, FW_NORMAL, 0, 0, 0, ANSI_CHARSET, 0, 0, 0, 0, L"Consolas");
		SendMessage(hButton, WM_SETFONT, (WPARAM)hFont, 0);
		SendMessage(hEdit, WM_SETFONT, (WPARAM)hFont, 0);
		break;
	case WM_DESTROY:
		DestroyCursor(hCursor);
		DeleteObject(hBrush);
		DeleteObject(hFont);
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPreInst, LPSTR pCmdLine, int nCmdShow)
{
	MSG msg;
	WNDCLASS wndclass = {
		CS_HREDRAW | CS_VREDRAW,
		WndProc,
		0,
		0,
		hInstance,
		0,
		LoadCursor(0,IDC_ARROW),
		(HBRUSH)(COLOR_WINDOW + 1),
		0,
		szClassName
	};
	RegisterClass(&wndclass);
	HWND hWnd = CreateWindow(
		szClassName,
		TEXT("Desktop Color Picker"),
		WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
		CW_USEDEFAULT,
		0,
		CW_USEDEFAULT,
		0,
		0,
		0,
		hInstance,
		0
	);
	ShowWindow(hWnd, SW_SHOWDEFAULT);
	UpdateWindow(hWnd);
	while (GetMessage(&msg, 0, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return (int)msg.wParam;
}
