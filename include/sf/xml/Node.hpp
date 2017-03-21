
#ifndef __Node_H__
#define __Node_H__

#include <vector>
#include <map>
#include <string>

namespace sf {
namespace xml {

//------------------------------------------------------------------------------
// Class: Node
//------------------------------------------------------------------------------
class Node
{
 public:
   Node() = default;
   Node(std::string tagName);
   Node(std::string name, std::string text);
   Node(const Node&);
   virtual ~Node();

   std::string getTagName() const;
   void setTagName(std::string name);

   Node *addChild(std::string tagName);

   Node *addChild(Node *child);

   int getChildCount() const;

   Node *getChild(std::string childName) const;

   std::vector<Node*> getChildren(std::string childName) const;

   Node *getChild(int index) const;

   void putAttribute(std::string name, std::string val);

   std::string getAttribute(const std::string name) const;

   void getAttributeNames(std::string *storeArray) const;

   int getAttributeCount() const;

   std::string getText() const;

   void setText(const std::string text);

   Node *getParent() const;

   void setParent(Node *);

   std::string toString() const;

   bool remove(Node *node);

 private:
   std::string name;
   std::string text;
   std::map<std::string, std::string> attrMap;
   std::vector<Node*> childList {};

   Node* parentNode {};
};
}
}

#endif
