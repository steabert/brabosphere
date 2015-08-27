/***************************************************************************
                       glorbitalview.cpp  -  description
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

///// Comments ////////////////////////////////////////////////////////////////
/*!
  \class GLOrbitalView
  \brief This class shows an orbital in 3D using OpenGL.

  It uses a thread to do the necessary computations and is based on GLView for
  basic OpenGL functionality.

*/
/// \file
/// Contains the implementation of the class GLOrbitalView.

///// Header files ////////////////////////////////////////////////////////////

// C++ header files
#include <cfloat>
#include <cmath>

// Qt header files
#include <qapplication.h>
#include <qcolor.h>
#include <qfiledialog.h>
#include <qimage.h>
#include <qmessagebox.h>
#include <qprogressdialog.h>
#include <qstringlist.h>
#include <qtimer.h>

// Xbrabo header files
#include "glorbitalview.h"
#include <point3d.h>
#include <quaternion.h>

///////////////////////////////////////////////////////////////////////////////
///// Public Member Functions                                             /////
///////////////////////////////////////////////////////////////////////////////

///// constructor /////////////////////////////////////////////////////////////
GLOrbitalView::GLOrbitalView(QWidget* parent, const char* name) : GLView(parent, name),
  colorPositive(QColor(0, 0, 255)),
  colorNegative(QColor(255, 0, 0)),
  maximumRadius(1.0f),
  scaleFactor(1.0f)
/// The default constructor.
{

}

///// destructor //////////////////////////////////////////////////////////////
GLOrbitalView::~GLOrbitalView()
/// The default destructor.
{
  
}

///// updateColors ////////////////////////////////////////////////////////////
void GLOrbitalView::updateColors(QColor pos, QColor neg)
/// Updates the colors of the orbitals' phases.
{
  colorPositive = pos;
  colorNegative = neg;
}

///// getCoordinates //////////////////////////////////////////////////////////
std::vector<Point3D<float> >* GLOrbitalView::getCoordinates()
/// Returns a pointer to the coords vector.
{
  return &coords;
}

///// getMutex ////////////////////////////////////////////////////////////////
QMutex* GLOrbitalView::getMutex()
/// Returns a pointer to the mutex used for accessing the coords vector.
{
  return &mutex;
}

///// setMaximumRadius ////////////////////////////////////////////////////////
void GLOrbitalView::setMaximumRadius(const double radius)
/// Sets the maximum radius of the coordinates.
/// Don't calculate it here from the coordinates because it can be a lot of
/// points, now it's calculated in a separate thread and this thread uses
/// polar coordinates.
{
  maximumRadius = radius;
  scaleFactor = 10.0f/maximumRadius; // scale the radius of the bounding sphere to 10.0f  
  zoomFit(false); // doesn't work correctly if called with (true), possibly calls updateGL() of base class
  updateGL(); 
}

/*//// updateValues ////////////////////////////////////////////////////////////
void GLOrbitalView::updateValues(int atom, int n, int l, int m, QColor pos, QColor neg, int type, int resolution, float probability, int dots)
{
  ///// Public member function. Updates the variables defining the type of orbital
  
  //qDebug("updating GLorbitalView with atom = %d, n = %d, l = %d, m = %d, type = %d, res = %d, prob = %f, dots = %d", atom, n, l, m, type, resolution, probability, dots);
  atomNumber = static_cast<unsigned int>(atom);
  qnPrincipal = static_cast<unsigned int>(n);
  qnOrbital = static_cast<unsigned int>(l);
  qnMomentum = static_cast<int>(m);
  ASSERT(qnOrbital - abs(qnMomentum) >= 0);
  ASSERT(qnPrincipal > qnOrbital);
  
  colorPositive = pos;
  colorNegative = neg;

  ASSERT(probability > 0.0f);
  ASSERT(type >= 0 && type < 5);

  ////// determine the needed precision (float, double or long double
  unsigned int neededPrecision = PRECISION_UNKNOWN;
  if(type != 4)
  {
    ///// PRECISION CHECKS FOR ALL BUT GETANGULARPART
    // check whether the largest possible number ((4n)^n) can be represented by a float
    if(largestResult<float>(qnPrincipal, qnOrbital, qnMomentum, atomNumber) != HUGE_VAL)
    //if(powf(static_cast<float>(4 * atomNumber * qnPrincipal), qnPrincipal) != HUGE_VALF)
      neededPrecision = PRECISION_FLOAT;
    // by a double
    else if(pow(static_cast<double>(4 * atomNumber * qnPrincipal), qnPrincipal) != HUGE_VAL)
      neededPrecision = PRECISION_DOUBLE;
    // or by a long double
    else if(powl(static_cast<long double>(4 * atomNumber * qnPrincipal), qnPrincipal) != HUGE_VAL)
      neededPrecision = PRECISION_LONG_DOUBLE;
  }
  else
  {     
    ///// PRECISION CHECKS FOR ANGULAR PART
    // check whether the largest possible number ((2l)!) can be represented by a float
    if(factorial<float>(2*qnOrbital) != HUGE_VAL)
      neededPrecision = PRECISION_FLOAT;
    // by a double
    else if(factorial<float>(2*qnOrbital) != HUGE_VAL)
      neededPrecision = PRECISION_DOUBLE;
    // or by a long double
    else if(factorial<float>(2*l) != HUGE_VAL)
      neededPrecision = PRECISION_LONG_DOUBLE;
  }
  qDebug("required precision = %d", neededPrecision);
  if(neededPrecision == PRECISION_UNKNOWN)
  {
    // QMessageBox
    qDebug("exceeded long double limits");
    return;
  }
  if(neededPrecision != PRECISION_FLOAT)
  {
    qDebug("precision higher than float not implemented yet");
    return;
  }

  allowedToQuit = false;
  cancelRequested = false;
  qDebug("starting calc");    

  switch(type)
  {
    case 0: getIsoProbability(static_cast<float>(resolution), probability);
            break;
    case 1: getAccumulatedProbability(static_cast<float>(resolution), probability);
            break;
    case 2: getRandomDots(static_cast<unsigned int>(dots));
            break;
    case 3: getRadialPart(static_cast<float>(resolution));
            break;
    case 4: getAngularPart(static_cast<float>(resolution));
  }
  allowedToQuit = true;
  qDebug("maximum radius = %f (= %f n^2)", maximumRadius, maximumRadius/(n*n));
  if(maximumRadius < 0.1f) maximumRadius = 0.1f;
  scaleFactor = 10.0f/maximumRadius; // scale the radius of the bounding sphere to 10.0f  
  zoomFit(false); // doesn't work correctly if called with (true), possibly calls updateGL() of base class
  updateGL();  
}
*/


///////////////////////////////////////////////////////////////////////////////
///// Protected Member Functions                                          /////
///////////////////////////////////////////////////////////////////////////////

///// drawScene ///////////////////////////////////////////////////////////////
void GLOrbitalView::drawScene()
/// Implementation of the pure virtual GLView::drawScene().
/// Does the drawing of the OpenGL scene.
{
  // scale if the boundaries exceed 100.0f (the far z-value)
  glScalef(scaleFactor, scaleFactor, scaleFactor);
  
  //*  
  ///// draw the precalculated isoprobability points
  glDisable(GL_LIGHTING);

  glBegin(GL_POINTS);
    mutex.lock();
    std::vector<Point3D<float> >::iterator it = coords.begin();
    while(it != coords.end())
    {
      if(it->id() == 1)
        qglColor(colorPositive);
      else
        qglColor(colorNegative);
      glVertex3f(it->x(), it->y(), it->z());
      it++;
    }
    mutex.unlock();
  glEnd();
  // no use in making a display list as the coordinates might be updated at any time

  /*// lines
  glBegin(GL_LINE_LOOP);

  // loop over the indices
  const unsigned int maxPoints = qnOrbital == 0 ? 2*qnPrincipal -1 : 2*(qnPrincipal - qnOrbital);
  const unsigned int resolution = 20;
  
  for(unsigned int i = 1; i <= maxPoints; i++)
  {
    // get 360/resolution phi's and draw a looped lines around them
    std::vector<float>::iterator itX = coordsX.begin();
    std::vector<float>::iterator itY = coordsY.begin();
    std::vector<float>::iterator itZ = coordsZ.begin();
    std::vector<bool>::iterator itP = phase.begin();
    std::vector<unsigned int>::iterator itI = index.begin();

    while(itX != coordsX.end())
    {
      glBegin(GL_LINE_LOOP);
      unsigned int numPoints = 0;
      while(numPoints < resolution && itX != coordsX.end())
      {
        if(*itI == i)
        {
          numPoints++;
          if(*itP)
            qglColor(QColor(255, 255, 0));
          else
            qglColor(QColor(0, 255, 255));
          glVertex3f(*itX, *itY, *itZ);
        }
        itX++;
        itY++;
        itZ++;
        itP++;        
        itI++;                
      }
      glEnd();
    }    
  }
  // other lines (180/resolution theta)
  for(unsigned int i = 1; i <= maxPoints; i++)
  {
    // fill some vectors with the data for the current point
    std::vector<float> lcoordsX, lcoordsY, lcoordsZ;
    std::vector<bool> lphase;
            
    std::vector<float>::iterator itX = coordsX.begin();
    std::vector<float>::iterator itY = coordsY.begin();
    std::vector<float>::iterator itZ = coordsZ.begin();
    std::vector<bool>::iterator itP = phase.begin();
    std::vector<unsigned int>::iterator itI = index.begin();
    while(itI != index.end())
    {
      if(*itI == i)
      {
        lcoordsX.push_back(*itX);
        lcoordsY.push_back(*itY);
        lcoordsZ.push_back(*itZ);
        lphase.push_back(*itP);        
      }
      itX++;
      itY++;
      itZ++;
      itP++;
      itI++;    
    }

    std::vector<float>::iterator itlX = lcoordsX.begin();
    std::vector<float>::iterator itlY = lcoordsY.begin();
    std::vector<float>::iterator itlZ = lcoordsZ.begin();
    std::vector<bool>::iterator itlP = lphase.begin();
    
    for(unsigned int startPoint = 0; startPoint < resolution; startPoint++)
    {
      // start at each point and draw the line
      std::vector<float>::iterator itlX2 = itlX;
      std::vector<float>::iterator itlY2 = itlY;
      std::vector<float>::iterator itlZ2 = itlZ;
      std::vector<bool>::iterator itlP2 = itlP;
      unsigned int numPoints = 0;
      glBegin(GL_LINE_LOOP);
      while(itlX2 != lcoordsX.end())
      {
        numPoints++;
        if(numPoints%resolution == 0)
        {
          if(*itlP2)
            qglColor(QColor(255, 255, 0));
          else
            qglColor(QColor(0, 255, 255));
          
          glVertex3f(*itlX2, *itlY2, *itlZ2);
        }
        itlX2++;
        itlY2++;
        itlZ2++;
        itlP2++;        
      }
      glEnd();
      itlX++;
      itlY++;
      itlZ++;
      itlP++;
      if(itlX == lcoordsX.end()) break;      
    }   
  }
  */      
}

///// boundingSphereRadius ////////////////////////////////////////////////////
float GLOrbitalView::boundingSphereRadius()
/// Implementation of the pure virtual GLView::boundingSphereRadius().
{
  return static_cast<float>(maximumRadius*scaleFactor); 
}

