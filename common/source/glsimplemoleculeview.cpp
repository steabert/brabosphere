/***************************************************************************
                  glsimplemoleculeview.cpp  -  description
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

///// Comments ////////////////////////////////////////////////////////////////
/*!
  \class GLSimpleMoleculeView
  \brief This class shows a molecule in 3D using OpenGL.

  It does not allow chaning of the molecule and is used in the program CrdView.
  Brabosphere uses the derived class GLMoleculeView with more features.

*/
/// \file
/// Contains the implementation of the class GLSimpleMoleculeView

///// Header files ////////////////////////////////////////////////////////////

// C++ header files
#include <cassert>
#include <cmath>

// STL header files
#include <algorithm>

// Qt header files
#include <qapplication.h>
#include <qdom.h>
#include <qfiledialog.h>
#include <qmessagebox.h>
#include <qpoint.h>
#include <qstringlist.h>

// Xbrabo header files
#include "atomset.h"
#include "domutils.h"
#include "glsimplemoleculeview.h"
#include "point3d.h"
#include "vector3d.h"

///////////////////////////////////////////////////////////////////////////////
///// Public Member Functions                                             /////
///////////////////////////////////////////////////////////////////////////////

///// constructor /////////////////////////////////////////////////////////////
GLSimpleMoleculeView::GLSimpleMoleculeView(AtomSet* atomset, QWidget* parent, const char* name ) : GLView(parent, name),
  chargeType(AtomSet::None),
  atoms(atomset),
  scaleFactor(1.0f)
/// The default constructor.
{
  moleculeStyle = moleculeParameters.defaultMoleculeStyle;
  forcesStyle = moleculeParameters.defaultForcesStyle;
  showElements = moleculeParameters.showElements;
  showNumbers = moleculeParameters.showNumbers;
  assert(atomset != 0);
  centerMolecule(); // just sets centerX|Y|Z to zero when no atoms are present
  reorderShapes();
  labelFont = QApplication::font();
#ifndef WIN32
  // Linux needs a non TTF font like Times (safest option)
  //labelFont = QFont("Times");
  // less safe option (not very well tested): use a bitmap font (works with MESA GL)
  labelFont.setStyleHint(QFont::AnyStyle, QFont::PreferBitmap); // uses a bitmap font
  labelFont.setPointSize(QApplication::font().pointSize());
  //labelFont.setStyleStrategy(QFont::OpenGLCompatible); // doesn't work on SuSE 9.2 64bit thru Cygwin/X and not supported on Qt 3.1.2
#endif
}

///// destructor //////////////////////////////////////////////////////////////
GLSimpleMoleculeView::~GLSimpleMoleculeView()
/// The default destructor.
{
  makeCurrent();
  glDeleteLists(atomObject, 4);
}

///// displayStyle ////////////////////////////////////////////////////////////
unsigned int GLSimpleMoleculeView::displayStyle(const DisplaySource source) const
/// Returns the display style of a certain primitive.
{
  if(source == Molecule)
    return moleculeStyle;
  else
    return forcesStyle;
}

///// isShowingElements ///////////////////////////////////////////////////////
bool GLSimpleMoleculeView::isShowingElements() const
/// Returns whether the atom elements are shown.
{
  return showElements;
}

///// isShowingNumbers ////////////////////////////////////////////////////////
bool GLSimpleMoleculeView::isShowingNumbers() const
/// Returns whether the atom numbers are shown.
{
  return showNumbers;
}

///// isShowingCharges ////////////////////////////////////////////////////////
bool GLSimpleMoleculeView::isShowingCharges(const unsigned int type) const
/// Returns whether the atomic charges of the specified type are shown.
{
  return type == chargeType;
}

///// selectedAtoms ///////////////////////////////////////////////////////////
unsigned int GLSimpleMoleculeView::selectedAtoms() const
/// Returns the number of selected atoms.
{
  return selectionList.size();
}
///// loadCML /////////////////////////////////////////////////////////////////
void GLSimpleMoleculeView::loadCML(QDomElement* root)
/// Loads the view settings from a QDomElement.
{
  float wQuat = 0.0f, xQuat = 0.0f, yQuat = 0.0f, zQuat = 0.0f;
  QDomNode childNode = root->firstChild();
  const QString prefix = "view_";
  while(!childNode.isNull())
  {
    if(childNode.isElement() && childNode.nodeName() == "parameter")
    {
      if(DomUtils::dictEntry(childNode, prefix + "orientation-w"))
        DomUtils::readNode(&childNode, &wQuat);
      else if(DomUtils::dictEntry(childNode, prefix + "orientation-x"))
        DomUtils::readNode(&childNode, &xQuat);
      else if(DomUtils::dictEntry(childNode, prefix + "orientation-y"))
        DomUtils::readNode(&childNode, &yQuat);
      else if(DomUtils::dictEntry(childNode, prefix + "orientation-z"))
        DomUtils::readNode(&childNode, &zQuat);
      else if(DomUtils::dictEntry(childNode, prefix + "position-x"))
        DomUtils::readNode(&childNode, &xPos);
      else if(DomUtils::dictEntry(childNode, prefix + "position-y"))
        DomUtils::readNode(&childNode, &yPos);
      else if(DomUtils::dictEntry(childNode, prefix + "position-z"))
        DomUtils::readNode(&childNode, &zPos);
      else if(DomUtils::dictEntry(childNode, prefix + "center-x"))
        DomUtils::readNode(&childNode, &centerX);
      else if(DomUtils::dictEntry(childNode, prefix + "center-y"))
        DomUtils::readNode(&childNode, &centerY);
      else if(DomUtils::dictEntry(childNode, prefix + "center-z"))
        DomUtils::readNode(&childNode, &centerZ);
      else if(DomUtils::dictEntry(childNode, prefix + "style_molecule"))
        DomUtils::readNode(&childNode, &moleculeStyle);
      else if(DomUtils::dictEntry(childNode, prefix + "style_forces"))
        DomUtils::readNode(&childNode, &forcesStyle);
      else if(DomUtils::dictEntry(childNode, prefix + "show_elements"))
        DomUtils::readNode(&childNode, &showElements);
      else if(DomUtils::dictEntry(childNode, prefix + "show_numbers"))
        DomUtils::readNode(&childNode, &showNumbers);
      else if(DomUtils::dictEntry(childNode, prefix + "show_charges_type"))
        DomUtils::readNode(&childNode, &chargeType);
    }
    childNode = childNode.nextSibling();
  }
  orientationQuaternion->setValues(wQuat, xQuat, yQuat, zQuat);

  makeCurrent(); // needed for call to updateFog
  updateFog(boundingSphereRadius());
  updateGL();
}

///// saveCML /////////////////////////////////////////////////////////////////
void GLSimpleMoleculeView::saveCML(QDomElement* root)
/// Saves the view settings to a QDomElement.
{
  const QString prefix = "view_";
  DomUtils::makeNode(root, orientationQuaternion->w(), prefix + "orientation-w");
  DomUtils::makeNode(root, orientationQuaternion->x(), prefix + "orientation-x");
  DomUtils::makeNode(root, orientationQuaternion->y(), prefix + "orientation-y");
  DomUtils::makeNode(root, orientationQuaternion->z(), prefix + "orientation-z");
  DomUtils::makeNode(root, static_cast<float>(xPos), prefix + "position-x");
  DomUtils::makeNode(root, static_cast<float>(yPos), prefix + "position-y");
  DomUtils::makeNode(root, static_cast<float>(zPos), prefix + "position-z");
  DomUtils::makeNode(root, static_cast<float>(centerX), prefix + "center-x");
  DomUtils::makeNode(root, static_cast<float>(centerY), prefix + "center-y");
  DomUtils::makeNode(root, static_cast<float>(centerZ), prefix + "center-z");
  DomUtils::makeNode(root, moleculeStyle, prefix + "style_molecule");
  DomUtils::makeNode(root, forcesStyle, prefix + "style_forces");
  DomUtils::makeNode(root, showElements, prefix + "show_elements");
  DomUtils::makeNode(root, showNumbers, prefix + "show_numbers");
  DomUtils::makeNode(root, chargeType, prefix + "show_charges_type");
}

///// setDisplayStyle /////////////////////////////////////////////////////////
void GLSimpleMoleculeView::setDisplayStyle(const DisplaySource source, const unsigned int style)
/// Sets the display style for a certain primitive. Does not redraw the scene.
{
  if(source == Molecule)
  {
    if(style > VanDerWaals)
      moleculeStyle = BallAndStick;
    else
      moleculeStyle = style;
  }
  else
  {
    if(style > Tubes)
      forcesStyle = Tubes;
    else
      forcesStyle = style;
  }
  setModified();
}

///// setLabels ///////////////////////////////////////////////////////////////
void GLSimpleMoleculeView::setLabels(const bool element, const bool number, const unsigned int type)
/// Sets up showing of the atom labels. Does not redraw the scene.
{
  showElements = element;
  showNumbers = number;
  if(type > AtomSet::Stockholder)
    chargeType = AtomSet::None;
  else
    chargeType = type;
  setModified();
}

///// setParameters ///////////////////////////////////////////////////////////
void GLSimpleMoleculeView::setParameters(GLMoleculeParameters params)
/// Updates the OpenGL parameters and
/// makes sure they are adapted at the next call to updateGL()
{
  moleculeParameters = params;
  //staticUpdateIndex++; //assume a call to GLView::setParameters has also been done
}

///////////////////////////////////////////////////////////////////////////////
///// Public Slots                                                        /////
///////////////////////////////////////////////////////////////////////////////

///// updateAtomSet ///////////////////////////////////////////////////////////
void GLSimpleMoleculeView::updateAtomSet(const bool reset)
/// Updates the view under influence of changes in the atomset.
/// It has to be called under the following conditions:
/// \arg a new coordinate set has been read
/// \arg atoms have been added/removed/changes
/// When reset = true, the scene will be reset completely
{
  centerMolecule(); // always keep the molecule centered
  if(reset)
  {
    resetView(false);
    unselectAll(false);


    /*qDebug("selecting all atoms 9+ Angstrom away from atom 16");
    unsigned int center = 15;
    double distance = 9.0;
    distance *= distance;
    for(unsigned int i = 0; i < atoms->count(); i++)
    {
      if(  (atoms->x(i) - atoms->x(center))*(atoms->x(i) - atoms->x(center))
         + (atoms->y(i) - atoms->y(center))*(atoms->y(i) - atoms->y(center))
         + (atoms->z(i) - atoms->z(center))*(atoms->z(i) - atoms->z(center)) >= distance)
        selectionList.push_back(i);
    }
    */
    /*qDebug("selecting first 32 atoms");
    if(atoms->count() >= 32)
    {
      for(unsigned int i = 0; i < 32; i++)
        selectionList.push_back(i);
    }
    */
    switch(selectionList.size())
    {
      case 0: selectionType = SELECTION_NONE;
              break;
      case 1: selectionType = SELECTION_ATOM;
              break;
      case 2: selectionType = SELECTION_BOND;
              break;
      case 3: selectionType = SELECTION_ANGLE;
              break;
      case 4: selectionType = SELECTION_TORSION;
              break;
      default: selectionType = SELECTION_GROUP;
    }

  }
  updateGL();
}

///// selectAll ///////////////////////////////////////////////////////////////
void GLSimpleMoleculeView::selectAll(const bool update)
/// Selects all atoms.
{
  unselectAll(false);
  for(unsigned int i = 0; i < atoms->count(); i++)
    selectionList.push_back(i);
  switch(selectionList.size())
  {
    case 0: selectionType = SELECTION_NONE;
            break;
    case 1: selectionType = SELECTION_ATOM;
            break;
    case 2: selectionType = SELECTION_BOND;
            break;
    case 3: selectionType = SELECTION_ANGLE;
            break;
    case 4: selectionType = SELECTION_TORSION;
            break;
    default: selectionType = SELECTION_GROUP;
  }
  if(update)
    updateGL();
  emit changed();
}

///// unselectAll /////////////////////////////////////////////////////////////
void GLSimpleMoleculeView::unselectAll(const bool update)
/// Unselects all atoms.
{
  selectionList.clear();
  selectionType = SELECTION_NONE;

  if(update)
    updateGL();
  emit changed();
}

///////////////////////////////////////////////////////////////////////////////
///// Protected Slots                                                     /////
///////////////////////////////////////////////////////////////////////////////

///// reorderShapes ///////////////////////////////////////////////////////////
void GLSimpleMoleculeView::reorderShapes()
/// Orders all drawn shapes according to their opacity.
{
  updateShapes();
  std::sort(shapes.begin(), shapes.end());
}

///////////////////////////////////////////////////////////////////////////////
///// Protected Member Functions                                          /////
///////////////////////////////////////////////////////////////////////////////

///// keyPressEvent ///////////////////////////////////////////////////////////
void GLSimpleMoleculeView::keyPressEvent(QKeyEvent* e)
/// Overridden from GLView::keyPressEvent. Handles key presses for font changes
/// (no public interface yet)
{
  if(e->state() & Qt::ControlButton && (e->key() == Qt::Key_Plus || e->key() == Qt::Key_1))
  {
    labelFont.setPointSize(labelFont.pointSize() + 1);
    qDebug("increasing font size by 1");
    updateGL();
  }
  else if(e->state() & Qt::ControlButton && (e->key() == Qt::Key_Minus || e->key() == Qt::Key_2))
  {
    labelFont.setPointSize(labelFont.pointSize() - 1);
    qDebug("decreasing font size by 1");
    updateGL();
  }
  else
    GLView::keyPressEvent(e);
}

///// initializeGL ////////////////////////////////////////////////////////////
void GLSimpleMoleculeView::initializeGL()
/// Overridden from QGLWidget::initializeGL(). Initializes the OpenGL window.
{
  int numSlices = static_cast<int>(pow(2.0,static_cast<double>(moleculeParameters.quality)));
  atomObject = makeObjects(numSlices);
  bondObject = atomObject + 1;
  forceObjectLines = atomObject + 2;
  forceObjectTubes = atomObject + 3;
  updateGLSettings();

  GLView::initializeGL();
}

///// boundingSphereRadius ////////////////////////////////////////////////////
float GLSimpleMoleculeView::boundingSphereRadius()
/// Calculates the radius of the bounding sphere.
{
  float radius = 0.0;
  float x, y, z, tempradius;
  for(unsigned int i = 0; i < atoms->count(); i++)
  {
    x = static_cast<float>(atoms->x(i) - centerX);
    y = static_cast<float>(atoms->y(i) - centerY);
    z = static_cast<float>(atoms->z(i) - centerZ);
    ///// the following might have to be changed when scaling of atomsizes is permitted
    tempradius = sqrt(x*x + y*y + z*z) + static_cast<float>(AtomSet::vanderWaals(atoms->atomicNumber(i)))/2.0f;
    if(tempradius > radius)
      radius = tempradius;
  }
  if(radius > 25.0f)
  {
    scaleFactor = 25.0f/radius;
    radius = 25.0f;
  }
  else
    scaleFactor = 1.0f;
  if(radius < 0.4f) // VdW(H)
    radius = 0.4f;
  return radius;
}

///// drawItem ///////////////////////////////////////////////////////////////
void GLSimpleMoleculeView::drawItem(const unsigned int)
/// Draws the item shapes[index]. It should be overridden by a subclass to draw
/// new shapes. It is not made pure virtual because implementation is not
/// mandatory.
{

}

///// clicked /////////////////////////////////////////////////////////////////
void GLSimpleMoleculeView::clicked(const QPoint& position)
/// Implementation of GLView::clicked(). Handles mouse click events.
{
  selectEntity(position);
}

///// updateGLSettings ////////////////////////////////////////////////////////
void GLSimpleMoleculeView::updateGLSettings()
/// Updates the OpenGL settings.
/// It is only called when an update is really needed (determined by
/// staticUpdateIndex != updateIndex in GLView).
{
  GLView::updateGLSettings(); // update the settings of the base class

  ///// update the local OpenGL settings
  ///// atom and bond quality
  //int numSlices = static_cast<int>(pow(2.0,static_cast<double>(moleculeParameters.quality)));
  changeObjects(atomObject, moleculeParameters.quality);

  ///// linewidths and pointsizes for selections in None or Lines mode
  ///// get the maximum linewidth and pointsize
  //makeCurrent();
  GLfloat lwRange[] = {0.0f, 0.0f};
  glGetFloatv(GL_LINE_WIDTH_RANGE, lwRange);
  GLfloat psRange[] = {0.0f, 0.0f};
  glGetFloatv(GL_POINT_SIZE_RANGE, psRange);
  ///// set the linewidth for selections
  selectionLineWidth = moleculeParameters.sizeLines * 3.0f;
  if(selectionLineWidth < 3.0f)
    selectionLineWidth = 3.0f;
  if(selectionLineWidth > lwRange[1])
    selectionLineWidth = lwRange[1];
   ///// set the point size for selections
  selectionPointSize = moleculeParameters.sizeLines * 5.0f;
  if(selectionPointSize < 5.0f)
    selectionPointSize = 5.0f;
  if(selectionPointSize > psRange[1])
    selectionPointSize = psRange[1];

  ///// update the regular parameters
  glLineWidth(moleculeParameters.sizeLines);

  ///// transparency settings might have changed
  reorderShapes();
}

///// updateShapes ///////////////////////////////////////////////////////////
void GLSimpleMoleculeView::updateShapes()
/// Updates the contents of the shapes vector.
{
  shapes.clear();
  ShapeProperties prop;

  // atoms
  prop.id = 0; // not used
  prop.opacity = 100; // always
  prop.type = SHAPE_ATOMS;
  shapes.push_back(prop);
  // bonds
  prop.id = 0; // not used
  prop.opacity = 100; // always
  prop.type = SHAPE_BONDS;
  shapes.push_back(prop);
  // forces
  prop.id = 0; // not used
  prop.opacity = moleculeParameters.opacityForces;
  prop.type = SHAPE_FORCES;
  shapes.push_back(prop);
  // labels
  prop.id = 0; // not used
  prop.opacity = 100; // always
  prop.type = SHAPE_LABELS;
  shapes.push_back(prop);
  // IC value
  prop.id = 0;
  prop.opacity = 100;
  prop.type = SHAPE_IC;
  shapes.push_back(prop);
  // selection
  prop.id = 0; // not used
  prop.opacity = moleculeParameters.opacitySelections;
  prop.type = SHAPE_SELECTION;
  shapes.push_back(prop);
}

///////////////////////////////////////////////////////////////////////////////
///// Private Member Functions                                            /////
///////////////////////////////////////////////////////////////////////////////

///// makeObjects /////////////////////////////////////////////////////////////
GLuint GLSimpleMoleculeView::makeObjects(const int numSlices)
/// Generates the shapes for the atoms and the bonds.
{
  GLuint startList = glGenLists(4);
  changeObjects(startList, numSlices);
  return startList;
}

///// changeObjects ///////////////////////////////////////////////////////////
void GLSimpleMoleculeView::changeObjects(const GLuint startList, const int numSlices)
/// Changes the quality of the atom and bond shapes.
{
  GLUquadricObj* qobj;
  qobj = gluNewQuadric();
  ///// Drawing style for quadrics
  //gluQuadricDrawStyle(qobj, GLU_POINT); // default = GLU_FILL
  gluQuadricNormals(qobj, GLU_SMOOTH);
  gluQuadricOrientation(qobj, GLU_OUTSIDE);

  ///// Atom //////////////////////////
  glNewList(startList, GL_COMPILE);
    gluSphere(qobj, 1.0f, numSlices, numSlices);
  glEndList();

  ///// first part of bond ////////////
  glNewList(startList + 1, GL_COMPILE);
    gluCylinder(qobj, 1.0f, 1.0f, cylinderHeight, numSlices, 1);
  glEndList();

  ///// force arrow in Lines style ////
  glNewList(startList + 2, GL_COMPILE);
    glBegin(GL_LINES);
      //glColor3f(1.0f, 1.0f, 0.0f);
      glVertex3f(0.0f, 0.0f, 0.0f);
      glVertex3f(0.0f, 0.0f, cylinderHeight);
      //glColor3f(0.5f, 0.5f, 0.0f);
      glVertex3f(0.0f, 0.0f, cylinderHeight);
      glVertex3f(-0.1f, -0.1f, 0.9f * cylinderHeight);
      glVertex3f(0.0f, 0.0f, cylinderHeight);
      glVertex3f(0.1f, 0.1f, 0.9f * cylinderHeight);
    glEnd();
  glEndList();

  ///// force arrow in Tubes style ////
  glNewList(startList + 3, GL_COMPILE);
    /*
    // draw the top in the atoms' color
    glTranslatef(0.0f, 0.0f, 0.9f*cylinderHeight);
    gluCylinder(qobj, 1.0f, 0.0f, 0.1f*cylinderHeight, numSlices, 1);
    gluQuadricOrientation(qobj, GLU_INSIDE);
    gluDisk(qobj, 0.0f, 1.0f, numSlices, 1);
    gluQuadricOrientation(qobj, GLU_OUTSIDE);
    // draw the rest in yellow
    qglColor(QColor(255, 255, 0));
    glTranslatef(0.0f, 0.0f, -0.9f*cylinderHeight);
    gluCylinder(qobj, 1.0f, 0.2f, 0.9f*cylinderHeight, numSlices, 1);
    */
    gluCylinder(qobj, 1.0f, 1.0f, cylinderHeight - 2.4f, numSlices, 1); // 2.4 = 2 * 1.2
    glTranslatef(0.0f, 0.0f, cylinderHeight - 2.4f);
    gluCylinder(qobj, 1.2f, 0.0f, 2.4f, numSlices, 1);
    gluQuadricOrientation(qobj, GLU_INSIDE);
    gluDisk(qobj, 0.0f, 1.2f, numSlices, 1);
  glEndList();

  gluDeleteQuadric(qobj);
}

///// selectEntity ////////////////////////////////////////////////////////////
void GLSimpleMoleculeView::selectEntity(const QPoint position)
/// Selects the entity (atom, bond, etc.) pointed to by the mouse position.
{
  makeCurrent();
  ///// needed storage
  const unsigned int BUFFER_SIZE = 64;
  GLuint selectionBuffer[BUFFER_SIZE]; // selection buffer
  GLuint* pBuffer = selectionBuffer;
  GLint viewport[4]; // viewport

  ///// get the selections
  glSelectBuffer(BUFFER_SIZE, pBuffer); // set up the selection buffer
  glGetIntegerv(GL_VIEWPORT, viewport); // get the viewport
  glMatrixMode(GL_PROJECTION); // set projection mode
  glPushMatrix(); // save the matrix
  glRenderMode(GL_SELECT); // set selection mode
  glLoadIdentity();
  GLint xPosition = position.x();
  GLint yPosition = viewport[3] - position.y();
  gluPickMatrix(xPosition, yPosition, 2, 2, viewport); // set up a viewport of 2 pixels wide around the mouse position
  setPerspective(); // calls gluPerspective or glOrtho depending on the prespective setting
  glMatrixMode(GL_MODELVIEW);

  if(moleculeStyle == None || moleculeStyle == Lines)
  {
    ///// temporarily change to a tubes representation to be able to get the selections
    unsigned int oldStyle = moleculeStyle;
    moleculeStyle = Tubes;
    updateGL();
    moleculeStyle = oldStyle;
  }
  else
    updateGL(); // just draw the scene

  ///// generate the selection
  GLint hits = glRenderMode(GL_RENDER);

  ///// restore the original view
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);

  ///// process the selection
  if(hits != 0)
  {
    pBuffer++; // GLuint count = *pBuffer++; // the number of names in the buffer
    pBuffer++; // GLuint minz = *pBuffer++; // minimum z-value of selection
    pBuffer++; // GLuint maxz = *pBuffer++; // maximum z-value of selection
    GLuint id = *pBuffer; // the id of the first selection
    //qDebug("Number of selections: %d", count);
    //qDebug("ID of first selection: %d", id);
    processSelection(id);
    updateGL();
  }
  emit changed();
}

///// processSelection ////////////////////////////////////////////////////////
void GLSimpleMoleculeView::processSelection(const unsigned int id)
/// Changes the selection according to the change in
/// selection of the entity with an ID id.
{
  switch(id)
  {
    case START_BONDS:  selectionList.clear();
                       selectionType = SELECTION_BONDS;
                       break;
    case START_FORCES: selectionList.clear();
                       selectionType = SELECTION_FORCES;
                       break;
    default: if(id >= START_ATOMS)
    {
      unsigned int selectedAtom = id - START_ATOMS;

      ///// check whether the atom is already selected
      std::list<unsigned int>::iterator it = std::find(selectionList.begin(), selectionList.end(), selectedAtom);
      if(it == selectionList.end())
      {
        ///// atom is not selected -> add it
        selectionList.push_back(selectedAtom);
      }
      else
      {
        ///// atom is selected -> remove it
        selectionList.erase(it);
      }

      ///// update the selection type
      switch(selectionList.size())
      {
        case 0: selectionType = SELECTION_NONE;
                break;
        case 1: selectionType = SELECTION_ATOM;
                break;
        case 2: selectionType = SELECTION_BOND;
                break;
        case 3: selectionType = SELECTION_ANGLE;
                break;
        case 4: selectionType = SELECTION_TORSION;
                break;
        default: selectionType = SELECTION_GROUP;
      }
    }
  }
}

///// centerMolecule //////////////////////////////////////////////////////////
void GLSimpleMoleculeView::centerMolecule()
/// Calculates the translations needed to center the molecule. The center of
/// mass cannot be used, just the largest extents of the molecule.
{
  centerX = 0.0f;
  centerY = 0.0f;
  centerZ = 0.0f;

  if(atoms->count() == 0)
    return;

  ///// determine maxima & minima
  double maxx = atoms->x(0);
  double maxy = atoms->y(0);
  double maxz = atoms->z(0);
  double minx = maxx;
  double miny = maxy;
  double minz = maxz;
  for(unsigned int i = 1; i < atoms->count(); i++)
  {
    if(atoms->x(i) > maxx)
      maxx = atoms->x(i);
    else if(atoms->x(i) < minx)
      minx = atoms->x(i);
    if(atoms->y(i) > maxy)
      maxy = atoms->y(i);
    else if(atoms->y(i) < miny)
      miny = atoms->y(i);
    if(atoms->z(i) > maxz)
      maxz = atoms->z(i);
    else if(atoms->z(i) < minz)
      minz = atoms->z(i);
  }
  ///// calculate the new centers
  centerX = static_cast<GLfloat>((maxx + minx)/2.0);
  centerY = static_cast<GLfloat>((maxy + miny)/2.0);
  centerZ = static_cast<GLfloat>((maxz + minz)/2.0);
}

///// drawScene ///////////////////////////////////////////////////////////////
void GLSimpleMoleculeView::drawScene()
/// Implementation of the pure virtual GLView::drawScene().
/// Changed from protected to private as subclasses should implement drawItem.
/// Does the drawing of the OpenGL scene (called from paintGL).
/// The default state is lighting enabled. If certain routine need to disable lighting
/// they are required to enable it again at the end.
{
  //qDebug("starting drawScene for atoms->count() = %d", atoms->count());
  if(scaleFactor < 1.0f)
    glScalef(scaleFactor, scaleFactor, scaleFactor);

  glTranslatef(-centerX, -centerY, -centerZ); // center the molecule

  glInitNames();
  glPushName(0);

  ///// switch to fast rendering if the number of atoms is too large
  unsigned int oldMoleculeStyle = moleculeStyle;
  unsigned int oldForcesStyle = forcesStyle;
  bool oldShowElements = showElements;
  bool oldShowNumbers = showNumbers;
  unsigned int oldChargeType = chargeType;
  if(atoms->count() > moleculeParameters.fastRenderLimit)
  {
    moleculeStyle = Lines;
    forcesStyle = None;
    showElements = false;
    showNumbers = false;
    chargeType = AtomSet::None;
  }

  bool usedBlending = false;
  ///// draw all shapes in order of decreasing opacity
  for(unsigned int i = 0; i < shapes.size(); i++)
  {
    ///// switch to blending for the first opacity < 100
    if(!usedBlending && shapes[i].opacity < 100)
    {
      usedBlending = true;
      glEnable(GL_BLEND);
    }
    switch(shapes[i].type)
    {
      case SHAPE_ATOMS:
        drawAtoms();
        break;
      case SHAPE_BONDS:
        drawBonds();
        break;
      case SHAPE_FORCES:
        drawForces();
        break;
      case SHAPE_LABELS:
        drawLabels();
        break;
      case SHAPE_IC:
        drawICValue();
        break;
      case SHAPE_SELECTION:
        drawSelections();
        break;
      default:
        //qDebug("about to call drawItem(%d)",i);
        drawItem(i);
    }
  }
  if(usedBlending)
    glDisable(GL_BLEND);

  ///// restore the old settings in case of fast rendering
  if(atoms->count() > moleculeParameters.fastRenderLimit)
  {
    moleculeStyle = oldMoleculeStyle;
    forcesStyle = oldForcesStyle;
    showElements = oldShowElements;
    showNumbers = oldShowNumbers;
    chargeType = oldChargeType;
  }
}

///// drawAtoms ///////////////////////////////////////////////////////////////
void GLSimpleMoleculeView::drawAtoms()
/// Draws the atoms in the OpenGL scene.
{
  //qDebug("calling drawAtoms");
  if(moleculeStyle == None || moleculeStyle == Lines)
    return;

  for(unsigned int i = 0; i < atoms->count(); i++)
  {
    glPushMatrix(); // save the current matrix
    qglColor(atoms->color(i)); // set the color (works cos of glColorMaterial)
    glTranslatef(atoms->x(i), atoms->y(i), atoms->z(i)); // set the position
    if(moleculeStyle == Tubes)
    {
      glScalef(moleculeParameters.sizeBonds,
               moleculeParameters.sizeBonds,
               moleculeParameters.sizeBonds);
    }
    else if(moleculeStyle == BallAndStick)
    {
      glScalef(AtomSet::vanderWaals(atoms->atomicNumber(i))/2.0f,
               AtomSet::vanderWaals(atoms->atomicNumber(i))/2.0f,
               AtomSet::vanderWaals(atoms->atomicNumber(i))/2.0f);
    }
    else if(moleculeStyle == VanDerWaals)
    {
      glScalef(AtomSet::vanderWaals(atoms->atomicNumber(i))*1.5f,
               AtomSet::vanderWaals(atoms->atomicNumber(i))*1.5f,
               AtomSet::vanderWaals(atoms->atomicNumber(i))*1.5f);
    }
    glLoadName(START_ATOMS+i);
    glCallList(atomObject); // make the atom
    glPopMatrix(); // restore the matrix
  }
  glLoadName(START_BONDS); // just to make sure the following items do not get the same name as the last atom
}

///// drawBonds ///////////////////////////////////////////////////////////////
void GLSimpleMoleculeView::drawBonds()
/// Draws the bonds in the OpenGL scene in a format depending in the OpenGL parameters.
/// if bonds don't need to be selected individually, all bonds can be stored
/// together in an OpenGL display list.
{
  //qDebug("calling drawBonds");
  if(moleculeStyle == None || moleculeStyle == VanDerWaals)
    return;

  float distance, distanceXY, x1, x2, y1, y2, z1, z2, phi, theta;
  vector<unsigned int>* firstAtom;
  vector<unsigned int>* secondAtom;
  atoms->bonds(firstAtom, secondAtom); // assigns both pointers

  if(moleculeStyle == Lines)
  {
    glLineWidth(moleculeParameters.sizeLines);
    glDisable(GL_LIGHTING);
    glBegin(GL_LINES);
      for(unsigned int i = 0; i < firstAtom->size(); i++)
      {
        const unsigned int atom1 = firstAtom->operator[](i);
        const unsigned int atom2 = secondAtom->operator[](i);
        if(atoms->color(atom1) == atoms->color(atom2))
        {
          ///// the bond has one color
          qglColor(atoms->color(atom1));
          glVertex3d(atoms->x(atom1), atoms->y(atom1), atoms->z(atom1));
          glVertex3d(atoms->x(atom2), atoms->y(atom2), atoms->z(atom2));
        }
        else
        {
          ///// 2 half-bonds
          const double midX = (atoms->x(atom1) + atoms->x(atom2))/2.0;
          const double midY = (atoms->y(atom1) + atoms->y(atom2))/2.0;
          const double midZ = (atoms->z(atom1) + atoms->z(atom2))/2.0;
          qglColor(atoms->color(atom1));
          glVertex3d(atoms->x(atom1), atoms->y(atom1), atoms->z(atom1));
          glVertex3d(midX, midY, midZ);

          qglColor(atoms->color(atom2));
          glVertex3d(midX, midY, midZ);
          glVertex3d(atoms->x(atom2), atoms->y(atom2), atoms->z(atom2));
        }
      }
    glEnd();
    glEnable(GL_LIGHTING);
    return;
  }

  ///// here moleculeStyle is either DisplayStyle::Tubes or DisplayStyle::BallAndStick
  ///// which are rendered in the same way
  for(unsigned int i = 0; i < firstAtom->size(); i++)
  {
    ///// add the bond between atoms firstAtom[i] and secondAtom[i]
    const unsigned int atom1 = firstAtom->operator[](i);
    const unsigned int atom2 = secondAtom->operator[](i);

    x1 = static_cast<float>(atoms->x(atom1));
    x2 = static_cast<float>(atoms->x(atom2));
    y1 = static_cast<float>(atoms->y(atom1));
    y2 = static_cast<float>(atoms->y(atom2));
    z1 = static_cast<float>(atoms->z(atom1));
    z2 = static_cast<float>(atoms->z(atom2));
    distanceXY = sqrt((x1 - x2)*(x1 - x2) + (y1 - y2)*(y1 - y2));
    distance = sqrt((x1 - x2)*(x1 - x2) + (y1 - y2)*(y1 - y2) + (z1 - z2)*(z1 - z2));
    if(distance < 0.01f)
      continue; //no need to draw those small bonds

    glPushMatrix();

    ///// TRANSLATE
    glTranslatef(x1, y1, z1);

    ///// ROTATE
    ///// determine PHI rotation angle (around y-axis)
    phi = acos((z2 - z1)/distance);
    ///// determine THETA rotation angle (around z-axis)
    if(distanceXY <= 0.0f)
      theta = 0.0f;
    else
    {
      theta = acos((x2 - x1)/distanceXY);
      if(y2 < y1)
        theta = 2.0f*Point3D<float>::PI - theta;
    }
    ///// convert them to degrees
    phi *= Point3D<float>::RADTODEG;
    theta *= Point3D<float>::RADTODEG;
    glRotatef(theta, 0.0f, 0.0f, 1.0f);
    glRotatef(phi, 0.0f, 1.0f, 0.0f);

    /////SCALE
    float scaleFactor = 1.0f;
    if(atoms->color(atom1) != atoms->color(atom2))
      scaleFactor = 2.0f;
    glScalef(moleculeParameters.sizeBonds, moleculeParameters.sizeBonds, distance/(scaleFactor*cylinderHeight));

    if(atoms->color(atom1) == atoms->color(atom2))
    {
      ///// the bond has one color
      qglColor(atoms->color(atom1));
      glCallList(bondObject);
    }
    else
    {
      ///// the bond has two colors
      //// make firstAtom's part of bond
      qglColor(atoms->color(atom1));
      glCallList(bondObject);
      ///// make secondAtom's part of bond
      qglColor(atoms->color(atom2));
      glTranslatef(0.0f, 0.0f, cylinderHeight);
      glCallList(bondObject);
    }
    glPopMatrix();
  }
}

///// drawLabels //////////////////////////////////////////////////////////////
void GLSimpleMoleculeView::drawLabels()
/// Draws the element types and numbers.
{
  //qDebug("calling drawLabels");
  if(!(showElements || showNumbers || chargeType != AtomSet::None))
    return;

  // determine the amount of backrotation
  Vector3D<float> axis;
  float angle;
  orientationQuaternion->getAxisAngle(axis, angle);

  // set up the visuals
  glDisable(GL_LIGHTING);
  qglColor(moleculeParameters.colorLabels);

  // loop over the atoms
  for(unsigned int i = 0; i < atoms->count(); i++)
  {
    glPushMatrix();
    glTranslated(atoms->x(i), atoms->y(i), atoms->z(i));
    glRotatef(-angle, axis.x(), axis.y(), axis.z()); // backrotation
    QString label;
    if(showElements)
      label = AtomSet::numToAtom(atoms->atomicNumber(i)).stripWhiteSpace();
    if(showNumbers)
      label += QString::number(i + 1);
    if(chargeType != AtomSet::None)
    {
      if(showElements || showNumbers)
        label += "(";

      if(chargeType == AtomSet::Mulliken)
        label += QString::number(atoms->charge(AtomSet::Mulliken, i),'f',3);
      else
        label += QString::number(atoms->charge(AtomSet::Stockholder, i),'f',3);

      if(showElements || showNumbers)
        label += ")";
    }
    renderText(0.0, 0.0, AtomSet::vanderWaals(atoms->atomicNumber(i))/2.0 + 0.05, label, labelFont); // the +0.05 is needed for certain crappy Windows OpenGL drivers
    glPopMatrix();
  }

  // restore the lighting
  glEnable(GL_LIGHTING);
}

///// drawForces //////////////////////////////////////////////////////////////
void GLSimpleMoleculeView::drawForces()
/// Draws the forces.
/// If forces don't need to be selected individually, all forces can be stored
/// together in an OpenGL display list.
{
  //qDebug("calling drawForces");
  if(forcesStyle == None || !atoms->hasForces())
    return;

  // forces are scaled. A force which is considered refined by relax (< 0.0009 mdyne/A) is equal
  // to 0.1 Angstrom.
  const float scaleFactor = 0.1f/0.0009f;

  const GLfloat opacity = moleculeParameters.opacityForces/100.0f;
  if(moleculeParameters.forcesOneColor)
  {
    // set the color and opacity here once
    const GLfloat red   = QColor(moleculeParameters.colorForces).red()/255.0f;
    const GLfloat green = QColor(moleculeParameters.colorForces).green()/255.0f;
    const GLfloat blue  = QColor(moleculeParameters.colorForces).blue()/255.0f;
    glColor4f(red, green, blue, opacity);
  }

  float x1, y1, z1, x2, y2, z2, distance, distanceXY, phi, theta;
  for(unsigned int i = 0; i < atoms->count(); i++)
  {
    x1 = static_cast<float>(atoms->x(i));
    y1 = static_cast<float>(atoms->y(i));
    z1 = static_cast<float>(atoms->z(i));
    x2 = static_cast<float>(atoms->dx(i));
    y2 = static_cast<float>(atoms->dy(i));
    z2 = static_cast<float>(atoms->dz(i));
    distanceXY = sqrt(x2*x2 + y2*y2);
    distance = sqrt(x2*x2 + y2*y2 + z2*z2);
    if(distance < 0.1f/scaleFactor)
      continue; //no need to draw those small (refined) forces

    glPushMatrix(); // save the current matrix

    ///// TRANSLATE
    glTranslatef(x1, y1, z1);
    ///// ROTATE
    ///// determine PHI rotation angle (around y-axis)
    phi = acos(z2/distance);
    ///// determine THETA rotation angle (around z-axis)
    if(distanceXY <= 0.01f)
      theta = 0.0f;
    else
    {
      theta = acos(x2/distanceXY);
      if(y2 < 0.0f)
        theta = 2.0f*Point3D<float>::PI - theta;
    }
    ///// convert them to degrees
    phi *= Point3D<float>::RADTODEG;
    theta *= Point3D<float>::RADTODEG;
    glRotatef(theta, 0.0f, 0.0f, 1.0f);
    glRotatef(phi, 0.0f, 1.0f, 0.0f);
    //// SCALE
    if(forcesStyle == Lines)
      glScalef(1.0f, 1.0f, scaleFactor*distance/(2.0f*cylinderHeight));
    else
      glScalef(moleculeParameters.sizeForces, moleculeParameters.sizeForces, scaleFactor*distance/(2.0f*cylinderHeight));

    if(!moleculeParameters.forcesOneColor)
    {
      const GLfloat red   = atoms->color(i).red()/255.0f;
      const GLfloat green = atoms->color(i).green()/255.0f;
      const GLfloat blue  = atoms->color(i).blue()/255.0f;
      glColor4f(red, green, blue, opacity);
    }

    //glLoadName(START_FORCES);
    if(forcesStyle == Lines)
      glCallList(forceObjectLines);
    else
      glCallList(forceObjectTubes);

    glPopMatrix(); // restore the matrix
  }
}

///// drawICValue /////////////////////////////////////////////////////////////
void GLSimpleMoleculeView::drawICValue()
/// Draws the value for the currently selected
/// internal coordinate.
{
  //qDebug("calling drawICValue");
  ///// set up the visuals
  glDisable(GL_LIGHTING);
  qglColor(moleculeParameters.colorICs);

  std::list<unsigned int>::iterator it = selectionList.begin();
  switch(selectionList.size())
  {
    case 2: ///// bond
    {
      // the atoms
      unsigned int atom1 = *it++;
      unsigned int atom2 = *it;
      float x1 = static_cast<float>(atoms->x(atom1));
      float x2 = static_cast<float>(atoms->x(atom2));
      float y1 = static_cast<float>(atoms->y(atom1));
      float y2 = static_cast<float>(atoms->y(atom2));
      float z1 = static_cast<float>(atoms->z(atom1));
      float z2 = static_cast<float>(atoms->z(atom2));
      // the text
      float distance = sqrt((x1 - x2)*(x1 - x2) + (y1 - y2)*(y1 - y2) + (z1 - z2)*(z1 - z2));
      // the actual rendering
      ///// on top of the selected bond -> old method
      /*
      glPushMatrix();
      glTranslatef((x1 + x2)/2.0f, (y1 + y2)/2.0f, (z1 + z2)/2.0f);
      glRotatef(-angle, axis.x(), axis.y(), axis.z());
      renderText(0.0, 0.0, parameters.bondSizeBS + 0.01, QString::number(distance, 'f', 4), QFont("Times"));
      glPopMatrix();
      */
      ///// on top of the selected bond -> new method
      // get the window coordinates of the centre of the bond
      GLdouble modelview[16];
      GLdouble projection[16];
      GLint viewport[4];
      glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
      glGetDoublev(GL_PROJECTION_MATRIX, projection);
      glGetIntegerv(GL_VIEWPORT, viewport);
      GLdouble xwin, ywin, zwin;
      GLint result = gluProject((atoms->x(atom1)+atoms->x(atom2))/2.0, (atoms->y(atom1)+atoms->y(atom2))/2.0, (atoms->z(atom1)+atoms->z(atom2))/2.0, modelview, projection, viewport, &xwin, &ywin, &zwin);
      if(result == GL_FALSE)
        break;
      renderText(static_cast<int>(xwin), height() - static_cast<int>(ywin), QString::number(distance, 'f', 4), labelFont);
      break;
    }
    case 3: ///// angle
    {
      // the atoms
      unsigned int atom1 = *it++;
      unsigned int atom2 = *it++;
      unsigned int atom3 = *it;
      float x1 = static_cast<float>(atoms->x(atom1));
      float x2 = static_cast<float>(atoms->x(atom2));
      float x3 = static_cast<float>(atoms->x(atom3));
      float y1 = static_cast<float>(atoms->y(atom1));
      float y2 = static_cast<float>(atoms->y(atom2));
      float y3 = static_cast<float>(atoms->y(atom3));
      float z1 = static_cast<float>(atoms->z(atom1));
      float z2 = static_cast<float>(atoms->z(atom2));
      float z3 = static_cast<float>(atoms->z(atom3));
      // the text
      Vector3D<float> bond1(x2, y2, z2, x1, y1, z1);
      Vector3D<float> bond2(x2, y2, z2, x3, y3, z3);
      float localAngle = bond1.angle(bond2);
      // the actual rendering
      ///// on the bond 1-3 -> old method
      /*
      glPushMatrix();
      glTranslatef((x1 + x3)/2.0f, (y1 + y3)/2.0f, (z1 + z3)/2.0f);
      glRotatef(-angle, axis.x(), axis.y(), axis.z());
      renderText(0.0, 0.0, AtomSet::vanderWaals(atoms->atomicNumber(atom2))/2.0 + 0.01, QString::number(localAngle, 'f', 2), QFont("Times"));
      glPopMatrix();
      */
      ///// on the bond 1-3 -> new method
      GLdouble modelview[16];
      GLdouble projection[16];
      GLint viewport[4];
      glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
      glGetDoublev(GL_PROJECTION_MATRIX, projection);
      glGetIntegerv(GL_VIEWPORT, viewport);
      GLdouble xwin, ywin, zwin;
      GLint result = gluProject((atoms->x(atom1)+atoms->x(atom3))/2.0, (atoms->y(atom1)+atoms->y(atom3))/2.0, (atoms->z(atom1)+atoms->z(atom3))/2.0, modelview, projection, viewport, &xwin, &ywin, &zwin);
      if(result == GL_FALSE)
        break;
      renderText(static_cast<int>(xwin), height() - static_cast<int>(ywin), QString::number(localAngle, 'f', 2), labelFont);
      break;
    }
    case 4: ///// torsion
    {
      // the atoms
      unsigned int atom1 = *it++;
      unsigned int atom2 = *it++;
      unsigned int atom3 = *it++;
      unsigned int atom4 = *it;
      float x1 = static_cast<float>(atoms->x(atom1));
      float x2 = static_cast<float>(atoms->x(atom2));
      float x3 = static_cast<float>(atoms->x(atom3));
      float x4 = static_cast<float>(atoms->x(atom4));
      float y1 = static_cast<float>(atoms->y(atom1));
      float y2 = static_cast<float>(atoms->y(atom2));
      float y3 = static_cast<float>(atoms->y(atom3));
      float y4 = static_cast<float>(atoms->y(atom4));
      float z1 = static_cast<float>(atoms->z(atom1));
      float z2 = static_cast<float>(atoms->z(atom2));
      float z3 = static_cast<float>(atoms->z(atom3));
      float z4 = static_cast<float>(atoms->z(atom4));
      // the text
      Vector3D<float> bond1(x2, y2, z2, x1, y1, z1);
      Vector3D<float> bond2(x3, y3, z3, x4, y4, z4);
      Vector3D<float> centralbond(x2, y2, z2, x3, y3, z3);
      float localAngle = bond1.torsion(bond2, centralbond);
      // the actual rendering
      ///// on the central bond -> old method
      /*
      glPushMatrix();
      glTranslatef((x2 + x3)/2.0f, (y2 + y3)/2.0f, (z2 + z3)/2.0f);
      glRotatef(-angle, axis.x(), axis.y(), axis.z());
      renderText(0.0, 0.0, parameters.bondSizeBS + 0.01, QString::number(localAngle, 'f', 2), QFont("Times"));
      glPopMatrix();
      */
      ///// on the central bond -> new method
      GLdouble modelview[16];
      GLdouble projection[16];
      GLint viewport[4];
      glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
      glGetDoublev(GL_PROJECTION_MATRIX, projection);
      glGetIntegerv(GL_VIEWPORT, viewport);
      GLdouble xwin, ywin, zwin;
      GLint result = gluProject((atoms->x(atom2)+atoms->x(atom3))/2.0, (atoms->y(atom2)+atoms->y(atom3))/2.0, (atoms->z(atom2)+atoms->z(atom3))/2.0, modelview, projection, viewport, &xwin, &ywin, &zwin);
      if(result == GL_FALSE)
        break;
      renderText(static_cast<int>(xwin), height() - static_cast<int>(ywin), QString::number(localAngle, 'f', 2), labelFont);
      break;
    }
  }
  glEnable(GL_LIGHTING);
}

///// drawSelections //////////////////////////////////////////////////////////
void GLSimpleMoleculeView::drawSelections()
/// Draws the selected atoms and internal coordinates.
{
  //qDebug("calling drawSelections");
  if(moleculeStyle == None || moleculeStyle == Lines)
  {
    qglColor(moleculeParameters.colorSelections);
    glDisable(GL_LIGHTING);
    ///// selected atoms
    glPointSize(selectionPointSize);
    glBegin(GL_POINTS);
      std::list<unsigned int>::iterator it = selectionList.begin();
      while(it != selectionList.end())
      {
        glVertex3d(atoms->x(*it), atoms->y(*it), atoms->z(*it));
        it++;
      }
    glEnd();
    ///// selected bonds
    if(selectionList.size() >= 2 && selectionList.size() <= 4)
    {
      glLineWidth(selectionLineWidth);
      ///// draw the bonds as a connected strip
      std::list<unsigned int>::iterator it = selectionList.begin();
      glBegin(GL_LINE_STRIP);
        while(it != selectionList.end())
        {
          glVertex3d(atoms->x(*it), atoms->y(*it), atoms->z(*it));
          it++;
        }
      glEnd();
      glLineWidth(moleculeParameters.sizeLines);
    }
    glEnable(GL_LIGHTING);
    return;
  }

  const GLfloat red   = QColor(moleculeParameters.colorSelections).red()/255.0f;
  const GLfloat green = QColor(moleculeParameters.colorSelections).green()/255.0f;
  const GLfloat blue  = QColor(moleculeParameters.colorSelections).blue()/255.0f;
  const GLfloat opacity = moleculeParameters.opacitySelections/100.0f;
  glColor4f(red, green, blue, opacity);

  ///// selected atoms
  std::list<unsigned int>::iterator it = selectionList.begin();
  while(it != selectionList.end())
  {
    // draw a yellow halo around the selected atom
    glPushMatrix();
    glTranslatef(atoms->x(*it), atoms->y(*it), atoms->z(*it)); // set the position
    if(moleculeStyle == VanDerWaals)
    {
      glScalef(AtomSet::vanderWaals(atoms->atomicNumber(*it))*1.5f * 1.1f,
               AtomSet::vanderWaals(atoms->atomicNumber(*it))*1.5f * 1.1f,
               AtomSet::vanderWaals(atoms->atomicNumber(*it))*1.5f * 1.1f);
    }
    else if(moleculeStyle == Tubes)
    {
      glScalef(moleculeParameters.sizeBonds * 1.6f,
               moleculeParameters.sizeBonds * 1.6f,
               moleculeParameters.sizeBonds * 1.6f);
    }
    else
    {
      glScalef(AtomSet::vanderWaals(atoms->atomicNumber(*it))/2.0f * 1.1f,
               AtomSet::vanderWaals(atoms->atomicNumber(*it))/2.0f * 1.1f,
               AtomSet::vanderWaals(atoms->atomicNumber(*it))/2.0f * 1.1f);
    }

    glLoadName(START_SELECTEDATOMS);
    glCallList(atomObject); // make the atom
    glPopMatrix();
    it++;
  }

  ///// bonds between selected atoms (up to torsions)
  if(selectionList.size() >= 2 && selectionList.size() <= 4)
  {
    float x1, y1, z1, x2, y2, z2, distance, distanceXY, phi, theta;

    std::list<unsigned int>::iterator it = selectionList.begin();
    unsigned int atom1 = *it++;
    unsigned int atom2 = *it;
    while(it != selectionList.end())
    {
      x1 = static_cast<float>(atoms->x(atom1));
      x2 = static_cast<float>(atoms->x(atom2));
      y1 = static_cast<float>(atoms->y(atom1));
      y2 = static_cast<float>(atoms->y(atom2));
      z1 = static_cast<float>(atoms->z(atom1));
      z2 = static_cast<float>(atoms->z(atom2));
      distanceXY = sqrt((x1 - x2)*(x1 - x2) + (y1 - y2)*(y1 - y2));
      distance = sqrt((x1 - x2)*(x1 - x2) + (y1 - y2)*(y1 - y2) + (z1 - z2)*(z1 - z2));
      if(distance < 0.01f)
        continue; //no need to draw those small bonds

      glPushMatrix();
      ///// TRANSLATE
      glTranslatef(x1, y1, z1);
      ///// ROTATE
      phi = acos((z2 - z1)/distance);
      if(distanceXY <= 0.0f)
        theta = 0.0f;
      else
      {
        theta = acos((x2 - x1)/distanceXY);
        if(y2 < y1)
          theta = 2.0f*Point3D<float>::PI - theta;
      }
      phi *= Point3D<float>::RADTODEG;
      theta *= Point3D<float>::RADTODEG;
      glRotatef(theta, 0.0f, 0.0f, 1.0f);
      glRotatef(phi, 0.0f, 1.0f, 0.0f);
      ///// SCALE
      glScalef(moleculeParameters.sizeBonds * 1.1f, moleculeParameters.sizeBonds * 1.1f, distance/cylinderHeight);
      glLoadName(START_SELECTEDBONDS);
      glCallList(bondObject);

      glPopMatrix();
      atom1 = atom2;
      atom2 = *(++it);
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
///// Static Variables                                                    /////
///////////////////////////////////////////////////////////////////////////////

const float GLSimpleMoleculeView::cylinderHeight = 10.0f;
GLMoleculeParameters GLSimpleMoleculeView::moleculeParameters = {5, 1.0f, 0.2f, 0.2f, BallAndStick, Tubes, 1000, false, true,
0x00FF00, 0x00FFFF, 0xFFFF00, 50, 0xFFFF0, false, 100};

