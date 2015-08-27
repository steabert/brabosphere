/***************************************************************************
                          domutils.h  -  description
                             -------------------
    begin                : Thu Feb 20 2003
    copyright            : (C) 2003-2006 by Ben Swerts
    email                : bswerts@users.sourceforge.net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

/// \file
/// Contains the declaration of the class DomUtils.

#ifndef DOMUTILS_H
#define DOMUTILS_H

///// Forward class declarations & header files ///////////////////////////////

// STL header files
#include <vector>

// Qt forward class declarations
class QDomElement;
class QDomNode;
class QString;
class QStringList;
template<class T> class QValueList;

///// class DomUtils //////////////////////////////////////////////////////////
class DomUtils
{
  public:
    static void readNode(QDomNode* node, QString* value);   // reads the value of a node into a QString
    static void readNode(QDomNode* node, QStringList* value);         // overloaded
    static void readNode(QDomNode* node, unsigned int* value);        // overloaded
    static void readNode(QDomNode* node, int* value);       // overloaded
    static void readNode(QDomNode* node, float* value);     // overloaded
    static void readNode(QDomNode* node, double* value);    // overloaded
    static void readNode(QDomNode* node, bool* value);      // overloaded
    static void readNode(QDomNode* node, std::vector<unsigned int>* values);         // overloaded
    static void readNode(QDomNode* node, std::vector<double>* values); // overloaded
    static void readNode(QDomNode* node, std::vector<bool>* values); // overloaded
    static void readNode(QDomNode* node, QValueList<int>* values);     // overloaded        
    static void makeNode(QDomElement* root, const QString nodeData, const QString dictRef = 0,const QString attributeName = 0, const QString attributeValue = 0); // adds a node to root of type string with a title and another attribute
    static void makeNode(QDomElement* root, const QStringList nodeData, const QString dictRef = 0,const QString attributeName = 0, const QString attributeValue = 0);       // overloaded
    static void makeNode(QDomElement* root, const unsigned int nodeData, const QString dictRef);      // overloaded
    static void makeNode(QDomElement* root, const int nodeData, const QString dictRef);     // overloaded
    static void makeNode(QDomElement* root, const float nodeData, const QString dictRef);   // overloaded
    static void makeNode(QDomElement* root, const bool nodeData, const QString dictRef);    // overloaded
    static void makeNode(QDomElement* root, const std::vector<unsigned int> nodeData, const QString dictRef);  // overloaded
    static void makeNode(QDomElement* root, const std::vector<double> nodeData, const QString dictRef = QString::null,const QString attributeName = QString::null, const QString attributeValue = QString::null);  // overloaded
    static void makeNode(QDomElement* root, const std::vector<bool> nodeData, const QString dictRef);  // overloaded
    static void makeNode(QDomElement* root, const QValueList<int> nodeData, const QString dictRef);   // overloaded
    static bool dictEntry(QDomNode& node, const QString value);       //Returns whether the node has a dictRef attribute with the namespaced contents of value 

    static const QString uriDict10;     // The URI for the Brabosphere dictionary version 1.0
    static const QString uriNSCML;      // The URI for the CML namespace
    static const QString uriNSDC;       // The URI for the Dublin Core namespace
    static const QString uriDictCMLM;   // The URI for the CML Measured dictionary
    static const QString uriDictXSD;    // The URI for the XSD dictionary (datatypes)
    static const QString uriDictAtomic; // The URI for the CML Atomic Units dictionary
    static const QString uriDictSI;     // The URI for the CML SI Units dictionary
    static const QString ns;            // The namespace prefix for the Brabosphere dictionary
    static const QString nsCMLM;        // The namespace prefix for the CML Measured dictionary
    static const QString nsXSD;         // The namespace prefix for the XSD dictionary
    static const QString nsAtomic;      // The namespace prefix for the Atomic Units dictionary
    static const QString nsSI;          // The namespace prefix for the SI Units dictionary
    static const QString nsDC;          // The namespace prefix for the Dublin Core dictionary

  private:
    DomUtils();                         // constructor
    ~DomUtils();                        // destructor  
};

#endif

