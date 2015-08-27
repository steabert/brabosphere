/***************************************************************************
                          statustext.cpp  -  description
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

///// Comments ////////////////////////////////////////////////////////////////
/*!
  \class StatusText
  \brief This class is used for logging events for a calculation.

  It provides a little bit of added functionality with respect to QTextEdit.
*/
/// \file
/// Contains the implementation of the class StatusText.

///// Header files ////////////////////////////////////////////////////////////

// Qt header files

// Xbrabo header files
#include "statustext.h"

///////////////////////////////////////////////////////////////////////////////
///// Public Member Functions                                             /////
///////////////////////////////////////////////////////////////////////////////

///// constructor /////////////////////////////////////////////////////////////
StatusText::StatusText(QWidget* parent, const char* name) : QTextEdit(parent, name)
/// The default constructor.
{
  // Basic setup
  setSizePolicy(QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding, false));
  setReadOnly(true);
  setTextFormat(Qt::LogText);
  setMaxLogLines(2000);
  setWordWrap(QTextEdit::NoWrap);

  // A fixed pitch font?
  QFont font = currentFont();
  font.setStyleHint(QFont::TypeWriter); // does nothing on X11 by definition
  font.setFixedPitch(true);
  setFont(font); // can't get a fixed pitch font in this way on Windows
}

///// destructor //////////////////////////////////////////////////////////////
StatusText::~StatusText()
/// The default destructor.
{

}

///////////////////////////////////////////////////////////////////////////////
///// Public Slots                                                        /////
///////////////////////////////////////////////////////////////////////////////

///// append /////////////////////////////////////////////////////////////////
void StatusText::append(const QString& text)
/// Overridden from QTextEdit::append in that it autoamtically positions itself
/// at the last line.
{
  QTextEdit::append(text);
  scrollToBottom();
}

///////////////////////////////////////////////////////////////////////////////
///// Protected Member Functions                                          /////
///////////////////////////////////////////////////////////////////////////////

///// createPopupMenu /////////////////////////////////////////////////////////
QPopupMenu* StatusText::createPopupMenu(const QPoint&)
/// This function is overridden to disable the construction of a default popup
/// menu, but instead emit the signal rightButtonClicked so a popup can be
/// shown by the another class (XbraboView in this case).
{
  emit rightButtonClicked();
  return 0;
}

