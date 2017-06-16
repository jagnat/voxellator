#include "vox_platform.h"

#include <windows.h>
#include <GL/gl.h>
#include "thirdparty/wglext.h"

#include <stdio.h>
#include <stdlib.h>

typedef struct
{
	WNDCLASSEX windowClass;
	HWND window;
	HDC deviceContext;
	HGLRC glRenderContext;
	double timerResolution;
} Win32Data;

// TODO: Consider allocating these on the heap
static Win32Data _win32 = {0};
static Win32Data *win32 = &_win32;

static PlatformState _win32_platform = {0};
static PlatformState *win32_platform = &_win32_platform;

// Functions
LRESULT CALLBACK win32_windowProc(
	HWND window,
	UINT message,
	WPARAM wParam, LPARAM lParam);
int CreateGLContext();
double win32_elapsedMs();
void win32_handleEvents();

int CALLBACK WinMain(
	HINSTANCE instance,
	HINSTANCE prevInstance,
	LPSTR commandLine,
	int cmdShow)
{

	LARGE_INTEGER queryRes;
	QueryPerformanceCounter(&queryRes);
	win32->timerResolution = (double)queryRes.QuadPart / 1000.L;

	// TODO: move to init
	win32_platform->running = true;
	win32_platform->targetDelta = 1000.L / 60.L;

	WNDCLASSEX *wc = &win32->windowClass;
	wc->cbSize = sizeof(WNDCLASSEX);
	wc->style = CS_OWNDC;
	wc->lpfnWndProc = win32_windowProc;
	wc->hInstance = instance;
	wc->hCursor = LoadCursor(0, IDC_ARROW);
	wc->lpszClassName = "VoxWindowClass";
	if (!RegisterClassEx(wc))
	{
		// TODO: Log
		return 1;
	}

	// Set client viewport to resolution size, not including borders
	// TODO: get from settings
	RECT tempClientRect = {0, 0, 1280, 720};
	AdjustWindowRectEx(
		&tempClientRect, // rectangle - src/dest
		WS_OVERLAPPEDWINDOW, // window style
		false, // window has no menu
		0); // no extended window style

	win32->window = CreateWindowEx(
		0, // extended style
		"VoxWindowClass", // window class
		"Voxellator", // window title
		WS_OVERLAPPEDWINDOW, // window style
		CW_USEDEFAULT, CW_USEDEFAULT, // pos
		tempClientRect.right - tempClientRect.left, // size
		tempClientRect.bottom - tempClientRect.top,
		0, // no parent
		0, // no menu
		instance,
		0); // no params
	if (!win32->window)
	{
		// TODO: Log
		return 1;
	}

	win32->deviceContext = GetDC(win32->window);
	if (!win32->deviceContext)
	{
		// TODO: Log
		return 1;
	}

	
	// TODO: Support hotswitching render APIs, this or vulkan
	if (win32_createGLContext())
	{
		// TODO: Log
		return 1;
	}

	ShowWindow(win32->window, cmdShow);
	UpdateWindow(win32->window);

	SwapBuffers(win32->deviceContext);

	double prevTime, currentTime, elapsedTime, accumTime;
	elapsedTime = accumTime = 0;

	prevTime = win32_elapsedMs();

	while (win32_platform->running)
	{
		win32_handleEvents();

		currentTime = win32_elapsedMs();

		elapsedTime = currentTime - prevTime;

		if (elapsedTime > 100.L)
		{
			// TODO: Diagnose delay, adjust frequency
			elapsedTime = 100;
		}

		prevTime = currentTime;

		accumTime += elapsedTime;

		if (accumTime >= win32_platform->targetDelta)
		{
			printf("test2\n");
			tick(accumTime);
			SwapBuffers(win32->deviceContext);
			accumTime = 0;
		}

		Sleep(1);
	}

	return 0;
}

// For /SUBSYSTEM:CONSOLE flag
// TODO: Don't even compile this if release build
int main(int argc, char **argv)
{
	return WinMain(GetModuleHandle(0), 0, GetCommandLine(), SW_SHOW);
}

void win32_handleEvents()
{
	MSG msg;
	while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
	{
		if (msg.message == WM_QUIT)
			win32_platform->running = false;
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

// TODO: Add to event queue
LRESULT CALLBACK win32_windowProc(
	HWND window,
	UINT message,
	WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 1;

	switch (message)
	{
		case WM_CLOSE:
		{
			DestroyWindow(window);
		}
		break;

		case WM_DESTROY:
		{
			PostQuitMessage(0);
		}
		break;

		case WM_KEYDOWN:
		case WM_KEYUP:
		{
			if (wParam == VK_ESCAPE)
				DestroyWindow(window);
		}
		break;

		case WM_MOUSEMOVE:
		{

		}
		break;

		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		{

		}
		break;

		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		{

		}
		break;

		default:
		{
			result = DefWindowProc(window, message, wParam, lParam);
		}
		break;
	}

	return result;
}

int win32_createGLContext()
{
	HDC dc = win32->deviceContext;

	PIXELFORMATDESCRIPTOR desc =
	{
		sizeof(PIXELFORMATDESCRIPTOR),
		1,
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
		32,
		0, 0, 0, 0, 0, 0,
		0,
		0,
		0,
		0, 0, 0, 0,
		24,
		8,
		0,
		PFD_MAIN_PLANE,
		0,
		0, 0, 0
	};

	int pixelFormat = ChoosePixelFormat(dc, &desc);
	if (!pixelFormat)
	{
		// TODO: Log
		return 1;
	}

	if (!SetPixelFormat(dc, pixelFormat, &desc))
	{
		// TODO: Log
		return 1;
	}
	
	// Modern opengl render context loading:
	// Make temporary render context, use it to get pointer to
	// modern context creator function (wglCreateContextAttribsARB).
	// call new function with attribute params, delete old context.

	HGLRC tempRC = wglCreateContext(dc);
	wglMakeCurrent(dc, tempRC);

	typedef HGLRC (WINAPI *ContextLoaderFunc) (HDC hdc, HGLRC sharecontext, const int *attriblist);
	ContextLoaderFunc _wglCreateContextAttribsARB =
		(ContextLoaderFunc)wglGetProcAddress("wglCreateContextAttribsARB");
	if (!_wglCreateContextAttribsARB)
	{
		// TODO: Log
		return 1;
	}

	// TODO: Set this based on loaded settings and supported hardware
	int glAttribs[] =
	{
		WGL_CONTEXT_MAJOR_VERSION_ARB, 3, // Target GL 3.3
		WGL_CONTEXT_MINOR_VERSION_ARB, 3,
		WGL_CONTEXT_FLAGS_ARB, 0,
		WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
		0
	};

	win32->glRenderContext = _wglCreateContextAttribsARB(dc, 0, glAttribs);
	if (!win32->glRenderContext)
	{
		// TODO: Log
		return 1;
	}

	wglMakeCurrent(dc, win32->glRenderContext);
	wglDeleteContext(tempRC);

	return 0;
}

double win32_elapsedMs()
{
	LARGE_INTEGER res;
	QueryPerformanceCounter(&res);
	double count = (double)res.QuadPart;
	return count / win32->timerResolution;
}

#include "vox_main.c"

