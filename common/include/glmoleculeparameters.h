/***************************************************************************
                    glmoleculeparameters.h  -  description
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
/// Contains the declaration of the struct GLMoleculeParameters

#ifndef GLMOLECULEPARAMETERS_H
#define GLMOLECULEPARAMETERS_H

////// Forward class declarations /////////////////////////////////////////////

// Qt header files
#include <qgl.h>

///// struct GLMoleculeParameters /////////////////////////////////////////////
struct GLMoleculeParameters
/// Struct containing OpenGL parameters specific to the visualisation of molecules.
{
  int quality;                          ///< The rendering quality of spheres (atoms) and cylinders (bonds) = the number of slices
  GLfloat sizeLines;                    ///< Thickness for line-type bonds
  GLfloat sizeBonds;                    ///< The bond size for cylinder-type bonds 
  GLfloat sizeForces;                   ///< The size for cylinder-type forces 
  unsigned int defaultMoleculeStyle;    ///< The default molecule display style
  unsigned int defaultForcesStyle;      ///< The default forces display style
  unsigned int fastRenderLimit;         ///< The number of atoms above which to switch to fast rendering (lines and no labels)
  bool showElements;                    ///< Whether to show the element labels by default
  bool showNumbers;                     ///< Whether to show the number labels by default
  unsigned int colorLabels;             ///< The color for rendering the textlabels
  unsigned int colorICs;                ///< The color for rendering the values of the internal coordinates
  unsigned int colorSelections;         ///< The color for rendering the selections
  unsigned int opacitySelections;       ///< The opacity of the selection color (0-100)
  unsigned int colorForces;             ///< The color for rendering the forces
  bool forcesOneColor;                  ///< Whether to render the forces in one color or in the atom's color
  unsigned int opacityForces;           ///< The opacity of the color of the forces (0-100)
};

#endif

