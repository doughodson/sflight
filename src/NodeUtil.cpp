
#include "xml/NodeUtil.hpp"
#include "xml/Node.hpp"
#include "xml/XMLParser.hpp"

#include <vector>
#include <string>
#include <iostream>

using namespace std;


namespace SimpleFlight {
    
    /** returns a list of nodes that contain the childName */
    vector <Node*> NodeUtil :: getList(Node* parent, string childName) {
        
        if (parent == 0 ) return vector <Node*> ();
        
        return parent->getChildren(childName);
    }
    
    string NodeUtil :: get( Node* parent, string pathName, string defaultVal) {
        
        if ( parent == 0 ) return defaultVal;
        
        Node* node = parent->getChild(pathName);
        
        if ( node == 0 ) return defaultVal;
        
        return node->getText();
    }
    
    int NodeUtil :: getInt(Node* parent, string pathName, int defaultVal) {
        
        if ( parent == 0 ) return defaultVal;
        
        Node* node = parent->getChild(pathName);
        
        if ( node == 0 ) return defaultVal;
        
        return atoi(node->getText().c_str());
    }
    
    long NodeUtil :: getLong(Node* parent, string pathName, long defaultVal) {
        
        if ( parent == 0 ) return defaultVal;
        
        Node* node = parent->getChild(pathName);
        
        if ( node == 0 ) return defaultVal;
        
        return atol(node->getText().c_str());
    }
    
    double NodeUtil :: getDouble(Node* parent, string pathName, double defaultVal) {
        
        if ( parent == 0 ) return defaultVal;
        
        Node* node = parent->getChild(pathName);
        
        if ( node == 0 ) return defaultVal;
        
        double val = atof(node->getText().c_str());
        
        return atof(node->getText().c_str());
    }
    
    float NodeUtil :: getFloat(Node* parent, string pathName, float defaultVal) {
        
        if ( parent == 0 ) return defaultVal;
        
        Node* node = parent->getChild(pathName);
        
        if ( node == 0 ) return defaultVal;
        
        return (float) atof(node->getText().c_str());
    }
    
    bool NodeUtil :: getBool(Node* parent, string pathName, bool defaultVal) {
        
        if ( parent == 0 ) return defaultVal;
        
        Node* node = parent->getChild(pathName);
        if ( node == 0 ) return defaultVal;
        
        string text = node->getText();
        
        for(int i=0; i<text.length(); i++) {
            text[i] = toupper(text[i]);
        }
        
        if ( text == "TRUE") return true;
        if ( text == "FALSE") return false;
        
        return defaultVal;
    }
    
    vector<string> NodeUtil :: splitString(string inStr, char splitChar) {
        
        vector<string> retV;
        
        int startLoc = 0;
        
        const char* charStr = inStr.c_str();
        int len = inStr.length();
        
        int i = 0;
        while( i < len ) {
            
            //trim leading delimeters
            while( i < len && ( charStr[i] == splitChar || XMLParser::isWhitespace(charStr[i])) ) {
                i++;
            }
            
            startLoc = i;
            i++;
            
            while( i < len && charStr[i] != splitChar && !XMLParser::isWhitespace(charStr[i]) ) {
                i++;
            }
            
            if ( i > len) break;
            
            retV.push_back( inStr.substr(startLoc, i - startLoc));
            
        }
        
        return retV;
        
    }
    
}

