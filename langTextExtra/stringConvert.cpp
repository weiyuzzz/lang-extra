#include "stringConvert.h"

std::wstring stringToWstring(const std::string& str) {
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.length()), NULL, 0);
	std::wstring wstr(size_needed, 0);
	MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.length()), &wstr[0], size_needed);
	return wstr;
}

std::string wstringToString(const std::wstring& wstr) {
	int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), static_cast<int>(wstr.size()), NULL, 0, NULL, NULL);
	std::string str(size_needed, 0);
	WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), static_cast<int>(wstr.size()), &str[0], size_needed, NULL, NULL);
	return str;
}

std::string tchar_to_string(const TCHAR* tcharStr) {

	std::wstring wstr(tcharStr);
	int sizeNeeded = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, NULL, 0, NULL, NULL);
	std::string str(sizeNeeded, 0);
	WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, &str[0], sizeNeeded, NULL, NULL);

	return str;
}