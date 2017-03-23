
#ifndef __sflight_xml_Node_H__
#define __sflight_xml_Node_H__

#include <map>
#include <string>
#include <vector>

namespace sflight {
namespace xml {

//------------------------------------------------------------------------------
// Class: Node
//------------------------------------------------------------------------------
class Node {
 public:
   Node() = default;
   Node(const std::string& tagName);
   Node(const std::string& name, const std::string& text);
   Node(const Node&);
   virtual ~Node();

   std::string getTagName() const;
   void setTagName(const std::string&);

   Node* addChild(const std::string&);

   Node* addChild(Node* const);

   int getChildCount() const;

   Node* getChild(const std::string&) const;

   std::vector<Node*> getChildren(const std::string& childName) const;

   Node* getChild(const int index) const;

   void putAttribute(std::string name, std::string val);

   std::string getAttribute(const std::string name) const;

   void getAttributeNames(std::string* const) const;

   int getAttributeCount() const;

   std::string getText() const;

   void setText(const std::string& text);

   Node* getParent() const;

   void setParent(Node* const);

   std::string toString() const;

   bool remove(Node* const);

 private:
   std::string name;
   std::string text;
   std::map<std::string, std::string> attrMap;
   std::vector<Node*> childList{};

   Node* parentNode{};
};
}
}

#endif
