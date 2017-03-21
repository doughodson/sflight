
#include "sf/xml/Node.hpp"

#include <algorithm>
#include <iterator>
#include <iostream>

namespace xml
{

Node::Node(std::string tagName)
{
   name = tagName;
}

Node::Node(std::string tagName, std::string text)
{
   name = tagName;
   text = text;
}

Node::Node(const Node& src)
{
   name = src.getTagName();
   text = src.getText();

   int cnt = src.getAttributeCount();
   std::string *arry = new std::string[cnt];
   src.getAttributeNames(arry);

   for (int i = 0; i < cnt; i++)
   {
      putAttribute(arry[i], src.getAttribute(arry[i]));
   }

   cnt = src.getChildCount();

   for (int i = 0; i < cnt; i++)
   {
      Node *tmp = new Node(*src.getChild(i));
      addChild(tmp);
   }

   delete[] arry;
}

Node::~Node()
{
   for (int i = childList.size() - 1; i >= 0; i--)
   {
      delete childList[i];
   }
}

std::string Node::getTagName() const
{
   return name;
}

void Node::setTagName(std::string name)
{
   name = name;
}

Node *Node::addChild(std::string tagName)
{
   Node *child = new Node(tagName);
   return addChild(child);
}

Node *Node::addChild(Node *child)
{
   childList.push_back(child);
   child->setParent(this);
   return child;
}

int Node::getChildCount() const
{
   return childList.size();
}

Node *Node::getChild(int index) const
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
Node *Node::getChild(std::string childName) const
{
   const Node *tmp = this;

   const int splitPt = childName.find_first_of("/");
   std::string tail = "";

   if (splitPt != std::string::npos)
   {
      tail = childName.substr(splitPt + 1);
      childName = childName.substr(0, splitPt);
   }

   for (int i = 0; i < tmp->childList.size(); i++)
   {
      if (tmp->childList[i]->getTagName() == childName)
      {
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
std::vector<Node*> Node::getChildren(std::string childName) const
{
   std::vector<Node*> list;

   const Node *tmp = this;

   const int splitPt = childName.find_first_of("/");
   std::string tail = "";

   if (splitPt != std::string::npos)
   {
      tail = childName.substr(splitPt + 1);
      childName = childName.substr(0, splitPt);
   }

   for (int i = 0; i < tmp->childList.size(); i++)
   {
      if (tmp->childList[i]->getTagName() == childName)
      {
         if (tail != "")
         {
            std::vector<Node *> sublist = childList[i]->getChildren(tail);
            for (int j = 0; j < sublist.size(); j++)
            {
               list.push_back(sublist[j]);
            }
         }
         else
         {
            list.push_back(childList[i]);
         }
      }
   }

   return list;
}

void Node::putAttribute(std::string name, std::string val)
{
   attrMap.insert(std::pair<std::string, std::string>(name, val));
}

std::string Node::getAttribute(const std::string name) const
{
   if (attrMap.count(name) == 1)
   {
      return attrMap.at(name);
   }
   return 0;
}

void Node::getAttributeNames(std::string *storeArray) const
{
   std::map<std::string, std::string>::const_iterator iter;
   int i = 0;

   for (iter = attrMap.begin(); iter != attrMap.end(); ++iter)
   {
      storeArray[i] = iter->first;
      i++;
   }
}

int Node::getAttributeCount() const
{
   return attrMap.size();
}

std::string Node::getText() const
{
   return text;
}

void Node::setText(const std::string x)
{
   text = x;
}

Node *Node::getParent() const
{
   return parentNode;
}

void Node::setParent(Node *x)
{
   parentNode = x;
}

bool Node::remove(Node *node)
{
   std::vector<Node *>::iterator it;

   for (it = childList.begin(); it < childList.end(); it++)
   {

      if (*it == node)
      {
         childList.erase(it);
         return true;
      }
   }
   return false;
}

std::string Node::toString() const
{
   std::string ret = "<" + getTagName() + " ";
   std::string *attrNames = new std::string[getAttributeCount()];
   getAttributeNames(attrNames);

   for (int i = 0; i < getAttributeCount(); i++)
   {
      ret += (attrNames[i] + "=" + getAttribute(attrNames[i]) + " ");
   }
   ret += ">";

   for (int i = 0; i < childList.size(); i++)
   {
      ret += "\n";
      ret += childList[i]->toString();
   }

   if (getText() != "")
   {
      ret += ("\n  " + getText());
   }

   ret += "\n</" + getTagName() + " ";
   +"\n";
   delete[] attrNames;
   return ret;
}
}
