
#include "xml/Node.hpp"
#include <algorithm>
#include <iterator>
#include <iostream>
#include "xml/NodeUtil.hpp"

using namespace std;

namespace SimpleFlight {
    
    Node :: Node() {
        this->name = "";
        this->text = "";
        this->parentNode = 0;
    }
    
    Node :: Node(string tagName) {
        this->name = tagName;
        this->text = "";
        this->parentNode = 0;
    }
    
    Node :: Node(string tagName, string text) {
        this->name = tagName;
        this->text = text;
        this->parentNode = 0;
    }
    
    Node :: Node( Node& node) {
        
        name = node.getTagName();
        text = node.getText();
        
        int cnt = node.getAttributeCount();
        string* arry = new string[cnt] ;
        node.getAttributeNames(arry);
        
        for(int i=0; i<cnt; i++) {
            putAttribute(arry[i], node.getAttribute(arry[i]));
        }
        
        cnt = node.getChildCount();
        
        for(int i=0; i<cnt; i++) {
            Node* tmp = new Node( *node.getChild(i) );
            addChild( tmp );
        }
        
        delete [] arry;
        
    }
    
    Node :: ~Node() {
        for(int i=childList.size()-1; i>=0; i--) {
            delete childList[i];
        }
    }
    
    
    /** @brief (one liner)
     *
     * (documentation goes here)
     */
    string Node::getTagName(){
        return this->name;
    }
    
    /** @brief (one liner)
     *
     * (documentation goes here)
     */
    void Node::setTagName(string name) {
        this->name = name;
    }
    
    /** @brief (one liner)
     *
     * (documentation goes here)
     */
    Node* Node::addChild(string tagName) {
        Node* child = new Node(tagName);
        return addChild(child);
    }
    
    Node* Node :: addChild(Node* child) {
        childList.push_back(child);
        child->setParent( this );
        return child;
    }
    
    /** @brief (one liner)
     *
     * (documentation goes here)
     */
    int Node :: getChildCount() {
        return childList.size();
    }
    
    Node* Node :: getChild(int index) {
        if (index < getChildCount())
            return childList[index];
        
        return 0;
    }
    
    
    /**
     * Returns the first child encountered with specified name, or null
     * if none is found.  To find a nested child, specify the childname
     * as tags separated by "/"
     */
    Node* Node :: getChild(string childName) {
        
        Node* tmp = this;
        
        int splitPt = childName.find_first_of("/");
        string tail = "";
        
        if (splitPt != string::npos) {
            tail = childName.substr(splitPt + 1);
            childName = childName.substr(0, splitPt);
        }
        
        for(int i=0; i<tmp->childList.size(); i++) {
            if ( tmp->childList[i]->getTagName() == childName){
                if (tail != "")
                    return childList[i]->getChild(tail);
                else
                    return childList[i];
            }
        }
        return 0;
    }

    
    /**
     * Returns a vector containing all children encountered with specified name, or null
     * if none are found.
     */
    vector<Node*> Node :: getChildren(string childName) {

        vector <Node*> list;

        Node* tmp = this;
        
        int splitPt = childName.find_first_of("/");
        string tail = "";
        
        if (splitPt != string::npos) {
            tail = childName.substr(splitPt + 1);
            childName = childName.substr(0, splitPt);
        }
        
        for(int i=0; i<tmp->childList.size(); i++) {
            if ( tmp->childList[i]->getTagName() == childName){
                if (tail != "") {
                    vector <Node*> sublist = childList[i]->getChildren(tail);
                    for(int j=0; j<sublist.size(); j++) {
                        list.push_back( sublist[j]);
                    }
                } else {
                    list.push_back( childList[i]);
                }
            }
        }
        
        return list;
        
    }
    
    
    /** @brief (one liner)
     *
     * (documentation goes here)
     */
    void Node::putAttribute(string name, string val) {
        attrMap.insert( pair<string, string> (name, val) );
    }
    
    /** @brief (one liner)
     *
     * (documentation goes here)
     */
    string Node::getAttribute(string name) {
        if ( attrMap.count(name) == 1) {
            return attrMap[name];
        }
        return 0;
    }
    
    /** @brief (one liner)
     *
     * (documentation goes here)
     */
    void Node::getAttributeNames( string* storeArray ) {
        map<string, string> :: const_iterator iter;
        int i=0;
        
        for( iter = attrMap.begin(); iter != attrMap.end(); ++iter) {
            storeArray[i] =  iter->first;
            i++;
        }
    }
    
    /** @brief (one liner)
     *
     * (documentation goes here)
     */
    int Node :: getAttributeCount(){
        
        return this->attrMap.size();
    }
    
    /** @brief (one liner)
     *
     * (documentation goes here)
     */
    string Node::getText(){
        return this->text;
    }
    
    /** @brief (one liner)
     *
     * (documentation goes here)
     */
    void Node::setText(string text) {
        this->text = text;
    }
    
    /** @brief (one liner)
     *
     * (documentation goes here)
     */
    Node* Node::getParent(){
        return this->parentNode;
    }
    
    /** @brief (one liner)
     *
     * (documentation goes here)
     */
    void Node::setParent(Node *parentNode) {
        this->parentNode = parentNode;
    }
    
    bool Node :: remove(Node* node) {
        
        vector <Node*> :: iterator it;
        
        for( it = childList.begin(); it<childList.end(); it++) {
            
            if ( *it == node ) {
                childList.erase(it);
                return true;
            }
        }
        return false;
    }
    
    /** @brief (one liner)
     *
     * (documentation goes here)
     */
    string Node::toString() {
        string ret = "<" + getTagName() + " ";
        string* attrNames = new string[getAttributeCount()];
        getAttributeNames( attrNames );
        
        for(int i=0; i<getAttributeCount(); i++) {
            ret += (attrNames[i] + "=" + getAttribute(attrNames[i]) + " ");
        }
        ret += ">";
        
        for(int i=0; i<childList.size(); i++) {
            ret += "\n";
            ret += childList[i]->toString();
        }
        
        if (getText() != "") {
            ret += ("\n  " + getText() );
        }
        
        ret += "\n</" + getTagName() + " "; + "\n";
        delete [] attrNames;
        return ret;
    }
    
    
}
