#include "taskbar.hpp"
#include <memory>
#include <cassert>
#include <range/v3/view/iota.hpp>
#include <range/v3/view/transform.hpp>
#include <range/v3/to_container.hpp>

#include <UIAutomationClient.h>


#define VERIFY(expr) { auto hr = expr; assert(SUCCEEDED(hr)); }
auto constexpr task_list_cls = L"MSTaskListWClass";

CComPtr<IUIAutomationElement> find_task_list(IUIAutomation* ui_automation) {
  CComPtr<IUIAutomationElement> desktop;
  VERIFY(ui_automation->GetRootElement(&desktop));

  std::unique_ptr<wchar_t, decltype(&SysFreeString)> bstr_task_list_cls(SysAllocString(task_list_cls), &SysFreeString);
  VARIANT vt;
  vt.vt = VT_BSTR;
  vt.bstrVal = bstr_task_list_cls.get();
  CComPtr<IUIAutomationCondition> c1;
  VERIFY(ui_automation->CreatePropertyCondition(UIA_ClassNamePropertyId, vt, &c1));

  CComPtr<IUIAutomationElement> task_list;
  VERIFY(desktop->FindFirst(TreeScope_Descendants, c1, &task_list));

  return task_list;
}

std::vector<CComPtr<IUIAutomationElement>> get_running_apps(IUIAutomation* ui_automation, IUIAutomationElement* task_list) {
  CComPtr<IUIAutomationCondition> true_cond;
  VERIFY(ui_automation->CreateTrueCondition(&true_cond));

  CComPtr<IUIAutomationElementArray> children;
  VERIFY(task_list->FindAll(TreeScope_Children, true_cond, &children));

  int count;
  VERIFY(children->get_Length(&count));

  using namespace ranges;
  using namespace view;

  return closed_ints(0, count - 1) 
    | transform([&children](int i) {
        CComPtr<IUIAutomationElement> child;
        VERIFY(children->GetElement(i, &child));

        return child;
      })
    | to_<std::vector>();
}