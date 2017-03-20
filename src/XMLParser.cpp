
#include "xml/XMLParser.hpp"

#include <fstream>
#include <sstream>
#include <iostream>
#include <string>

using namespace std;

namespace sf {

    XMLParser :: XMLParser() {}

    XMLParser :: ~XMLParser() {}

    /** @brief (one liner)
      *
      * (documentation goes here)
      */
    Node* XMLParser :: parse( string filename, bool treatAttributesAsChildren) {
        ifstream fin( filename.c_str() );

        if ( !fin ) {
            cerr << "could not read file" << filename << endl;
            return new Node("");
        }

        Node* node = parse( fin, treatAttributesAsChildren );
        fin.close();
        return node;
    }
    
    Node* XMLParser :: parseString( string xmlString, bool treatAttributesAsChildren) {
        istringstream fin( xmlString );

        if ( !fin ) {
            cerr << "could not read string" << xmlString << endl;
            return new Node("");
        }

        Node* node = parse( fin, treatAttributesAsChildren );
        return node;
    }


    /** @brief (one liner)
      *
      * (documentation goes here)
      */
    Node* XMLParser::parse( istream &r, bool treatAttributesAsChildren ) {

        Node* rootNode = 0;
        Node* node = 0;

        string str = "";

        while ( true) {
            str = readChunk(r);

            if (str == "")
                break;

            // declaration
            if ( startsWith(str, "<?") ) {
                while ( !endsWith(str, "?") ) {
                    str += readChunk(r);
                }
                continue;
            }

            // comment
            if ( startsWith(str, "<!--")) {
                while ( !endsWith(str, "--")) {
                    str += readChunk(r);
                }
                continue;
            }

            // instruction
            if ( startsWith(str, "<!")) {
                continue;
            }

            // cdata node
            if ( startsWith(str, "<![CDATA[") ) {
                while ( !endsWith(str, "]]") ) {
                    str += readChunk(r);
                }
                continue;
            }

            // if this is a text node or close tag, set the element text
            unsigned int i = str.find("</");
            if ( i != string :: npos && node != 0) {
                node->setText( str.substr(0, i));
                str = str.substr(i + 1);

                    if ( (str.substr(1) == node->getTagName()) && node->getParent() != 0) {
                        node = node->getParent();
                    }

                continue;
            }

            // regular element node
            if ( startsWith(str, "<") ) {

                str = str.substr(1);
                if (rootNode == 0) {
                    rootNode = new Node("");
                    node = rootNode;
                }
                else {
                    Node* tmpNode = node->addChild("");
                    node = tmpNode;
                }

                int splitPt = str.find(" ");
                if ( splitPt != -1) {
                    string tag = str.substr(0, splitPt);
                    node->setTagName(tag);
                    putAttributes( str.substr(splitPt), node, treatAttributesAsChildren);
                }
                else {
                    node->setTagName(str);
                }

                if ( str.rfind("/") == str.length() - 1 && node->getParent() != 0) {
                    node = node->getParent();
                }
            }

        }

        return rootNode;

    }

    /** @brief (one liner)
      *
      * (documentation goes here)
      */
    string XMLParser::readChunk( istream &r ) {

        string buf;
        try {
            int ch = r.get();
            while ( ((char) ch) != '>') {
                if ( ch == -1) {
                    if (buf.length() > 0) {
                        throw 0;
                    }
                    return "";
                }
                if ( !isWhitespace( (char) ch) || buf.length() > 0 ) {
                    buf += (char) ch;
                }
                ch = r.get();
            }

        } catch (int e) {
            return "";
        }

        return buf;
    }

    /** @brief (one liner)
      *
      * (documentation goes here)
      */
    void XMLParser::subChars( string &srcStr ) {

        unsigned int loc = srcStr.find( "&lt" );
        while ( loc != string::npos ) {
            srcStr.replace( loc, 3, "<" );
            loc = srcStr.find( "&lt" );
        }

        loc = srcStr.find( "&gt" );
        while ( loc != string::npos ) {
            srcStr.replace( loc, 3, ">" );
            loc = srcStr.find( "&gt" );
        }

        loc = srcStr.find( "&amp" );
        while ( loc != string::npos ) {
            srcStr.replace( loc, 4, "&" );
            loc = srcStr.find( "&amp" );
        }

        loc = srcStr.find( "&apos" );
        while ( loc != string::npos ) {
            srcStr.replace( loc, 5, "'" );
            loc = srcStr.find( "&apos" );
        }

        loc = srcStr.find( "&quot" );
        while ( loc != string::npos ) {
            srcStr.replace( loc, 5, "\"" );
            loc = srcStr.find( "&quot" );
        }

    }

    /** @brief (one liner)
      *
      * (documentation goes here)
      */
    string XMLParser::putAttributes( string str, Node* node, bool treatAsChildren ) {

        try {
            while ( str.length() > 0 ) {

                while ( isWhitespace( str[ 0 ] ) ) {
                    str = str.substr( 1, str.length() - 1 );
                }
                unsigned int nameEnd = str.find( "=\"", 0 );
                if ( nameEnd == string :: npos )
                    return str;

                int attrEnd = str.find( "\"", nameEnd + 3 );
                if ( attrEnd == string :: npos )
                    throw 1;

                string name = str.substr( 0, nameEnd );
                string attr = str.substr( nameEnd + 2, attrEnd - nameEnd - 2 );

                //subChars( name );
                //subChars( attr );
				if (treatAsChildren) {
					Node* tmp = node->addChild(name);
					tmp->setText(attr);	
				} 
				else {
                	node->putAttribute( name, attr );
				}
                str = str.substr( attrEnd + 1 );

            }
        }
        catch ( int e ) {
            cerr << "error" << endl;
        }
        return str;

    }


    /** @brief (one liner)
    *
    * (documentation goes here)
    */
    bool XMLParser :: isWhitespace( char ch ) {

        if ( ch == '\t' || ch == '\n' || ch == '\r' || ch == ' ' ) {
            return true;
        }
        return false;
    }

    bool XMLParser :: startsWith(string str, string search) {

            return  (str.find(search, 0) == 0);
    }

    bool XMLParser :: endsWith(string str, string search) {
            int searchLimit = str.length() - search.length();
            return ( str.rfind(search, searchLimit) == searchLimit );
    }

}

