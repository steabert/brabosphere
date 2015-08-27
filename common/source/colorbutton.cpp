/***************************************************************************
                        colorbutton.cpp  -  description
                             -------------------
    begin                : Tue Apr 1 2003
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
  \class ColorButton
  \brief Provides a button for selecting a color.

  This class is an extension of the QPushButton class. 
  It shows the color on the button and presents a QColorDialog when clicked.

*/
/// \file
/// Contains the implementation of the class ColorButton.

///// Header files ////////////////////////////////////////////////////////////

// Qt header files
#include <qcolor.h>
#include <qcolordialog.h>
 
// Xbrabo header files
#include <colorbutton.h>

///////////////////////////////////////////////////////////////////////////////
///// Public Member Functions                                             /////
///////////////////////////////////////////////////////////////////////////////

///// constructor /////////////////////////////////////////////////////////////
ColorButton::ColorButton(QWidget* parent, const char* name) : QPushButton(parent, name)
/// The default constructor.
{
  pixmap = new QPixmap();
  setColor(QColor(0, 0, 0));

  connect(this, SIGNAL(resized()), this, SLOT(updatePixmap()));
  connect(this, SIGNAL(clicked()), this, SLOT(selectColor()));
}

///// constructor (overloaded) ////////////////////////////////////////////////
ColorButton::ColorButton(const QColor& color, QWidget* parent, const char* name) : QPushButton(parent, name)
/// \overloaded
{
  pixmap = new QPixmap();
  setColor(color);

  connect(this, SIGNAL(resized()), this, SLOT(updatePixmap()));
  connect(this, SIGNAL(clicked()), this, SLOT(selectColor()));  
}

///// destructor //////////////////////////////////////////////////////////////
ColorButton::~ColorButton()
/// The default destructor
{
  delete pixmap;
}

///// color ///////////////////////////////////////////////////////////////////
QColor ColorButton::color() const
/// Returns the color of the button.
{  
  return fillColor;
}

///// setColor ////////////////////////////////////////////////////////////////
void ColorButton::setColor(const QColor color)
/// Sets the color of the button.
{  
  fillColor = color;
  updatePixmap();
}


///////////////////////////////////////////////////////////////////////////////
///// Protected Member Functions                                          /////
///////////////////////////////////////////////////////////////////////////////

///// resizeEvent /////////////////////////////////////////////////////////////
void ColorButton::resizeEvent(QResizeEvent* e)
/// Overridden from QPushButton::resizeEvent().
{  
  QPushButton::resizeEvent(e);
  emit resized();
}


///////////////////////////////////////////////////////////////////////////////
///// Private Slots                                                       /////
///////////////////////////////////////////////////////////////////////////////

///// selectColor /////////////////////////////////////////////////////////////
void ColorButton::selectColor()
/// Allows the selection of a new color.
{  
  QColor tempColor = QColorDialog::getColor(fillColor, this);
  if(tempColor.isValid())
  {
    setColor(tempColor);
    emit newColor(&fillColor);
  }
}

///// updatePixmap ////////////////////////////////////////////////////////////
void ColorButton::updatePixmap()
/// Updates the size and color of the pixmap representing the current color.
{  
  pixmap->resize(width() - height()/2, height()/2);
  pixmap->fill(fillColor);
  this->setPixmap(*pixmap);
}

