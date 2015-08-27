/***************************************************************************
                      glmoleculeview.cpp  -  description
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

///// Comments ////////////////////////////////////////////////////////////////
/*!
  \class GLMoleculeView
  \brief This class shows a molecule and various properties in 3D using OpenGL.

  It is a subclass of GLSimpleMoleculeView and additionally allows changing the
  molecule, visualisation of isodensity surfaces etc.

*/
/// \file
/// Contains the implementation of the class GLMoleculeView

///// Header files ////////////////////////////////////////////////////////////

// STL header files
#include <algorithm>

// Qt header files
#include <qfiledialog.h>
#include <qinputdialog.h>
#include <qmessagebox.h>
//#include <qpoint.h>
#include <qradiobutton.h>
#include <qstringlist.h>
#include <qtimer.h>
#include <qvalidator.h>

// Xbrabo header files
#include "atomset.h"
#include "coordinateswidget.h"
#include "densitybase.h"
#include "glmoleculeview.h"
#include "isosurface.h"
#include "newatombase.h"
#include "point3d.h"
#include "quaternion.h"
#include "vector3d.h"

///////////////////////////////////////////////////////////////////////////////
///// Public Member Functions                                             /////
///////////////////////////////////////////////////////////////////////////////

///// constructor /////////////////////////////////////////////////////////////
GLMoleculeView::GLMoleculeView(AtomSet* atomset, QWidget* parent, const char* name) : GLSimpleMoleculeView(atomset, parent, name),
  atoms(atomset),
  densityDialog(NULL),
  newAtomDialog(NULL),
  manipulateSelection(false)
/// The default constructor.
{
  isoSurface = new IsoSurface();
}

///// destructor //////////////////////////////////////////////////////////////
GLMoleculeView::~GLMoleculeView()
/// The default destructor.
{
  makeCurrent();
  for(unsigned int i = 0; i < glSurfaces.size(); i++)
    glDeleteLists(glSurfaces[i], 1);
  delete isoSurface;
}

///////////////////////////////////////////////////////////////////////////////
///// Public Slots                                                        /////
///////////////////////////////////////////////////////////////////////////////

///// alterCartesian //////////////////////////////////////////////////////////
void GLMoleculeView::alterCartesian()
/// Changes the cartesian coordinates of the selection.
/// If one atom is selected, the absolute coordinates can be changed
/// If multiple atoms are selected, only relative changes can be given.
{
  if(selectionList.size() == 0)
    return;

  ///// setup the dialog
  CoordinatesWidget* coords = new CoordinatesWidget(this, 0, true);
  QDoubleValidator* v = new QDoubleValidator(-9999.0,9999.,12,this);
  coords->LineEditX->setValidator(v);
  coords->LineEditY->setValidator(v);
  coords->LineEditZ->setValidator(v);
  v = 0;
  if(selectionList.size() == 1)
  {
    // use default absolute changes
    unsigned int atom = *selectionList.begin();
    coords->LineEditX->setText(QString::number(atoms->x(atom), 'f', 8));
    coords->LineEditY->setText(QString::number(atoms->y(atom), 'f', 8));
    coords->LineEditZ->setText(QString::number(atoms->z(atom), 'f', 8));
  }
  else
  {
    // only relative changes
    coords->RadioButtonAbsolute->setEnabled(false);
    coords->RadioButtonRelative->setChecked(true);
    coords->LineEditX->setText("0.0");
    coords->LineEditY->setText("0.0");
    coords->LineEditZ->setText("0.0");
  }

  ///// run the dialog
  if(coords->exec() == QDialog::Accepted)
  {
    if(coords->RadioButtonAbsolute->isChecked())
    {
      // absolute changes for one atom
      unsigned int atom = *selectionList.begin();
      bool ok;
      double newx = coords->LineEditX->text().toDouble(&ok);
      if(ok)
        atoms->setX(atom, newx);
      double newy = coords->LineEditY->text().toDouble(&ok);
      if(ok)
        atoms->setY(atom, newy);
      double newz = coords->LineEditZ->text().toDouble(&ok);
      if(ok)
        atoms->setZ(atom, newz);
    }
    else
    {
      // relative changes for one or more atoms
      bool ok;
      double deltax = coords->LineEditX->text().toDouble(&ok);
      if(!ok)
        deltax = 0.0;
      double deltay = coords->LineEditY->text().toDouble(&ok);
      if(!ok)
        deltay = 0.0;
      double deltaz = coords->LineEditZ->text().toDouble(&ok);
      if(!ok)
        deltaz = 0.0;
      std::list<unsigned int>::iterator it = selectionList.begin();
      while(it != selectionList.end())
      {
        atoms->setX(*it, atoms->x(*it) + deltax);
        atoms->setY(*it, atoms->y(*it) + deltay);
        atoms->setZ(*it, atoms->z(*it) + deltaz);
        it++;
      }
    }
  }
  delete coords;
  setModified();
  updateAtomSet();
}

///// alterInternal ///////////////////////////////////////////////////////////
void GLMoleculeView::alterInternal()
/// Changes the cartesian coordinates of the selection.
/// If one atom is selected, the absolute coordinates can be changed
/// If multiple atoms are selected, only relative changes can be given
{
  switch(selectionType)
  {
    case SELECTION_BOND:
    {
      std::list<unsigned int>::iterator it = selectionList.begin();
      unsigned int atom1 = *it++;
      unsigned int atom2 = *it;
      ///// get the current bond length
      // <double> version
      Vector3D<double> bond(atoms->x(atom2), atoms->y(atom2), atoms->z(atom2), atoms->x(atom1), atoms->y(atom1), atoms->z(atom1));
      double bondLength = bond.length();

      bool ok;
      double newLength = QInputDialog::getDouble("Xbrabo", tr("Change the distance between atoms ")+QString::number(atom1+1)+" and "+QString::number(atom2+1), bondLength, -1000.0, 1000.0, 4, &ok, this);
      if(ok && fabs(newLength - bondLength) > 0.00001)
        atoms->changeBond(newLength - bondLength, atom1, atom2, true);
      else
        return; // no new value was entered
      break;
    }

    case SELECTION_ANGLE:
    {
      std::list<unsigned int>::iterator it = selectionList.begin();
      unsigned int atom1 = *it++;
      unsigned int atom2 = *it++;
      unsigned int atom3 = *it;
      ///// get the current angle
      Vector3D<double> bond1(atoms->x(atom2), atoms->y(atom2), atoms->z(atom2), atoms->x(atom1), atoms->y(atom1), atoms->z(atom1));
      Vector3D<double> bond2(atoms->x(atom2), atoms->y(atom2), atoms->z(atom2), atoms->x(atom3), atoms->y(atom3), atoms->z(atom3));
      double angle = bond1.angle(bond2);
      bool ok;
      double newAngle = QInputDialog::getDouble("Xbrabo", tr("Change the angle ")+QString::number(atom1+1)+"-"+QString::number(atom2+1)+"-"+QString::number(atom3+1), angle, -1000.0, 1000.0, 2, &ok, this);
      if(ok && fabs(newAngle - angle) > 0.001)
        atoms->changeAngle(newAngle - angle, atom1, atom2, atom3, true);
      else
        return; // no new value was entered
      break;
    }

    case SELECTION_TORSION:
    {
      std::list<unsigned int>::iterator it = selectionList.begin();
      unsigned int atom1 = *it++;
      unsigned int atom2 = *it++;
      unsigned int atom3 = *it++;
      unsigned int atom4 = *it;
      ///// get the current torsion angle
      Vector3D<double> bond1(atoms->x(atom2), atoms->y(atom2), atoms->z(atom2), atoms->x(atom1), atoms->y(atom1), atoms->z(atom1));
      Vector3D<double> centralbond(atoms->x(atom2), atoms->y(atom2), atoms->z(atom2), atoms->x(atom3), atoms->y(atom3), atoms->z(atom3));
      Vector3D<double> bond2(atoms->x(atom3), atoms->y(atom3), atoms->z(atom3), atoms->x(atom4), atoms->y(atom4), atoms->z(atom4));
      double torsion = bond1.torsion(bond2, centralbond);
      bool ok;
      double newTorsion = QInputDialog::getDouble("Xbrabo", tr("Change the torsion angle ")+QString::number(atom1+1)+"-"+QString::number(atom2+1)+"-"+QString::number(atom3+1)+"-"+QString::number(atom4+1), torsion, -1000.0, 1000.0, 2, &ok, this);
      if(ok && fabs(newTorsion - torsion) > 0.001)
        atoms->changeTorsion(torsion - newTorsion, atom1, atom2, atom3, atom4, true);
      else
        return; // no new value was entered
      break;
    }
    default: return;
  }
  setModified();
  updateAtomSet();
}

///// showDensity /////////////////////////////////////////////////////////////
void GLMoleculeView::showDensity()
/// Shows electron density isosurfaces from a Gaussian .cube file.
{
  if(densityDialog == NULL)
  {
    densityDialog = new DensityBase(isoSurface, this);
    connect(densityDialog, SIGNAL(newSurface(const unsigned int)), this, SLOT(addGLSurface(const unsigned int)));
    connect(densityDialog, SIGNAL(updatedSurface(const unsigned int)), this, SLOT(updateGLSurface(const unsigned int)));
    connect(densityDialog, SIGNAL(deletedSurface(const unsigned int)), this, SLOT(deleteGLSurface(const unsigned int)));
    connect(densityDialog, SIGNAL(redrawScene()), this, SLOT(updateGL()));
  }
  densityDialog->show();
  if(!isoSurface->densityPresent())
    densityDialog->loadDensityA();
}

///// addAtoms ////////////////////////////////////////////////////////////////
void GLMoleculeView::addAtoms()
/// Shows a dialog allowing the addition of atoms to the molecule.
{
  if(newAtomDialog == NULL)
  {
    newAtomDialog = new NewAtomBase(atoms, this);
    connect(newAtomDialog, SIGNAL(atomAdded()), this, SLOT(updateAtomSet()));
    connect(newAtomDialog, SIGNAL(atomAdded()), this, SLOT(setModified()));
    connect(newAtomDialog, SIGNAL(atomAdded()), this, SIGNAL(atomsetChanged()));
  }
  newAtomDialog->show();
}

///// deleteSelectedAtoms /////////////////////////////////////////////////////
void GLMoleculeView::deleteSelectedAtoms()
/// Deletes all selected atoms.
{
  if(selectionList.empty())
    return;

  ///// delete the atoms from largest to smallest index
  // make a copy
  std::vector<unsigned int> sortedList;
  sortedList.reserve(selectionList.size());
  sortedList.assign(selectionList.begin(), selectionList.end());
  // sort it from largest to smallest
  std::sort(sortedList.begin(), sortedList.end(), std::greater<unsigned int>());
  // delete the atoms
  for(unsigned int i = 0; i < sortedList.size(); i++)
    atoms->removeAtom(sortedList[i]);
  // clear the selection
  unselectAll();
  if(newAtomDialog != 0)
    newAtomDialog->updateAtomLimits();
  updateAtomSet();
  setModified();
  emit atomsetChanged();
}

///// toggleSelection /////////////////////////////////////////////////////////
void GLMoleculeView::toggleSelection()
/// Toggles between manipulating the selected atoms and the entire system.
{
  manipulateSelection = !manipulateSelection;
}

///////////////////////////////////////////////////////////////////////////////
///// Protected Member Functions                                          /////
///////////////////////////////////////////////////////////////////////////////

///// boundingSphereRadius ////////////////////////////////////////////////////
float GLMoleculeView::boundingSphereRadius()
/// Calculates the radius of the bounding sphere. If atoms are present, the
/// radius of the base class is used. If no atoms are present but there is a
/// density loaded, its box is used.
{
  float radius = GLSimpleMoleculeView::boundingSphereRadius();

  if(atoms->count() == 0 && isoSurface->densityPresent())
  {
    ///// get the boundaries of the box
    Point3D<float> origin = isoSurface->getOrigin();
    Point3D<float> delta = isoSurface->getDelta();
    Point3D<unsigned int> numPoints = isoSurface->getNumPoints();
    float x, y, z;
    std::vector<float> squaredR;
    // the current radius
    squaredR.push_back(static_cast<float>(radius*radius));
    // point 1: the origin
    x = origin.x() - centerX;
    y = origin.y() - centerY;
    z = origin.z() - centerZ;
    squaredR.push_back(x*x + y*y + z*z);
    // point 2: along the x-axis
    x = origin.x() + delta.x() * (numPoints.x() - 1) - centerX;
    y = origin.y() - centerY;
    z = origin.z() - centerZ;
    squaredR.push_back(x*x + y*y + z*z);
    // point 3: along the y-axis
    x = origin.x() - centerX;
    y = origin.y() + delta.y() * (numPoints.y() - 1) - centerY;
    z = origin.z() - centerZ;
    squaredR.push_back(x*x + y*y + z*z);
    // point 4: along the z-axis
    x = origin.x() - centerX;
    y = origin.y() - centerY;
    z = origin.z() + delta.z() * (numPoints.z() - 1) - centerZ;
    squaredR.push_back(x*x + y*y + z*z);
    // point 5: along the x and y axes
    x = origin.x() + delta.x() * (numPoints.x() - 1) - centerX;
    y = origin.y() + delta.y() * (numPoints.y() - 1) - centerY;
    z = origin.z() - centerZ;
    squaredR.push_back(x*x + y*y + z*z);
    // point 6: along the x and z axes
    x = origin.x() + delta.x() * (numPoints.x() - 1) - centerX;
    y = origin.y() - centerY;
    z = origin.z() + delta.z() * (numPoints.z() - 1) - centerZ;
    squaredR.push_back(x*x + y*y + z*z);
    // point 7: along the y and z axes
    x = origin.x() - centerX;
    y = origin.y() + delta.y() * (numPoints.y() - 1) - centerY;
    z = origin.z() + delta.z() * (numPoints.z() - 1) - centerZ;
    squaredR.push_back(x*x + y*y + z*z);
    // point 8: along all axes
    x = origin.x() + delta.x() * (numPoints.x() - 1) - centerX;
    y = origin.y() + delta.y() * (numPoints.y() - 1) - centerY;
    z = origin.z() + delta.z() * (numPoints.z() - 1) - centerZ;
    squaredR.push_back(x*x + y*y + z*z);

    float boxradius = sqrt(*(std::max_element(squaredR.begin(), squaredR.end())));
    qDebug("GLMoleculeView::boundingSphereRadius: boxradius = %f, radius = %f",boxradius, radius);
    if(boxradius > radius)
      radius = boxradius;
  }
  return radius;
}

///// mouseMoveEvent //////////////////////////////////////////////////////////
void GLMoleculeView::mouseMoveEvent(QMouseEvent* e)
/// Overridden from GLView::mouseMoveEvent.
/// Handles left mouse button drags.
{
  QPoint newPosition = e->pos();
  if(selectionType != SELECTION_NONE && e->state() & Qt::LeftButton && (manipulateSelection || e->state() & Qt::AltButton) && !(e->state() & Qt::ShiftButton && e->state() & Qt::ControlButton))
  {
    ///// leftbutton mousemoves for manipulation of the selected atoms
    if(e->state() & Qt::ShiftButton)
    {
      ///// up/down movement: zooming (z-translation)
      ///// left/right movement: z-rotation
      if(abs(newPosition.y() - mousePosition.y()) > abs(newPosition.x() - mousePosition.x()))
        translateSelection(0, 0, newPosition.y() - mousePosition.y());
      else if(newPosition.x() != mousePosition.x())
        rotateSelection(0.0, 0.0, 180.0 * static_cast<double>(newPosition.x() - mousePosition.x()) / static_cast<double>(width()));
    }
    else if(e->state() & Qt::ControlButton)
      ///// up/down movement: y-translation
      ///// left/right movement: x-translation
      translateSelection(newPosition.x() - mousePosition.x(), newPosition.y() - mousePosition.y(), 0);
    else
      ///// up/down movement: x-rotation
      ///// left/right movement: y-rotation
      rotateSelection(-180.0 * static_cast<double>(newPosition.y() - mousePosition.y()) / static_cast<double>(height()),
                      -180.0 * static_cast<double>(newPosition.x() - mousePosition.x()) / static_cast<double>(width()), 0.0);
  }
  else if(selectionType >= SELECTION_BOND && selectionType <= SELECTION_TORSION && e->state() & Qt::LeftButton && e->state() & Qt::ShiftButton && e->state() & Qt::ControlButton)
    ///// LEFTBUTTON + SHIFT + CONTROL + horizontal movement: change selected internal coordinate
    changeSelectedIC(e->pos().x() - mousePosition.x());
  else
    GLView::mouseMoveEvent(e); // normal manipulation of entire system

  mousePosition = newPosition;
}

///// keyPressEvent ///////////////////////////////////////////////////////////
void GLMoleculeView::keyPressEvent(QKeyEvent* e)
/// Overridden from GLSimpleMoleculeView::keyPressEvent. Handles key presses for manipulating
/// selections.
{
  if(selectionType != SELECTION_NONE && (manipulateSelection || e->state() & Qt::AltButton) && !(e->state() & Qt::ShiftButton && e->state() & Qt::ControlButton))
  {
    switch(e->key())
    {
      case Qt::Key_Left : if(e->state() & Qt::ShiftButton)
                            rotateSelection(0.0, 0.0, -5.0);
                          else if(e->state() & Qt::ControlButton)
                            translateSelection(-5, 0, 0);
                          else
                            rotateSelection(0.0, 5.0, 0.0);
                          break;

      case Qt::Key_Up   : if(e->state() & Qt::ShiftButton)
                            translateSelection(0, 0, -5);
                          else if(e->state() & Qt::ControlButton)
                            translateSelection(0, -5, 0);
                          else
                            rotateSelection(5.0, 0.0, 0.0);
                          break;

      case Qt::Key_Right: if(e->state() & Qt::ShiftButton)
                            rotateSelection(0.0, 0.0, 5.0);
                          else if(e->state() & Qt::ControlButton)
                            translateSelection(5, 0, 0);
                          else
                            rotateSelection(0.0, -5.0, 0.0);
                          break;

      case Qt::Key_Down : if(e->state() & Qt::ShiftButton)
                            translateSelection(0, 0, 5);
                          else if(e->state() & Qt::ControlButton)
                            translateSelection(0, 5, 0);
                          else
                            rotateSelection(-5.0, 0.0, 0.0);
                          break;

      //default:            e->ignore();
      //                    return;
    }
  }
  else if(selectionType >= SELECTION_BOND && selectionType <= SELECTION_TORSION && e->state() & Qt::ShiftButton && e->state() & Qt::ControlButton)
  {
    switch(e->key())
    {
      case Qt::Key_Left : changeSelectedIC(-1);
                          break;
      case Qt::Key_Right: changeSelectedIC(1);
                          break;
      //default:            e->ignore();
      //                    return;
    }
  }
  else
    GLSimpleMoleculeView::keyPressEvent(e);
}

///// updateShapes ///////////////////////////////////////////////////////////
void GLMoleculeView::updateShapes()
/// Updates the contents of the shapes vector.
/// Overridden from GLSimpleMoleculeView::updateShapes().
{
  GLSimpleMoleculeView::updateShapes(); // first the shapes of the base class

  ShapeProperties prop;

  ///// surfaces
  for(unsigned int i = 0; i < isoSurface->numSurfaces(); i++)
  {
    prop.id = i;
    if(densityDialog->surfaceType(i) == 0)
      prop.opacity = densityDialog->surfaceOpacity(i);
    else
      prop.opacity = 100;
    prop.type = SHAPE_SURFACE;
    shapes.push_back(prop);
  }
}

///////////////////////////////////////////////////////////////////////////////
///// Private Slots                                                       /////
///////////////////////////////////////////////////////////////////////////////

///// addGLSurface ////////////////////////////////////////////////////////////
void GLMoleculeView::addGLSurface(const unsigned int index)
/// Creates a display list for a new surface.
{
  ///// generate a new display list and save it
  makeCurrent();
  GLuint newList = glGenLists(1);
  glSurfaces.push_back(newList);

  ///// if this is the only surface and no atoms are present: zoomFit
  if(glSurfaces.size() == 1 && atoms->count() == 0)
    zoomFit(false);

  qDebug("creating surface %d", index);
  ///// populate the display list
  updateGLSurface(index);
}

///// updateGLSurface /////////////////////////////////////////////////////////
void GLMoleculeView::updateGLSurface(const unsigned int index)
/// Updates the display list for an existing surface.
{
  makeCurrent();
  QColor surfaceColor = densityDialog->surfaceColor(index);
  unsigned int surfaceOpacity = densityDialog->surfaceOpacity(index);
  Point3D<float> point1, point2, point3, normal1, normal2, normal3;

  qDebug("updating surface %d", index);
  qDebug(" which consists of %d vertices and %d triangles",isoSurface->numVertices(index),isoSurface->numTriangles(index));
  qDebug(" with color %d, %d, %d and opacity %d", surfaceColor.red(), surfaceColor.green(), surfaceColor.blue(), surfaceOpacity);
  glNewList(glSurfaces[index], GL_COMPILE);
    switch(densityDialog->surfaceType(index))
    {
      case 0: // Solid surface
        glBegin(GL_TRIANGLES);
          glColor4d(surfaceColor.red()/255.0, surfaceColor.green()/255.0, surfaceColor.blue()/255, surfaceOpacity/100.0);
	        for(unsigned int i = 0; i < isoSurface->numTriangles(index); i++)
	        {
            isoSurface->getTriangle(index, i, point1, point2, point3, normal1, normal2, normal3);
    		    glNormal3f(normal1.x(), normal1.y(), normal1.z());
            glVertex3f(point1.x(), point1.y(), point1.z());
		        glNormal3f(normal2.x(), normal2.y(), normal2.z());
            glVertex3f(point2.x(), point2.y(), point2.z());
		        glNormal3f(normal3.x(), normal3.y(), normal3.z());
            glVertex3f(point3.x(), point3.y(), point3.z());
	        }
        glEnd();
        break;
      case 1: // Wireframe
        //glLineWidth(1.0);
        {
          double lw, ps;
          glGetDoublev(GL_LINE_WIDTH, &lw);
          glGetDoublev(GL_POINT_SIZE, &ps);
          qDebug("linewidth and pointsize used for generating: %f and %f", lw, ps);
        }
        glBegin(GL_LINES);
          glColor3d(surfaceColor.red()/255.0, surfaceColor.green()/255.0, surfaceColor.blue()/255.0);
	        for(unsigned int i = 0; i < isoSurface->numTriangles(index); i++)
	        {
            isoSurface->getTriangle(index, i, point1, point2, point3, normal1, normal2, normal3);
            glVertex3f(point1.x(), point1.y(), point1.z());
		        glVertex3f(point2.x(), point2.y(), point2.z());
            glVertex3f(point1.x(), point1.y(), point1.z());
            glVertex3f(point3.x(), point3.y(), point3.z());
		        glVertex3f(point2.x(), point2.y(), point2.z());
            glVertex3f(point3.x(), point3.y(), point3.z());
	        }
        glEnd();
        break;
      case 2: // Dots
        glPointSize(1.0);
        glBegin(GL_POINTS);
          glColor3d(surfaceColor.red()/255.0, surfaceColor.green()/255.0, surfaceColor.blue()/255.0);
	        for(unsigned int i = 0; i < isoSurface->numVertices(index); i++)
	        {
            point1 = isoSurface->getPoint(index, i);
            glVertex3f(point1.x(), point1.y(), point1.z());
          }
        glEnd();
    }
  glEndList();
  reorderShapes();
}

///// deleteGLSurface /////////////////////////////////////////////////////////
void GLMoleculeView::deleteGLSurface(const unsigned int index)
/// Deletes the display list for an existing surface.
{
  makeCurrent();
  glDeleteLists(glSurfaces[index], 1);
  std::vector<GLuint>::iterator it = glSurfaces.begin();
  it += index;
  glSurfaces.erase(it);
  reorderShapes();
}

///////////////////////////////////////////////////////////////////////////////
///// Private Member Functions                                            /////
///////////////////////////////////////////////////////////////////////////////

///// translateSelection //////////////////////////////////////////////////////
void GLMoleculeView::translateSelection(const int xRange, const int yRange, const int zRange)
/// Translates the selected atoms according to the screen.
{
  if(selectionList.empty())
    return;

  makeCurrent();
  // needed variables
  GLdouble modelview[16];
  GLdouble projection[16];
  GLint viewport[4];

  // set up the modelview matrix to be the same as in paintGL
  Vector3D<float> axis;
  float angle;
  orientationQuaternion->getAxisAngle(axis, angle);
  glPushMatrix();
  glTranslatef(xPos, yPos, 0.0f);
  glRotatef(angle, axis.x(), axis.y(), axis.z());
  glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
  glPopMatrix();

  // get the other matrices
  glGetDoublev(GL_PROJECTION_MATRIX, projection);
  glGetIntegerv(GL_VIEWPORT, viewport);

  ///// project and unproject the first selected atom (maybe change this tothe center of mass for larger selections...)
  GLdouble x, y, z, xwin, ywin, zwin;
  std::list<unsigned int>::iterator it = selectionList.begin();

  // get the data
  x = atoms->x(*it);
  y = atoms->y(*it);
  z = atoms->z(*it);

  // map the atom coordinates to the window coordinates with gluProject
  if(gluProject(x, y, z, modelview, projection, viewport, &xwin, &ywin, &zwin) == GL_FALSE)
    return;

  // do the translation in window coordinates
  xwin += static_cast<double>(xRange);
  ywin -= static_cast<double>(yRange); // OpenGL coordinate system inverts the y-axis
  if(baseParameters.perspectiveProjection)
    zwin += static_cast<double>(zRange)/10000.0; // (10000 = far clipping distance/near clipping distance)
  else
    zwin += static_cast<double>(zRange)/100.0;
    //zwin += static_cast<double>(zRange)/(2.0f*boundingSphereRadius()); // is very slow!!!

  // map the new window coordinates to the atom coordinates with gluUnproject
  if(gluUnProject(xwin, ywin, zwin, modelview, projection, viewport, &x, &y, &z) == GL_FALSE)
    return;

  // determine the translation vector
  const double dx = x - atoms->x(*it);
  const double dy = y - atoms->y(*it);
  const double dz = z - atoms->z(*it);

  // apply this translation vector to all selected atoms
  while(it != selectionList.end())
  {
    atoms->setX(*it, atoms->x(*it) + dx);
    atoms->setY(*it, atoms->y(*it) + dy);
    atoms->setZ(*it, atoms->z(*it) + dz);
    it++;
  }

  updateAtomSet();
  setModified();
}

///// rotateSelection /////////////////////////////////////////////////////////
void GLMoleculeView::rotateSelection(const double angleX, const double angleY, const double angleZ)
/// Rotates the selected atoms around their local center of mass.
{
  if(selectionList.empty())
    return;

  ///// determine the axis around which to rotate all atoms + the amount of rotation
  ///// first the system should be backrotated from the scene's rotation (orientationQuaternion)
  ///// then the system should be rotated as desired
  ///// finally the system should be rotated to the scene's rotation again
  //* original version working with 3 successive axis/angle rotations
  Vector3D<double> axis;
  double angle;
  Quaternion<double> q(angleX, angleY, angleZ);
  q.getAxisAngle(axis, angle);
  Vector3D<float> axis2;
  float angle2;
  orientationQuaternion->getAxisAngle(axis2, angle2);
  double backAngle = - angle2;
  Vector3D<double> backAxis(axis2.x(), axis2.y(), axis2.z());
  // */
  /* construct the desired quaternions in double precision -> doesn't seem to work correctly
  Quaternion<double> orientation(static_cast<double>(orientationQuaternion->w()), static_cast<double>(orientationQuaternion->x()),
                               static_cast<double>(orientationQuaternion->y()), static_cast<double>(orientationQuaternion->z()));
  Quaternion<double> backOrientation(orientation);
  backOrientation.inverse();
  Quaternion<double> rotation(angleX, angleY, angleZ);
  Quaternion<double> totalRotation = backOrientation * rotation * orientation;
  // get the axis/angle representation
  Vector3D<double> axis;
  double angle;
  totalRotation.getAxisAngle(axis, angle);
  */
  if(fabs(angle) < Point3D<double>::TOLERANCE)
    return;

  ///// determine the local center of mass (all masses are taken equal)
  Point3D<double> centerOfMass(0.0, 0.0, 0.0);
  std::list<unsigned int>::iterator it = selectionList.begin();
  while(it != selectionList.end())
    centerOfMass.add(atoms->coordinates(*it++));
  centerOfMass.setValues(centerOfMass.x()/selectionList.size(), centerOfMass.y()/selectionList.size(), centerOfMass.z()/selectionList.size());
  //qDebug("centerOfMass = %f, %f, %f", centerOfMass.x(), centerOfMass.y(), centerOfMass.z());

  ///// rotate the atoms around this center
  it = selectionList.begin();
  while(it != selectionList.end())
  {
    Vector3D<double> v(centerOfMass, atoms->coordinates(*it));
    //v.rotate(axis, angle);
    v.rotate(backAxis, backAngle);
    v.rotate(axis, angle);
    v.rotate(backAxis, -backAngle);
    atoms->setX(*it, centerOfMass.x() + v.x());
    atoms->setY(*it, centerOfMass.y() + v.y());
    atoms->setZ(*it, centerOfMass.z() + v.z());
    it++;
  }
  updateAtomSet();
  setModified();
}

///// changeSelectedIC ////////////////////////////////////////////////////////
void GLMoleculeView::changeSelectedIC(const int range)
/// Changes the selected internal coordinate
/// according to the magnitude and direction of the range.
{
  if(range == 0)
    return;

  unsigned int atom1, atom2, atom3, atom4;
  std::list<unsigned int>::iterator it = selectionList.begin();
  switch(selectionType)
  {
    case SELECTION_BOND:    atom1 = *it++;
                            atom2 = *it;
                            atoms->changeBond(static_cast<double>(range) * 0.1, atom1, atom2, true);
                            break;

    case SELECTION_ANGLE:   atom1 = *it++;
                            atom2 = *it++;
                            atom3 = *it;
                            atoms->changeAngle(180.0 * static_cast<double>(range) / static_cast<double>(width()), atom1, atom2, atom3, true);
                            break;
    case SELECTION_TORSION: atom1 = *it++;
                            atom2 = *it++;
                            atom3 = *it++;
                            atom4 = *it;
                            atoms->changeTorsion(-180.0 * static_cast<double>(range) / static_cast<double>(width()), atom1, atom2, atom3, atom4, true);
                            break;
  }
  updateAtomSet();
  setModified();
}

///// drawItem ////////////////////////////////////////////////////////////////
void GLMoleculeView::drawItem(const unsigned int index)
/// Draws the item shapes[index].
{
  if(shapes[index].type != SHAPE_SURFACE)
    return; // this routine only draws isosurfaces at the moment

  const unsigned int currentSurface = shapes[index].id;
  if(currentSurface >= isoSurface->numSurfaces())
    return; // asked to draw a non-existing surface

  if(densityDialog->surfaceVisible(currentSurface))
  {
    if(densityDialog->surfaceType(currentSurface) == 0)
    {
      glCallList(glSurfaces[currentSurface]);
    }
    else
    {
      glDisable(GL_LIGHTING);
      glCallList(glSurfaces[currentSurface]);
      glEnable(GL_LIGHTING);
    }
  }
}

