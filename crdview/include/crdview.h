/***************************************************************************
                          crdview.h  -  description
                             -------------------
    begin                : Tue Jan 13 2004
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
/// Contains the declaration of the class CrdView.

#ifndef CRDVIEW_H
#define CRDVIEW_H

///// Forward class declarations & header files ///////////////////////////////

// Qt forward class declarations
class QAction;
class QPopupmenu;

// CrdView forward class declarations and includes
class AtomSet;
class GLSimpleMoleculeView;
class PreferencesCV;
#include "glbaseparameters.h"
#include "glmoleculeparameters.h"
#include "iconsets.h"

// CrdView base class header file
#include <qmainwindow.h>

///// class CrdView ///////////////////////////////////////////////////////////
class CrdView : public QMainWindow
{
  Q_OBJECT

  public:
    CrdView();                          // constructor
    ~CrdView();                         // destructor

    void readCoordinates(QString filename = QString::null); // loads the given filename for viewing or asks for one if not specified

  private slots:
    void fileOpen();                    // Opens a coordinate file
    void fileSave();                    // Saves a coordinate file
    void fileExport();                  // Saves the current view as an image
    void filePreferences();             // Sets up the preferences
    void viewToolBar(bool toggle);      // Toggles the visibility of the toolbar
    void viewStatusBar(bool toggle);    // Toggles the visibility of the statusbar
    void helpHelp();                    // Shows help
    void helpWhatsThis();               // Enters What's This mode
    void helpAbout();                   // Shows the About dialog box

  //protected:
    //void showEvent(QShowEvent*);        // re-updates the GLMoleculeView in case of preloading

  private:
    ///// private member functions
    void initActions();                 // creates all actions
    void initMenuBar();                 // creates the menu
    void initToolBar();                 // creates the toolbar(s)
    void readSettings();                // reads the Brabosphere and CrdView settings
    void saveSettings();                // saves the CrdView settings
    QString actionText(const QString title, const QString brief, const QString details = QString::null, const IconSets::IconSetID iconID = IconSets::LastIcon); // constructs a text for the What's This mode for actions

    ///// private variables
    // toolbars
    QToolBar* ToolBarFile;              // the toolbar containing shortcuts to the File menu
    // actions: menu File
    QAction* actionFileOpen;
    QAction* actionFileSave;
    QAction* actionFilePreferences;
    QAction* actionFileExport;
    QAction* actionFileQuit;
    // actions: menu View
    QAction* actionViewToolBar;
    QAction* actionViewStatusBar;
    // actions: menu Reset
    QAction* actionCenterView;
    QAction* actionResetOrientation;
    QAction* actionZoomFit;
    QAction* actionResetView;
    // actions: other
    QAction* actionSelectAll;
    QAction* actionSelectNone;
    QAction* actionAnimate;
    // actions: menu Help
    QAction* actionHelp;
    QAction* actionWhatsThis;
    QAction* actionAbout;
    // Xbrabo classes
    AtomSet* atoms;
    GLSimpleMoleculeView* glview;
};
#endif

