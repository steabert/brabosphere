/***************************************************************************
                          paths.cpp  -  description
                             -------------------
    begin                : Thu Jul 3 2003
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
  \class Paths
  \brief This class of static member data contains the paths to all executables and some other
         directories.

*/
/// \file
/// Contains the implementation of the class Paths

///// Header files ////////////////////////////////////////////////////////////

// Qt header files
#include <qstring.h>

// Xbrabo header files
#include "paths.h"

///////////////////////////////////////////////////////////////////////////////
///// Private Member Functions                                            /////
///////////////////////////////////////////////////////////////////////////////

///// constructor /////////////////////////////////////////////////////////////
Paths::Paths()
/// The default constructor. Made private to inhibit instantiation.
{

}

///////////////////////////////////////////////////////////////////////////////
///// Static Variables                                                    /////
///////////////////////////////////////////////////////////////////////////////

QString Paths::abc = QString();
QString Paths::achar = QString();
QString Paths::atomscf = QString();
QString Paths::atomscf_inp = QString();
QString Paths::atomscf_o2d = QString();
QString Paths::b112gfchk = QString();
QString Paths::bios2ped = QString();
QString Paths::brabo = QString();
QString Paths::buur = QString();
QString Paths::cnvrtaff = QString();
QString Paths::crd2gauss = QString();
QString Paths::crd2xyz = QString();
QString Paths::diffcrd = QString();
QString Paths::distor = QString();
QString Paths::forkon = QString();
QString Paths::frex = QString();
QString Paths::gar2ped = QString();
QString Paths::geom = QString();
QString Paths::gfchk2b11 = QString();
QString Paths::gfchk2crd = QString();
QString Paths::gfchk2pun = QString();
QString Paths::hkl = QString();
QString Paths::log2crd = QString();
QString Paths::maff = QString();
QString Paths::makeden = QString();
QString Paths::makexit = QString();
QString Paths::molsplit = QString();
QString Paths::out2aff = QString();
QString Paths::potdicht = QString();
QString Paths::pullarc = QString();
QString Paths::refine = QString();
QString Paths::relax = QString();
QString Paths::ring = QString();
QString Paths::spfmap = QString();
QString Paths::startvec = QString();
QString Paths::stock = QString();
QString Paths::symm = QString();
QString Paths::table = QString();
QString Paths::xyz2crd = QString();
QString Paths::bin = QString();
QString Paths::basisset = QString();

