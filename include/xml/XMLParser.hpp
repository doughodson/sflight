#ifndef XMLPARSER_H
#define XMLPARSER_H

#include "Node.hpp"

using std :: string;
using std :: istream;



namespace sf {


    class XMLParser {

    public:

        XMLParser();
        ~XMLParser();


        static Node* parse (istream &reader, bool treatAttributesAsChildren);
        static Node* parse (string filename, bool treatAttributesAsChildren);
        static Node* parseString (string xmlString, bool treatAttributesAsChildren);
        static bool isWhitespace(char ch);

    private:

        static string readChunk(istream &reader);

        static void subChars(string &srcStr);

        static string putAttributes(string str, Node* node, bool treatAsChildren);

        static bool startsWith(string str, string search);

        static bool endsWith(string str, string search);



    };



}
#endif
