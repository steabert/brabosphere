/***************************************************************************
                       orbitalthread.h  -  description
                             -------------------
    begin                : Sun Mar 27 2005
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
/// Contains the declaration of the class OrbitalThread

#ifndef ORBITALTHREAD_H
#define ORBITALTHREAD_H

///// Forward class declarations & header files ///////////////////////////////

// STL header files
#include <vector>

// Qt forward class declarations
class QMutex;
class QWidget;

// Xbrabo header files
#include "point3d.h"

// Base class header files
#include <qthread.h>

///// class OrbitalThread /////////////////////////////////////////////////////
class OrbitalThread : public QThread
{
  public:
    ///// constructor/destructor
    OrbitalThread(QWidget* parentWidget, QMutex* sharedMutex, std::vector<Point3D<float> >* coordinates, const unsigned int type, const unsigned int atom, const unsigned int n, const unsigned int l, const int m, const float res, const float prob, const unsigned int dots);        // constructor
    ~OrbitalThread();                   // destructor

    ///// public enums
    enum Types{IsoProbability = 0, Density, AngularPart, AccumulatedProbability, RadialPart}; ///< Holds the types of calculations available

    ///// public member functions
    void stop();                        // requests stopping the thread
    double boundingSphereRadius();      // returns the radius of the bounding sphere

  private:
    // private enums
    enum Precision{PRECISION_UNKNOWN, PRECISION_FLOAT, PRECISION_DOUBLE, PRECISION_LONG_DOUBLE}; ///< The needed precision for a certain set of quantum numbers

    ///// private member functions
    virtual void run();                 // reimplementation of this pure virtual does the actual work
    void calcIsoProbability();          // calculates an isoprobability
    void calcAccumulatedProbability();  // calculates points with the given total accumulated probability
    void calcRandomDots();              // calculates random points according to the probability
    void calcRadialPart();              // calculates only the radial part of the orbital
    void calcAngularPart();             // calculates only the angular part of the orbital
    void updateList(std::vector<Point3D<float> >& newCoords, bool final = false);         // updates the shared list of coordinates with a new set
    float associatedLegendre(const float x, const int m, const unsigned int l); // returns the associated Legendre polynomial
    float associatedLaguerre(const float x, const int m, const unsigned int n); // returns the associated Laguerre polynomial
    template <class T> T factorial(const unsigned int number);        // returns the factorial (n!) of the number (n)
    float random(const float min, const float max);         // returns a random number between min and max
    template <class T> T largestResult(const unsigned int n, const unsigned int l, const int m, const unsigned int Z);  // calculates the largest possible result for the given quantum numbers

    ///// private member data
    QWidget* receiver;                  ///< The widget that receives any sent events.
    QMutex* mutex;                      ///< A mutex to lock the access to the vector of calculated points.
    std::vector<Point3D<float> >* coords;         ///< Coordinates of calculated probability points.
    unsigned int atomNumber;            ///< The atom type for which the orbital is to be shown.
    unsigned int qnPrincipal;           ///< Principal quantum number (n) (1 - x).
    unsigned int qnOrbital;             ///< Orbital quantum number (l) (0 - n-1).
    int qnMomentum;                     ///< Angular momentum quantum number (m) (-l - +l).
    unsigned int calculationType;       ///< The type of calculation to be done.
    float probability;                  ///< The iso or accumulated probability.
    float resolution;                   ///< The desired resolution.
    unsigned int numDots;               ///< The number of dots to calculate for random dots.
    bool stopRequested;                 ///< Is set to true if the thread should be stopped.
    float maximumRadius;                ///< Holds the maximum encountered value of r during the calculation.
    unsigned int progress;              ///< Holds the progress of the calculation.

    // private static constants
    static const float abohr;           ///< The Bohr radius.
    static const unsigned int updateSize;         // The amount of dots that have to be calculated before the list of dots is updated and the mutex is locked
};

#endif

