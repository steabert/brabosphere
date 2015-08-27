/***************************************************************************
                          xbrabo.h  -  description
                             -------------------
    begin                : Fri Jul 19 2002
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
/// Contains the declaration of the class Xbrabo.

#ifndef XBRABO_H
#define XBRABO_H

///// Forward class declarations & header files ///////////////////////////////

// Qt forward class declarations
class QAction;
class QPopupMenu;

// Xbrabo forward class declarations
#include "iconsets.h"
class PreferencesBase;
class XbraboView;

// Base class header file
#if defined(USE_KMDI) || defined(USE_KMDI_DLL)
#  include<kmdimainfrm.h>
#  define QextMdiMainFrm KMdiMainFrm
#else
  #include <qextmdimainfrm.h>
#endif

///// class Xbrabo ////////////////////////////////////////////////////////////
class Xbrabo : public QextMdiMainFrm
{
  Q_OBJECT
  
  public:
    ///// constructor/destructor
    Xbrabo();                           // constructor
    ~Xbrabo();                          // destructor

    ///// public members
    void init();                        // initializes everything
    void fileOpen(const QString filename);        // opens a calculation with the specified name

  protected:
    bool event(QEvent* e);              // handles MDI child close events
    void resizeEvent(QResizeEvent* );   // fits the system menu button position to the menu position
    void closeEvent(QCloseEvent* e);    // closes the application

  private slots:
    ///// file menu
    void fileNew();                     // creates a new calculation
    void fileOpen();                    // opens an existing calculation and asks a filename (overloaded)
    void fileSave();                    // saves a calculation
    void fileSaveAs();                  // saves a calculation under a different name
    void fileClose();                   // closes the active calculation

    ///// edit menu
    void editCut();                     // deletes the marked text and put it on the clipboard
    void editCopy();                    // puts the marked text on the clipboard
    void editPaste();                   // pastes the contents of the clipboard
    void editPrefs();                   // changes program references

    ///// view menu
    void viewToolBarStandard();         // toggles the Standard toolbar
    void viewToolBarCalculation();      // toggles the Calculation toolbar
    void viewToolBarCoordinates();      // toggles the Coordinates toolbar    
    void viewStatusBar();               // toggles the statusbar
    void viewTaskBar();                 // toggles the taskbar

    ///// molecule menu
    void moleculeReadCoordinates();     // reads coordinates from disk
    void moleculeCenterView();          // centers the molecule in the view
    void moleculeResetOrientation();    // resets the orientation of the molecule
    void moleculeZoomFit();             // resets the zoom of the molecule
    void moleculeResetView();           // resets translation/orientation/zoom of the molecule
    void moleculeAnimate();             // toggles animation of the molecule on/off
    void moleculeFPS();                 // calculates the FPS with the current view parameters
    void moleculeImage();               // saves the current view to an image
    void moleculeAddAtoms();            // allows atoms to be added
    void moleculeDeleteSelection();     // deletes the selected atoms
    void moleculeDensity();             // changes the density isosurfaces
    void moleculeDisplayMode();         // changes the molecular display mode
    void moleculeAlterCartesian();      // alters the cartesian coordinates of the selected atoms
    void moleculeAlterInternal();       // alters the selected internal coordinate
    void moleculeSelectAll();           // selects all atoms
    void moleculeSelectNone();          // deselects all atoms
    void moleculeSaveCoordinates();     // saves the coordinates from disk
    void moleculeSelection();           // toggles manipulation of the selection

    ///// setup menu
    void setupGlobal();                 // sets up global options
    void setupBrabo();                  // sets up Brabo options
    void setupRelax();                  // sets up Relax options
    void setupFreq();                   // sets up Distor & Forkon options
    void setupBuur();                   // sets up Buur options

    ///// run menu
    void runStart();                    // starts the calculation
    void runPause();                    // pauses the calculation
    void runStop();                     // stops the calculation
    void runWrite();                    // writes all the input files needed for the calculation
    void runClean();                    // cleans the calculation directory.

    ///// results menu
    void resultsViewOutput();           // shows the output file(s)

    ///// tools menu
    void toolsPlotMap();                // shows a 2D map from a .map.1 file
    void toolsOrbitals();               // shows a 3D representation of orbitals
            
    ///// help menu
    void helpHelp();                    // shows some general help
    void helpWhatsThis();               // enters What's This mode
    void helpCredits();                 // gives some credits
    void helpAbout();                   // shows an about dialog

    ///// other
    virtual void switchToToplevelMode();          // undocks all view windows
    virtual void switchToChildframeMode();        // docks all view windows
    virtual void switchToTabPageMode();           // docks all view windows in tabpage mode
    void updateActions();               // adapts actions when focus between views changed
    void fixToplevelModeHeight();       // updates the fixed height when in Toplevel mode
    void fixToplevelModeHeight2();      // the actual worker function for the previously declared function

  private:
    ///// Private member functions 
    ///// initialization
    void loadGeometry();                // initialization needed for the first showing of the mainwindow
    void initActions();                 // creates the actions
    void initMenuBar();                 // creates the menubar
    void initToolBars();                // creates the toolbars
    //void initStatusBar();               // creates the statusbar

    ///// other
    void updatePreferences();           // updates the preferences
    void updateToolbarsInfo();          // updates editPreferences with the toolbar info
    void restoreToolbars();             // restores the toolbars
    QString actionText(const QString title, const QString brief, const QString details = QString::null, const IconSets::IconSetID iconID = IconSets::LastIcon); // constructs a text for the What's This mode for actions
    
    ///// Private widgets 
    ///// toolbars
    QToolBar* ToolBarStandard;          ///< The File & Edit toolbar.
    QToolBar* ToolBarCalculation;       ///< The Setup & Run toolbar.
    QToolBar* ToolBarCoordinates;       ///< The MoleculeView toolbar.
    ///// actions
    QAction* actionFileNew;             ///< Action that's connected to fileNew().
    QAction* actionFileOpen;            ///< Action that's connected to fileOpen().
    QAction* actionFileSave;            ///< Action that's connected to fileSave().
    QAction* actionFileSaveAs;          ///< Action that's connected to fileSaveAs().
    QAction* actionFileClose;           ///< Action that's connected to fileClose().
    QAction* actionFileQuit;            ///< Action that's connected to fileClose().
    QAction* actionEditCut;             ///< Action that's connected to editCut().
    QAction* actionEditCopy;            ///< Action that's connected to editCopy().
    QAction* actionEditPaste;           ///< Action that's connected to editPaste().
    QAction* actionEditPrefs;           ///< Action that's connected to editPrefs().
    QAction* actionViewToolBarStandard; ///< Action that's connected to viewToolBarStandard
    QAction* actionViewToolBarCalculation;        ///< Action that's connected to viewToolBarCalculation
    QAction* actionViewToolBarCoordinates;        ///< Action that's connected to viewToolBarCoordinates
    QAction* actionViewStatusBar;       ///< Action that's connected to viewStatusBar
    QAction* actionViewTaskBar;         ///< Action that's connected to viewTaskBar
    QAction* actionMoleculeReadCoordinates;       ///< Action that's connected to moleculeReadCoordinates().
    QAction* actionMoleculeCenterView;  ///< Action that's connected to moleculeCenterView().
    QAction* actionMoleculeResetOrientation;      ///< Action that's connected to moleculeResetOrientation().
    QAction* actionMoleculeZoomFit;     ///< Action that's connected to moleculeZoomFit().
    QAction* actionMoleculeResetView;   ///< Action that's connected to moleculeResetView().
    QAction* actionMoleculeAnimate;     ///< Action that's connected to moleculeAnimate().
    QAction* actionMoleculeFPS;         ///< Action that's connected to moleculeFPS().
    QAction* actionMoleculeImage;       ///< Action that's connected to moleculeImage().
    QAction* actionMoleculeAddAtom;     ///< Action that's connected to moleculeAddAtom().
    QAction* actionMoleculeDeleteSelection;       ///< Action that's connected to moleculeDeleteSelection().
    QAction* actionMoleculeDensity;     ///< Action that's connected to moleculeDensity().
    QAction* actionMoleculeDisplayMode; ///< Action that's connected to moleculeDisplayMode().
    QAction* actionMoleculeAlterCartesian;        ///< Action that's connected to moleculeAlterCartesian().
    QAction* actionMoleculeAlterInternal;         ///< Action that's connected to moleculeAlterInternal().
    QAction* actionMoleculeSelectAll;   ///< Action that's connected to moleculeSelectAll().
    QAction* actionMoleculeSelectNone;  ///< Action that's connected to moleculeSelectNone().
    QAction* actionMoleculeSaveCoordinates;       ///< Action that's connected to moleculeSaveCoordinates().
    QAction* actionMoleculeSelection;   ///< Action that's connected to moleculeSelection().
    QAction* actionSetupGlobal;         ///< Action that's connected to setupGlobal().
    QAction* actionSetupBrabo;          ///< Action that's connected to setupBrabo().
    QAction* actionSetupRelax;          ///< Action that's connected to setupRelax().
    QAction* actionSetupFreq;           ///< Action that's connected to setupFreq().
    QAction* actionSetupBuur;           ///< Action that's connected to setupBuur().
    QAction* actionRunStart;            ///< Action that's connected to runStart().
    QAction* actionRunPause;            ///< Action that's connected to runPause().
    QAction* actionRunStop;             ///< Action that's connected to runStop().
    QAction* actionRunWrite;            ///< Action that's connected to runWrite().
    QAction* actionRunClean;            ///< Action that's connected to runClean().
    QAction* actionResultsViewOutput;   ///< Action that's connected to resultsViewOutput().
    QAction* actionToolsPlotMap;        ///< Action that's connected to toolsPlotMap().
    QAction* actionToolsOrbitals;       ///< Action that's connected to toolsOrbitals().
    QAction* actionHelp;                ///< Action that's connected to help().
    QAction* actionWhatsThis;           ///< Action that's connected to whatsThis().
    QAction* actionCredits;             ///< Action that's connected to helpCredits().
    QAction* actionAbout;               ///< Action that's connected to helpAbout().
    ///// dialogs
    XbraboView* view;                   ///< Holds the active view.
    PreferencesBase* editPreferences;   ///< The Preferences dialog.
};

#endif

