/***************************************************************************
                        glorbitalview.h  -  description
                             -------------------
    begin                : Thu Nov 4 2004
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
/// Contains the declaration of the class GLOrbitalView.

#ifndef GLORBITALVIEW_H
#define GLORBITALVIEW_H

///// Forward class declarations & header files ///////////////////////////////

// C++ forward class declarations
#include <vector>

// Qt forward class declarations
class QColor;
#include <qmutex.h>

// Xbrabo header files
#include "point3d.h"

// Base class header file
#include <glview.h>

///// class GLOrbitalView /////////////////////////////////////////////////////
class GLOrbitalView : public GLView
{
  Q_OBJECT
  
  public:
    // constructor/destructor
    GLOrbitalView(QWidget* parent = 0, const char* name = 0);         // constructor
    ~GLOrbitalView();                   // destructor

    // public member functions
    void updateColors(QColor pos, QColor neg);
    std::vector<Point3D<float> >* getCoordinates();         // returns a pointer to the coords vector
    QMutex* getMutex();                   // returns a pointer to the mutex used for accessing the coords vector
    void setMaximumRadius(const double radius);   // updates the maximum radius of the coordinates
        
  protected:
    void drawScene();                   // does the local drawing of the scene
    float boundingSphereRadius();       // calculates the radius of the bounding sphere
    
  private:
    // private enums
    //enum Precision{PRECISION_UNKNOWN, PRECISION_FLOAT, PRECISION_DOUBLE, PRECISION_LONG_DOUBLE};
    
    // private member variables
    QColor colorPositive;               ///< The color of positive values.
    QColor colorNegative;               ///< The color of negative values.
    std::vector<Point3D<float> > coords;          ///< The coordinates including phases.
    QMutex mutex;                       ///< The mutex for accessing the coords vector.
    float maximumRadius;                ///< The overall maximum radius (used by boundingSphereRadius).
    float scaleFactor;                  ///< A scaling factor used when maximumRadius exceeds the far z-value (100.0f).
};

#endif

