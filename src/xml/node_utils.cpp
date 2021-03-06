
#include "sflight/xml/node_utils.hpp"

#include "sflight/xml/Node.hpp"
#include "sflight/xml/parser_utils.hpp"

#include <cctype>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

namespace sflight {
namespace xml {

// returns a list of nodes that contain the childName
std::vector<Node*> getList(Node* const parent, const std::string& childName)
{
   if (!parent) {
      return std::vector<Node*>();
   }

   return parent->getChildren(childName);
}

std::string getString(Node* const parent, const std::string& pathName, const std::string& defaultVal)
{
   if (!parent) {
      return defaultVal;
   }

   Node* node{parent->getChild(pathName)};
   if (!node) {
      return defaultVal;
   }

   return node->getText();
}

int getInt(Node* const parent, const std::string& pathName, const int defaultVal)
{
   if (!parent) {
      return defaultVal;
   }

   Node* node{parent->getChild(pathName)};

   if (!node) {
      return defaultVal;
   }

   return std::atoi(node->getText().c_str());
}

long getLong(Node* const parent, const std::string& pathName, const long defaultVal)
{
   if (!parent) {
      return defaultVal;
   }

   Node* node{parent->getChild(pathName)};

   if (!node) {
      return defaultVal;
   }

   return std::atol(node->getText().c_str());
}

float getFloat(Node* const parent, const std::string& pathName, const float defaultVal)
{
   if (!parent) {
      return defaultVal;
   }

   Node* node{parent->getChild(pathName)};

   if (!node) {
      return defaultVal;
   }

   return static_cast<float>(std::atof(node->getText().c_str()));
}

double getDouble(Node* const parent, const std::string& pathName, const double defaultVal)
{
   if (!parent) {
      return defaultVal;
   }

   Node* node{parent->getChild(pathName)};

   if (!node) {
      return defaultVal;
   }

   const double val{std::atof(node->getText().c_str())};
   return val;
}

bool getBool(Node* const parent, const std::string& pathName, const bool defaultVal)
{
   if (!parent) {
      return defaultVal;
   }

   Node* node{parent->getChild(pathName)};
   if (!node) {
      return defaultVal;
   }

   std::string text{node->getText()};
   for (std::size_t i = 0; i < text.length(); i++) {
      text[i] = std::toupper(text[i]);
   }

   if (text == "TRUE")
      return true;
   if (text == "FALSE")
      return false;

   return defaultVal;
}

std::vector<std::string> splitString(const std::string& inStr, const char splitChar)
{
   std::vector<std::string> retV;

   const char* charStr{inStr.c_str()};
   const size_t len{inStr.length()};

   std::size_t i{};
   while (i < len) {
      // trim leading delimeters
      while (i < len && (charStr[i] == splitChar || isWhitespace(charStr[i]))) {
         i++;
      }

      std::size_t startLoc{i};
      i++;

      while (i < len && charStr[i] != splitChar && !isWhitespace(charStr[i])) {
         i++;
      }

      if (i > len) {
         break;
      }

      retV.push_back(inStr.substr(startLoc, i - startLoc));
   }

   return retV;
}
}
}
