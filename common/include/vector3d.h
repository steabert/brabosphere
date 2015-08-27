/***************************************************************************
                          vector3d.h  -  description
                             -------------------
    begin                : Mon Mar 3 2003
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
  \class Vector3D
  \brief This class represents a 3D vector.
  
  It is a templated version so the implementation is in the header file. 

*/
/// \file
/// Contains the declaration and implementation of the templated class Vector3D.

#ifndef VECTOR3D_H
#define VECTOR3D_H

///// Header files ////////////////////////////////////////////////////////////

// C++ header files
#include <cmath>
#include <iostream>

// Xbrabo header files
#include <point3d.h>
#include <quaternion.h>

///// class Vector3D //////////////////////////////////////////////////////////
template<class T> class Vector3D
{
  public:
    Vector3D();                         // default constructor
    Vector3D(const T x, const T y, const T z);    // constructs a vector (xyz)
    Vector3D(const T x1, const T y1, const T z1, const T x2, const T y2, const T z2);     // constructs a vector from point1 to point2
    Vector3D(const Point3D<T> point1, const Point3D<T> point2);       // constructs a vector from point1 to point2
    Vector3D(const Vector3D& v);        // copy constructor
    ~Vector3D();                        // destructor

    // public member functions for retrieving data
    bool isUnit() const;                // returns true of it's a unit vector
    bool isZero() const;                // returns true if the vector is zero
    T x() const;                        // returns the x-value
    T y() const;                        // returns the y-value
    T z() const;                        // returns the z-value
    T length() const;                   // returns the length of the vector

    // public member functions for changing data
    void setValues(const T x, const T y, const T z);        // sets the vector
    void normalize();                   // normalizes the vector
    void setLength(const T newLength);  // sets a new length for the vector
    void changeLength(const T amount);  // changes the length of the vector by amount
    void invert();                      // inverts the direction of the vector
    void add(const Vector3D v);         // adds the vector v
    void rotate(const Vector3D axis, const T angle);        // rotate the vector around the axis by an angle
    void setTorsion(const T angle, const Vector3D refBond, const Vector3D centralBond);   // sets the torsion angle

    // public member functions for data generation
    Vector3D cross(const Vector3D v) const;       // calculates the cross product of 2 vectors
    T dot(const Vector3D v) const;      // calculates the dot product of 2 vectors
    T angle(const Vector3D v) const;    // calculates the angle between 2 vectors
    T torsion(const Vector3D v1, const Vector3D v2) const;  // calculates the torsion angle with another vector

  private:
    // private member variables
    T xVect, yVect, zVect;              ///< The vector values.
};

///////////////////////////////////////////////////////////////////////////////
///// Public Member Functions                                             /////
///////////////////////////////////////////////////////////////////////////////

///// Constructor /////////////////////////////////////////////////////////////
template <class T> Vector3D<T>::Vector3D()
/// The default constructor.
{
  setValues(0.0, 0.0, 0.0);
}

///// Constructor (overloaded) ////////////////////////////////////////////////
template <class T> Vector3D<T>::Vector3D(const T x, const T y, const T z)
/// \overload
/// It allows setting the values explicitly.
{
  setValues(x, y, z);
}

///// Constructor (overloaded) ////////////////////////////////////////////////
template <class T> Vector3D<T>::Vector3D(const T x1, const T y1, const T z1, const T x2, const T y2, const T z2)
/// \overload
/// It constructs a vector from 2 points.
{
  setValues(x2 - x1, y2 - y1, z2 - z1);
}

///// Constructor (overloaded) ////////////////////////////////////////////////
template <class T> Vector3D<T>::Vector3D(const Point3D<T> point1, const Point3D<T> point2)
/// \overload
/// It constructs a vector from 2 points.
{
  setValues(point2.x() - point1.x(), point2.y() - point1.y(), point2.z() - point1.z());
}

///// Copy Constructor ////////////////////////////////////////////////////////
template <class T> Vector3D<T>::Vector3D(const Vector3D& v)
/// The copy constructor.
{
  xVect = v.xVect;
  yVect = v.yVect;
  zVect = v.zVect;
}

///// Destructor //////////////////////////////////////////////////////////////
template <class T> Vector3D<T>::~Vector3D()
/// The default destructor.
{

}

///// isUnit //////////////////////////////////////////////////////////////////
template <class T> bool Vector3D<T>::isUnit() const
/// Returns true if the vector is a unit vector.
{
  if(fabs(length() - 1.0) < Point3D<T>::TOLERANCE)
    return true;
  else
    return false;
}

///// isZero //////////////////////////////////////////////////////////////////
template <class T> bool Vector3D<T>::isZero() const
/// Returns true if the vector has zero length.
{
  if(fabs(length()) < Point3D<T>::TOLERANCE)
    return true;
  else
    return false;
}

///// x ///////////////////////////////////////////////////////////////////////
template <class T> T Vector3D<T>::x() const
/// Returns the x-component of the vector.
{
  return xVect;
}

///// y ///////////////////////////////////////////////////////////////////////
template <class T> T Vector3D<T>::y() const
/// Returns the y-component of the vector.
{
  return yVect;
}
///// z ///////////////////////////////////////////////////////////////////////
template <class T> T Vector3D<T>::z() const
/// Returns the z-component of the vector.
{
  return zVect;
}

///// length //////////////////////////////////////////////////////////////////
template <class T> T Vector3D<T>::length() const
/// Returns the length of the vector.
{
  return sqrt(xVect*xVect + yVect*yVect + zVect*zVect);
}

///// setValues ///////////////////////////////////////////////////////////////
template <class T> void Vector3D<T>::setValues(const T x, const T y, const T z)
/// Sets the values of all vector components.
{
  xVect = x;
  yVect = y;
  zVect = z;
}

///// normalize ///////////////////////////////////////////////////////////////
template <class T> void Vector3D<T>::normalize()
/// Normalizes the vector.
{
  T len = length();
  if(len < Point3D<T>::TOLERANCE)
  {
    xVect = 0.0;
    yVect = 0.0;
    zVect = 0.0;
  }
  else
  {
    xVect = xVect/len;
    yVect = yVect/len;
    zVect = zVect/len;
  }
}

///// setLength ///////////////////////////////////////////////////////////////
template <class T> void Vector3D<T>::setLength(const T newLength)
/// Sets a new length for the vector.
{
  // determine the scaling factor
  T scalingFactor = newLength/length();
  // scale the values
  xVect *= scalingFactor;
  yVect *= scalingFactor;
  zVect *= scalingFactor;

}

///// changeLength ////////////////////////////////////////////////////////////
template <class T> void Vector3D<T>::changeLength(const T amount)
/// Makes the vector \c amount longer (or shorter if negative).
{
  // determine the scaling factor
  T oldLength = length();
  if(oldLength < Point3D<T>::TOLERANCE)
    return;
  T scalingFactor = (oldLength + amount)/oldLength;
  // scale the values
  xVect *= scalingFactor;
  yVect *= scalingFactor;
  zVect *= scalingFactor;

}

///// invert //////////////////////////////////////////////////////////////////
template <class T> void Vector3D<T>::invert()
/// Inverts the direction of the vector.
{
  xVect = -xVect;
  yVect = -yVect;
  zVect = -zVect;
}

///// add /////////////////////////////////////////////////////////////////////
template <class T> void Vector3D<T>::add(const Vector3D v)
/// Adds the vector \c v to the current vector.
{
  xVect += v.xVect;
  yVect += v.yVect;
  zVect += v.zVect;
}

///// rotate //////////////////////////////////////////////////////////////////
template <class T> void Vector3D<T>::rotate(const Vector3D axis, const T angle)
/// Rotates the vector by the specified angle around the given axis.
{
  ///// build a quaternion from the axis and angle
  Quaternion<T> rotation(axis, angle);
  ///// determine the inverse
  Quaternion<T> inverse = rotation;
  inverse.inverse();
  ///// build a quaternion from the vector to rotate
  Quaternion<T> thisVector(0.0, xVect, yVect, zVect);
  ///// determine the resulting vector
  Quaternion<T> result = rotation*thisVector*inverse;
  result.normalize();
  setValues(result.x(), result.y(), result.z());
}

///// cross ///////////////////////////////////////////////////////////////////
template <class T> Vector3D<T> Vector3D<T>::cross(const Vector3D v) const
/// Calculates the cross product of the vector \c v and the current vector.
{
  T resultX = yVect * v.zVect - v.yVect * zVect;
  T resultY = zVect * v.xVect - v.zVect * xVect;
  T resultZ = xVect * v.yVect - v.xVect * yVect;

  return Vector3D(resultX, resultY, resultZ);
}

///// dot /////////////////////////////////////////////////////////////////////
template <class T> T Vector3D<T>::dot(const Vector3D v) const
/// Calculates the dot product of the vector \c v and the current vector.
{
  return xVect*v.xVect + yVect * v.yVect + zVect * v.zVect;
}

///// angle ///////////////////////////////////////////////////////////////////
template <class T> T Vector3D<T>::angle(const Vector3D v) const
/// Calculates the angle between the vector \c v and the
/// current vector. (arccos(dot(v1, v2))). The result is returned in degrees.
{
  Vector3D<T> v1 = v;
  Vector3D<T> v2 = *this;
  v1.normalize();
  v2.normalize();
  return acos(v1.dot(v2))*Point3D<T>::RADTODEG;
}

///// torsion ///////////////////////////////////////////////////////////////////
template <class T> T Vector3D<T>::torsion(const Vector3D v1, const Vector3D v2) const
/// Calculates the torsion angle between the current vector
/// and vector \c v1 around vector \c v2. The result is returned in degrees.
{
  Vector3D<T> va = *this; // the first vector
  Vector3D<T> vb = v2; // the central vector
  Vector3D<T> vc = v1; // the second vector

  va.normalize();
  vb.normalize();
  vc.normalize();

  ///// determine -va.vc + (va.vb)(vb.vc) and va.(vbxvc)
  T argx = - va.dot(vc) + va.dot(vb) * vb.dot(vc);
  T argy = va.dot(vb.cross(vc));

  ///// determine the torsion angle using 2 methods (using arrcos and arcsin)
  T acosAngle = acos(argx/sqrt(argx*argx + argy*argy))*Point3D<T>::RADTODEG;
  ///// Windows sometimes gives a QNAN for angle = 90 degrees, so use the C99 function hypot (also works with GCC)
//#ifdef Q_OS_WIN32
//  T asinAngle = asin(argy/sqrt(hypot(argx, argy)))*RADTODEG; //=> doesn't work too well when using templates
//#else
  T asinAngle = asin(argy/sqrt(argx*argx + argy*argy))*Point3D<T>::RADTODEG;
//#endif

  ///// chose the correct angle for each quadrant
#ifdef Q_OS_WIN32
  if(acosAngle == 90.0)
    return 90.0; // because the 'hypot' trick doesn't work when changing to template code
  else if(acosAngle > 90.0)
#else      
  if(acosAngle >= 90.0) // 1st and 4th
#endif
    return -asinAngle;
  else if(asinAngle < 0.0) // 3rd
    return asinAngle + 180.0;
  else
    return asinAngle - 180.0; // 2nd

  return 0.0; // omits warnings
}

///// setTorsion //////////////////////////////////////////////////////////////
template <class T> void Vector3D<T>::setTorsion(const T angle, const Vector3D refBond, const Vector3D centralBond)
/// Changes the orientation of the current vector to give
/// the specified torsion angle with the \c refBond around the \c centralBond.
/// \warning Not implemented yet.
{
  // new implementation: - get the current torsion angle
  //                     - calculate the difference with the requested torsion angle
  //                     - call rotate() to do the rotation around the central bond

  /*
  T torsionAngle = angle * DEGTORAD;
  Vector3D va = *this;
  Vector3D vb = centralBond;
  Vector3D vc = centralBond;
  vc.invert();
  Vector3D vd = refBond;

  T orgLength = va.length();
  va.normalize();
  vb.normalize();
  vc.normalize();
  vd.normalize();

  Vector3D v1 = va.cross(vb);
  Vector3D v2 = vc.cross(vd);

  while(torsionAngle > 180.0f)
    torsionAngle -= 360.0f;
  while(torsionAngle < -180.0f)
    torsionAngle += 360.0f;

  if(torsionAngle <= 90.0f && torsionAngle >= -90.0f) // 1st or 4th quadrant
  {
    v1.rotate(vb, torsionAngle);
  }
  */

  std::cerr << "Vector3D::setTorsion not implemented yet. DO NOT USE" << std::endl;
  exit(1);
}

#endif

