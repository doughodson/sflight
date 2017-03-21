
#include "xml/NodeUtil.hpp"
#include "xml/Node.hpp"
#include "xml/XMLParser.hpp"

#include <vector>
#include <string>
#include <iostream>
#include <cmath>
#include <cctype>

namespace sf
{

/** returns a list of nodes that contain the childName */
std::vector<Node *> NodeUtil::getList(Node *parent, std::string childName)
{

   if (parent == nullptr)
      return std::vector<Node *>();

   return parent->getChildren(childName);
}

std::string NodeUtil::get(Node *parent, std::string pathName, std::string defaultVal)
{

   if (parent == nullptr)
      return defaultVal;

   Node *node = parent->getChild(pathName);

   if (node == nullptr)
      return defaultVal;

   return node->getText();
}

int NodeUtil::getInt(Node *parent, std::string pathName, int defaultVal)
{

   if (parent == nullptr)
      return defaultVal;

   Node *node = parent->getChild(pathName);

   if (node == nullptr)
      return defaultVal;

   return std::atoi(node->getText().c_str());
}

long NodeUtil::getLong(Node *parent, std::string pathName, long defaultVal)
{

   if (parent == nullptr)
      return defaultVal;

   Node *node = parent->getChild(pathName);

   if (node == nullptr)
      return defaultVal;

   return std::atol(node->getText().c_str());
}

double NodeUtil::getDouble(Node *parent, std::string pathName, double defaultVal)
{

   if (parent == nullptr)
      return defaultVal;

   Node *node = parent->getChild(pathName);

   if (node == nullptr)
      return defaultVal;

   double val = std::atof(node->getText().c_str());

   return std::atof(node->getText().c_str());
}

float NodeUtil::getFloat(Node *parent, std::string pathName, float defaultVal)
{

   if (parent == nullptr)
      return defaultVal;

   Node *node = parent->getChild(pathName);

   if (node == nullptr)
      return defaultVal;

   return (float)atof(node->getText().c_str());
}

bool NodeUtil::getBool(Node *parent, std::string pathName, bool defaultVal)
{

   if (parent == nullptr)
      return defaultVal;

   Node *node = parent->getChild(pathName);
   if (node == nullptr)
      return defaultVal;

   std::string text = node->getText();

   for (int i = 0; i < text.length(); i++)
   {
      text[i] = std::toupper(text[i]);
   }

   if (text == "TRUE")
      return true;
   if (text == "FALSE")
      return false;

   return defaultVal;
}

std::vector<std::string> NodeUtil::splitString(std::string inStr, char splitChar)
{
   std::vector<std::string> retV;

   int startLoc = 0;

   const char *charStr = inStr.c_str();
   int len = inStr.length();

   int i = 0;
   while (i < len)
   {

      //trim leading delimeters
      while (i < len && (charStr[i] == splitChar || XMLParser::isWhitespace(charStr[i])))
      {
         i++;
      }

      startLoc = i;
      i++;

      while (i < len && charStr[i] != splitChar && !XMLParser::isWhitespace(charStr[i]))
      {
         i++;
      }

      if (i > len)
         break;

      retV.push_back(inStr.substr(startLoc, i - startLoc));
   }

   return retV;
}
}
