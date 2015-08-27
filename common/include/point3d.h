/***************************************************************************
                          point3d.h  -  description
                             -------------------
    begin                : Tue Mar 15 2005
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
  \class Point3D
  \brief This class represents a 3D point.
  
  Template version so the implementation is in the header file. 
  Only static constants are assigned in the cpp file. At the moment partial
  specialisation is added for the types float, double and unsigned int.

*/
/// \file
/// Contains the declaration and implementation of the template class Point3D.

#ifndef POINT3D_H
#define POINT3D_H

///// Forward class declarations & header files ///////////////////////////////
#include <cmath>


///// class Point3D ///////////////////////////////////////////////////////////
template<class T> class Point3D
{
  public:
    Point3D();                          // default constructor
    Point3D(const T x, const T y, const T z);     // constructs a point with coordinates x, y, z
    Point3D(const Point3D& p);          // copy constructor
    ~Point3D();                         // destructor

    ///// public member functions for retrieving data
    T x() const;                        // returns the x-value
    T y() const;                        // returns the y-value
    T z() const;                        // returns the z-value
    unsigned int id() const;            // returns the point's ID (used in IsoSurface and PlotMapBase)
    bool operator==(const Point3D<double>& p) const         
    /// partial specialization of equality testing for type double
    {
      if(xCoord == p.xCoord && yCoord == p.yCoord && zCoord == p.zCoord)
        return true;
      return fabs(xCoord - p.xCoord) < TOLERANCE && fabs(yCoord - p.yCoord) < TOLERANCE && fabs(zCoord - p.zCoord) < TOLERANCE;
    }
    bool operator==(const Point3D<float>& p) const          
    /// partial specialization of equality testing for type float
    {
      if(xCoord == p.xCoord && yCoord == p.yCoord && zCoord == p.zCoord)
        return true;
      return fabs(xCoord - p.xCoord) < TOLERANCE && fabs(yCoord - p.yCoord) < TOLERANCE && fabs(zCoord - p.zCoord) < TOLERANCE;
    }
    bool operator==(const Point3D<unsigned int>& p) const   
    /// partial specialization of equality testing for type unsigned int
    {  
      return xCoord == p.xCoord && yCoord == p.yCoord && zCoord == p.zCoord;
    }

    ///// public member functions for changing data
    void setValues(const T x, const T y, const T z);           // sets the soordinates (cartesian by default)
    void setCartesian(const T x, const T y, const T z);        // sets the cartesian coordinates directly
    void setPolar(const T theta, const T phi, const T r);      // sets the coordinates from polar coordinates
    void setID(const unsigned int id);  // sets the point's ID
    void add(const Point3D& p);         // adds the point p (maybe do the same for adding a Vector3D to it)

    ///// public constants (made static for ease, also used by Quaternion and Vector3D)
    static const T PI;                  ///< The value of Pi
    static const T DEGTORAD;            ///< A conversion factor from degrees to radians.
    static const T RADTODEG;            ///< A conversion factor from radians to degrees.
    static const T TOLERANCE;           ///< A tolerance for differences in floating point values to be still considered equal.

  private:
    ///// private member variables
    T xCoord, yCoord, zCoord;           ///< Coordinate values.
    unsigned int ID;                    ///< ID of the point.

};

///////////////////////////////////////////////////////////////////////////////
///// Public Member Functions                                             /////
///////////////////////////////////////////////////////////////////////////////

///// Constructor /////////////////////////////////////////////////////////////
template <class T> Point3D<T>::Point3D()
/// The default constructor.
{
  setValues(0, 0, 0);
}

///// Constructor (overloaded) ////////////////////////////////////////////////
template <class T> Point3D<T>::Point3D(const T x, const T y, const T z)
/// \overload
{
  setValues(x, y, z);
}

///// Copy Constructor ////////////////////////////////////////////////////////
template <class T> Point3D<T>::Point3D(const Point3D& p)
/// The copy constructor.
{
  xCoord = p.xCoord;
  yCoord = p.yCoord;
  zCoord = p.zCoord;
  ID = p.ID;
}

///// Destructor //////////////////////////////////////////////////////////////
template <class T> Point3D<T>::~Point3D()
/// The default destructor
{

}

///// x ///////////////////////////////////////////////////////////////////////
template <class T> T Point3D<T>::x() const
/// Returns the x-coordinate.
{
  return xCoord;
}

///// y ///////////////////////////////////////////////////////////////////////
template <class T> T Point3D<T>::y() const
/// Returns the y-coordinate.
{
  return yCoord;
}

///// z ///////////////////////////////////////////////////////////////////////
template <class T> T Point3D<T>::z() const
/// Returns the z-coordinate.
{
  return zCoord;
}

///// id //////////////////////////////////////////////////////////////////////
template <class T> unsigned int Point3D<T>::id() const
/// Returns the ID.
{
  return ID;
}

///// setValues ///////////////////////////////////////////////////////////////
template <class T> void Point3D<T>::setValues(const T x, const T y, const T z)
/// Sets the values of all coordinates. By default this is cartesian input.
{
  xCoord = x;
  yCoord = y;
  zCoord = z;
}

///// setCartesian ////////////////////////////////////////////////////////////
template <class T> void Point3D<T>::setCartesian(const T x, const T y, const T z)
/// Sets the values of all coordinates from cartesian input.
{
  setValues(x, y, z);
}

///// setPolar ////////////////////////////////////////////////////////////////
template <class T> void Point3D<T>::setPolar(const T theta, const T phi, const T r)
/// Sets the values of all coordinates from spherical polar input.
/// The angles are assumed to be in degrees.
{
  setValues(r*sin(theta*DEGTORAD)*cos(phi*DEGTORAD), r*sin(theta*DEGTORAD)*sin(phi*DEGTORAD), r*cos(theta*DEGTORAD));
}

///// setID ///////////////////////////////////////////////////////////////////
template <class T> void Point3D<T>::setID(const unsigned int id)
/// Sets the ID.
{
  ID = id;
}

///// add /////////////////////////////////////////////////////////////////////
template <class T> void Point3D<T>::add(const Point3D& p)
/// Adds the point \c p.
{
  xCoord += p.xCoord;
  yCoord += p.yCoord;
  zCoord += p.zCoord;
}

#endif

