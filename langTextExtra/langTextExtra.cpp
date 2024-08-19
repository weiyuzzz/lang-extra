#include <shlobj.h>  
#include <filesystem>
#include <map>
#include <list>
#include <vector>
#include <string>
#include <xlsxwriter.h>
#include <stdafx.h>

#include "stringConvert.h"
#include "dxml.h"
#include "UIlib.h"
#include "Utils.h"

#define XMLFILEPATH  "..\\out\\config.xml"

std::vector<std::string> fileFilter = { "style.xml",
							  "style_ar.xml",
							  "style_cht.xml",
							  "style_de.xml",
							  "style_en.xml",
							  "style_es.xml",
							  "style_fr.xml",
							  "style_he.xml",
							  "style_it.xml",
							  "style_nl.xml",
							  "style_pl.xml",
							  "style_pt.xml",
							  "style_ru.xml",
							  "style_slo.xml",
							  "style_tr.xml",
							  "style_vi.xml" };

std::vector<std::wstring> language = { _T("简体中文"),
					  _T("Arabic"),
					  _T("繁體中文"),
					  _T("German"),
					  _T("English"),
					  _T("Spanish"),
					  _T("French"),
					  _T("Hebrew"),
					  _T("Italian"),
					  _T("Dutch"),
					  _T("Polish"),
					  _T("Portuguese"),
					  _T("Russian"),
					  _T("Slovakia"),
					  _T("Turkish"),
					  _T("Vietnamese") };

std::vector<std::string> sepcial_label = { "panel",
								"wall",
								"center",
								"person",
								"busy" };



using namespace DuiLib;

class langTextExtraFrameWnd : public WindowImplBase
{

public:
	langTextExtraFrameWnd() {}
	virtual ~langTextExtraFrameWnd() { PostQuitMessage(0); }
	
	virtual LPCTSTR    GetWindowClassName() const override { return _T("langTextExtraFrameWnd"); }
	virtual void OnFinalMessage(HWND hWnd) override { delete this; }
	virtual CDuiString GetSkinFolder() override { return _T("skin\\"); }
	virtual CDuiString GetSkinFile() override { return _T("duilib.xml"); }
	
	void InitWindow() override {		
		m_path = static_cast<CEditUI*>(m_PaintManager.FindControl(_T("edit1")));
		m_result = static_cast<CListUI*>(m_PaintManager.FindControl(_T("list1")));
		m_pMinBtn = static_cast<CButtonUI*>(m_PaintManager.FindControl(_T("minbtn")));
		m_pCloseBtn = static_cast<CButtonUI*>(m_PaintManager.FindControl(_T("closebtn")));
	}
		

	void Notify(TNotifyUI& msg) override {
		if (msg.sType == _T("click")) {
			if (msg.pSender->GetName() == _T("btn_import")) OnBtnImport(msg);
			else if (msg.pSender->GetName() == _T("btn_start")) OnBtnStart(msg);
			else if (msg.pSender->GetName() == _T("closebtn")) { PostQuitMessage(0); return; }
			else if (msg.pSender->GetName() == _T("minbtn")) { SendMessage(WM_SYSCOMMAND, SC_MINIMIZE, 0); return; }
		}
	}

	void OnBtnImport(TNotifyUI& msg) {
		WCHAR szPath[MAX_PATH];
		ZeroMemory(szPath, sizeof(szPath));
		BROWSEINFO bi;
		bi.hwndOwner = m_PaintManager.GetPaintWindow();
		bi.pidlRoot = NULL;
		bi.pszDisplayName = szPath;
		bi.lpszTitle = L"请选择需要打包的目录：";
		bi.ulFlags = BIF_NEWDIALOGSTYLE | BIF_USENEWUI; // 添加样式以确保使用新对话框
		bi.lpfn = NULL;
		bi.lParam = 0;
		bi.iImage = 0;

		LPITEMIDLIST lp = SHBrowseForFolder(&bi);
		if (lp && SHGetPathFromIDList(lp, szPath)) {
			std::wstring importPath(szPath);
			m_result->RemoveAll();
			m_path->SetText(importPath.c_str());
		}
		else {
			::MessageBox(NULL, L"无效的目录，请重新选择", L"点击了按钮", MB_OK);
		}
	}

	void OnBtnStart(TNotifyUI& msg) {
		std::wstring path;
		path = m_path->GetText().GetData(); // 获取文本数据
		if (path.empty()) {
			::MessageBox(NULL, L"请先选择文件目录", L"提示", MB_OK);
			return;
		}
		m_result->RemoveAll();
		textExtractAndSaveXlsx(getFiles(wstringToString(path)), path);
	}

	bool isSkip(std::wstring str) {
		if (str.size() >= 32 || str.size() == 1) {
			return true;
		}
		for (unsigned int i = 0; i < str.size(); ++i) {
			if (str[i] >= '0' && str[i] <= '9') {
				continue;
			}
			else if (str[i] == '-' || str[i] == ':'
				|| str[i] == '.' || str[i] == ' '
				|| str[i] == '@' || str[i] == '?') {
				continue;
			}
			else {
				return false;
			}
		}
		return true;
	}

	std::map<std::string, std::list<std::string>> getFiles(const std::string& path ) {
		std::map<std::string, std::list<std::string>> filesMap;
		std::list<std::string> filesList;

		for (const auto& entry : std::filesystem::recursive_directory_iterator(path)) {
			if (entry.is_directory()) {
				std::string subDirPath = entry.path().string();
				std::map<std::string, std::list<std::string>> subFiles = getFiles(subDirPath);
				filesMap.insert(subFiles.begin(), subFiles.end());
			}
			else {
				std::string filePath = entry.path().string();
				std::string fileDir = entry.path().parent_path().string();
				filesList.push_back(filePath);
				filesMap[fileDir].push_back(filePath);
			}
		}

		return filesMap;
	}


	void textExtractAndSaveXlsx(std::map<std::string, std::list<std::string>> filesMap, std::wstring path) {
		std::vector<std::vector<std::wstring>> data;
		std::map<std::string, std::list<std::string>>::iterator iter;
		std::string filePath;

		TCHAR szPath[MAX_PATH];
		ZeroMemory(szPath, sizeof(szPath));
		BROWSEINFO bi;
		bi.hwndOwner = m_PaintManager.GetPaintWindow();
		bi.pidlRoot = NULL;
		bi.pszDisplayName = szPath;
		bi.lpszTitle = _T("请选择保存目录：");
		bi.ulFlags = 0;
		bi.lpfn = NULL;
		bi.lParam = 0;
		bi.iImage = 0;
		// 弹出选择目录对话框
		LPITEMIDLIST lp = SHBrowseForFolder(&bi);
		if (lp && SHGetPathFromIDList(lp, szPath)) {
			OPENFILENAME ofn;
			TCHAR szFile[260] = _T(".xlsx");
			ZeroMemory(&ofn, sizeof(ofn));
			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = m_PaintManager.GetPaintWindow();
			ofn.lpstrFile = szFile;
			ofn.nMaxFile = sizeof(szFile);
			ofn.lpstrFilter = _T("Excel Files (*.xlsx)\0*.xlsx\0All Files (*.*)\0*.*\0");
			ofn.nFilterIndex = 1;
			ofn.lpstrFileTitle = NULL;
			ofn.nMaxFileTitle = 0;
			ofn.lpstrInitialDir = szPath;
			ofn.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

			if (GetSaveFileName(&ofn) == TRUE) {
				filePath = tchar_to_string(szFile);
				::MessageBox(m_PaintManager.GetPaintWindow(), szFile, _T("保存文件"), MB_OK);
			}
		}
		else {
			::MessageBox(NULL, _T("未选择保存文件"), _T("提示"), NULL);
		}

		lxw_workbook* workbook = workbook_new(filePath.c_str());
		lxw_worksheet* worksheet = workbook_add_worksheet(workbook, NULL);

		// 写入标题行
		for (unsigned int j = 0; j < language.size(); ++j) {
			worksheet_write_string(worksheet, 0, j, wstringToString(language[j]).c_str(), NULL);
		}

		int currentRow = 1; 
		int countfloder = 0, countfile = 0;
		for (const auto& iter : filesMap) {
			countfloder++;
			// 遍历所有目录
			const std::string& folderPath = iter.first;
			const std::list<std::string>& files = iter.second;

			// 在每个文件夹名称之前添加一个空白行
			if (currentRow > 1) { 
				worksheet_write_string(worksheet, currentRow, 0, "", NULL);
				currentRow++;
			}

			// 文件夹名
			lxw_format* format = workbook_add_format(workbook);
			format_set_align(format, LXW_ALIGN_CENTER);
			format_set_bg_color(format, 0xFFEFD5);
			std::string unprefixPath = removeCommonPrefix(folderPath , wstringToString(path)); //长路径 参数1
			std::string mapRelation = getElementTextByPath(XMLFILEPATH, unprefixPath); //define XMLFILEPATH "./config.xml"
			worksheet_merge_range(worksheet, currentRow, 0, currentRow, language.size() - 1, mapRelation.c_str(), format);
			currentRow++;

			if (folderPath.empty()) continue;

			// 遍历同个目录文件	
			int find = 0;
			bool processedFile = false;
			for (const auto& it : files) {
				countfile++;
				int lang = -1;
				std::string file = it;
				for (unsigned int i = 0; i < fileFilter.size(); ++i) {
					if (file.find(fileFilter[i]) != std::string::npos) {
						lang = i;
					}
				}
				if (lang < 0) continue;
				langText.clear();
				dxml p;
				p.loadFile(file.c_str());
				parseElement(p.root());
				if (langText.size()) {
					find = 1;
					for (unsigned int j = 0; j < langText.size(); ++j) {
						worksheet_write_string(worksheet, currentRow + j, lang , wstringToString(langText[j]).c_str() , NULL);
					}
				}
				if (langText.size()) {
					CListLabelElementUI* pItem = new CListLabelElementUI;
					auto wfile = stringToWstring(file);
					pItem->SetText(wfile.c_str());
					m_result->Add(pItem);
				}
			}
			currentRow += langText.size();
		}
		workbook_close(workbook);
	}
	
	std::string getElementTextByPath(const std::string& xmlFilePath, const std::string& elementPath) {
		TiXmlDocument doc;
		if (!doc.LoadFile(xmlFilePath.c_str())) {

			return "";
		}

		TiXmlElement* currentElement = doc.RootElement();
		if (!currentElement) {

			return "";
		}

		std::vector<std::string> pathComponents;
		size_t pos = 0;
		std::string token;
		std::string delimiter = "\\";
		std::string path = elementPath;

		while ((pos = path.find(delimiter)) != std::string::npos) {
			token = path.substr(0, pos);
			pathComponents.push_back(token);
			path.erase(0, pos + delimiter.length());
		}
		pathComponents.push_back(path);

		for (const std::string& component : pathComponents) {
			currentElement = currentElement->FirstChildElement(component.c_str());
			if (!currentElement) {

				return "";
			}
		}

		const char* text = currentElement->GetText();
		if (text) {
			return std::string(text);
		}
		else {

			return "";
		}
	}

	std::string removeCommonPrefix(const std::string& path1, const std::string& path2) {
		std::filesystem::path fsPath1(path1);
		std::filesystem::path fsPath2(path2);

		std::filesystem::path commonPrefix;
		auto it1 = fsPath1.begin();
		auto it2 = fsPath2.begin();

		while (it1 != fsPath1.end() && it2 != fsPath2.end() && *it1 == *it2) {
			commonPrefix /= *it1;
			++it1;
			++it2;
		}

		std::filesystem::path remainingPath1 = fsPath1.lexically_relative(commonPrefix);

		return remainingPath1.string();
	}

	void parseElement(TiXmlNode* pElem, bool print = 0, bool special = 0) {
		if (NULL == pElem) {
			return;
		}
		TiXmlNode* pElement = pElem->FirstChild();
		for (; pElement; pElement = pElement->NextSibling())
		{
			int nType = pElement->Type();
			switch (nType)
			{
			case TiXmlNode::TINYXML_ELEMENT:
			{
				std::string val = pElement->Value();
				if (val == "text") {
					parseElement(pElement, true);
				}
				else {
					bool special_str = false;
					for (unsigned int i = 0; i < sepcial_label.size(); ++i) {
						if (val == sepcial_label[i]) {
							special_str = true;
							break;
						}
					}
					parseElement(pElement, print, special_str);
				}
				if (0 == _stricmp(pElement->Value(), "Property")) {
					std::string strValue = pElement->ToElement()->Attribute("Value");
					size_t pos = strValue.find("set");
					if (std::string::npos != pos) {
					}
				}
			}
			break;
			case TiXmlNode::TINYXML_TEXT:
				if (print || special) {
					std::wstring val = Utils::utf8ToMultiByte(pElement->Value());
					if (!isSkip(val)) {
						langText.push_back(val);
					}
				}
				break;
			default:
				break;
			}
		}
	}

	std::string getLastFolderName(const std::string& path) {
		std::string delimiter = "\\";  // 设置分隔符为 "//"
		std::string::size_type start = 0;
		std::string::size_type end;

		end = path.find_last_of(delimiter);
		if (end != std::string::npos) {
			return path.substr(end + delimiter.length());
		}

		return path;
	}

private:
	bool m_bDragging = false;
	POINT m_ptLastMouse ;
	CEditUI* m_path = nullptr;
	CListUI* m_result = nullptr;
	CButtonUI* m_pMinBtn = nullptr;
	CButtonUI* m_pCloseBtn = nullptr;
	std::wstring importPath;
	std::vector<std::wstring> langText;
};



int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	CPaintManagerUI::SetInstance(hInstance);
	CPaintManagerUI::SetResourcePath(CPaintManagerUI::GetResourcePath());

	langTextExtraFrameWnd* pFrame = new langTextExtraFrameWnd();
	if (pFrame == nullptr) return 0;

	pFrame->Create(NULL, _T("langTextExtra"), UI_WNDSTYLE_FRAME, WS_EX_WINDOWEDGE);
	pFrame->CenterWindow();
	pFrame->ShowWindow(true);

	CPaintManagerUI::MessageLoop();

	delete pFrame;

	return 0;
}