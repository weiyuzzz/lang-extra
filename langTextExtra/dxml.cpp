#include "dxml.h"

dxml::dxml()
{
	m_doc.Parse("<?xml version=\"1.0\"?>", 0, TIXML_ENCODING_LEGACY);
	m_printer.SetIndent("	");
}

dxml::dxml(const char *body)
{
	m_doc.Parse(body, 0, TIXML_ENCODING_LEGACY);
	m_printer.SetIndent("	");
}

dxml::~dxml()
{
}

int dxml::getInt(const char *url, int val)
{
	const char *s;

	if ((s = this->getText(url)))
		return atol(s);
	return val;
}

const char *dxml::getText(const char *url)
{
	TiXmlNode *node;

	if ((node = m_doc.FindFirstNode(url)) && node->FirstChild())
		return node->FirstChild()->Value();
	return NULL;
}

int dxml::setInt(const char *url, int value)
{
	TiXmlNode *node;

	if (url == NULL)
		return -1;

	if ((node = m_doc.InsertFirstNode (url))) {
		TiXmlNode *child;
		child = node->IterateChildren(NULL);
		while (child) {
			node->RemoveChild(child);
			child = node->IterateChildren(NULL);
		}
		node->InsertEndChild(TiXmlText(value));
		return 0;
	}
	return -1;
}

int dxml::setText(const char *url, const char *value)
{
	TiXmlNode *node;

	if (url == NULL || value == NULL)
		return -1;

	if ((node = m_doc.InsertFirstNode (url))) {
		TiXmlNode *child;
		child = node->IterateChildren(NULL);
		while (child) {
			node->RemoveChild(child);
			child = node->IterateChildren(NULL);
		}
		if (value != NULL)
			node->InsertEndChild(TiXmlText(value));
		return 0;
	}
	return -1;
}

void dxml::rm(const char *url)
{
	TiXmlNode *node;

	if (url == NULL)
		return;

	if ((node = m_doc.FindFirstNode(url))) {
		TiXmlNode *parent;
		parent = node->Parent();
		if (parent)
			parent->RemoveChild(node);
	}
}

const char *dxml::data()
{
	m_printer.clear();
	m_doc.Accept(&m_printer);
	return m_printer.CStr();
}

void dxml::reset(void)
{
	m_doc.Clear();
//	m_doc.Parse("<?xml version=\"1.0\"?>", 0, TIXML_ENCODING_LEGACY);
}

void dxml::load(const char *data)
{
	m_doc.Clear();
	m_doc.Parse(data, 0, TIXML_ENCODING_LEGACY);
}

bool dxml::loadFile(const char *url)
{
	m_doc.Clear();
	if (m_doc.LoadFile(url, TIXML_ENCODING_LEGACY))
		return true;
	return false;
}

bool dxml::saveFile(const char *url)
{
	return m_doc.SaveFile (url);
}

TiXmlNode *dxml::root(void)
{
	return &m_doc;
}
