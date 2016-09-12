// wnum.cpp : Defines the entry point for the application.
//
#include <UIAutomationClient.h>

#include "stdafx.h"
#include <string>
#include <vector>
#include <range/v3/view/iota.hpp>
#include <range/v3/view/transform.hpp>
#include <range/v3/view/zip.hpp>
#include <range/v3/to_container.hpp>
#include <range/v3/range_for.hpp>
#include <gdiplus.h>

#include "resource.h"
#include "gdi_raii.hpp"
#include "taskbar.hpp"

#pragma comment(lib, "gdiplus.lib")

ATOM register_badge_class(HINSTANCE);
HWND make_badge_window(HINSTANCE, int);
LRESULT CALLBACK    wnd_proc(HWND, UINT, WPARAM, LPARAM);
void show_badges(const std::vector<HWND>&, IUIAutomation*, IUIAutomationElement*);

auto constexpr badge_size = 16;
auto constexpr max_num = 9;
auto constexpr badge_opacity = 200;
auto constexpr badge_cls = L"wnum_badge_class";

auto constexpr msg_show = WM_USER + 1;
auto constexpr msg_hide = WM_USER + 2;

HHOOK hook;

int APIENTRY wWinMain(HINSTANCE instance, HINSTANCE, LPWSTR, int) {
  using namespace ranges;
  using namespace ranges::view;

  using namespace Gdiplus;
  ULONG_PTR gdip_token;
  GdiplusStartupInput gdip_startup_input;
  GdiplusStartup(&gdip_token, &gdip_startup_input, nullptr);

  // get the task list
  CoInitialize(nullptr);
  CComPtr<IUIAutomation> ui_automation;
  CoCreateInstance(__uuidof(CUIAutomation), NULL, CLSCTX_INPROC_SERVER,
    __uuidof(IUIAutomation), (void**)&ui_automation);
  auto task_list = find_task_list(ui_automation);

  // create badge windows
  register_badge_class(instance);
  auto wnds = closed_ints(1, max_num)
    | transform([instance](int i) { return make_badge_window(instance, i); })
    | to_<std::vector>();


  hook = SetWindowsHookExW(WH_KEYBOARD_LL, [](int code, WPARAM wparam, LPARAM lparam) {
    auto vk = reinterpret_cast<KBDLLHOOKSTRUCT*>(lparam)->vkCode;
    if (vk == VK_LWIN || vk == VK_RWIN && wparam == WM_KEYDOWN || wparam == WM_KEYUP) {
      PostThreadMessageW(GetCurrentThreadId(), wparam == WM_KEYDOWN ? msg_show : msg_hide, 0, 0);
    }
    return CallNextHookEx(hook, code, wparam, lparam);
  }, nullptr, 0);

  MSG msg;
  while (GetMessage(&msg, nullptr, 0, 0)) {
    switch (msg.message) {
    case msg_show:
      show_badges(wnds, ui_automation, task_list);
      break;

    case msg_hide:
      RANGES_FOR(auto wnd, wnds) { ShowWindow(wnd, SW_HIDE); }
      break;

    default:
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    } 
  }

  UnhookWindowsHookEx(hook);
  GdiplusShutdown(gdip_token);
  return (int)msg.wParam;
}


ATOM register_badge_class(HINSTANCE hInstance) {
  WNDCLASSEXW wcex;

  wcex.cbSize = sizeof(WNDCLASSEX);

  wcex.style = CS_HREDRAW | CS_VREDRAW;
  wcex.lpfnWndProc = wnd_proc;
  wcex.cbClsExtra = 0;
  wcex.cbWndExtra = 0;
  wcex.hInstance = hInstance;
  wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WNUM));
  wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wcex.hbrBackground = (HBRUSH)(GetStockObject(BLACK_BRUSH));
  wcex.lpszClassName = badge_cls;
  wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
  wcex.lpszMenuName = nullptr;

  return RegisterClassExW(&wcex);
}

HWND make_badge_window(HINSTANCE instance, int badge_number) {
  HWND hWnd = CreateWindowExW(
    WS_EX_COMPOSITED | WS_EX_LAYERED | WS_EX_NOACTIVATE | WS_EX_TOPMOST | WS_EX_TRANSPARENT,
    badge_cls,
    nullptr,
    WS_POPUP,
    0,
    0,
    badge_size,
    badge_size,
    nullptr,
    nullptr,
    instance,
    nullptr
  );

  if (!hWnd) {
    return nullptr;
  }

  SetLayeredWindowAttributes(hWnd, RGB(0, 0, 0), badge_opacity, LWA_ALPHA | LWA_COLORKEY);
  SetWindowLongW(hWnd, GWL_USERDATA, badge_number);

  ShowWindow(hWnd, SW_HIDE);
  UpdateWindow(hWnd);

  return hWnd;
}

void draw_badge_circle(HDC dc, const RECT& rc) {
  using namespace Gdiplus;
  Rect grc{ rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top };
  Graphics g{ dc };
  SolidBrush brush{ Color{ 70, 70, 70 } };
  g.FillEllipse(&brush, grc);
}

void draw_badge_number(HDC dc, RECT& rc, int number) {
  auto font = make_unique_gdi_object(CreateFontW(
    badge_size,
    0,
    0,
    0,
    FW_DEMIBOLD,
    FALSE,
    FALSE,
    FALSE,
    DEFAULT_CHARSET,
    OUT_CHARACTER_PRECIS,
    CLIP_CHARACTER_PRECIS,
    CLEARTYPE_QUALITY,
    FF_DONTCARE,
    L"Segoe UI"
  ));
  auto g1 = guard_select_object(dc, font.get());
  SetTextColor(dc, RGB(255, 255, 255));
  DrawTextW(
    dc,
    std::to_wstring(number).c_str(),
    -1,
    &rc,
    DT_CENTER | DT_VCENTER | DT_SINGLELINE
  );
}

LRESULT CALLBACK wnd_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
  switch (message) {

  case WM_PAINT:
  {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hWnd, &ps);
    SetBkMode(hdc, TRANSPARENT);
    RECT rc;
    GetClientRect(hWnd, &rc);
    draw_badge_circle(hdc, rc);
    draw_badge_number(hdc, rc, GetWindowLongW(hWnd, GWL_USERDATA));


    EndPaint(hWnd, &ps);
  }
  break;

  case WM_DESTROY:
    PostQuitMessage(0);
    break;

  default:
    return DefWindowProc(hWnd, message, wParam, lParam);
  }
  return 0;
}

void show_badges(const std::vector<HWND>& wnds, IUIAutomation * ui_automation, IUIAutomationElement * task_list) {
  using namespace ranges;
  auto apps = get_running_apps(ui_automation, task_list);
  auto app_rcs = apps | view::transform([](auto&& elem) {
    RECT rc;
    elem->get_CurrentBoundingRectangle(&rc);
    return rc;
  });

  RANGES_FOR(auto&& tup, view::zip(wnds, app_rcs)) {
    HWND wnd;
    RECT rc;
    std::tie(wnd, rc) = tup;

    SetWindowPos(wnd, HWND_TOPMOST, rc.left, rc.top, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE);
  }
}