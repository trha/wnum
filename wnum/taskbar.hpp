#pragma once
#include <atlbase.h>
#include <vector>

struct IUIAutomationElement;
struct IUIAutomation;
struct IUIAutomationElementArray;

CComPtr<IUIAutomationElement> find_task_list(IUIAutomation* ui_automation);
std::vector<CComPtr<IUIAutomationElement>> get_running_apps(IUIAutomation* ui_automation, IUIAutomationElement* task_list);