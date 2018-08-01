#include "vox_platform.h"

#include <windows.h>
#include <process.h>
#undef near
#undef far
#include <GL/gl.h>
#include "thirdparty/glext.h"
#include "thirdparty/wglext.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "vox_gldefs.h"

typedef struct
{
	WNDCLASSEX windowClass;
	HWND window;
	HDC deviceContext;
	HGLRC glRenderContext;
	double timerResolution;

	LPARAM lastResizeDimensions;
	bool resizing;
	bool sizeEvent;

	bool mouseLocked;

	int clientW, clientH;
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
int win32_createGLContext();
void win32_handleEvents();
void win32_centerCursor();
void win32_setPlatformFlag(int flag, bool state);

int CALLBACK WinMain(
	HINSTANCE instance,
	HINSTANCE prevInstance,
	LPSTR commandLine,
	int cmdShow)
{

	LARGE_INTEGER queryRes;
	QueryPerformanceFrequency(&queryRes);
	win32->timerResolution = (double)queryRes.QuadPart / 1000.L;

	// TODO: move to init
	win32_platform->running = true;
	win32_platform->updateTarget = 1000.L / 120.L;
	win32_platform->renderTarget = 1000.L / 60.L;

	SYSTEM_INFO info;
	GetSystemInfo(&info);
	win32_platform->info.logicalCores = info.dwNumberOfProcessors;

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
	win32->clientW = tempClientRect.right - tempClientRect.left;
	win32->clientH = tempClientRect.bottom - tempClientRect.top;

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

	init(win32_platform);

	double prevTime, currentTime, elapsedTime, updateDelta, renderDelta;
	elapsedTime = updateDelta = renderDelta = 0;

	prevTime = getElapsedMs();

	while (win32_platform->running)
	{
		win32_handleEvents();

		if (getPlatformFlag(MOUSE_LOCKED))
			win32_centerCursor();

		currentTime = getElapsedMs();

		elapsedTime = currentTime - prevTime;

		if (elapsedTime > 100.L)
		{
			// TODO: Diagnose delay, adjust frequency
			elapsedTime = 100;
		}

		prevTime = currentTime;

		updateDelta += elapsedTime;
		renderDelta += elapsedTime;

		while (updateDelta >= win32_platform->updateTarget)
		{
			update();

			// Reset event queue
			win32_platform->filledEvents = 0;

			updateDelta -= win32_platform->updateTarget;
		}
		
		// Don't catch up lost render frames, just render last one
		if (renderDelta >= win32_platform->renderTarget)
		{
			render(updateDelta / win32_platform->updateTarget);
			SwapBuffers(win32->deviceContext);
			renderDelta = 0;
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

void win32_setPlatformFlag(int flag, bool state)
{
	if (state)
		win32_platform->flags |= flag;
	else
		win32_platform->flags &= ~flag;
}

bool getPlatformFlag(int flag)
{
	return !!(win32_platform->flags & flag);
}

POINT win32_getCenterScreen()
{
	RECT rect;
	GetClientRect(win32->window, &rect);
	POINT client = {(rect.right - rect.left) / 2, (rect.bottom - rect.top) / 2};
	ClientToScreen(win32->window, &client);
	return client;
}

void win32_centerCursor()
{
	RECT clientRect;
	GetClientRect(win32->window, &clientRect);
	
	POINT existing, client = win32_getCenterScreen();
	GetCursorPos(&existing);

	// Skip adding a redundant message if possible
	if (client.x == existing.x && client.y == existing.y)
		return;
	SetCursorPos(client.x, client.y);
}

void setMouseState(bool locked)
{
	if (locked == getPlatformFlag(MOUSE_LOCKED))
		return;
	
	if (locked)
	{
		win32_setPlatformFlag(MOUSE_LOCKED, true);
		win32_centerCursor();
		ShowCursor(false);
	}
	else
	{
		win32_setPlatformFlag(MOUSE_LOCKED, false);
		ShowCursor(true);
	}
}

void win32_handleEvents()
{
	MSG msg;
	while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

void win32_postEvent(Event event)
{
	assert(win32_platform->filledEvents < EVENT_QUEUE_SIZE);
	win32_platform->eventQueue[win32_platform->filledEvents] = event;
	win32_platform->filledEvents++;
}

// TODO: Add to event queue
LRESULT CALLBACK win32_windowProc(
	HWND window,
	UINT message,
	WPARAM wParam, LPARAM lParam)
{
	Event e = {};

	LRESULT result = 0;

	switch (message)
	{
		case WM_CLOSE:
		{
			DestroyWindow(window);
		}
		break;

		case WM_DESTROY:
		{
			win32_platform->running = false;
			PostQuitMessage(0);
		}
		break;

		case WM_SIZE:
		{
			win32->lastResizeDimensions = lParam;
			if (!win32->resizing)
			{
				e.type = EVENT_RESIZE;
				e.resize.width = LOWORD(lParam);
				e.resize.height = HIWORD(lParam);
				win32_platform->viewportWidth = LOWORD(lParam);
				win32_platform->viewportHeight = HIWORD(lParam);
				win32_postEvent(e);
			}
			else
			{
				win32->sizeEvent = true;
			}
		}
		break;

		case WM_ENTERSIZEMOVE:
		{
			win32->resizing = true;
		}
		break;

		case WM_EXITSIZEMOVE:
		{
			win32->resizing = false;
			if (win32->sizeEvent)
			{
				win32->sizeEvent = false;
				e.type = EVENT_RESIZE;
				e.resize.width = LOWORD(win32->lastResizeDimensions);
				e.resize.height = HIWORD(win32->lastResizeDimensions);
				win32_platform->viewportWidth = LOWORD(win32->lastResizeDimensions);
				win32_platform->viewportHeight = HIWORD(win32->lastResizeDimensions);
				win32_postEvent(e);
			}
		}
		break;

		case WM_KEYDOWN:
		case WM_KEYUP:
		{
			if (wParam == VK_ESCAPE)
				PostMessage(window, WM_CLOSE, 0, 0);

			if (wParam == VK_SHIFT)
			{
				unsigned short lShift = GetKeyState(VK_LSHIFT);
				unsigned short rShift = GetKeyState(VK_RSHIFT);
				lShift >>= 15; rShift >>= 15;
				if (message == WM_KEYDOWN)
				{
					if (lShift)
						e.key.keyCode = VK_LSHIFT;
					else
						e.key.keyCode = VK_RSHIFT;
				}
				else
				{
					if (!lShift)
						e.key.keyCode = VK_LSHIFT;
					else
						e.key.keyCode = VK_RSHIFT;
				}
			}
			else
				e.key.keyCode = wParam;

			e.type = EVENT_KEY;
			e.key.state = message == WM_KEYDOWN ? BUTTON_PRESSED : BUTTON_RELEASED;
			win32_postEvent(e);
		}
		break;

		case WM_MOUSEMOVE:
		{
			e.type = EVENT_MOUSE_MOVE;
			e.mouseMove.x = LOWORD(lParam);
			e.mouseMove.y = HIWORD(lParam);

			if (getPlatformFlag(MOUSE_LOCKED))
			{
				POINT center = win32_getCenterScreen();
				POINT now;
				GetCursorPos(&now);
				e.mouseMove.locked = true;
				e.mouseMove.dx = now.x - center.x; //win32->mouseDeltaX;
				e.mouseMove.dy = now.y - center.y; //win32->mouseDeltaY;
			}

			win32_postEvent(e);
		}
		break;

		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		{
			e.type = EVENT_MOUSE_CLICK;
			e.mouseClick.mouseButton = MOUSE_BUTTON_LEFT;
			e.mouseClick.state = wParam == MK_LBUTTON ? BUTTON_PRESSED : BUTTON_RELEASED;
			win32_postEvent(e);
		}
		break;

		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		{
			e.type = EVENT_MOUSE_CLICK;
			e.mouseClick.mouseButton = MOUSE_BUTTON_RIGHT;
			e.mouseClick.state = wParam == MK_RBUTTON ? BUTTON_PRESSED : BUTTON_RELEASED;
			win32_postEvent(e);
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

	typedef HGLRC (WINAPI *ContextLoaderFunc)
		(HDC hdc, HGLRC sharecontext, const int *attriblist);
	
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

	// Load GL functions
	#define GLDEF(ret, name, ...) gl##name = \
		(name##proc *) wglGetProcAddress("gl" #name);
	GL_LIST
	#undef GLDEF

	// Ensure functions have successfully loaded
	// TODO: something more robust for release mode
	#define GLDEF(retrn, name, ...) assert(gl##name);
	GL_LIST
	#undef GLDEF

	return 0;
}

bool createThread(void (*threadProc)(void*), void *threadArgs)
{
	uint64 res = _beginthread(threadProc, 0, threadArgs);

	if (res == -1L)
		return false;
	return true;
}

void atomicIncrement(volatile int *val)
{
	InterlockedIncrement((LONG*)val);
}

void atomicDecrement(volatile int *val)
{
	InterlockedDecrement((LONG*)val);
}

void* createMutex()
{
	// TODO: GROSS - avoid allocation if possible
	LPCRITICAL_SECTION cs = (LPCRITICAL_SECTION)malloc(sizeof(CRITICAL_SECTION));
	InitializeCriticalSection(cs);
	return (void*)cs;
}

int lockMutex(void *mutex)
{
	LPCRITICAL_SECTION cs = (LPCRITICAL_SECTION)mutex;
	EnterCriticalSection(cs); // TODO: Use TryEnterCriticalException, return a code
	return 0;
}

int unlockMutex(void *mutex)
{
	LPCRITICAL_SECTION cs = (LPCRITICAL_SECTION)mutex;
	LeaveCriticalSection(cs); // TODO: Same as lockMutex
	return 0;
}

double getElapsedMs()
{
	LARGE_INTEGER res;
	QueryPerformanceCounter(&res);
	double count = (double)res.QuadPart;
	return count / win32->timerResolution;
}

void sleepMs(int ms)
{
	Sleep(ms);
}

#include "vox_main.cpp"

