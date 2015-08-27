/***************************************************************************
                         domutils.cpp  -  description
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

///// Comments ////////////////////////////////////////////////////////////////
/*!
  \class DomUtils
  \brief This is a utility class used to read and make nodes in a QDomDocument.

  Constructor & destructor are declared private so this class remains a pure
  utility class. Only 2 functions are present, but each has a lot of overloads.

  \note Should be changed to use 'pass by reference' instead of the current 
        pointers (which aren't even declared const). This would clean up the
        code a lot.
*/
/// \file
/// Contains the implementation of the class DomUtils

///// Header files ////////////////////////////////////////////////////////////

// Qt header files
#include <qdom.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qvaluelist.h>

// Xbrabo header files
#include <domutils.h>


///////////////////////////////////////////////////////////////////////////////
///// Public Member Functions                                             /////
///////////////////////////////////////////////////////////////////////////////

///// readNode ////////////////////////////////////////////////////////////////
void DomUtils::readNode(QDomNode* node, QString* value)
/// Reads a string from a QDomNode.
{  
  *value = node->toElement().attribute("value").stripWhiteSpace();
}

///// readNode (overloaded) ///////////////////////////////////////////////////
void DomUtils::readNode(QDomNode* node, QStringList* values)
/// Reads a stringlist from a QDomNode.
{
  //if(node->toElement().attribute("dataType") != "xsd:string")
  //  return;
  QDomNode childNode = node->namedItem("array");
  if(!childNode.isNull())
    *values =  QStringList::split(" ",childNode.toElement().text());
}

///// readNode (overloaded) ///////////////////////////////////////////////////
void DomUtils::readNode(QDomNode* node, unsigned int* value)
/// Reads an unsigned int from a QDomNode.
{
  //if(node->toElement().attribute("dataType") != "xsd:unsignedShort")
  //  return;
  *value =  node->toElement().attribute("value","0").toUShort();
}

///// readNode (overloaded) ///////////////////////////////////////////////////
void DomUtils::readNode(QDomNode* node, int* value)
/// Reads an int from a QDomNode.
{
  //if(node->toElement().attribute("dataType") != "xsd:int")
  //  return;
  *value =  node->toElement().attribute("value","0").toInt();
}

///// readNode (overloaded) ///////////////////////////////////////////////////
void DomUtils::readNode(QDomNode* node, float* value)
/// Reads a float from a QDomNode.
{
  //if(node->toElement().attribute("dataType") != "xsd:float")
  //  return;
  *value =  node->toElement().attribute("value","0.0").toFloat();
}

///// readNode (overloaded) ///////////////////////////////////////////////////
void DomUtils::readNode(QDomNode* node, double* value)
/// Reads a float from a QDomNode.
{
  //if(node->toElement().attribute("dataType") != "xsd:double")
  //  return;
  *value =  node->toElement().attribute("value","0.0").toDouble();
}

///// readNode (overloaded) ///////////////////////////////////////////////////
void DomUtils::readNode(QDomNode* node, bool* value)
/// Reads a bool from a QDomNode.
{
  //if(node->toElement().attribute("dataType") != "xsd:boolean")
  //  return;
  *value = node->toElement().attribute("value").stripWhiteSpace() == "true";
}

///// readNode (overloaded) ///////////////////////////////////////////////////
void DomUtils::readNode(QDomNode* node, std::vector<unsigned int>* values)
/// Reads a vector<unsigned int> from a QDomNode.
{
  //if(node->toElement().attribute("dataType") != "xsd:unsigned")
  //  return;  
  values->clear();
  QDomNode childNode = node->namedItem("array");
  if(!childNode.isNull())
  {
    QStringList valueList = QStringList::split(" ",childNode.toElement().text());
    for(QStringList::Iterator it = valueList.begin(); it != valueList.end(); it++)
      values->push_back((*it).toUShort());  
  }
}

///// readNode (overloaded) ///////////////////////////////////////////////////
void DomUtils::readNode(QDomNode* node, std::vector<double>* values)
/// Reads a vector<double> from a QDomNode.
{
  //if(node->toElement().attribute("dataType") != "xsd:float")
  //  return;
  values->clear();
  QDomNode childNode = node->namedItem("array");
  if(!childNode.isNull())
  {
    QStringList valueList = QStringList::split(" ",childNode.toElement().text());
    for(QStringList::Iterator it = valueList.begin(); it != valueList.end(); it++)
      values->push_back((*it).toDouble());
  }
}

///// readNode (overloaded) ///////////////////////////////////////////////////
void DomUtils::readNode(QDomNode* node, std::vector<bool>* values)
/// Reads a vector<bool> from a QDomNode.
{
  //if(node->toElement().attribute("dataType") != "xsd:string")
  //  return;  
  values->clear();
  QDomNode childNode = node->namedItem("array");
  if(!childNode.isNull())
  {
    QStringList valueList = QStringList::split(" ",childNode.toElement().text());
    for(QStringList::Iterator it = valueList.begin(); it != valueList.end(); it++)
      values->push_back(*it == "true");
  }
}

///// readNode (overloaded) ///////////////////////////////////////////////////
void DomUtils::readNode(QDomNode* node, QValueList<int>* values)
/// Reads a QValueList<int> from a QDomNode.
{
  //if(node->toElement().attribute("dataType") != "xsd:int")
  //  return;
  values->clear();
  QDomNode childNode = node->namedItem("array");
  if(!childNode.isNull())
  {
    QStringList valueList = QStringList::split(" ",childNode.toElement().text());
    for(QStringList::Iterator it = valueList.begin(); it != valueList.end(); it++)
      values->push_back((*it).toInt());
  }
}

///// makeNode ////////////////////////////////////////////////////////////////
void DomUtils::makeNode(QDomElement* root, const QString nodeData, const QString dictRef, const QString attributeName, const QString attributeValue)
/// Creates a QDomElement from a string with a dictRef attribute.
/// One extra attribute can be specified.
{
  QDomElement childNode = root->ownerDocument().createElement("parameter");
  if(!dictRef.isEmpty())
    childNode.setAttribute("dictRef", ns + ":" + dictRef);
  ///// xsd:string is the only dataType for parameter
  //else
  //  childNode.setAttribute("dataType", "xsd:string"); // type is only needed when its not specified in the dictionary
  if(!attributeName.isEmpty())
    childNode.setAttribute(attributeName, attributeValue);  
  //QDomText textNode = root->ownerDocument().createTextNode(nodeData);
  childNode.setAttribute("value", nodeData);
  //childNode.appendChild(textNode);
  root->appendChild(childNode);
}

///// makeNode (overloaded) ///////////////////////////////////////////////////
void DomUtils::makeNode(QDomElement* root, const QStringList nodeData, const QString dictRef, const QString attributeName, const QString attributeValue)
/// Creates a QDomElement from a stringlist.
{
  if(nodeData.size() == 0)  // an array should at least contain 1 element
    return;
  QDomElement childNode = root->ownerDocument().createElement("parameter");
  if(!dictRef.isEmpty())
    childNode.setAttribute("dictRef", ns + ":" + dictRef);
  //else
  //  childNode.setAttribute("dataType", "xsd:string");
  if(!attributeName.isEmpty())
    childNode.setAttribute(attributeName, attributeValue);

  QDomElement grandChildNode = root->ownerDocument().createElement("array");
  grandChildNode.setAttribute("size",QString::number(nodeData.size()));
  QDomText textNode = root->ownerDocument().createTextNode(nodeData.join(" "));
  grandChildNode.appendChild(textNode);
  childNode.appendChild(grandChildNode);
  root->appendChild(childNode);
}

///// makeNode (overloaded) ///////////////////////////////////////////////////
void DomUtils::makeNode(QDomElement* root, const unsigned int nodeData, const QString dictRef)
/// Creates a QDomElement from an unsigned int.
{
  QDomElement childNode = root->ownerDocument().createElement("parameter");
  if(!dictRef.isEmpty())
    childNode.setAttribute("dictRef", ns + ":" + dictRef);
  //else
  //  childNode.setAttribute("dataType", "xsd:unsigned");
  //QDomText textNode = root->ownerDocument().createTextNode(QString::number(nodeData));
  childNode.setAttribute("value", nodeData);
  //childNode.appendChild(textNode);
  root->appendChild(childNode);
}

///// makeNode (overloaded) ///////////////////////////////////////////////////
void DomUtils::makeNode(QDomElement* root, const int nodeData, const QString dictRef)
/// Creates a QDomElement from an int.
{
  QDomElement childNode = root->ownerDocument().createElement("parameter");
  if(!dictRef.isEmpty())
    childNode.setAttribute("dictRef", ns + ":" + dictRef);
  //else
  //  childNode.setAttribute("dataType", "xsd:int");
  //QDomText textNode = root->ownerDocument().createTextNode(QString::number(nodeData));
  childNode.setAttribute("value", nodeData);
  //childNode.appendChild(textNode);
  root->appendChild(childNode);
}

///// makeNode (overloaded) ///////////////////////////////////////////////////
void DomUtils::makeNode(QDomElement* root, const float nodeData, const QString dictRef)
/// Creates a QDomElement from a float.
{
  QDomElement childNode = root->ownerDocument().createElement("parameter");
  if(!dictRef.isEmpty())
    childNode.setAttribute("dictRef", ns + ":" + dictRef);
  //else
  //  childNode.setAttribute("dataType", "xsd:float");
  //QDomText textNode = root->ownerDocument().createTextNode(QString::number(nodeData));
  childNode.setAttribute("value", nodeData);
  //childNode.appendChild(textNode);
  root->appendChild(childNode);
}

///// makeNode (overloaded) ///////////////////////////////////////////////////
void DomUtils::makeNode(QDomElement* root, const bool nodeData, const QString dictRef)
/// Creates a QDomElement from a bool.
{
  QDomElement childNode = root->ownerDocument().createElement("parameter");
  if(!dictRef.isEmpty())
    childNode.setAttribute("dictRef", ns + ":" + dictRef);
  //else
  //  childNode.setAttribute("dataType", "xsd:boolean");
  QString text;
  if(nodeData)
    text = "true";
  else
    text = "false";
  //QDomText textNode = root->ownerDocument().createTextNode(text);
  childNode.setAttribute("value", text);
  //childNode.appendChild(textNode);
  root->appendChild(childNode);
}

///// makeNode (overloaded) ///////////////////////////////////////////////////
void DomUtils::makeNode(QDomElement* root, const std::vector<unsigned int> nodeData, const QString dictRef)
/// Creates a QDomElement from a vector<unsigned int>.
{
  if(nodeData.size() == 0)  // an array should at least contain 1 element
    return;
  QDomElement childNode = root->ownerDocument().createElement("parameter");
  if(!dictRef.isEmpty())
    childNode.setAttribute("dictRef", ns + ":" + dictRef);
  //else
  //  childNode.setAttribute("dataType", "xsd:unsigned");
  QDomElement grandChildNode = root->ownerDocument().createElement("array");
  grandChildNode.setAttribute("size",QString::number(nodeData.size()));
  QString text;
  for(std::vector<unsigned int>::const_iterator it = nodeData.begin(); it < nodeData.end(); it++)
    text += " " + QString::number(*it);
  text.stripWhiteSpace();
  QDomText textNode = root->ownerDocument().createTextNode(text);
  grandChildNode.appendChild(textNode);
  childNode.appendChild(grandChildNode);
  root->appendChild(childNode);
}

///// makeNode (overloaded) ///////////////////////////////////////////////////
void DomUtils::makeNode(QDomElement* root, const std::vector<double> nodeData, const QString dictRef, const QString attributeName, const QString attributeValue)
/// Creates a QDomElement from a vector<double>.
{
  if(nodeData.size() == 0)  // an array should at least contain 1 element
    return;
  QDomElement childNode = root->ownerDocument().createElement("parameter");
  if(!dictRef.isEmpty())
    childNode.setAttribute("dictRef", ns + ":" + dictRef);
  //else
  //  childNode.setAttribute("dataType", "xsd:float");
  if(!attributeName.isEmpty())
    childNode.setAttribute(attributeName, attributeValue);    
  
  QDomElement grandChildNode = root->ownerDocument().createElement("array");
  grandChildNode.setAttribute("size",QString::number(nodeData.size()));
  QString text;
  for(std::vector<double>::const_iterator it = nodeData.begin(); it < nodeData.end(); it++)
    text += " " + QString::number(*it, 'f', 12);
  text.stripWhiteSpace();
  QDomText textNode = root->ownerDocument().createTextNode(text);
  grandChildNode.appendChild(textNode);
  childNode.appendChild(grandChildNode);
  root->appendChild(childNode);
}

///// makeNode (overloaded) ///////////////////////////////////////////////////
void DomUtils::makeNode(QDomElement* root, const std::vector<bool> nodeData, const QString dictRef)
/// Creates a QDomElement from a vector<bool>.
{
  if(nodeData.size() == 0)  // an array should at least contain 1 element
    return;
  QDomElement childNode = root->ownerDocument().createElement("parameter");
  if(!dictRef.isEmpty())
    childNode.setAttribute("dictRef", ns + ":" + dictRef);
  //else
  //  childNode.setAttribute("dataType", "xsd:boolean");

  QDomElement grandChildNode = root->ownerDocument().createElement("array");
  grandChildNode.setAttribute("size",QString::number(nodeData.size()));
  QString text;
  for(std::vector<bool>::const_iterator it = nodeData.begin(); it != nodeData.end(); it++)
    text += " " + *it ? "true" : "false";
  text.stripWhiteSpace();
  QDomText textNode = root->ownerDocument().createTextNode(text);
  grandChildNode.appendChild(textNode);
  childNode.appendChild(grandChildNode);
  root->appendChild(childNode);
}

///// makeNode (overloaded) ///////////////////////////////////////////////////
void DomUtils::makeNode(QDomElement* root, const QValueList<int> nodeData, const QString dictRef)
/// Creates a QDomElement from a QValueList<int>.
{
  if(nodeData.size() == 0)  // an array should at least contain 1 element
    return;
  QDomElement childNode = root->ownerDocument().createElement("parameter");
  if(!dictRef.isEmpty())
    childNode.setAttribute("dictRef", ns + ":" + dictRef);
  //else
  //  childNode.setAttribute("dataType", "xsd:int");
  QDomElement grandChildNode = root->ownerDocument().createElement("array");
  grandChildNode.setAttribute("size",QString::number(nodeData.size()));
  QString text;
  for(QValueList<int>::const_iterator it = nodeData.begin(); it != nodeData.end(); it++)
    text += " " + QString::number(*it);
  text.stripWhiteSpace();
  QDomText textNode = root->ownerDocument().createTextNode(text);
  grandChildNode.appendChild(textNode);
  childNode.appendChild(grandChildNode);
  root->appendChild(childNode);
}

///// dictEntry ///////////////////////////////////////////////////////////////
bool DomUtils::dictEntry(QDomNode& node, const QString value)
/// Returns whether a QDomNode is a QDomElement with an attribute dictRef = "ns:prefix_value"
{
  return(node.isElement() && node.toElement().attribute("dictRef") == ns + ":" + value);
}

///////////////////////////////////////////////////////////////////////////////
///// Private Member Functions                                            /////
///////////////////////////////////////////////////////////////////////////////

///// constructor /////////////////////////////////////////////////////////////
DomUtils::DomUtils()
/// The default constructor. Made private so no instantiation is possible.
{
}

///// destructor //////////////////////////////////////////////////////////////
DomUtils::~DomUtils()
/// The default destructor.
{
}

///////////////////////////////////////////////////////////////////////////////
///// Static Variables                                                    /////
///////////////////////////////////////////////////////////////////////////////
const QString DomUtils::uriDict10   = "http://brabosphere.sourceforge.net/dict/1.0";      //< The URI for version 1.0 of the Brabosphere CML dictionary
const QString DomUtils::uriNSCML    = "http://www.xml-cml.org/schema";                    //< The URI for the CML namespace
const QString DomUtils::uriNSDC     = "http://www.dublincore.org/dict";                   //< The URI for the Dublin Core namespace
const QString DomUtils::uriDictCMLM = "http://www.xml-cml.org/dict/cmlMeasured";          //< The URI for the CML Measured dictionary
const QString DomUtils::uriDictXSD = "http://www.w3.org/2001/XMLSchema";                  //< The URI for the XSD dictionary (datatypes)
const QString DomUtils::uriDictAtomic = "http://www.xml-cml.org/units/atomic";            //< The URI for the CML Atomic Units dictionary
const QString DomUtils::uriDictSI = "http://www.xml-cml.org/units/siUnits";               //< The URI for the CML SI Units dictionary

const QString DomUtils::ns = "bs";                //< The namespace prefix for the Brabosphere dictionary
const QString DomUtils::nsCMLM = "cmlm";          //< The namespace prefix for the CML Measured dictionary
const QString DomUtils::nsXSD = "xsd";            //< The namespace prefix for the XSD dictionary
const QString DomUtils::nsAtomic = "atomic";      //< The namespace prefix for the CML Atomic Units dictionary
const QString DomUtils::nsSI = "siUnits";         //< The namespace prefix for the CML SI Units dictionary
const QString DomUtils::nsDC = "dc";              //< The namespace prefix for the Dublin Core dictionary

