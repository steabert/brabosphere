/***************************************************************************
                          glview.cpp  -  description
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
  \class GLView
  \brief This class serves as a base class for the OpenGL visualization classes.

  These currently are:
  \arg GLSimpleMoleculeView
  \arg GLMoleculeView
  \arg GLOrbitalView

  It implements common routines like OpenGL-setup, mouse-interaction,
  keyboard-interaction, image-saving and animation. It contains a pure virtual
  so has to be subclassed.

*/
/// \file
/// Contains the implementation of the class GLView

///// Header files ////////////////////////////////////////////////////////////

// Qt header files
#include <qdatetime.h>
#include <qfiledialog.h>
#include <qimage.h>
#include <qmessagebox.h>
#include <qpoint.h>
#include <qstringlist.h>
#include <qtimer.h>

// Xbrabo header files
#include "glview.h"
#include "quaternion.h"
#include "vector3d.h"

///////////////////////////////////////////////////////////////////////////////
///// Public Member Functions                                             /////
///////////////////////////////////////////////////////////////////////////////

///// constructor /////////////////////////////////////////////////////////////
GLView::GLView(QWidget* parent, const char* name) : QGLWidget(parent,name),
/// The default constructor.
  xPos(0.0f),
  yPos(0.0f),
  zPos(0.0f),
  //aspectRatio(1.0f),
  xRot(0.0f),
  yRot(0.0f),
  zRot(0.0f),
  animation(false),
  maxRadius(1.0f),
  currentPerspectiveProjection(baseParameters.perspectiveProjection)
{
  setFocusPolicy(QWidget::StrongFocus); // needed to receive all keystrokes
  orientationQuaternion = new Quaternion<float>(0.0f, 0.0f, 0.0f);
  timer = new QTimer(this);
  connect(timer, SIGNAL(timeout()), this, SLOT(update()));
  setModified(false);
  //resetView(false); // can't call this as it indirectly calles the pure virtual boundingSphereRadius
                      // which is not defined yet for the dervived class which calls this constructor
                      // in its constructor
  updateIndex = staticUpdateIndex - 1; // an update is needed
}

///// destructor //////////////////////////////////////////////////////////////
GLView::~GLView()
/// The destructor.
{
  delete orientationQuaternion;
}

///// isModified //////////////////////////////////////////////////////////////
bool GLView::isModified() const
/// Returns whether the view has been modified and possibly needs to be saved.
{
  return viewModified;
}

///// isAnimating /////////////////////////////////////////////////////////////
bool GLView::isAnimating() const
/// Returns whether the scene is animating.
{
  return animation;
}

///// calculateFPS ////////////////////////////////////////////////////////////
unsigned int GLView::calculateFPS()
/// Returns the maximum attainable number of frames per
/// second. It draws as much frames as possible in 5 seconds.
{
  unsigned int numFPS = 0;        // the counter for the FPS
  const int nummSec = 5000;             // the number of milliseconds measured
  QTime t;                              // the time object

  ///// backup
  bool oldAnimation = animation;
  Quaternion<float> oldOrientation = *orientationQuaternion;

  ///// start
  animation = false;
  t.start();
  while (t.elapsed() < nummSec)
  {
    xRot = 1.00;
    yRot = 1.25;
    zRot = 1.50;
    updateGL();
    numFPS++;
  }

  ///// restore
  animation = oldAnimation;
  *orientationQuaternion = oldOrientation;
  updateGL();

  ///// result
  return numFPS*1000/nummSec;
}

///// setParameters ///////////////////////////////////////////////////////////
void GLView::setParameters(GLBaseParameters params)
/// Updates the OpenGL parameters and
/// makes sure they are adapted at the next call to updateGL().
{
  baseParameters = params;
  staticUpdateIndex++;
}

 //////////////////////////////////////////////////////////////////////////////
///// Public Slots                                                        /////
///////////////////////////////////////////////////////////////////////////////

///// setModified /////////////////////////////////////////////////////////////
void GLView::setModified(const bool status)
/// Sets the 'modified' status of the scene. Defaults to true.
{
  if(!status)
  {
    viewModified = false;
    return;
  }
  if(!viewModified)
  {
    viewModified = true;
    emit modified();
  }
  emit changed();
}

///// toggleAnimation /////////////////////////////////////////////////////////
void GLView::toggleAnimation()
/// Toggles animation on/off.
{
  animation = !animation;
  if(animation)
  {
    xRot = 1.0;
    yRot = 0.0;
    zRot = 0.0;
    updateGL();
  }
  else
    timer->stop();

  emit changed(); // don't call setModified as animation does not get saved/restored
}

///// centerView //////////////////////////////////////////////////////////////
void GLView::centerView(const bool update)
/// Centers the view on the scene.
{
  xPos = 0.0;
  yPos = 0.0;
  if(update)
    updateGL();
  setModified();
}

///// resetOrientation ////////////////////////////////////////////////////////
void GLView::resetOrientation(const bool update)
/// Resets the orientation of the scene.
{
  orientationQuaternion->eulerToQuaternion(0.0f, 0.0f, 0.0f);
  if(update)
    updateGL();
  setModified();
}

///// zoomFit /////////////////////////////////////////////////////////////////
void GLView::zoomFit(const bool update)
/// Calculates zPos so that the scene fits in the current window
/// It uses the result of boundingSphereRadius, a pure virtual
{
  ///// determine the radius of the bounding sphere
  maxRadius = boundingSphereRadius();
  ///// calculate the camera position
  if(baseParameters.perspectiveProjection)
  {
    if(width() > height())
      zPos = maxRadius/tan(fieldOfView)/1.5f;
    else
      zPos = maxRadius/tan(fieldOfView)/1.5f * static_cast<float>(height())/static_cast<float>(width());
    if(zPos < 0.1f)
      zPos = 0.1f;
  }
  else
  {
    zPos = 1.0f;
    resizeGL(width(), height());
  }

  ///// update the scene
  updateFog(maxRadius);
  if(update)
    updateGL();
  setModified();
}

///// resetView //////////////////////////////////////////////////////////////
void GLView::resetView(const bool update)
/// Resets translation/orientation/zoom of the scene.
{
  centerView(false);
  resetOrientation(false);
  zoomFit(); // calls setModified()
  if(update)
    updateGL();
}

///// saveImage ///////////////////////////////////////////////////////////////
void GLView::saveImage()
/// Saves the view to an image.
{
  ///// get the available output formats
  QStringList saveFormats = QImage::outputFormatList();
  QStringList::Iterator it = saveFormats.begin();
  while(it != saveFormats.end())
  {
    if((*it) == "JPEG")
      *it = "JPEG (*.jpg)";
    else
      *it += " (*."+(*it).lower()+")";  // 'BMP' -> 'BMP (*.bmp)'
    it++;
  }

  ///// set up and show the dialog
  /*
  QFileDialog saveDialog("", QString::null, this, 0, true);
  saveDialog.setFilters(saveFormats);
  saveDialog.setSelectedFilter("PNG (*.png)");
  saveDialog.setCaption(tr("Choose a filename and format"));
  saveDialog.setMode(QFileDialog::AnyFile);
  if(saveDialog.exec() != QDialog::Accepted)
    return;
  ///// get the filename and the desired extension/filter
  QString filename = saveDialog.selectedFile();
  if(filename.isEmpty())
    return;
  */
  // this way does not allow to set the selected filter. maybe put PNG at the top manually. now BMP is the default
  QString selectedFilter;
  QString filename = QFileDialog::getSaveFileName(0, saveFormats.join(";;"), 0, 0, tr("Choose a filename and format"), &selectedFilter);
  if(filename.isEmpty())
    return;

  //QString extension = saveDialog.selectedFilter();
  //extension = extension.mid(extension.find("."));
  //extension = extension.remove(")");
  //qDebug("extension: "+extension);
  QString extension = selectedFilter.mid(selectedFilter.find(".")).remove(")");

  if(filename.contains(extension) == 0)
    filename += extension;

  //QString format = saveDialog.selectedFilter();
  //format = format.left(format.find(" "));
  QString format = selectedFilter.left(selectedFilter.find(" "));

  // generate an image from the OpenGL view
  // -> It is possible to get a transparant image when using grabFrameBuffer(true)
  QImage image = grabFrameBuffer();

  // save it
  if(!image.save(filename, format))
    QMessageBox::warning(this, tr("Save image"), tr("An error occured. Image is not saved"));
}

///////////////////////////////////////////////////////////////////////////////
///// Protected Member Functions                                          /////
///////////////////////////////////////////////////////////////////////////////

///// initializeGL ////////////////////////////////////////////////////////////
void GLView::initializeGL()
/// Overridden from QGLWidget::initializeGL().
/// Initializes the OpenGL window.
{
  GLfloat lightAmbient[] = {0.2f, 0.2f, 0.2f, 0.0f};
  GLfloat lightDiffuse[] = {0.5f, 0.5f, 0.5f, 0.0f};
  GLfloat lightSpecular[] = {1.0f, 1.0f, 1.0f, 0.0f};

  updateGLSettings();

  ///// lighting
  ///// lighting model
  //glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE);         // disable twosided lighting (default)
  //glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_FALSE);     // disable local viewpoint (default)
  //glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lightAmbient);     // set the ambient light (default)
  ///// setup light 0
  glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);     // enable ambient
  glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);     // enable diffuse
  glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpecular);    // enable specular
  glEnable(GL_LIGHTING);                // enable lighting
  glEnable(GL_LIGHT0);                  // enable light 0

  glEnable(GL_AUTO_NORMAL);
  glEnable(GL_NORMALIZE); // maybe this copes with the non-uniform scaling of the bonds => nope

  /////every change of glColor sets a new diffuse+ambient color
  glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
  glEnable(GL_COLOR_MATERIAL);

  glEnable(GL_DITHER);                  // enable dithering (default)
  glEnable(GL_DEPTH_TEST);              // use the depth buffer for hidden surface removal

  glCullFace(GL_BACK);                 // we will only see the outsides of objects
  glEnable(GL_CULL_FACE);               // => enable culling

  ///// AA setup
  //glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);                // antialiasing method
  //glBlendFunc(GL_SRC_ALPHA_SATURATE, GL_ONE);               // set the proper blending mode
  ///// AA for lines
  glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
  //glEnable(GL_LINE_SMOOTH);
  ///// AA for points
  glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
  //glEnable(GL_POINT_SMOOTH);

  // transparancy setup
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  ///// Fog setup
  glFogi(GL_FOG_MODE, GL_LINEAR);

  ///// other
  glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);        // color interpolation method
}

///// resizeGL ////////////////////////////////////////////////////////////////
void GLView::resizeGL(int w, int h)
/// Overridden from QGlWidget::resizeGL().
/// Resizes the OpenGL viewport according to the size of the widget.
{
  glViewport(0, 0, w, h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  //aspectRatio = static_cast<float>(w) / static_cast<float>(h);
  setPerspective(); // calls gluPerspective or glOrtho depending on the prespective setting
  glMatrixMode(GL_MODELVIEW);
}

///// paintGL /////////////////////////////////////////////////////////////////
void GLView::paintGL()
/// Overridden from QGlWidget::paintGL(). Does the drawing of the OpenGL scene.
{
  if(staticUpdateIndex != updateIndex)
    updateGLSettings();

  ///// build a quaternion from the user's changes
  Quaternion<float> changeQuaternion(xRot, yRot, zRot);
  ///// multiply it with the current quaternion
  Quaternion<float> tempQuaternion = *orientationQuaternion;
  *orientationQuaternion = tempQuaternion*changeQuaternion;  // no *= operator implemented yet => calling default constructor?
  ///// generate an axis-angle representation for OpenGL
  Vector3D<float> axis;
  float angle;
  orientationQuaternion->getAxisAngle(axis, angle);

  if(!animation)
  {
    ///// reset the changes
    xRot = 0.0f;
    yRot = 0.0f;
    zRot = 0.0f;
  }

  //if(baseParameters.antialias)
  //{
  //  glClear(GL_COLOR_BUFFER_BIT);
  //}
  //else
  //{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  //}

  glLoadIdentity();
  //qDebug("GlView::xPos = %f, yPos = %f, zPos = %f", xPos, yPos, zPos);
  //qDebug("GlView::orientationQuaternion = (%f, %f, %f, %f)",orientationQuaternion->x(),orientationQuaternion->y(),orientationQuaternion->z(),orientationQuaternion->w());
  ///// camera setup
  if(baseParameters.perspectiveProjection)
    gluLookAt(0.0f, 0.0f, zPos, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
  else
    resizeGL(width(), height());
  glPushMatrix();
  glTranslatef(xPos, yPos, 0.0f);
  glRotatef(angle, axis.x(), axis.y(), axis.z());

  drawScene(); // pure virtual

  glPopMatrix();
  glFlush(); // drawing is complete => send for execution
  if(animation)
    timer->start(redrawWait, true);
}

///// mousePressEvent /////////////////////////////////////////////////////////
void GLView::mousePressEvent(QMouseEvent* e)
/// Overridden from QGlWidget::mousePressEvent. Handles left mouse button presses.
{
  if(e->button() & Qt::LeftButton) // only track mousemoves for the left button
  {
    mousePosition = e->pos(); // set the starting position of the move
    startingClick = true;
  }
  else
    e->ignore();
}

///// mouseMoveEvent //////////////////////////////////////////////////////////
void GLView::mouseMoveEvent(QMouseEvent* e)
/// Overridden from QGlWidget::mouseMoveEvent. Handles left mouse button drags.
{
  if(e->state() & Qt::LeftButton) // only track mousemoves for the left button
  {
    QPoint newPosition = e->pos(); // the mouse moved from mousePosition to newPosition

    if(e->state() & Qt::ShiftButton) // shift has precedence over control
    {
      ///// LEFTBUTTON + SHIFT
      ///// up/down movement: zooming (z-translation)
      ///// left/right movement: z-rotation
      ///// decoupled
      if(abs(newPosition.y() - mousePosition.y()) > abs(newPosition.x() - mousePosition.x()))
        translateZ(newPosition.y() - mousePosition.y());
      else
        zRot = -180.0f * static_cast<float>(newPosition.x() - mousePosition.x()) / static_cast<float>(width()); // z-rotation of entire system
    }
    else if(e->state() & Qt::ControlButton)
    {
      ///// LEFTBUTTON + CONTROL
      ///// up/down movement: y-translation
      ///// left/right movement: x-translation
      ///// coupled
      translateXY(newPosition.x() - mousePosition.x(), newPosition.y() - mousePosition.y());
    }
    else
    {
      ///// LEFTBUTTON ONLY
      ///// up/down movement: x-rotation
      ///// left/right movement: y-rotation
      yRot = 180.0f * static_cast<float>(newPosition.x() - mousePosition.x()) / static_cast<float>(width());
      xRot = 180.0f * static_cast<float>(newPosition.y() - mousePosition.y()) / static_cast<float>(height());
    }
    setModified();
    mousePosition = newPosition;
    updateGL();
    startingClick = false;
  }
  else
    e->ignore();
}

///// mouseReleaseEvent ///////////////////////////////////////////////////////
void GLView::mouseReleaseEvent(QMouseEvent* e)
/// Overridden from QGlWidget::mouseReleaseEvent.
/// Handles left mouse button releases.
{
  if(e->button() & Qt::LeftButton) // only track mousemoves for the left button
  {
    if(startingClick)
    {
      ///// this release is not the end of a move event => position is clicked
      clicked(mousePosition);
    }
  }
  else
    e->ignore(); //event will be handled by the parent widget
}

///// keyPressEvent ///////////////////////////////////////////////////////////
void GLView::keyPressEvent(QKeyEvent* e)
/// Overridden from QGlWidget::mousepressEvent. Handles key presses.
/// \arg <left>         : rotate left
/// \arg <right>        : rotate right
/// \arg <up>           : rotate up
/// \arg <down>         : rotate down
/// \arg <shift>+<left> : rotate counterclockwise
/// \arg <shift>+<right>: rotate clockwise
/// \arg <shift>+<up>   : zoom in
/// \arg <shift>+<down> : zoom out
/// \arg <ctrl>+<left>  : translate left
/// \arg <ctrl>+<right> : translate right
/// \arg <ctrl>+<up>    : translate up
/// \arg <ctrl>+<down>  : translate down
/// \arg <ctrl>+<shift>+<left>: change internal coordinate of selection (smaller). Implementation in GLMoleculeView.
/// \arg <ctrl>+<shift>+<right>: change internal coordinate of selection (larger). Implementation in GLMoleculeView.
{
  switch(e->key())
  {
    case Qt::Key_Left :   if(e->state() & Qt::ShiftButton)
                            zRot = 5.0f; // rotate counterclockwise
                          else if(e->state() & Qt::ControlButton)
                            translateXY(-5, 0);
                          else
                            yRot = -5.0f; // rotate left
                          break;

    case Qt::Key_Up     : if(e->state() & Qt::ShiftButton)
                            translateZ(-5);
                          else if(e->state() & Qt::ControlButton)
                            translateXY(0, -5);
                          else
                             xRot = -5.0f; // rotate up
                          break;

    case Qt::Key_Right  : if(e->state() & Qt::ShiftButton)
                            zRot = -5.0f; // rotate clockwise
                          else if(e->state() & Qt::ControlButton)
                            translateXY(5, 0);
                          else
                            yRot =  5.0f; // rotate right
                          break;

    case Qt::Key_Down   : if(e->state() & Qt::ShiftButton)
                            translateZ(5);
                          else if(e->state() & Qt::ControlButton)
                            translateXY(0, 5);
                          else
                            xRot = 5.0f; // rotate down
                          break;

    default:              e->ignore();
                          return;
  }
  setModified();
  updateGL();
}

///// wheelEvent //////////////////////////////////////////////////////////////
void GLView::wheelEvent(QWheelEvent* e)
/// Overridden from QGlWidget::wheelEvent. Handles scrolls with the scrollwheel
/// of the mouse. It provides an alternative way of zooming.
{
  translateZ(-e->delta()/4); // abs(e->delta()) is always WHEELDELTA == 120
  setModified();
  updateGL();
  e->accept();
}

///// translateZ //////////////////////////////////////////////////////////////
void GLView::translateZ(const int amount)
/// Handles the request for a translation in
/// the Z-direction. Here the complete scene is translated (=zoomed).
{
  // a zoom over a distance of the height of the OpenGL window should translate
  // in half the size of the molecule
  if(amount != 0)
  {
    float zoomFactor = static_cast<float>(amount)/height(); // percentage
    if(baseParameters.perspectiveProjection)
      zoomFactor *= 2.0f * maxRadius;
    zPos += zoomFactor;
    if(zPos < 0.1f)
      zPos = 0.1f;
    if(!baseParameters.perspectiveProjection)
      resizeGL(width(), height()); // zooming for ortho projection is in fact direct scaling of the view
  }
}

///// translateXY /////////////////////////////////////////////////////////////
void GLView::translateXY(const int amountX, const int amountY)
/// Handles the request for a translation in
/// the X- or Y-direction. Here the complete scene is translated.
{
  if(amountX != 0)
    xPos += amountX < 0 ? -0.1f : 0.1f;
  if(amountY != 0)
    yPos += amountY > 0 ? -0.1f : 0.1f;
}

///// clicked /////////////////////////////////////////////////////////////////
void GLView::clicked(const QPoint&)
/// Handles leftbutton mouse clicks at the given position.
/// No implementation in this base class. Not a pure virtual because it does
/// not need an implementation by the derived classes.
{
}

///// updateGLSettings ////////////////////////////////////////////////////////
void GLView::updateGLSettings()
/// Updates the OpenGL settings.
/// It is only called when an update is really needed (determined by
/// staticUpdateIndex != updateIndex).
{
  updateIndex = staticUpdateIndex;

  ///// light position, specular and shininess
  GLfloat lightPosition[] = {baseParameters.lightPositionX, baseParameters.lightPositionY, baseParameters.lightPositionZ, 0.0f};
  GLfloat materialSpecular[] = {baseParameters.materialSpecular/100.0f, baseParameters.materialSpecular/100.0f, baseParameters.materialSpecular/100.0f, 0.0f};
  GLfloat materialShininess[] = {baseParameters.materialShininess};

  glLightfv(GL_LIGHT0, GL_POSITION, lightPosition); // set light position for directional light
  glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, materialSpecular);
  glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, materialShininess);

  ///// background color
  //qglClearColor(baseParameters.backgroundColor);
  qglClearColor(QColor(baseParameters.backgroundColor));

  ///// smooth shading
  if(baseParameters.smoothShading)
    glShadeModel(GL_SMOOTH);
  else
    glShadeModel(GL_FLAT);

  ///// antialiasing
  if(baseParameters.antialias)
  {
    //polygon antialiasing is way to slow.
    //glEnable(GL_BLEND);
    //glEnable(GL_POLYGON_SMOOTH);
    //glDisable(GL_DEPTH_TEST);
    // antialiasing for lines and points.
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_POINT_SMOOTH);
  }
  else
  {
    //glDisable(GL_BLEND);
    //glDisable(GL_POLYGON_SMOOTH);
    //glEnable(GL_DEPTH_TEST);
    glDisable(GL_LINE_SMOOTH);
    glDisable(GL_POINT_SMOOTH);
  }

  ///// depth-cueing
  if(baseParameters.depthCue)
    glEnable(GL_FOG);
  else
    glDisable(GL_FOG);

  updateProjection(); // pure virtual which takes care of a change in projection

  ///// update the projection
  resizeGL(width(), height());
}

///// updateFog ///////////////////////////////////////////////////////////////
void GLView::updateFog(const float radius)
/// Updates the fog parameters depending on the
/// size of the object that's to be depth-cue'd.
{
  makeCurrent();
  GLfloat fogStart = zPos;
  GLfloat fogEnd = zPos + 2.0*radius;
  //qDebug("setting fog from %f to %f",fogStart, fogEnd);
  glFogf(GL_FOG_START, fogStart);
  glFogf(GL_FOG_END, fogEnd);
}

///// updateProjection ////////////////////////////////////////////////////////
void GLView::updateProjection()
/// Checks whether the projection type changed and does the necessary updating.
{
  if(currentPerspectiveProjection == baseParameters.perspectiveProjection)
    return;

  resizeGL(width(), height());
  zoomFit();

  currentPerspectiveProjection = baseParameters.perspectiveProjection;
}

///// setPerspective //////////////////////////////////////////////////////////
void GLView::setPerspective()
/// Sets the correct perspective.
{
  GLfloat aspectRatio = static_cast<float>(width()) / static_cast<float>(height());
  if(baseParameters.perspectiveProjection)
    gluPerspective(fieldOfView, aspectRatio, 0.1f, 100.0f); // originally 0.01f and 100.0f but artifacts from intersecting triangles
                                                            // are greatly reduced. Problem was the inaccuracy of the Z-buffer
                                                            // best results are obtained with a ratio near/far as low as possible
  else
    glOrtho(-maxRadius*aspectRatio*zPos, maxRadius*aspectRatio*zPos, -maxRadius*zPos, maxRadius*zPos, -maxRadius, maxRadius);
}

///////////////////////////////////////////////////////////////////////////////
///// Static Variables                                                    /////
///////////////////////////////////////////////////////////////////////////////

int GLView::staticUpdateIndex = 0;
const int GLView::redrawWait = 33;
const float GLView::fieldOfView = 60.0f;

///// MSVC .NET 2002 cannot compile static initializers when non-trivial constructors
///// are used for the members of (non)aggregates. Should be fixed in .NET 2003
//  OpenGLParameters GLMoleculeView::parameters = {1.0, 1.0, 1.0, QColor(255, 255, 255), 0.80, 100.0,
//                                                 QColor(0, 0, 0), false, true, true, 5, true, 0, 0.2, 3.0};
GLBaseParameters GLView::baseParameters = {1.0f, 1.0f, 1.0f, 0xffffff, 0.80f, 100.0f,
                                           0x000000, false, true, false, true};

