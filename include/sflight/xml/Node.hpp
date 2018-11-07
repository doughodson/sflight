
#ifndef __sflight_xml_Node_HPP__
#define __sflight_xml_Node_HPP__

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
   Node() = delete;
   Node(const std::string& tagName) : tagName(tagName)                                   {}
   Node(const std::string& name, const std::string& text) : tagName(tagName), text(text) {}
   Node(const Node&);
   virtual ~Node();

   void setTagName(const std::string& x)            { tagName = x;  return; }
   std::string getTagName() const                   { return tagName;       }

   Node* addChild(const std::string&);
   Node* addChild(Node* const);

   std::size_t getChildCount() const;

   Node* getChild(const std::string&) const;

   std::vector<Node*> getChildren(const std::string& childName) const;

   Node* getChild(const std::size_t index) const;

   std::string getText() const                      { return text;       }
   void setText(const std::string& x)               { text = x;          }

   Node* getParent() const                          { return parentNode; }
   void setParent(Node* const x)                    { parentNode = x;    }

   std::string toString() const;

   bool remove(Node* const);

 private:
   std::string tagName;
   std::string text;
   // vector of pointers to children Nodes
   std::vector<Node*> childList{};
   // pointer to parent Node
   Node* parentNode{};
};
}
}

#endif
