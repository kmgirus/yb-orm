
#include <util/xmlnode.h>
#if defined(YB_USE_WX)
#include <wx/mstream.h>
#include <wx/xml/xml.h>
#elif defined(YB_USE_QT)
#include <QtXml>
#else
#include <libxml/tree.h>
#include <libxml/parser.h>
#endif
#include <sstream>

namespace Yb {

namespace ElementTree {

Element::Element(const Yb::String &name, const Yb::String &s)
    : name_(name)
{
    if (s.size())
        text_.push_back(s);
}

ElementPtr
new_element(const Yb::String &name, const Yb::String &s)
{
    return ElementPtr(new Element(name, s));
}

ElementPtr
Element::sub_element(const Yb::String &name, const Yb::String &s)
{
    ElementPtr p = new_element(name, s);
    children_.push_back(p);
    return p;
}

const Yb::String
Element::get_text() const
{
    Yb::String r;
    size_t i = 0;
    for (; i < children_.size(); ++i) {
        if (i < text_.size())
            r += text_[i];
        r += children_[i]->get_text();
    }
    for (; i < text_.size(); ++i)
        r += text_[i];
    return r;
}

void
Element::set_text(const Yb::String &s)
{
    text_.resize(1);
    text_[0] = s;
}

bool
Element::has_attr(const Yb::String &name) const
{
    return attrib_.find(name) != attrib_.end();
}

const Yb::String
Element::get_attr(const Yb::String &name) const
{
    AttribMap::const_iterator i = attrib_.find(name);
    if (i != attrib_.end())
        return i->second;
    return Yb::String();
}

ElementPtr
Element::find_first(const Yb::String &path)
{
    for (size_t i = 0; i < children_.size(); ++i)
        if (path == children_[i]->name_)
            return children_[i];
    throw ElementNotFound(NARROW(path));
}

ElementsPtr
Element::find_all(const Yb::String &path)
{
    ElementsPtr elems(new Elements);
    for (size_t i = 0; i < children_.size(); ++i)
        if (path == children_[i]->name_)
            elems->push_back(children_[i]);
    return elems;
}

void
Element::serialize(Yb::Writer::Document &doc) const
{
    Yb::Writer::Element e(doc, name_);
    AttribMap::const_iterator q = attrib_.begin(),
        qend = attrib_.end();
    for (; q != qend; ++q) {
        Yb::Writer::Attribute a(e, q->first, q->second);
    }
    size_t i = 0;
    for (; i < children_.size(); ++i) {
        if (i < text_.size())
            e.set_content(text_[i]);
        children_[i]->serialize(doc);
    }
    for (; i < text_.size(); ++i)
        e.set_content(text_[i]);
}

const std::string
Element::serialize() const
{
    Yb::Writer::Document doc;
    serialize(doc);
    return doc.end_document();
}

#if defined(YB_USE_WX)
static ElementPtr
convert_node(wxXmlNode *node)
{
    ElementPtr p = new_element(node->GetName());
    for (wxXmlProperty *attr = node->GetProperties();
            attr; attr = attr->GetNext())
    {
        p->attrib_[attr->GetName()] = attr->GetValue();
    }
    for (wxXmlNode *cur = node->GetChildren(); cur; cur = cur->GetNext())
    {
        if (wxXML_ELEMENT_NODE == cur->GetType())
            p->children_.push_back(convert_node(cur));
        else if (wxXML_TEXT_NODE == cur->GetType()) {
            Yb::String value = cur->GetContent();
            if (!Yb::str_empty(value))
                p->text_.push_back(value);
        }
    }
    return p;
}
#elif defined(YB_USE_QT)
static ElementPtr
convert_node(QDomElement node)
{
    ElementPtr p = new_element(node.tagName());
    QDomNamedNodeMap attr = node.attributes();
    for (int i = 0; i < attr.length(); ++i) {
        QDomNode it = attr.item(i);
        p->attrib_[it.nodeName()] = it.nodeValue();
    }
    for (QDomNode cur = node.firstChild(); !cur.isNull(); cur = cur.nextSibling()) {
        QDomElement e = cur.toElement();
        if (!e.isNull())
            p->children_.push_back(convert_node(e));
        else {
            QDomText t = cur.toText();
            if (!t.isNull())
                p->text_.push_back(t.data());
        }
    }
    return p;
}
#else
static Yb::String
get_node_content(xmlNodePtr node)
{
    Yb::String value;
    xmlChar *content = xmlNodeGetContent(node);
    if (content) {
        value = WIDEN((const char *)content);
        xmlFree(content);
    }
    return value;
}

static Yb::String
get_attr(xmlNodePtr node, const xmlChar *name)
{
    Yb::String value;
    xmlChar *prop = xmlGetProp(node, name);
    if (prop) {
        value = WIDEN((const char *)prop);
        xmlFree(prop);
    }
    return value;
}

static ElementPtr
convert_node(xmlNodePtr node)
{
    ElementPtr p = new_element(WIDEN((const char *)node->name));
    for (xmlAttrPtr attr = node->properties; attr; attr = attr->next)
    {
        p->attrib_[WIDEN((const char *)attr->name)] = get_attr(node, attr->name);
    }
    for (xmlNodePtr cur = node->children; cur; cur = cur->next)
    {
        if (XML_ELEMENT_NODE == cur->type)
            p->children_.push_back(convert_node(cur));
        else if (XML_TEXT_NODE == cur->type) {
            Yb::String value = get_node_content(cur);
            if (!Yb::str_empty(value))
                p->text_.push_back(value);
        }
    }
    return p;
}
#endif

ElementPtr
parse(const std::string &content)
{
#if defined(YB_USE_WX)
    wxMemoryInputStream input(content.c_str(), content.size());
    wxXmlDocument doc(input);
    if (!doc.IsOk())
        throw ParseError("parsing XML failed");
    ElementPtr p = convert_node(doc.GetRoot());
    return p;
#elif defined(YB_USE_QT)
    QBuffer buf;
    buf.setData(content.c_str(), content.size());
    QDomDocument doc("xml-data");
    if (!doc.setContent(&buf))
        throw ParseError("parsing XML failed");
    ElementPtr p = convert_node(doc.documentElement());
    return p;
#else
    xmlDocPtr doc = xmlParseMemory(content.c_str(), content.size());
    if (!doc)
        throw ParseError("xmlParseMemory failed");
    xmlNodePtr node = xmlDocGetRootElement(doc);
    ElementPtr p = convert_node(node);
    xmlFreeDoc(doc);
    return p;
#endif
}

ElementPtr
parse(std::istream &inp)
{
    std::ostringstream out;
    while (inp.good()) {
        char c = inp.get();
        if (inp.good())
            out.put(c);
    }
    return parse(out.str());
}

} // end of namespace ElementTree

} // end of namespace Yb

// vim:ts=4:sts=4:sw=4:et:
