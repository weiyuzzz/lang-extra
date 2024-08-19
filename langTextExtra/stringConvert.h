#pragma once
#include <string>   
#include <windows.h> 

std::wstring stringToWstring(const std::string& str);
std::string wstringToString(const std::wstring& wstr);
std::string tchar_to_string(const TCHAR* tcharStr);