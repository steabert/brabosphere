/***************************************************************************
                          version.h  -  description
                             -------------------
    begin                : Sun Dec 4 2005
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
/// Contains the declaration of the class Version.

#ifndef VERSION_H
#define VERSION_H

///// Forward class declarations & header files ///////////////////////////////

// Qt header files
#include <qstring.h>

///// class Version ///////////////////////////////////////////////////////////
class Version
{
  public:
    ///// static variables
    static const QString appName;       //< The name of the application
    static const QString appCompany;    //< The company
    static const QString appVersion;    //< The version of the application
    static const QString appBuild;      //< The build number of the application

  private:
    Version();                           // constructor
};

#endif

