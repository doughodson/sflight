
#include "sflight/xml/parser_utils.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace sflight {
namespace xml {

Node* parse(const std::string filename, const bool treatAttributesAsChildren)
{
   std::ifstream fin(filename.c_str());

   if (!fin) {
      std::cerr << "could not read file" << filename << std::endl;
      return new Node("");
   }

   Node* node = parse(fin, treatAttributesAsChildren);
   fin.close();
   return node;
}

Node* parseString(const std::string& xmlString, const bool treatAttributesAsChildren)
{
   std::istringstream fin(xmlString);

   if (!fin) {
      std::cerr << "could not read string" << xmlString << std::endl;
      return new Node("");
   }

   Node* node = parse(fin, treatAttributesAsChildren);
   return node;
}

Node* parse(std::istream& r, const bool treatAttributesAsChildren)
{
   Node* rootNode = nullptr;
   Node* node = nullptr;

   std::string str = "";

   while (true) {
      str = readChunk(r);

      if (str == "")
         break;

      // declaration
      if (startsWith(str, "<?")) {
         while (!endsWith(str, "?")) {
            str += readChunk(r);
         }
         continue;
      }

      // comment
      if (startsWith(str, "<!--")) {
         while (!endsWith(str, "--")) {
            str += readChunk(r);
         }
         continue;
      }

      // instruction
      if (startsWith(str, "<!")) {
         continue;
      }

      // cdata node
      if (startsWith(str, "<![CDATA[")) {
         while (!endsWith(str, "]]")) {
            str += readChunk(r);
         }
         continue;
      }

      // if this is a text node or close tag, set the element text
      unsigned int i = str.find("</");
      if (i != std::string::npos && node != 0) {
         node->setText(str.substr(0, i));
         str = str.substr(i + 1);

         if ((str.substr(1) == node->getTagName()) && node->getParent() != 0) {
            node = node->getParent();
         }

         continue;
      }

      // regular element node
      if (startsWith(str, "<")) {

         str = str.substr(1);
         if (rootNode == 0) {
            rootNode = new Node("");
            node = rootNode;
         } else {
            Node* tmpNode = node->addChild("");
            node = tmpNode;
         }

         int splitPt = str.find(" ");
         if (splitPt != -1) {
            std::string tag = str.substr(0, splitPt);
            node->setTagName(tag);
            putAttributes(str.substr(splitPt), node, treatAttributesAsChildren);
         } else {
            node->setTagName(str);
         }

         if (str.rfind("/") == str.length() - 1 && node->getParent() != 0) {
            node = node->getParent();
         }
      }
   }

   return rootNode;
}

bool isWhitespace(const char ch)
{
   if (ch == '\t' || ch == '\n' || ch == '\r' || ch == ' ') {
      return true;
   }
   return false;
}

std::string readChunk(std::istream& r)
{
   std::string buf;
   try {
      int ch = r.get();
      while (((char)ch) != '>') {
         if (ch == -1) {
            if (buf.length() > 0) {
               throw 0;
            }
            return "";
         }
         if (!isWhitespace(static_cast<char>(ch)) || buf.length() > 0) {
            buf += static_cast<char>(ch);
         }
         ch = r.get();
      }
   } catch (int e) {
      return "";
   }

   return buf;
}

void subChars(std::string& srcStr)
{
   unsigned int loc = srcStr.find("&lt");
   while (loc != std::string::npos) {
      srcStr.replace(loc, 3, "<");
      loc = srcStr.find("&lt");
   }

   loc = srcStr.find("&gt");
   while (loc != std::string::npos) {
      srcStr.replace(loc, 3, ">");
      loc = srcStr.find("&gt");
   }

   loc = srcStr.find("&amp");
   while (loc != std::string::npos) {
      srcStr.replace(loc, 4, "&");
      loc = srcStr.find("&amp");
   }

   loc = srcStr.find("&apos");
   while (loc != std::string::npos) {
      srcStr.replace(loc, 5, "'");
      loc = srcStr.find("&apos");
   }

   loc = srcStr.find("&quot");
   while (loc != std::string::npos) {
      srcStr.replace(loc, 5, "\"");
      loc = srcStr.find("&quot");
   }
}

std::string putAttributes(std::string str, Node* node, const bool treatAsChildren)
{
   try {
      while (str.length() > 0) {
         while (isWhitespace(str[0])) {
            str = str.substr(1, str.length() - 1);
         }
         unsigned int nameEnd = str.find("=\"", 0);
         if (nameEnd == std::string::npos)
            return str;

         int attrEnd = str.find("\"", nameEnd + 3);
         if (attrEnd == std::string::npos)
            throw 1;

         std::string name = str.substr(0, nameEnd);
         std::string attr = str.substr(nameEnd + 2, attrEnd - nameEnd - 2);

         // subChars( name );
         // subChars( attr );
         if (treatAsChildren) {
            Node* tmp = node->addChild(name);
            tmp->setText(attr);
         } else {
            node->putAttribute(name, attr);
         }
         str = str.substr(attrEnd + 1);
      }
   } catch (int e) {
      std::cerr << "error" << std::endl;
   }
   return str;
}

bool startsWith(const std::string str, const std::string search)
{
   return (str.find(search, 0) == 0);
}

bool endsWith(const std::string str, const std::string search)
{
   int searchLimit = str.length() - search.length();
   return (str.rfind(search, searchLimit) == searchLimit);
}
}
}
