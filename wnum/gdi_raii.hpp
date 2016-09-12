#pragma once
#include "stdafx.h"
#include <memory>

template <typename H>
auto make_unique_gdi_object(H object_handle)
{
  return std::unique_ptr<std::remove_pointer_t<H>, decltype(&DeleteObject)>(object_handle, &DeleteObject);
}

auto make_unique_gdi_object(HDC dc)
{
  return std::unique_ptr<std::remove_pointer_t<HDC>, decltype(&DeleteDC)>(dc, &DeleteDC);
}

struct Select_object_guard {
  HDC dc;
  HGDIOBJ old_object;
  ~Select_object_guard()
  {
    SelectObject(dc, old_object);
  }
};

Select_object_guard guard_select_object(HDC dc, HGDIOBJ object)
{
  return{
    dc,
    SelectObject(dc, object)
  };
}

auto get_dc(HWND wnd)
{
  auto dc = GetDC(wnd);
  auto deleter = [wnd](auto dc) {
    ReleaseDC(wnd, dc);
  };

  return std::unique_ptr<std::remove_pointer_t<decltype(dc)>, decltype(deleter)>(dc, deleter);
}