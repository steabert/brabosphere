/***************************************************************************
                       orbitalthread.cpp  -  description
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
///// Comments ////////////////////////////////////////////////////////////////
/*!
  \class OrbitalThread
  \brief This class calculates Hydrogen orbitals for visualisation in 3D in a thread.

  It uses increased precision (double, long double) only when needed. They are, however, not implemented
  yet so the limit for 'n' (the main quantum number) lies somewhere around 15 
  depending on the values of the other quantum numbers.

*/
/// \file
/// Contains the implementation of the class OrbitalThread

///// Header files ////////////////////////////////////////////////////////////

// C++ header files
#include <cassert>
#include <cfloat>
#include <cmath>

// Qt header files
#include <qapplication.h>
#include <qmutex.h>

// Xbrabo header files
#include "orbitalthread.h"

///////////////////////////////////////////////////////////////////////////////
///// Public Member Functions                                             /////
///////////////////////////////////////////////////////////////////////////////

///// Constructor /////////////////////////////////////////////////////////////
OrbitalThread::OrbitalThread(QWidget* parentWidget, QMutex* sharedMutex, std::vector<Point3D<float> >* coordinates, 
                             const unsigned int type, const unsigned int atom, const unsigned int n, 
                             const unsigned int l, const int m, const float res, const float prob, 
                             const unsigned int dots) : QThread(),
  receiver(parentWidget),
  mutex(sharedMutex),
  coords(coordinates),
  atomNumber(atom),
  qnPrincipal(n),
  qnOrbital(l),
  qnMomentum(m),
  calculationType(type), 
  probability(prob),
  resolution(res),        
  numDots(dots),
  stopRequested(false)
/// The default constructor. All needed parameters are passed upon creation of the thread as it is one-shot.
/// Using multiple threads is easily accomplished by further providing start and end values for the angle Phi.
{
  assert(parentWidget != 0);
  assert(sharedMutex != 0);
  assert(coordinates != 0);
  assert(atom > 0);
  assert(prob >= 0.0f && prob <= 1.0f);
  assert(res > 1.0f);
  assert(n > l);
  assert(l >= static_cast<unsigned int>(abs(m))); // VC++ warns without the cast
  assert(type < 3); // < 5 if AccumulatedProbability and RadialPart work as required

  // get some statistics
  //qDebug("maximum float = %e (%d bits)", FLT_MAX, sizeof(float));
  //qDebug("maximum double = %e (%d bits)", DBL_MAX, sizeof(double));
  //qDebug("maximum long double = %Le (%d bits)", LDBL_MAX, sizeof(long double));

  ///// comment: maybe template-ize the getXXX functions and find the lowest
  ///// precision that can represent (4Zn)^n (as occurs in associatedLaguerre for r=2n^2)
  ///// => not needed for getAngularPart => largest number is
}

///// Destructor //////////////////////////////////////////////////////////////
OrbitalThread::~OrbitalThread()
/// The default destructor.
{

}

///// stop ////////////////////////////////////////////////////////////////////
void OrbitalThread::stop()
/// Requests the thread to stop.
{
  stopRequested = true;
}

///// boudingSphereRadius /////////////////////////////////////////////////////
double OrbitalThread::boundingSphereRadius()
/// Returns the radius of the sphere encompassing the calculated orbital.
{
  return static_cast<double>(maximumRadius);
}

///////////////////////////////////////////////////////////////////////////////
///// Private Member Functions                                            /////
///////////////////////////////////////////////////////////////////////////////

///// run /////////////////////////////////////////////////////////////////////
void OrbitalThread::run()
/// Dispatches the work to the proper calculating
/// routine. It is run with a call to start().
{  
  ////// determine the needed precision (float, double or long double
  unsigned int neededPrecision = PRECISION_UNKNOWN;
  if(calculationType != AngularPart)
  {
    ///// PRECISION CHECKS FOR ALL BUT GETANGULARPART
    // check whether the largest possible number ((4n)^n) can be represented by a float
    if(largestResult<float>(qnPrincipal, qnOrbital, qnMomentum, atomNumber) != HUGE_VAL)
    //if(powf(static_cast<float>(4 * atomNumber * qnPrincipal), qnPrincipal) != HUGE_VALF)
      neededPrecision = PRECISION_FLOAT;
    // by a double
    else if(largestResult<double>(qnPrincipal, qnOrbital, qnMomentum, atomNumber) != HUGE_VAL)
    //else if(pow(static_cast<double>(4 * atomNumber * qnPrincipal), qnPrincipal) != HUGE_VAL)
      neededPrecision = PRECISION_DOUBLE;
    // or by a long double
    else if(largestResult<long double>(qnPrincipal, qnOrbital, qnMomentum, atomNumber) != HUGE_VAL)
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
    else if(factorial<float>(2*qnOrbital) != HUGE_VAL)
      neededPrecision = PRECISION_LONG_DOUBLE;
  }
  qDebug("required precision = %d", neededPrecision);
  if(neededPrecision == PRECISION_UNKNOWN)
  {
  // notify the thread has ended
  QCustomEvent* e = new QCustomEvent(static_cast<QEvent::Type>(1002));
  QApplication::postEvent(receiver, e);
    qDebug("exceeded long double limits");
    return;
  }
  if(neededPrecision != PRECISION_FLOAT)
  {
  // notify the thread has ended
  QCustomEvent* e = new QCustomEvent(static_cast<QEvent::Type>(1002));
  QApplication::postEvent(receiver, e);
    qDebug("precision higher than float not implemented yet");
    return;
  }

  switch(calculationType)
  {
    case IsoProbability: 
            calcIsoProbability();
            break;
    case AccumulatedProbability: 
            calcAccumulatedProbability();
            break;
    case Density: 
            calcRandomDots();
            break;
    case RadialPart: 
            calcRadialPart();
            break;
    case AngularPart: 
            calcAngularPart();
            break;
  }
  // notify the thread has ended
  QCustomEvent* e = new QCustomEvent(static_cast<QEvent::Type>(1002));
  QApplication::postEvent(receiver, e);
}

///// calcIsoProbability //////////////////////////////////////////////////////
void OrbitalThread::calcIsoProbability()
/// Calculates the points having the given probability
/// ( |psi|^2 ). (between 0.0 and 1.0 by definition)
{
  const int updateFreq = static_cast<int>(resolution*resolution)/100;

  // clear the data
  mutex->lock();
  coords->clear();
  mutex->unlock();
  maximumRadius = 0.0f;
  std::vector<Point3D<float> > coordsList;
  coordsList.reserve(updateSize);

  // a short local form of the quantum numbers
  const unsigned int n = qnPrincipal;
  const unsigned int l = qnOrbital;
  const int m = qnMomentum;

  // values independent of r, theta or phi
  const float zna = static_cast<float>(atomNumber) / static_cast<float>(n) / abohr; // conversion factor for r->rho
  const float normR = pow(2.0f*zna, 1.5f) * sqrt(factorial<float>(n-l-1)/2.0f/n/factorial<float>(n+l)); // normalization factor for the radial part
  const float normTheta = pow(-1.0f, (m + abs(m))/2) * sqrtf( (2*l+1) * factorial<float>(l-abs(m)) / (2.0f * factorial<float>(l+abs(m)))); // normalization factor for the angular part (Theta dependent)
  const float normPhi = 1.0f / sqrtf(Point3D<float>::PI); // normalization factor for the angular part (Phi dependent)
  const float incTheta = 180.0f/resolution; // increment for theta
  const float incPhi = 360.0f/resolution; // increment for phi
  const float maxR = 2.0f*static_cast<float>(n*n); // maximum value for r to check
  const float incR = maxR / (10.0f * resolution); // increment for r

  ///// loop over THETA ///////////////
  for(float theta = 0.0f; theta < 180.0f; theta += incTheta) // rotation away from z-axis: only 180 degrees
  {
    ///// loop over PHI ///////////////
    for(float phi = 0.0f; phi < 360.0f; phi += incPhi) // rotation around z-axis: full 360 degrees
    {
      // notify the progress
      progress = static_cast<int>(theta/incTheta * resolution + phi/incPhi);
      if(progress % updateFreq == 0)
      {
        QCustomEvent* e = new QCustomEvent(static_cast<QEvent::Type>(1001),&progress);
        QApplication::postEvent(receiver, e);
      }
                        
      // randomize theta within its square
      const float randTheta = theta + random(-incTheta/2.0f, incTheta/2.0f);
      // determine whether this point is to be drawn (|sin(theta)| function is maximum near the equator and zero at the poles)
      if(random(0.0f, 1.0f) > fabs(sinf(randTheta*Point3D<float>::DEGTORAD)))
        continue;
      // randomize phi within its square
      const float randPhi = phi + random(-incPhi/2.0f, incPhi/2.0f);
        
      // get the result for theta
      const float partTheta = normTheta * associatedLegendre(cosf(randTheta*Point3D<float>::DEGTORAD), abs(m), l);
      // get the result for phi
      float partPhi = normPhi;
      if(m == 0)
        partPhi /= sqrtf(2.0f);
      else if(m > 0)
       partPhi *= sinf(m*randPhi*Point3D<float>::DEGTORAD);
      else
       partPhi *= cosf(m*randPhi*Point3D<float>::DEGTORAD);

      // total angular result
      const float partY = partTheta * partPhi;

      ///// loop over R ///////////////
      float r = incR;
      float newProbability = 0.0f;
      const unsigned int maxPoints = l == 0 ? 2*n -1 : 2*(n - l); // the maximum number of times a value can be the given probability
      unsigned int numPoints = 0;
      // determine a starting oldProbability (for r = 0.0f)
      float oldProbability = pow(partY * normR * associatedLaguerre(0.0f, 2*l+1, n-l-1) , 2);
      // start the loop
      while(r < maxR && numPoints < maxPoints)
      {
        if(stopRequested)
          return;
        const float rho = 2.0f * zna * r;
        const float partR = normR * pow(rho, static_cast<int>(l)) * exp(-rho/2.0f) * associatedLaguerre(rho, 2*l+1, n-l-1);

        newProbability = partY * partR * partY * partR;
        //totalProbability += 4.0f * Point3D<float>::PI * newProbability * r*r / resolution;

        if(r > incR && ((oldProbability <= probability && newProbability >= probability) || (oldProbability >= probability && newProbability <= probability)))
        {
            // found a point
            const float minProb = oldProbability < newProbability ? oldProbability : newProbability;
            const float maxProb = oldProbability > newProbability ? oldProbability : newProbability;
            const float interpolatedR = r + incR * (probability - minProb)/(maxProb-minProb);

            //if(theta <= 180.0f/resolution && phi <= 360.0f/resolution)
            //  qDebug("point found at r = %f, interpolatedR = %f", r, interpolatedR);

            numPoints++;
            Point3D<float> newCoord;
            newCoord.setPolar(randTheta, randPhi, interpolatedR);
            newCoord.setID(partY*partR > 0.0f ? 1 : 0); // misuse Point3D's ID to store the phase (1 = pos, 0 = neg)
            coordsList.push_back(newCoord);
            updateList(coordsList);
            if(r > maximumRadius) maximumRadius = r;
        }
        oldProbability = newProbability;
        r += incR;
      }
      
      if(numPoints < maxPoints && oldProbability > probability)
      {
        //if(theta < 180.0f/resolution && phi < 360.0f/resolution)
        //  qDebug("trying remaining space");

        // the remaining cloud still has at least one point with the given probability
        while(true)
        {
          if(stopRequested)
            return;
          const float rho = 2.0f * zna * r;
          const float partR = normR * pow(rho, static_cast<int>(l)) * exp(-rho/2.0f) * associatedLaguerre(rho, 2*l+1, n-l-1);
          newProbability = partY * partR * partY * partR;
          if(oldProbability >= probability && newProbability <= probability)
          {
            const float interpolatedR = r + incR * (probability - newProbability)/(oldProbability-newProbability);

            //if(theta <= 180.0f/resolution && phi <= 360.0f/resolution)
            //  qDebug("extra point found at r = %f, interpolatedR = %f", r, interpolatedR);

            numPoints++;
            Point3D<float> newCoord;
            newCoord.setPolar(randTheta, randPhi, interpolatedR);
            newCoord.setID(partY*partR > 0.0f ? 1 : 0); 
            //mutex->lock();
            //coords->push_back(newCoord);
            //mutex->unlock();
            coordsList.push_back(newCoord);
            updateList(coordsList);
            if(r > maximumRadius) maximumRadius = r;
            break;
          }
          oldProbability = newProbability;
          r += incR;
        }
      }
    }    
  }
  updateList(coordsList, true);
}

///// calcAccumulatedProbability //////////////////////////////////////////////
void OrbitalThread::calcAccumulatedProbability()
/// Calculates the points having the given accumulated probability
/// ( |psi|^2 ). (between 0.0 and 1.0 by definition);
{
  const int updateFreq = static_cast<int>(resolution*resolution)/100;

  // clear the data
  mutex->lock();
  coords->clear();
  mutex->unlock();
  maximumRadius = 0.0f;
  std::vector<Point3D<float> > coordsList;
  coordsList.reserve(updateSize);

  // a short local form of the quantum numbers
  const unsigned int n = qnPrincipal;
  const unsigned int l = qnOrbital;
  const int m = qnMomentum;
  
  // values independent of r, theta or phi
  const float zna = static_cast<float>(atomNumber) / static_cast<float>(n) / abohr; // conversion factor for r->rho
  const float normR = pow(2.0f*zna, 1.5f) * sqrt(factorial<float>(n-l-1)/2.0f/n/factorial<float>(n+l)); // normalization factor for the radial part
  const float normTheta = pow(-1.0f, (m + abs(m))/2) * sqrtf( (2*l+1) * factorial<float>(l-abs(m)) / (2.0f * factorial<float>(l+abs(m)))); // normalization factor for the angular part (Theta dependent)
  const float normPhi = 1.0f / sqrtf(Point3D<float>::PI); // normalization factor for the angular part (Phi dependent)
  const float incTheta = 180.0f/resolution; // increment for theta
  const float incPhi = 360.0f/resolution; // increment for phi
  const float maxR = 2.0f*static_cast<float>(n*n); // maximum value for r to check
  const float incR = maxR / (10.0f * resolution); // increment for r
  
  ///// loop over THETA ///////////////
  for(float theta = 0.0f; theta < 180.0f; theta += incTheta) // rotation away from z-axis: only 180 degrees
  {

    ///// loop over PHI ///////////////
    for(float phi = 0.0f; phi < 360.0f; phi += incPhi) // rotation around z-axis: full 360 degrees
    {
      // notify the progress
      progress = static_cast<int>(theta/incTheta * resolution + phi/incPhi);
      if(progress % updateFreq == 0)
      {
        QCustomEvent* e = new QCustomEvent(static_cast<QEvent::Type>(1001),&progress);
        QApplication::postEvent(receiver, e);
      }

      // randomize theta within its square
      const float randTheta = theta + random(-incTheta/2.0f, incTheta/2.0f);
      // determine whether this point is to be drawn (|sin(theta)| function is maximum near the equator and zero at the poles)
      if(random(0.0f, 1.0f) > fabs(sinf(randTheta*Point3D<float>::DEGTORAD)))
        continue;
      
      // get the result for theta
      const float partTheta = normTheta * associatedLegendre(cosf(randTheta*Point3D<float>::DEGTORAD), abs(m), l);

      // randomize phi within its square
      const float randPhi = phi + random(-incPhi/2.0f, incPhi/2.0f);
      // get the result for phi
      float partPhi = normPhi;
      if(m == 0)
        partPhi /= sqrtf(2.0f);
      else if(m > 0)
       partPhi *= sinf(m*randPhi*Point3D<float>::DEGTORAD);
      else
       partPhi *= cosf(m*randPhi*Point3D<float>::DEGTORAD);

      // total angular result
      const float partY = partTheta * partPhi;

      ///// loop over R ///////////////
      float r = 0.0f;
      float totalProbability = 0.0f;
      float amplitude = 0.0f;
      while (r < maxR)
      {
        if(stopRequested)
          return;
        const float rho = 2.0f * zna * r;
        const float partR = normR * pow(rho, static_cast<int>(l)) * exp(-rho/2.0f) * associatedLaguerre(rho, 2*l+1, n-l-1);

        // total amplitude
        amplitude = partY * partR;
        totalProbability += 4.0f * Point3D<float>::PI * amplitude*amplitude * r*r * incR;

        //qDebug("r = %f, amplitude = %f, totalProbability = %20.15f", r, amplitude, totalProbability);
        if(totalProbability >= probability)
          break;

        r += incR;
      }
      //break;
      // now r is the required radius
      //qDebug("getAccumProb: phi = %f, theta = %f, distance = %f", theta, phi, partY);
      Point3D<float> newCoord;
      newCoord.setPolar(randTheta, randPhi, r);
      newCoord.setID(amplitude > 0.0f ? 1 : 0); 
      coordsList.push_back(newCoord);
      updateList(coordsList);
      if(r > maximumRadius) maximumRadius = r;
    }
  }
  updateList(coordsList, true);
}

///// calcRandomDots //////////////////////////////////////////////////////////
void OrbitalThread::calcRandomDots()
/// Calculates random points according to the probability at a random point.
{
  const int updateFreq = static_cast<int>(numDots)/100;

  // clear the data
  mutex->lock();
  coords->clear();
  mutex->unlock();
  maximumRadius = 0.0f;
  std::vector<Point3D<float> > coordsList;
  coordsList.reserve(updateSize);

  // a short local form of the quantum numbers
  const unsigned int n = qnPrincipal;
  const unsigned int l = qnOrbital;
  const int m = qnMomentum;

  // values independent of r, theta or phi
  const float zna = static_cast<float>(atomNumber) / static_cast<float>(n) / abohr; // conversion factor for r->rho
  const float normR = pow(2.0f*zna, 1.5f) * sqrt(factorial<float>(n-l-1)/2.0f/n/factorial<float>(n+l)); // normalization factor for the radial part
  const float normTheta = pow(-1.0f, (m + abs(m))/2) * sqrtf( (2*l+1) * factorial<float>(l-abs(m)) / (2.0f * factorial<float>(l+abs(m)))); // normalization factor for the angular part (Theta dependent)
  const float normPhi = 1.0f / sqrtf(Point3D<float>::PI); // normalization factor for the angular part (Phi dependent)
  const float maxR = 2.0f * static_cast<float>(n*n); // n*n is maximum radial probability for ns => this is the maximum radius for drawing points
  float maxRtest = 0.0f; // => this is the maximum radius needed for the maximum probability test (0 if l == 0)
  if(l == n-1)
    maxRtest = static_cast<float>(n*l); // correct (= l*(l+1))
  else if(l != 0)
    maxRtest = static_cast<float>(l*(l+1)); // maximum (higher n is smaller maxRtest)

  // run for 1/10th of the amount (1000 points max) to get an estimate on the maximum probability
  float maxProb = 0.0f;
  float maxProbR = 0.0f;
  unsigned int testLimit = numDots/10 < 1000 ? numDots/10 : 1000;
  for(unsigned int i = 0; i < testLimit; i++)
  {
    if(stopRequested)
      return;
    const float theta = random(0.0f, 180.0f);
    const float phi = random(0.0f, 360.0f);
    const float r = random(0.0f, maxRtest);
    // calculate the probability
    const float partTheta = normTheta * associatedLegendre(cosf(theta*Point3D<float>::DEGTORAD), abs(m), l);
    float partPhi = normPhi;
    if(m == 0)
      partPhi /= sqrtf(2.0f);
    else if(m > 0)
     partPhi *= sinf(m*phi*Point3D<float>::DEGTORAD);
    else
     partPhi *= cosf(m*phi*Point3D<float>::DEGTORAD);
    const float rho = 2.0f * zna * r;
    const float partR = normR * pow(rho, static_cast<int>(l)) * exp(-rho/2.0f) * associatedLaguerre(rho, 2*l+1, n-l-1);
    const float probability = pow(partTheta * partPhi * partR, 2);
    if(probability > maxProb)
    {
      maxProb = probability;
      maxProbR = r;
    }
  }
  qDebug("estimated maximum probability = %f", maxProb);

  // do the actual run
  unsigned int currentDots = 0;
  while(currentDots < numDots)
  {
    if(stopRequested)
      return;

    // generate a position in spherical coordinates
    const float theta = random(0.1f, 179.9f);
    const float phi = random(0.0f, 360.0f);
    const float r = random(0.0f, maxR);

    // determine whether this point is to be drawn (|sin(theta)| function is maximum near the equator and zero at the poles)
    if(random(0.0f, 1.0f) > fabs(sinf(theta*Point3D<float>::DEGTORAD)))
      continue;

    // calculate the probability
    const float partTheta = normTheta * associatedLegendre(cosf(theta*Point3D<float>::DEGTORAD), abs(m), l);
    float partPhi = normPhi;
    if(m == 0)
      partPhi /= sqrtf(2.0f);
    else if(m > 0)
     partPhi *= sinf(m*phi*Point3D<float>::DEGTORAD);
    else
     partPhi *= cosf(m*phi*Point3D<float>::DEGTORAD);
    const float rho = 2.0f * zna * r;
    const float partR = normR * pow(rho, static_cast<int>(l)) * exp(-rho/2.0f) * associatedLaguerre(rho, 2*l+1, n-l-1);
    const float probability = pow(partTheta * partPhi * partR, 2);
    if(probability > maxProb)
    {
      maxProb = probability; // adjust while in the loop
      maxProbR = r;
    }
    if(probability > random(0.0f, maxProb))
    {
      // add this point
      Point3D<float> newCoord;
      newCoord.setPolar(theta, phi, r);
      newCoord.setID(partTheta*partPhi*partR > 0.0f ? 1 : 0); 
      coordsList.push_back(newCoord);
      updateList(coordsList);
      if(r > maximumRadius) maximumRadius = r;
      currentDots++;
      // notify the progress
      if(currentDots % updateFreq == 0)
      {
        progress = currentDots;
        QCustomEvent* e = new QCustomEvent(static_cast<QEvent::Type>(1001),&progress);
        QApplication::postEvent(receiver, e);
      }
    }
  }
  updateList(coordsList, true);
  qDebug("final maximum probability = %f", maxProb);
  qDebug(" corresponding radius = %f", maxProbR);
}

///// calcRadialPart //////////////////////////////////////////////////////////
void OrbitalThread::calcRadialPart()
/// Draws the radial part of the orbital.
{
  const int updateFreq = static_cast<int>(10.0f * resolution)/100;

  // clear the data
  mutex->lock();
  coords->clear();
  mutex->unlock();
  std::vector<Point3D<float> > coordsList;
  coordsList.reserve(updateSize);
  
  // a short local form of the quantum numbers
  const unsigned int n = qnPrincipal;
  const unsigned int l = qnOrbital;

  // values independent of r
  const float zna = static_cast<float>(atomNumber) / static_cast<float>(n) / abohr; // conversion factor for r->rho
  const float normR = pow(2.0f*zna, 1.5f) * sqrt(factorial<float>(n-l-1)/2.0f/n/factorial<float>(n+l)); // normalization factor for the radial part
  const float maxR = 2.0f*static_cast<float>(n*n); // maximum value for r to check
  const float incR = maxR / (10.0f * resolution); // increment for r

  maximumRadius = maxR;

  //float probability = 0.0f;
  //float probabilityRadius = 0.0f;

  // loop over r
  for(float r = 0.0f; r < maxR; r += incR)
  {
    if(stopRequested)
      return;

    // notify the progress
    progress = static_cast<unsigned int>(r/incR);
    if(progress % updateFreq == 0)
    {
      QCustomEvent* e = new QCustomEvent(static_cast<QEvent::Type>(1001),&progress);
      QApplication::postEvent(receiver, e);
    }

    // calculate the value
    const float rho = 2.0f * zna * r;
    const float partR = normR * pow(rho, static_cast<int>(l)) * exp(-rho/2.0f) * associatedLaguerre(rho, 2*l+1, n-l-1);

    // the radial part => amplitude
    Point3D<float> newCoord1(r, partR, 0.0f);
    newCoord1.setID(1);
    // the radial part => probability = R^2r^2
    Point3D<float> newCoord2(r, partR*partR*r*r, 0.0f);
    newCoord2.setID(0);
    // add the points
    coordsList.push_back(newCoord1);
    coordsList.push_back(newCoord2);
    updateList(coordsList);
    /*// determine the distance with 90% probability
    if(probability < 0.90f)
    {
      probability += partr*partr*r*r/resolution;
      if(probability >= 0.90f)
        probabilityRadius = r;
    }*/
  }
  updateList(coordsList, true);
}

///// calcAngularPart /////////////////////////////////////////////////////////
void OrbitalThread::calcAngularPart()
/// Calculates the angular part of the orbital.
{
  const int updateFreq = static_cast<int>(resolution * resolution)/100;

  // clear the data
  mutex->lock();
  coords->clear();
  mutex->unlock();
  maximumRadius = 0.0f;
  std::vector<Point3D<float> > coordsList;
  coordsList.reserve(updateSize);
  
  // a short local form of the quantum numbers
  const unsigned int l = qnOrbital;
  const int m = qnMomentum;

  // values independent of theta or phi
  const float normTheta = pow(-1.0f, (m + abs(m))/2) * sqrtf( (2*l+1) * factorial<float>(l-abs(m)) / (2.0f * factorial<float>(l+abs(m)))); // normalization factor for the angular part (Theta dependent)
  const float normPhi = 1.0f / sqrtf(Point3D<float>::PI); // normalization factor for the angular part (Phi dependent)
  const float incTheta = 180.0f/resolution; // increment for theta
  const float incPhi = 360.0f/resolution; // increment for phi

  ///// loop over THETA ///////////////
  for(float theta = 0.0f; theta < 180.0f; theta += incTheta) // rotation away from z-axis: only 180 degrees
  {

    ///// loop over PHI ///////////////
    for(float phi = 0.0f; phi < 360.0f; phi += incPhi) // rotation around z-axis: full 360 degrees
    {              
      if(stopRequested)
        return;

      // notify the progress
      progress = static_cast<int>(theta/incTheta * resolution + phi/incPhi);
      if(progress % updateFreq == 0)
      {
        QCustomEvent* e = new QCustomEvent(static_cast<QEvent::Type>(1001),&progress);
        QApplication::postEvent(receiver, e);
      }

      // randomize theta within its square
      const float randTheta = theta + random(-incTheta/2.0f, incTheta/2.0f);
      // determine whether this point is to be drawn (|sin(theta)| function is maximum near the equator and zero at the poles)
      if(random(0.0f, 1.0f) > fabs(sinf(randTheta*Point3D<float>::DEGTORAD)))      
        continue;
      // get the result for theta
      const float partTheta = normTheta * associatedLegendre(cosf(randTheta*Point3D<float>::DEGTORAD), abs(m), l);

      // randomize phi within its square
      const float randPhi = phi + random(-incPhi/2.0f, incPhi/2.0f);
      // get the result for phi
      float partPhi = normPhi;
      if(m == 0)
        partPhi /= sqrtf(2.0f);
      else if(m > 0)
       partPhi *= sinf(m*randPhi*Point3D<float>::DEGTORAD);
      else
       partPhi *= cosf(m*randPhi*Point3D<float>::DEGTORAD);

      // total angular result
      const float partY = partTheta * partPhi;

      // save the data
      const float r = fabs(partY);
      Point3D<float> newCoord;
      newCoord.setPolar(randTheta, randPhi, r);
      newCoord.setID(partY > 0.0f ? 1 : 0); 
      coordsList.push_back(newCoord);
      updateList(coordsList);
      if(r > maximumRadius) maximumRadius = r;
    }
  }
  updateList(coordsList, true);
}

///// updateList //////////////////////////////////////////////////////////////
void OrbitalThread::updateList(std::vector<Point3D<float> >& newCoords, bool final)
/// Updates the shared coordinate list with a set of newly calculated points.
/// If final is true, the update is forces, even if less than updateSize values are present.
{
  if(newCoords.size() >= updateSize || (final && !newCoords.empty()))
  {
    mutex->lock();
    coords->insert(coords->end(), newCoords.begin(), newCoords.end());
    mutex->unlock();
    newCoords.clear();
    qDebug("Added %d new points", updateSize);
  }
}

///// associatedLegendre //////////////////////////////////////////////////////
float OrbitalThread::associatedLegendre(const float x, const int m, const unsigned int l)
/// Calculates the associated Legendre polynomial P ^m _l (x).
{
  float prefactor = pow(1.0f - x*x, static_cast<float>(m)/2.0f) / pow(2.0f, static_cast<int>(l));
  int upperBound = 0;
  if(((l-m) % 2) == 0) // l-m is even
    upperBound = (l-m)/2;
  else // l-m is odd
    upperBound = (l-m-1)/2;

  float result = 0.0f;
  for(int j = 0; j <= upperBound; j++)
  {
    float tempvar = pow(-1.0f, j)*factorial<float>(2*l-2*j);
    tempvar /= factorial<float>(j)*factorial<float>(l-j)*factorial<float>(l-abs(m)-2*j);
    tempvar *= pow(x, static_cast<int>(l-m-2*j));
    result +=  tempvar;
  }
  return prefactor*result;
}

///// associatedLaguerre //////////////////////////////////////////////////////
float OrbitalThread::associatedLaguerre(const float x, const int m, const unsigned int n)
/// Calculates the associated Laguerre polynomial L ^m _n (x).
{  
  // from MathWorld: Rodrigues representation of the Laguerre polynomial (Arfken & Webers definition)
  if(n == 0)
    return 1.0f;
  float result = 0.0f;
  for(unsigned int j = 0; j <= n; j++)
    result += pow(-1.0f, static_cast<int>(j)) * factorial<float>(n+m) / factorial<float>(n-j) / factorial<float>(m+j) / factorial<float>(j) * pow(x, static_cast<int>(j));
  return result;
}

///// factorial ///////////////////////////////////////////////////////////////
template <class T> T OrbitalThread::factorial(const unsigned int number)
/// Calculates the factorial function of the given number.
/// Not too efficient but does the job.
/// Converted to template because the limits of unsigned int's are reached for n+l=13
/// (for unsigned long int's it's n+l=21).
{
  if(number <= 1)
    return 1;
  unsigned int counter = number - 1;
  T result = static_cast<T>(number);
  while(counter > 1)
  {
    result *= static_cast<T>(counter);
    counter--;
  }
  return result;
}

///// random //////////////////////////////////////////////////////////////////
float OrbitalThread::random(const float min, const float max)
/// Returns a random floating point number between min and max.
{  
  return min + static_cast<float>(rand())/RAND_MAX*(max - min);
}

///// largestResult ///////////////////////////////////////////////////////////
template <class T> T OrbitalThread::largestResult(const unsigned int n, const unsigned int l, const int m, const unsigned int Z)
/// Returns an approximation to the largest possible result for
/// the given quantum numbers.
{
  T partPhi = static_cast<T>(0.5641895835); // 1/sqrt(PI) // is partial specialization for local variables possible
  T partTheta = sqrt(factorial<T>(2*l+1) * factorial<T>(l-abs(m)) / 2.0 / factorial<T>(l+abs(m))) * associatedLegendre(1.0f, abs(m), l);
  T rmax = static_cast<T>(3*n*n);
  T partR = pow(2.0*Z/n/abohr, 1.5) * sqrt(factorial<T>(n-l-1)/2.0/n/factorial<T>(n+l)) * pow(2.0*Z/n/abohr*rmax, static_cast<int>(l)) * associatedLaguerre(2.0*Z/n/abohr*rmax, 2*l+1, n-l-1);
  return partPhi*partTheta*partR;
}

///////////////////////////////////////////////////////////////////////////////
///// Static Variables                                                    /////
///////////////////////////////////////////////////////////////////////////////

const float OrbitalThread::abohr = 1.0f;
const unsigned int OrbitalThread::updateSize = 1000;

