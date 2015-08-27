/***************************************************************************
                         statustext.h  -  description
                             -------------------
    begin                : Thu Jul 28 2005
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
/// Contains the declaration of the class StatusText.

#ifndef STATUSTEXT_H
#define STATUSTEXT_H

///// Forward class declarations & header files ///////////////////////////////

// Base class header file
#include <qtextedit.h>

///// class StatusText ////////////////////////////////////////////////////////
class StatusText : public QTextEdit
{
  Q_OBJECT

  public:
    StatusText(QWidget* parent = 0, const char* name = 0);  // Constructor
    ~StatusText();                      // Destructor

  signals:
    void rightButtonClicked();          ///< Is emitted when a right mouse button is clicked

  public slots:
    void append(const QString& text);   // Adds the specified text

  protected:
    QPopupMenu* createPopupMenu(const QPoint&);   // Overridden to disable the default popup menu
};

#endif

