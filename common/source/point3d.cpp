/***************************************************************************
                          point3d.cpp  -  description
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
/*

As this is a template class, the implementation is added to the header file.
Here only the static variables are initialized.

*/
/// \file
/// Contains the static variables initialisation of the template class Point3D.

#include "point3d.h"

// This is a specialized template class member function
// It only works with GCC and VC++ 7.1 and higher
// -> VC++ needs this routine in the header file, but GCC is fine with
// this file. Check whether GCC allows it in the header file too (probably).
/*#ifdef USE_PARTIAL_SPECIALIZATION
///// operator== (overloaded) //////////////////////////////////////////////////
template <> bool Point3D<unsigned int>::operator==(const Point3D& p) const
/// Returns whether Point3D \c p equals this point. Specialized version for integers.
{  
  return xCoord == p.xCoord && yCoord == p.yCoord && zCoord == p.zCoord;
}
#endif
*/


///////////////////////////////////////////////////////////////////////////////
///// Static Variables                                                    /////
///////////////////////////////////////////////////////////////////////////////
// partial specialisation to eliminate Visual C++ complaints about implicit conversions
template <> const float Point3D<float>::PI = 3.14159265358979323846f;
template <> const float Point3D<float>::DEGTORAD = PI/180.0f;
template <> const float Point3D<float>::RADTODEG = 180.0f/PI;
template <> const float Point3D<float>::TOLERANCE = 0.0005f;

template <> const double Point3D<double>::PI = 3.14159265358979323846;
template <> const double Point3D<double>::DEGTORAD = PI/180.0;
template <> const double Point3D<double>::RADTODEG = 180.0/PI;
template <> const double Point3D<double>::TOLERANCE = 0.0005;

// yes, I'm certainly misusing this class...
template <> const unsigned int Point3D<unsigned int>::PI = 1;
template <> const unsigned int Point3D<unsigned int>::DEGTORAD = 1;
template <> const unsigned int Point3D<unsigned int>::RADTODEG = 1;
template <> const unsigned int Point3D<unsigned int>::TOLERANCE = 0;

