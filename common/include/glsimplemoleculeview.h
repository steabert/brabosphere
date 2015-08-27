/***************************************************************************
                    glsimplemoleculeview.h  -  description
                             -------------------
    begin                : Fri Nov 5 2004
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
/// Contains the declaration of the class GLSimpleMoleculeView.

#ifndef GLSIMPLEMOLECULEVIEW_H
#define GLSIMPLEMOLECULEVIEW_H

///// Forward class declarations & header files ///////////////////////////////

// STL includes
#include <vector>

// Qt forward class declarations
class QDomElement;
#include <qfont.h>

// Xbrabo forward class declarations
class AtomSet;

// Xbrabo header files
#include "glmoleculeparameters.h"
#include "glview.h"

class GLSimpleMoleculeView : public GLView
{
  Q_OBJECT
  
  public: 
    ////// constructor/destructor
    GLSimpleMoleculeView(AtomSet* atomset, QWidget* parent=0, const char* name=0);      // constructor
    ~GLSimpleMoleculeView();            // destructor

    ///// public enums
    enum DisplayStyle{None, Lines, Tubes, BallAndStick, VanDerWaals};   ///< The rendering styles for the molecule and the forces.
                                                                        ///< The forces can only be displayed in one of the first 3 styles
    enum DisplaySource{Molecule, Forces};         ///< The types of primitives that can have different display styles

    ///// public member functions
    unsigned int displayStyle(const DisplaySource source) const;      // Returns the display style of a certain primitive
    bool isShowingElements() const;     // returns whether the atom elements are shown
    bool isShowingNumbers() const;      // returns whether the atom numbers are shown
    bool isShowingCharges(const unsigned int type) const;   // returns whether charges of a certain type are showing
    unsigned int selectedAtoms() const; // returns the number of atoms selected
    void loadCML(QDomElement* root);    // loads the setup of the view
    void saveCML(QDomElement* root);    // saves the setup of the view
    void setDisplayStyle(const DisplaySource source, const unsigned int style); // sets the display style for a certain primitive
    void setLabels(const bool element, const bool number, const unsigned int type);       // sets up showing of the labels

    ///// static public member functions
    static void setParameters(GLMoleculeParameters params); // sets new OpenGL parameters

  public slots:
    void updateAtomSet(const bool reset = false); // updates the view when the atomset has changed
    void selectAll(const bool update = true);     // select all atoms
    void unselectAll(const bool update = true);   // unselect all atoms

  protected slots:
    void reorderShapes();               // orders the shapes based on opacity

  protected:
    ///// protected enums
    enum SelectionType{SELECTION_NONE, SELECTION_ATOM, SELECTION_BOND, SELECTION_ANGLE, SELECTION_TORSION, SELECTION_GROUP,
                       SELECTION_BONDS, SELECTION_FORCES}; ///< Selection indices for OpenGL selection.

    enum ShapeTypes{SHAPE_ATOMS, SHAPE_BONDS, SHAPE_FORCES, SHAPE_LABELS, SHAPE_IC, SHAPE_SELECTION, SHAPE_NEXT};     ///< The different shapes that can be drawn by this class. Always keep SHAPE_NEXT as the last entry.

    ///// protected member functions
    virtual void keyPressEvent(QKeyEvent* e);     // handles key presses
    virtual void initializeGL();        // called once upon initialization
    virtual void drawItem(const unsigned int);    // draws the item shapes[i]
    virtual void updateGLSettings();    // updates the GL View according to parameters
    virtual float boundingSphereRadius();         // calculates the radius of the bounding sphere
    void clicked(const QPoint& position);         // handles click events
    virtual void updateShapes();        // updates the shapes vector

    ///// protected structs
    struct ShapeProperties             
    /// Utility struct for sorting of shapes by decreasing opacity.
    {
      unsigned int id;                  ///< A unique identifier for the shape.
      unsigned int opacity;             ///< The opacity of the shape.
      unsigned int type;                ///< The type of shape.
      bool operator<(ShapeProperties prop) const
      ///< Comparison operator. The \c const is needed for GCC
      {
        return opacity > prop.opacity;
      }
    };  

    ///// protected member data
    GLfloat centerX;                    ///< X-offset needed to center the molecule.
    GLfloat centerY;                    ///< Y-offset needed to center the moelcule.
    GLfloat centerZ;                    ///< Z-offset needed to center the molecule.
    unsigned int selectionType;         ///< Keeps track of the type of selection.
    std::list<unsigned int> selectionList;        ///< List that holds the selected atoms and their order.
    std::vector<ShapeProperties> shapes;///< the list of shapes ordered by opacity.

    ///// static protected member data
    static GLMoleculeParameters moleculeParameters;         ///< Holds the OpenGL molecule parameters.

  private:
    ///// private enums
    enum Directions{DIRECTION_X, DIRECTION_Y, DIRECTION_Z}; ///< The different directions for translations and rotations
    enum StartIndices{START_ATOMS = 100, START_BONDS = 1, START_FORCES = 2, START_SELECTEDATOMS = 3, START_SELECTEDBONDS = 4}; ///< Indices for the selection of entities
 
    ///// private member functions
    GLuint makeObjects(const int numSlices);      // generates the atom and bond shapes
    void changeObjects(const GLuint startList, const int numSlices);  // changes the atom and bond shapes
    void selectEntity(const QPoint position);     // selects the entity at the position
    void processSelection(const unsigned int id); // updates the selection according to the change in selection of the ID
    void centerMolecule();              // calculates the translations needed to have the molecule centered
    void drawScene();                   // does the actual repainting of the OpenGL scene
    void drawAtoms();                   // draws the atoms in the OpenGL scene
    void drawBonds();                   // draws the bonds
    void drawLabels();                  // draws the element names&numbers and possibly charges
    void drawForces();                  // draws the forces
    void drawICValue();                 // draws the value of the currently selected internal coordinate
    void drawSelections();              // draws the selected atoms and internal coordinates

    ///// private member data   
    int atomObject;                     ///< The OpenGL atom shape object pointer.
    int bondObject;                     ///< The OpenGL bond shape object pointer.
    int forceObjectLines;               ///< The OpenGL force shape object pointer for lines style.
    int forceObjectTubes;               ///< The OpenGL force shape object pointer for tubes style.
    unsigned int moleculeStyle;         ///< The rendering style of the molecule
    unsigned int forcesStyle;           ///< The rendering style of the forces
    bool showElements;                  ///< Is true if elements should be shown.
    bool showNumbers;                   ///< Is true if atom numbers should be shown
    unsigned int chargeType;            ///< Determines the type of charges to be shown
    AtomSet* atoms;                     ///< The list of atoms.
    GLfloat selectionLineWidth;         ///< The linewidth for drawing selected bonds. 
    GLfloat selectionPointSize;         ///< The pointsize for drawing selected atoms.
    float scaleFactor;                  ///< scalefactor for scenes exceeding 50A in radius
    QFont labelFont;                    ///< The font used to render labels and other values

    // private constants (made static for ease) 
    static const float cylinderHeight;  ///< The cylinder height. A too low value shows severe bugs in the Mesa OpenGL implementation.

};

#endif

