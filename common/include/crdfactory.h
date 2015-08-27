/***************************************************************************
                         crdfactory.h  -  description
                             -------------------
    begin                : Sat Jan 17 2004
    copyright            : (C) 2004-2006 by Ben Swerts
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
/// Contains the declaration of the class CrdFactory.

#ifndef CRDFACTORY_H
#define CRDFACTORY_H

///// Forward class declarations & header files ///////////////////////////////

// Qt forward class declarations
class QString;
class QStringList;

// CrdView forward class declarations
class AtomSet;

///// class CrdFactory ////////////////////////////////////////////////////////
class CrdFactory  
{
  public:
    ~CrdFactory();                      // destructor

    static unsigned int readFromFile(AtomSet* atoms, QString filename);         // reads coordinates from a predefined file and fills the AtomSet
    static unsigned int readFromFile(AtomSet* atoms);       // reads coordinates from an unknown file and fills the AtomSet (overloaded)
    static unsigned int writeToFile(AtomSet* atoms, QString filename = QString::null, bool extendedFormat = false);     // writes coordinates from the AtomSet to a (predefined) file and a certain format for .crd files
    static unsigned int convert(const QString inputFileName, const QString outputFileName, const bool extendedFormat = false);    // converts between coordinate file formats
    static unsigned int readForces(AtomSet* atoms, QString filename); // reads the forces from a predefined file and fills the AtomSet
        
    enum returnCodes{OK, Cancelled, UnknownExtension, ErrorOpen, ErrorRead, ErrorWrite, UnknownFormat, NormalFormat, ExtendedFormat};  // return codes

  private:
    CrdFactory();                       // constructor

    static QStringList supportedInputFormats();   // returns a list of the supported file extensions that can be read
    static QStringList supportedOutputFormats();  // returns a list of the supported file extensions that can be written
    static bool validInputFormat(const QString filename);   // returns true if the given extension can be read
    static bool validForceInputFormat(const QString filename);        // returns true if forces can be read for the given extension
    static bool validOutputFormat(const QString filename);  // returns true if the given extension can be written
    static bool braboExtension(const QString filename);     // returns true if the extension is a Brabo format
    static bool xmolExtension(const QString filename);      // returns true if the extension is an Xmol format
    static bool gaussianExtension(const QString filename);  // returns true if the extension is a Gaussian format

    static unsigned int readBraboFile(AtomSet* atoms, const QString filename);  // reads a Brabo .crd/.c00
    static unsigned int writeBraboFile(AtomSet* atoms, const QString filename, const bool extendedFormat);    // reads a Brabo .crd/.c00
    static unsigned int readBraboForces(AtomSet* atoms, const QString filename);// reads a Brabo .pun/.f00
    static void readCrdAtoms(AtomSet* atoms, QStringList& lines, const bool extendedFormat);        // fills the AtomSet with the coordinates from the QStringList
    static void readPunchForces(AtomSet* atoms, QStringList& lines, const bool extendedFormat);     // fills the AtomSet with the forces from the QStringList
    static unsigned int determineCrdFormat(QStringList& lines);       // determines the format of the .crd file parsed into the QStringList
    static unsigned int determinePunchFormat(QStringList& lines);     // determines the format of the .pun file parsed into the QStringList
    static bool possiblyNormalCrd(const QString line);      // returns true if the QString can contain coordinates in normal format
    static bool possiblyExtendedCrd(const QString line);    // returns true if the QString can contain coordinates in extended format

    static unsigned int readXmolFile(AtomSet* atoms, const QString filename);      // reads an Xmol .xyz file
    static unsigned int readGaussianFile(AtomSet* atoms, const QString filename);  // reads a Gaussian .fchk file

	  static const double AUTOANG;
};

#endif

