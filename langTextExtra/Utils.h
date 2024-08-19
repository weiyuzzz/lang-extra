#ifndef UTILS_H
#define UTILS_H

#include <vector>
#include <string>

#define _CRT_SECURE_NO_WARNINGS

using namespace std;

#ifdef _WCHAR_T
#define mystring std::wstring
#else
#define mystring std::string
#endif

class Utils
{
public:
    Utils();
    static int readCSV(string path, vector<vector<string>>& data);
    static int saveCSV(string path, vector<vector<wstring>>& data);
    static void fileToString(vector<string> &record, string& line, char delimiter);
	static int unicodeToUtf8(const vector<wstring> src, unsigned char *u8);
	static mystring multiByte2Utf8(const char* cont);
	static std::wstring utf8ToMultiByte(const char* cont);
};

#endif // UTILS_H
