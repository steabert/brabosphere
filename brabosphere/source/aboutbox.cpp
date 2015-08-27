/***************************************************************************
                         aboutbox.cpp  -  description
                             -------------------
    begin                : Sat Jul 30 2005
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
  \class AboutBox
  \brief This class shows an About Box.

  This subclass of QDialog handles te closing of the box by any keypress or any 
  mouse release event.
*/
/// \file
/// Contains the implementation of the class AboutBox.

///// Header files ////////////////////////////////////////////////////////////

// Qt header files
#include <qapplication.h>

// Xbrabo header files
#include "aboutbox.h"
#include "iconsets.h"

///////////////////////////////////////////////////////////////////////////////
///// Public Member Functions                                             /////
///////////////////////////////////////////////////////////////////////////////

///// constructor /////////////////////////////////////////////////////////////
AboutBox::AboutBox(QWidget* parent, const char* name) : QDialog(parent, name, true, Qt::WStyle_Customize | Qt::WStyle_NoBorder)
/// The default constructor. It loads the required pixmap and centers the dialog 
/// on the screen.
{
  // load the pixmap and adapt the dialog to it
  QPixmap pm(IconSets::getSplash());
  resize(pm.size());
  setBackgroundPixmap(pm);
  if(pm.mask())
    setMask(*pm.mask());
  
  // center it on the dekstop
  QRect screenRect = QApplication::desktop()->availableGeometry(QApplication::desktop()->primaryScreen());
  move(screenRect.width()/2 - 309, screenRect.height()/2 - 193);
}

///// destructor //////////////////////////////////////////////////////////////
AboutBox::~AboutBox()
/// The default destructor.
{
}

///////////////////////////////////////////////////////////////////////////////
///// Protected Member Functions                                          /////
///////////////////////////////////////////////////////////////////////////////

///// keyPressEvent ///////////////////////////////////////////////////////////
void AboutBox::keyPressEvent(QKeyEvent*)
/// Intercept all key presses and closes the dialog when it encounters one.
{
  close();
}

///// mousePressEvent /////////////////////////////////////////////////////////
void AboutBox::mousePressEvent(QMouseEvent*)
/// Intercept all mouse presses and closes the dialog when it encounters one.
{
  close();
}

