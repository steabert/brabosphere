/***************************************************************************
                        newatombase.cpp  -  description
                             -------------------
    begin                : Sun Jul 31 2005
    copyright            : (C) 2005-2006 by Ben Swerts
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
  \class NewAtomBase
  \brief Allows the addition of a new atom.

  This can be done either by specifying the required cartesian coordinates 
  or by entering internal coordinates using other atoms as references.
*/
/// \file
/// Contains the implementation of the class NewAtomBase.

///// Header files ////////////////////////////////////////////////////////////

// C++ header files
#include <cassert>

// Qt header files
#include <qbuttongroup.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qspinbox.h>
#include <qwidgetstack.h>

// Xbrabo header files
#include "atomset.h"
#include "newatombase.h"
#include "point3d.h"

///////////////////////////////////////////////////////////////////////////////
///// Public Member Functions                                             /////
///////////////////////////////////////////////////////////////////////////////

///// Constructor /////////////////////////////////////////////////////////////
NewAtomBase::NewAtomBase(AtomSet* atomset, QWidget* parent, const char* name, bool modal, WFlags fl) : NewAtomWidget(parent, name, modal, fl),
/// The default constructor.
  atoms(atomset)
{
  assert(atomset != 0);

  connect(PushButtonAdd, SIGNAL(clicked()), this, SLOT(addAtom()));
  connect(PushButtonClose, SIGNAL(clicked()), this, SLOT(hide()));
  connect(ButtonGroupType, SIGNAL(clicked(int)), this, SLOT(updateSelectedAtom(int)));
  connect(ButtonGroupCoordinates, SIGNAL(clicked(int)), WidgetStackCoordinates, SLOT(raiseWidget(int)));
  connect(ButtonGroupCoordinates, SIGNAL(clicked(int)), this, SLOT(checkAdd()));
  connect(ButtonGroupType, SIGNAL(clicked(int)), this, SLOT(updateICAtoms()));
  connect(SpinBoxReference1, SIGNAL(valueChanged(int)), this, SLOT(updateICAtoms()));
  connect(SpinBoxReference2, SIGNAL(valueChanged(int)), this, SLOT(updateICAtoms()));
  connect(SpinBoxReference3, SIGNAL(valueChanged(int)), this, SLOT(updateICAtoms()));

  updateAtomLimits();
}

///// Destructor //////////////////////////////////////////////////////////////
NewAtomBase::~NewAtomBase()
/// The default destructor.
{
}

///////////////////////////////////////////////////////////////////////////////
///// Public Slots                                                        /////
///////////////////////////////////////////////////////////////////////////////

///// updateAtomLimits ////////////////////////////////////////////////////////
void NewAtomBase::updateAtomLimits()
/// Updates all widgets that are based on the AtomSet. This slot should be called 
/// whenever the AtomSet changes.
{
  const unsigned int numAtoms = atoms->count();
  if(numAtoms == 0)
  {
    // no atoms present, so none can be used as a reference
    RadioButtonInternal->setEnabled(false);
    RadioButtonCartesian->setChecked(true);
    WidgetStackCoordinates->raiseWidget(0);
    RadioButtonRelative->setEnabled(false);
    RadioButtonAbsolute->setChecked(true);
    SpinBoxRelative->setEnabled(false);
  }
  else
  {
    // at least one atom is present
    RadioButtonInternal->setEnabled(true);
    RadioButtonRelative->setEnabled(true);
    SpinBoxRelative->setEnabled(true);
    SpinBoxRelative->setMaxValue(numAtoms);
    SpinBoxReference1->setMaxValue(numAtoms);
    SpinBoxReference2->setMaxValue(numAtoms);
    SpinBoxReference3->setMaxValue(numAtoms);
    if(!SpinBoxReference2->isEnabled() && numAtoms > 1)
      SpinBoxReference2->setValue(numAtoms);
    SpinBoxReference2->setEnabled(numAtoms > 1);
    if(!SpinBoxReference3->isEnabled() && numAtoms > 2)
      SpinBoxReference3->setValue(numAtoms);
    SpinBoxReference3->setEnabled(numAtoms > 2);
    LineEditAngle->setEnabled(numAtoms > 1);
    TextLabelAngle1->setEnabled(numAtoms > 1);
    TextLabelAngle2->setEnabled(numAtoms > 1);
    TextLabelAngle3->setEnabled(numAtoms > 1);
    TextLabelAngleA->setEnabled(numAtoms > 1);
    TextLabelAngleB->setEnabled(numAtoms > 1);
    TextLabelAngleC->setEnabled(numAtoms > 1);
    TextLabelAngleD->setEnabled(numAtoms > 1);
    TextLabelAngleE->setEnabled(numAtoms > 1);
    LineEditTorsion->setEnabled(numAtoms > 2);
    TextLabelTorsion1->setEnabled(numAtoms > 2);
    TextLabelTorsion2->setEnabled(numAtoms > 2);
    TextLabelTorsion3->setEnabled(numAtoms > 2);
    TextLabelTorsion4->setEnabled(numAtoms > 2);
    TextLabelTorsionA->setEnabled(numAtoms > 2);
    TextLabelTorsionB->setEnabled(numAtoms > 2);
    TextLabelTorsionC->setEnabled(numAtoms > 2);
    TextLabelTorsionD->setEnabled(numAtoms > 2);
    TextLabelTorsionE->setEnabled(numAtoms > 2);
    TextLabelTorsionF->setEnabled(numAtoms > 2);
    updateICAtoms();
  }
}

///////////////////////////////////////////////////////////////////////////////
///// Protected Member Functions                                          /////
///////////////////////////////////////////////////////////////////////////////

///// showEvent ///////////////////////////////////////////////////////////////
void NewAtomBase::showEvent(QShowEvent* e)
/// Updates everything when showing the dialog. Overridden from NewAtomWidget.
{
  updateAtomLimits();
  NewAtomWidget::showEvent(e);
}

///////////////////////////////////////////////////////////////////////////////
///// Private Slots                                                       /////
///////////////////////////////////////////////////////////////////////////////

///// addAtom /////////////////////////////////////////////////////////////////
void NewAtomBase::addAtom()
/// Adds an atom based on the status of the widgets
{
  unsigned int selectedAtomType = ButtonGroupType->selectedId();
  if(RadioButtonCartesian->isOn())
  {
    ///// Add the atom by absolute or relative cartesian coordinates
    if(RadioButtonAbsolute->isOn())
      // absolute
      atoms->addAtom(LineEditX->text().toDouble(), LineEditY->text().toDouble(), LineEditZ->text().toDouble(), selectedAtomType);
    else
      // relative      
      atoms->addAtom(LineEditX->text().toDouble() + atoms->x(SpinBoxRelative->value() - 1), LineEditY->text().toDouble() + atoms->y(SpinBoxRelative->value() - 1), LineEditZ->text().toDouble() + atoms->z(SpinBoxRelative->value() - 1), selectedAtomType);
  }
  else
  {
    ///// Add the atom by internal coordinates
    // get some indices
    const unsigned int newIndex = atoms->count();
    const unsigned int refIndex1 = SpinBoxReference1->value() - 1;
    const unsigned int refIndex2 = SpinBoxReference2->value() - 1;
    const unsigned int refIndex3 = SpinBoxReference3->value() - 1;
    // add the atom with the correct bond distance from the first reference atom (along the X-axis)
    atoms->addAtom(atoms->x(refIndex1) + LineEditBond->text().toDouble(), atoms->y(refIndex1), atoms->z(refIndex1), selectedAtomType);
    if(newIndex > 1)
    {
      // only set the angle when more than one atom is present 
      // get the current angle
      double angle = atoms->angle(newIndex, refIndex1, refIndex2);
      if(fabs(angle) < Point3D<double>::TOLERANCE || fabs(angle - 180) < Point3D<double>::TOLERANCE || fabs(angle + 180) < Point3D<double>::TOLERANCE)
      {
        qDebug("current angle is %f when using X-axis, so changing to Y-axis", angle); 
        // bad initial axis chosen, try the Y-axis
        atoms->setX(newIndex, atoms->x(refIndex1));
        atoms->setY(newIndex, atoms->y(refIndex1) + LineEditBond->text().toDouble());
        angle = atoms->angle(newIndex, refIndex1, refIndex2);
        /*if(abs(angle) < Point3D<double>::TOLERANCE)
        {
          // again bad initial axis chosen, try the Z-axis, last chance
          atoms->setY(newIndex, atoms->y(refIndex1));
          atoms->setZ(newIndex, atoms->z(refIndex1) + LineEditBond->text().toDouble());
          angle = atoms->angle(newIndex, refIndex1, refIndex2);
        }*/
      }
      // change it to the desired value
      atoms->changeAngle(LineEditAngle->text().toDouble() - angle, newIndex, refIndex1, refIndex2, false);
      if(newIndex > 2)
      {
        // only set the torsion when more than 2 atoms were present
        // get the current torsion
        const double torsion = atoms->torsion(newIndex, refIndex1, refIndex2, refIndex3);
        // change it to the desired value
        atoms->changeTorsion(torsion - LineEditTorsion->text().toDouble(), newIndex, refIndex1, refIndex2, refIndex3, false);
      }
    }
  }
  emit atomAdded();
  /*
  qDebug("finished, coordinates of all atoms:");
  for(unsigned int i = 0; i < atoms->count(); i++)
    qDebug("atom i: %f %f %f",atoms->x(i), atoms->y(i), atoms->z(i));
  */
  updateAtomLimits();
}

///// updateICAtoms ///////////////////////////////////////////////////////////
void NewAtomBase::updateICAtoms()
/// Updates the labels for the internal coordinate specifications depending
/// on the values in the spinboxes holding the reference atoms and the type
/// of the new atom
{
  QString newAtomSymbol = TextLabelSymbol->text().stripWhiteSpace() + QString::number(atoms->count() + 1);
  QString refAtom1 = AtomSet::numToAtom(atoms->atomicNumber(SpinBoxReference1->value() - 1)).stripWhiteSpace() + QString::number(SpinBoxReference1->value());
  QString refAtom2 = AtomSet::numToAtom(atoms->atomicNumber(SpinBoxReference2->value() - 1)).stripWhiteSpace() + QString::number(SpinBoxReference2->value());
  QString refAtom3 = AtomSet::numToAtom(atoms->atomicNumber(SpinBoxReference3->value() - 1)).stripWhiteSpace() + QString::number(SpinBoxReference3->value());

  ///// Bond
  TextLabelBond1->setText(newAtomSymbol);
  TextLabelBond2->setText(refAtom1);
  ///// Angle
  TextLabelAngle1->setText(newAtomSymbol);
  TextLabelAngle2->setText(refAtom1);
  TextLabelAngle3->setText(refAtom2);
  ///// Torsion
  TextLabelTorsion1->setText(newAtomSymbol);
  TextLabelTorsion2->setText(refAtom1);
  TextLabelTorsion3->setText(refAtom2);
  TextLabelTorsion4->setText(refAtom3);

  checkAdd();
}

///// updateSelectedAtom //////////////////////////////////////////////////////
void NewAtomBase::updateSelectedAtom(int number)
/// Updates the symbol and atomic number of the selected atom type. The atomic
/// number of this atom is passed as an argument.
{
  TextLabelSymbol->setText(AtomSet::numToAtom(number));
  TextLabelNumber->setText(QString::number(number));
}

///// checkAdd ////////////////////////////////////////////////////////////////
void NewAtomBase::checkAdd()
/// Checks the status of the widgets to verify whether an atom can be added or
/// not and subsequently enables/disables the addAtom button.
{
  if(RadioButtonCartesian->isOn())
    // always possible to add an atom
    PushButtonAdd->setEnabled(true);
  else
  {
    // for Internal Coordinates, adding is only possible under specific circumstances
    const unsigned int refAtom1 = SpinBoxReference1->value();
    const unsigned int refAtom2 = SpinBoxReference2->value();
    const unsigned int refAtom3 = SpinBoxReference3->value();
    if(   (SpinBoxReference2->isEnabled() && (refAtom1 == refAtom2)) // ref1 cannot be equal to ref2
       || (SpinBoxReference3->isEnabled() && (refAtom1 == refAtom3)) // ref1 cannot be equal to ref3
       || (SpinBoxReference3->isEnabled() && (refAtom2 == refAtom3)) // ref2 cannot be equal to ref3
       || (fabs(LineEditBond->text().toDouble()) < 0.1)) // bond distance should be at least 0.1
      PushButtonAdd->setEnabled(false);
    else
      PushButtonAdd->setEnabled(true);
  }
}

