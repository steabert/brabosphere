/***************************************************************************
                         atomset.cpp  -  description
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

///// Comments ////////////////////////////////////////////////////////////////

/*! 
  \class AtomSet
  \brief Stores the atoms of a molecular system.

  Each atom has an index which starts at 0. This class can accomodate as many
  atoms as RAM can hold.

  Atomic properties that can be/are stored:
  \arg coordinates (x, y, z)
  \arg color
  \arg atomic number
  \arg charge (with type = "Mulliken" or "stockholder" and SCFmethod and density used to calculate them)
  \arg atomic dipole, quadrupole, octopole, etc.

  Diatomic properties that can be/are stored:
  \arg Mayer bond orders (N*N values)

  Atoms with atomic number between 1 and maxElements (54) can be stored. Other
  atomic numbers are assigned a zero for the unknown atom type "?"

*/
/// \file 
/// Contains the implementation of the class AtomSet.

///// Header files ////////////////////////////////////////////////////////////

// C++ header files
#include <cmath>         

// STL header files
#include <algorithm>

// Qt header files
#include <qcolor.h>
#include <qdom.h>
#include <qstring.h>
#include <qstringlist.h>

#include <qdatetime.h>

// Xbrabo header files
#include "atomset.h"
#include "domutils.h"
#include "vector3d.h" // includes the Point3D header file

///////////////////////////////////////////////////////////////////////////////
///// Public Member Functions                                             /////
///////////////////////////////////////////////////////////////////////////////

///// Constructor /////////////////////////////////////////////////////////////
AtomSet::AtomSet() :
  numAtoms(0),
  forces(0),
  chargesMulliken(0),
  chargesStockholder(0),
  boxMax(new Point3D<double>()),
  boxMin(new Point3D<double>()),
  dirtyBox(true)
/// The default constructor.
{

}

///// Destructor //////////////////////////////////////////////////////////////
AtomSet::~AtomSet()
/// The default destructor.
{
  clearProperties(); // releases all allocated memory
  delete boxMax;
  delete boxMin;
}

///// clear ///////////////////////////////////////////////////////////////////
void AtomSet::clear()
/// Removes all atoms.
{ 
  coords.clear();
  colors.clear();
  clearProperties();
  bonds1.clear();
  bonds2.clear();
  numAtoms = 0;
  setChanged(false);
}

///// reserve /////////////////////////////////////////////////////////////////
void AtomSet::reserve(const unsigned int size)
/// Resizes all the vectors to accomodate the requested number of atoms.
{ 
  if(size <= coords.size())
    return;

  coords.reserve(size);
  colors.reserve(size);
  if(forces != 0)
    forces->reserve(size);
  if(chargesMulliken != 0)
    chargesMulliken->reserve(size);
  if(chargesStockholder != 0)
    chargesStockholder->reserve(size);
}

///// addAtom /////////////////////////////////////////////////////////////////
void AtomSet::addAtom(const Point3D<double>& location, const unsigned int atomicNumber, const QColor color, const int index)
/// Adds an atom of type \a atomicNumber with
/// coordinates \a location and color \a color at position \a index. 
/// If \a index is negative (the default) the atom is added at the end.
/// \warning Implies resetting all properties (forces, charges, etc.)
{
  ///// return if the limit is reached (unlikely)
  if(numAtoms == coords.max_size())
  {
    qDebug("AtomSet::addAtom: the maximum number of atoms has been reached.");
    return;
  }

  ///// fix the atomic number if needed
  unsigned int atomNum = atomicNumber;
  if(atomNum > maxElements)
    atomNum = 0; // the unknown element
  ///// make a local copy with the atomic number added
  Point3D<double> newAtom(location);
  newAtom.setID(atomNum);

  ///// add the atom 
  if(index < 0 || static_cast<unsigned int>(index) > numAtoms)
  {
    ///// add the atom at the end
    coords.push_back(newAtom);
    colors.push_back(color);
  }
  else
  {
    if(static_cast<unsigned int>(index) < numAtoms - 1)
    {
      ///// add the atom at position index
      vector<Point3D<double> >::iterator it = coords.begin();
      it += index;
      coords.insert(it, newAtom);
      vector<QColor>::iterator it2 = colors.begin();
      it2 += index;
      colors.insert(it2, color);
    }
  }
  numAtoms++;

  setGeometryChanged(); // also calls setChanged and clearProperties
}

///// addAtom (overloaded) ////////////////////////////////////////////////////
void AtomSet::addAtom(const Point3D<double>& location, const unsigned int atomicNumber, const int index)
/// Adds an atom. \overload
{
  addAtom(location, atomicNumber, stdColor(atomicNumber), index);
}

///// addAtom (overloaded) ////////////////////////////////////////////////////
void AtomSet::addAtom(const double x, const double y, const double z, const unsigned int atomicNumber, const QColor color, const int index)
/// Adds an atom with cartesian coordinates and a special color. \overload
{
  addAtom(Point3D<double>(x, y, z), atomicNumber, color, index);
}

///// addAtom (overloaded) ////////////////////////////////////////////////////
void AtomSet::addAtom(const double x, const double y, const double z, const unsigned int atomicNumber, const int index)
/// Adds an atom with cartesian coordinates.
{
  addAtom(Point3D<double>(x, y, z), atomicNumber, stdColor(atomicNumber), index);
}

///// removeAtom //////////////////////////////////////////////////////////////
void AtomSet::removeAtom(const unsigned int index)
/// Removes the atom at position index.
/// \warning Implies resetting all properties (forces, charges, etc.)
{
  if((numAtoms > 0) && (index < numAtoms))
  {
    if(index == numAtoms - 1)
    {
      coords.pop_back();
      colors.pop_back();
    }
    else
    {
      vector<Point3D<double> >::iterator it = coords.begin();
      it += index;
      coords.erase(it);
      vector<QColor>::iterator it2 = colors.begin();
      it2 += index;
      colors.erase(it2);
    }
    numAtoms--;
    setGeometryChanged();
  }
}

///// setX ////////////////////////////////////////////////////////////////////
void AtomSet::setX(const unsigned int index, const double x)
/// Sets the x-coordinate of the atom at position \c index.
/// \warning Implies resetting all properties (forces, charges, etc.)
{  
  if(index >= numAtoms)
    return;
  
  coords[index].setValues(x, coords[index].y(), coords[index].z());
  setGeometryChanged();
}

///// setY ////////////////////////////////////////////////////////////////////
void AtomSet::setY(const unsigned int index, const double y)
/// Sets the y-coordinate of the atom at position \c index.
/// \warning Implies resetting all properties (forces, charges, etc.)
{ 
  if(index >= numAtoms)
    return;
  
  coords[index].setValues(coords[index].x(), y, coords[index].z());
  setGeometryChanged();  
}

///// setZ ////////////////////////////////////////////////////////////////////
void AtomSet::setZ(const unsigned int index, const double z)
/// Sets the z-coordinate of the atom at position index.
/// \warning Implies resetting all properties (forces, charges, etc.)
{  
  if(index >= numAtoms)
    return;
  
  coords[index].setValues(coords[index].x(), coords[index].y(), z);
  setGeometryChanged();
}

///// setColor /////////////////////////////////////////////////////////////////
void AtomSet::setColor(const unsigned int index, const QColor color)
/// Sets the color of the atom at position index.
{
  if(index >= numAtoms)
    return;
  
  colors[index] = color;
  setChanged();
}

///// setCharges ///////////////////////////////////////////////////////////////
void AtomSet::setCharges(const vector<double>& charges, const ChargeType type, const QString scf, const QString density)
/// Sets the charges on all atoms. 
/// \param[in] charges: the vector containing all charges. It's size has to
///                     be equal to count() or no charges will be assigned.
/// \param[in] type: The type of charges to be added/updated.
/// \param[in] scf: The method with which the charges were calculated.
/// \param[in] density: The density from which the charges were calculated.
{
  if(charges.size() != numAtoms || type == None)
    return;
  
  if(type == Mulliken)
  {
    ///// Mulliken charges
    if(chargesMulliken == 0)
    {
      chargesMulliken = new vector<double>();
      chargesMulliken->reserve(numAtoms);
    }
    chargesMulliken->assign(charges.begin(), charges.end());
    chargesMullikenSCF = scf;
    chargesMullikenDensity = density;
  }
  else
  {
    ///// stockholder charges
    if(chargesStockholder == 0)
    {
      chargesStockholder = new vector<double>();
      chargesStockholder->reserve(numAtoms);
    }
    chargesStockholder->assign(charges.begin(), charges.end());
    chargesStockholderSCF = scf;
    chargesStockholderDensity = density;
  }
  setChanged();
}

///// clearCharges ////////////////////////////////////////////////////////////
void AtomSet::clearCharges(const ChargeType type)
/// Clears the charges of a specific type.
{
  if(type == None)
    return;
  else if(type == Mulliken)
  {
    if(chargesMulliken == 0)
      return;
    delete chargesMulliken;
    chargesMulliken = 0;
  }
  else
  {
    if(chargesStockholder == 0)
      return;
    delete chargesStockholder;
    chargesStockholder = 0;
  }
  setChanged();
}

///// setForces ///////////////////////////////////////////////////////////////
void AtomSet::setForces(const unsigned int index, const double dx, const double dy, const double dz)
/// Sets the forces on the atom at position index.
{
  if(index >= numAtoms)
    return;

  if(forces == 0)
    forces = new vector<Point3D<double> >(numAtoms);
  
  forces->operator[](index) = Point3D<double>(dx, dy, dz);
}

///// changeBond //////////////////////////////////////////////////////////////
void AtomSet::changeBond(const double amount, const unsigned int movingAtom, const unsigned int secondAtom, const bool includeNeighbours)
/// Changes the bond length between two atoms. This is done by moving
/// movingAtom with respect to secondAtom. If includeNeighbours = true the
/// atoms bonded to movingAtom will be moved by the same amount.
/// \warning Implies resetting all properties (forces, charges, etc.)
{
  ///// check the validity of the input
  if(movingAtom >= numAtoms || secondAtom >= numAtoms)
    return;
  if(fabs(amount) < Point3D<double>::TOLERANCE)
    return;

  ///// build a list of atoms to move
  vector<unsigned int> moveableAtoms;
  if(includeNeighbours)
  {
    ///// call the bonds routine to be sure the bond list gets updated
    {
      vector<unsigned int>* tempBonds1, * tempBonds2;
      bonds(tempBonds1, tempBonds2);
    }
    ///// fill the atom list
    if(!addBondList(movingAtom, movingAtom, secondAtom, secondAtom, &moveableAtoms))
      moveableAtoms.clear();
  }
  moveableAtoms.push_back(movingAtom);
  
  ///// determine the amount of displacement
  const Vector3D<double> oldPosition(coords[secondAtom], coords[movingAtom]);
  Vector3D<double> newPosition = oldPosition;
  if((oldPosition.length() + amount) < 0.1)
    newPosition.setLength(0.1); // the bond would get too small or even flip over
  else
    newPosition.changeLength(amount); // normal displacement
  const double dx = newPosition.x() - oldPosition.x();
  const double dy = newPosition.y() - oldPosition.y();
  const double dz = newPosition.z() - oldPosition.z();

  ///// move all atoms
  vector<unsigned int>::iterator it = moveableAtoms.begin();
  while(it != moveableAtoms.end())
    coords[*it++].add(Point3D<double>(dx, dy, dz));

  setGeometryChanged();
}

///// changeAngle /////////////////////////////////////////////////////////////
void AtomSet::changeAngle(const double amount, const unsigned int movingAtom, const unsigned int centralAtom, const unsigned int lastAtom, const bool includeNeighbours)
/// Changes the angle between two bonds. This is done between the bonds
/// movingAtom-centralAtom and centralAtom-lastAtom by rotating movingAtom
/// with respect to the other atoms. If includeNeighbours = true the
/// atoms bonded to movingAtom will be rotated by the same amount.
/// \warning Implies resetting all properties (forces, charges, etc.)
{
  ///// check the validity of the input
  if(movingAtom >= numAtoms || centralAtom >= numAtoms || lastAtom >= numAtoms)
    return;
  if(fabs(amount) < Point3D<double>::TOLERANCE)
    return;

  ///// build a list of atoms to move
  vector<unsigned int> moveableAtoms;
  if(includeNeighbours)
  {
    ///// call the bonds routine to be sure the bond list gets updated
    {
      vector<unsigned int>* tempBonds1, * tempBonds2;
      bonds(tempBonds1, tempBonds2);
    }
    ///// fill the atom list
    if(!addBondList(movingAtom, movingAtom, centralAtom, lastAtom, &moveableAtoms))
      moveableAtoms.clear();
  }
  moveableAtoms.push_back(movingAtom);

  ///// determine the vector to rotate around => cross product of vectors along bonds
  Vector3D<double> bond1(coords[centralAtom], coords[movingAtom]);
  Vector3D<double> bond2(coords[centralAtom], coords[lastAtom]);
  Vector3D<double> axis = bond1.cross(bond2);

  ///// rotate all atoms
  std::vector<unsigned int>::iterator it = moveableAtoms.begin();
  while(it != moveableAtoms.end())
  {
    Vector3D<double> rotatebond(coords[centralAtom], coords[*it]);
    axis = rotatebond.cross(bond2);
    rotatebond.rotate(axis, amount);
    coords[*it].setValues(coords[centralAtom].x() + rotatebond.x(),
                          coords[centralAtom].y() + rotatebond.y(),
                          coords[centralAtom].z() + rotatebond.z());
    it++;
  }
  setGeometryChanged();
}

///// changeTorsion /////////////////////////////////////////////////////////////
void AtomSet::changeTorsion(const double amount, const unsigned int movingAtom, const unsigned int secondAtom, const unsigned int thirdAtom, const unsigned int fourthAtom, const bool includeNeighbours)
/// Changes the torsion angle between two bonds. This is done between the bonds
/// movingAtom-secondAtom and thirdAtom-fourthAtom by rotating movingAtom
/// with respect to the central bond secondAtom-thirdAtom.
/// If includeNeighbours = true the atoms bonded to movingAtom will be rotated
/// by the same amount.
/// \warning Implies resetting all properties (forces, charges, etc.)
{
  ///// check the validity of the input
  if(movingAtom >= numAtoms || secondAtom >= numAtoms || thirdAtom >= numAtoms || fourthAtom >= numAtoms)
    return;
  if(fabs(amount) < Point3D<double>::TOLERANCE)
    return;

  ///// build a list of atoms to move
  vector<unsigned int> moveableAtoms;
  if(includeNeighbours)
  {
    ///// call the bonds routine to be sure the bond list gets updated
    {
      vector<unsigned int>* tempBonds1, * tempBonds2;
      bonds(tempBonds1, tempBonds2);
    }
    ///// fill the atom list
    if(!addBondList(secondAtom, secondAtom, thirdAtom, thirdAtom, &moveableAtoms))
    {
      moveableAtoms.clear();
      moveableAtoms.push_back(movingAtom);
    }
    else
    {
      ///// check whether movingAtom is already in the list and add it otherwise
      vector<unsigned int>::iterator it = std::find(moveableAtoms.begin(), moveableAtoms.end(), movingAtom);
      if(it == moveableAtoms.end())
        moveableAtoms.push_back(movingAtom);
    }
  }
  else
    moveableAtoms.push_back(movingAtom);

  ///// determine the vector to rotate around => central bond
  Vector3D<double> centralbond(coords[secondAtom], coords[thirdAtom]);

  ///// rotate all atoms
  std::vector<unsigned int>::iterator it = moveableAtoms.begin();
  while(it != moveableAtoms.end())
  {
    Vector3D<double> rotatebond(coords[secondAtom], coords[*it]);
    rotatebond.rotate(centralbond, -amount);
    coords[*it].setValues(coords[secondAtom].x() + rotatebond.x(),
                          coords[secondAtom].y() + rotatebond.y(),
                          coords[secondAtom].z() + rotatebond.z());
    it++;
  }
  setGeometryChanged();
}

///// transfer ////////////////////////////////////////////////////////////////
void AtomSet::transferCoordinates(const AtomSet* source)
/// Copies the coordinates from another AtomSet and resets the properties by default.
/// It resembles a limited copy constructor and only works when both sets contain
/// the same amount of atoms. Atom types are not checked.
/// \warning Implies resetting all properties (forces, charges, etc.)

{
  if(count() != source->count())
    return;

  ///// copy the coordsX, Y and Z vectors
  coords.assign(source->coords.begin(), source->coords.end());

  setGeometryChanged();
}

///// count ///////////////////////////////////////////////////////////////////
unsigned int AtomSet::count() const
/// Returns the number of atoms.
{
  return numAtoms;
}

///// coordinates /////////////////////////////////////////////////////////////
Point3D<double> AtomSet::coordinates(const unsigned int index) const
/// Returns the coordinates of the atom at position \c index.
{
  if(index < numAtoms)
    return coords[index];
  else
    return Point3D<double>();
}

///// x ///////////////////////////////////////////////////////////////////////
double AtomSet::x(const unsigned int index) const
/// Returns the x-coordinate of the atom at position \c index.
{
  if(index < numAtoms)
    return coords[index].x();
  else
    return 0.0;
}

///// y ///////////////////////////////////////////////////////////////////////
double AtomSet::y(const unsigned int index) const
/// Returns the y-coordinate of the atom at position \c index.
{
  if(index < numAtoms)
    return coords[index].y();
  else
    return 0.0;
}

///// z ///////////////////////////////////////////////////////////////////////
double AtomSet::z(const unsigned int index) const
/// Returns the z-coordinate of the atom at position \c index
{
  if(index < numAtoms)
    return coords[index].z();
  else
    return 0.0;
}

///// atomicNumber ////////////////////////////////////////////////////////////
unsigned int AtomSet::atomicNumber(const unsigned int index) const
/// Returns the atomic number of the atom at position \c index.
{
  if(index < numAtoms)
    return coords[index].id();
  else
    return 0;
}

///// color ///////////////////////////////////////////////////////////////////
QColor AtomSet::color(const unsigned int index) const
/// Returns the color of the atom at position \c index.
{
  if(index < numAtoms)
    return colors[index];
  else
    return QColor(0,0,0);
}

///// usedAtomicNumbers ///////////////////////////////////////////////////////
vector<unsigned int> AtomSet::usedAtomicNumbers() const
/// Returns a vector containing a sorted list of
/// the atomic numbers present in the set. Only the known atoms are returned.
{
  vector<unsigned int> result;
  for(unsigned int i = 1; i <= maxElements; i++)
  {
    ///// check if this atomic number appears in the list atomicNumbers
    //if(std::find(atomicNumbers.begin(), atomicNumbers.end(), i) != atomicNumbers.end())
    //  result.push_back(i); // found an occurence
    for(unsigned int j = 0; j < numAtoms; j++)
    {
      if(coords[j].id() == i)
      {
        result.push_back(i);
        break;
      }
    }
  }
  return result;
}

///// bonds ///////////////////////////////////////////////////////////////////
void AtomSet::bonds(vector<unsigned int>*& first, vector<unsigned int>*& second)
/// Returns a list of the bonds between the atoms. Unknown atoms can never have
/// bonds. This routine puts atoms in boxes of 4x4x4 Angstrom and only looks for
/// bonds between neighbouring boxes.
{
  QTime timer;
  timer.start();
  if(bonds1.empty() && numAtoms != 0) // only recalculate when necessary
  {
    // reserve some space (guesstimate of the number of bonds to be generated, normally between 0.67x and 1x the number of atoms)
    bonds1.reserve(numAtoms);
    bonds2.reserve(numAtoms);
    // update the dimensions of the box
    updateBoxDimensions();
    // divide it into cells of 4x4x4 Angstrom 
    // (4.0A because largest VdW radius = 3.0A => largest distance = 1.25*(3.0 + 3.0) = 7.5A < 2 * 4.0A)
    const double cellSize = 4.0;
    Point3D<unsigned int> numCells(static_cast<unsigned int>((boxMax->x() - boxMin->x())/cellSize) + 1,
                                   static_cast<unsigned int>((boxMax->y() - boxMin->y())/cellSize) + 1,
                                   static_cast<unsigned int>((boxMax->z() - boxMin->z())/cellSize) + 1);
    unsigned int cellsXY = numCells.x() * numCells.y();
    unsigned int totalCells = cellsXY * numCells.z();

    // assign all atoms to their cells
    vector< vector<unsigned int> > atomCell(totalCells); // each cell contains a vector of all assigned atom indices
    for(unsigned int i = 0; i < numAtoms; i++)
    {
      if(coords[i].id() != 0) // check for point charges, because they never have bonds
      {
        unsigned int planeX = static_cast<unsigned int>((coords[i].x() - boxMin->x())/cellSize);
        unsigned int planeY = static_cast<unsigned int>((coords[i].y() - boxMin->y())/cellSize);
        unsigned int planeZ = static_cast<unsigned int>((coords[i].z() - boxMin->z())/cellSize);
        // the atom belongs to the cell at the crossing of planeX, planeY and planeZ
        atomCell[planeX + numCells.x()*planeY + cellsXY*planeZ].push_back(i);
      }
    }
    /*qDebug("atoms in each cell:");
    for(unsigned int i = 0; i < totalCells; i++)
    {
      qDebug("cell %d: %d atoms", i, atomCell[i].size());
      for(unsigned int j = 0; j < atomCell[i].size(); j++)
        qDebug("contains atom %d", atomCell[i][j]+1);
    }*/

    // loop over combinations of all cells and calculate bonds only if the cells
    // are connected and cell2 has a larger index than cell1 in each direction
    unsigned int cellIndex = 0;
    vector<unsigned int>* atomList;
    // loop over the first set of cells
    for(unsigned int cellZ = 0; cellZ < numCells.z(); cellZ++)
    {
      for(unsigned int cellY = 0; cellY < numCells.y(); cellY++)
      {
        for(unsigned int cellX = 0; cellX < numCells.x(); cellX++)
        {
          atomList = &atomCell[cellIndex++];
          if(atomList->size() == 0)
            continue; // an empty cell can be skipped
          ///// find all surrounding cells (X/Y/Z, X+1/Y/Z, X/Y+1/Z, X/Y/Z+1, X+1/Y+1/Z, X+1/Y/Z+1, X/Y+1/Z+1, X+1/Y+1/Z+1,
          /////                             X-1/Y/Z+1, X+1/Y+1/Z-1, X/Y+1/Z-1, X-1/Y+1/Z-1, X-1/Y+1/Z, X-1/Y+1/Z+1)
          ///// other neighbouring cells (remaining of 13 total of 26) will already have been combined with this cell before 
          ///// (no double counting)
          // X/Y/Z -> intra-cell bonds
          addBonds(atomList, atomList);
          // X+1/Y/Z
          if(cellX != (numCells.x() - 1))
            addBonds(atomList, &atomCell[cellX+1 + numCells.x()*cellY + cellsXY*cellZ]);
          // X/Y+1/Z
          if(cellY != (numCells.y() - 1))
           addBonds(atomList, &atomCell[cellX + numCells.x()*(cellY+1) + cellsXY*cellZ]);
          // X/Y/Z+1
          if(cellZ != (numCells.z() - 1))
            addBonds(atomList, &atomCell[cellX + numCells.x()*cellY + cellsXY*(cellZ+1)]);
          // X+1/Y+1/Z
          if(cellX != (numCells.x() - 1) && cellY != (numCells.y() - 1))
            addBonds(atomList, &atomCell[(cellX+1) + numCells.x()*(cellY+1) + cellsXY*cellZ]);
          // X+1/Y/Z+1
          if(cellX != (numCells.x() - 1) && cellZ != (numCells.z() - 1))
            addBonds(atomList, &atomCell[(cellX+1) + numCells.x()*cellY + cellsXY*(cellZ+1)]);
          // X/Y+1/Z+1
          if(cellY != (numCells.y() - 1) && cellZ != (numCells.z() - 1))
            addBonds(atomList, &atomCell[cellX + numCells.x()*(cellY+1) + cellsXY*(cellZ+1)]);
          // X+1/Y+1/Z+1
          if(cellX != (numCells.x() - 1) && cellY != (numCells.y() - 1) && cellZ != (numCells.z() - 1))
            addBonds(atomList, &atomCell[(cellX+1) + numCells.x()*(cellY+1) + cellsXY*(cellZ+1)]);
          // X-1/Y/Z+1
          if(cellX != 0 && cellZ != (numCells.z() - 1))
            addBonds(atomList, &atomCell[(cellX-1) + numCells.x()*cellY + cellsXY*(cellZ+1)]);
          // X+1/Y+1/Z-1
          if(cellX != (numCells.x() - 1) && cellY != (numCells.y() - 1) && cellZ != 0)
            addBonds(atomList, &atomCell[(cellX+1) + numCells.x()*(cellY+1) + cellsXY*(cellZ-1)]);
          // X/Y+1/Z-1
          if(cellY != (numCells.y() - 1) && cellZ != 0)
            addBonds(atomList, &atomCell[cellX + numCells.x()*(cellY+1) + cellsXY*(cellZ-1)]);
          // X-1/Y+1/Z-1
          if(cellX != 0 && cellY != (numCells.y() - 1) && cellZ != 0)
            addBonds(atomList, &atomCell[(cellX-1) + numCells.x()*(cellY+1) + cellsXY*(cellZ-1)]);
          // X-1/Y+1/Z
          if(cellX != 0 && cellY != (numCells.y() - 1))
            addBonds(atomList, &atomCell[(cellX-1) + numCells.x()*(cellY+1) + cellsXY*cellZ]);
          // X-1/Y+1/Z+1
          if(cellX != 0 && cellY != (numCells.y() - 1) && cellZ != (numCells.z() - 1))
            addBonds(atomList, &atomCell[(cellX-1) + numCells.x()*(cellY+1) + cellsXY*(cellZ+1)]);
        }
      }
    }
    qDebug("bonds generation took %f seconds", timer.restart()/1000.0f);
  }
  // old unoptimized code (44 times slower for 8870 atoms of acetone cluster, 25 times slower for GFP)
  /*if(bonds1.empty() && numAtoms != 0) // only recalculate when necessary
  {
    float distance2, refdistance, dx, dy, dz;
    unsigned int numBonds = 0;
    unsigned int atomNumI, atomNumJ;

    ///// do a double loop over all atoms and check whether the sum of their
    ///// Van der Waals radii is less than the distance between them 
    ///// use the squared distances for speed
    for(unsigned int i = 0; i < numAtoms; i++)
    {
      atomNumI = atomicNumbers[i];
      if(atomNumI == 0)
        continue;
      for(unsigned int j = 0; j < i; j++)
      {
        atomNumJ = atomicNumbers[j];
        if(atomNumJ == 0)
          continue;
        dx = static_cast<float>(coords[i].x() - coords[j].x());
        dy = static_cast<float>(coords[i].y() - coords[j].y());
        dz = static_cast<float>(coords[i].z() - coords[j].z());
        distance2 = dx*dx + dy*dy +dz*dz;
        refdistance = 1.25f*(vanderWaals(atomNumI) + vanderWaals(atomNumJ));
        if(distance2 <= refdistance*refdistance)
        {
          numBonds++;
          /*if(numBonds == bonds1.max_size())
          {
            qDebug("AtomSet::bonds: the maximum number of bonds has been reached.");
            first = &bonds1;
            second = &bonds2;
            return;
          }* /
          bonds1.push_back(i);
          bonds2.push_back(j);
        }
      }
    }
    qDebug("old bonds generation took %f seconds", timer.restart()/1000.0f);
  }
  */

  first = &bonds1;
  second = &bonds2;
}

///// numberOfBonds ///////////////////////////////////////////////////////////
unsigned int AtomSet::numberOfBonds(unsigned int index) const
/// Returns the number of bonds an atom has.
{
  if(index >= numAtoms || atomicNumber(index) == 0)
    return 0;

  unsigned int result = 0;
  float distance2, refdistance, dx, dy, dz;

  ///// similar to bonds-code
  for(unsigned int i = 0; i < numAtoms; i++)
  {
    if(i == index || atomicNumber(i) == 0)
      continue;

    dx = static_cast<float>(coords[i].x() - coords[index].x());
    dy = static_cast<float>(coords[i].y() - coords[index].y());
    dz = static_cast<float>(coords[i].z() - coords[index].z());

    distance2 = dx*dx + dy*dy + dz*dz;
    refdistance = 1.25f*(vanderWaals(atomicNumber(i)) + vanderWaals(atomicNumber(index)));
    
    if(distance2 <= refdistance*refdistance)
      result++;
  }
  return result;
}

///// isLinear ////////////////////////////////////////////////////////////////
bool AtomSet::isLinear() const
/// Returns true if the atoms form a linear molecule.
{
  if(numAtoms < 3)
    return true; // 2 atoms or less are always on a line

  ///// check whether each point (3-numAtoms) is collinear with the points 1 and 2
  ///// => (x2-x1)/(x3-x1) = (y2-y1)/(y3-y1) = (z2-z1)/(z3-z1) (from mathforum.org FAQ)
  ///// => (x2-x1)(y3-y1) == (y2-y1)(x3-x1) && (y2-y1)(z3-z1) == (z2-z1)(y3-y1)
  double dx10 = coords[1].x() - coords[0].x();
  double dy10 = coords[1].y() - coords[0].y();
  double dz10 = coords[1].z() - coords[0].z();

  for(unsigned int i = 2; i < numAtoms; i++)
  {
    //double test1 = (x(1) - x(0)) * (y(i) - y(0));
    //double test2 = (y(1) - y(0)) * (x(i) - x(0));
    //double test3 = (y(1) - y(0)) * (z(i) - z(0));
    //double test4 = (z(1) - z(0)) * (y(i) - y(0));
    double test1 = dx10 * (coords[i].y() - coords[0].y());
    double test2 = dy10 * (coords[i].x() - coords[0].x());
    double test3 = dy10 * (coords[i].z() - coords[0].z());
    double test4 = dz10 * (coords[i].y() - coords[0].y());
    if((fabs(test1 - test2) > Point3D<double>::TOLERANCE) || (fabs(test3 - test4) > Point3D<double>::TOLERANCE))
      return false;
  }
  ///// all collinearity tests passed
  return true;
}

///// isChanged ///////////////////////////////////////////////////////////////
bool AtomSet::isChanged() const
/// Returns true if the AtomSet has changed.
{
  return changed;
}

///// dx //////////////////////////////////////////////////////////////////////
double AtomSet::dx(const unsigned int index) const
/// Returns the x-component of the force on the atom
/// at position index.
{
  if(forces == 0 || index >= numAtoms)
    return 0.0;
  
  return forces->operator[](index).x();
}

///// dy //////////////////////////////////////////////////////////////////////
double AtomSet::dy(const unsigned int index) const
/// Returns the y-component of the force on the atom
/// at position index.
{
  if(forces == 0 || index >= numAtoms)
    return 0.0;
  
  return forces->operator[](index).y();
}

///// dz //////////////////////////////////////////////////////////////////////
double AtomSet::dz(const unsigned int index) const
/// Returns the z-component of the force on the atom
/// at position index.
{
  if(forces == 0 || index >= numAtoms)
    return 0.0;
  
  return forces->operator[](index).z();
}

///// bond ////////////////////////////////////////////////////////////////////
double AtomSet::bond(const unsigned int atom1, const unsigned int atom2) const
/// Returns the distance between the two atoms.
{
  Vector3D<double> bondSize(coords[atom1], coords[atom2]);
  return bondSize.length();
}

///// angle ///////////////////////////////////////////////////////////////////
double AtomSet::angle(const unsigned int atom1, const unsigned int atom2, const unsigned int atom3) const
/// Returns the value of the valence angle 1-2-3.
{
  Vector3D<double> bond1(coords[atom2], coords[atom1]);
  Vector3D<double> bond2(coords[atom2], coords[atom3]);
  return bond1.angle(bond2);
}

///// torsion /////////////////////////////////////////////////////////////////
double AtomSet::torsion(const unsigned int atom1, const unsigned int atom2, const unsigned int atom3, const unsigned int atom4) const
/// Returns the value of the torsion angle 1-2-3-4.
{
  Vector3D<double> bond1(coords[atom2], coords[atom1]);
  Vector3D<double> centralbond(coords[atom2], coords[atom3]);
  Vector3D<double> bond2(coords[atom3], coords[atom4]);
  return bond1.torsion(bond2, centralbond);
}

///// charge //////////////////////////////////////////////////////////////////
double AtomSet::charge(const ChargeType type, const unsigned int index) const
/// Returns the charge on the atom at position index for a specific charge type.
{
  if(index >= numAtoms || type == None)
    return 0.0;

  if(type == Mulliken)
  {
    if(chargesMulliken == 0)
      return 0.0;

    return chargesMulliken->operator[](index);
  }
  else
  {
    if(chargesStockholder == 0)
      return 0.0;

    return chargesStockholder->operator[](index);
  }
}

//// hasCharges ///////////////////////////////////////////////////////////////
bool AtomSet::hasCharges(const ChargeType type) const
/// Returns whether charges of the specified type are present.
{
  if((type == Mulliken && chargesMulliken != 0) || 
     (type == Stockholder && chargesStockholder != 0))
    return true;

  return false;
}

//// chargesSCF ///////////////////////////////////////////////////////////////
QString AtomSet::chargesSCF(const ChargeType type) const
/// Returns the SCF method used to determine the charges. If no charges of the
/// specified type are available an empty string is returned.
{
  if(type == Mulliken && chargesMulliken != 0)
      return chargesMullikenSCF;
  else if(type == Stockholder && chargesStockholder != 0)
      return chargesStockholderSCF;
  
  return QString::null;
}

//// chargesDensity ///////////////////////////////////////////////////////////
QString AtomSet::chargesDensity(const ChargeType type) const
/// Returns the density used to determine the charges. If no charges of the
/// specified type are available an empty string is returned.
{
  if(type == Mulliken && chargesMulliken != 0)
      return chargesMullikenDensity;
  else if(type == Stockholder && chargesStockholder != 0)
      return chargesStockholderDensity;
  
  return QString::null;
}

//// hasForces ////////////////////////////////////////////////////////////////
bool AtomSet::hasForces() const
/// Returns whether forces are present
{
  return forces != 0;
}

//// rotationCenter ///////////////////////////////////////////////////////////
Point3D<float> AtomSet::rotationCenter() const
/// Returns the optimal center of rotation for the molecule.
{
  return Point3D<float>(static_cast<float>((boxMax->x() + boxMin->x())/2.0), 
                        static_cast<float>((boxMax->y() + boxMin->y())/2.0), 
                        static_cast<float>((boxMax->z() + boxMin->z())/2.0));
}

//// needsExtendedFormat //////////////////////////////////////////////////////
bool AtomSet::needsExtendedFormat()
/// Returns true if the coordinates need to be written in BRABO's extended 
/// format in order to prevent clipping. This is needed as soon as one coordinate
/// value is at least 100.0 or a negative value at least -10.0.
{
  updateBoxDimensions();
  return(boxMax->x() >= 100.0 || boxMax->y() >= 100.0 || boxMax->z() >= 100.0 ||
         boxMin->x() <= -10.0 || boxMin->y() <= -10.0 || boxMin->z() <= -10.0);
}

//// loadCML //////////////////////////////////////////////////////////////////
void AtomSet::loadCML(const QDomElement* root)
/// Reads all atom data from a QDomElement.
{
  clear();
  vector<double> mullikenCharges, stockholderCharges;
  QStringList newForces;

  ///// iterate over all nodes
  QDomNode childNode = root->firstChild();  
  while(!childNode.isNull())
  {
    if(childNode.isElement() && childNode.toElement().tagName() == "atomArray")
    {
      ///// <atomArray> found, iterate over the nodes  
      QDomNode grandChildNode = childNode.firstChild();
      QDomElement atomElement;
      while(!grandChildNode.isNull())
      {
        atomElement = grandChildNode.toElement();
        if(!atomElement.isNull() && atomElement.tagName() == "atom")
        {
          ///// new atom found, read the attributes 
          const unsigned int atomicNumber = atomToNum(atomElement.attribute("elementType", "H"));
          const Point3D<double> xyz3(atomElement.attribute("x3","0.0").toDouble(), 
                                      atomElement.attribute("y3","0.0").toDouble(),
                                      atomElement.attribute("z3","0.0").toDouble());
          ///// iterate over the child <scalar> nodes
          QDomNode scalarNode = atomElement.firstChild();
          QString color = "#000000";
          while(!scalarNode.isNull())
          {
            if(scalarNode.isElement())
            {
              if(scalarNode.toElement().tagName() == "scalar")
              { 
                if(DomUtils::dictEntry(scalarNode, "atom_color"))
                  color = scalarNode.toElement().text();
                else if(scalarNode.toElement().attribute("dictRef") == DomUtils::nsCMLM + ":mulliken" && count() == mullikenCharges.size())
                {
                  // only save the Muliken charges if all previous atoms also have charges
                  double charge = scalarNode.toElement().text().toDouble();
                  mullikenCharges.push_back(charge);                
                }
                else if(scalarNode.toElement().attribute("dictRef") == DomUtils::nsCMLM + ":stockholder" && count() == stockholderCharges.size())
                {
                  double charge = scalarNode.toElement().text().toDouble();
                  stockholderCharges.push_back(charge);
                }
              }
              else if(scalarNode.toElement().tagName() == "vector3" && scalarNode.toElement().attribute("dictRef") == DomUtils::nsCMLM + ":dcart")
                newForces += QStringList::split(" ",scalarNode.toElement().text());
            }
            scalarNode = scalarNode.nextSibling();
          }
          ///// create the new atom
          QColor nColor;
          nColor.setNamedColor(color);
          addAtom(xyz3, atomicNumber, nColor);
        }
        grandChildNode = grandChildNode.nextSibling();
      }
    }
    childNode = childNode.nextSibling();
  }
  
  ///// set the charges
  if(mullikenCharges.size() == count())
    setCharges(mullikenCharges, Mulliken);
  if(stockholderCharges.size() == count())
    setCharges(stockholderCharges, Stockholder);

  ///// set the forces
  if(newForces.size() == 3*count())
  {
    unsigned int index = 0;
    double dx, dy, dz;
    for(QStringList::Iterator it = newForces.begin(); it != newForces.end(); ++it)
    {
      dx = (*it).toDouble();
      dy = (*(++it)).toDouble();
      dz = (*(++it)).toDouble();
      setForces(index++, dx, dy, dz);
    }
  }
}

//// saveCML //////////////////////////////////////////////////////////////////
void AtomSet::saveCML(QDomElement* root)
/// Saves all atom data to a QDomElement. The QDomElement passed as root should
/// be the </molecule> tag.
{
  QDomElement childNode, grandChildNode;
  QDomText textNode;
  
  ///// The list of atoms added as individual entries in an atomArray
  QDomElement atomArray = root->ownerDocument().createElement("atomArray");
  root->appendChild(atomArray);
  for(unsigned int i = 0; i < count(); i++)
  {
    ///// atom
    childNode = root->ownerDocument().createElement("atom");
    childNode.setAttribute("id", QString(numToAtom(atomicNumber(i)).stripWhiteSpace() + QString::number(i + 1)));
    childNode.setAttribute("elementType", numToAtom(atomicNumber(i)).stripWhiteSpace());
    childNode.setAttribute("x3", QString::number(coords[i].x(), 'f', 12));
    childNode.setAttribute("y3", QString::number(coords[i].y(), 'f', 12));
    childNode.setAttribute("z3", QString::number(coords[i].z(), 'f', 12));
    atomArray.appendChild(childNode);
    ///// color
    grandChildNode = root->ownerDocument().createElement("scalar");
    grandChildNode.setAttribute("dictRef", DomUtils::ns + ":atom_color");
    grandChildNode.setAttribute("dataType", DomUtils::nsXSD + ":string");
    childNode.appendChild(grandChildNode);
    textNode = root->ownerDocument().createTextNode(color(i).name());
    grandChildNode.appendChild(textNode);
    ///// charges
    if(chargesMulliken != 0)
    {
      grandChildNode = root->ownerDocument().createElement("scalar");
      //grandChildNode.setAttribute("title", "Mulliken charge");
      grandChildNode.setAttribute("dictRef", DomUtils::nsCMLM + ":mulliken");
      grandChildNode.setAttribute("units", DomUtils::nsAtomic + ":elementary_charge");
      grandChildNode.setAttribute("dataType", DomUtils::nsXSD + ":double");
      childNode.appendChild(grandChildNode);
      textNode = root->ownerDocument().createTextNode(QString::number(chargesMulliken->operator[](i)));
      grandChildNode.appendChild(textNode);
    }
    if(chargesStockholder != 0)
    {
      grandChildNode = root->ownerDocument().createElement("scalar");
      //grandChildNode.setAttribute("title", "stockholder charge");
      grandChildNode.setAttribute("dictRef", DomUtils::nsCMLM + ":stockholder");
      grandChildNode.setAttribute("units", DomUtils::nsAtomic + ":elementary_charge");
      grandChildNode.setAttribute("dataType", DomUtils::nsXSD + ":double");
      childNode.appendChild(grandChildNode);
      textNode = root->ownerDocument().createTextNode(QString::number(chargesStockholder->operator[](i)));
      grandChildNode.appendChild(textNode);
    }
    ///// forces
    if(forces != 0)
    {
      grandChildNode = root->ownerDocument().createElement("vector3");
      grandChildNode.setAttribute("dictRef", DomUtils::nsCMLM + ":dcart");
      grandChildNode.setAttribute("units", DomUtils::nsSI + ":newton");
      grandChildNode.setAttribute("multiplierToSI", "1E-8");
      grandChildNode.setAttribute("dataType", DomUtils::nsXSD + ":double");
      childNode.appendChild(grandChildNode);
      textNode = root->ownerDocument().createTextNode(  QString::number(forces->operator [](i).x(), 'f', 12) + " " 
                                                      + QString::number(forces->operator [](i).y(), 'f', 12) + " " 
                                                      + QString::number(forces->operator [](i).z(), 'f', 12) + " ");
      grandChildNode.appendChild(textNode);      
    }
  }
}

///// atomToNum ///////////////////////////////////////////////////////////////
unsigned int AtomSet::atomToNum(const QString& atom)
/// Returns the atomic number corresponding
/// to the element atom.
{
  QString atom2 = atom.upper().simplifyWhiteSpace();  
  if(atom2 == "H")  return  1;
  if(atom2 == "HE") return  2;
  if(atom2 == "LI") return  3;
  if(atom2 == "BE") return  4;
  if(atom2 == "B")  return  5;
  if(atom2 == "C")  return  6;
  if(atom2 == "N")  return  7;
  if(atom2 == "O")  return  8;
  if(atom2 == "F")  return  9;
  if(atom2 == "NE") return 10;
  if(atom2 == "NA") return 11;
  if(atom2 == "MG") return 12;
  if(atom2 == "AL") return 13;
  if(atom2 == "SI") return 14;
  if(atom2 == "P")  return 15;
  if(atom2 == "S")  return 16;
  if(atom2 == "CL") return 17;
  if(atom2 == "AR") return 18;
  if(atom2 == "K")  return 19;
  if(atom2 == "CA") return 20;
  if(atom2 == "SC") return 21;
  if(atom2 == "TI") return 22;
  if(atom2 == "V")  return 23;
  if(atom2 == "CR") return 24;
  if(atom2 == "MN") return 25;
  if(atom2 == "FE") return 26;
  if(atom2 == "CO") return 27;
  if(atom2 == "NI") return 28;
  if(atom2 == "CU") return 29;
  if(atom2 == "ZN") return 30;
  if(atom2 == "GA") return 31;
  if(atom2 == "GE") return 32;
  if(atom2 == "AS") return 33;
  if(atom2 == "SE") return 34;
  if(atom2 == "BR") return 35;
  if(atom2 == "KR") return 36;
  if(atom2 == "RB") return 37;
  if(atom2 == "SR") return 38;
  if(atom2 == "Y")  return 39;
  if(atom2 == "ZR") return 40;
  if(atom2 == "NB") return 41;
  if(atom2 == "MO") return 42;
  if(atom2 == "TC") return 43;
  if(atom2 == "RU") return 44;
  if(atom2 == "RH") return 45;
  if(atom2 == "PD") return 46;
  if(atom2 == "AG") return 47;
  if(atom2 == "CD") return 48;
  if(atom2 == "IN") return 49;
  if(atom2 == "SN") return 50;
  if(atom2 == "SB") return 51;
  if(atom2 == "TE") return 52;
  if(atom2 == "I")  return 53;
  if(atom2 == "XE") return 54;
  //qDebug("AtomSet::atomToNum: Unknown element " + atom2 + " encountered. Returning 0");
  // any other atom type is returned as number 0, the unknown type "?"
  return 0;
}

///// numToAtom ///////////////////////////////////////////////////////////////
QString AtomSet::numToAtom(const unsigned int atom)
/// Returns the element corresponding to
/// the atomic number atom.
{
  switch(atom)
  {
    case  1: return "H ";
    case  2: return "He";
    case  3: return "Li";
    case  4: return "Be";
    case  5: return "B ";
    case  6: return "C ";
    case  7: return "N ";
    case  8: return "O ";
    case  9: return "F ";
    case 10: return "Ne";
    case 11: return "Na";
    case 12: return "Mg";
    case 13: return "Al";
    case 14: return "Si";
    case 15: return "P ";
    case 16: return "S ";
    case 17: return "Cl";
    case 18: return "Ar";
    case 19: return "K ";
    case 20: return "Ca";
    case 21: return "Sc";
    case 22: return "Ti";
    case 23: return "V ";
    case 24: return "Cr";
    case 25: return "Mn";
    case 26: return "Fe";
    case 27: return "Co";
    case 28: return "Ni";
    case 29: return "Cu";
    case 30: return "Zn";
    case 31: return "Ga";
    case 32: return "Ge";
    case 33: return "As";
    case 34: return "Se";
    case 35: return "Br";
    case 36: return "Kr";
    case 37: return "Rb";
    case 38: return "Sr";
    case 39: return "Y ";
    case 40: return "Zr";
    case 41: return "Nb";
    case 42: return "Mo";
    case 43: return "Tc";
    case 44: return "Ru";
    case 45: return "Rh";
    case 46: return "Pd";
    case 47: return "Ag";
    case 48: return "Cd";
    case 49: return "In";
    case 50: return "Sn";
    case 51: return "Sb";
    case 52: return "Te";
    case 53: return "I ";
    case 54: return "Xe";
  }
  //qDebug("AtomSet::numToAtom: atomic number %d out of range. Returning 'XX'", atom);
  // an unknown atom number is returned as "?"
  return "?";
}

///// vanderWaals /////////////////////////////////////////////////////////////
float AtomSet::vanderWaals(const unsigned int atom)
/// Returns the Van der Waals radius for
/// the atomic number atom.
{
  switch(atom)
  {                         // Accelrys ViewerLite radii
    case  1: return 0.40f;   // 1.10
    case  2: return 0.30f;   // 2.20
    case  3: return 1.25f;   // 1.22
    case  4: return 0.97f;   // 0.63
    case  5: return 0.82f;   // 1.75
    case  6: return 0.75f;   // 1.55
    case  7: return 0.69f;   // 1.40
    case  8: return 0.68f;   // 1.35
    case  9: return 0.64f;   // 1.30
    case 10: return 0.40f;   // 2.02
    case 11: return 1.70f;   // 2.20
    case 12: return 1.60f;   // 1.50
    case 13: return 1.20f;   // 1.50
    case 14: return 1.05f;   // 2.00
    case 15: return 1.00f;   // 1.88
    case 16: return 0.80f;   // 1.81
    case 17: return 1.00f;   // 1.75
    case 18: return 3.00f;   // 2.77
    case 19: return 2.00f;   // 2.39
    case 20: return 0.99f;   // 1.95
    case 21: return 1.44f;   // 1.32
    case 22: return 1.47f;   // 1.95
    case 23: return 1.33f;   // 1.06
    case 24: return 1.35f;   // 1.13
    case 25: return 1.35f;   // 1.19
    case 26: return 1.34f;   // 1.26
    case 27: return 1.33f;   // 1.13
    case 28: return 1.50f;   // 1.24
    case 29: return 1.52f;   // 1.15
    case 30: return 1.45f;   // 1.15
    case 31: return 1.22f;   // 1.55
    case 32: return 1.17f;   // 1.48
    case 33: return 1.21f;   // 0.83
    case 34: return 1.22f;   // 0.90
    case 35: return 1.21f;   // 1.95
    case 36: return 3.00f;   // 1.90
    case 37: return 0.00f;   // 2.65
    case 38: return 0.00f;   // 2.02
    case 39: return 1.78f;   // 1.61
    case 40: return 1.56f;   // 1.42
    case 41: return 1.48f;   // 1.33
    case 42: return 1.47f;   // 1.75
    case 43: return 1.35f;   // 1.80
    case 44: return 1.40f;   // 1.20
    case 45: return 1.45f;   // 1.22
    case 46: return 1.50f;   // 1.44
    case 47: return 1.59f;   // 1.55
    case 48: return 1.69f;   // 1.75
    case 49: return 1.63f;   // 1.46
    case 50: return 1.46f;   // 1.67
    case 51: return 1.46f;   // 1.12
    case 52: return 1.47f;   // 1.26
    case 53: return 1.40f;   // 2.15
    case 54: return 3.00f;   // 2.10
  }
  //qDebug("AtomSet::vanderWaals: atomic number %d out of range. Returning 0.0", atom);
  // the radius of an unknown atom is set to that of helium (the smallest one disregarding
  // the zeroes) This sort of atom does not bond at all.
  return 0.30f; 
}
  
///// stdColor ////////////////////////////////////////////////////////////////
QColor AtomSet::stdColor(const unsigned int atom)
/// Returns the standard color for the atomic
/// number atom.
//  (from Accelrys ViewerLite)
{
  switch(atom)
  {
    case  1: return QColor(255, 255, 255);
    case  2: return QColor(217, 255, 255);
    case  3: return QColor(205, 126, 255);
    case  4: return QColor(197, 255,   0);
    case  5: return QColor(255, 183, 183);
    case  6: return QColor(146, 146, 146);
    case  7: return QColor(143, 143, 255);
    case  8: return QColor(240,   0,   0);
    case  9: return QColor(179, 255, 255);
    case 10: return QColor(175, 227, 244);
    case 11: return QColor(170,  94, 242);
    case 12: return QColor(137, 255,   0);
    case 13: return QColor(210, 165, 165);
    case 14: return QColor(129, 154, 154);
    case 15: return QColor(255, 128,   0);
    case 16: return QColor(255, 200,  50);
    case 17: return QColor( 32, 240,  32);
    case 18: return QColor(129, 209, 228);
    case 19: return QColor(143,  65, 211);
    case 20: return QColor( 61, 255,   0);
    case 21: return QColor(230, 230, 228);
    case 22: return QColor(192, 195, 198);
    case 23: return QColor(167, 165, 172);
    case 24: return QColor(139, 153, 198);
    case 25: return QColor(156, 123, 198);
    case 26: return QColor(129, 123, 198);
    case 27: return QColor(112, 123, 195);
    case 28: return QColor( 93, 123, 195);
    case 29: return QColor(255, 123,  98);
    case 30: return QColor(124, 129, 175);
    case 31: return QColor(195, 146, 145);
    case 32: return QColor(102, 146, 146);
    case 33: return QColor(190, 129, 227);
    case 34: return QColor(255, 162,   0);
    case 35: return QColor(165,  42,  42);
    case 36: return QColor( 93, 186, 209);
    case 37: return QColor(113,  46, 178);
    case 38: return QColor(  0, 255,   0);
    case 39: return QColor(150, 253, 255);
    case 40: return QColor(150, 225, 225);
    case 41: return QColor(116, 195, 203);
    case 42: return QColor( 85, 181, 183);
    case 43: return QColor( 60, 159, 168);
    case 44: return QColor( 35, 142, 151);
    case 45: return QColor( 11, 124, 140);
    case 46: return QColor(  0, 105, 134);
    case 47: return QColor(153, 198, 255);
    case 48: return QColor(255, 216, 145);
    case 49: return QColor(167, 118, 115);
    case 50: return QColor(102, 129, 129);
    case 51: return QColor(159, 101, 181);
    case 52: return QColor(213, 123,   0);
    case 53: return QColor(147,   0, 147);
    case 54: return QColor( 66, 159, 176);
  }
  //qDebug("AtomSet::stdColor: atomic number %d out of range. Returning white", atom);
  // unknown atoms are intense purple so they stand out.
  return QColor(255, 0, 255);
}


///////////////////////////////////////////////////////////////////////////////
///// Private Member Functions                                            /////
///////////////////////////////////////////////////////////////////////////////

///// setChanged //////////////////////////////////////////////////////////////
void AtomSet::setChanged(const bool state)
/// Sets the 'changed' property of the AtomSet.
{
  changed = state;
}

///// setGeometryChanged //////////////////////////////////////////////////////
void AtomSet::setGeometryChanged()
/// Indicates the geometry has changed.
{
  setChanged(); // something has changed

  clearProperties(); // properties like forces and charges do not coincide with the 
                     // structure anymore
  ///// set 'dirty' flags for a number of other things
  bonds1.clear();
  bonds2.clear();
  dirtyBox = true;
}

///// addBondList /////////////////////////////////////////////////////
bool AtomSet::addBondList(const unsigned int callingAtom, const unsigned int startAtom, const unsigned int endAtom1, const unsigned int endAtom2, std::vector<unsigned int>* result)
/// Returns all direct and indirectly bonded atoms to startAtom in result.
/// If any atom of this list (except startAtom) is bonded to endAtom, false is returned (indicates the presence of
/// a ring-structure including the startAtom-endAtom bond. If true is returned the resulting list of bonded atoms
/// is complete
/// -> recursive !
{
  qDebug("entering AtomSet::addBondList");
  ///// exit if callingAtom only has at most 1 bond
  unsigned int numBonds = std::count(bonds1.begin(), bonds1.end(), callingAtom);
  numBonds += std::count(bonds2.begin(), bonds2.end(), callingAtom);
  if(numBonds == 0)
    return false; // if the atom is not bonded, it is to be changed independently
  if(numBonds == 1)
  {
    //qDebug("atom %d has only 1 bond.", callingAtom);
    if(callingAtom == startAtom)
      return false; // if startAtom is only bonded to endAtom, it can be changed indepedently
    else
      return true; // end of list has been reached
  }

  ///// more than 1 bond is present so further traverse the bond list
  vector<unsigned int>::iterator it1 = bonds1.begin();
  vector<unsigned int>::iterator it2 = bonds2.begin();
  while(it1 != bonds1.end() && it2 != bonds2.end())
  {
    if(*it1 == callingAtom)
    {
      // check for ring structures
      if(*it2 == endAtom1 || *it2 == endAtom2)
      {
        if(callingAtom != startAtom)
          return false;
      }
      else if(*it2 != startAtom)
      {
        // check whether the bonded atom is already in the list
        if(std::find(result->begin(), result->end(), *it2) == result->end())
        {
          // add the bond
          result->push_back(*it2);
          //qDebug("adding the bond between atoms %d and %d", callingAtom, *it2);

          // look for indirect bonds
          if(!addBondList(*it2, startAtom, endAtom1, endAtom2, result))
            return false;
        }
      }
    }
    else if(*it2 == callingAtom)
    {
      // check for ring structures
      if(*it1 == endAtom1 || *it1 == endAtom2)
      {
        if(callingAtom != startAtom)
          return false;
      }
      else if(*it1 != startAtom)
      {
        // check whether the bonded atom is already in the list
        if(std::find(result->begin(), result->end(), *it1) == result->end())
        {
          // add the bond
          result->push_back(*it1);
          //qDebug("adding the bond between atoms %d and %d", callingAtom, *it1);

          // look for indirect bonds
          if(!addBondList(*it1, startAtom, endAtom1, endAtom2, result))
            return false;
        }
      }
    }
    it1++;
    it2++;
  }
  return true; // surpresses a warning
}

///// clearProperties /////////////////////////////////////////////////////////
void AtomSet::clearProperties()
/// Deletes all properties. This includes the forces and charges.
{
  if(forces != 0)
  {
    delete forces;
    forces = 0;
  }
  if(chargesMulliken != 0)
  {
    delete chargesMulliken;
    chargesMulliken = 0;
  }
  if(chargesStockholder != 0)
  {
    delete chargesStockholder;
    chargesStockholder = 0;
  }
  chargesMullikenSCF = QString::null;
  chargesMullikenDensity = QString::null;
  chargesStockholderSCF = QString::null;
  chargesStockholderDensity = QString::null;
}

///// updateBoxDimensions /////////////////////////////////////////////////////
void AtomSet::updateBoxDimensions()
/// Calculates the dimensions of the smallest box (in the XYZ coordinate system)
/// including all atoms.
{
  if(!dirtyBox)
    return;
  dirtyBox = false;

  if(numAtoms == 0)
  {
    boxMin->setValues(0.0, 0.0, 0.0);
    boxMax->setValues(0.0, 0.0, 0.0);
  }
  else
  {
    double maxx = coords[0].x();
    double maxy = coords[0].y();
    double maxz = coords[0].z();
    double minx = maxx;
    double miny = maxy;
    double minz = maxz;
    for(unsigned int i = 1; i < numAtoms; i++)
    {
      if(coords[i].x() > maxx)
        maxx = coords[i].x();
      else if(coords[i].x() < minx)
        minx = coords[i].x();
      if(coords[i].y() > maxy)
        maxy = coords[i].y();
      else if(coords[i].y() < miny)
        miny = coords[i].y();
      if(coords[i].z() > maxz)
        maxz = coords[i].z();
      else if(coords[i].z() < minz)
        minz = coords[i].z();
    }
    boxMax->setValues(maxx, maxy, maxz);
    boxMin->setValues(minx, miny, minz);
  }
}

///// addBonds ////////////////////////////////////////////////////////////////
void AtomSet::addBonds(const vector<unsigned int>* atomList1, const vector<unsigned int>* atomList2)
/// Calculates all bonds between the atoms in the 2 provided lists and adds them
/// to the bonds1 and bonds2 vectors
{
  // check whether the second list contains any atoms (the first list is already checked in the
  // bonds function
  if(atomList2->size() == 0)
    return;

  //qDebug("sizes of atomList1 and atomList2: %d and %d", atomList1->size(), atomList2->size());
  float distance2, refdistance, dx, dy, dz;
  unsigned int atomIndex1, atomIndex2, atomNum1;

  ///// do a double loop over the atoms in each list and check whether the sum of their
  ///// Van der Waals radii is less than the distance between them 
  ///// use the squared distances for speed
  unsigned int limit = atomList2->size(); // normal iteration limit for different atom lists
  for(unsigned int i = 0; i < atomList1->size(); i++)
  {
    atomIndex1 = atomList1->operator[](i);
    atomNum1 = coords[atomIndex1].id(); // can never be zero as that has been checked in bonds
    if(atomList1 == atomList2)
      limit = i; // prevents bonds between same atoms or double counting of bonds between identical atom lists
    for(unsigned int j = 0; j < limit; j++)
    {
      atomIndex2 = atomList2->operator[](j);
      dx = static_cast<float>(coords[atomIndex1].x() - coords[atomIndex2].x());
      dy = static_cast<float>(coords[atomIndex1].y() - coords[atomIndex2].y());
      dz = static_cast<float>(coords[atomIndex1].z() - coords[atomIndex2].z());
      distance2 = dx*dx + dy*dy +dz*dz;
      refdistance = 1.25f*(vanderWaals(atomNum1) + vanderWaals(coords[atomIndex2].id()));
      if(distance2 <= refdistance*refdistance)
      {
        bonds1.push_back(atomIndex1);
        bonds2.push_back(atomIndex2);
      }
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
///// Static Variables                                                    /////
///////////////////////////////////////////////////////////////////////////////

const unsigned int AtomSet::maxElements = 54;

