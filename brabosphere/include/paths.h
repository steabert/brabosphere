/***************************************************************************
                           paths.h  -  description
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

/// \file
/// Contains the declaration of the class Paths

#ifndef PATHS_H
#define PATHS_H

///// Forward class declarations & header files ///////////////////////////////

// Qt forward class declarations
class QString;


///// class Paths //////////////////////////////////////////////////////////
class Paths
{
  public:
    ///// static public data
    static QString abc;
    static QString achar;
    static QString atomscf;
    static QString atomscf_inp;
    static QString atomscf_o2d;
    static QString b112gfchk;
    static QString bios2ped;
    static QString brabo;
    static QString buur;
    static QString cnvrtaff;
    static QString crd2gauss;
    static QString crd2xyz;
    static QString diffcrd;
    static QString distor;
    static QString forkon;
    static QString frex;
    static QString gar2ped;
    static QString geom;
    static QString gfchk2b11;
    static QString gfchk2crd;
    static QString gfchk2pun;
    static QString hkl;
    static QString log2crd;
    static QString maff;
    static QString makeden;
    static QString makexit;
    static QString molsplit;
    static QString out2aff;
    static QString potdicht;
    static QString pullarc;
    static QString refine;
    static QString relax;
    static QString ring;
    static QString spfmap;
    static QString startvec;
    static QString stock;
    static QString symm;
    static QString table;
    static QString xyz2crd;
    static QString bin;
    static QString basisset;

  private:
    Paths();                            // constructor
};  

#endif

