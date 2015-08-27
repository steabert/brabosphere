/***************************************************************************
                         colorbutton.h  -  description
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

/// \file
/// Contains the declaration of the class ColorButton

#ifndef COLORBUTTON_H
#define COLORBUTTON_H

///// Forward class declarations & header files ///////////////////////////////

// Qt forward class declarations
class QColor;

// Base class header file
#include <qpushbutton.h>

///// class ColorButton ///////////////////////////////////////////////////////
class ColorButton : public QPushButton
{
  Q_OBJECT
  
  public: 
    ColorButton(QWidget* parent = 0, const char* name = 0); // constructor
    ColorButton(const QColor& color, QWidget* parent = 0, const char* name = 0);// constructor (overloaded)
    ~ColorButton();                     // destructor

    QColor color() const;               // returns the color of the button
    void setColor(const QColor color);  // sets a new color for the button
    
  signals:
    void resized();                     // emitted when the button has been resized
    void newColor(QColor* color);       // emitted when the color has been changed

  protected:
    virtual void resizeEvent(QResizeEvent* e);    // occurs when the button is resized

  private slots:
    void selectColor();                 // selects a new color for the button  
    void updatePixmap();                // updates the pixmap of the button with a new size and color

  private:
    QColor fillColor;                   ///< the color of the pixmap
    QPixmap* pixmap;                    ///< the pixmap itself
  
};

#endif

