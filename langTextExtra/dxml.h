#ifndef __DXML_H__
#define __DXML_H__

#include <stdio.h>
#include <string.h>

#include "tinyxml/tinyxml.h"

class dxml {
public:
	dxml();
	dxml(const char *body);
	~dxml();

	int getInt(const char *url, int val);
	const char *getText(const char *url);

	int setInt(const char *url, int value);
	int setText(const char *url, const char *value);

	void rm(const char *url);

	const char *data();
	void reset();

	void load(const char *data);
	bool loadFile(const char *url);
	bool saveFile(const char *url);

	TiXmlNode *root(void);

private:
	TiXmlDocument m_doc;
	TiXmlPrinter m_printer;
};

#endif
