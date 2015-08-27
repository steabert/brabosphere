/***************************************************************************
                          atomset.h  -  description
                             -------------------
    begin                : Mon Sep 9 2002
    copyright            : (C) 2002-2006 by Ben Swerts
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
/// Contains the declaration of the class AtomSet

#ifndef ATOMSET_H
#define ATOMSET_H

///// Forward class declarations & header files ///////////////////////////////

// STL header files
#include <vector>
using std::vector;

// Qt forward class declarations
class QColor;
class QDomDocument;
class QDomDocumentFragment;
class QDomElement;
class QString;

// Xbrabo forward class declarations
//#include "point3d.h" // gives extremely strange errors when (and only when) compiling crdfactory.cpp
template <class T> class Point3D;

///// class AtomSet ///////////////////////////////////////////////////////////
class AtomSet
{
  public:
    AtomSet();                          // constructor
    ~AtomSet();                         // destructor

    ///// public enums
    enum ChargeType{None, Mulliken, Stockholder};///< The type of charge that is stored

    ///// public member functions for adding/removing data
    void clear();                       // remove all atoms
    void reserve(const unsigned int size);        // allocates space for more atoms
    void addAtom(const Point3D<double>& location, const unsigned int atomicNumber, const QColor color, const int index = -1);         // adds an atom at position index with a special color
    void addAtom(const Point3D<double>& location, const unsigned int atomicNumber, const int index = -1);         // adds an atom at position index
    void addAtom(const double x, const double y, const double z, const unsigned int atomicNumber, const QColor color, const int index = -1);    // adds an atom at position index with a special color
    void addAtom(const double x, const double y, const double z, const unsigned int atomicNumber, const int index = -1);    // adds an atom at position index
    void removeAtom(const unsigned int index);    // removes an atom

    ///// public member functions for changing data
    void setX(const unsigned int index, const double x);    // changes the x-coordinate of atom index
    void setY(const unsigned int index, const double y);    // changes the y-coordinate of atom index
    void setZ(const unsigned int index, const double z);    // changes the z-coordinate of atom index
    void setColor(const unsigned int index, const QColor color);      // changes the color of atom index
    void setCharges(const vector<double>& charges, const ChargeType type, const QString scf = QString::null, const QString density = QString::null);  // sets all charges of a certain type
    void clearCharges(const ChargeType type);  // removes the charges of a specified type  
    void setForces(const unsigned int index, const double dx, const double dy, const double dz);    // changes the forces of atom index
    void changeBond(const double amount, const unsigned int movingAtom, const unsigned int secondAtom, const bool includeNeighbours = false);         // changes a bond length
    void changeAngle(const double amount, const unsigned int movingAtom, const unsigned int centralAtom, const unsigned int lastAtom, const bool includeNeighbours = false);        // changes a valence angle
    void changeTorsion(const double amount, const unsigned int movingAtom, const unsigned int secondAtom, const unsigned int thirdAtom, const unsigned int fourthAtom, const bool includeNeighbours = false);     // changes a torsion angle
    void transferCoordinates(const AtomSet* source);        // copies the coordinates from another AtomSet
    
    ///// public member functions for retrieving data
    unsigned int count() const;         // returns the number of atoms
    Point3D<double> coordinates(const unsigned int index) const;         // returns the coordinates for atom index
    double x(const unsigned int index) const;     // returns the x-coordinate for atom index
    double y(const unsigned int index) const;     // returns the y-coordinate for atom index
    double z(const unsigned int index) const;     // returns the z-coordinate for atom index
    unsigned int atomicNumber(const unsigned int index) const;  // returns the atomic number for atom index
    QColor color(const unsigned int index) const; // returns the color of the specified atom
    vector<unsigned int> usedAtomicNumbers() const;   // returns a sorted list of all the used atomic numbers
    void bonds(vector<unsigned int>*& first, vector<unsigned int>*& second);    // returns a list of bonds between the atoms
    unsigned int numberOfBonds(const unsigned int index) const; // returns the number of bonds for an atom
    bool isLinear() const;              // returns true if the atoms form a linear molecule
    bool isChanged() const;             // returns true if the AtomSet has changed
    double dx(const unsigned int index) const;    // returns the x-component of the force on atom index
    double dy(const unsigned int index) const;    // returns the y-component of the force on atom index
    double dz(const unsigned int index) const;    // returns the z-component of the force on atom index
    double bond(const unsigned int atom1, const unsigned int atom2) const;      // returns the distance 1-2
    double angle(const unsigned int atom1, const unsigned int atom2, const unsigned int atom3) const;         // returns the angle 1-2-3
    double torsion(const unsigned int atom1, const unsigned int atom2, const unsigned int atom3, const unsigned int atom4) const; // returns the torsion angle 1-2-3-4
    double charge(const ChargeType type, const unsigned int index) const;// the charge on atom index
    bool hasCharges(const ChargeType type) const; // returns true if charges of the specified type are present
    QString chargesSCF(const ChargeType type) const;        // Returns the type of calculation used for the determination of the charges
    QString chargesDensity(const ChargeType type) const;    // Returns the density from which the charges were determined
    bool hasForces() const;             // returns if forces are present
    Point3D<float> rotationCenter() const;        // returns the center around which the molecule can best be rotated
    bool needsExtendedFormat();         // returns true if the coordinates need to be written in BRABO's extended format in order to prevent clipping

    // public member functions for doing IO
    void loadCML(const QDomElement* root);        // loads coordinates from a CML file
    void saveCML(QDomElement* root);    // saves the coordinates to a CML file under the node root

    // static public member functions/variables
    static const unsigned int maxElements;        // the number of supported elements (elements 1 - maxElements)
    static unsigned int atomToNum(const QString& atom);     // maps elements to atomic numbers
    static QString numToAtom(const unsigned int atom);      // maps atomic numbers to elements
    static float vanderWaals(const unsigned int atom);      // returns the Van der Waals radius of atomic number atom
    static QColor stdColor(const unsigned int atom);        // returns the standard color for atomic number atom
    
  private:
    // private member functions
    void setChanged(const bool state = true);     // sets the 'changed' property
    void setGeometryChanged();          // indicates the geometry has changed
    bool addBondList(const unsigned int callingAtom, const unsigned int startAtom, const unsigned int endAtom1, const unsigned int endAtom2, std::vector<unsigned int>* result);     // returns a list of all atoms bonded to startAtom
    void clearProperties();             // clears the properties
    void updateBoxDimensions();         // updates the smallest box surrounding the atoms
    void addBonds(const vector<unsigned int>* atomList1, const vector<unsigned int>* atomList2);    // calculates all bonds between the atoms in the 2 list

    // private member data
    unsigned int numAtoms;              ///< the number of atoms
    bool changed;                       ///< = true if anything changed

    vector<Point3D<double> > coords;    ///< cartesian coordinates of the atoms with the atomic numbers as the ID's
    vector<QColor> colors;              ///< colors of the atoms
    vector<Point3D<double> >* forces;   ///< forces on the atoms
    vector<unsigned int> bonds1;        ///< the first part of the bonds array
    vector<unsigned int> bonds2;        ///< the second part of the bonds array
    vector<double>* chargesMulliken;    ///< Contains the Mulliken charges if present
    vector<double>* chargesStockholder; ///< Contains the stockholder charges if present
    QString chargesMullikenSCF;         ///< The type of SCF method used for calculating the Mulliken charges (e.g. RHF/6-31G)
    QString chargesMullikenDensity;     ///< The density used to calculate the Mulliken charges
    QString chargesStockholderSCF;      ///< The type of SCF method used for calculating the stockholder charges (e.g. RHF/6-31G)
    QString chargesStockholderDensity;  ///< The density used to calculate the stockholder charges
    Point3D<double>* boxMax;            ///< The first point of the smallest box surrounding the atoms (have to use pointers because point3d.h cannot be included)
    Point3D<double>* boxMin;            ///< The second point of the smallest box surrounding the atoms
    bool dirtyBox;                      ///< If true the box needs to be recalculated
};

#endif

