/***************************************************************************
                       glmoleculeview.h  -  description
                             -------------------
    begin                : Mon Jul 29 2002
    copyright            : (C) 2002-2006 by Ben Swerts
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
/// Contains the declaration of the class GLMoleculeView

#ifndef GLMOLECULEVIEW_H
#define GLMOLECULEVIEW_H

///// Forward class declarations & header files ///////////////////////////////

// STL includes
#include <list>
#include <vector>

// Qt forward class declarations
//class QDomDocument;
//class QDomElement;

// Xbrabo forward class declarations
class AtomSet;
class IsoSurface;
class DensityBase;
class NewAtomBase;

// Base class header file
#include <glsimplemoleculeview.h>

///// class GLMoleculeView ////////////////////////////////////////////////////
class GLMoleculeView : public GLSimpleMoleculeView
{
  Q_OBJECT

  public:
    GLMoleculeView(AtomSet* atomset, QWidget* parent = 0, const char* name = 0);// constructor
    ~GLMoleculeView();                  // destructor

  signals:
    void atomsetChanged();              ///< Is emitted when the number of atoms has been changed

  public slots:
    void alterCartesian();              // alters the cartesian coordinates of the selected atoms
    void alterInternal();               // alters the value of the selected internal coordinate
	  void showDensity();                 // shows electron density isosurfaces 
    void addAtoms();                    // adds atoms using a dialog
    void deleteSelectedAtoms();         // deletes all selected atoms
    void toggleSelection();             // toggles the manipulation target

  protected:
    void mouseMoveEvent(QMouseEvent* e);// event which takes place when the mouse is moved while a mousebutton is pressed
    void keyPressEvent(QKeyEvent* e);   // event which takes places when a key is pressed
    //void wheelEvent(QWheelEvent* e);    // event which takes place when the scrollwheel of the mouse is used
    //void translateZ(const int amount);  // handles Z-direction translations
    //void translateXY(const int amountX, const int amountY); // handles X- and Y-direction translations
    virtual void updateShapes();        // updates the shapes vector

private slots:
    void addGLSurface(const unsigned int index);  // adds a surface to the GL display list
    void updateGLSurface(const unsigned int index);         // updates an existing surface GL display list 
    void deleteGLSurface(const unsigned int index);         // deletes an existing surface GL display list

  private:
    ///// private enums
    enum ShapeTypesExtra{SHAPE_SURFACE = SHAPE_NEXT};

    ///// private member functions
    float boundingSphereRadius();      // calculates the radius of the bounding sphere
    void translateSelection(const int xRange, const int yRange, const int zRange);        // translates the selected atoms according to the current view
    void rotateSelection(const double angleX, const double angleY, const double angleZ);  // rotates the selected atoms around their local center of mass
    void changeSelectedIC(const int range);       // changes the selected internal coordinate
    void drawItem(const unsigned int index);    // draws the item shapes[index]
    
    ///// private member data   
    AtomSet* atoms;                     ///< The list of atoms.
    IsoSurface* isoSurface;             ///< An isodensity surface.
    DensityBase* densityDialog;         ///< A dialog for changing the isodensity surfaces.
    NewAtomBase* newAtomDialog;         ///< A dialog for adding atoms to the atomset
    std::vector<GLuint> glSurfaces;     ///< A vector that holds the GL display list indices for surfaces.
    bool manipulateSelection;           ///< If true, only the selected atoms are manipulated instead of the entire system.
};
   
#endif

