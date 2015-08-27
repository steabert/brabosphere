/***************************************************************************
                      glbaseparameters.h  -  description
                             -------------------
    begin                : Sat Mar 26 2005
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
/// Contains the declaration of the struct GLBaseParameters

#ifndef GLBASEPARAMETERS_H
#define GLBASEPARAMETERS_H

////// Forward class declarations /////////////////////////////////////////////

// Qt forward class declarations
//class QColor;

// Qt header files
#include <qgl.h>

///// struct GLBaseParameters /////////////////////////////////////////////////
struct GLBaseParameters
/// A struct containing all the basic OpenGL parameters pertaining the lighting,
/// quality, material and other properties.
{
  GLfloat lightPositionX; ///< X-position of the light
  GLfloat lightPositionY; ///< Y-position of the light
  GLfloat lightPositionZ; ///< Z-position of the light
  //change needed for compilation under MSVC .NET 2002 (should be fixed in .NET 2003)
  //QColor  lightColor;
  unsigned int lightColor; ///< The color of the light
  GLfloat materialSpecular; ///< The specular property of all materials
  GLfloat materialShininess; ///< The shininess of all materials
  //QColor  backgroundColor;
  unsigned int backgroundColor; ///< The background color
  bool    antialias; ///< Antialiasing property
  bool    smoothShading; ///< Smoothshading property
  bool    depthCue; ///< Depthcueing property
  bool    perspectiveProjection; ///< Perspective vs orthogonal projection
};

#endif

