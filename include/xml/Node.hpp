
#ifndef NODE_H
#define NODE_H

#include <vector>
#include <map>
#include <iostream>
#include <string>


using std :: string;
using std :: map;
using std :: vector;

namespace SimpleFlight {

    class Node {

    public:

        Node();

        Node( string tagName);

        Node( string name, string text);
        
        Node( Node& );

        ~Node();

        string getTagName();

        void setTagName(string name);

        Node* addChild(string tagName);
        
        Node* addChild(Node* child);

        int getChildCount();
        
        Node* getChild(string childName);

        vector <Node*> getChildren(string childName);
        
        Node* getChild(int index);

        void putAttribute(string name, string val);

        string getAttribute(string name);

        void getAttributeNames(string* storeArray);

        int getAttributeCount();

        string getText();

        void setText( string text);

        Node* getParent();

        void setParent(Node *parentNode);

        string toString();
        
        bool remove(Node* node);


    private:
        
        string name;
        string text;
        map <string, string> attrMap;
        vector <Node*> childList;

        Node* parentNode;

    };

}

#endif
