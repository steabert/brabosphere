/***************************************************************************
                          glview.h  -  description
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
/// Contains the declaration of the class GLView

#ifndef GLVIEW_H
#define GLVIEW_H

///// Forward class declarations & header files ///////////////////////////////

// Qt forward class declarations
class QPoint;
class QTimer;

// Xbrabo forward class declarations
#include <glbaseparameters.h>  // struct GLBaseParameters can't be declared forward
template <class T> class Quaternion;

// Base class header file
#include <qgl.h>

///// class GLView ////////////////////////////////////////////////////////////
class GLView : public QGLWidget
{
  Q_OBJECT
  
  public:
    ///// constructor/destructor
    GLView(QWidget *parent=0, const char *name=0);// constructor
    virtual ~GLView();                          // destructor

    ///// public member functions
    bool isModified() const;            // returns whether the scene needs to be saved
    bool isAnimating() const;           // returns whether the scene is animating
    unsigned int calculateFPS();        // returns the maximum framerate for the current parameters

    ///// static public member functions
    static void setParameters(GLBaseParameters params);     // sets new OpenGL parameters

  signals:
    void modified();                    // is emitted when the status changes from non-modified to modified
    void changed();                     // is emitted everytime something changes

  public slots:
    void setModified(const bool status = true);   // sets the 'modified' status of the scene
    void toggleAnimation();             // turns animation on/off
    void centerView(const bool update = true);    // centers the scene (through xPos and yPos)
    void resetOrientation(const bool update = true);        // resets the orientation
    void zoomFit(const bool update = true);      // zooms so the scene fits the window
    void resetView(const bool update = true);     // resets translation/orientation/zoom
    void saveImage();           // saves the current view to an image

  protected:
    ///// protected member functions
    void initializeGL();                // called once upon initialization
    void resizeGL(int w, int h);        // called when the widget is resized
    void paintGL();                     // called when the widget has to be repainted
    void mousePressEvent(QMouseEvent* e);         // event which takes place when a mouse button is pressed
    void mouseMoveEvent(QMouseEvent* e);// event which takes place when the mouse is moved while a mousebutton is pressed
    void mouseReleaseEvent(QMouseEvent* e);       // event which takes place when a mouse button was released on the widget
    void keyPressEvent(QKeyEvent* e);   // event which takes place when a key was pressed
    void wheelEvent(QWheelEvent* e);    // event which takes place when the scrollwheel of the mouse is used
    virtual void drawScene() = 0;       // should contain the actual OpenGL drawing code for the scene
    void translateZ(const int amount);    // handles Z-direction translations
    void translateXY(const int amountX, const int amountY);   // handles X- and Y-direction translations
    virtual void clicked(const QPoint&);// handles left-mouse clicks at position
    virtual void updateGLSettings();    // updates the GL View according to parameters
    virtual float boundingSphereRadius() = 0;     // calculates the radius of the bounding sphere needed for zoomFit
    void updateFog(const float radius); // updates the fog parameters
    void updateProjection();            // does the necessary updating when the projection type changes
    void setPerspective();              // sets the perspective
    
    ///// protected member data
    GLfloat xPos;                       ///< Amount of translation on the x-axis.
    GLfloat yPos;                       ///< Amount of translation on the y-axis.
    GLfloat zPos;                       ///< Zoomfactor = distance camera from center.
    Quaternion<float>* orientationQuaternion;     ///< Orientation of the molecule in 4D.
    //GLfloat aspectRatio;                ///< Needed in resizeGL and selectEntity.
    QPoint mousePosition;               ///< Position of the mouse.

    ///// protected constants (made static for ease)
    static const int redrawWait;                  ///< Number of msec to wait between updates (for a max of 30 FPS).
    static const float fieldOfView;               ///< Field of view for gluPerspective and slotZoomFit.
    static GLBaseParameters baseParameters;       ///< Holds the OpenGL base parameters.
    
  private:
    
    ///// private member data
    GLfloat xRot;                       ///< Amount of step-rotation around the x-axis.
    GLfloat yRot;                       ///< Amount of step-rotation around the y-axis.
    GLfloat zRot;                       ///< Amount of step-rotation around the z-axis.
    QTimer* timer;                      ///< Timer object.
    bool animation;                     ///< Is true if the scene should be animated.
    int updateIndex;                    ///< Holds the index of the latest local update.
    bool viewModified;                  ///< Holds the 'modified' status of the scene.
    bool startingClick;                 ///< Keeps track of click vs. move events.
    float maxRadius;                    ///< A copy of the result of boundingSphereRadius for use in translateZ
    bool currentPerspectiveProjection;  ///< Is true if the current projection is perspective

    ///// static private member data
    static int staticUpdateIndex;       ///< holds the index of the latest update of the OpenGL parameters.
};

#endif

