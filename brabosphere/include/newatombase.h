/***************************************************************************
                        newatombase.h  -  description
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

/// \file
/// Contains the declaration of the class NewAtomBase.

#ifndef NEWATOMBASE_H
#define NEWATOMBASE_H

///// Forward class declarations & header files ///////////////////////////////

// Xbrabo forward class declarations
class AtomSet;

// Base class header file
#include "newatomwidget.h"

///// class NewAtomBase ///////////////////////////////////////////////////////
class NewAtomBase : public NewAtomWidget
{
  Q_OBJECT

  public:
    NewAtomBase(AtomSet* atomset, QWidget* parent = 0, const char* name = 0, bool modal = FALSE, WFlags fl = 0);        // constructor
    ~NewAtomBase();                     // destructor

  signals:
    void atomAdded();                   //< Is fired when an atom has been added

  public slots:
    void updateAtomLimits();            // updates all widgets pertaining to the AtomSet

  protected:
    void showEvent(QShowEvent* e);      // updates everything when the dialog is shown

  private slots:
    void addAtom();                     // adds an atom depending on the status of the widgets
    void updateICAtoms();               // updates the IC labels from the reference atoms
    void updateSelectedAtom(int number);// updates the proerties of the selected atom type
    void checkAdd();                    // check whether an atom can be added with the current status of the widgets

  private:

    ///// private member data
    AtomSet* atoms;                     ///< A pointer to the active AtomSet
};

#endif

