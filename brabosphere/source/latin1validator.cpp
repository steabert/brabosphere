/***************************************************************************
                     latin1validator.cpp  -  description
                             -------------------
    begin                : Fri Dec 01 2005
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
  \class Latin1Validator
  \brief This class validates QString's so they only contain latin1 characters.

  It must be used for cases where strings are passed on to BRABO programs as
  input. As these strings (at this point) may never contain spaces, this is
  checked too.
*/
/// \file
/// Contains the implementation of the class Latin1Validator.

///// Header files ////////////////////////////////////////////////////////////

// Xbrabo header files
#include "latin1validator.h"

///////////////////////////////////////////////////////////////////////////////
///// Public Member Functions                                             /////
///////////////////////////////////////////////////////////////////////////////

///// constructor /////////////////////////////////////////////////////////////
Latin1Validator::Latin1Validator(QObject* parent, const char* name) : QValidator(parent, name)
/// The default constructor.
{
}

///// destructor //////////////////////////////////////////////////////////////
Latin1Validator::~Latin1Validator()
/// The default destructor.
{
}

///// fixup ///////////////////////////////////////////////////////////////////
void Latin1Validator::fixup(QString& input) const
/// Fixes the input string by removing all non-latin1 characters.
{
  //qDebug("called fixup");
  for(unsigned int i = input.length(); i != 0; i--)
  {
    if(!input.at(i - 1).latin1() || input.at(i-1).isSpace())
      input.remove(i - 1,1);
  }
}

///// validate ////////////////////////////////////////////////////////////////
QValidator::State Latin1Validator::validate(QString& input, int& pos) const
/// Returns whether the string is a valid latin1-only string.
{
  //qDebug("called validate");
  for(unsigned int i = 0; i < input.length(); i++)
  {
    if(!input.at(i).latin1() || input.at(i).isSpace())
    {
      pos = i;
      return QValidator::Invalid;
    }
  }
  return QValidator::Acceptable;
}

