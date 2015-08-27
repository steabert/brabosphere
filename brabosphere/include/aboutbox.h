/***************************************************************************
                          aboutbox.h  -  description
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

/// \file
/// Contains the declaration of the class AboutBox.

#ifndef ABOUTBOX_H
#define ABOUTBOX_H

///// Forward class declarations & header files ///////////////////////////////

// Base class header file
#include <qdialog.h>

///// class AboutBox //////////////////////////////////////////////////////////

class AboutBox: public QDialog
{
  public:
    AboutBox(QWidget* parent = 0, const char* name = 0);    // Constructor
    ~AboutBox();                        // Destructor

  protected:
    void keyPressEvent(QKeyEvent* e);   // handles key presses
    void mousePressEvent(QMouseEvent* e);         // handles mouse presses
};

#endif

