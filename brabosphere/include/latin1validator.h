/***************************************************************************
                      latin1validator.h  -  description
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

/// \file
/// Contains the declaration of the class Latin1Validator

#ifndef LATIN1VALIDATOR_H
#define LATIN1VALIDATOR_H

///// Forward class declarations & header files ///////////////////////////////

// Base class header file
#include <qvalidator.h>

///// class Latin1Validator ///////////////////////////////////////////////////

class Latin1Validator: public QValidator
{
  public:
    Latin1Validator(QObject* parent, const char* name = 0); // Constructor
    ~Latin1Validator();                 // Destructor

    void fixup(QString& input) const;   // fixes the string by removeing unwanted characters
    State validate(QString& input, int& pos) const;         // checks for the presence of non-latin1 characters
};

#endif

