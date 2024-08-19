#include "Utils.h"
#include "atlstr.h"
#include <string>
#include <fstream>


Utils::Utils()
{

}

int Utils::readCSV(string path, vector<vector<string>> &data)
{
    vector<string> row;
    string line;
    ifstream in(path);
    if (in.fail()) {
        return -1;
    }
    while (in.good() && getline(in, line)) {
        fileToString(row, line, ',');  //把line里的单元格数字字符提取出来，","为单元格分隔符
        data.push_back(row);
        row.clear();
    }
    in.close();
    return 0;
}

int Utils::unicodeToUtf8(const vector<wstring> src, unsigned char *u8)
{
	int u8_pos = 0;
	for (unsigned int i = 0; i < src.size(); ++i) {
		for (unsigned int j = 0; j < src[i].size(); ++j) {
			int uc = src[i][j];
			unsigned char *pout = u8 + u8_pos;
			if (uc <= 0x0000007F) {
				// * U-00000000 - U-0000007F:  0xxxxxxx
				*pout = (uc & 0x7F);
				u8_pos++;
			} else if (uc >= 0x00000080 && uc <= 0x000007FF) {
				// * U-00000080 - U-000007FF:  110xxxxx 10xxxxxx
				*(pout + 1) = (uc & 0x3F) | 0x80;
				*pout = ((uc >> 6) & 0x1F) | 0xC0;
				u8_pos += 2;
			} else if (uc >= 0x00000800 && uc <= 0x0000FFFF) {
				// * U-00000800 - U-0000FFFF:  1110xxxx 10xxxxxx 10xxxxxx
				*(pout + 2) = (uc & 0x3F) | 0x80;
				*(pout + 1) = ((uc >> 6) & 0x3F) | 0x80;
				*pout = ((uc >> 12) & 0x0F) | 0xE0;
				u8_pos += 3;
			} else if (uc >= 0x00010000 && uc <= 0x001FFFFF) {
				// * U-00010000 - U-001FFFFF:  11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
				*(pout + 3) = (uc & 0x3F) | 0x80;
				*(pout + 2) = ((uc >> 6) & 0x3F) | 0x80;
				*(pout + 1) = ((uc >> 12) & 0x3F) | 0x80;
				*pout = ((uc >> 18) & 0x07) | 0xF0;
				u8_pos += 4;
			} else if (uc >= 0x00200000 && uc <= 0x03FFFFFF) {
				// * U-00200000 - U-03FFFFFF:  111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
				*(pout + 4) = (uc & 0x3F) | 0x80;
				*(pout + 3) = ((uc >> 6) & 0x3F) | 0x80;
				*(pout + 2) = ((uc >> 12) & 0x3F) | 0x80;
				*(pout + 1) = ((uc >> 18) & 0x3F) | 0x80;
				*pout = ((uc >> 24) & 0x03) | 0xF8;
				u8_pos += 5;
			} else if (uc >= 0x04000000 && uc <= 0x7FFFFFFF) {
				// * U-04000000 - U-7FFFFFFF:  1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
				*(pout + 5) = (uc & 0x3F) | 0x80;
				*(pout + 4) = ((uc >> 6) & 0x3F) | 0x80;
				*(pout + 3) = ((uc >> 12) & 0x3F) | 0x80;
				*(pout + 2) = ((uc >> 18) & 0x3F) | 0x80;
				*(pout + 1) = ((uc >> 24) & 0x3F) | 0x80;
				*pout = ((uc >> 30) & 0x01) | 0xFC;
				u8_pos += 6;
			}
		}
		*(u8 + u8_pos) = ',';
		u8_pos++;
	}
	return u8_pos;
}



int Utils::saveCSV(string path, vector<vector<wstring>> &data)
{
	FILE* fp = fopen(path.c_str(), "wb");  
    if (fp == NULL) {
		perror("打开文件失败");
        return -1;
    }
	for (unsigned int i = 0; i < data.size(); ++i) {
		vector<wstring> line = data[i];
		// write line
		unsigned char buf[4096] = { 0 };
		int len = Utils::unicodeToUtf8(line, buf);
		buf[len] = '\r';
		buf[len+1] = '\n';
		fwrite(buf, len+2, 1, fp);
	}
    fclose(fp);
    return 0;
}

void Utils::fileToString(vector<string> &record, string &line, char delimiter)
{
    char c;
    int linepos = 0;
    int linemax = line.length();
    string curstring;
    record.clear();
    while (linepos<linemax) {
        c = line[linepos];
        if (c == '\r' || c == '\n') {
            break;
        } else if (c == delimiter) {
            record.push_back(curstring.c_str());
            curstring = "";
        } else {
            curstring += c;
        }
        ++linepos;
    }
    if (curstring.size()) {
        record.push_back(curstring.c_str());
    }
}

#include <wchar.h>
#include <windows.h>

wchar_t *ANSIToUnicode(const char* str)
{
	int    textlen;
	wchar_t * result;
	textlen = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
	result = (wchar_t *)malloc((textlen + 1) * sizeof(wchar_t));
	memset(result, 0, (textlen + 1) * sizeof(wchar_t));
	MultiByteToWideChar(CP_ACP, 0, str, -1, (LPWSTR)result, textlen);
	return result;
}

char * UnicodeToANSI(const wchar_t *str)
{
	char * result;
	int textlen;
	// wide char to multi char
	textlen = WideCharToMultiByte(CP_ACP, 0, str, -1, NULL, 0, NULL, NULL);
	result = (char *)malloc((textlen + 1) * sizeof(char));
	memset(result, 0, sizeof(char) * (textlen + 1));
	WideCharToMultiByte(CP_ACP, 0, str, -1, result, textlen, NULL, NULL);
	return result;
}

mystring Utils::multiByte2Utf8(const char* str)
{
	if (NULL == str) {
		return "";
	}
	// ANSIToUnicode
	int num = MultiByteToWideChar(CP_ACP, NULL, str, -1, NULL, NULL);
	if (num <= 0) {
		return "";
	}
	wchar_t* buffw = new (std::nothrow) wchar_t[num+1];
	if (NULL == buffw) {
		return "";
	}
	MultiByteToWideChar(CP_ACP, NULL, str, -1, buffw, num);
	// UnicodeToUTF8
	int len = WideCharToMultiByte(CP_UTF8, 0, buffw, -1, NULL, NULL, NULL, NULL);
	if (len <= 0) {
		delete[] buffw;
		return "";
	}
	char* lpsz = new (std::nothrow) char[len + 1];
	if (NULL == lpsz) {
		delete[] buffw;
		return "";
	}
	WideCharToMultiByte(CP_UTF8, 0, buffw, num, lpsz, len, NULL, NULL);
	lpsz[len] = '\0';
	delete[] buffw;
	mystring rtn(lpsz);
	delete[] lpsz;
	return rtn;
}

std::wstring Utils::utf8ToMultiByte(const char* str)
{
	if (NULL == str) {
		return _T("");
	}
	int num = MultiByteToWideChar(CP_UTF8, NULL, str, -1, NULL, NULL);
	if (num <= 0) {
		return _T("");
	}
	wchar_t* buffw = new (std::nothrow) wchar_t[num+1];
	if (NULL == buffw) {
		return _T("");
	}
	MultiByteToWideChar(CP_UTF8, NULL, str, -1, buffw, num);
	//int pSize = WideCharToMultiByte(CP_ACP, 0, buffw, wcslen(buffw), NULL, 0, NULL, NULL);
	//char* pCStrKey = new char[pSize + 1];
	//WideCharToMultiByte(CP_ACP, 0, buffw, wcslen(buffw), pCStrKey, pSize, NULL, NULL);
	//pCStrKey[pSize] = '\0';
	std::wstring rtn(buffw);
	delete[] buffw;
	return rtn;
}
