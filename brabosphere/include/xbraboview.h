/***************************************************************************
                         xbraboview.h  -  description
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
/// Contains the declaration of the class XbraboView

#ifndef XBRABOVIEW_H
#define XBRABOVIEW_H

///// Forward class declarations & header files ///////////////////////////////

///// Qt forward class declarations
class QDomDocument;
class QDomElement;
class QLabel;
class QLayout;
//class QListBox;
class QListViewItem;
class QPoint;
class QProcess;
class QProgressBar;
class QSplitter;

///// Xbrabo forward class declarations
class AtomSet;
class BraboBase;
class Calculation;
class GLMoleculeView;
class GlobalBase;
class RelaxBase;
class StatusText;

///// Base class header file
#if defined(USE_KMDI) || defined(USE_KMDI_DLL)
#  include<kmdichildview.h>
#  define QextMdiChildView KMdiChildView
#else
  #include <qextmdichildview.h>
#endif

///// class XbraboView ////////////////////////////////////////////////////////
class XbraboView : public QextMdiChildView
{
  Q_OBJECT

  public:
    XbraboView(QWidget* mainWin, QWidget* parent = 0, QString title = QString::null, const char* name = 0, WFlags f = 0);         // constructor
    ~XbraboView();                      // destructor

    ///// public members
    unsigned int calculationType() const;         // returns the type of the calculation
    unsigned int buurType() const;      // returns the type of crystal environment
    bool isModified() const;            // returns true if the calculation has been modified
    //bool isAnimating() const;           // returns true if the molecule is animating
    QString name() const;               // returns the name of the calculation
    QString fileName() const;           // returns the name the calculation was opened under
    QString directory() const;          // returns the calculation directory
    bool isRunning() const;             // returns true if the calculation is running
    bool isPaused() const;              // returns true if the calculation is paused
    //bool hasSelection() const;          // returns true if atoms are selected
    void cut();                         // implements a cut operation
    void copy();                        // implements a copy operation
    void paste();                       // implements a paste operation
    GLMoleculeView* moleculeView() const;         // returns a pointer to the GLMoleculeView widget

    bool loadCML(const QDomDocument* doc);        // loads all data from a QDomDocument
    QDomDocument* saveCML();            // saves all data to a QDomDocument
    void setFileName(const QString filename);     // sets the name the calculation should be saved under

  signals:
    void changed();                     // is emitted when the calculation has changed or when it recieves focus
    
  public slots:
    void setModified(bool state = true);// sets the 'modified' property of the calculation
    void moleculeReadCoordinates();     // reads coordinates
    void moleculeSaveCoordinates();     // saves coordinates
    void moleculeFPS();                 // calculates the FPS for the current parameters
    void setupGlobal();                 // sets up global options
    void setupBrabo();                  // sets up Brabo options
    void setupRelax();                  // sets up Relax options
    void setupFreq();                   // sets up Distor & Forkon options
    void setupBuur();                   // sets up Buur options
    void start();                       // starts the calculation
    void pause();                       // pauses the calculation
    void stop();                        // stops the calculation
    void writeInput();                  // writes the input files
    void cleanCalculation();            // cleans the calculation directory
    void viewOutput();                  // shows the output files
    void updatePVMHosts(const QStringList& hosts);// updates the PVM host list in braboSetup
    void showProperties();              // changes which properties are shown in the OpenGL window.

  protected:
    void keyPressEvent(QKeyEvent* e);   // event which takes place when a key is pressed
    void mouseReleaseEvent(QMouseEvent* e);       // event which takes place when a mouse button was release on the widget
       
  private slots:
    void popup();                       // shows the popupmenu 
    void updateIteration(unsigned int iteration, double energy);      // updates the status window for a Brabo iteration
    void updateCycle(unsigned int cycle);         // updates the status window for a Relax cycle
    void cleanupCalculation(unsigned int error);  // cleans up a stopped calculation
    void showOutput(QListViewItem* item, const QPoint&, int column);  // shows a certain output chosen in an OutputChooserWidget

  private:
    ///// Private member functions 
    void loadCMLLocal(const QDomElement* root);   // load geometry
    void saveCMLLocal(QDomElement* root);         // save geometry
    void updateCaptions();              // updates the captions of the window and taskbar
    void updateBraboSetup();            // updates braboSetup according to globalSetup
    void updateRelaxSetup();            // updates relaxSetup according to globalSetup
    bool makeDirCurrent(const QString dir, const QString title);      // makes a directory current
    void initGlobalSetup();             // constructs the GlobalBase widget if it hasn't been created yet
    void initBraboSetup();              // constructs the BraboBase widget if it hasn't been created yet
    void initRelaxSetup();              // constructs the RelaxBase widget if it hasn't been created yet
    bool initCalculation();             // constructs the Calculation and does an initial setup
    void updateAtomSet();               // does some updates when the atomset has changed

    ///// Private widgets 
    // Window
    QVBoxLayout* BigLayout;             ///< Main layout for the widget.
    QSplitter* Splitter;                ///< Splitter between the OpenGL window and the statuswindow.
    GLMoleculeView* MoleculeView;       ///< The OpenGL view.
    QVBoxLayout* StatusLayout;          ///< A sublayout for the status window.
    QLabel* TextLabelStatus;            ///< The label above the status window.
    StatusText* TextEditStatus;         ///< The status window itself.
    QHBoxLayout* ProgressLayout;        ///< A sublayout for the progressbar.
    QLabel* TextLabelProgress;          ///< A textlabel for the progressbar.
    QProgressBar* ProgressBar;          ///< The progressbar itself.
    // Dialogs
    GlobalBase* globalSetup;            ///< Dialog for setup of global options.
    BraboBase* braboSetup;              ///< Dialog for setup of Brabo options.
    RelaxBase* relaxSetup;              ///< Dialog for setup of Relax options.
    // Other pointers
    QWidget* mainWindow;                ///< A pointer to the QextMDIMainFrm as the parent can be zero (used for Toplevel mode)

    ///// Private data
    bool calcModified;                  ///< = TRUE if calculation has been modified and should be saved.
    QString calcDefaultName;            ///< The default name of the calculation constructed from calcCounter
    QString calcFileName;               ///< The filename of the calculation.
    QString calcDate;                   ///< The creation date of the calculation as saved in the CML file.
    AtomSet* atoms;                     ///< Contains the atoms and their properties.

    unsigned int calcBraboSteps;        ///< Contains the maximum number of brabo iterations.
    unsigned int lastProgress;          ///< The progress from the last brabo calculation.
    Calculation* calculation;           ///< The calculation class
    QStringList hostListPVM;            ///< A temporary copy of the PVM host list in case braboSetup is not initialized yet

    static unsigned int calcCounter;    ///< A counter that holds the number of new calculations for the current session.
};

#endif

