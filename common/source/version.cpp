/***************************************************************************
                          version.cpp  -  description
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

///// Comments ////////////////////////////////////////////////////////////////
/*!
  \class Version
  \brief This class holds the application's name, company, version and build number.

*/
/// \file
/// Contains the implementation of the class Version

///// Header files ////////////////////////////////////////////////////////////

// Xbrabo header files
#include "version.h"

///////////////////////////////////////////////////////////////////////////////
///// Private Members                                                      /////
///////////////////////////////////////////////////////////////////////////////

///// Constructor /////////////////////////////////////////////////////////////
Version::Version()
/// The default constructor. Made private to inhibit instantations.
{
}

///////////////////////////////////////////////////////////////////////////////
///// Static Variables                                                    /////
///////////////////////////////////////////////////////////////////////////////
const QString Version::appName = "Brabosphere";                        // The name of the application
const QString Version::appCompany = "University of Antwerp";           // The company
const QString Version::appVersion = "1.0.0";                           // The version of the application
const QString Version::appBuild = QString::number(                     // The build number of the application read from an automatically 
  #include "buildnumber.h"                                             // updated file. The value depends on whether Brabosphere or
);                                                                     // CrdView is being built as it is pulled from the local include dir.

