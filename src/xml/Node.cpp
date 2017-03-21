
#include "xml/Node.hpp"

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

Node::Node(Node &node)
{
   name = node.getTagName();
   text = node.getText();

   int cnt = node.getAttributeCount();
   std::string *arry = new std::string[cnt];
   node.getAttributeNames(arry);

   for (int i = 0; i < cnt; i++)
   {
      putAttribute(arry[i], node.getAttribute(arry[i]));
   }

   cnt = node.getChildCount();

   for (int i = 0; i < cnt; i++)
   {
      Node *tmp = new Node(*node.getChild(i));
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

std::string Node::getTagName()
{
   return this->name;
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

int Node::getChildCount()
{
   return childList.size();
}

Node *Node::getChild(int index)
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
Node *Node::getChild(std::string childName)
{
   Node *tmp = this;

   int splitPt = childName.find_first_of("/");
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
std::vector<Node *> Node::getChildren(std::string childName)
{

   std::vector<Node *> list;

   Node *tmp = this;

   int splitPt = childName.find_first_of("/");
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

std::string Node::getAttribute(std::string name)
{
   if (attrMap.count(name) == 1)
   {
      return attrMap[name];
   }
   return 0;
}

void Node::getAttributeNames(std::string *storeArray)
{
   std::map<std::string, std::string>::const_iterator iter;
   int i = 0;

   for (iter = attrMap.begin(); iter != attrMap.end(); ++iter)
   {
      storeArray[i] = iter->first;
      i++;
   }
}

int Node::getAttributeCount()
{

   return attrMap.size();
}

std::string Node::getText()
{
   return text;
}

void Node::setText(std::string text)
{
   text = text;
}

Node *Node::getParent()
{
   return parentNode;
}

void Node::setParent(Node *parentNode)
{
   parentNode = parentNode;
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

std::string Node::toString()
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
