
#ifndef __Node_H__
#define __Node_H__

#include <vector>
#include <map>
#include <iostream>
#include <string>

namespace sf {

//------------------------------------------------------------------------------
// Class: Node
//------------------------------------------------------------------------------
class Node
{
public:
   Node();
   Node( std::string tagName);
   Node( std::string name, std::string text);
   Node( Node& );
   ~Node();

   std::string getTagName();
   void setTagName(std::string name);

   Node* addChild(std::string tagName);

   Node* addChild(Node* child);

   int getChildCount();

   Node* getChild(std::string childName);

   std::vector<Node*> getChildren(std::string childName);

   Node* getChild(int index);

   void putAttribute(std::string name, std::string val);

   std::string getAttribute(std::string name);

   void getAttributeNames(std::string* storeArray);

   int getAttributeCount();

   std::string getText();

   void setText(std::string text);

   Node* getParent();

   void setParent(Node *parentNode);

   std::string toString();

   bool remove(Node* node);

private:
   std::string name;
   std::string text;
   std::map<std::string, std::string> attrMap;
   std::vector<Node*> childList {};

   Node* parentNode {};
};

}

#endif
