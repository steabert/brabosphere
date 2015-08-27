/***************************************************************************
                        relaxbase.cpp  -  description
                             -------------------
    begin                : Mon Jun 16 2003
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
  \class RelaxBase
  \brief This class allows generation of an input file for the program Relax.

  It provides the functionality to RelaxWidget.

*/
/// \file
/// Contains the implementation of the class RelaxBase.

///// Header files ////////////////////////////////////////////////////////////

// C++ header files
#include <algorithm>
#include <cassert>
#include <cmath>

// Qt header files
#include <qapplication.h>
#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qcombobox.h>
#include <qdir.h>
#include <qdom.h>
#include <qfiledialog.h>
#include <qheader.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qlistbox.h>
#include <qlistview.h>
#include <qmessagebox.h>
#include <qprocess.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qspinbox.h>
#include <qtable.h>
#include <qtextedit.h>
#include <qtoolbutton.h>
#include <qvalidator.h>
#include <qwidgetstack.h>

// Xbrabo header files
#include "atomset.h"
#include "crdfactory.h"
#include "domutils.h"
#include "iconsets.h"
#include "paths.h"
#include "relaxbase.h"
#include "textviewwidget.h"
#include "utils.h"
#include "version.h"

///////////////////////////////////////////////////////////////////////////////
///// Public Member Functions                                             /////
///////////////////////////////////////////////////////////////////////////////

///// Constructor /////////////////////////////////////////////////////////////
RelaxBase::RelaxBase(AtomSet* atomset, QWidget* parent, const char* name, bool modal, WFlags fl) : RelaxWidget(parent, name, modal, fl),
  atoms(atomset)
/// The default constructor.
{
  assert(atomset != 0);
  
  makeConnections();
  init();
}

///// Destructor //////////////////////////////////////////////////////////////
RelaxBase::~RelaxBase()
/// The default destructor.
{

}

///// setName /////////////////////////////////////////////////////////////////
void RelaxBase::setName(const QString name)
/// Sets the filename prefix of the calculation. This is the basename from
/// which all input and output files will be named.
{
  calcName = name;
}

///// setDescription //////////////////////////////////////////////////////////
void RelaxBase::setDescription(const QString description)
/// Sets the description of the calculation.
{
  calcDescription = description;
}

///// setDir //////////////////////////////////////////////////////////////////
void RelaxBase::setDir(const QString dir)
/// Sets the directory of the calculation.
{
  calcDir = dir;
}

///// setExtendedFormat ///////////////////////////////////////////////////////
void RelaxBase::setExtendedFormat(const bool state)
/// Sets the format of the coordinates (normal/extended).
{
  calcXF = state;
}

///// inputGenerationFrequency ////////////////////////////////////////////////
unsigned int RelaxBase::inputGenerationFrequency()
/// Returns the frequency with which the internal
/// coordinates should be regenerated. 0 means just once at the beginning.
{
  if(!CheckBoxAutogen->isChecked())
    return 0; // no regeneration if manually defined
  else if(!CheckBoxRegen->isChecked())
    return 0; // no regeneration
  
  return SpinBoxGenFreq->value(); // regeneration frequency
}

///// maxSteps ////////////////////////////////////////////////////////////////
unsigned int RelaxBase::maxSteps()
/// Returns the maximum number of optimization steps
/// to be done. 0 means no limit.
{
  return SpinBoxSteps->value();
}

///// scaleFactors ////////////////////////////////////////////////////////////
void RelaxBase::scaleFactors(vector<unsigned int>& steps, vector<double>& factors)
/// Returns the scalefactors.
/// Get it from the data struct (easier).
{
  steps.clear();
  factors.clear();

  if(data.useOneScaleFactor)
  {
    steps.push_back(0);
    factors.push_back(data.sfacValue1.toDouble());
  }
  else
  {
    steps.reserve(data.listNumSteps.size());
    steps.assign(data.listNumSteps.begin(), data.listNumSteps.end());
    for(QStringList::iterator it = data.listValues.begin(); it != data.listValues.end(); it++)
      factors.push_back((*it).toDouble());
  }

  qDebug("RelaxBase::scaleFactors:");
  for(unsigned int i = 0; i < steps.size(); i++)
    qDebug(" %d, step %d, factor %f", i, steps[i], factors[i]);
}
  
///// generateInput ///////////////////////////////////////////////////////////
QStringList RelaxBase::generateInput(const Input type)
/// Generates input for a specific program.
/// \arg AFF: complete .aff in case of manual input or just the header if using MAFF
/// \arg MAFF: input for a call to the program MAFF
{  
  QStringList result;  
  if(type == RelaxBase::AFF)
    return generateAFFHeader();
  else if(type == RelaxBase::MAFF)     
  {
    if(!CheckBoxAutogen->isChecked())
      return QStringList(); // this indicates that the aff returned by generateInput(AFF) is the complete one

    result += calcName; // Identification code
    if(calcXF)
      result += "y"; // use extended format
    else
      result += "n"; 
    switch(ComboBoxType->currentItem())
    {
      case 0: // GBMA
        result += "a"; // GBMA
        result += "y"; // automatically generate the bond list
        result += "n"; // add extra bonds
        if(CheckBoxHBonds->isChecked())
          result += "y"; // add H-bonds      
        else
          result += "n";                
        result += "n"; // check all atoms connect ? new option ?
        break;
      case 1: // BMAT
        result += "b";   // A or B => bmat
        result += "y";   // Use standard force constants
        result += "y";   // Use diagonal force constant matrix
        result += "n";   // Input op fort.4
        result += generateBmatInput();
        break;
    }
  }
  return result;
}

///// loadCML /////////////////////////////////////////////////////////////////
void RelaxBase::loadCML(const QDomElement* root)
/// Reads the widgetdata from a QDomElement.
{ 
  ///// This function will normally never be called while the dialog is showing
  ///// => the data struct is always up to date with the contents of the widgets
  assert(!isVisible());

  const QString prefix = "geometry_optimization_";
  QDomNode childNode = root->firstChild();
  while(!childNode.isNull())
  {
    if(childNode.isElement() && childNode.toElement().tagName() == "parameter")
    {
      ///// Basic
      if(DomUtils::dictEntry(childNode, prefix + xml.type))
        DomUtils::readNode(&childNode, &data.type);
      else if(DomUtils::dictEntry(childNode, prefix + xml.autoGenerate))
        DomUtils::readNode(&childNode, &data.autoGenerate);
      else if(DomUtils::dictEntry(childNode, prefix + xml.autoRegen))
        DomUtils::readNode(&childNode, &data.autoRegen);
      else if(DomUtils::dictEntry(childNode, prefix + xml.autoGenFreq))
        DomUtils::readNode(&childNode, &data.autoGenFreq);
      else if(DomUtils::dictEntry(childNode, prefix + xml.autoHBonds))
        DomUtils::readNode(&childNode, &data.autoHBonds);
      else if(DomUtils::dictEntry(childNode, prefix + xml.useOneScaleFactor))
        DomUtils::readNode(&childNode, &data.useOneScaleFactor);
      else if(DomUtils::dictEntry(childNode, prefix + xml.sfacValue1))
        DomUtils::readNode(&childNode, &data.sfacValue1);
      else if(DomUtils::dictEntry(childNode, prefix + xml.sfacValue2))
        DomUtils::readNode(&childNode, &data.sfacValue2);
      else if(DomUtils::dictEntry(childNode, prefix + xml.sfacNumSteps))
        DomUtils::readNode(&childNode, &data.sfacNumSteps);
      else if(DomUtils::dictEntry(childNode, prefix + xml.numDIIS))
        DomUtils::readNode(&childNode, &data.numDIIS);
      else if(DomUtils::dictEntry(childNode, prefix + xml.printShifts))
        DomUtils::readNode(&childNode, &data.printShifts);
      else if(DomUtils::dictEntry(childNode, prefix + xml.numSteps))
        DomUtils::readNode(&childNode, &data.numSteps);

      else if(DomUtils::dictEntry(childNode, prefix + xml.listValues))
        DomUtils::readNode(&childNode, &data.listValues);
      else if(DomUtils::dictEntry(childNode, prefix + xml.listNumSteps))
        DomUtils::readNode(&childNode, &data.listNumSteps);

      ///// Internal coordinates
      else if(DomUtils::dictEntry(childNode, prefix + xml.ic_number))
      {
        vector<unsigned int> numbers;
        DomUtils::readNode(&childNode, &numbers);
        data.ic.resize(numbers.size());
        for(unsigned int i = 0; i < data.ic.size(); i++)
          data.ic[i].number = numbers[i];
      }
      else if(DomUtils::dictEntry(childNode, prefix + xml.ic_rootItem))
      {
        vector<bool> rootItems;
        DomUtils::readNode(&childNode, &rootItems);
        data.ic.resize(rootItems.size());
        for(unsigned int i = 0; i < data.ic.size(); i++)
          data.ic[i].rootItem = rootItems[i];
      }
      else if(DomUtils::dictEntry(childNode, prefix + xml.ic_type))
      {
        QStringList types;
        DomUtils::readNode(&childNode, &types);
        data.ic.resize(types.size());
        for(unsigned int i = 0; i < data.ic.size(); i++)
          data.ic[i].type = types[i];
      }
      else if(DomUtils::dictEntry(childNode, prefix + xml.ic_fc))
      {
        QStringList fcs;
        DomUtils::readNode(&childNode, &fcs);
        data.ic.resize(fcs.size());
        for(unsigned int i = 0; i < data.ic.size(); i++)
          data.ic[i].fc = fcs[i];
      }
      else if(DomUtils::dictEntry(childNode, prefix + xml.ic_fix))
      {
        vector<bool> fixed;
        DomUtils::readNode(&childNode, &fixed);
        data.ic.resize(fixed.size());
        for(unsigned int i = 0; i < data.ic.size(); i++)
          data.ic[i].fix = fixed[i];
      }
      else if(DomUtils::dictEntry(childNode, prefix + xml.ic_maximize))
      {
        vector<bool> maximized;
        DomUtils::readNode(&childNode, &maximized);
        data.ic.resize(maximized.size());
        for(unsigned int i = 0; i < data.ic.size(); i++)
          data.ic[i].maximize = maximized[i];
      }
      else if(DomUtils::dictEntry(childNode, prefix + xml.ic_refValue))
      {
        QStringList refValues;
        DomUtils::readNode(&childNode, &refValues);
        data.ic.resize(refValues.size());
        for(unsigned int i = 0; i < data.ic.size(); i++)
          data.ic[i].refValue = refValues[i];
      }
      else if(DomUtils::dictEntry(childNode, prefix + xml.ic_weight))
      {
        QStringList weights;
        DomUtils::readNode(&childNode, &weights);
        data.ic.resize(weights.size());
        for(unsigned int i = 0; i < data.ic.size(); i++)
         data.ic[i].weight = weights[i];
      }
      else if(DomUtils::dictEntry(childNode, prefix + xml.ic_atom1))
      {
        vector<unsigned int> firstAtoms;
        DomUtils::readNode(&childNode, &firstAtoms);
        data.ic.resize(firstAtoms.size());
        for(unsigned int i = 0; i < data.ic.size(); i++)
          data.ic[i].atom1 = firstAtoms[i];
      }
      else if(DomUtils::dictEntry(childNode, prefix + xml.ic_atom2))
      {
        vector<unsigned int> secondAtoms;
        DomUtils::readNode(&childNode, &secondAtoms);
        data.ic.resize(secondAtoms.size());
        for(unsigned int i = 0; i < data.ic.size(); i++)
          data.ic[i].atom2 = secondAtoms[i];
      }
      else if(DomUtils::dictEntry(childNode, prefix + xml.ic_atom3))
      {
        vector<unsigned int> thirdAtoms;
        DomUtils::readNode(&childNode, &thirdAtoms);
        data.ic.resize(thirdAtoms.size());
        for(unsigned int i = 0; i < data.ic.size(); i++)
          data.ic[i].atom3 = thirdAtoms[i];
      }
      else if(DomUtils::dictEntry(childNode, prefix + xml.ic_atom4))
      {
        vector<unsigned int> fourthAtoms;
        DomUtils::readNode(&childNode, &fourthAtoms);
        data.ic.resize(fourthAtoms.size());
        for(unsigned int i = 0; i < data.ic.size(); i++)
          data.ic[i].atom4 = fourthAtoms[i];
      }

      ///// Cartesian coordinates - Symmetry
      else if(DomUtils::dictEntry(childNode, prefix + xml.checkSymmetry))
        DomUtils::readNode(&childNode, &data.checkSymmetry);
      else if(DomUtils::dictEntry(childNode, prefix + xml.checkSymmX))
        DomUtils::readNode(&childNode, &data.checkSymmX);
      else if(DomUtils::dictEntry(childNode, prefix + xml.checkSymmY))
        DomUtils::readNode(&childNode, &data.checkSymmY);
      else if(DomUtils::dictEntry(childNode, prefix + xml.checkSymmZ))
        DomUtils::readNode(&childNode, &data.checkSymmZ);
      else if(DomUtils::dictEntry(childNode, prefix + xml.checkSymmXY))
        DomUtils::readNode(&childNode, &data.checkSymmXY);
      else if(DomUtils::dictEntry(childNode, prefix + xml.checkSymmXZ))
        DomUtils::readNode(&childNode, &data.checkSymmXZ);
      else if(DomUtils::dictEntry(childNode, prefix + xml.checkSymmYZ))
        DomUtils::readNode(&childNode, &data.checkSymmYZ);
      else if(DomUtils::dictEntry(childNode, prefix + xml.checkSymmXYZ))
        DomUtils::readNode(&childNode, &data.checkSymmXYZ);

      ///// Cartesian coordinates - Other
      else if(DomUtils::dictEntry(childNode, prefix + xml.fixAtom))
        DomUtils::readNode(&childNode, &data.fixAtom);
      else if(DomUtils::dictEntry(childNode, prefix + xml.fixX))
        DomUtils::readNode(&childNode, &data.fixX);
      else if(DomUtils::dictEntry(childNode, prefix + xml.fixY))
        DomUtils::readNode(&childNode, &data.fixY);
      else if(DomUtils::dictEntry(childNode, prefix + xml.fixZ))
        DomUtils::readNode(&childNode, &data.fixZ);
      else if(DomUtils::dictEntry(childNode, prefix + xml.optExternal))
        DomUtils::readNode(&childNode, &data.optExternal);
      else if(DomUtils::dictEntry(childNode, prefix + xml.optExternalA))
        DomUtils::readNode(&childNode, &data.optExternalA);
      else if(DomUtils::dictEntry(childNode, prefix + xml.optExternalB))
        DomUtils::readNode(&childNode, &data.optExternalB);
      else if(DomUtils::dictEntry(childNode, prefix + xml.optExternalC))
        DomUtils::readNode(&childNode, &data.optExternalC);
      else if(DomUtils::dictEntry(childNode, prefix + xml.optExternalAlpha))
        DomUtils::readNode(&childNode, &data.optExternalAlpha);
      else if(DomUtils::dictEntry(childNode, prefix + xml.optExternalBeta))
        DomUtils::readNode(&childNode, &data.optExternalBeta);
      else if(DomUtils::dictEntry(childNode, prefix + xml.optExternalGamma))
        DomUtils::readNode(&childNode, &data.optExternalGamma);
      else if(DomUtils::dictEntry(childNode, prefix + xml.mirrorImage))
        DomUtils::readNode(&childNode, &data.mirrorImage);
      else if(DomUtils::dictEntry(childNode, prefix + xml.writeXYZ))
        DomUtils::readNode(&childNode, &data.writeXYZ);

      else if(DomUtils::dictEntry(childNode, prefix + xml.listFixedAtoms))
        DomUtils::readNode(&childNode, &data.listFixedAtoms);
      else if(DomUtils::dictEntry(childNode, prefix + xml.listFixedXYZ))
        DomUtils::readNode(&childNode, &data.listFixedXYZ);

      ///// Advanced - Thresholds
      else if(DomUtils::dictEntry(childNode, prefix + xml.refineThre1))
        DomUtils::readNode(&childNode, &data.refineThre1);
      else if(DomUtils::dictEntry(childNode, prefix + xml.refineThre2))
        DomUtils::readNode(&childNode, &data.refineThre2);
      else if(DomUtils::dictEntry(childNode, prefix + xml.refineThre3))
        DomUtils::readNode(&childNode, &data.refineThre3);
      else if(DomUtils::dictEntry(childNode, prefix + xml.refineThre4))
        DomUtils::readNode(&childNode, &data.refineThre4);
      else if(DomUtils::dictEntry(childNode, prefix + xml.refineThre5))
        DomUtils::readNode(&childNode, &data.refineThre5);
      else if(DomUtils::dictEntry(childNode, prefix + xml.maxIter))
        DomUtils::readNode(&childNode, &data.maxIter);
      else if(DomUtils::dictEntry(childNode, prefix + xml.iterThre1))
        DomUtils::readNode(&childNode, &data.iterThre1);
      else if(DomUtils::dictEntry(childNode, prefix + xml.iterThre2))
        DomUtils::readNode(&childNode, &data.iterThre2);
      else if(DomUtils::dictEntry(childNode, prefix + xml.iterThre3))
        DomUtils::readNode(&childNode, &data.iterThre3);
      else if(DomUtils::dictEntry(childNode, prefix + xml.iterThre4))
        DomUtils::readNode(&childNode, &data.iterThre4);

      ///// Advanced - Other
      else if(DomUtils::dictEntry(childNode, prefix + xml.xbonAtom1))
        DomUtils::readNode(&childNode, &data.xbonAtom1);
      else if(DomUtils::dictEntry(childNode, prefix + xml.xbonAtom2))
        DomUtils::readNode(&childNode, &data.xbonAtom2);
      else if(DomUtils::dictEntry(childNode, prefix + xml.massType))
        DomUtils::readNode(&childNode, &data.massType);
      else if(DomUtils::dictEntry(childNode, prefix + xml.setLowFreqs))
        DomUtils::readNode(&childNode, &data.setLowFreqs);
      else if(DomUtils::dictEntry(childNode, prefix + xml.lowFreqs))
        DomUtils::readNode(&childNode, &data.lowFreqs);
      else if(DomUtils::dictEntry(childNode, prefix + xml.orden))
        DomUtils::readNode(&childNode, &data.orden);

      else if(DomUtils::dictEntry(childNode, prefix + xml.listXbonAtoms1))
        DomUtils::readNode(&childNode, &data.listXbonAtoms1);
      else if(DomUtils::dictEntry(childNode, prefix + xml.listXbonAtoms2))
        DomUtils::readNode(&childNode, &data.listXbonAtoms2);

      ///// Debug
      else if(DomUtils::dictEntry(childNode, prefix + xml.goon))
        DomUtils::readNode(&childNode, &data.goon);
      else if(DomUtils::dictEntry(childNode, prefix + xml.printDebug))
        DomUtils::readNode(&childNode, &data.printDebug);
      else if(DomUtils::dictEntry(childNode, prefix + xml.update))
        DomUtils::readNode(&childNode, &data.update);
      else if(DomUtils::dictEntry(childNode, prefix + xml.updateType))
        DomUtils::readNode(&childNode, &data.updateType);
      else if(DomUtils::dictEntry(childNode, prefix + xml.updateScaleFactor))
        DomUtils::readNode(&childNode, &data.updateScaleFactor);
      else if(DomUtils::dictEntry(childNode, prefix + xml.compact))
        DomUtils::readNode(&childNode, &data.compact);
      else if(DomUtils::dictEntry(childNode, prefix + xml.compactBonds))
        DomUtils::readNode(&childNode, &data.compactBonds);
      else if(DomUtils::dictEntry(childNode, prefix + xml.compactAngles))
        DomUtils::readNode(&childNode, &data.compactAngles);
      else if(DomUtils::dictEntry(childNode, prefix + xml.compactTorsions))
        DomUtils::readNode(&childNode, &data.compactTorsions);

      ///// Extra
      else if(DomUtils::dictEntry(childNode, prefix + xml.numLines))
        DomUtils::readNode(&childNode, &data.numLines);
      else if(DomUtils::dictEntry(childNode, prefix + xml.hPos))
        DomUtils::readNode(&childNode, &data.hPos);
      else if(DomUtils::dictEntry(childNode, prefix + xml.vPos))
        DomUtils::readNode(&childNode, &data.vPos);
      else if(DomUtils::dictEntry(childNode, prefix + xml.contents))
        DomUtils::readNode(&childNode, &data.contents);
    }
    childNode = childNode.nextSibling();
  }
  restoreWidgets();
  widgetChanged = false;
}

///// saveCML /////////////////////////////////////////////////////////////////
void RelaxBase::saveCML(QDomElement* root)
/// Saves the widgetdata to a QDomElement.
{  
  ///// This function will normally never be called while the dialog is showing
  ///// => the data struct is always up to date with the contents of the widgets
  assert(!isVisible());
  
  const QString prefix = "geometry_optimization_";
  ///// Basic
  DomUtils::makeNode(root, data.type, prefix + xml.type);
  DomUtils::makeNode(root, data.autoGenerate, prefix + xml.autoGenerate);
  DomUtils::makeNode(root, data.autoRegen, prefix + xml.autoRegen);    
  DomUtils::makeNode(root, data.autoGenFreq, prefix + xml.autoGenFreq);
  DomUtils::makeNode(root, data.autoHBonds, prefix + xml.autoHBonds);
  DomUtils::makeNode(root, data.useOneScaleFactor, prefix + xml.useOneScaleFactor);
  DomUtils::makeNode(root, data.sfacValue1, prefix + xml.sfacValue1);
  DomUtils::makeNode(root, data.sfacValue2, prefix + xml.sfacValue2);
  DomUtils::makeNode(root, data.sfacNumSteps, prefix + xml.sfacNumSteps);
  DomUtils::makeNode(root, data.listValues, prefix + xml.listValues);
  DomUtils::makeNode(root, data.listNumSteps, prefix + xml.listNumSteps);
  DomUtils::makeNode(root, data.numDIIS, prefix + xml.numDIIS);
  DomUtils::makeNode(root, data.printShifts, prefix + xml.printShifts);
  DomUtils::makeNode(root, data.numSteps, prefix + xml.numSteps);

  ///// Internal coordinates
  if(data.ic.size() != 0)
  {
    // fill vectors/QStringLists with the data to store
    vector<unsigned int> numbers;
    vector<bool> rootItems;
    QStringList types;
    QStringList fcs;
    vector<bool> fixed;
    vector<bool> maximized;
    QStringList refValues;
    QStringList weights;
    vector<unsigned int> firstAtoms;
    vector<unsigned int> secondAtoms;
    vector<unsigned int> thirdAtoms;
    vector<unsigned int> fourthAtoms;
    for(vector<ICData>::iterator it = data.ic.begin(); it != data.ic.end(); it++)
    {
      numbers.push_back(it->number);
      rootItems.push_back(it->rootItem);
      types.push_back(it->type);
      fcs.push_back(it->fc);
      fixed.push_back(it->fix);
      maximized.push_back(it->maximize);
      refValues.push_back(it->refValue);
      weights.push_back(it->weight);
      firstAtoms.push_back(it->atom1);
      secondAtoms.push_back(it->atom2);
      thirdAtoms.push_back(it->atom3);
      fourthAtoms.push_back(it->atom4);
    }
    // store them as arrays
    DomUtils::makeNode(root, numbers, prefix + xml.ic_number);
    DomUtils::makeNode(root, rootItems, prefix + xml.ic_rootItem);
    DomUtils::makeNode(root, types, prefix + xml.ic_type);
    DomUtils::makeNode(root, fcs, prefix + xml.ic_fc);
    DomUtils::makeNode(root, fixed, prefix + xml.ic_fix);
    DomUtils::makeNode(root, maximized, prefix + xml.ic_maximize);
    DomUtils::makeNode(root, refValues, prefix + xml.ic_refValue);
    DomUtils::makeNode(root, weights, prefix + xml.ic_weight);
    DomUtils::makeNode(root, firstAtoms, prefix + xml.ic_atom1);
    DomUtils::makeNode(root, secondAtoms, prefix + xml.ic_atom2);
    DomUtils::makeNode(root, thirdAtoms, prefix + xml.ic_atom3);
    DomUtils::makeNode(root, fourthAtoms, prefix + xml.ic_atom4);
  }

  ///// Cartesian coordinates - Symmetry
  DomUtils::makeNode(root, data.checkSymmetry, prefix + xml.checkSymmetry);
  DomUtils::makeNode(root, data.checkSymmX, prefix + xml.checkSymmX);
  DomUtils::makeNode(root, data.checkSymmY, prefix + xml.checkSymmY);
  DomUtils::makeNode(root, data.checkSymmZ, prefix + xml.checkSymmZ);
  DomUtils::makeNode(root, data.checkSymmXY, prefix + xml.checkSymmXY);
  DomUtils::makeNode(root, data.checkSymmXZ, prefix + xml.checkSymmXZ);
  DomUtils::makeNode(root, data.checkSymmYZ, prefix + xml.checkSymmYZ);
  DomUtils::makeNode(root, data.checkSymmXYZ, prefix + xml.checkSymmXYZ);

  ///// Cartesian coordinates - Other
  DomUtils::makeNode(root, data.fixAtom, prefix + xml.fixAtom);
  DomUtils::makeNode(root, data.fixX, prefix + xml.fixX);
  DomUtils::makeNode(root, data.fixY, prefix + xml.fixY);
  DomUtils::makeNode(root, data.fixZ, prefix + xml.fixZ);
  DomUtils::makeNode(root, data.listFixedAtoms, prefix + xml.listFixedAtoms);
  DomUtils::makeNode(root, data.listFixedXYZ, prefix + xml.listFixedXYZ);
  DomUtils::makeNode(root, data.optExternal, prefix + xml.optExternal);
  DomUtils::makeNode(root, data.optExternalAngle, prefix + xml.optExternalAngle);
  DomUtils::makeNode(root, data.optExternalA, prefix + xml.optExternalA);
  DomUtils::makeNode(root, data.optExternalB, prefix + xml.optExternalB);
  DomUtils::makeNode(root, data.optExternalC, prefix + xml.optExternalC);
  DomUtils::makeNode(root, data.optExternalAlpha, prefix + xml.optExternalAlpha);
  DomUtils::makeNode(root, data.optExternalBeta, prefix + xml.optExternalBeta);
  DomUtils::makeNode(root, data.optExternalGamma, prefix + xml.optExternalGamma);
  DomUtils::makeNode(root, data.mirrorImage, prefix + xml.mirrorImage);
  DomUtils::makeNode(root, data.writeXYZ, prefix + xml.writeXYZ);
      
  ///// Advanced - Thresholds
  DomUtils::makeNode(root, data.refineThre1, prefix + xml.refineThre1);
  DomUtils::makeNode(root, data.refineThre2, prefix + xml.refineThre2);
  DomUtils::makeNode(root, data.refineThre3, prefix + xml.refineThre3);
  DomUtils::makeNode(root, data.refineThre4, prefix + xml.refineThre4);
  DomUtils::makeNode(root, data.refineThre5, prefix + xml.refineThre5);
  DomUtils::makeNode(root, data.maxIter, prefix + xml.maxIter);
  DomUtils::makeNode(root, data.iterThre1, prefix + xml.iterThre1);
  DomUtils::makeNode(root, data.iterThre2, prefix + xml.iterThre2);
  DomUtils::makeNode(root, data.iterThre3, prefix + xml.iterThre3);
  DomUtils::makeNode(root, data.iterThre4, prefix + xml.iterThre4);

  ///// Advanced - Other
  DomUtils::makeNode(root, data.xbonAtom1, prefix + xml.xbonAtom1);
  DomUtils::makeNode(root, data.xbonAtom2, prefix + xml.xbonAtom2);
  DomUtils::makeNode(root, data.listXbonAtoms1, prefix + xml.listXbonAtoms1);
  DomUtils::makeNode(root, data.listXbonAtoms2, prefix + xml.listXbonAtoms2);
  DomUtils::makeNode(root, data.massType, prefix + xml.massType);
  DomUtils::makeNode(root, data.setLowFreqs, prefix + xml.setLowFreqs);
  DomUtils::makeNode(root, data.lowFreqs, prefix + xml.lowFreqs);
  DomUtils::makeNode(root, data.orden, prefix + xml.orden);
      
  ///// Debug
  DomUtils::makeNode(root, data.goon, prefix + xml.goon);
  DomUtils::makeNode(root, data.printDebug, prefix + xml.printDebug);
  DomUtils::makeNode(root, data.update, prefix + xml.update);
  DomUtils::makeNode(root, data.updateType, prefix + xml.updateType);
  DomUtils::makeNode(root, data.updateScaleFactor, prefix + xml.updateScaleFactor);
  DomUtils::makeNode(root, data.compact, prefix + xml.compact);
  DomUtils::makeNode(root, data.compactBonds, prefix + xml.compactBonds);
  DomUtils::makeNode(root, data.compactAngles, prefix + xml.compactAngles);
  DomUtils::makeNode(root, data.compactTorsions, prefix + xml.compactTorsions);

  ///// Extra
  qDebug("calling makeNode with data.numLines = %d and prefix = "+prefix+xml.numLines, data.numLines);
  DomUtils::makeNode(root, data.numLines, prefix + xml.numLines);
  DomUtils::makeNode(root, data.hPos, prefix + xml.hPos);
  DomUtils::makeNode(root, data.vPos, prefix + xml.vPos);
  DomUtils::makeNode(root, data.contents, prefix + xml.contents);
}

///////////////////////////////////////////////////////////////////////////////
///// Public Slots                                                        /////
///////////////////////////////////////////////////////////////////////////////

///// reset ///////////////////////////////////////////////////////////////////
void RelaxBase::reset()
/// Resets all widgets to their default values.
{
  ///// Basic
  ComboBoxType->setCurrentItem(0);
  CheckBoxAutogen->setChecked(true);
  CheckBoxRegen->setChecked(false);
  SpinBoxGenFreq->setValue(1);
  CheckBoxHBonds->setChecked(true);
  RadioButtonSFAC1->setChecked(true);
  RadioButtonSFAC2->setChecked(false);
  LineEditSFAC1->setText("1.0");
  LineEditSFAC2->setText("1.0");
  SpinBoxSFAC->setValue(5);
  ListViewSFAC->clear();
  SpinBoxMAXD->setValue(5);
  CheckBoxSUMM->setChecked(true);
  SpinBoxSteps->setValue(atoms->count() < 4 ? 10 : 3 * atoms->count()); // 3N steps with a minimum of 10
  enableSFACWidgets();

  ///// Internal coordinates
  ListViewIC->clear();
  TextLabelICNumber->setText("1");
  ComboBoxICType->setCurrentItem(0);
  LineEditFC->setText("5.0");
  CheckBoxFIXC->setChecked(false);
  CheckBoxMAXI->setChecked(false);
  CheckBoxTORO->setChecked(false);
  LineEditTORO->setText("");
  TextLabelICPart->setText("1");
  TextLabelICType->setPixmap(QPixmap());
  LineEditICWeight->setText("1.0");
  SpinBoxICAtom1->setValue(1);
  SpinBoxICAtom2->setValue(1);
  SpinBoxICAtom3->setValue(1);
  SpinBoxICAtom4->setValue(1);
  GroupBoxICGlobal->setEnabled(false);
  GroupBoxICSpecific->setEnabled(false);
  ToolButtonICAdd2->setEnabled(false);
  ToolButtonICRemove->setEnabled(false);
  ToolButtonICClear->setEnabled(false);
  ToolButtonICUp->setEnabled(false);
  ToolButtonICDown->setEnabled(false);

  ///// Cartesian coordinates - Symmetry
  CheckBoxSYMM->setChecked(false);
  CheckBoxSYMMx->setChecked(false);
  CheckBoxSYMMy->setChecked(false);
  CheckBoxSYMMz->setChecked(false);
  CheckBoxSYMMxy->setChecked(false);
  CheckBoxSYMMxz->setChecked(false);
  CheckBoxSYMMyz->setChecked(false);
  CheckBoxSYMMxyz->setChecked(false);
  GroupBoxSYMM->setEnabled(false);
    
  ///// Cartesian coordinates - Other
  SpinBoxFIXA->setValue(1);
  CheckBoxFIXA1->setChecked(true);
  CheckBoxFIXA2->setChecked(true);
  CheckBoxFIXA3->setChecked(true);
  ListViewFIXA->clear();
  CheckBoxTROT->setChecked(false);
  ComboBoxTROT->setCurrentItem(1);
  LineEditXTFK1->setText("1.0");
  LineEditXTFK2->setText("1.0");
  LineEditXTFK3->setText("1.0");
  LineEditXTFK4->setText("10.0");
  LineEditXTFK5->setText("10.0");
  LineEditXTFK6->setText("10.0");
  CheckBoxSIGN->setChecked(false);
  CheckBoxANIM->setChecked(false);
  enableTROTWidgets(false);

  ///// Advanced - Thresholds
  LineEditREFT1->setText("0.0009");
  LineEditREFT2->setText("0.0009");
  LineEditREFT3->setText("0.0009");
  LineEditREFT4->setText("0.0009");
  LineEditREFT5->setText("0.0009");
  SpinBoxITER->setValue(0);
  LineEditTHRE1->setText("0.499");
  LineEditTHRE2->setText("9.0");
  LineEditTHRE3->setText("1.0");
  LineEditTHRE4->setText("7.0");

  ///// Advanced - Other
  SpinBoxXBON1->setValue(1);
  SpinBoxXBON2->setValue(2);
  ListViewXBON->clear();
  ButtonGroupMASS->setButton(0);
  CheckBoxFRET->setChecked(false);
  LineEditFRET->setText("");
  CheckBoxORDA->setChecked(false);

  ///// Debug
  CheckBoxGOON->setChecked(false);
  CheckBoxLONG->setChecked(false);
  CheckBoxUPDT->setChecked(false);
  ComboBoxUPDT->setCurrentItem(0);
  LineEditUPDT->setText("1.0");
  CheckBoxSHRT->setChecked(false);
  SpinBoxSHRT->setValue(100);
  LineEditSHRT1->setText("0.003");
  LineEditSHRT2->setText("0.5");
  LineEditSHRT3->setText("1.0");
  enableUPDTWidgets(false);
  enableSHRTWidgets(false);

  ///// Extra
  resetTable(Table);
}

///////////////////////////////////////////////////////////////////////////////
///// Protected Member Functions                                          /////
///////////////////////////////////////////////////////////////////////////////

///// showEvent ///////////////////////////////////////////////////////////////
void RelaxBase::showEvent(QShowEvent* e)
/// Overridden from RelaxWidget::showEvent().
{
  ///// check whether the number of atoms has changed and update the widgets
  ///// accordingly
  if(atoms->count() == 0)
  {
    ///// no atoms, so no widgets available concerning atoms
    GroupBoxIC->setEnabled(false);
    TextLabelICType->setPixmap(QPixmap());
    GroupBoxFIXA->setEnabled(false);
    GroupBoxXBON->setEnabled(false);
    ButtonPreview->setEnabled(false);
  }
  else
  {
    ///// at least 1 atom
    GroupBoxIC->setEnabled(true);
    GroupBoxFIXA->setEnabled(true);
    GroupBoxXBON->setEnabled(true);
    ButtonPreview->setEnabled(true);
    unsigned int numAtoms = atoms->count();
    unsigned int firstAtom, secondAtom, thirdAtom;
    int fourthAtom;
    ///// check the contents of ListViewIC
    QListViewItemIterator it1(ListViewIC);
    while(it1.current())
    {
      getAtomsFromIC(it1.current()->text(7), firstAtom, secondAtom, thirdAtom, fourthAtom);

      if((firstAtom > numAtoms) || (secondAtom > numAtoms) || (thirdAtom > numAtoms) || (fourthAtom > static_cast<int>(numAtoms)))
        delete it1.current();
      else
      {
        it1.current()->setText(7, icRepresentation(firstAtom, secondAtom, thirdAtom, fourthAtom));                
        it1++;
      }
    }
    renumberICs(); // some root IC's might have been deleted
    
    ///// check the contents of ListViewFIXA
    QListViewItemIterator it2(ListViewFIXA);
    while(it2.current())
    {
      unsigned int fixedAtom = it2.current()->text(0).replace(QRegExp("[A-Z, a-z]"), "").toUInt();
      if(fixedAtom > numAtoms)
        delete it2.current();
      else
      {
        it2.current()->setText(0, AtomSet::numToAtom(atoms->atomicNumber(fixedAtom-1)).stripWhiteSpace() + QString::number(fixedAtom));
        it2++;
      }
    }

    ///// check the contents of ListViewXBON
    QListViewItemIterator it3(ListViewXBON);
    while(it3.current())
    {
      unsigned int firstAtom = it3.current()->text(0).replace(QRegExp("[A-Z, a-z]"), "").toUInt();
      unsigned int secondAtom = it3.current()->text(1).replace(QRegExp("[A-Z, a-z]"), "").toUInt();
      if((firstAtom > numAtoms) || (secondAtom > numAtoms))
        delete it3.current();
      else
      {
        it3.current()->setText(0, AtomSet::numToAtom(atoms->atomicNumber(firstAtom-1)).stripWhiteSpace() + QString::number(firstAtom));
        it3.current()->setText(1, AtomSet::numToAtom(atoms->atomicNumber(secondAtom-1)).stripWhiteSpace() + QString::number(secondAtom));
        it3++;
      }
    }
    
    ///// update the ranges of the spinboxes
    SpinBoxICAtom1->setMaxValue(numAtoms);
    SpinBoxICAtom2->setMaxValue(numAtoms);
    SpinBoxICAtom3->setMaxValue(numAtoms);
    SpinBoxICAtom4->setMaxValue(numAtoms);
    SpinBoxFIXA->setMaxValue(numAtoms);
    SpinBoxXBON1->setMaxValue(numAtoms);
    SpinBoxXBON2->setMaxValue(numAtoms);            
  }
  
  RelaxWidget::showEvent(e);
}

///////////////////////////////////////////////////////////////////////////////
///// Protected Slots                                                     /////
///////////////////////////////////////////////////////////////////////////////

///// accept //////////////////////////////////////////////////////////////////
void RelaxBase::accept()
/// Overridden from RelaxWidget::accept().
{
  if(widgetChanged)
  {
    setChanged(false);
    saveWidgets();
    RelaxWidget::accept();
  }
  else
    RelaxWidget::reject();
}

///// reject //////////////////////////////////////////////////////////////////
void RelaxBase::reject()
/// Overridden from RelaxWidget::reject().
{
  if(widgetChanged)
  {
    setChanged(false);
    restoreWidgets();
  }
  RelaxWidget::reject();
}


///////////////////////////////////////////////////////////////////////////////
///// Private Slots                                                       /////
///////////////////////////////////////////////////////////////////////////////

///// showPreview /////////////////////////////////////////////////////////////
void RelaxBase::showPreview()
/// Shows a preview from the widgets. This function only starts MAFF if needed.
/// The actual showing is done by showPreview2.
{
  if(!CheckBoxAutogen->isChecked())
  {
    showPreview2();
    return;
  }

  ///// start MAFF
  if(!startMAFF(RelaxBase::PreviewAFF))
    return;

  ///// disable some stuff during the run of MAFF
  ButtonPreview->setEnabled(false); // no reentracy
}

///// showPreview2 ////////////////////////////////////////////////////////////
void RelaxBase::showPreview2()
/// Shows a preview from the widgets. This function is either called directly
/// by showPreview if IC's are not generated or as a slot when the MAFF process 
/// is finished.
{
  bool ranMaff = false;
  if(!ButtonPreview->isEnabled())
  {
    // MAFF was run
    ButtonPreview->setEnabled(true);
    ranMaff = true;
  }

  ///// construct the input file
  QStringList relaxInp = generateAFFHeader();
  if(ranMaff)
  {
    relaxInp += readMAFFGeneratedICs(); // MAFF was run 
    relaxInp += "stop";
  }

  ///// show the input file
  TextViewWidget* preview = new TextViewWidget(this, 0, true, 0);
  preview->TextEdit->setTextFormat(Qt::LogText);
  QFontMetrics metrics(preview->TextEdit->currentFont());
  int newWidth = 81*metrics.width(" ") + 2*preview->layout()->margin() + preview->TextEdit->verticalScrollBar()->sliderRect().width(); // 20 = 2 x layoutMargin
  preview->resize(newWidth, preview->height()); 
  QString previewCaption = tr("Preview input file ")+calcName + ".aff";
  if(!ranMaff)
    previewCaption += "1";  
  preview->setCaption(previewCaption);
  preview->TextEdit->setText(relaxInp.join("\n"));      
  preview->exec();
  delete preview;
}

///// readInputFile ///////////////////////////////////////////////////////////
void RelaxBase::readInputFile()
/// Reads a Relax AFF file into the widgets.  
{
  ///// select an input file
  QString filename = QFileDialog::getOpenFileName(QString::null, "*.aff", this, 0, tr("Choose an AFF file"));
  if(filename.isEmpty())
    return;

  ///// read its contents
  QFile file(filename);
  if(!file.open(IO_ReadOnly))
    return;
  QTextStream stream(&file);
  QString line, icTypeString;
  QStringList aff;
  unsigned int icType = 0;
  while(!stream.atEnd() && line.left(4).lower() != "stop")
  {
    line = stream.readLine();
    if(line.left(1) != "!" && !line.stripWhiteSpace().isEmpty())
      aff += line;
    if(line.left(4).lower() == "bmat")
      icType = 1;
    else if(line.left(4).lower() == "gbma")
      icType = 2;
  }
  
  bool readIC = false;
  if(icType != 0)
  {
    // Ask whether the internal coordinates should be read too.
    QString question = tr("Also read the ");
    if(icType == 1)
      question += tr("non");
    question += tr("redundant internal coordinates?");
    readIC = QMessageBox::question(this, Version::appName, question, QMessageBox::Yes, QMessageBox::No, QMessageBox::NoButton) == QMessageBox::Yes;    
  }

  ///// starting configuration
  reset();

  ///// read the internal coordinates
  if(readIC)
    fillICListFromAFF(aff);

  ///// indicates whether we're past the BMAT or GBMA keywords where unknown lines 
  ///// should not be added to the extra keywords
  bool pastICDef = false;

  ///// only add existing atoms
  unsigned numAtoms = atoms->count();

  ///// read the rest of the keywords
  for(QStringList::Iterator it = aff.begin(); it != aff.end(); it++)
  {
    QString line = (*it);
    QString key = line.left(4).lower(); // all recognized keys are 4 chars

    if(key == "bmat" || key == "gbma" || key == "fmat")
      pastICDef = true;
    else if(key == "fixc" || key == "maxi" || key == "toro")
      ; // already parsed in fillICListFromAFF
    else if(key == "maxd")
      SpinBoxMAXD->setValue(line.mid(10,10).toUInt());
    else if(key == "summ")
      CheckBoxSUMM->setChecked(true);
    else if(key == "symm")
    {
      CheckBoxSYMM->setChecked(true);
      for(unsigned int i = 0; i < 7; i++)
      {
        QString symmType = line.mid(10+10*i,10).stripWhiteSpace();
        if(symmType == "all")
        {
          CheckBoxSYMMx->setChecked(true);
          CheckBoxSYMMy->setChecked(true);
          CheckBoxSYMMz->setChecked(true);
          CheckBoxSYMMxy->setChecked(true);
          CheckBoxSYMMxz->setChecked(true);
          CheckBoxSYMMyz->setChecked(true);
          CheckBoxSYMMxyz->setChecked(true);
          break;
        }
        else if(symmType == "x")
          CheckBoxSYMMx->setChecked(true);
        else if(symmType == "y")
          CheckBoxSYMMy->setChecked(true);
        else if(symmType == "z")
          CheckBoxSYMMz->setChecked(true);
        else if(symmType == "xy")
          CheckBoxSYMMxy->setChecked(true);
        else if(symmType == "xz")
          CheckBoxSYMMxz->setChecked(true);
        else if(symmType == "yz")
          CheckBoxSYMMyz->setChecked(true);
        else if(symmType == "xyz")
          CheckBoxSYMMxyz->setChecked(true);
      }
    }
    else if(key == "fixa")
    {
      unsigned int atom = line.mid(10,10).toUInt();
      if(atom <= numAtoms)
      {
        SpinBoxFIXA->setValue(atom);  
        QString xyz = line.mid(20,10);
        CheckBoxFIXA1->setChecked(xyz.contains("x", false) != 0);
        CheckBoxFIXA2->setChecked(xyz.contains("y", false) != 0);
        CheckBoxFIXA3->setChecked(xyz.contains("z", false) != 0);
        newFixedAtom();
      }
    }
    else if(key == "trot")
    {
      CheckBoxTROT->setChecked(true);
      unsigned int trotType = line.mid(10,10).toUInt();
      if(trotType != 0)
        ComboBoxTROT->setCurrentItem(trotType - 1);
    }
    else if(key == "xtfk")
    {
      line = *(++it);
      LineEditXTFK1->setText(QString::number(line.section(" ",0,0,QString::SectionSkipEmpty).toDouble()));
      LineEditXTFK2->setText(QString::number(line.section(" ",1,1,QString::SectionSkipEmpty).toDouble()));
      LineEditXTFK3->setText(QString::number(line.section(" ",2,2,QString::SectionSkipEmpty).toDouble()));
      LineEditXTFK4->setText(QString::number(line.section(" ",3,3,QString::SectionSkipEmpty).toDouble()));
      LineEditXTFK5->setText(QString::number(line.section(" ",4,4,QString::SectionSkipEmpty).toDouble()));
      LineEditXTFK6->setText(QString::number(line.section(" ",5,5,QString::SectionSkipEmpty).toDouble()));
    }
    else if(key == "sign")
      CheckBoxSIGN->setChecked(true);
    else if(key == "anim")
      CheckBoxANIM->setChecked(true);
    else if(key == "reft")
    {
      if(!line.mid(10,10).stripWhiteSpace().isEmpty())
        LineEditREFT1->setText(QString::number(line.mid(10,10).toDouble()));
      if(!line.mid(20,10).stripWhiteSpace().isEmpty())
        LineEditREFT1->setText(QString::number(line.mid(20,10).toDouble()));
      if(!line.mid(30,10).stripWhiteSpace().isEmpty())
        LineEditREFT1->setText(QString::number(line.mid(30,10).toDouble()));
      if(!line.mid(40,10).stripWhiteSpace().isEmpty())
        LineEditREFT1->setText(QString::number(line.mid(40,10).toDouble()));
      if(!line.mid(50,10).stripWhiteSpace().isEmpty())
        LineEditREFT1->setText(QString::number(line.mid(50,10).toDouble()));
    }
    else if(key == "iter")
      SpinBoxITER->setValue(line.mid(10,10).toUInt());
    else if(key == "thre")
    {
      if(!line.mid(10,10).stripWhiteSpace().isEmpty())
      {
        double thre = line.mid(10,10).toDouble();
        double threA = log10(floor(thre) - thre);
        int threB = static_cast<int>(thre); // maybe use floor? 
        LineEditTHRE1->setText(QString::number(threA));
        LineEditTHRE2->setText(QString::number(threB));
      }
      if(!line.mid(20,10).stripWhiteSpace().isEmpty())
      {
        double thre = line.mid(20,10).toDouble();
        double threA = log10(floor(thre) - thre);
        int threB = static_cast<int>(thre); 
        LineEditTHRE3->setText(QString::number(threA));
        LineEditTHRE4->setText(QString::number(threB));
      }
    }
    else if(key == "xbon")
    {
      unsigned int atom1 = line.mid(10,10).toUInt();
      unsigned int atom2 = line.mid(20,10).toUInt();
      if(atom1 > numAtoms || atom2 > numAtoms)
        continue;
      SpinBoxXBON1->setValue(atom1);
      SpinBoxXBON1->setValue(atom1);
      newExtraBond();
    }
    else if(key == "mass")
      RadioButtonMASS2->setChecked(true);
    else if(key == "avma")
      RadioButtonMASS3->setChecked(true);
    else if(key == "fret")
      LineEditFRET->setText(QString::number(line.mid(10,10).toDouble()));
    else if(key == "orda")
      CheckBoxORDA->setChecked(true);
    else if(key == "goon")
      CheckBoxGOON->setChecked(true);
    else if(key == "long")
      CheckBoxLONG->setChecked(true);
    else if(key == "updt")
    {
      CheckBoxUPDT->setChecked(true);
      ComboBoxUPDT->setCurrentItem(line.mid(10,10).toUInt() - 1);
      LineEditUPDT->setText(QString::number(line.mid(20,10).toDouble()));
    }
    else if(key == "shrt")
    {
      CheckBoxSHRT->setChecked(true);
      LineEditSHRT1->setText(QString::number(line.mid(10,10).toDouble()));
      LineEditSHRT2->setText(QString::number(line.mid(20,10).toDouble()));
      LineEditSHRT3->setText(QString::number(line.mid(30,10).toDouble()));
    }
    else
    {
      if(pastICDef)
      {
        ///// when passed the start of BMAT, GBMA or FMAT only check for the 
        ///// existance of the XFTK keyword and quit
        while(it != aff.end() && key != "xtfk")
          key = (*(++it)).left(4).lower();
        if(key == "xtfk")
        {
          line = *(++it);
          // free format or not?

          LineEditXTFK1->setText(QString::number(line.section(" ",0,0,QString::SectionSkipEmpty).toDouble()));
          LineEditXTFK2->setText(QString::number(line.section(" ",1,1,QString::SectionSkipEmpty).toDouble()));
          LineEditXTFK3->setText(QString::number(line.section(" ",2,2,QString::SectionSkipEmpty).toDouble()));
          LineEditXTFK4->setText(QString::number(line.section(" ",3,3,QString::SectionSkipEmpty).toDouble()));
          LineEditXTFK5->setText(QString::number(line.section(" ",4,4,QString::SectionSkipEmpty).toDouble()));
          LineEditXTFK6->setText(QString::number(line.section(" ",5,5,QString::SectionSkipEmpty).toDouble()));  
        }
        return;
      }
      else
      {
        ///// unknown piece of data to be added to Table
        QStringList texts;
        for(unsigned int i = 0; i < 8; i++)
          texts += line.mid(i*10,10);
        int row= firstEmptyTableRow(Table);
        for(unsigned int i = 0; i < 8; i++)
        {
          if(!(*(texts.at(i))).isEmpty())
            Table->setText(row, i, *(texts.at(i)));
        }
      }
    }
  }
}

///// setChanged //////////////////////////////////////////////////////////////
void RelaxBase::setChanged(bool state)
/// Sets the 'changed' property of the widget.
{
  widgetChanged = state;
}

///// selectWidget ////////////////////////////////////////////////////////////
void RelaxBase::selectWidget(QListViewItem* newItem)
/// Shows a widget from WidgetStackCategory depending on newItem.
{
  if(newItem->childCount() != 0)
  {
    ///// this is a root item with children, so select the first child (this should retrigger this slot)
    newItem->setOpen(true);
    ListViewCategory->setSelected(newItem->firstChild(), true);
    return;
  }

  ///// show a widget from the widgetstack depending on the selected listview item
  if(newItem->parent() == 0)
  {
    ///// this is a root item
    if(newItem->text(0) == category[BASIC])
      WidgetStackCategory->raiseWidget(0);
    else if(newItem->text(0) == category[INTERNAL])
      WidgetStackCategory->raiseWidget(5);
    else if(newItem->text(0) == category[DEBUG1])
      WidgetStackCategory->raiseWidget(6);
    else if(newItem->text(0) == category[EXTRA])
      WidgetStackCategory->raiseWidget(7);
  }
  else
  {
    ///// this is a child item
    if(newItem->parent()->text(0) == category[CARTESIAN])
    {
      if(newItem->text(0) == category[CARTESIAN_SYMMETRY])
        WidgetStackCategory->raiseWidget(3);
      else if(newItem->text(0) == category[CARTESIAN_OTHER])
        WidgetStackCategory->raiseWidget(4);
    }
    else if(newItem->parent()->text(0) == category[ADVANCED])
    {
      if(newItem->text(0) == category[ADVANCED_THRESHOLDS])
        WidgetStackCategory->raiseWidget(1);
      else if(newItem->text(0) == category[ADVANCED_OTHER])
        WidgetStackCategory->raiseWidget(2);
    }
  }
}

///// enableAutogenWidgets ////////////////////////////////////////////////////
void RelaxBase::enableAutogenWidgets()
/// Enabled/disables the widgets for configuring the
/// (re)generation of the internal coordinates.
{
  CheckBoxRegen->setEnabled(CheckBoxAutogen->isChecked());
  SpinBoxGenFreq->setEnabled(CheckBoxRegen->isChecked() && CheckBoxRegen->isEnabled());
  CheckBoxHBonds->setEnabled(CheckBoxAutogen->isChecked());
}

///// enableSFACWidgets ///////////////////////////////////////////////////////
void RelaxBase::enableSFACWidgets(bool state)
/// Enabled/disables the widgets for configuring the scale factor
/// depending on RadioButtonSFAC{1|2}.
{
  LineEditSFAC1->setEnabled(state);
  TextLabelSFAC1->setEnabled(!state);
  TextLabelSFAC2->setEnabled(!state);
  LineEditSFAC2->setEnabled(!state);
  SpinBoxSFAC->setEnabled(!state);
  ToolButtonSFAC1->setEnabled(!state);
  ToolButtonSFAC2->setEnabled(!state);
  ListViewSFAC->setEnabled(!state);  
}

///// newScaleFactor //////////////////////////////////////////////////////////
void RelaxBase::newScaleFactor()
/// Adds a new scale factor to the list.
{
  ///// create a new item
  QListViewItem* lastItem = ListViewSFAC->lastItem();
  QListViewItem* newItem = new QListViewItem(ListViewSFAC, lastItem);

  ///// set it's properties
  newItem->setText(0, LineEditSFAC2->text());
  newItem->setText(1, QString::number(SpinBoxSFAC->value()));  
}

///// deleteScaleFactor ///////////////////////////////////////////////////////
void RelaxBase::deleteScaleFactor()
/// Deletes the currently selected scale factor from the list.
{
  QListViewItem* item = ListViewSFAC->selectedItem();
  if(item == 0)
    return;
  delete item;
}

///// updateICEdit ////////////////////////////////////////////////////////////
void RelaxBase::updateICEdit()
/// Updates the widgets for changing an IC with the contents
/// of the currently selected IC in ListViewIC.
{
  /*
  qDebug("RelaxBase::updateICEdit() called");
  if(ListViewIC->currentItem() == 0)
    qDebug("  current  item: none");
  else
    qDebug("  current  item: "+ListViewIC->currentItem()->text(0)+"|"+ListViewIC->currentItem()->text(7));
  if(ListViewIC->selectedItem() == 0)
    qDebug("  selected item: none");
  else  
    qDebug("  selected item: "+ListViewIC->selectedItem()->text(0)+"|"+ListViewIC->selectedItem()->text(7));
  */
  
  ///// determine the sub item
  QListViewItem* subItem = ListViewIC->selectedItem();
  if(subItem == 0)
  {
    ///// no item available
    GroupBoxICGlobal->setEnabled(false);
    GroupBoxICSpecific->setEnabled(false);
    ToolButtonICAdd2->setEnabled(false);
    ToolButtonICRemove->setEnabled(false);
    ToolButtonICClear->setEnabled(false);
    ToolButtonICUp->setEnabled(false);
    ToolButtonICDown->setEnabled(false);
    return;
  }
  else
  {
    GroupBoxICGlobal->setEnabled(true);
    GroupBoxICSpecific->setEnabled(true);    
    ToolButtonICAdd2->setEnabled(true);
    ToolButtonICRemove->setEnabled(true);
    ToolButtonICClear->setEnabled(true);
    ToolButtonICUp->setEnabled(true);
    ToolButtonICDown->setEnabled(true);
  }

  ///// determine the root item  
  QListViewItem* rootItem;
  if(!subItem->text(0).isEmpty())
    rootItem = subItem;
  else
    rootItem = subItem->parent();

  ///// update the global widgets
  TextLabelICNumber->setText(rootItem->text(0));
  ComboBoxICType->blockSignals(true); // updateICSpecifics should be called manually
  ComboBoxICType->setCurrentText(rootItem->text(1));
  ComboBoxICType->blockSignals(false);
  LineEditFC->blockSignals(true);
  LineEditFC->setText(rootItem->text(2));
  LineEditFC->blockSignals(false);
  CheckBoxFIXC->blockSignals(true);
  CheckBoxFIXC->setChecked(rootItem->pixmap(3) != 0);
  CheckBoxFIXC->blockSignals(false);
  CheckBoxMAXI->blockSignals(true);
  CheckBoxMAXI->setChecked(rootItem->pixmap(4) != 0);
  CheckBoxMAXI->blockSignals(false);
  CheckBoxTORO->blockSignals(true);
  CheckBoxTORO->setChecked(!rootItem->text(5).isEmpty());
  CheckBoxTORO->blockSignals(false);
  LineEditTORO->blockSignals(true);
  LineEditTORO->setText(rootItem->text(5));
  LineEditTORO->blockSignals(false);

  ///// update the specific widgets
  unsigned int icPart = 1;
  QListViewItemIterator it = rootItem->firstChild();
  while(it.current())
  {
    icPart++;
    if(it.current() == subItem)
      break;
    it++;
  }
  //qDebug("RelaxBase::updateICEdit(): icPart = %d", icPart);
  TextLabelICPart->setText(QString::number(icPart));
  LineEditICWeight->blockSignals(true);
  LineEditICWeight->setText(subItem->text(6));
  LineEditICWeight->blockSignals(false);
  updateICSpecifics();
}

///// updateICList ////////////////////////////////////////////////////////////
void RelaxBase::updateICList()
/// Updates ListViewIC with the contents of the widgets for
/// changing an IC. It blocks calls to updateICEdit to prevent infinite recursion.
{
  ListViewIC->blockSignals(true);
  
  QString icNumber = TextLabelICNumber->text();
  QListViewItem* rootItem = ListViewIC->findItem(icNumber, 0);
  if(rootItem == 0) // no items in the ListView
    return;
      
  QListViewItem* subItem;
  unsigned int icPart = TextLabelICPart->text().toUShort();
  if(icPart == 1)
    subItem = rootItem;
  else
  {
    ///// look thru the childs of the rootItem for the correct child item
    QListViewItemIterator it = rootItem->firstChild();
    for(unsigned int i = 2; i < icPart; i++)
      it++;
    subItem = it.current();
    ASSERT(subItem != 0);
  }
  ///// update the global stuff (ListView item only really get updated if the contents is different)
  rootItem->setText(1, ComboBoxICType->currentText());
  rootItem->setText(2, LineEditFC->text());
  if(CheckBoxFIXC->isChecked())
    rootItem->setPixmap(3, IconSets::getPixmap(IconSets::OK));
  else
    rootItem->setPixmap(3, QPixmap()); // clears the pixmap
  if(CheckBoxMAXI->isChecked())
    rootItem->setPixmap(4, IconSets::getPixmap(IconSets::OK));
  else
    rootItem->setPixmap(4, QPixmap());
  if(CheckBoxTORO->isChecked())
    rootItem->setText(5, LineEditTORO->text());
  else
    rootItem->setText(5, "");  
  ///// update the specific stuff
  subItem->setText(6, LineEditICWeight->text());
  unsigned int atom3 = 0;
  if(SpinBoxICAtom3->isEnabled())
    atom3 = SpinBoxICAtom3->value();
  unsigned int atom4 = 0;
  if(SpinBoxICAtom4->isEnabled())
  {
    atom4 = SpinBoxICAtom4->value();
    if(atom4 < 1)
      atom4--;
  }
  subItem->setText(7, icRepresentation(SpinBoxICAtom1->value(), SpinBoxICAtom2->value(), atom3, atom4));
          
  ListViewIC->blockSignals(false);  
}

///// updateICSpecifics ///////////////////////////////////////////////////////
void RelaxBase::updateICSpecifics()
/// Updates the widgets for the specific data.
{
  QListViewItem* subItem = ListViewIC->selectedItem();
  unsigned int numAtoms = 0;;
  switch(ComboBoxICType->currentItem())
  {
    case 0: ///// stretch
            numAtoms = 2;
            TextLabelICType->setPixmap(IconSets::getPixmap(IconSets::Stretch));
            break;
    case 1: ///// bend
            numAtoms = 3;
            TextLabelICType->setPixmap(IconSets::getPixmap(IconSets::Bend));
            break;
    case 2: ///// torsion
            numAtoms = 4;
            TextLabelICType->setPixmap(IconSets::getPixmap(IconSets::Torsion));
            break;
    case 3: ///// out-of-plane
            numAtoms = 4;
            TextLabelICType->setPixmap(IconSets::getPixmap(IconSets::OutOfPlane));
            break;
    case 4: ///// linear bend
            numAtoms = 4;
            TextLabelICType->setPixmap(IconSets::getPixmap(IconSets::LinearBend));
  }
  SpinBoxICAtom1->blockSignals(true);
  SpinBoxICAtom1->setValue(subItem->text(7).section("-", 0, 0, QString::SectionSkipEmpty).replace(QRegExp("[A-Z, a-z]"), "").toInt());
  SpinBoxICAtom1->blockSignals(false);
  SpinBoxICAtom2->blockSignals(true);
  SpinBoxICAtom2->setValue(subItem->text(7).section("-", 1, 1, QString::SectionSkipEmpty).replace(QRegExp("[A-Z, a-z]"), "").toInt());
  SpinBoxICAtom2->blockSignals(false);
  SpinBoxICAtom3->blockSignals(true);
  if(numAtoms >= 3)
  {
    SpinBoxICAtom3->setEnabled(true);
    SpinBoxICAtom3->setValue(subItem->text(7).section("-", 2, 2, QString::SectionSkipEmpty).replace(QRegExp("[A-Z, a-z]"), "").toInt());
  }
  else
  {
    SpinBoxICAtom3->setEnabled(false);
    SpinBoxICAtom3->setValue(1);
  }
  SpinBoxICAtom3->blockSignals(false);
  SpinBoxICAtom4->blockSignals(true);
  if(numAtoms == 4)
  {
    SpinBoxICAtom4->setEnabled(true);
    QString section4 = subItem->text(7).section("-", 3, 3, QString::SectionSkipEmpty);
    if(section4 == "X")
      SpinBoxICAtom4->setValue(0);
    else if(section4 == "Y")
      SpinBoxICAtom4->setValue(-1);
    else if(section4 == "Z")
      SpinBoxICAtom4->setValue(-2);
    else
      SpinBoxICAtom4->setValue(section4.replace(QRegExp("[A-Z, a-z]"), "").toInt());
  }
  else
  {
    SpinBoxICAtom4->setEnabled(false);
    SpinBoxICAtom4->setValue(1);
  }
  SpinBoxICAtom4->blockSignals(false);

  //qDebug("RelaxBase::updateICSpecifics: manual call of updateICList()");
  //updateICList();
}

///// updateICType ////////////////////////////////////////////////////////////
void RelaxBase::updateICType()
/// Does the necessary updating of all parts of an internal
// coordinate when the type changes.
{
  ///// determine the subItem and the rootItem
  QListViewItem* subItem = ListViewIC->selectedItem();
  if(subItem == 0)
    return; // no selection present
  QListViewItem* rootItem = subItem;
  if(subItem->parent() != 0)
    rootItem = subItem->parent();

  ///// determine the new IC type and update the widgets
  unsigned int numAtoms = 0;
  LineEditFC->blockSignals(true);
  switch(ComboBoxICType->currentItem())
  {
    case 0: ///// stretch
            numAtoms = 2;
            TextLabelICType->setPixmap(IconSets::getPixmap(IconSets::Stretch));
            LineEditFC->setText("5.0");
            break;
    case 1: ///// bend
            numAtoms = 3;
            TextLabelICType->setPixmap(IconSets::getPixmap(IconSets::Bend));
            LineEditFC->setText("1.0");
            break;
    case 2: ///// torsion
            numAtoms = 4;
            TextLabelICType->setPixmap(IconSets::getPixmap(IconSets::Torsion));
            LineEditFC->setText("0.1");
            break;
    case 3: ///// out-of-plane
            numAtoms = 4;
            TextLabelICType->setPixmap(IconSets::getPixmap(IconSets::OutOfPlane));
            LineEditFC->setText("0.1");
            break;
    case 4: ///// linear bend
            numAtoms = 4;
            TextLabelICType->setPixmap(IconSets::getPixmap(IconSets::LinearBend));
            LineEditFC->setText("0.5");
  }
  LineEditFC->blockSignals(false);
  LineEditTORO->blockSignals(true);
  LineEditTORO->setText("");
  LineEditTORO->blockSignals(false);  
  SpinBoxICAtom3->blockSignals(true);
  if(numAtoms >= 3)
  {
    if(!SpinBoxICAtom3->isEnabled())
    {
      SpinBoxICAtom3->setEnabled(true);
      SpinBoxICAtom3->setValue(1);
    }
  }
  else
  {
    SpinBoxICAtom3->setEnabled(false);
    SpinBoxICAtom3->setValue(1);
  }
  SpinBoxICAtom3->blockSignals(false);
  SpinBoxICAtom4->blockSignals(true);
  if(numAtoms == 4)
  {
    if(!SpinBoxICAtom4->isEnabled())
    {
      SpinBoxICAtom4->setEnabled(true);
      SpinBoxICAtom4->setValue(1);
    }
  }
  else
  {
    SpinBoxICAtom4->setEnabled(false);
    SpinBoxICAtom4->setValue(1);
  }
  SpinBoxICAtom4->blockSignals(false);
  
  ///// update ListViewIC /////
  ListViewIC->blockSignals(true);
  ///// update the rootItem
  rootItem->setText(1, ComboBoxICType->currentText());
  rootItem->setText(2, LineEditFC->text());
  rootItem->setText(5, "");
  unsigned int atom1 = rootItem->text(7).section("-", 0, 0, QString::SectionSkipEmpty).replace(QRegExp("[A-Z, a-z]"), "").toInt();
  unsigned int atom2 = rootItem->text(7).section("-", 1, 1, QString::SectionSkipEmpty).replace(QRegExp("[A-Z, a-z]"), "").toInt();
  unsigned int atom3 = rootItem->text(7).section("-", 2, 2, QString::SectionSkipEmpty).replace(QRegExp("[A-Z, a-z]"), "").toInt();
  unsigned int atom4 = rootItem->text(7).section("-", 3, 3, QString::SectionSkipEmpty).replace(QRegExp("[A-Z, a-z]"), "").toInt();
  switch(numAtoms)
  {
    case 2: atom3 = atom4 = 0;
            break;            
    case 3: if(atom3 == 0)
              atom3 = 1;
            atom4 = 0;
            break;
    case 4: if(atom3 == 0)
              atom3 = 1;
            if(atom4 == 0)
              atom4 = 1;
  }
  rootItem->setText(7, icRepresentation(atom1, atom2, atom3, atom4));   

  ///// update all child items
  QListViewItemIterator it(rootItem);
  while(it.current())
  {
    atom1 = it.current()->text(7).section("-", 0, 0, QString::SectionSkipEmpty).replace(QRegExp("[A-Z, a-z]"), "").toInt();
    atom2 = it.current()->text(7).section("-", 1, 1, QString::SectionSkipEmpty).replace(QRegExp("[A-Z, a-z]"), "").toInt();
    atom3 = it.current()->text(7).section("-", 2, 2, QString::SectionSkipEmpty).replace(QRegExp("[A-Z, a-z]"), "").toInt();
    atom4 = it.current()->text(7).section("-", 3, 3, QString::SectionSkipEmpty).replace(QRegExp("[A-Z, a-z]"), "").toInt();
    switch(numAtoms)
    {
      case 2: atom3 = atom4 = 0;
              break;
      case 3: if(atom3 == 0)
                atom3 = 1;
              atom4 = 0;
              break;
      case 4: if(atom3 == 0)
                atom3 = 1;
              if(atom4 == 0)
                atom4 = 1;
    }
    it.current()->setText(7, icRepresentation(atom1, atom2, atom3, atom4));  
    it++;
  }

  ListViewIC->blockSignals(false);  
}

///// newIC ///////////////////////////////////////////////////////////////////
void RelaxBase::newIC()
/// Creates a new internal coordinate at the bottom of ListViewIC.
{
  QListViewItem* newItem; // this will be the newly inserted item
  QListViewItem* lastItem = ListViewIC->lastItem(); // get the last item
  if(lastItem != 0)
  {
    if(lastItem->parent() != 0)
      lastItem = lastItem->parent(); // if it's a subitem, get the root one
    newItem = new QListViewItem(ListViewIC, lastItem);
  }
  else
    newItem = new QListViewItem(ListViewIC); // the listview is empty
    
  newItem->setText(0, QString::number(ListViewIC->childCount()));
  newItem->setText(1, ComboBoxICType->text(0));
  newItem->setText(2, "5.0");
  newItem->setText(6, "1.0");
  switch(atoms->count())
  {
    case 0: newItem->setText(7, "X1-X1");
            break;
    case 1: newItem->setText(7, icRepresentation(1, 1));
            break;
    default: newItem->setText(7, icRepresentation(1, 2));
  }
  ListViewIC->setSelected(newItem, true);
  ListViewIC->ensureItemVisible(newItem);
  //qDebug("RelaxBase::newIC: manual call of updateICEdit()");
  updateICEdit();
  ToolButtonICAdd2->setEnabled(true);
  setChanged();
}

///// newICPart ///////////////////////////////////////////////////////////////
void RelaxBase::newICPart()
/// Creates a new part of an internal coordinate.
{
  ///// determine the root item under which the new part should be added
  QListViewItem* rootItem = ListViewIC->selectedItem();
  if(rootItem == 0)
    return;
  if(rootItem->parent() != 0)
    rootItem = rootItem->parent();

  ///// create the new item
  QListViewItem* newItem;
  if(rootItem->childCount() > 0)
  {
    ///// find the last child item
    QListViewItem* lastItem = 0;
    QListViewItemIterator it(rootItem);
    while(it.current())
    {
      lastItem = it.current();
      it++;
    }
    newItem = new QListViewItem(rootItem, lastItem);
  }
  else
    newItem = new QListViewItem(rootItem);

  ///// populate it wih the contents of the root item
  newItem->setText(6, rootItem->text(6));
  newItem->setText(7, rootItem->text(7));

  ///// update
  ListViewIC->setSelected(newItem, true);
  ListViewIC->ensureItemVisible(newItem);
  //qDebug("RelaxBase::newICPart: manual call of updateICEdit()");
  updateICEdit();
  setChanged();  
}

///// deleteIC ////////////////////////////////////////////////////////////////
void RelaxBase::deleteIC()
/// Deletes the currently selected internal coordinate.
{
  ///// determine whether it's a root or sub item
  QListViewItem* item = ListViewIC->selectedItem();
  if(item == 0)
    return;

  ///// delete the item and determine the newly selected item    
  QListViewItem* newItem = item->nextSibling();
  if(item->parent() == 0)
  {
    ///// it's a root item so delete it. It's subitems will be deleted too
    ///// first find it's next sibling
    if(newItem == 0) // no root item below this one
      newItem = item->itemAbove();
    if(newItem == 0) // no item above this one (root or sub)
    {
      ///// item is the only item
      clearIC();
      return;
    }
    else
    {
      ///// newItem points to the newly selected item
      delete item; // delete the selected item
      renumberICs(); // renumber the remaining internal coordinates
      ListViewIC->setSelected(newItem, true); // select the new item
    }
  }
  else
  {
    ///// it's a sub item
    if(newItem == 0) // no sub item below this one
      newItem = item->itemAbove(); // previous subitem or root item. always exists
    delete item;
    ///// select the new item
    ListViewIC->setSelected(newItem, true);
  }
  setChanged();  
}

///// clearIC /////////////////////////////////////////////////////////////////
void RelaxBase::clearIC()
/// Deletes all internal coordinates.
{
  ListViewIC->clear();
  updateICEdit(); // disables everything
  setChanged();
}

///// genIC ///////////////////////////////////////////////////////////////////
void RelaxBase::genIC()
/// Fills the internal coordinates list with (non)redundant internal coordinates.
/// This function only starts MAFF. The reuslting file is parsed in genIC2.
{
  if(!startMAFF(RelaxBase::ICListAFF))
    return;

  ///// disable some stuff during the run of MAFF
  ToolButtonICGen->setEnabled(false);
}

///// genIC2 //////////////////////////////////////////////////////////////////
void RelaxBase::genIC2()
/// Fills the internal coordinates list with (non)redundant internal coordinates.
/// It is only called when a MAFF run has ended.
{
  ToolButtonICGen->setEnabled(true);

  QStringList icList = readMAFFGeneratedICs();
  if(icList.isEmpty())
    return;

  fillICListFromAFF(icList);
  updateICEdit();
  setChanged();
}

///// moveICUp ////////////////////////////////////////////////////////////////
void RelaxBase::moveICUp()
/// Moves the selected internal coordinate one place up in the list.
{
  ///// determine whether it's a root or sub item
  QListViewItem* item = ListViewIC->selectedItem();
  if(item == 0)
    return;
  if(item->parent() == 0)
  {
    ///// root item
    if(item == ListViewIC->firstChild())
      return; // can't move the first item further up
    QListViewItem* newItem = item->itemAbove();
    while(newItem->parent() != 0)
      newItem = newItem->itemAbove();
    ///// now newItem is the previous root item
    newItem->moveItem(item); // newItem is moved after item => equivalent to moving item before newItem
    renumberICs(); // maybe make an optimized version which just switches the 2 numbers
  }
  else
  {
    ///// sub item
    QListViewItem* rootItem = item->parent();
    if(item == rootItem->firstChild())
      return; // can't move the first item further up
    QListViewItem* newItem = item->itemAbove(); // certainly also a sub item
    newItem->moveItem(item);
  }
  ListViewIC->setSelected(item, true); // needed?
  setChanged();
}

///// moveICDown //////////////////////////////////////////////////////////////
void RelaxBase::moveICDown()
/// Moves the selected internal coordinate one place down in the list.
{
  ///// determine whether it's a root or sub item
  QListViewItem* item = ListViewIC->selectedItem();
  if(item == 0)
    return;

  QListViewItem* newItem = item->nextSibling();
  if(newItem == 0)
    return; // can't move last item further down (root or sub item)
  item->moveItem(newItem);

  if(item->parent() == 0)
    renumberICs(); // maybe make an optimized version which just switches the 2 numbers
    
  ListViewIC->setSelected(item, true); // needed?
  setChanged();
}

///// newFixedAtom ////////////////////////////////////////////////////////////
void RelaxBase::newFixedAtom()
/// Adds a new atom to the list of fixed atoms.
{
  ///// check whether the atom is already present in the list
  ///// If it is, just update it
  QListViewItem* newItem = 0;
  QString newAtom = AtomSet::numToAtom(atoms->atomicNumber(SpinBoxFIXA->value() - 1)).stripWhiteSpace() + QString::number(SpinBoxFIXA->value());
  QListViewItemIterator it(ListViewFIXA);
  while(it.current())
  {
    if(it.current()->text(0) == newAtom)
    {
      newItem = it.current();
      break;
    }
    it++;
  }
  if(newItem == 0)
  {
    ///// create a new item
    QListViewItem* lastItem = ListViewFIXA->lastItem();
    newItem = new QListViewItem(ListViewFIXA, lastItem);
    ///// set it's properties according to the widgets
    newItem->setText(0, newAtom);
  }
  ///// update the fixed coordinates, deleting the item if no coordinate is fixed
  QString xyz;
  if(CheckBoxFIXA1->isChecked())
    xyz += "x";
  if(CheckBoxFIXA2->isChecked())
    xyz += "y";
  if(CheckBoxFIXA3->isChecked())
    xyz += "z";
  if(xyz.isEmpty())
    delete newItem;
  else
  {
    if(newItem->text(1) == xyz)
      return; // updating an existing item with identical content should not trigger a changed signal
    newItem->setText(1, xyz);
  }
  
  setChanged();  
}

///// deleteFixedAtom /////////////////////////////////////////////////////////
void RelaxBase::deleteFixedAtom()
/// Deletes the selected atom from the list of fixed atoms.
{
  QListViewItem* item = ListViewFIXA->selectedItem();
  if(item == 0)
    return;

  delete item;
  setChanged();
}

///// checkFIXABoxes //////////////////////////////////////////////////////////
void RelaxBase::checkFIXABoxes()
/// Makes sure at least 1 box stays checked for the fixed atoms. Otherwise
/// nothing would be actually fixed.
{
  // determine the number of checked boxes
  unsigned int numChecked = 0;
  if(CheckBoxFIXA1->isChecked())
    numChecked++;
  if(CheckBoxFIXA2->isChecked())
    numChecked++;
  if(CheckBoxFIXA3->isChecked())
    numChecked++;
  
  // if only 1 is checked, disable it so it cannot be unchecked
  if(numChecked == 1)
  {
    if(CheckBoxFIXA1->isChecked())
      CheckBoxFIXA1->setEnabled(false);
    else if(CheckBoxFIXA2->isChecked())
      CheckBoxFIXA2->setEnabled(false);
    else if(CheckBoxFIXA3->isChecked())
      CheckBoxFIXA3->setEnabled(false);
  }
  // otherwise make sure all are enabled
  else
  {
    CheckBoxFIXA1->setEnabled(true);
    CheckBoxFIXA2->setEnabled(true);
    CheckBoxFIXA3->setEnabled(true);
  }
}

///// enableTROTWidgets ///////////////////////////////////////////////////////
void RelaxBase::enableTROTWidgets(bool state)
/// Enables/disables the widgets for configuring the optimization
/// of the external coordinates depending on CheckBoxTROT.
{
  ComboBoxTROT->setEnabled(state);
  LineEditXTFK1->setEnabled(state);
  LineEditXTFK2->setEnabled(state);
  LineEditXTFK3->setEnabled(state);
  LineEditXTFK4->setEnabled(state);
  LineEditXTFK5->setEnabled(state);
  LineEditXTFK6->setEnabled(state);
}

///// newExtraBond ////////////////////////////////////////////////////////////
void RelaxBase::newExtraBond()
/// Adds a new bond to the XBON list.
{
  ///// check whether the bond is already present in the list
  QString atom1 = AtomSet::numToAtom(atoms->atomicNumber(SpinBoxXBON1->value() - 1)).stripWhiteSpace() + QString::number(SpinBoxXBON1->value());
  QString atom2 = AtomSet::numToAtom(atoms->atomicNumber(SpinBoxXBON2->value() - 1)).stripWhiteSpace() + QString::number(SpinBoxXBON2->value());
  QListViewItemIterator it(ListViewXBON);
  while(it.current())
  {
    if((it.current()->text(0) == atom1) && (it.current()->text(1) == atom2))
      return;
    it++;
  }

  ///// add the bond to the end of the list  
  QListViewItem* lastItem = ListViewXBON->lastItem();
  QListViewItem* newItem = new QListViewItem(ListViewXBON, lastItem);
  ///// set it's properties
  newItem->setText(0, atom1);
  newItem->setText(1, atom2);
}

///// deleteExtraBond /////////////////////////////////////////////////////////
void RelaxBase::deleteExtraBond()
/// Deletes the selected bond from the XBON list.
{
  QListViewItem* item = ListViewXBON->selectedItem();
  if(item == 0)
    return;

  delete item;
}

///// enableUPDTWidgets ///////////////////////////////////////////////////////
void RelaxBase::enableUPDTWidgets(bool state)
/// Enables/disables the widgets for configuring the update
/// of the force constants depending on CheckBoxUPDT.
{
  ComboBoxUPDT->setEnabled(state);
  LineEditUPDT->setEnabled(state);
}

///// enableSHRTWidgets ///////////////////////////////////////////////////////
void RelaxBase::enableSHRTWidgets(bool state)
/// Enables/disables the widgets for configuring the short
/// output depending on CheckBoxSHRT.
{
  TextLabelSHRT1->setEnabled(state);
  TextLabelSHRT2->setEnabled(state);
  TextLabelSHRT3->setEnabled(state);
  TextLabelSHRT4->setEnabled(state);
  TextLabelSHRT5->setEnabled(state);
  SpinBoxSHRT->setEnabled(state);
  LineEditSHRT1->setEnabled(state);
  LineEditSHRT2->setEnabled(state);
  LineEditSHRT3->setEnabled(state);  
}
    
///// checkOverflow ///////////////////////////////////////////////////////////
void RelaxBase::checkOverflow(int row, int col)
/// Spans cell(row, col) over multiple columns if it contains
/// more than 10 characters.
{
  checkTableOverflow(Table, row, col);
}

///// addExtraRow /////////////////////////////////////////////////////////////
void RelaxBase::addExtraRow()
/// If a row is selected, inserts a row before the selection.
/// If no row is selected, adds a row at the end.
{
  addTableRow(Table);
}

///// removeExtraRow //////////////////////////////////////////////////////////
void RelaxBase::removeExtraRow()
/// Removes the selected rows.
{
  if(removeTableRow(Table))
    setChanged();
}

///// clearSelection //////////////////////////////////////////////////////////
void RelaxBase::clearSelection()
/// Clears the selected cells.
{
  if(clearTableSelection(Table))
    setChanged();
}


///////////////////////////////////////////////////////////////////////////////
///// Private Member Functions                                            /////
///////////////////////////////////////////////////////////////////////////////

///// makeConnections /////////////////////////////////////////////////////////
void RelaxBase::makeConnections()
/// Sets up the permanent connections. Called once from the constructor.
{
  ///// connections for ListView-WidgetStack
  connect(ListViewCategory, SIGNAL(selectionChanged(QListViewItem*)), this, SLOT(selectWidget(QListViewItem*)));

  ///// connections for buttons
  ///// bottom ones
  connect(ButtonOK, SIGNAL(clicked()), this, SLOT(accept()));
  connect(ButtonCancel, SIGNAL(clicked()), this, SLOT(reject()));
  connect(ButtonPreview, SIGNAL(clicked()), this, SLOT(showPreview()));
  connect(ButtonRead, SIGNAL(clicked()), this, SLOT(readInputFile()));
  connect(ButtonReset, SIGNAL(clicked()), this, SLOT(reset()));

  ///// connections for enabling/disabling widgets
  ///// Basic
  connect(CheckBoxAutogen, SIGNAL(toggled(bool)), this, SLOT(enableAutogenWidgets()));
  connect(CheckBoxRegen, SIGNAL(toggled(bool)), this, SLOT(enableAutogenWidgets()));
  connect(RadioButtonSFAC1, SIGNAL(toggled(bool)), this, SLOT(enableSFACWidgets(bool)));
  ///// Internal coordinates
  connect(CheckBoxTORO, SIGNAL(toggled(bool)), LineEditTORO, SLOT(setEnabled(bool)));  
  ///// Cartesian coordinates - Symmetry
  connect(CheckBoxSYMM, SIGNAL(toggled(bool)), GroupBoxSYMM, SLOT(setEnabled(bool)));
  ///// Cartesian coordinates - Other
  connect(CheckBoxTROT, SIGNAL(toggled(bool)), this, SLOT(enableTROTWidgets(bool)));
  ///// Advanced - Other
  connect(CheckBoxFRET, SIGNAL(toggled(bool)), LineEditFRET, SLOT(setEnabled(bool)));  
  ///// Debug
  connect(CheckBoxUPDT, SIGNAL(toggled(bool)), this, SLOT(enableUPDTWidgets(bool)));
  connect(CheckBoxSHRT, SIGNAL(toggled(bool)), this, SLOT(enableSHRTWidgets(bool)));
        
  ///// connections for changes
  ///// Basic
  connect(ComboBoxType, SIGNAL(activated(int)), this, SLOT(setChanged()));
  connect(CheckBoxAutogen, SIGNAL(clicked()), this, SLOT(setChanged()));
  connect(CheckBoxRegen, SIGNAL(clicked()), this, SLOT(setChanged()));
  connect(SpinBoxGenFreq, SIGNAL(valueChanged(int)), this, SLOT(setChanged()));
  connect(CheckBoxHBonds, SIGNAL(clicked()), this, SLOT(setChanged()));
  connect(RadioButtonSFAC1, SIGNAL(clicked()), this, SLOT(setChanged()));
  connect(RadioButtonSFAC2, SIGNAL(clicked()), this, SLOT(setChanged()));
  connect(LineEditSFAC1, SIGNAL(textChanged(const QString&)), this, SLOT(setChanged()));
  connect(LineEditSFAC2, SIGNAL(textChanged(const QString&)), this, SLOT(setChanged()));
  connect(ToolButtonSFAC1, SIGNAL(clicked()), this, SLOT(setChanged()));
  connect(ToolButtonSFAC2, SIGNAL(clicked()), this, SLOT(setChanged()));
  connect(SpinBoxMAXD, SIGNAL(valueChanged(int)), this, SLOT(setChanged()));
  connect(CheckBoxSUMM, SIGNAL(clicked()), this, SLOT(setChanged()));
  connect(SpinBoxSteps, SIGNAL(valueChanged(int)), this, SLOT(setChanged()));
  ///// Internal coordinates
  connect(ComboBoxICType, SIGNAL(activated(int)), this, SLOT(setChanged()));
  connect(LineEditFC, SIGNAL(textChanged(const QString&)), this, SLOT(setChanged()));
  connect(CheckBoxFIXC, SIGNAL(clicked()), this, SLOT(setChanged()));
  connect(CheckBoxMAXI, SIGNAL(clicked()), this, SLOT(setChanged()));
  connect(CheckBoxTORO, SIGNAL(clicked()), this, SLOT(setChanged()));
  connect(LineEditTORO, SIGNAL(textChanged(const QString&)), this, SLOT(setChanged()));
  connect(LineEditICWeight, SIGNAL(textChanged(const QString&)), this, SLOT(setChanged()));
  connect(SpinBoxICAtom1, SIGNAL(valueChanged(int)), this, SLOT(setChanged()));
  connect(SpinBoxICAtom2, SIGNAL(valueChanged(int)), this, SLOT(setChanged()));
  connect(SpinBoxICAtom3, SIGNAL(valueChanged(int)), this, SLOT(setChanged()));
  connect(SpinBoxICAtom4, SIGNAL(valueChanged(int)), this, SLOT(setChanged()));
  ///// Cartesian coordinates - Symmetry
  connect(CheckBoxSYMM, SIGNAL(clicked()), this, SLOT(setChanged()));
  connect(CheckBoxSYMMx, SIGNAL(clicked()), this, SLOT(setChanged()));
  connect(CheckBoxSYMMy, SIGNAL(clicked()), this, SLOT(setChanged()));
  connect(CheckBoxSYMMz, SIGNAL(clicked()), this, SLOT(setChanged()));
  connect(CheckBoxSYMMxy, SIGNAL(clicked()), this, SLOT(setChanged()));
  connect(CheckBoxSYMMxz, SIGNAL(clicked()), this, SLOT(setChanged()));
  connect(CheckBoxSYMMyz, SIGNAL(clicked()), this, SLOT(setChanged()));
  connect(CheckBoxSYMMxyz, SIGNAL(clicked()), this, SLOT(setChanged()));
  ///// Cartesian coordinates - Other
  connect(SpinBoxFIXA, SIGNAL(valueChanged(int)), this, SLOT(setChanged()));
  connect(CheckBoxFIXA1, SIGNAL(clicked()), this, SLOT(setChanged()));
  connect(CheckBoxFIXA2, SIGNAL(clicked()), this, SLOT(setChanged()));
  connect(CheckBoxFIXA3, SIGNAL(clicked()), this, SLOT(setChanged()));
  connect(CheckBoxTROT, SIGNAL(clicked()), this, SLOT(setChanged()));
  connect(ComboBoxTROT, SIGNAL(activated(int)), this, SLOT(setChanged()));
  connect(LineEditXTFK1, SIGNAL(textChanged(const QString&)), this, SLOT(setChanged()));
  connect(LineEditXTFK2, SIGNAL(textChanged(const QString&)), this, SLOT(setChanged()));
  connect(LineEditXTFK3, SIGNAL(textChanged(const QString&)), this, SLOT(setChanged()));
  connect(LineEditXTFK4, SIGNAL(textChanged(const QString&)), this, SLOT(setChanged()));
  connect(LineEditXTFK5, SIGNAL(textChanged(const QString&)), this, SLOT(setChanged()));
  connect(LineEditXTFK6, SIGNAL(textChanged(const QString&)), this, SLOT(setChanged()));
  connect(CheckBoxSIGN, SIGNAL(clicked()), this, SLOT(setChanged()));
  connect(CheckBoxANIM, SIGNAL(clicked()), this, SLOT(setChanged()));
  ///// Advanced - Thresholds      
  connect(LineEditREFT1, SIGNAL(textChanged(const QString&)), this, SLOT(setChanged()));
  connect(LineEditREFT2, SIGNAL(textChanged(const QString&)), this, SLOT(setChanged()));
  connect(LineEditREFT3, SIGNAL(textChanged(const QString&)), this, SLOT(setChanged()));
  connect(LineEditREFT4, SIGNAL(textChanged(const QString&)), this, SLOT(setChanged()));
  connect(LineEditREFT5, SIGNAL(textChanged(const QString&)), this, SLOT(setChanged()));
  connect(SpinBoxITER, SIGNAL(valueChanged(int)), this, SLOT(setChanged()));
  connect(LineEditTHRE1, SIGNAL(textChanged(const QString&)), this, SLOT(setChanged()));
  connect(LineEditTHRE2, SIGNAL(textChanged(const QString&)), this, SLOT(setChanged()));
  connect(LineEditTHRE3, SIGNAL(textChanged(const QString&)), this, SLOT(setChanged()));
  connect(LineEditTHRE4, SIGNAL(textChanged(const QString&)), this, SLOT(setChanged()));
  ///// Advanced - Other
  connect(SpinBoxXBON1, SIGNAL(valueChanged(int)), this, SLOT(setChanged()));
  connect(SpinBoxXBON2, SIGNAL(valueChanged(int)), this, SLOT(setChanged()));
  connect(ButtonGroupMASS, SIGNAL(clicked(int)), this, SLOT(setChanged()));
  connect(CheckBoxFRET, SIGNAL(clicked()), this, SLOT(setChanged()));
  connect(LineEditFRET, SIGNAL(textChanged(const QString&)), this, SLOT(setChanged()));
  connect(CheckBoxORDA, SIGNAL(clicked()), this, SLOT(setChanged()));
  ///// Debug
  connect(CheckBoxGOON, SIGNAL(clicked()), this, SLOT(setChanged()));
  connect(CheckBoxLONG, SIGNAL(clicked()), this, SLOT(setChanged()));
  connect(CheckBoxUPDT, SIGNAL(clicked()), this, SLOT(setChanged()));
  connect(ComboBoxUPDT, SIGNAL(activated(int)), this, SLOT(setChanged()));
  connect(LineEditUPDT, SIGNAL(textChanged(const QString&)), this, SLOT(setChanged()));
  connect(CheckBoxSHRT, SIGNAL(clicked()), this, SLOT(setChanged()));
  connect(LineEditSHRT1, SIGNAL(textChanged(const QString&)), this, SLOT(setChanged()));
  connect(LineEditSHRT2, SIGNAL(textChanged(const QString&)), this, SLOT(setChanged()));
  connect(LineEditSHRT3, SIGNAL(textChanged(const QString&)), this, SLOT(setChanged()));
  ///// Extra
  connect(Table, SIGNAL(valueChanged(int, int)), this, SLOT(setChanged()));
  connect(ToolButtonExtraAdd, SIGNAL(clicked()), this, SLOT(setChanged()));
  // Remove's and Clear's changes are handled in the slots removeRow and clearSelection

  ///// connections for Basic
  connect(ToolButtonSFAC1, SIGNAL(clicked()), this, SLOT(newScaleFactor()));
  connect(ToolButtonSFAC2, SIGNAL(clicked()), this, SLOT(deleteScaleFactor()));  
  
  ///// connections for Internal coordinates
  connect(ToolButtonICAdd, SIGNAL(clicked()), this, SLOT(newIC()));
  connect(ToolButtonICAdd2, SIGNAL(clicked()), this, SLOT(newICPart()));
  connect(ToolButtonICRemove, SIGNAL(clicked()), this, SLOT(deleteIC()));
  connect(ToolButtonICClear, SIGNAL(clicked()), this, SLOT(clearIC()));
  connect(ToolButtonICGen, SIGNAL(clicked()), this, SLOT(genIC()));
  connect(ToolButtonICUp, SIGNAL(clicked()), this, SLOT(moveICUp()));
  connect(ToolButtonICDown, SIGNAL(clicked()), this, SLOT(moveICDown()));
  connect(ListViewIC, SIGNAL(selectionChanged(QListViewItem*)), this, SLOT(updateICEdit()));
  connect(LineEditFC, SIGNAL(textChanged(const QString&)), this, SLOT(updateICList()));
  connect(CheckBoxFIXC, SIGNAL(clicked()), this, SLOT(updateICList()));
  connect(CheckBoxMAXI, SIGNAL(clicked()), this, SLOT(updateICList()));
  connect(CheckBoxTORO, SIGNAL(clicked()), this, SLOT(updateICList()));
  connect(LineEditTORO, SIGNAL(textChanged(const QString&)), this, SLOT(updateICList()));
  connect(LineEditICWeight, SIGNAL(textChanged(const QString&)), this, SLOT(updateICList()));
  connect(SpinBoxICAtom1, SIGNAL(valueChanged(int)), this, SLOT(updateICList()));
  connect(SpinBoxICAtom2, SIGNAL(valueChanged(int)), this, SLOT(updateICList()));
  connect(SpinBoxICAtom3, SIGNAL(valueChanged(int)), this, SLOT(updateICList()));
  connect(SpinBoxICAtom4, SIGNAL(valueChanged(int)), this, SLOT(updateICList()));
  connect(ComboBoxICType, SIGNAL(activated(int)), this, SLOT(updateICType()));

  ///// connections for Cartesian coordiantes - Other
  connect(ToolButtonFIXA1, SIGNAL(clicked()), this, SLOT(newFixedAtom()));
  connect(ToolButtonFIXA2, SIGNAL(clicked()), this, SLOT(deleteFixedAtom()));  
  connect(CheckBoxFIXA1, SIGNAL(clicked()), this, SLOT(checkFIXABoxes()));
  connect(CheckBoxFIXA2, SIGNAL(clicked()), this, SLOT(checkFIXABoxes()));
  connect(CheckBoxFIXA3, SIGNAL(clicked()), this, SLOT(checkFIXABoxes()));

  ///// connections for Advanced - Other
  connect(ToolButtonXBON1, SIGNAL(clicked()), this, SLOT(newExtraBond()));
  connect(ToolButtonXBON2, SIGNAL(clicked()), this, SLOT(deleteExtraBond()));
    
  ///// connections for Extra
  connect(Table, SIGNAL(valueChanged(int, int)), this, SLOT(checkOverflow(int, int)));
  connect(ToolButtonExtraAdd, SIGNAL(clicked()), this, SLOT(addExtraRow()));
  connect(ToolButtonExtraRemove, SIGNAL(clicked()), this, SLOT(removeExtraRow()));
  connect(ToolButtonExtraClear, SIGNAL(clicked()), this, SLOT(clearSelection()));
  
  /*
  connect(CheckBox, SIGNAL(clicked()), this, SLOT(setChanged()));
  connect(SpinBox, SIGNAL(valueChanged(int)), this, SLOT(setChanged()));
  connect(ComboBox, SIGNAL(activated(int)), this, SLOT(setChanged()));
  connect(LineEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setChanged()));
  */
 
}

///// init ////////////////////////////////////////////////////////////////////
void RelaxBase::init()
/// Initializes the dialog. Called once from the
/// constructor. Connections should be up.
{

  ///// fill the category array (the order of the strings must coincide with the order in the enum Categories
  //category.clear();
  //TODO: setsize to the desired final size => done in header file
  category = vector<QString>(10);
  category.at(BASIC) = tr("Basic");
  category.at(INTERNAL) = tr("Internal coordinates");
  category.at(CARTESIAN) = tr("Cartesian coordinates");
  category.at(ADVANCED) = tr("Advanced");
  category.at(DEBUG1) = tr("Debug");
  category.at(EXTRA) = tr("Extra");
  category.at(CARTESIAN_SYMMETRY) = tr("Symmetry");
  category.at(CARTESIAN_OTHER) = tr("Other");
  category.at(ADVANCED_THRESHOLDS) = tr("Thresholds");
  category.at(ADVANCED_OTHER) = tr("Other");

  ///// setup ListViewCategory (setSorting can't be set in Designer, so items are always sorted)
  ListViewCategory->clear();
  ListViewCategory->setSorting(-1);
  ListViewCategory->header()->hide();
  ///// add all items in reverse order as they are added at the top
  QListViewItem* rootItem;
  ///// Extra
  new QListViewItem(ListViewCategory, category[EXTRA]);
  ///// Debug
  new QListViewItem(ListViewCategory, category[DEBUG1]);
  ///// Advanced
  rootItem = new QListViewItem(ListViewCategory, category[ADVANCED]);
    new QListViewItem(rootItem, category[ADVANCED_OTHER]);
    new QListViewItem(rootItem, category[ADVANCED_THRESHOLDS]);
  ///// Cartesian coordinates
  rootItem = new QListViewItem(ListViewCategory, category[CARTESIAN]);
    new QListViewItem(rootItem, category[CARTESIAN_OTHER]);
    new QListViewItem(rootItem, category[CARTESIAN_SYMMETRY]);
  ///// Internal coordinates
  new QListViewItem(ListViewCategory, category[INTERNAL]);  
  ///// Basic
  rootItem = new QListViewItem(ListViewCategory, category[BASIC]);
  ///// select Basic
  ListViewCategory->setSelected(rootItem, true);
  ///// Done
  rootItem = 0;
  ///// calculate the needed width of the listview
  QListViewItemIterator it(ListViewCategory);
  int minWidth = 0;
  while(it.current())
  {
    int itemWidth = it.current()->width(fontMetrics(), ListViewCategory, 0);
    if(itemWidth > minWidth)
      minWidth = itemWidth;
    it++;
  }
  ListViewCategory->setFixedWidth(minWidth + 2*ListViewCategory->treeStepSize());

  ///// add validators to LineEdit widgets
  ///// a double value which should fit into a 10 character field
  // positive or negative
  QDoubleValidator* v = new QDoubleValidator(-99999999.0,999999999.,9,this);
  LineEditFRET->setValidator(v);
  LineEditTORO->setValidator(v);
  LineEditICWeight->setValidator(v);
  // positive only
  v = new QDoubleValidator(0.0,999999999.,9,this);
  LineEditSFAC1->setValidator(v);
  LineEditSFAC2->setValidator(v);
  LineEditFC->setValidator(v);
  LineEditXTFK1->setValidator(v);
  LineEditXTFK2->setValidator(v);
  LineEditXTFK3->setValidator(v);
  LineEditXTFK4->setValidator(v);
  LineEditXTFK5->setValidator(v);
  LineEditXTFK6->setValidator(v);
  LineEditREFT1->setValidator(v);
  LineEditREFT2->setValidator(v);
  LineEditREFT3->setValidator(v);
  LineEditREFT4->setValidator(v);
  LineEditREFT5->setValidator(v);
  LineEditTHRE1->setValidator(v);
  LineEditTHRE2->setValidator(v);
  LineEditTHRE3->setValidator(v);
  LineEditTHRE4->setValidator(v);
  LineEditUPDT->setValidator(v);
  LineEditSHRT1->setValidator(v);
  LineEditSHRT2->setValidator(v);
  LineEditSHRT3->setValidator(v); 
  v = 0;

  ///// assign icons
  ToolButtonSFAC1->setIconSet(IconSets::getIconSet(IconSets::ArrowDown));
  ToolButtonSFAC2->setIconSet(IconSets::getIconSet(IconSets::ArrowUp));
  ToolButtonICAdd->setIconSet(IconSets::getIconSet(IconSets::New));
  ToolButtonICAdd2->setIconSet(IconSets::getIconSet(IconSets::NewPart));
  ToolButtonICGen->setIconSet(IconSets::getIconSet(IconSets::Generate));
  ToolButtonICRemove->setIconSet(IconSets::getIconSet(IconSets::Cut));
  ToolButtonICClear->setIconSet(IconSets::getIconSet(IconSets::Clear));
  ToolButtonICUp->setIconSet(IconSets::getIconSet(IconSets::ArrowUp));
  ToolButtonICDown->setIconSet(IconSets::getIconSet(IconSets::ArrowDown));
  ToolButtonFIXA1->setIconSet(IconSets::getIconSet(IconSets::ArrowRight));
  ToolButtonFIXA2->setIconSet(IconSets::getIconSet(IconSets::ArrowLeft));
  ToolButtonXBON1->setIconSet(IconSets::getIconSet(IconSets::ArrowRight));
  ToolButtonXBON2->setIconSet(IconSets::getIconSet(IconSets::ArrowLeft));
  ToolButtonExtraAdd->setIconSet(IconSets::getIconSet(IconSets::New));
  ToolButtonExtraRemove->setIconSet(IconSets::getIconSet(IconSets::Cut));
  ToolButtonExtraClear->setIconSet(IconSets::getIconSet(IconSets::Clear));

  ///// assign pixmaps
  CheckBoxSYMMx->setPixmap(IconSets::getPixmap(IconSets::SymmX));
  CheckBoxSYMMy->setPixmap(IconSets::getPixmap(IconSets::SymmY));
  CheckBoxSYMMz->setPixmap(IconSets::getPixmap(IconSets::SymmZ));
  CheckBoxSYMMxy->setPixmap(IconSets::getPixmap(IconSets::SymmXY));
  CheckBoxSYMMxz->setPixmap(IconSets::getPixmap(IconSets::SymmXZ));
  CheckBoxSYMMyz->setPixmap(IconSets::getPixmap(IconSets::SymmYZ));
  CheckBoxSYMMxyz->setPixmap(IconSets::getPixmap(IconSets::SymmXYZ));

  ///// disable sorting of all QListView's
  ListViewSFAC->setSorting(-1);
  ListViewIC->setSorting(-1);
  ListViewFIXA->setSorting(-1);
  ListViewXBON->setSorting(-1);
  
  ///// give the dialog the smallest possible size
  resize(1,1);

  ///// set widget and data to defaults
  reset();
  saveWidgets();
  widgetChanged = false;
}

///// saveWidgets /////////////////////////////////////////////////////////////
void RelaxBase::saveWidgets()
/// Saves the status of the widgets to a data struct.
{
  ///// Basic
  data.type = ComboBoxType->currentItem();
  data.autoGenerate = CheckBoxAutogen->isChecked();
  data.autoRegen = CheckBoxRegen->isChecked();
  data.autoGenFreq = SpinBoxGenFreq->value();
  data.autoHBonds = CheckBoxHBonds->isChecked();
  data.useOneScaleFactor = RadioButtonSFAC1->isChecked();
  data.sfacValue1 = LineEditSFAC1->text();
  data.sfacValue2 = LineEditSFAC2->text();
  data.sfacNumSteps = SpinBoxSFAC->value();
  data.listValues.clear();
  data.listNumSteps.clear();
  {
    QListViewItemIterator it(ListViewSFAC);
    for( ; it.current(); it++)
    {
      data.listValues += it.current()->text(0);
      data.listNumSteps.push_back(it.current()->text(1).toUInt());
    }
  }
  data.numDIIS = SpinBoxMAXD->value();
  data.printShifts = CheckBoxSUMM->isChecked();
  data.numSteps = SpinBoxSteps->value();

  ///// Internal coordinates
  data.ic.clear();
  {
    QListViewItemIterator it(ListViewIC);
    ICData tempIC;
    for ( ; it.current(); it++)
    {
      if(!it.current()->text(0).isEmpty())
      {
        ///// root item
        tempIC.number = it.current()->text(0).toUInt();
        tempIC.rootItem = true;
      
        tempIC.type = it.current()->text(1);
        tempIC.fc = it.current()->text(2);
        tempIC.fix = it.current()->pixmap(3) != 0;
        tempIC.maximize = it.current()->pixmap(4) != 0;
        tempIC.refValue = it.current()->text(5);
      }
      else
      {
        ///// sub item
        tempIC.number = 0;
        tempIC.rootItem = false;
      }   
      tempIC.weight = it.current()->text(6);
      tempIC.atom1 = it.current()->text(7).section("-", 0, 0, QString::SectionSkipEmpty).replace(QRegExp("[A-Z, a-z]"), "").toUInt();
      tempIC.atom2 = it.current()->text(7).section("-", 1, 1, QString::SectionSkipEmpty).replace(QRegExp("[A-Z, a-z]"), "").toUInt();
      tempIC.atom3 = it.current()->text(7).section("-", 2, 2, QString::SectionSkipEmpty).replace(QRegExp("[A-Z, a-z]"), "").toUInt();
      tempIC.atom4 = it.current()->text(7).section("-", 3, 3, QString::SectionSkipEmpty).replace(QRegExp("[A-Z, a-z]"), "").toUInt();
      data.ic.push_back(tempIC);
    }
  }

  ///// Cartesian coordinates - Symmetry
  data.checkSymmetry = CheckBoxSYMM->isChecked();
  data.checkSymmX = CheckBoxSYMMx->isChecked();
  data.checkSymmY = CheckBoxSYMMy->isChecked();
  data.checkSymmZ = CheckBoxSYMMz->isChecked();
  data.checkSymmXY = CheckBoxSYMMxy->isChecked();
  data.checkSymmXZ = CheckBoxSYMMxz->isChecked();
  data.checkSymmYZ = CheckBoxSYMMyz->isChecked();
  data.checkSymmXYZ = CheckBoxSYMMxyz->isChecked();

  ///// Cartesian coordinates - Other
  data.fixAtom = SpinBoxFIXA->value();
  data.fixX = CheckBoxFIXA1->isChecked();
  data.fixY = CheckBoxFIXA2->isChecked();
  data.fixZ = CheckBoxFIXA3->isChecked();
  data.listFixedAtoms.clear();
  data.listFixedXYZ.clear();
  {
    QListViewItemIterator it(ListViewFIXA);
    for ( ; it.current(); it++)
    {
      data.listFixedAtoms.push_back(it.current()->text(0).replace(QRegExp("[A-Z, a-z]"), "").toUInt());
      data.listFixedXYZ += it.current()->text(1);
    }
  }
  data.optExternal = CheckBoxTROT->isChecked();
  data.optExternalAngle = ComboBoxTROT->currentItem();
  data.optExternalA = LineEditXTFK1->text();
  data.optExternalB = LineEditXTFK2->text();
  data.optExternalC = LineEditXTFK3->text();
  data.optExternalAlpha = LineEditXTFK4->text();
  data.optExternalBeta  = LineEditXTFK5->text();
  data.optExternalGamma = LineEditXTFK6->text();
  data.mirrorImage = CheckBoxSIGN->isChecked();
  data.writeXYZ = CheckBoxANIM->isChecked();

  ///// Advanced - Thresholds
  data.refineThre1 = LineEditREFT1->text();
  data.refineThre2 = LineEditREFT2->text();
  data.refineThre3 = LineEditREFT3->text();
  data.refineThre4 = LineEditREFT4->text();
  data.refineThre5 = LineEditREFT5->text();
  data.maxIter = SpinBoxITER->value();
  data.iterThre1 = LineEditTHRE1->text();
  data.iterThre2 = LineEditTHRE2->text();
  data.iterThre3 = LineEditTHRE3->text();
  data.iterThre4 = LineEditTHRE4->text();

  ///// Advanced - Other
  data.xbonAtom1 = SpinBoxXBON1->value();
  data.xbonAtom2 = SpinBoxXBON2->value();
  data.listXbonAtoms1.clear();
  data.listXbonAtoms2.clear();
  {
    QListViewItemIterator it(ListViewXBON);
    for ( ; it.current(); it++)
    {
      data.listXbonAtoms1.push_back(it.current()->text(0).replace(QRegExp("[A-Z, a-z]"), "").toUInt());
      data.listXbonAtoms2.push_back(it.current()->text(1).replace(QRegExp("[A-Z, a-z]"), "").toUInt());
    }
  }
  data.massType = ButtonGroupMASS->id(ButtonGroupMASS->selected());
  data.setLowFreqs = CheckBoxFRET->isChecked();
  data.lowFreqs = LineEditFRET->text();
  data.orden = CheckBoxORDA->isChecked();

  ///// Debug
  data.goon = CheckBoxGOON->isChecked();
  data.printDebug = CheckBoxLONG->isChecked();
  data.update = CheckBoxUPDT->isChecked();
  data.updateType = ComboBoxUPDT->currentItem();
  data.updateScaleFactor = LineEditUPDT->text();
  data.compact = CheckBoxSHRT->isChecked();
  data.compactBonds = LineEditSHRT1->text();
  data.compactAngles = LineEditSHRT2->text();
  data.compactTorsions = LineEditSHRT3->text();

  ///// Extra
  saveTable(Table, data.numLines, data.hPos, data.vPos, data.contents);
}

///// restoreWidgets //////////////////////////////////////////////////////////
void RelaxBase::restoreWidgets()
/// Restores the status of the widgets from a data struct.
{
  ///// Basic
  ComboBoxType->setCurrentItem(data.type);
  CheckBoxAutogen->setChecked(data.autoGenerate);
  CheckBoxRegen->setChecked(data.autoRegen);
  SpinBoxGenFreq->setValue(data.autoGenFreq);
  CheckBoxHBonds->setChecked(data.autoHBonds);
  RadioButtonSFAC1->setChecked(data.useOneScaleFactor);
  RadioButtonSFAC2->setChecked(!data.useOneScaleFactor);
  LineEditSFAC1->setText(data.sfacValue1);
  LineEditSFAC2->setText(data.sfacValue2);
  SpinBoxSFAC->setValue(data.sfacNumSteps);
  ListViewSFAC->clear();
  { for(unsigned int i = 0; i < data.listNumSteps.size(); i++)
      new QListViewItem(ListViewSFAC, data.listValues[i], QString::number(data.listNumSteps[i])); }      
  SpinBoxMAXD->setValue(data.numDIIS);
  CheckBoxSUMM->setChecked(data.printShifts);
  SpinBoxSteps->setValue(data.numSteps);

  ///// Internal coordinates
  ListViewIC->clear();
  {
    QListViewItem* tempRoot = 0;
    QListViewItem* tempItem = 0;
    for(unsigned int i = 0; i < data.ic.size(); i++)
    {
      if(data.ic[i].rootItem)
      {
        ///// root item
        tempItem = new QListViewItem(ListViewIC);
        tempItem->setText(0, QString::number(data.ic[i].number));
        tempItem->setText(1, data.ic[i].type);
        tempItem->setText(2, data.ic[i].fc);
        if(data.ic[i].fix)
          tempItem->setPixmap(3, IconSets::getPixmap(IconSets::OK));
        if(data.ic[i].maximize)
          tempItem->setPixmap(4, IconSets::getPixmap(IconSets::OK));      
        tempItem->setText(5, data.ic[i].refValue);

        tempRoot = tempItem;      
      }
      else
      {
        ///// sub item
        ASSERT(tempRoot != 0);
        tempItem = new QListViewItem(tempRoot);
      }
      tempItem->setText(6, data.ic[i].weight);
      tempItem->setText(7, icRepresentation(data.ic[i].atom1, data.ic[i].atom2, data.ic[i].atom3, data.ic[i].atom4));
    }
  }

  ///// Cartesian coordinates - Symmetry
  CheckBoxSYMM->setChecked(data.checkSymmetry);
  CheckBoxSYMMx->setChecked(data.checkSymmX);
  CheckBoxSYMMy->setChecked(data.checkSymmY);
  CheckBoxSYMMz->setChecked(data.checkSymmZ);
  CheckBoxSYMMxy->setChecked(data.checkSymmXY);
  CheckBoxSYMMxz->setChecked(data.checkSymmXZ);
  CheckBoxSYMMyz->setChecked(data.checkSymmYZ);
  CheckBoxSYMMxyz->setChecked(data.checkSymmXYZ);

  ///// Cartesian coordinates - Other
  SpinBoxFIXA->setValue(data.fixAtom);
  CheckBoxFIXA1->setChecked(data.fixX);
  CheckBoxFIXA2->setChecked(data.fixY);
  CheckBoxFIXA3->setChecked(data.fixZ);
  ListViewFIXA->clear();
  { for(unsigned int i = 0; i < data.listFixedAtoms.size(); i++)
      new QListViewItem(ListViewFIXA, AtomSet::numToAtom(atoms->atomicNumber(data.listFixedAtoms[i]-1)).stripWhiteSpace() + QString::number(data.listFixedAtoms[i]), data.listFixedXYZ[i]); }
  CheckBoxTROT->setChecked(data.optExternal);
  ComboBoxTROT->setCurrentItem(data.optExternalAngle);
  LineEditXTFK1->setText(data.optExternalA);
  LineEditXTFK2->setText(data.optExternalB);
  LineEditXTFK3->setText(data.optExternalC);
  LineEditXTFK4->setText(data.optExternalAlpha);
  LineEditXTFK5->setText(data.optExternalBeta);
  LineEditXTFK6->setText(data.optExternalGamma);
  CheckBoxSIGN->setChecked(data.mirrorImage);
  CheckBoxANIM->setChecked(data.writeXYZ);

  ///// Advanced - Thresholds
  LineEditREFT1->setText(data.refineThre1);
  LineEditREFT2->setText(data.refineThre2);
  LineEditREFT3->setText(data.refineThre3);
  LineEditREFT4->setText(data.refineThre4);
  LineEditREFT5->setText(data.refineThre5);
  SpinBoxITER->setValue(data.maxIter);
  LineEditTHRE1->setText(data.iterThre1);
  LineEditTHRE2->setText(data.iterThre2);
  LineEditTHRE3->setText(data.iterThre3);
  LineEditTHRE4->setText(data.iterThre4);

  ///// Advanced - Other
  SpinBoxXBON1->setValue(data.xbonAtom1);
  SpinBoxXBON2->setValue(data.xbonAtom2);
  ListViewXBON->clear();
  {
    for(unsigned int i = 0; i < data.listXbonAtoms1.size(); i++)
      new QListViewItem(ListViewXBON, AtomSet::numToAtom(atoms->atomicNumber(data.listXbonAtoms1[i]-1)).stripWhiteSpace() + QString::number(data.listXbonAtoms1[i]),
                                      AtomSet::numToAtom(atoms->atomicNumber(data.listXbonAtoms2[i]-1)).stripWhiteSpace() + QString::number(data.listXbonAtoms2[i]));
  }
  ButtonGroupMASS->setButton(data.massType);
  CheckBoxFRET->setChecked(data.setLowFreqs);
  LineEditFRET->setText(data.lowFreqs);
  CheckBoxORDA->setChecked(data.orden);

  ///// Debug
  CheckBoxGOON->setChecked(data.goon);
  CheckBoxLONG->setChecked(data.printDebug);
  CheckBoxUPDT->setChecked(data.update);
  ComboBoxUPDT->setCurrentItem(data.updateType);
  LineEditUPDT->setText(data.updateScaleFactor);
  CheckBoxSHRT->setChecked(data.compact);
  LineEditSHRT1->setText(data.compactBonds);
  LineEditSHRT2->setText(data.compactAngles);
  LineEditSHRT3->setText(data.compactTorsions);

  ///// Extra
  restoreTable(Table, data.numLines, data.hPos, data.vPos, data.contents);  
}

///// icRepresentation ////////////////////////////////////////////////////////
QString RelaxBase::icRepresentation(const unsigned int atom1, const unsigned int atom2, const unsigned int atom3, const int atom4)
/// Returns a QString containing a representation
/// of an internal coordinate from the 4 atom indices.
/// A zero value will not be added.
{
  QString result =  AtomSet::numToAtom(atoms->atomicNumber(atom1 - 1)).stripWhiteSpace() + QString::number(atom1);
  result +=   "-" + AtomSet::numToAtom(atoms->atomicNumber(atom2 - 1)).stripWhiteSpace() + QString::number(atom2);
  if(atom3 != 0)
    result += "-" + AtomSet::numToAtom(atoms->atomicNumber(atom3 - 1)).stripWhiteSpace() + QString::number(atom3);
  if(atom4 != 0)
  {
    if(atom4 == -1) 
      result += "-X";
    else if(atom4 == -2)
      result += "-Y";
    else if(atom4 == -3)
      result += "-Z";
    else
      result += "-" + AtomSet::numToAtom(atoms->atomicNumber(atom4 - 1)).stripWhiteSpace() + QString::number(atom4);
  }
  return result;  
}

///// getAtomsFromIC //////////////////////////////////////////////////////////
void RelaxBase::getAtomsFromIC(const QString textIC, unsigned int& atom1, unsigned int& atom2, unsigned int& atom3, int& atom4)
/// Returns the atom numbers from a textual representation of an internal coordinate.
{
  atom1 = textIC.section("-", 0, 0, QString::SectionSkipEmpty).replace(QRegExp("[A-Z, a-z]"), "").toUInt();
  atom2 = textIC.section("-", 1, 1, QString::SectionSkipEmpty).replace(QRegExp("[A-Z, a-z]"), "").toUInt();
  atom3 = textIC.section("-", 2, 2, QString::SectionSkipEmpty).replace(QRegExp("[A-Z, a-z]"), "").toUInt();
  atom4 = textIC.section("-", 3, 3, QString::SectionSkipEmpty).replace(QRegExp("[A-Z, a-z]"), "").toUInt();

  if(atom4 == 0)
  {
    QString section4 = textIC.section("-", 3, 3, QString::SectionSkipEmpty);
    if(!section4.isEmpty())
    {
      if(section4 == "X")
        atom4 = -1;
      else if(section4 == "Y")
        atom4 = -2;
      else if(section4 == "Z")
        atom4 = -3;
    }
  }
}

///// renumberICs /////////////////////////////////////////////////////////////
void RelaxBase::renumberICs()
/// Renumbers the internal coordinates.
{  
  unsigned int icNumber = 0;
  ListViewIC->blockSignals(true);
  QListViewItem* item = ListViewIC->firstChild();
  while(item != 0)
  {
    icNumber++;
    item->setText(0, QString::number(icNumber));
    item = item->nextSibling();
  }
  ListViewIC->blockSignals(false);
}

///// generateAFFHeader ///////////////////////////////////////////////////////
QStringList RelaxBase::generateAFFHeader()
/// Generates input for the first part of an .aff file.
{
  QStringList result;
  
  result += "! " + tr("This file has been generated by") + " " + Version::appName + " " + Version::appVersion;
  result += "! " + tr("All changes made in this file will be lost!");
  ///// NATO
  result += inputLine("nato", static_cast<int>(atoms->count()));
  ///// TEXT
  result  += "text      " + calcDescription;
  ///// LINE
  if(atoms->isLinear())
    result += "line";
  ///// SFCR
  if(calcXF)
  {
    result += "xfcr";
    result += "xffo";
  }
      
  ///// Basic /////
  ///// MAXD
  if(SpinBoxMAXD->value() != 0)
    result += inputLine("maxd", SpinBoxMAXD->value());
  ///// SUMM
  if(CheckBoxSUMM->isChecked())
    result += "summ";
  ///// FDIA ->always
  result += "fdia";

  ///// Internal coordinates /////
  ///// FIXC
  QListViewItem* item = ListViewIC->firstChild();
  while(item != 0)
  {
    if(item->pixmap(3) != 0)
      result += inputLine("fixc", item->text(0).toInt());  
    item = item->nextSibling();
  }
  ///// MAXI
  item = ListViewIC->firstChild();
  while(item != 0)
  {
    if(item->pixmap(4) != 0)
      result += inputLine("maxi", item->text(0).toInt());
    item = item->nextSibling();
  }
  ///// TORO
  item = ListViewIC->firstChild();
  while(item != 0)
  {
    if(!item->text(5).isEmpty())
      result += inputLine("toro", item->text(0).toInt(), item->text(5).toDouble());
    item = item->nextSibling();
  }  
  
  ///// Cartesian coordinates - Symmetry /////
  if(CheckBoxSYMM->isChecked())
    result += validatedSYMMLine(CheckBoxSYMMx, CheckBoxSYMMy, CheckBoxSYMMz, CheckBoxSYMMxy, CheckBoxSYMMxz, CheckBoxSYMMyz, CheckBoxSYMMxyz);

  ///// Cartesian coordinates - Other /////
  ///// FIXA
  {
    QListViewItemIterator it(ListViewFIXA);
    for( ; it.current(); it++)
      result += inputLine("fixa", it.current()->text(0).replace(QRegExp("[A-Z, a-z]"), "").toInt()) + it.current()->text(1);
  }
  ///// TROT
  if(CheckBoxTROT->isChecked())
  {
    result += inputLine("trot", ComboBoxTROT->currentItem() + 1);
    result += "xtfk";
    QString temp1 =  QString::number(LineEditXTFK1->text().toDouble());
    temp1.truncate(13);
    QString temp2 = QString::number(LineEditXTFK2->text().toDouble());
    temp2.truncate(13);
    temp1 += " " + temp2;
    temp2 = QString::number(LineEditXTFK3->text().toDouble());
    temp2.truncate(13);
    temp1 += " " + temp2;
    temp2 = QString::number(LineEditXTFK4->text().toDouble());
    temp2.truncate(13);
    temp1 += " " + temp2;
    temp2 = QString::number(LineEditXTFK5->text().toDouble());
    temp2.truncate(13);
    temp1 += " " + temp2;
    temp2 = QString::number(LineEditXTFK6->text().toDouble());
    temp2.truncate(13);
    temp1 += " " + temp2;
    result += temp1;
  }
  ///// SIGN
  if(CheckBoxSIGN->isChecked())
    result += "symm";
  ///// ANIM
  if(CheckBoxANIM->isChecked())
    result += "anim";    
      
  ///// Advanced - Thresholds /////
  ///// REFT
  double reft1 = LineEditREFT1->text().toDouble();
  double reft2 = LineEditREFT2->text().toDouble();
  double reft3 = LineEditREFT3->text().toDouble();
  double reft4 = LineEditREFT4->text().toDouble();
  double reft5 = LineEditREFT5->text().toDouble();
  if((reft1 != 0.0009) || (reft2 != 0.0009) || (reft3 != 0.0009) || (reft4 != 0.0009) || (reft5 != 0.0009))
  {
    if((reft1 == reft2) && (reft2 == reft3) && (reft3 == reft4) && (reft4 == reft5))
      result += inputLine("reft", reft1);
    else
    {
      result += "reft";
      QString a, b, c, d, e;
      a.setNum(reft1,'f',10).truncate(10);
      b.setNum(reft2,'f',10).truncate(10);
      c.setNum(reft3,'f',10).truncate(10);
      d.setNum(reft4,'f',10).truncate(10);
      e.setNum(reft5,'f',10).truncate(10);
      result += a + b + c + d + e;
    }    
  }
  ///// ITER
  if(SpinBoxITER->value() != 0)
    result += inputLine("iter", SpinBoxITER->value());
  ///// THRE
  double thre1 = - log10(LineEditTHRE1->text().toDouble()*pow(10.0, - LineEditTHRE2->text().toDouble()));
  double thre2 = - log10(LineEditTHRE3->text().toDouble()*pow(10.0, - LineEditTHRE4->text().toDouble())); 
  if((fabs(thre1 - 9.3019) > 0.00001) || (fabs(thre2 - 7.0) > 0.00001))
    result += inputLine("thre", thre1, thre2);

  ///// Advanced - Other
  ///// XBON  
  {
    QListViewItemIterator it(ListViewXBON);
    while(it.current())
    {
      result += inputLine("xbon", it.current()->text(0).replace(QRegExp("[A-Z, a-z]"), "").toInt(),
                                  it.current()->text(1).replace(QRegExp("[A-Z, a-z]"), "").toInt());
      it++;
    }
  }
  ///// MASS/AVMA/none
  switch(ButtonGroupMASS->id(ButtonGroupMASS->selected()))
  {
    case 1: result += "mass";
            break; 
    case 2: result += "avma";
  }
  ///// FRET
  if(CheckBoxFRET->isChecked())
    result += inputLine("fret", LineEditFRET->text().toDouble());
  ///// ORDA
  if(CheckBoxORDA->isChecked())
    result += "orda";

  ///// Debug /////
  ///// GOON
  if(CheckBoxGOON->isChecked())
    result += "goon";
    ///// LONG
  if(CheckBoxLONG->isChecked())
    result += "long";
  ///// UPDT
  if(CheckBoxUPDT->isChecked())
    result += inputLine("updt", ComboBoxUPDT->currentItem() + 1, LineEditUPDT->text().toDouble());
  ///// SHRT
  if(CheckBoxSHRT->isChecked())
  {
    QString c, d;
    c.setNum(LineEditSHRT2->text().toDouble(),'f',10).truncate(10);
    d.setNum(LineEditSHRT3->text().toDouble(),'f',10).truncate(10);
    result += inputLine("shrt", SpinBoxSHRT->value(), LineEditSHRT1->text().toDouble()) + c + d;
  }  

  ///// Extra /////
  result += tableContents(Table);

  ///// Manually defined internal coordinates
  switch(ComboBoxType->currentItem())
  {
    case 0: result += "GBMA";
            break;
    case 1: result += "BMAT";
  }
  unsigned int icType = 0;
  double fc = 0.0;
  QListViewItemIterator it(ListViewIC);
  while(it.current())
  {
    QString line;

    ///// construct the first 10 places of the string
    if(!(it.current()->text(0).isEmpty()))
    {
      ///// root item
      for(int i = 0; i < ComboBoxICType->count(); i++)
      {
        if(it.current()->text(1) == ComboBoxICType->text(i))
        {
          icType = i;
          break;
        }
      }
      fc = it.current()->text(2).toDouble();
      if(it.current()->pixmap(4) != 0)
        fc = -fc; // if maximizing the internal coordinate, the force constant should be negative
      QString fcString; fcString.setNum(fc,'f',7).truncate(7);
      line = "K " + fcString + " ";
    }
    else
    {
      ///// sub item
      line = "          ";
    }

    ///// construct the rest
    ///// weight
    QString weightString; weightString.setNum(it.current()->text(6).toDouble(), 'f', 9).truncate(9);
    line += weightString + " ";    
    ///// type
    switch(icType)
    {
      case 0: ///// Stretch
              line += "stre      ";
              break;
      case 1: ///// Bend
              line += "bend      ";
              break;
      case 2: ///// Torsion
              line += "tors      ";
              break;
      case 3: ///// Out-of-plane
              line += "out       ";
              break;
      case 4: ///// Linear bend
              line += "lin1      ";
              break;
    }         
    
    ///// atoms
    unsigned int atom1 = it.current()->text(7).section("-", 0, 0, QString::SectionSkipEmpty).replace(QRegExp("[A-Z, a-z]"), "").toUInt();
    unsigned int atom2 = it.current()->text(7).section("-", 1, 1, QString::SectionSkipEmpty).replace(QRegExp("[A-Z, a-z]"), "").toUInt();
    unsigned int atom3 = it.current()->text(7).section("-", 2, 2, QString::SectionSkipEmpty).replace(QRegExp("[A-Z, a-z]"), "").toUInt();
    QString atom4t = it.current()->text(7).section("-", 3, 3, QString::SectionSkipEmpty);  
    QString atom1s, atom2s, atom3s, atom4s;
    atom1s.setNum(atom1); atom1s += ".         "; atom1s.truncate(10);
    atom2s.setNum(atom2); atom2s += ".         "; atom2s.truncate(10);
    atom3s.setNum(atom3); atom3s += ".         "; atom3s.truncate(10);
    if(atom4t == "X")
      atom4s = "-1.";
    else if(atom4t == "Y")
      atom4s = "-2.";
    else if(atom4t == "Z")
      atom4s = "-3.";
    else
    {
      atom4s.setNum(atom4t.replace(QRegExp("[A-Z, a-z]"), "").toUInt()); atom4s += ".         "; atom4s.truncate(10);
    }
    switch(icType)
    {
      case 0: ///// Stretch
              line += atom1s + atom2s;
              break;
      case 1: ///// Bend
              line += atom1s + atom3s + atom2s;
              break;
      case 2: ///// Torsion
              line += atom1s + atom2s + atom3s + atom4s;
              break;
      case 3: ///// Out-of-plane
              line += atom1s + atom2s + atom3s + atom4s;
              break;
      case 4: ///// Linear bend 1
              line += atom1s + atom2s + atom3s + atom4s;
    }
    result += line;
    if(icType == 4)
    {
      // linear bends always occur as pairs
      line.replace(23, 1, "2");
      result += line;
    }
    it++;
  }
  ///// finish up with some more informative messages
  if(CheckBoxAutogen->isChecked())
  {
    if(CheckBoxRegen->isChecked())
    {
      result += "! " + tr("The following internal coordinates will be automatically generated");
      result += "! " + tr("every") + " " + QString::number(SpinBoxGenFreq->value()) + " " + tr("steps");
    }
    else
      result += "! " + tr("The following internal coordinates were automatically generated");
  }
  else
    result += "STOP";

  return result;
}

///// generateBmatInput ///////////////////////////////////////////////////////
QStringList RelaxBase::generateBmatInput()
/// Generates input for Maff to generate non-redundant internal coordinates.
/// Only generates the actual input for the IC definitions.
/// The input for the initial questions is generated in generateICs.
{
  ///// retrieve the bond list
  vector<unsigned int>* first;
  vector<unsigned int>* second;
  atoms->bonds(first, second);

  if(first->size() == 0)
    return QStringList(); // no bonds

  ///// here come the actual IC definitions
  ///// input needed for defining the input: connectivity data, number of atoms, filename
  ///// input needed for starting the process: the crd file, location of the maff exe

  ///// bonds => auto
  QStringList result = "1";
  ///// torsions
  for(unsigned int bond = 0; bond < first->size(); bond++)
  {
    const unsigned int atom1 = first->operator[](bond);
    const unsigned int atom2 = second->operator[](bond);
    
    if((atoms->numberOfBonds(atom1) > 1) && (atoms->numberOfBonds(atom2) > 1))
    {
      result += "2";
      result += QString::number(atom1) + "," + QString::number(atom2);
    }
  }
  ///// h-bonds
  if(CheckBoxHBonds->isChecked())
    result += "22";

  ///// other options
  unsigned int a,b,c,d, numba, numbb, numbc, numbd;
  for(unsigned int i = 0; i < atoms->count(); i++)
  {
    ///// determine the neighbours
    vector<unsigned int> neighbours;
    for(unsigned int bond = 0; bond < first->size(); bond++)
    {
      const unsigned int atom1 = first->operator[](bond);
      const unsigned int atom2 = second->operator[](bond);
      if(atom1 == i)
        neighbours.push_back(atom2);
      else if(atom2 == i)
        neighbours.push_back(atom1);
    }
    if(neighbours.size() != atoms->numberOfBonds(i))
    {
      qDebug("Number of neighbours is not equal to number of bonds");
      qDebug("neighbours.size() = %d", static_cast<int>(neighbours.size()));
      qDebug("numberOfBonds() = %d", atoms->numberOfBonds(i));
      qDebug("first.size() = %d", static_cast<int>(first->size()));
      for(unsigned int bond = 0; bond < first->size(); bond++)
        qDebug("bond found between atoms %d and %d", first->operator[](bond), second->operator[](bond));
      qFatal("atom = %d",i);
    }

    ///// determine the environment
    ///// unsupported cases are: 10. 4-ring                  (C1234)
    /////                        11. 5-ring                  (C12345)
    /////                        12. 6-ring                  (C123456)
    /////                        14. CH2 on ring  X-CH2-Y    (C,X,Y,H,H)
    /////                        15. X=C on ring  A-(C=X)-B  (C,X,A,B)
    /////                        16. CH on 2 rings           (C,X,Y,Z,H)
    /////                        17. 7-ring                  (C1234567)
    /////                        18. spiro-atom              (X,A,B,C,D)
    /////                        20. X-C on ring  A-(C-X)-B  (C,X,A,B)
    /////                        21. butterfly               (2 ABCD torsions)
    /////                        23. 8-ring                  (C12345678)
    /////                        24. 6+ring                  (C123456789...)
    switch(atoms->numberOfBonds(i))
    {
      case 0: ///// nothing needed
      case 1: ///// nothing needed
              break;
      case 2: ///// possibilities: 13. OH-bend        X-O-H
              /////                19. linear bend
              a = atoms->atomicNumber(neighbours[0]);
              b = atoms->atomicNumber(neighbours[1]);
              if((atoms->atomicNumber(i) == 8) && ((a == 1) || (b == 1))) // really special, do not add single bonded atoms
              {
                ///// OH-bend
                result += "13";
                QString stringX, stringH, stringO;
                if(a == 1)
                  result += QString::number(neighbours[1]+1) + "," + QString::number(neighbours[0]+1) + "," + QString::number(i+1);
                else
                  result += QString::number(neighbours[0]+1) + "," + QString::number(neighbours[1]+1) + "," + QString::number(i+1);
              }
              else
              {
                ///// linear bend
                ///// choose a non-colinear D  (det x // y // z != 0)
                QString stringD = "-1";
                if(fabs(  atoms->y(neighbours[0])*atoms->z(neighbours[1])
                        - atoms->z(neighbours[0])*atoms->y(neighbours[1]) ) < 0.00001)
                  stringD = "-2";
                result += QString::number(i+1) + "," + QString::number(neighbours[0]+1) + "," + QString::number(neighbours[1]+1) + "," + stringD;
              }
              break;
      case 3: ///// possibilities: 05. methylene(sp2) H-C(=X)-H
              /////                06. methyne(sp2)   Y-C(=X)-H
              /////                08. amino          X-NH2
              /////                09. imino          X-NH-Y
              a = atoms->atomicNumber(neighbours[0]);
              b = atoms->atomicNumber(neighbours[1]);
              c = atoms->atomicNumber(neighbours[2]);
              if(atoms->atomicNumber(i) == 7) // really special, do not add halogens
              {
                ///// amino/imino
                if(a == b == 1)
                {
                  ///// amino, also handles a == b == c == 1
                  result += "8";
                  result += QString::number(i+1) + "," + QString::number(neighbours[2]+1) + "," + QString::number(neighbours[0]+1) + "," + QString::number(neighbours[1]+1);
                }
                else if(a == c == 1)
                {
                  ///// amino
                  result += "8";
                  result += QString::number(i+1) + "," + QString::number(neighbours[1]+1) + "," + QString::number(neighbours[0]+1) + "," + QString::number(neighbours[2]+1);
                }
                else if(b == c == 1)
                {
                  ///// amino
                  result += "8";
                  result += QString::number(i+1) + "," + QString::number(neighbours[0]+1) + "," + QString::number(neighbours[1]+1) + "," + QString::number(neighbours[2]+1);
                }
                else
                {
                  ///// imino => maybe check which one is the H atom?
                  result += "9";
                  result += QString::number(i+1) + "," + QString::number(neighbours[0]+1) + "," + QString::number(neighbours[1]+1) + "," + QString::number(neighbours[2]+1);
                }
              }
              else
              {
                numba = atoms->numberOfBonds(neighbours[0]);
                numbb = atoms->numberOfBonds(neighbours[1]);
                numbc = atoms->numberOfBonds(neighbours[2]);
                ///// methylene/methyne
                if((a == b) && (numba == numbb == 1))
                {
                  ///// methylene
                  result += "5";
                  result += QString::number(i+1) + "," + QString::number(neighbours[2]+1) + "," + QString::number(neighbours[0]+1) + "," + QString::number(neighbours[1]+1);

                }
                else if((a == c) && (numba == numbc == 1))
                {
                  ///// methylene
                  result += "5";
                  result += QString::number(i+1) + "," + QString::number(neighbours[1]+1) + "," + QString::number(neighbours[0]+1) + "," + QString::number(neighbours[2]+1);
                }
                else if((b == c) && (numbb == numbc == 1))
                {
                  ///// methylene
                  result += "5";
                  result += QString::number(i+1) + "," + QString::number(neighbours[0]+1) + "," + QString::number(neighbours[1]+1) + "," + QString::number(neighbours[2]+1);
                }
                else
                {
                  ///// methyne
                  result += "6";
                  result += QString::number(i+1) + "," + QString::number(neighbours[0]+1) + "," + QString::number(neighbours[1]+1) + "," + QString::number(neighbours[2]+1);
                }
              }
              break;
      case 4: ///// possibilities: 03. methyl(sp3)    X-CH3
              /////                04. methylene(sp3) X-CH2-Y
              /////                07. methyne(sp3)   H-C-XYZ
              a = atoms->atomicNumber(neighbours[0]);
              b = atoms->atomicNumber(neighbours[1]);
              c = atoms->atomicNumber(neighbours[2]);
              d = atoms->atomicNumber(neighbours[3]);
              numba = atoms->numberOfBonds(neighbours[0]);
              numbb = atoms->numberOfBonds(neighbours[1]);
              numbc = atoms->numberOfBonds(neighbours[2]);
              numbd = atoms->numberOfBonds(neighbours[3]);
              if((a == b == c) && (numba == numbb == numbc == 1))
              {
                ///// methyl
                result += "3";
                result += QString::number(i+1) + "," + QString::number(neighbours[0]+1) + "," + QString::number(neighbours[1]+1) + "," + QString::number(neighbours[2]+1) + "," + QString::number(neighbours[3]+1);
              }
              else if((a == b == d) && (numba == numbb == numbd == 1))
              {
                ///// methyl
                result += "3";
                result += QString::number(i+1) + "," + QString::number(neighbours[0]+1) + "," + QString::number(neighbours[1]+1) + "," + QString::number(neighbours[3]+1) + "," + QString::number(neighbours[2]+1);
              }
              else if((a == c == d) && (numba == numbc == numbd == 1))
              {
                ///// methyl
                result += "3";
                result += QString::number(i+1) + "," + QString::number(neighbours[0]+1) + "," + QString::number(neighbours[2]+1) + "," + QString::number(neighbours[3]+1) + "," + QString::number(neighbours[1]+1);
              }
              else if((b == c == d) && (numbb == numbc == numbd == 1))
              {
                ///// methyl
                result += "3";
                result += QString::number(i+1) + "," + QString::number(neighbours[1]+1) + "," + QString::number(neighbours[2]+1) + "," + QString::number(neighbours[3]+1) + "," + QString::number(neighbours[0]+1);
              }
              else if((a == b) && (numba == numbb == 1))
              {
                ///// methylene
                result += "4";
                result += QString::number(i+1) + "," + QString::number(neighbours[2]+1) + "," + QString::number(neighbours[3]+1) + "," + QString::number(neighbours[0]+1) + "," + QString::number(neighbours[1]+1);

              }
              else if((a == c) && (numba == numbc == 1))
              {
                ///// methylene
                result += "4";
                result += QString::number(i+1) + "," + QString::number(neighbours[1]+1) + "," + QString::number(neighbours[3]+1) + "," + QString::number(neighbours[0]+1) + "," + QString::number(neighbours[2]+1);
              }
              else if((a == d) && (numba == numbd == 1))
              {
                ///// methylene
                result += "4";
                result += QString::number(i+1) + "," + QString::number(neighbours[1]+1) + "," + QString::number(neighbours[2]+1) + "," + QString::number(neighbours[0]+1) + "," + QString::number(neighbours[3]+1);
              }
              else if((b == c) && (numbb == numbc == 1))
              {
                ///// methylene
                result += "4";
                result += QString::number(i+1) + "," + QString::number(neighbours[0]+1) + "," + QString::number(neighbours[3]+1) + "," + QString::number(neighbours[1]+1) + "," + QString::number(neighbours[2]+1);
              }
              else if((b == d) && (numbb == numbd == 1))
              {
                ///// methylene
                result += "4";
                result += QString::number(i+1) + "," + QString::number(neighbours[0]+1) + "," + QString::number(neighbours[2]+1) + "," + QString::number(neighbours[1]+1) + "," + QString::number(neighbours[3]+1);
              }
              else if((c == d) && (numbc == numbd == 1))
              {
                ///// methylene
                result += "4";
                result += QString::number(i+1) + "," + QString::number(neighbours[0]+1) + "," + QString::number(neighbours[1]+1) + "," + QString::number(neighbours[2]+1) + "," + QString::number(neighbours[3]+1);
              }
              else
              {
                ///// methyne
                result += "7";
                result += QString::number(i+1) + "," + QString::number(neighbours[0]+1) + "," + QString::number(neighbours[1]+1) + "," + QString::number(neighbours[2]+1) + "," + QString::number(neighbours[3]+1);
              }

              break;
      default:
              qWarning("More than 4 bonds per atom are not supported by MAFF");
    }
  }
  ///// closing
  result += "0";
  //if(CheckBoxFDIA->isChecked())
    result += "n";   // Only diagonal elements in F
  //else
  //  result += "y";   // Off-diagonal elements in F

  return result;
}

///// makeDirCurrent //////////////////////////////////////////////////////////
bool RelaxBase::makeDirCurrent(const QString dir, const QString title)
/// Makes \c dir the current directory.
/// Shows a messagebox with the error and \c title when something goes wrong.
{
  QDir workDir = dir;
  if(!workDir.exists())
  {
    if(!workDir.mkdir(workDir.path()))
    {
      QMessageBox::warning(this, title, tr("Unable to create the directory ") + workDir.path());
      return false;
    }
  }
  if(!QDir::setCurrent(workDir.path()))
  {
    QMessageBox::warning(this, title, tr("Unable to change to the directory ") + workDir.path());
    return false;
  }
  return true;
}

///// fillICListFromAFF ///////////////////////////////////////////////////////
void RelaxBase::fillICListFromAFF(const QStringList aff)
/// Adds the internal coordinates found in the latest generated aff to the IC list widget.
{
  ///// build a list of all FIXC, MAXI, and TORO keywords
  vector<unsigned int> fixc, maxi, toro; // each value is the number of an internal coordinate
  vector<double> toroValues;
  ///// loop over the input and stop at the start of the IC definitions
  QStringList::ConstIterator it = aff.begin();
  QString key;
  while(it != aff.end() && !(key == "gbma" || key == "bmat"))
  {
    key = (*it).left(4).lower();
    if(key == "fixc")
      fixc.push_back((*it).mid(10,10).toUInt());
    else if(key == "maxi")
      maxi.push_back((*it).mid(10,10).toUInt()); 
    else if(key == "toro")
    {
      toro.push_back((*it).mid(10,10).toUInt()); 
      toroValues.push_back((*it).mid(20,10).toDouble()); 
    }
    it++;
  }

  ///// if the input is generated by a call to readMAFFGeneratedICs
  ///// only the actual internal coordinates are present in cnvrtaff form
  if(it == aff.end() && !aff.isEmpty())
    it = aff.begin();

  ///// read the first internal coordinate and determine whether it passed through cnvrtaff
  bool cnvrtaff = !(*it).mid(2,7).stripWhiteSpace().isEmpty();

  ///// read the force constants of FMAT
  vector<double> fmat;
  if(!cnvrtaff)
  {
    // position the input after FMAT
    QStringList::ConstIterator itFMAT = it;
    while(itFMAT != aff.end() && (*itFMAT).left(4).lower() != "fmat")
      itFMAT++;

    // read as many doubles as possible as long as they decode OK;
    bool ok = true;
    while(itFMAT != aff.end() && ok)
    {
      QStringList values = QStringList::split(" ",*itFMAT);
      for(QStringList::Iterator itValues = values.begin(); itValues != values.end(); itValues++)
      {
        double fc = (*itValues).toDouble(&ok);
        if(!ok)
          break;
        fmat.push_back(fc);
      }
      itFMAT++;
    }
  }
  qDebug("force constants read from FMAT:");
  for(unsigned int i = 0; i < fmat.size(); i++)
    qDebug("%f",fmat[i]);

  ///// only read IC's containing atom numbers that exist
  unsigned int numAtoms = atoms->count();

  ///// keep track of the number of IC's so the FMAT constant
  ///// to be read can be determined
  unsigned int numICs = 0;

  ///// parse all internal coordinates and add them to the view
  while(it != aff.end() && ((*it).left(2) == "K " || (*it).left(10).stripWhiteSpace().isEmpty()))
  {
    QString line = (*it).lower();
    //qDebug("parsing IC: "+line);
    if(!cnvrtaff && line.left(1) == "k")
      numICs++;

    ///// common parts of a coordinate
    // weight
    QString weight;
    if(line.mid(10,10).stripWhiteSpace().isEmpty())
      weight = "1.0";
    else
      weight = QString::number(line.mid(10,10).toDouble());
    // atom numbers
    unsigned int atom1 = static_cast<unsigned int>(line.mid(30,10).toDouble());
    unsigned int atom2 = static_cast<unsigned int>(line.mid(40,10).toDouble());
    unsigned int atom3 = static_cast<unsigned int>(line.mid(50,10).toDouble());
    int atom4 = static_cast<int>(line.mid(60,10).toDouble());
    if(atom1 > numAtoms || atom2 > numAtoms || atom3 > numAtoms || (atom4 > 0 && static_cast<unsigned int>(atom4) > numAtoms))
    {
      it++;
      continue;
    }
    // type
    QString type = line.mid(20,4);
    if(type == "stre")
    {
      type = tr("Stretch");
      atom3 = atom4 = 0;
    }
    else if(type == "bend")
    {
      type = tr("Bend");
      // swap atom2 and atom3 + clear atom4
      atom4 = atom2;
      atom2 = atom3;
      atom3 = atom4;
      atom4 = 0;
    }
    else if(type == "tors")
      type = tr("Torsion");
    else if(type == "out ")
      type = tr("Out-of-plane");
    else if(type == "lin1")
      type = tr("Linear bend");
    else if(type == "lin2")
    {
      it++; // LIN1 and LIN2 are always written as a pair in MAFF
      continue;
    }

    if(line.left(1) == "k")
    {
      ///// main coordinate
      newIC();
      QListViewItem* newItem = ListViewIC->selectedItem();
      newItem->setText(1, type);
      if(cnvrtaff)
        newItem->setText(2, QString::number(line.mid(2,7).toDouble())); // force constant from AFF
      else
        newItem->setText(2, QString::number(fmat[numICs-1]));
      newItem->setText(6, weight);
      newItem->setText(7, icRepresentation(atom1, atom2, atom3, atom4)); // force constant from FMAT
      ///// check whether any values in the fixc, maxi or toro lists agree
      if(std::find(fixc.begin(), fixc.end(), numICs) != fixc.end())
        newItem->setPixmap(3, IconSets::getPixmap(IconSets::OK));
      if(std::find(maxi.begin(), maxi.end(), numICs) != maxi.end())
        newItem->setPixmap(4, IconSets::getPixmap(IconSets::OK));
      if(!toro.empty())
      {
        vector<double>::iterator itToroValues = toroValues.begin();
        for(vector<unsigned int>::iterator itToro = toro.begin(); itToro != toro.end(); itToro++, itToroValues++)
        {
          if(*itToro == numICs)
            newItem->setText(5, QString::number(*itToroValues));
        }
      }
    }
    else
    { 
      ///// part of a coordinate
      newICPart();
      QListViewItem* newItem = ListViewIC->selectedItem();
      newItem->setText(6, weight);
      newItem->setText(7, icRepresentation(atom1, atom2, atom3, atom4));
    }
    it++;
  }
  
  ///// if the number of FMAT force constants read is exactly 6 more than needed
  ///// update the external force constants too.
  ///// This never happens with generated AFFs, so it can only occur with the
  ///// 'Read input file' command 
  if(!cnvrtaff && fmat.size() == numICs + 6)
  {
    LineEditXTFK1->setText(QString::number(fmat[fmat.size() - 6]));
    LineEditXTFK2->setText(QString::number(fmat[fmat.size() - 5]));
    LineEditXTFK3->setText(QString::number(fmat[fmat.size() - 4]));
    LineEditXTFK4->setText(QString::number(fmat[fmat.size() - 3]));
    LineEditXTFK5->setText(QString::number(fmat[fmat.size() - 2]));
    LineEditXTFK6->setText(QString::number(fmat[fmat.size() - 1]));
  }
}

///// startMAFF ///////////////////////////////////////////////////////////////
bool RelaxBase::startMAFF(const unsigned int slot)
/// Initiates the generation of an AFF file using MAFF. Upon completion a specific
/// slot will be called.
{
  ///// do some checks
  if(atoms->count() == 0)
    return false;    
  
  ///// set the current directory
  if(!makeDirCurrent(calcDir + QDir::separator() + "tmp", tr("Generate internal coordinates")))
    return false;

  ///// write out the coordinates
  if(CrdFactory::writeToFile(atoms, calcName + ".crd", calcXF) != CrdFactory::OK)
  {
    QMessageBox::warning(this, tr("Generate internal coordinates"), tr("Unable to write the cartesian coordinates"));
    return false;
  }

  ///// Generate the input file for MAFF
  QStringList maffInput = calcName; // Identification code
  if(calcXF)
    maffInput += "y"; // use extended format
  else
    maffInput += "n";
          
  switch(ComboBoxType->currentItem())
  {
    case 0:

      maffInput += "a"; // GBMA
      maffInput += "y"; // automatically generate the bond list
      maffInput += "n"; // add extra bonds
      if(CheckBoxHBonds->isChecked())
        maffInput += "y"; // add H-bonds      
      else
        maffInput += "n";                
      break;
      
    case 1:

      maffInput += "b";   // BMAT
      maffInput += "y";   // Use standard force constants
      maffInput += "y";   // Use diagonal force constant matrix
      maffInput += "n";   // Input op fort.4

      QStringList tempInput = generateBmatInput();
      if(tempInput.isEmpty())
        return false;
      else
        maffInput += tempInput;
      break;
  }
    
  ///// start MAFF
  maffProcess = new QProcess(this);
  // connect to the proper slot for processing of the output
  switch(slot)
  {
    case RelaxBase::PreviewAFF:    
      // generation of internal coordinates for showing as a preview
      connect(maffProcess, SIGNAL(processExited()), this, SLOT(showPreview2()));
      break;
    case RelaxBase::ICListAFF:     
      // generation of internal coordinates for filling in ListViewIC
      connect(maffProcess, SIGNAL(processExited()), this, SLOT(genIC2()));            
  }
  maffProcess->addArgument(Paths::maff);
  QString processStdin = maffInput.join("\n") + "\n";
  maffProcess->launch(processStdin);  

  return true;
}

///// readMAFFGeneratedICs ////////////////////////////////////////////////////
QStringList RelaxBase::readMAFFGeneratedICs()
/// Returns the internal coordinates generated by the latest MAFF run.
{
  bool ok = maffProcess->normalExit();
  delete maffProcess;
  maffProcess = 0;
  if(!ok)
    return QStringList();

  ///// Read the contents of the output file and manually do the work of CNVRTAFF
  ///// open the file
  QFile affFile(calcDir + QDir::separator() + "tmp" + QDir::separator() + calcName + ".aff");
  if(!affFile.exists())
  {
    QMessageBox::warning(this, tr("Generate internal coordinates"),tr("No internal coordinates were generated"));
    return QStringList();
  }
  if(!affFile.open(IO_ReadOnly))
  {
    QMessageBox::warning(this, tr("Generate internal coordinates"),tr("Unable to open the file ") + affFile.name());
    return QStringList();
  }
  ///// read the force constants
  QTextStream affStream(&affFile);
  while((affStream.readLine() != "FMAT") && !affStream.atEnd())
    ;
  vector<double> fc;
  double oneFC;
  while(!affStream.atEnd())
  {
    affStream >> oneFC;
    fc.push_back(oneFC);            
  }
     
  ///// read the internal coordinates
  affFile.close();
  ///// re-open as rewinding doesn't work
  if(!affFile.open(IO_ReadOnly))
  {
    QMessageBox::warning(this, tr("Generate internal coordinates"),tr("Unable to open the file ") + affFile.name());
    return QStringList();
  }
  QTextStream affStream2(&affFile);
  QString line;
  while((line != "GBMA") && (line != "BMAT") && !affStream2.atEnd())
    line = affStream2.readLine();

  QStringList result;
  unsigned int counter = 0;
  line = affStream2.readLine(); // the first line after GBMA || BMAT
  while((line != "FMAT") && !affStream2.atEnd())
  {
    if(line.left(1) == "K")
    {
      if(counter == fc.size())
        break;
      QString fcString;
      fcString.setNum(fc[counter],'f',7).truncate(7);
      line = line.left(2) + fcString + line.mid(9);      
      counter++;
    }
    result += line;
    line = affStream2.readLine();    
  }    
     
  return result;
}

///////////////////////////////////////////////////////////////////////////////
///// Static Variables                                                    /////
///////////////////////////////////////////////////////////////////////////////
const RelaxBase::XMLData RelaxBase::xml = {
  ///// Basic
  "internal_coordinates_type",
  "autogenerate",
  "regenerate",   
  "generation_frequency",
  "hydrogen_bonds",
  "one_scale_factor",
  "scale_factor_single_value",
  "scale_factor_single_step",
  "scale_factor_number",
  "scale_factor_values",
  "scale_factor_steps",
  "diis",
  "shifts",
  "maximum_steps",
  ///// Internal coordinates
  "ic_numbers",
  "ic_root_items",
  "ic_types",
  "ic_force_constants",
  "ic_fixed",
  "ic_maximized",
  "ic_reference_values",
  "ic_weights",
  "ic_first_parts",
  "ic_second_parts",
  "ic_third_parts",
  "ic_fourth_parts",
  ///// Cartesian coordinates - Symmetry
  "symmetry_check",
  "symmetry_x__",
  "symmetry_y__",
  "symmetry_z__",
  "symmetry_xy_",
  "symmetry_xz_",
  "symmetry_yz_",
  "symmetry_xyz",
  ///// Cartesian coordinates - Other
  "fix_atom_number",
  "fix_atom_x",
  "fix_atom_y",
  "fix_atom_z",
  "fixed_atom_numbers",
  "fixed_atom_axes",
  "use_external_coordinates",
  "external_angle",
  "external_coordinate_a_",
  "external_coordinate_b_",
  "external_coordinate_c_",
  "external_coordinate_alpha",
  "external_coordinate_beta",
  "external_coordinate_gamma",
  "mirror",
  "animate",
  ///// Advanced - Thresholds
  "threshold_refine_1",
  "threshold_refine_2",
  "threshold_refine_3",
  "threshold_refine_4",
  "threshold_refine_5",
  "maximum_iterations",
  "threshold_iteration_1",
  "threshold_iteration_2",
  "threshold_iteration_3",
  "threshold_iteration_4",
  ///// Advanced - Other
  "extra_bond_1",
  "extra_bond_2",
  "extra_bond_list_1",
  "extra_bond_list_2",
  "mass",
  "low_frequencies_set",
  "low_frequencies_value",
  "orden",
  ///// Debug
  "override_checks",
  "debug_output",
  "use_update",
  "update_method",
  "update_scale_factor",
  "compact_output",
  "compact_bonds",
  "compact_angles",
  "compact_torsions",
  ///// Extra
  "extra_lines",
  "extra_hpos",
  "extra_vpos",
  "extra_contents"
};

