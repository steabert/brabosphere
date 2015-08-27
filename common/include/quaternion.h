/***************************************************************************
                          quaternion.h  -  description
                             -------------------
    begin                : Fri Feb 28 2003
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
  \class Quaternion
  \brief This class represents a quaternion (4D vector).

  It is based on the quaternion class by Sobiet Void (robin@cyberversion.com).
  From the url http://www.gamedev.net/reference/articles/article1095.asp.
  This is a template version so no implementation file is present. The header file
  contains both the declarationand the implementation.

*/
/// \file
/// Contains the declaration and implementation of the template class Quaternion.

#ifndef QUATERNION_H
#define QUATERNION_H


///// Header files ////////////////////////////////////////////////////////////

// C++ header files
#include <cmath>
#ifndef QT_NO_DEBUG
  #include <iostream>
#endif

// Xbrabo header files
#include <vector3d.h> // includes point3d.h
template <class T> class Vector3D; // if vector3d.h was already included, we still need a forward declaration (don't know why)

///////////////////////////////////////////////////////////////////////////////
///// TEMPLATE CLASS DEFINITION                                           /////
///////////////////////////////////////////////////////////////////////////////

///// class Quaternion ///////////////////////////////////////////////////////
template <class T> class Quaternion
{
  public:
    Quaternion();                       // default constructor (quaternion with zero rotation)
    Quaternion(const T w, const T x, const T y, const T z); // constructs a quaternion (w, x, y, z)
    Quaternion(const T xAngle, const T yAngle, const T zAngle);       // constructs a quaternion from Euler angles xAngle, yAngle and zAngle
    Quaternion(const Vector3D<T> v, const T angle);         // constructs a quaternion from an 'angle around a certain axis' representation
    ~Quaternion();                      // destructor

    ///// public operators
    Quaternion operator*(const Quaternion& quat); // multiplication
    Quaternion& operator=(const Quaternion& quat);// copy constructor

    ///// public member functions for changing the quaternion
    void identity();                    // set the quaternion to the identity quaternion
    void normalize();                   // normalize the quaternion
    void setValues(const T w, const T x, const T y, const T z);       // set the values
    void conjugate();                   // changes the quaternion into its conjugate
    void inverse();                     // changes the quaternion into its inverse

    // public member function for retrieving data
    T w() const;                        // returns the w-value
    T x() const;                        // returns the x-value
    T y() const;                        // returns the y-value
    T z() const;                        // returns the z-value

    // public member functions for converting between representations
    void eulerToQuaternion(const T xAngle, const T yAngle, const T zAngle);     // converts Euler angles to a quaternion
    void axisToQuaternion(const Vector3D<T> v, const T angle);        // converts from axis/angle to quaternion
    void getAxisAngle(Vector3D<T>& v, T& angle);  // returns axis/angle for the quaternion

  private:
    // private member functions
    void limitRange(const T minimum, T& value, const T maximum);      // limits a value between a min and a max

    // private member data
    T wQuat, xQuat, yQuat, zQuat;   ///< Internal quaternion representation (w, xi, yj, zk).
};

///////////////////////////////////////////////////////////////////////////////
///// Public Member Functions                                             /////
///////////////////////////////////////////////////////////////////////////////

///// Constructor /////////////////////////////////////////////////////////////
template <class T> Quaternion<T>::Quaternion()
/// The default constructor.
{
  identity();
}

///// Constructor (overloaded) ////////////////////////////////////////////////
template <class T> Quaternion<T>::Quaternion(const T w, const T x, const T y, const T z)
/// \overload
/// With this version the individual components of the quaternion can be set.
{
  setValues(w, x, y, z);
}

///// Constructor (overloaded) ////////////////////////////////////////////////
template <class T> Quaternion<T>::Quaternion(const T xAngle, const T yAngle, const T zAngle)
/// \overload
/// This version allows creation of a quaternion from an Euler representation.
{
  eulerToQuaternion(xAngle, yAngle, zAngle);
}

///// Constructor (overloaded) ////////////////////////////////////////////////
template <class T> Quaternion<T>::Quaternion(const Vector3D<T> v, const T angle)
/// \overload
/// This version allows creation of a quaternion from an axis/angle representation.
{
  axisToQuaternion(v, angle);
}

///// Destructor //////////////////////////////////////////////////////////////
template <class T> Quaternion<T>::~Quaternion()
/// The default destructor.
{
}

///// operator* ///////////////////////////////////////////////////////////////
template <class T> Quaternion<T> Quaternion<T>::operator*(const Quaternion& quat)
/// Implements quaternion multiplication.
{
  T rw = wQuat*quat.wQuat - xQuat*quat.xQuat - yQuat*quat.yQuat - zQuat*quat.zQuat;
  T rx = xQuat*quat.wQuat + wQuat*quat.xQuat + zQuat*quat.yQuat - yQuat*quat.zQuat;
  T ry = yQuat*quat.wQuat + wQuat*quat.yQuat + xQuat*quat.zQuat - zQuat*quat.xQuat;
  T rz = zQuat*quat.wQuat + wQuat*quat.zQuat + yQuat*quat.xQuat - xQuat*quat.yQuat;

  return Quaternion(rw, rx, ry, rz);
}

///// operator= ///////////////////////////////////////////////////////////////
template <class T> Quaternion<T>& Quaternion<T>::operator=(const Quaternion& quat)
/// The copy constructor.
{
  wQuat = quat.wQuat;
  xQuat = quat.xQuat;
  yQuat = quat.yQuat;
  zQuat = quat.zQuat;

  return (*this);
}

///// identity ////////////////////////////////////////////////////////////////
template <class T> void Quaternion<T>::identity()
/// Sets the quaternion to the identity quaternion (1, 0, 0, 0).
{
  wQuat = 1.0f; // identity quaternion for multiplication
  xQuat = 0.0f;
  yQuat = 0.0f;
  zQuat = 0.0f;
}

///// normalize ///////////////////////////////////////////////////////////////
template <class T> void Quaternion<T>::normalize()
/// Normalizes the quaternion.
{
  T norm = wQuat*wQuat + xQuat*xQuat + yQuat*yQuat + zQuat*zQuat;
  if(norm < Point3D<T>::TOLERANCE)
  {
#ifndef QT_NO_DEBUG
    std::cerr << "Quaternion::normalize(): Norm of the quaternion too close to zero" << std::endl;
#endif
    return;
  }

  wQuat /= norm;
  xQuat /= norm;
  yQuat /= norm;
  zQuat /= norm;

  norm = wQuat*wQuat + xQuat*xQuat + yQuat*yQuat + zQuat*zQuat;
  if(fabs(norm - 1.0f) > Point3D<T>::TOLERANCE)
  {
#ifndef QT_NO_DEBUG
    std::cerr << "Quaternion::normalize(): Norm of the quaternion not equal to 1 but " << norm << std::endl;
#endif
    return;
  }

  limitRange(-1.0, wQuat, 1.0);
  limitRange(-1.0, xQuat, 1.0);
  limitRange(-1.0, yQuat, 1.0);
  limitRange(-1.0, zQuat, 1.0);
}

///// setValues ///////////////////////////////////////////////////////////////
template <class T> void Quaternion<T>::setValues(const T w, const T x, const T y, const T z)
/// Sets the quaternion to the given values.
{
  if((w*w + x*x + y*y + z*z) < Point3D<T>::TOLERANCE)
  {
#ifndef QT_NO_DEBUG
    std::cerr << "Quaternion::setValues(wxyz): Norm of the quaternion too close to zero" << std::endl;
    std::cerr << " values: " << w << " " << x << " " << y << " " << z << std::endl;
#endif
    identity();
    return;
  }

  wQuat = w;
  xQuat = x;
  yQuat = y;
  zQuat = z;

  normalize();
}

///// conjugate ///////////////////////////////////////////////////////////////
template <class T> void Quaternion<T>::conjugate()
/// Changes the quaternion into its conjugate.
{
  xQuat = -xQuat;
  yQuat = -yQuat;
  zQuat = -zQuat;
}

///// inverse /////////////////////////////////////////////////////////////////
template <class T> void Quaternion<T>::inverse()
/// Changes the quaternion into its inverse.
{
  conjugate();
  T norm = wQuat*wQuat + xQuat*xQuat + yQuat*yQuat + zQuat*zQuat;
  wQuat /= norm;
  xQuat /= norm;
  yQuat /= norm;
  zQuat /= norm;
}

///// w ///////////////////////////////////////////////////////////////////////
template <class T> T Quaternion<T>::w() const
/// Returns the w-value of the quaternion.
{
  return wQuat;
}

///// x ///////////////////////////////////////////////////////////////////////
template <class T> T Quaternion<T>::x() const
/// Returns the x-value of the quaternion.
{
  return xQuat;
}

///// y ///////////////////////////////////////////////////////////////////////
template <class T> T Quaternion<T>::y() const
/// Returns the y-value of the quaternion.
{
  return yQuat;
}

///// z ///////////////////////////////////////////////////////////////////////
template <class T> T Quaternion<T>::z() const
/// Returns the z-value of the quaternion.
{
  return zQuat;
}

///// eulerToQuaternion ///////////////////////////////////////////////////////
template <class T> void Quaternion<T>::eulerToQuaternion(const T xAngle, const T yAngle, const T zAngle)
/// Returns a quaternion constructed from 3 Euler angles (in degrees).
{
  T ex = xAngle*Point3D<T>::DEGTORAD / 2.0;  // temp half euler angles
  T ey = yAngle*Point3D<T>::DEGTORAD / 2.0;  // temp half euler angles
  T ez = zAngle*Point3D<T>::DEGTORAD / 2.0;  // temp half euler angles

  T cr = cos(ex);  // roll
  T cp = cos(ey);  // pitch
  T cy = cos(ez);  // yaw

  T sr = sin(ex);  // roll
  T sp = sin(ey);  // pitch
  T sy = sin(ez);  // yaw

  T cpcy = cp*cy;
  T spsy = sp*sy;

  wQuat = cr*cpcy + sr*spsy;
  xQuat = sr*cpcy - cr*spsy;
  yQuat = cr*sp*cy + sr*cp*sy;
  zQuat = cr*cp*sy - sr*sp*cy;

  normalize();
}

///// axisToQuaternion ///////////////////////////////////////////////////////
template <class T> void Quaternion<T>::axisToQuaternion(const Vector3D<T> v, const T angle)
/// Returns a quaternion constructed from an
/// axis/angle representation (in degrees).
{
  ///// if the vector is zero, return the (1, 0, 0, 0) quaternion
  if(v.isZero())
  {
    setValues(1.0, 0.0, 0.0, 0.0);
    return;
  }

  ///// generate a unit vector from the input vector
  Vector3D<T> unitVector = v;
  unitVector.normalize();

  ///// calculate the Quaternion
  T rad = angle * Point3D<T>::DEGTORAD / 2.0;
  T scale = sin(rad);
  setValues(cos(rad), unitVector.x() * scale, unitVector.y() * scale, unitVector.z() * scale);
  normalize();
}

///// getAxisAngle ////////////////////////////////////////////////////////////
template <class T> void Quaternion<T>::getAxisAngle(Vector3D<T>& v, T& angle)
/// Returns the axis (vector) - angle (in degrees)
/// representation of the quaternion.
{
  T scale = sqrt(xQuat*xQuat + yQuat*yQuat + zQuat*zQuat);

  if(fabs(scale) < Point3D<T>::TOLERANCE)
  {
    ///// angle is 0 or 360 degrees
    angle = 0.0;
    v.setValues(0.0, 0.0, 1.0);
  }
  else
  {
    angle = acos(wQuat)*2.0*Point3D<T>::RADTODEG;
    v.setValues(xQuat/scale, yQuat/scale, zQuat/scale);
    v.normalize();
  }
}


///////////////////////////////////////////////////////////////////////////////
///// Private Member Functions                                            /////
///////////////////////////////////////////////////////////////////////////////

///// limitRange //////////////////////////////////////////////////////////////
template <class T> void Quaternion<T>::limitRange(const T minimum, T& value, const T maximum)
/// Limits the range of value between minimum and maximum.
{
  if(value < minimum)
    value = minimum;
  else if(value > maximum)
    value = maximum;
}

#endif

