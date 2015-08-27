/***************************************************************************
                          basisset.h  -  description
                             -------------------
    begin                : Mon Jun 30 2003
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
/// Contains the declaration of the class Basisset

#ifndef BASISSET_H
#define BASISSET_H

///// Forward class declarations & header files ///////////////////////////////

// C++ header files
#include <vector>

// Qt header files
#include <qmap.h> // forward class declaration doesn't work

// Qt forward class declarations
class QString;
class QStringList;

///// class Basisset //////////////////////////////////////////////////////////
class Basisset
{
  public:
    ///// public static member functions
    static unsigned int basisToNum(const QString& basis);   // returns the index to the given basis set
    static QString numToBasis(const unsigned int basis);    // returns the basis set corresponding to the given index
    static QString numToBasisDir(const unsigned int basis); // returns the subdirectory corresponding to the given index
    static QString extension();         // returns the extension used for the basisset files
    static unsigned int maxBasissets(); // returns the number of basissets available
    static unsigned int contractedFunctions(const unsigned int basis, const unsigned int atom);     // returns the number of contracted gaussian functions for a specific basisset and atom type
    
  private:
    ///// constructor/destructor
    Basisset();                         // constructor
    ~Basisset();                        // destructor
    
    ///// private static member functions
    static void initBasissets();        // initializes the static class variables class
    static bool isInitialized;          // is set to true when initBasissets() has been called
    
    ///// private static member variables
    static QStringList numToBas;        // Holds conversion data.
    static QStringList numToDir;        // Holds conversion data.
    static QMap<QString, unsigned int> basToNum;  // Holds conversion data.
    static unsigned int numBasissets;   // Holds the number of basissets available.
    static std::vector< std::vector<unsigned int> > ncf;    // Holds the number of contracted functions for each basis set and row in the periodic table.    
};
#endif

