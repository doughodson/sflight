
#include "sflight/xml/Node.hpp"

#include <algorithm>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace sflight {
namespace xml {

Node::Node(const Node& src)
{
   tagName = src.tagName;
   text = src.text;

   // copy all children
   std::size_t cnt = src.getChildCount();
   for (std::size_t i = 0; i < cnt; i++) {
      Node* tmp{new Node(*src.getChild(i))};
      addChild(tmp);
   }
}

Node::~Node()
{
   for (std::size_t i = childList.size() - 1; i >= 0; i--) {
      delete childList[i];
   }
}

Node* Node::addChild(const std::string& x)
{
   Node* child{new Node(x)};
   return addChild(child);
}

Node* Node::addChild(Node* const x)
{
   childList.push_back(x);
   x->setParent(this);
   return x;
}

std::size_t Node::getChildCount() const { return childList.size(); }

Node* Node::getChild(const std::size_t index) const
{
   if (index < getChildCount())
      return childList[index];

   return 0;
}

//
// Returns the first child encountered with specified name, or null
// if none is found.  To find a nested child, specify the childname
// as tags separated by "/"
//
Node* Node::getChild(const std::string& x) const
{
   std::string childName{x};
   const Node* tmp{this};

   const std::size_t splitPt{childName.find_first_of("/")};
   std::string tail;

   if (splitPt != std::string::npos) {
      tail = childName.substr(splitPt + 1);
      childName = childName.substr(0, splitPt);
   }

   for (std::size_t i = 0; i < tmp->childList.size(); i++) {
      if (tmp->childList[i]->getTagName() == childName) {
         if (tail != "")
            return childList[i]->getChild(tail);
         else
            return childList[i];
      }
   }
   return nullptr;
}

//
// Returns a vector containing all children encountered with specified name, or null
// if none are found.
//
std::vector<Node*> Node::getChildren(const std::string& x) const
{
   std::string childName{x};
   std::vector<Node*> list;

   const Node* tmp{this};

   const std::size_t splitPt{childName.find_first_of("/")};
   std::string tail;

   if (splitPt != std::string::npos) {
      tail = childName.substr(splitPt + 1);
      childName = childName.substr(0, splitPt);
   }

   for (std::size_t i = 0; i < tmp->childList.size(); i++) {
      if (tmp->childList[i]->getTagName() == childName) {
         if (tail != "") {
            std::vector<Node*> sublist{childList[i]->getChildren(tail)};
            for (std::size_t j = 0; j < sublist.size(); j++) {
               list.push_back(sublist[j]);
            }
         } else {
            list.push_back(childList[i]);
         }
      }
   }

   return list;
}

bool Node::remove(Node* const node)
{
   std::vector<Node*>::iterator it;

   for (it = childList.begin(); it < childList.end(); it++) {

      if (*it == node) {
         childList.erase(it);
         return true;
      }
   }
   return false;
}

std::string Node::toString() const
{
   std::string ret{"<" + getTagName() + ">"};
   for (std::size_t i = 0; i < childList.size(); i++) {
      ret += "\n";
      ret += childList[i]->toString();
   }

   if (getText() != "") {
      ret += ("\n" + getText());
   }

   ret += "\n</" + getTagName() + ">";
   return ret;
}

}
}
