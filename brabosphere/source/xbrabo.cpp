/***************************************************************************
                          xbrabo.cpp  -  description
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

///// Comments ////////////////////////////////////////////////////////////////
/*!
  \class Xbrabo
  \brief This class is the main MDI widget which controls the XbraboViews.

  It is a subclass of QextMDIMainForm providing an MDI interface, a menu
  and various toolbars.

*/
/// \file
/// Contains the implementation of the class Xbrabo

///// Header files ////////////////////////////////////////////////////////////

// Qt header files
#include <qaccel.h>
#include <qaction.h>
#include <qapp.h>
#include <qbitmap.h>
#include <qdir.h>
#include <qdockarea.h>
#include <qdom.h>
#include <qfiledialog.h>
#include <qmainwindow.h>
#include <qmenubar.h>
#include <qmsgbox.h>
#include <qnamespace.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qpopupmenu.h>
#include <qsettings.h>
#include <qstatusbar.h>
#include <qstring.h>
#include <qtimer.h>
#include <qtoolbar.h>
#include <qtoolbutton.h>
#include <qwhatsthis.h>

// Xbrabo header files
#include "aboutbox.h"
#include "brabobase.h"
#include "iconsets.h"
#include "glmoleculeview.h"
#include "globalbase.h"
#include "orbitalviewerbase.h"
#include "plotmapbase.h"
#include "preferencesbase.h"
#include "statustext.h"
#include "textviewwidget.h"
#include "version.h"
#include "xbrabo.h"
#include "xbraboview.h"

#if defined(USE_KMDI) || defined(USE_KMDI_DLL)
#  define QextMdiMainFrm KMdiMainFrm
#  define QextMdiViewCloseEvent KMdiViewCloseEvent
#  define QextMdi KMdi
#  define m_pWinList m_pDocumentViews
#endif

///////////////////////////////////////////////////////////////////////////////
///// Public Members                                                      /////
///////////////////////////////////////////////////////////////////////////////

///// Constructor /////////////////////////////////////////////////////////////
Xbrabo::Xbrabo() : QextMdiMainFrm(0)
/// The default constructor.
{
  loadGeometry(); // basic settings for initial showing of the mainwindow
}

///// Destructor //////////////////////////////////////////////////////////////
Xbrabo::~Xbrabo()
/// The default destructor.
{

}

///// init ////////////////////////////////////////////////////////////////////
void Xbrabo::init()
/// Initializes everything. Normally this is done in the constructor, but
/// making it a public function it can be invoked after this class is constructed.
/// This way the splashscreen is more useful.
/// \warning The application will crash if this function is not called.
/// \warning This function may only be called once.
{  
  setDefaultChildFrmSize(QSize(400, 400));
  
  editPreferences = new PreferencesBase(this, 0, true, 0);  // this loads the rest of the settings

  ///// construct all parts
  initActions();
  initMenuBar();
  initToolBars();
  //initStatusBar();

  updateActions();

  ///// connections
  connect(this, SIGNAL(lastChildViewClosed()),this, SLOT(updateActions()));

  updatePreferences(); // update the static variables of XbraboView
  editPreferences->updateVisuals(); // update the visuals
  restoreToolbars(); // was being called in showevent
}

///// fileOpen ////////////////////////////////////////////////////////////////
void Xbrabo::fileOpen(const QString filename)
/// Opens an existing calculation with the specified name.
{  
  QFile file(filename);
  if(!file.open(IO_ReadOnly))
  {
    QMessageBox::warning(this, Version::appName, tr("The selected file could not be opened"), QMessageBox::Ok, QMessageBox::NoButton);
    statusBar()->message(tr("Opening aborted"), 2000);
    return;
  }
  QDomDocument* doc = new QDomDocument(filename);
  QString errorMessage;
  int errorLine, errorColumn;
  if(!doc->setContent(&file, false, &errorMessage, &errorLine, &errorColumn))
  {
    QString message =          tr("An error occured while parsing")
                      + "\n" + tr("Error Message: ") + errorMessage
                      + "\n" + tr("Line number: ") + QString::number(errorLine)
                      + "\n" + tr("Column number: ") + QString::number(errorColumn);
    QMessageBox::warning(this, Version::appName, message, QMessageBox::Ok, QMessageBox::NoButton);
    file.close();
    delete doc;
    statusBar()->message(tr("Opening aborted"), 2000);
    return;
  }
  file.close();

  /*
  ///// Check whether it's an Xbrabo enhanced CML file
  ///// If the root element is <molecule></molecule> or <cml></cml> without an XBrabo identifier
  ///// => reject it ATM (possibly make new calculation with the found coordinates preloaded)
  QDomElement root = doc->documentElement();
  if(root.tagName() == "molecule")
  {
    QMessageBox::warning(this, appName, tr("The selected file is a regular CML file"), QMessageBox::Ok, QMessageBox::NoButton);
    delete doc;
    statusBar()->message(tr("Opening aborted"), 2000);
    return;
  }
  else if(root.tagName() == "cml")
  {
    ///// Xbrabo CML documents always have a root Element <cml></cml> and an <brabosphere>X</brabosphere> version
    QDomNodeList list = doc->elementsByTagName("brabosphere");
    if(list.count() == 0)
    {
      QMessageBox::warning(this, appName, tr("The selected file is a regular CML file"), QMessageBox::Ok, QMessageBox::NoButton);
      delete doc;
      statusBar()->message(tr("Opening aborted"), 2000);
      return;
    }
    if(list.item(0).toElement().text() != "1.0")
    {
      QMessageBox::warning(this, appName, tr("This CML file has been written with a more recent version of ") + appName, QMessageBox::Ok, QMessageBox::NoButton);
      delete doc;
      statusBar()->message(tr("Opening aborted"), 2000);
     return;
    }
  }
  else
  {
    QMessageBox::warning(this, appName, tr("The selected file is a regular XML file"), QMessageBox::Ok, QMessageBox::NoButton);
    delete doc;
    statusBar()->message(tr("Opening aborted"), 2000);
    return;
  } 
  */

  ///// file is a valid Xbrabo CML file, so create an XbraboView to load it into
  if(mdiMode() == QextMdi::ToplevelMode)
    view = new XbraboView(this); // no parent
  else
    view = new XbraboView(this, this);
  if(!view->loadCML(doc))
  {
    // An error occured during the loading of the CML file. 
    // The user has already been informed of the problem.
    delete view;
    delete doc;
    statusBar()->message(tr("Opening aborted"), 2000);
    return;
  }

  delete doc;
  view->setFileName(filename);
  connect(editPreferences, SIGNAL(newPVMHosts(const QStringList&)), view, SLOT(updatePVMHosts(const QStringList&)));
  view->updatePVMHosts(editPreferences->getPVMHosts());
  connect(view, SIGNAL(changed()), this, SLOT(updateActions()));
  addWindow(view);
  updateActions();

  statusBar()->message(tr("Loaded document: ") + filename, 2000);
}

///////////////////////////////////////////////////////////////////////////////
///// Protected Members                                                   /////
///////////////////////////////////////////////////////////////////////////////

///// event ///////////////////////////////////////////////////////////////////
bool Xbrabo::event (QEvent* e)
/// Handles MDI child close events.
{
  ///// code taken from QextMdiMainFrm::event()
  if(e->type() == QEvent::User)
  {
    XbraboView* tempview = (XbraboView*)((QextMdiViewCloseEvent*)e)->data();
    if(tempview != 0)
    {
      fileClose();
      return true;
    }
  }
  
  return QextMdiMainFrm::event(e);
}

///// resizeEvent /////////////////////////////////////////////////////////////
void Xbrabo::resizeEvent (QResizeEvent* e)
/// Fits the system menu button position to the menu position.
{  
   QextMdiMainFrm::resizeEvent(e);
   setSysButtonsAtMenuPosition();
}

///// closeEvent //////////////////////////////////////////////////////////////
void Xbrabo::closeEvent (QCloseEvent* e)
/// Closes the application.
{
  statusBar()->message(tr("Exiting application..."));

  /*if(QMessageBox::information(this, tr("Quit"), tr("Do your really want to quit?"), QMessageBox::Yes, QMessageBox::No) != QMessageBox::Yes)
  {
    statusBar()->message(IDS_STATUS_DEFAULT);
    return;
  }*/

  ///// iterate over all open calculations and call fileClose() for each of them
  XbraboView* localview = dynamic_cast<XbraboView*>(activeWindow()); // view is used by fileClose()
  while(localview != 0)
  {
    fileClose();
    ///// check if the close was cancelled in any way
    qApp->processEvents(); // needed so activeWindow() points to next window
    XbraboView* tempview = dynamic_cast<XbraboView*>(activeWindow());
    if(localview == tempview) // activeWindow is still the same, so not closed
    {
      statusBar()->clear();
      return;
    }
    localview = tempview;
  }

  updateToolbarsInfo();
  e->accept();
}

///////////////////////////////////////////////////////////////////////////////
///// Private slots                                                       /////
///////////////////////////////////////////////////////////////////////////////

///// fileNew /////////////////////////////////////////////////////////////////
void Xbrabo::fileNew()
/// Generates a new calculation.
{  
  statusBar()->message(tr("Creating new calculation..."));
 
  if(mdiMode() == QextMdi::ToplevelMode)
    view = new XbraboView(this);
  else
    view = new XbraboView(this, this);
  connect(editPreferences, SIGNAL(newPVMHosts(const QStringList&)), view, SLOT(updatePVMHosts(const QStringList&)));
    view->updatePVMHosts(editPreferences->getPVMHosts());
  connect(view, SIGNAL(changed()), this, SLOT(updateActions()));
  if(mdiMode() == QextMdi::ToplevelMode)
  {
    // from detachWindow (doesn't work as the size is larger than (1,1) because of the added layout)
    // this should be called before addWindow(), otherwise it references itself
    // position it outside of the screen so it doesn't flicker briefly when moving after addWindow
    // (which does a show, and using QextMdi::Hide with addWindow crashes at the next show())
    if(m_pCurrentWindow)
      view->setInternalGeometry(QRect(QPoint(0,-m_pCurrentWindow->size().height()-100), m_pCurrentWindow->size()));
    else
      view->setInternalGeometry(QRect(QPoint(0,-defaultChildFrmSize().height()-100), defaultChildFrmSize()));
  }
  addWindow(view);
  if(mdiMode() == QextMdi::ToplevelMode)
  {
    // also from detachWindow 
    // this should be called after addWindow otherwise the first 2 windows overlap
    view->move(QPoint(x() + 10, y() + frameGeometry().height()) + m_pMdi->getCascadePoint(m_pWinList->count()-1));
  }
  updateActions(); 
  statusBar()->clear();
}

///// fileOpen ////////////////////////////////////////////////////////////////
void Xbrabo::fileOpen()
/// Opens an existing calculation.
{  
  statusBar()->message(tr("Opening file..."));

  QString fileName = QFileDialog::getOpenFileName(QString::null, "*.cml", this, 0, tr("Choose a") + " " + Version::appName + " " + tr("file to open"));
  if (fileName.isEmpty())
  {
    statusBar()->message(tr("Opening aborted"), 2000);
    return;
  }

  fileOpen(fileName);
}

///// fileSave ////////////////////////////////////////////////////////////////
void Xbrabo::fileSave()
/// Saves a calculation to disk.
{  
  view = dynamic_cast<XbraboView*>(activeWindow());
  if(view == 0)
    return;

  if(view->fileName().contains("cml", false) == 0)
  {
    fileSaveAs();
    return;
  }

  statusBar()->message(tr("Saving file..."));

  QFile file(view->fileName());
  if(!file.open(IO_WriteOnly))
  {
    QMessageBox::warning(this, Version::appName, tr("The file could not be overwritten"), QMessageBox::Ok, QMessageBox::NoButton);
    statusBar()->message(tr("Saving aborted"), 2000);   
    return;
  }

  QTextStream stream(&file);
  stream.setEncoding(QTextStream::UnicodeUTF8);
  QDomDocument* doc = view->saveCML();
  stream << doc->toString(2) << endl;
  delete doc;
  view->setModified(false); // emits changed() which calls updateActions

  statusBar()->clear();
}

///// fileSaveAs //////////////////////////////////////////////////////////////
void Xbrabo::fileSaveAs()
/// Saves a calculation to disk under a new name.
{  
  view = dynamic_cast<XbraboView*>(activeWindow());
  if(view == 0)
    return;  

  statusBar()->message(tr("Saving file under new filename..."));

  QString startName;
  if(view->fileName().contains("cml", false) == 0)
    startName = QDir::homeDirPath();
  else
    startName = view->fileName();
  QString fileName = QFileDialog::getSaveFileName(startName,"*.cml",this,0,"Choose a filename to save under");
  if (fileName.isEmpty())
  {
    statusBar()->message(tr("Saving aborted"), 2000);
    return;
  }
  if(fileName.right(4) != ".cml")
    fileName += ".cml";
  QFile file(fileName);
  if(file.exists())
  {
    if(QMessageBox::warning(this, Version::appName, tr("The selected file already exists. Do you want to overwrite it?"), QMessageBox::Yes, QMessageBox::No) == QMessageBox::No)
    {
      statusBar()->message(tr("Saving aborted"), 2000);
      return;
    }
  }
  if(!file.open(IO_WriteOnly))
  {
    QMessageBox::warning(this, Version::appName, tr("The selected file could not be created"), QMessageBox::Ok, QMessageBox::NoButton);
    statusBar()->message(tr("Saving aborted"), 2000);
    return;
  }   

  QDomDocument* doc = view->saveCML();  
  QString cml = doc->toString();
  delete doc;
    
  QTextStream stream(&file);
  stream.setEncoding(QTextStream::UnicodeUTF8);
  stream << cml << endl;

  view->setFileName(fileName);
  view->setModified(false);
  statusBar()->clear();
}

///// fileClose ///////////////////////////////////////////////////////////////
void Xbrabo::fileClose()
/// Closes a calculation.
{  
  view = dynamic_cast<XbraboView*>(activeWindow());
  if(view == 0)
    return;

  statusBar()->message(tr("Closing file..."));

  if(view->isRunning() && !view->isPaused())
  {
    if(QMessageBox::information(this, Version::appName, tr("The current calculation is running."), tr("Close anyway"), tr("Cancel"), 0, 1) == 1)
    {
      statusBar()->message(tr("Closing aborted"), 2000);
      return;
    }
  }
  if(view->isModified())
  {
    int result = QMessageBox::information(this, Version::appName, tr("Calculation ") + view->name() + tr(" has been modified since the last save."), tr("Save"), tr("Discard changes"), tr("Cancel"), 0, 2);
    if(result == 2)
      return;    
    else if(result == 0)
    {        
      fileSave();
      if(view->isModified())
      {
        statusBar()->message(tr("Closing aborted"), 2000);
        return; // cancellation of save occured
      }
    }
  }
  ///// this line is reached when - the calculation has been saved succesfully
  /////                           - the calculation didn't need to be saved
  /////                           - the changes were discarded  
  closeWindow(view); // closeActiveView loops because of the overridden event()
  statusBar()->clear();
}

///// editCut /////////////////////////////////////////////////////////////////
void Xbrabo::editCut()
/// Invokes the cut action of the current view.
{  
  statusBar()->message(tr("Cutting selection..."), 1000);

  view = dynamic_cast<XbraboView*>(activeWindow());
  if(view != 0)
    view->cut();  
}

///// editCopy ////////////////////////////////////////////////////////////////
void Xbrabo::editCopy()
/// Invokes the copy action of the current view.
{  
  statusBar()->message(tr("Copying selection to clipboard..."), 1000);

  view = dynamic_cast<XbraboView*>(activeWindow());
  if(view != 0)
    view->copy();  
}

///// editPaste ///////////////////////////////////////////////////////////////
void Xbrabo::editPaste()
/// Invokes the paste action of the current view.
{  
  statusBar()->message(tr("Inserting clipboard contents..."), 1000);

  view = dynamic_cast<XbraboView*>(activeWindow());
  if(view != 0)
    view->paste();  
}

///// editPrefs ///////////////////////////////////////////////////////////////
void Xbrabo::editPrefs()
/// Changes the program's preferences.
{  
  statusBar()->message(tr("Changing program preferences..."));

  if(editPreferences->exec() == QDialog::Accepted)
  {
    updatePreferences();
    editPreferences->updateVisuals();
  }

  statusBar()->clear();
}

///// viewToolBarStandard /////////////////////////////////////////////////////
void Xbrabo::viewToolBarStandard()
/// Toggles the visibility of the Standard toolbar.
{  
  statusBar()->message(tr("Toggling Standard toolbar..."), 1000);

  // turn toolbar on or off
  if (ToolBarStandard->isVisible())
    ToolBarStandard->hide();
  else
    ToolBarStandard->show();
}

///// viewToolBarCalculation //////////////////////////////////////////////////
void Xbrabo::viewToolBarCalculation()
/// Toggles the visibility of the Calculation toolbar.
{
  statusBar()->message(tr("Toggling Calculation toolbar..."), 1000);

  // turn toolbar on or off
  if (ToolBarCalculation->isVisible())
    ToolBarCalculation->hide();
  else
    ToolBarCalculation->show();
}

///// viewToolBarCoordinates //////////////////////////////////////////////////
void Xbrabo::viewToolBarCoordinates()
/// Toggles the visibility of the Coordinates toolbar.
{
  statusBar()->message(tr("Toggling Coordinates toolbar..."), 1000);

  // turn toolbar on or off
  if (ToolBarCoordinates->isVisible())
    ToolBarCoordinates->hide();
  else
    ToolBarCoordinates->show();
}

///// viewStatusBar ///////////////////////////////////////////////////////////
void Xbrabo::viewStatusBar()
/// Toggles the visibility of the statusbar.
{  
  statusBar()->message(tr("Toggling statusbar..."), 1000);

  //turn statusbar on or off
  if (statusBar()->isVisible())
    statusBar()->hide();
  else
    statusBar()->show();
  fixToplevelModeHeight();
}

///// viewTaskBar /////////////////////////////////////////////////////////////
void Xbrabo::viewTaskBar()
/// Toggles the visibility of the taskbar.
{  
  statusBar()->message(tr("Toggling taskbar..."), 1000);

  //turn taskbar on or off
  if (m_pTaskBar->isVisible())
    hideViewTaskBar();
  else
    showViewTaskBar();
  actionViewTaskBar->setOn(m_pTaskBar->isVisible());
}

///// moleculeReadCoordinates /////////////////////////////////////////////////
void Xbrabo::moleculeReadCoordinates()
/// Reads coordinates for the active calculation.
{
  statusBar()->message(tr("Reading Coordinates..."));

  view = dynamic_cast<XbraboView*>(activeWindow());
  if(view != 0)
    view->moleculeReadCoordinates();  

  statusBar()->clear();
}

///// moleculeCenterView //////////////////////////////////////////////////////
void Xbrabo::moleculeCenterView()
/// Centers the view for the active calculation.
{
  statusBar()->message(tr("Centering view..."), 1000);

  view = dynamic_cast<XbraboView*>(activeWindow());
  if(view != 0)
    view->moleculeView()->centerView();
}

///// moleculeResetOrientation ////////////////////////////////////////////////
void Xbrabo::moleculeResetOrientation()
/// Resets the orientation for the active calculation.
{
  statusBar()->message(tr("Resetting orientation..."), 1000);

  view = dynamic_cast<XbraboView*>(activeWindow());
  if(view != 0)
    view->moleculeView()->resetOrientation();
}

///// moleculeZoomFit /////////////////////////////////////////////////////////
void Xbrabo::moleculeZoomFit()
/// Resets the zoom for the active calculation.
{
  statusBar()->message(tr("Resetting zoom..."), 1000);

  view = dynamic_cast<XbraboView*>(activeWindow());
  if(view != 0)
    view->moleculeView()->zoomFit();
}

///// moleculeResetView ///////////////////////////////////////////////////////
void Xbrabo::moleculeResetView()
/// Resets the complete view for the active calculation.
{
  statusBar()->message(tr("Resetting view..."), 1000);

  view = dynamic_cast<XbraboView*>(activeWindow());
  if(view != 0)
    view->moleculeView()->resetView();
}

///// moleculeAnimate /////////////////////////////////////////////////////////
void Xbrabo::moleculeAnimate()
/// Toggles animation of the molecule.
{
  statusBar()->message(tr("Toggling animation..."), 1000);

  view = dynamic_cast<XbraboView*>(activeWindow());
  if(view != 0)
  {
    view->moleculeView()->toggleAnimation();
    updateActions(); // toggling animation doesn't fire a XbraboView::changed signal
  }
}

///// moleculeFPS /////////////////////////////////////////////////////////////
void Xbrabo::moleculeFPS()
/// Calculates the maximum framerate for the current parameters.
{
  statusBar()->message(tr("Calculating FPS..."));

  view = dynamic_cast<XbraboView*>(activeWindow());
  if(view != 0)
    view->moleculeFPS();

  statusBar()->clear();
}

///// moleculeImage ///////////////////////////////////////////////////////////
void Xbrabo::moleculeImage()
/// Saves the current view as an image.
{
  statusBar()->message(tr("Saving to an image..."));

  view = dynamic_cast<XbraboView*>(activeWindow());
  if(view != 0)
    view->moleculeView()->saveImage();

  statusBar()->clear();
}

///// moleculeAddAtoms ////////////////////////////////////////////////////////
void Xbrabo::moleculeAddAtoms()
/// Shows a dialog allowing the addition of atoms
{
  statusBar()->message(tr("Adding atoms..."), 1000);

  view = dynamic_cast<XbraboView*>(activeWindow());
  if(view != 0)
    view->moleculeView()->addAtoms();
}

///// moleculeDeleteSelection /////////////////////////////////////////////////
void Xbrabo::moleculeDeleteSelection()
/// Deletes all selected atoms
{
  statusBar()->message(tr("Deleting selection..."), 1000);

  view = dynamic_cast<XbraboView*>(activeWindow());
  if(view != 0)
    view->moleculeView()->deleteSelectedAtoms();
}

///// moleculeDensity /////////////////////////////////////////////////////////
void Xbrabo::moleculeDensity()
/// Changes the density isosurfaces.
{
  statusBar()->message(tr("Changing density isosurfaces..."), 1000);

  view = dynamic_cast<XbraboView*>(activeWindow());
  if(view != 0)
    view->moleculeView()->showDensity();
}

///// moleculeDisplayMode /////////////////////////////////////////////////////
void Xbrabo::moleculeDisplayMode()
/// Changes the display mode of the molecule
{
  statusBar()->message(tr("Changing molecular display mode..."), 1000);

  view = dynamic_cast<XbraboView*>(activeWindow());
  if(view != 0)
    view->showProperties();
}

///// moleculeAlterCartesian //////////////////////////////////////////////////
void Xbrabo::moleculeAlterCartesian()
/// Alters the cartesian coordinates of the selected atoms.
{
  statusBar()->message(tr("Altering cartesian coordinates..."), 1000);

  view = dynamic_cast<XbraboView*>(activeWindow());
  if(view != 0)
    view->moleculeView()->alterCartesian();
}

///// moleculeAlterInternal ///////////////////////////////////////////////////
void Xbrabo::moleculeAlterInternal()
/// Alters the selected internal coordinate.
{
  statusBar()->message(tr("Altering selected internal coordinate..."), 1000);

  view = dynamic_cast<XbraboView*>(activeWindow());
  if(view != 0)
    view->moleculeView()->alterInternal();
}

///// moleculeSelectAll ///////////////////////////////////////////////////////
void Xbrabo::moleculeSelectAll()
/// Selects all atoms.
{
  statusBar()->message(tr("Selecting all atoms..."), 1000);

  view = dynamic_cast<XbraboView*>(activeWindow());
  if(view != 0)
    view->moleculeView()->selectAll();
}

///// moleculeSelectNone //////////////////////////////////////////////////////
void Xbrabo::moleculeSelectNone()
/// Deselects all atoms.
{
  statusBar()->message(tr("Deselecting all atoms..."), 1000);

  view = dynamic_cast<XbraboView*>(activeWindow());
  if(view != 0)
    view->moleculeView()->unselectAll();
}

///// moleculeSaveCoordinates /////////////////////////////////////////////////
void Xbrabo::moleculeSaveCoordinates()
/// Saves the coordinates.
{
  statusBar()->message(tr("Saving coordinates..."), 1000);

  view = dynamic_cast<XbraboView*>(activeWindow());
  if(view != 0)
    view->moleculeSaveCoordinates();
}

///// moleculeSelection ///////////////////////////////////////////////////////
void Xbrabo::moleculeSelection()
/// Toggles between manipulating the selected atoms and the entire system.
{
  statusBar()->message(tr("Toggling manipulation target..."), 1000);

  view = dynamic_cast<XbraboView*>(activeWindow());
  if(view != 0)
    view->moleculeView()->toggleSelection();
}

///// setupGlobal /////////////////////////////////////////////////////////////
void Xbrabo::setupGlobal()
/// Sets up the global options for the active calculation.
{  
  statusBar()->message(tr("Setup Global..."));

  view = dynamic_cast<XbraboView*>(activeWindow());
  if(view != 0)
    view->setupGlobal();

  statusBar()->clear();
}

///// setupBrabo //////////////////////////////////////////////////////////////
void Xbrabo::setupBrabo()
/// Sets up the Brabo options for the active calculation.
{  
  statusBar()->message(tr("Setup Energy & Forces..."));

  view = dynamic_cast<XbraboView*>(activeWindow());
  if(view != 0)
    view->setupBrabo();

  statusBar()->clear();
}

///// setupRelax //////////////////////////////////////////////////////////////
void Xbrabo::setupRelax()
/// Sets up the Relax options for the active calculation.
{  
  statusBar()->message(tr("Setup Geometry Optimization..."));

  view = dynamic_cast<XbraboView*>(activeWindow());
  if(view != 0)
    view->setupRelax();

  statusBar()->clear();
}

///// setupFreq ///////////////////////////////////////////////////////////////
void Xbrabo::setupFreq()
/// Sets up Distor & Forkon options for the active calculation.
{  
  statusBar()->message(tr("Setup Frequencies..."));

  view = dynamic_cast<XbraboView*>(activeWindow());
  if(view != 0)
    view->setupFreq();

  statusBar()->clear();
}

///// setupBuur ///////////////////////////////////////////////////////////////
void Xbrabo::setupBuur()
/// Sets up the Buur options for the active calculation.
{  
  statusBar()->message(tr("Setup Crystal..."));

  view = dynamic_cast<XbraboView*>(activeWindow());
  if(view != 0)
    view->setupBuur();

  statusBar()->clear();
}

///// runStart ////////////////////////////////////////////////////////////////
void Xbrabo::runStart()
/// Starts the active calculation.
{  
  statusBar()->message(tr("Starting Calculation..."), 1000);

  view = dynamic_cast<XbraboView*>(activeWindow());
  if(view != 0)
    view->start();
}

///// runPause ////////////////////////////////////////////////////////////////
void Xbrabo::runPause()
/// Pauses the active calculation.
{  
  statusBar()->message(tr("Pausing Calculation..."), 1000);

  view = dynamic_cast<XbraboView*>(activeWindow());
  if(view != 0)
    view->pause();
}

///// runStop /////////////////////////////////////////////////////////////////
void Xbrabo::runStop()
/// Stops the active calculation.
{  
  statusBar()->message(tr("Stopping Calculation..."), 1000);

  view = dynamic_cast<XbraboView*>(activeWindow());
  if(view != 0)
    view->stop();
}

///// runWrite ////////////////////////////////////////////////////////////////
void Xbrabo::runWrite()
/// Writes the input files for the active calculation.
{  
  statusBar()->message(tr("Writing Input Files..."), 1000);

  view = dynamic_cast<XbraboView*>(activeWindow());
  if(view != 0)
    view->writeInput();
}

///// runClean ////////////////////////////////////////////////////////////////
void Xbrabo::runClean()
/// Cleans the calculation directory for the active calculation.
{  
  statusBar()->message(tr("Cleaning the calculation directory..."), 1000);

  view = dynamic_cast<XbraboView*>(activeWindow());
  if(view != 0)
    view->cleanCalculation();
}

///// resultsViewOutput ///////////////////////////////////////////////////////
void Xbrabo::resultsViewOutput()
/// Shows the output from the active calculation.
{  
  statusBar()->message(tr("Viewing Output Files..."));

  view = dynamic_cast<XbraboView*>(activeWindow());
  if(view != 0)
    view->viewOutput();

  statusBar()->clear();
}

///// toolsPlotMap ////////////////////////////////////////////////////////////
void Xbrabo::toolsPlotMap()
/// Generates a 2D map from a .map.1 file (output from potdicht).
{
  statusBar()->message(tr("Generating a 2D density map..."));

  PlotMapBase* plotmap = new PlotMapBase(this, 0, false, Qt::WDestructiveClose);
  
  /*
  view = dynamic_cast<XbraboView*>(activeWindow());
  if(view != 0)
  {
    ///// active calculation
    if(plotmap->loadMapFile(view->directory() + QDir::separator() + view->name() + ".map.1"), true)
      plotmap->show();
    else if(plotmap->loadMapFile())
      plotmap->show();
    else
      plotmap->close();
  }
  else*/
  {
    ///// no active calculation
    if(plotmap->loadMapFile())
      plotmap->show();
    else
      plotmap->close();
  }

  statusBar()->clear();
}

///// toolsOrbitals ///////////////////////////////////////////////////////////
void Xbrabo::toolsOrbitals()
/// Shows a 3D representation of orbitals.
{
  statusBar()->message(tr("Showing orbitals..."), 1000);

  OrbitalViewerBase* orbitalView = new OrbitalViewerBase(this, 0, false, Qt::WDestructiveClose);
  orbitalView->show();
}

///// helpHelp ////////////////////////////////////////////////////////////////
void Xbrabo::helpHelp()
/// 
{  
  statusBar()->message(tr("Showing general help..."));

  TextViewWidget* context = new TextViewWidget(this, 0, true, 0);
  context->TextEdit->setText(tr(
    "<p>As usual, the help pages are worked on at the last moment, so you will not find a "
    "complete help system here. The aim was to make the functionality of the programs as self-evident as possible. "
    "There are, however, a few ways to get some limited help.</p>"
    "<ul><li>All elements of both <em><b>Brabosphere</b></em> and <em><b>CrdView</b></em> have context sensitive help "
    "associated with them. These can be invoked by making the programs enter the appropriate state. "
    "This can be done with the corresponding icon: <img source=\"WhatsThisNormal\" size=16>, by using "
    "Shift+F1 or clicking the question mark in the title bar of most dialogs. At that point the mouse "
    "cursor changes into a question mark. When clicking an element context sensitive help for that "
    "element will be shown (if any help is associated with it, of course). This normally only works "
    "for elements that can be changed (so not regular text).</li>"
    "<li>When hovering over the entries in the programs' menu, 1 line of help will appear in the "
    "statusbar. The visibility of this statusbar can be changed using the menu (View->Status Bar)</li></ul>"
    "<p>Now for some general information not explainable with context sensitive help:</p>"
    "<ul><li>The central entity of <em><b>Brabosphere</b></em> is a calculation, which can be created, opened, saved, "
    "and closed. The visual representation of a calculation shows a 3D window on the top and status "
    "messages at the bottom. The 3D window shows the molecule and possibly some of its properties.</li>"
    "<li>As soon as coordinates are present, a calculation can be started with the default settings. "
    "The type of calculation and all details can be changed in the appropriate dialogs. Most things "
    "can even be changed while the calculation is running (for example the SCF method, the basisset, "
    "whether or not to calculate stockholder charges at the next step, etc.).</li>"
    "<li>Coordinates can not only be read in, but also manipulated using cartesian or internal coordinates. "
    "Additionally, a limited molecular builder is available.</li><ul>"));

  context->TextEdit->setWordWrap(QTextEdit::WidgetWidth);
  QWhatsThis::add(context->TextEdit, "Yes, you found out how to use the context sensitive help!");
  context->TextEdit->setFont(QApplication::font());
  context->setCaption(tr("Contents"));
  context->exec();
  delete context;

  statusBar()->clear();
}

///// helpWhatsThis ///////////////////////////////////////////////////////////
void Xbrabo::helpWhatsThis()
/// 
{  
  statusBar()->message(tr("Entering What's This mode..."), 1000);
 
  QWhatsThis::enterWhatsThisMode();
}

///// helpCredits /////////////////////////////////////////////////////////////
void Xbrabo::helpCredits()
/// 
{  
  statusBar()->message(tr("Showing credits..."));

  TextViewWidget* credits = new TextViewWidget(this, 0, true, 0);
  credits->TextEdit->setText(tr(
    "<p>Although the code of <em><b>Brabosphere</b></em> and <em><b>CrdView</b></em> is only written by one person at the moment, "
    "these programs would not exist without the help of a number of very useful libraries."
    "This is the ideal place to give some credit to those.</p>"
    "<ul><li> Trolltech's <b>Qt</b> (<font color=blue>http://www.trolltech.com</font>): provides all the visuals you can see. "
    "It is an excellent multiplatform toolkit with excellent documentation. It is also the sole "
    "reason I learnt to program in <nobr>C++.</nobr></li>"
    "<li> Falk Brettschneider's <b>QextMDI</b> (<font color=blue>http://www.geocities.com/gigafalk/qextmdi.htm</font>): provides "
    "the choice between the Childframe, Toplevel, and Tabpage modes of operation. This code has been "
    "included into KDE, so the Qt-only version is not maintained anymore.</li>"
    "<li><b>OpenBabel</b> (<font color=blue>http://openbabel.sourceforge.net</font>): without this library, you could only read "
    "and write a limited number of coordinate files (only BRABO .crd, Xmol .xyz, and Gaussian .fchk).</li>"
    "<li>The <b>Marching Cubes</b> algorithm coded by Raghavendra Chandrashekara "
    "(<font color=blue>http://astronomy.swin.edu.au/~pbourke/modelling/polygonise/</font>): this made the visualisation "
    "of density isosurfaces possible.</li>"
    "<li>The <b>quaternion</b> class of Sobiet Void (<font color=blue>http://www.gamedev.net/reference/articles/article1095.asp</font>): "
    "quaternion's are practically essential when you want to be able to control a molecule in 3D.</li>"
    "<li>Nullsoft's <b>NSIS</b> (<font color=blue>http://nsis.sourceforge.net</font>), responsible for the "
    "easy installation on Windows.</li>"
    "</ul>"));

  credits->TextEdit->setWordWrap(QTextEdit::WidgetWidth);
  credits->TextEdit->setFont(QApplication::font());
  credits->setCaption(tr("Credits"));
  credits->exec();
  delete credits;

  statusBar()->clear();
}

///// helpAbout ///////////////////////////////////////////////////////////////
void Xbrabo::helpAbout()
/// Shows an About box for Xbrabo.
{  
  statusBar()->message(tr("Showing info about") + " " + Version::appName + " " + Version::appVersion + " (build " + Version::appBuild + ")...");

  AboutBox about(this); 
  about.exec();

  statusBar()->clear();
}

///// switchToTopLevelMode ////////////////////////////////////////////////////
void Xbrabo::switchToToplevelMode()
/// Switches the application to Toplevel mode.
{  
  statusBar()->message(tr("Switching to Toplevel mode..."), 1000);
  QextMdiMainFrm::switchToToplevelMode();
  fixToplevelModeHeight();
}

///// switchToChildFrameMode //////////////////////////////////////////////////
void Xbrabo::switchToChildframeMode()
/// Switches the application to Childframe mode.
{  
  statusBar()->message(tr("Switching to Childframe mode..."), 1000);
  //qDebug("before min and max heights: %d, %d", minimumHeight(), maximumHeight());
  //qDebug("before stored min and max heights: %d, %d", m_oldMainFrmMinHeight, m_oldMainFrmMaxHeight);
  //qDebug("before stored height = %d", m_oldMainFrmHeight);
  int keepHeight = m_oldMainFrmHeight;
  QextMdiMainFrm::switchToChildframeMode();
  qApp->processEvents();
  setMinimumHeight(0);
  setMaximumHeight(m_oldMainFrmMaxHeight > maximumHeight() ? m_oldMainFrmMaxHeight : maximumHeight());
  if(keepHeight > 0)
    resize(width(), keepHeight);
  //qDebug("after min and max heights: %d, %d", minimumHeight(), maximumHeight());
  //qDebug("after stored min and max heights: %d, %d", m_oldMainFrmMinHeight, m_oldMainFrmMaxHeight); 
  //qDebug("after stored height = %d", m_oldMainFrmHeight);
  // enable all dockareas
  setDockEnabled(Qt::DockLeft, true);
  setDockEnabled(Qt::DockRight, true);
}

///// switchToTabPageMode /////////////////////////////////////////////////////
void Xbrabo::switchToTabPageMode()
/// Switches the application to TabPage mode.
{  
  statusBar()->message(tr("Switching to TabPage mode..."), 1000);
  int keepHeight = m_oldMainFrmHeight;
  QextMdiMainFrm::switchToTabPageMode();
  qApp->processEvents();
  setMinimumHeight(0);
  setMaximumHeight(m_oldMainFrmMaxHeight > maximumHeight() ? m_oldMainFrmMaxHeight : maximumHeight());
  if(keepHeight > 0)
    resize(width(), keepHeight);
  // enable all dockareas
  setDockEnabled(Qt::DockLeft, true);
  setDockEnabled(Qt::DockRight, true);
}

///// updateActions ///////////////////////////////////////////////////////////
void Xbrabo::updateActions()
/// Adapts certain actions to the active calculation. It handles all cases,
/// including when there are no opened calculations. It's normally called due
/// to a changed() signal from an XbraboView, but not necessarily from the one
/// with the focus. Some unneccesary updating is bound to occur.
{
  ///// check whether there is an active calculation
  view = dynamic_cast<XbraboView*>(activeWindow());
  
  ///// file
  actionFileSave->setEnabled(view != 0);
  actionFileSaveAs->setEnabled(view != 0);
  actionFileClose->setEnabled(view != 0);
  ///// molecule
  actionMoleculeReadCoordinates->setEnabled(view != 0 && !view->isRunning());
  actionMoleculeSaveCoordinates->setEnabled(view != 0);
  actionMoleculeCenterView->setEnabled(view != 0);
  actionMoleculeResetOrientation->setEnabled(view != 0);
  actionMoleculeZoomFit->setEnabled(view != 0);
  actionMoleculeResetView->setEnabled(view != 0);
  actionMoleculeAnimate->setEnabled(view != 0);
  actionMoleculeAnimate->setOn(view != 0 && view->moleculeView()->isAnimating());
  //actionMoleculeFPS->setEnabled(view != 0);
  actionMoleculeImage->setEnabled(view != 0);
  actionMoleculeAddAtom->setEnabled(view != 0 && !view->isRunning());
  actionMoleculeDeleteSelection->setEnabled(view != 0 && view->moleculeView()->selectedAtoms() > 0);
  actionMoleculeAlterCartesian->setEnabled(view != 0 && !view->isRunning() && view->moleculeView()->selectedAtoms() > 0);
  actionMoleculeAlterInternal->setEnabled(view !=0 && !view->isRunning() && view->moleculeView()->selectedAtoms() > 1 && view->moleculeView()->selectedAtoms() < 5);
  actionMoleculeSelectAll->setEnabled(view != 0);
  actionMoleculeSelectNone->setEnabled(view != 0 && view->moleculeView()->selectedAtoms() > 0);
  actionMoleculeDensity->setEnabled(view != 0);
  actionMoleculeDisplayMode->setEnabled(view != 0);
  actionMoleculeSelection->setEnabled(view != 0 && view->moleculeView()->selectedAtoms() > 0);
  ///// setup
  actionSetupGlobal->setEnabled(view != 0);
  actionSetupBrabo->setEnabled(view != 0);
  actionSetupRelax->setEnabled(view != 0 && view->calculationType() == GlobalBase::GeometryOptimization);
  actionSetupFreq->setEnabled(view != 0 && view->calculationType() == GlobalBase::Frequencies);
  actionSetupBuur->setEnabled(view != 0 && view->buurType() != GlobalBase::NoBuur);
  ///// run
  actionRunStart->setEnabled(view != 0 && (!view->isRunning() || view->isPaused()));
  actionRunStart->setOn(view != 0 && view->isRunning() && !view->isPaused());
  actionRunPause->setEnabled(view != 0 && view->isRunning() && !view->isPaused());
  actionRunPause->setOn(view != 0 && view->isPaused());
  actionRunStop->setEnabled(view != 0 && view->isRunning());
  actionRunWrite->setEnabled(view != 0);
  actionRunClean->setEnabled(view != 0 && !view->isRunning());
  actionResultsViewOutput->setEnabled(view != 0);

  ///// set the caption to the active calculation
  if(view != 0)
    setCaption(Version::appName + " - " + view->caption());
  else
    setCaption(Version::appName);
}

///// fixToplevelModeHeight ///////////////////////////////////////////////////
void Xbrabo::fixToplevelModeHeight()
/// Updates the fixed height of the mainwindow when in Toplevel mode. It also
/// resizes the mainwindow to this size. This function only makes sure that
/// the actual work is done after all events are processed. Otherwise the calculation
/// of the new height of the mainwindow uses old values.
{
  if(mdiMode() != QextMdi::ToplevelMode)
    return;

  QTimer::singleShot(0,this,SLOT(fixToplevelModeHeight2()));
  return;
}

///// fixToplevelModeHeight2 //////////////////////////////////////////////////
void Xbrabo::fixToplevelModeHeight2()
/// The actual worker function
{
  // make sure all toolbars are in a horizontal orientation in the top (or bottom) dock.
  //*
  disconnect(ToolBarStandard, SIGNAL(placeChanged(QDockWindow::Place)), this, SLOT(fixToplevelModeHeight()));
  disconnect(ToolBarStandard, SIGNAL(visibilityChanged(bool)), this, SLOT(fixToplevelModeHeight()));
  disconnect(ToolBarCalculation, SIGNAL(placeChanged(QDockWindow::Place)), this, SLOT(fixToplevelModeHeight()));
  disconnect(ToolBarCalculation, SIGNAL(visibilityChanged(bool)), this, SLOT(fixToplevelModeHeight()));
  disconnect(ToolBarCoordinates, SIGNAL(placeChanged(QDockWindow::Place)), this, SLOT(fixToplevelModeHeight()));
  disconnect(ToolBarCoordinates, SIGNAL(visibilityChanged(bool)), this, SLOT(fixToplevelModeHeight()));
  disconnect(m_pTaskBar, SIGNAL(placeChanged(QDockWindow::Place)), this, SLOT(fixToplevelModeHeight()));
  disconnect(m_pTaskBar, SIGNAL(visibilityChanged(bool)), this, SLOT(fixToplevelModeHeight()));
  if(m_pTaskBar->orientation() == Qt::Vertical && m_pTaskBar->isVisible())
    moveDockWindow(m_pTaskBar);//, Qt::DockTop, true, 3);
  if(ToolBarStandard->orientation() == Qt::Vertical && ToolBarStandard->isVisible())
    moveDockWindow(ToolBarStandard);//, Qt::DockTop, true, 0);
  if(ToolBarCalculation->orientation() == Qt::Vertical && ToolBarCalculation->isVisible())
    moveDockWindow(ToolBarCalculation);//, Qt::DockTop, true, 1);
  if(ToolBarCoordinates->orientation() == Qt::Vertical && ToolBarCoordinates->isVisible())
    moveDockWindow(ToolBarCoordinates);//, Qt::DockTop, true, 2);
  connect(ToolBarStandard, SIGNAL(placeChanged(QDockWindow::Place)), this, SLOT(fixToplevelModeHeight()));
  connect(ToolBarStandard, SIGNAL(visibilityChanged(bool)), this, SLOT(fixToplevelModeHeight()));
  connect(ToolBarCalculation, SIGNAL(placeChanged(QDockWindow::Place)), this, SLOT(fixToplevelModeHeight()));
  connect(ToolBarCalculation, SIGNAL(visibilityChanged(bool)), this, SLOT(fixToplevelModeHeight()));
  connect(ToolBarCoordinates, SIGNAL(placeChanged(QDockWindow::Place)), this, SLOT(fixToplevelModeHeight()));
  connect(ToolBarCoordinates, SIGNAL(visibilityChanged(bool)), this, SLOT(fixToplevelModeHeight()));
  connect(m_pTaskBar, SIGNAL(placeChanged(QDockWindow::Place)), this, SLOT(fixToplevelModeHeight()));
  connect(m_pTaskBar, SIGNAL(visibilityChanged(bool)), this, SLOT(fixToplevelModeHeight()));
  // disable the vertical dockareas
  setDockEnabled(Qt::DockLeft, false);
  setDockEnabled(Qt::DockRight, false);
  //*/
  // calculate the height needed to fully show all dockwidgets
  // the menubar
  int newHeight = menuBar()->height();
  // the statusbar
  if(statusBar()->isVisible())
    newHeight += statusBar()->height();
  // the vertical QDockArea's (leftDock and rightDock) were emptied above
  if(!topDock()->isEmpty())
    newHeight += topDock()->height();
  if(!bottomDock()->isEmpty())
    newHeight += bottomDock()->height();

  // apply the new height and resize to the maximum allowed height
  setFixedHeight(newHeight);
  resize(width(), newHeight);
}

///////////////////////////////////////////////////////////////////////////////
///// Private Members                                                     /////
///////////////////////////////////////////////////////////////////////////////

///// loadGeometry ////////////////////////////////////////////////////////////
void Xbrabo::loadGeometry()
/// Loads as much as needed from the settings file (normally done in PreferencesBase)
/// in order to show the mainwindow during startup. The rest of the settings are
/// then loaded as a PreferencesBase class is instantiated in init()
{
  ///// should be kept exactly the same as in PreferencesBase
  QSettings settings;
  settings.setPath(Version::appCompany, Version::appName.lower(), QSettings::User);
#ifdef Q_OS_WIN32
  const QString unixPrefix = "/";
#else
  const QString unixPrefix = "/" + Version::appName.lower() + "/";
#endif
  const QString prefix = unixPrefix + "geometry/";

  ///// geometry
  int x      = settings.readNumEntry(prefix + "x", -1);
  int y      = settings.readNumEntry(prefix + "y", -1);
  int width  = settings.readNumEntry(prefix + "width", -1);
  int height = settings.readNumEntry(prefix + "height", -1);
  bool maxed = settings.readBoolEntry(prefix + "maximized", false);
  
  ///// mdi
#ifdef Q_OS_WIN32
  int mdiMode = settings.readNumEntry(unixPrefix + "mdi_mode", QextMdi::ChildframeMode);
#else
  int mdiMode = settings.readNumEntry(unixPrefix + "mdi_mode", QextMdi::ToplevelMode);
#endif
// */

  ///// set up the geometry ////////////
  ///// get some info about the available space and adapt the desired geometry
  QRect screenRect = QApplication::desktop()->availableGeometry(QApplication::desktop()->primaryScreen());
  ///// center the window if no data were found
  if((x == -1) || (y == -1) || (width == -1) || (height == -1))
  {
    width = screenRect.width()/2;
    height = screenRect.height()/2;
    x = width/2;
    y = height/2;
  }
  ///// check bounds
  if(x < 0) x = 0;
  if(y < 0) y = 0;
  if(width > screenRect.width())
  {
    x = 0;
    width = screenRect.width();
  }
  if(height > screenRect.height())
  {
    y = 0;
    height = screenRect.height();
  }
  if((x + width) > screenRect.width())
    x = (screenRect.width() - width)/2; // centered
  if((y + height) > screenRect.height())
    y = (screenRect.height() - height)/2; // centered   
  ///// set the new geometry
  QPoint position = QPoint(x, y);
  QSize size = QSize(width, height);
  resize(size);
  move(position);
  if(maxed)
    showMaximized();

  ///// set up the MDI mode ///////////
  switch(mdiMode)
  {
    case QextMdi::ToplevelMode:   QextMdiMainFrm::switchToToplevelMode();
                                  break;
    case QextMdi::TabPageMode:    QextMdiMainFrm::switchToTabPageMode();
                                  break;
    case QextMdi::ChildframeMode:
    default:                      QextMdiMainFrm::switchToChildframeMode();
  }
}

///// initActions /////////////////////////////////////////////////////////////
void Xbrabo::initActions()
/// Creates the actions and connects them to the relevant slots.
{
  ///// File->New
  actionFileNew = new QAction(IconSets::getIconSet(IconSets::New), tr("&New"), CTRL+Key_N, this);
  actionFileNew->setToolTip(tr("Create a new calculation")); // also sets StatusTip
  actionFileNew->setWhatsThis(actionText(tr("New File"), tr("Creates a new calculation."),
                              tr("All calculation settings are at their default values "
                                 "which means a Single Point Energy calculation using RHF/MIA will be performed when "
                                 "starting it. Before that is possible, coordinates will have to be read in."),
                              IconSets::New));
  connect(actionFileNew, SIGNAL(activated()), this, SLOT(fileNew()));

  ///// File->Open
  actionFileOpen = new QAction(IconSets::getIconSet(IconSets::Open), tr("&Open..."), CTRL+Key_O, this);
  actionFileOpen->setToolTip(tr("Open an existing calculation"));
  actionFileOpen->setWhatsThis(actionText(tr("Open File"), tr("Opens an existing calculation."),
                               tr("Any calculation saved using Brabosphere in CML-format can be opened again "
                                  "for further manipulation. General CML-files without the Brabosphere-specific "
                                  "enhancements will be rejected."),
                               IconSets::Open));
  connect(actionFileOpen, SIGNAL(activated()), this, SLOT(fileOpen()));

  ///// File->Save
  actionFileSave = new QAction(IconSets::getIconSet(IconSets::Save), tr("&Save"), CTRL+Key_S, this);
  actionFileSave->setToolTip(tr("Save the calculation"));
  actionFileSave->setWhatsThis(actionText(tr("Save File"), tr("Saves the active calculation."),
                               tr("The calculation that is currently active will be saved in the Brabosphere "
                                  "CML format. This Chemical Markup Language format can be read by all "
                                  "application that support the format (e.g. CrdView). Only the cartesian "
                                  "coordinates will then be read, however.<br>"
                                  "If the calculation has not been saved yet, a new filename will be asked for."),
                               IconSets::Save));
  connect(actionFileSave, SIGNAL(activated()), this, SLOT(fileSave()));

  ///// File->Save As
  actionFileSaveAs = new QAction(QIconSet(), tr("Save &as..."), 0, this);
  actionFileSaveAs->setToolTip(tr("Saves the calculation under a different name..."));
  actionFileSaveAs->setWhatsThis(actionText(tr("Save File As"), tr("Saves the active calculation under a new name."),
                                 tr("The calculation that is currently active will be saved with a filename that has to be chosen.")));
  connect(actionFileSaveAs, SIGNAL(activated()), this, SLOT(fileSaveAs()));

  ///// File->Close
  actionFileClose = new QAction(QIconSet(), tr("&Close"), CTRL+Key_W, this);
  actionFileClose->setToolTip(tr("Close the calculation"));
  actionFileClose->setWhatsThis(actionText(tr("Close File"), tr("Closes the active calculation."),
                                tr("The calculation that is currently active will be closed. If any changes were made "
                                   "since the last save, the opportunity will be presented to save the calculation.")));
  connect(actionFileClose, SIGNAL(activated()), this, SLOT(fileClose()));

  //// File->Quit
  actionFileQuit = new QAction(QIconSet(), tr("E&xit"), CTRL+Key_Q, this); 
  actionFileQuit->setToolTip(tr("Exit the application"));
  actionFileQuit->setWhatsThis(actionText(tr("Exit"), tr("Exits Brabosphere."),
                               tr("All opened calculations that have unsaved changes will be given the opportunity "
                                  "to be saved before exiting the application.")));
  connect(actionFileQuit, SIGNAL(activated()), this, SLOT(close()));

  ///// Edit->Cut
  actionEditCut = new QAction(IconSets::getIconSet(IconSets::Cut), tr("Cu&t"), CTRL+Key_X, this);
  actionEditCut->setToolTip(tr("Cut the selected text"));
  actionEditCut->setWhatsThis(actionText(tr("Cut"), tr("Cuts the selected text."),
                              tr("When invoking this command from the menu or the toolbar, only the "
                                 "selected text in the statuswindow of the active calculation will be "
                                 "copied to the clipboard. No text will be actually removed. The shortcut, "
                                 "however, can be used in all places were text can be given as input "
                                 "(e.g. Global Setup, Energy & Forces Setup, etc.).<br>"
                                 "This command cannot be used yet to cut selected atoms from the 3D view."),
                              IconSets::Cut));
  connect(actionEditCut, SIGNAL(activated()), this, SLOT(editCut()));

  ///// Edit->Copy
  actionEditCopy = new QAction(IconSets::getIconSet(IconSets::Copy), tr("&Copy"), CTRL+Key_C, this);
  actionEditCopy->setToolTip(tr("Copy the selected text to the clipboard"));
  actionEditCopy->setWhatsThis(actionText(tr("Copy"), tr("Copies the selected text."),
                               tr("When invoking this command from the menu or the toolbar, only the "
                                  "selected text in the statuswindow of the active calculation will be "
                                  "copied to the clipboard. It will have no effect on selected atoms in "
                                  "the 3D view (although this behaviour is subject to change). "
                                  "The shortcut can be used in all places were text can be given as input "
                                  "(e.g. Global Setup, Energy & Forces Setup, etc.).<br>"),
                               IconSets::Copy));
  connect(actionEditCopy, SIGNAL(activated()), this, SLOT(editCopy()));

  ///// Edit->Paste
  actionEditPaste = new QAction(IconSets::getIconSet(IconSets::Paste), tr("&Paste"), CTRL+Key_V, this);
  actionEditPaste->setToolTip(tr("Paste the contents of the clipboard"));
  actionEditPaste->setWhatsThis(actionText(tr("Paste"), tr("Pastes the contents of the clipboard."),
                                tr("When invoking this command from the menu or the toolbar, nothing will "
                                   "happen at the moment as it has no influence on the 3D view yet. "
                                   "The shortcut can, however, be used in all places were text can be given as input "
                                   "(e.g. Global Setup, Energy & Forces Setup, etc.).<br>"),
                                IconSets::Paste));
  connect(actionEditPaste, SIGNAL(activated()), this, SLOT(editPaste()));

  ///// Edit->Preferences
  actionEditPrefs = new QAction(IconSets::getIconSet(IconSets::Prefs), tr("P&references..."), 0, this);  
  actionEditPrefs->setToolTip(tr("Configure your preferences"));
  actionEditPrefs->setWhatsThis(actionText(tr("Edit Preferences"), tr("Allows editing of the program's preferences."),
                                tr("This command opens up a new window where the behaviour and visuals "
                                   "of Brabosphere can be adapted to your liking."),
                                IconSets::Prefs));
  connect(actionEditPrefs, SIGNAL(activated()), this, SLOT(editPrefs()));

  ///// View->ToolBars->Standard
  actionViewToolBarStandard = new QAction(QIconSet(), tr("&Standard"), 0, this);
  actionViewToolBarStandard->setToolTip(tr("Toggle the visibility of the Standard toolbar"));
  actionViewToolBarStandard->setToggleAction(true);
  actionViewToolBarStandard->setWhatsThis(actionText(tr("View Standard Toolbar"), tr("Toggles the visibility of the Standard toolbar."),
                                          tr("This is the toolbar containing shortcuts to selected entries in "
                                             "the File, Edit and Help menus.")));
  connect(actionViewToolBarStandard, SIGNAL(activated()), this, SLOT(viewToolBarStandard()));

  ///// View->ToolBars->Calculation
  actionViewToolBarCalculation = new QAction(QIconSet(), tr("&Calculation"), 0, this);
  actionViewToolBarCalculation->setToolTip(tr("Toggle the visibility of the Calculation toolbar"));
  actionViewToolBarCalculation->setToggleAction(true);
  actionViewToolBarCalculation->setWhatsThis(actionText(tr("View Calculation Toolbar"), tr("Toggles the visibility of the Calculation toolbar."),
                                             tr("This is the toolbar containing shortcuts to selected entries in "
                                                "the Setup and Run menus.")));
  connect(actionViewToolBarCalculation, SIGNAL(activated()), this, SLOT(viewToolBarCalculation()));

  ///// View->ToolBars->Molecule
  actionViewToolBarCoordinates = new QAction(QIconSet(), tr("&Molecule"), 0, this);
  actionViewToolBarCoordinates->setToolTip(tr("Toggle the visibility of the Molecule toolbar"));
  actionViewToolBarCoordinates->setToggleAction(true);
  actionViewToolBarCoordinates->setWhatsThis(actionText(tr("View Molecule Toolbar"), tr("Toggles the visibility of the Molecule toolbar."),
                                             tr("This is the toolbar containing shortcuts to selected entries in "
                                                "the Molecule menu.")));
  connect(actionViewToolBarCoordinates, SIGNAL(activated()), this, SLOT(viewToolBarCoordinates()));

  ///// View->Statusbar
  actionViewStatusBar = new QAction(QIconSet(), tr("&Statusbar"), 0, this);
  actionViewStatusBar->setToolTip(tr("Toggle the visibility of the statusbar"));
  actionViewStatusBar->setToggleAction(true);
  actionViewStatusBar->setWhatsThis(actionText(tr("View Statusbar"), tr("Toggles the visibility of the statusbar."),
                                    tr("This is de toolbar at the bottom of the main window which displays temporary messages.")));
  connect(actionViewStatusBar, SIGNAL(activated()), this, SLOT(viewStatusBar()));

  ///// View->Taskbar
  actionViewTaskBar = new QAction(QIconSet(), tr("T&askbar"), 0, this);
  actionViewTaskBar->setToolTip(tr("Toggle the visibility of the taskbar"));
  actionViewTaskBar->setToggleAction(true);
  actionViewTaskBar->setWhatsThis(actionText(tr("View Taskbar"), tr("Toggles the visibility of the taskbar."),
                                  tr("This is de toolbar providing a means for quickly switching between calculations. " 
                                     "It is only visible when at least 2 calculations are opened.")));
  connect(actionViewTaskBar, SIGNAL(activated()), this, SLOT(viewTaskBar()));

  ///// Molecule->Read Coordinates
  actionMoleculeReadCoordinates = new QAction(IconSets::getIconSet(IconSets::MoleculeRead), tr("Read &coordinates..."), 0, this);
  actionMoleculeReadCoordinates->setToolTip(tr("Read new coordinates"));
  actionMoleculeReadCoordinates->setWhatsThis(actionText(tr("Read Coordinates"), tr("Reads a new set of coordinates from a file."),
                                              tr("The existing coordinates will be removed together with selections, forces, charges "
                                                 "and other properties. The view of the 3D scene will be reset completely. "
                                                 "This command is only available when no calculation is running."),
                                              IconSets::MoleculeRead));
  connect(actionMoleculeReadCoordinates, SIGNAL(activated()), this, SLOT(moleculeReadCoordinates()));

  ///// Molecule->Reset->Translation
  actionMoleculeCenterView = new QAction(QIconSet(), tr("&Translation"), 0, this);
  actionMoleculeCenterView->setToolTip(tr("Center the molecule"));
  actionMoleculeCenterView->setWhatsThis(actionText(tr("Reset Translation"), tr("Restores the center of the molecule to the center of the 3D scene.")));
  connect(actionMoleculeCenterView, SIGNAL(activated()), this, SLOT(moleculeCenterView()));

  ///// Molecule->Reset->Orientation
  actionMoleculeResetOrientation = new QAction(QIconSet(), tr("&Orientation"), 0, this);
  actionMoleculeResetOrientation->setToolTip(tr("Reset the orientation of the molecule"));
  actionMoleculeResetOrientation->setWhatsThis(actionText(tr("Reset Orientation"), tr("Resets the rotational axes of the molecule."),
                                               tr("After the reset, the X-axis will again point to the right, "
                                                  "the Y-axis to the top and the Z-axis into the screen.")));
  connect(actionMoleculeResetOrientation, SIGNAL(activated()), this, SLOT(moleculeResetOrientation()));

  ///// Molecule->Reset->Zoom
  actionMoleculeZoomFit = new QAction(QIconSet(), tr("&Zoom"), 0, this);
  actionMoleculeZoomFit->setToolTip(tr("Zoom such that the molecule fits in the window"));
  actionMoleculeZoomFit->setWhatsThis(actionText(tr("Reset Zoom"), tr("Zooms the scene such that the molecule fits exactly in the window.")));
  connect(actionMoleculeZoomFit, SIGNAL(activated()), this, SLOT(moleculeZoomFit()));

  ///// Molecule->Reset->View
  actionMoleculeResetView = new QAction(QIconSet(), tr("&View"), 0, this);
  actionMoleculeResetView->setToolTip(tr("Reset translation, orientation and zoom"));
  actionMoleculeResetView->setWhatsThis(actionText(tr("Reset View"), tr("Resets the translation, orientation and zoom of the molecule.")));
  connect(actionMoleculeResetView, SIGNAL(activated()), this, SLOT(moleculeResetView()));

  ///// Molecule->Animate
  actionMoleculeAnimate = new QAction(IconSets::getIconSet(IconSets::MoleculeAnimate), tr("&Animate"), 0, this);  
  actionMoleculeAnimate->setToolTip(tr("Toggle the use of animation"));
  actionMoleculeAnimate->setToggleAction(true);
  actionMoleculeAnimate->setWhatsThis(actionText(tr("Animate Molecule"), tr("Toggles the use of animation."),
                                      tr("When the molecule is being animated through automated rotation around "
                                         "all axes, the direction and speed can be influenced by using the proper "
                                         "manipulations (mouse or keyboard) for the rotations around those axes."),
                                      IconSets::MoleculeAnimate));
  connect(actionMoleculeAnimate, SIGNAL(activated()), this, SLOT(moleculeAnimate()));

  // still allow FPS calculations, but only using a shortcut key
  actionMoleculeFPS = new QAction(QIconSet(), tr("Calculate &FPS"), CTRL+ALT+Key_F, this);  
  actionMoleculeFPS->setText(tr("Calculate the maximum attainable framerate"));
  connect(actionMoleculeFPS, SIGNAL(activated()), this, SLOT(moleculeFPS()));

  ///// Molecule->Save Image
  actionMoleculeImage = new QAction(IconSets::getIconSet(IconSets::Image), tr("&Save image..."), 0, this);
  actionMoleculeImage->setToolTip(tr("Save the current view as an image"));
  actionMoleculeImage->setWhatsThis(actionText(tr("Save Image"), tr("Saves the current view of the scene as an image."),
                                    tr("A number of fileformats can be chosen for the resulting file, "
                                       "but the size of the image will be determined by the size of the scene's window."),
                                    IconSets::Image));
  connect(actionMoleculeImage, SIGNAL(activated()), this, SLOT(moleculeImage()));

  ///// Molecule->Add Atom
  actionMoleculeAddAtom = new QAction(QIconSet(), tr("&Add atoms..."), 0, this);
  actionMoleculeAddAtom->setToolTip(tr("Add atoms to the molecule"));
  actionMoleculeAddAtom->setWhatsThis(actionText(tr("Add Atoms"), tr("Allows addition of atoms to the molecule."),
                                      tr("This command opens up a window allowing the addition of any number "
                                         "and type of atom by positioning it using either cartesian (absolute or relative) "
                                         "or internal coordinates. This command is not available when a calculation is running")));
  connect(actionMoleculeAddAtom, SIGNAL(activated()), this, SLOT(moleculeAddAtoms()));

  ///// Molecule->Delete Selected Atoms
  actionMoleculeDeleteSelection = new QAction(QIconSet(), tr("&Delete selected atoms..."), Qt::Key_Delete, this);
  actionMoleculeDeleteSelection->setToolTip(tr("Remove the selected atoms"));
  actionMoleculeDeleteSelection->setWhatsThis(actionText(tr("Delete Selected Atoms"), tr("Deletes the selected atoms."),
                                              tr("This command is not available when no atoms are selected or when "
                                                 "a calculation is running.")));
  connect(actionMoleculeDeleteSelection, SIGNAL(activated()), this, SLOT(moleculeDeleteSelection()));

  ///// Molecule->Density Isosurfaces
  actionMoleculeDensity = new QAction(QIconSet(), tr("Density &isosurfaces..."), 0, this);
  actionMoleculeDensity->setToolTip(tr("Change the density isosurfaces"));
  actionMoleculeDensity->setWhatsThis(actionText(tr("Density Isosurfaces"), tr("Allows changing the density isosurfaces."),
                                      tr("This command opens a window where up to 2 density (cube) files "
                                         "can be loaded and any number of isosurfaces can be defined. "
                                         "Loaded density files and generated isosurfaces are not saved with "
                                         "the calculation.")));
  connect(actionMoleculeDensity, SIGNAL(activated()), this, SLOT(moleculeDensity()));

  ///// Molecule->Display Mode
  actionMoleculeDisplayMode = new QAction(QIconSet(), tr("&Display mode..."), 0, this);
  actionMoleculeDisplayMode->setToolTip(tr("Change the display mode of the molecule"));
  actionMoleculeDisplayMode->setWhatsThis(actionText(tr("Display Mode"), tr("Changes the local display mode of the molecule."),
                                          tr("In the program's preferences, the default rendering type for "
                                             "atoms, bonds, forces and labels can be set for all calculations. "
                                             "This command opens up a window where these settings can be changed "
                                             "for the current molecule only.")));
  connect(actionMoleculeDisplayMode, SIGNAL(activated()), this, SLOT(moleculeDisplayMode()));

  ///// Molecule->Alter->Cartesian Coordinates
  actionMoleculeAlterCartesian = new QAction(QIconSet(), tr("&Cartesian coordinates..."), 0, this);
  actionMoleculeAlterCartesian->setToolTip(tr("Alter the cartesian coordinates of the selected atoms"));
  actionMoleculeAlterCartesian->setWhatsThis(actionText(tr("Alter Cartesian Coordinates"), tr("Alters the cartesian coordinates of the selected atoms."),
                                              tr("Although changing the cartesian coordinates of the selected atoms "
                                                 "can be done by the proper mouse or keyboard manipulations, they can "
                                                 "not be used for precise changes. This command shows a window where "
                                                 "an exact position can be given in case of a single atom selection, "
                                                 "or a change in relative position in case of a multiatom selection. "
                                                 "It is not available when a calculation is running or no atoms are selected.")));
  connect(actionMoleculeAlterCartesian, SIGNAL(activated()), this, SLOT(moleculeAlterCartesian()));

  ///// Molecule->Alter->Internal Coordinates
  actionMoleculeAlterInternal = new QAction(QIconSet(), tr("&Internal coordinate..."), 0, this);
  actionMoleculeAlterInternal->setToolTip(tr("Alter the internal coordinate specified by the selected atoms."));
  actionMoleculeAlterInternal->setWhatsThis(actionText(tr("Alter Internal Coordinate"), tr("Alters the internal coordinate of the selected atoms."),
                                            tr("Although changing the internal coordinate of the selected atoms "
                                               "can be done by the proper mouse or keyboard manipulations, they can "
                                               "not be used for precise changes. When exactly 2, 3 or 4 atoms are selected, "
                                               "this command allows giving an exact value for the resulting bond, valence "
                                               "angle or torsion angle. This command is not available when the correct number "
                                               "of atoms is not selected or when a calculation is running.")));
  connect(actionMoleculeAlterInternal, SIGNAL(activated()), this, SLOT(moleculeAlterInternal()));

  ///// Molecule->Select->All
  actionMoleculeSelectAll = new QAction(QIconSet(), tr("&All"), Qt::CTRL + Qt::Key_A, this);
  actionMoleculeSelectAll->setToolTip(tr("Select all atoms"));
  actionMoleculeSelectAll->setWhatsThis(actionText(tr("Select All"), tr("Selects all atoms.")));
  connect(actionMoleculeSelectAll, SIGNAL(activated()), this, SLOT(moleculeSelectAll()));

  ///// Molecule->Select->None
  actionMoleculeSelectNone = new QAction(QIconSet(), tr("&None"), Qt::CTRL + Qt::SHIFT + Qt::Key_A, this);
  actionMoleculeSelectNone->setToolTip(tr("Deselect all atoms"));
  actionMoleculeSelectNone->setWhatsThis(actionText(tr("Select None"), tr("Deselects all atoms.")));
  connect(actionMoleculeSelectNone, SIGNAL(activated()), this, SLOT(moleculeSelectNone()));

  ///// Molecule->Save Coordinates
  actionMoleculeSaveCoordinates = new QAction(IconSets::getIconSet(IconSets::MoleculeSave), tr("Save c&oordinates..."), 0, this);
  actionMoleculeSaveCoordinates->setToolTip(tr("Save the coordinates"));
  actionMoleculeSaveCoordinates->setWhatsThis(actionText(tr("Save Coordinates"), tr("Saves the molecule to a file."),
                                              tr("Shows a window allowing the coordinates to be saved in a variety of formats"),
                                              IconSets::MoleculeSave));
  connect(actionMoleculeSaveCoordinates, SIGNAL(activated()), this, SLOT(moleculeSaveCoordinates()));

  ///// Molecule->Manipulate Selection
  actionMoleculeSelection = new QAction(IconSets::getIconSet(IconSets::MoleculeSelection), tr("&Manipulate selection..."), 0, this);
  actionMoleculeSelection->setToolTip(tr("Toggle between manipulating the selected atoms and the entire system"));
  actionMoleculeSelection->setToggleAction(true); 
  actionMoleculeSelection->setWhatsThis(actionText(tr("Manipulate Selection"), tr("Toggles between manipulating the selected atoms and the entire system."),
                                        tr("By using the proper manipulation with mouse or keyboard, the 3D translation, "
                                           "rotation can be changed. When a selection is present, the same changes can be "
                                           "applied to the selected atoms alone using the additional <em>Aly</em> modifier key. "
                                           "On some systems (e.g. KDE) this key is already bound to other functionality and "
                                           "cannot be used. This command acts like a sticky <em>Alt</em> key: As long as it is "
                                           "activated, all manipulations will act on the selected atoms, regardless of the use "
                                           "of the <em>Alt</em> modifier."),
                                        IconSets::MoleculeSelection));
  connect(actionMoleculeSelection, SIGNAL(activated()), this, SLOT(moleculeSelection()));

  ///// Setup->Global
  actionSetupGlobal = new QAction(IconSets::getIconSet(IconSets::SetupGlobal), tr("&Global..."), Key_F2, this);
  actionSetupGlobal->setToolTip(tr("Setup global options"));
  actionSetupGlobal->setWhatsThis(actionText(tr("Global Setup"), tr("Allows setting up the type of calculation."),
                                  tr("Depending on the type of calculation chosen, extra setup windows will "
                                     "become available. The global setup cannot be changed when a calculation "
                                     "is running."),
                                   IconSets::SetupGlobal));
  connect(actionSetupGlobal, SIGNAL(activated()), this, SLOT(setupGlobal()));

  ///// Setup->Energy & Forces
  actionSetupBrabo = new QAction(IconSets::getIconSet(IconSets::SetupBrabo), tr("&Energy && Forces..."), Key_F3, this);
  actionSetupBrabo->setToolTip(tr("Setup energy & forces"));
  actionSetupBrabo->setWhatsThis(actionText(tr("Energy & Forces Setup"), tr("Allows setting up all aspects of the calculation of the energy and forces."),
                                 tr("An interface is provided for easily changing the common options, but experts can use the "
                                    "'Extra' tables for each section of the input file for providing new or other options not covered. "
                                    "From the settings an input file will be generated which can be checked before accepting the changes. "
                                    "Even during the run of a calculation all settings can be changed. They will come into effect "
                                    "the next  time an energy evaluation is started."),
                                 IconSets::SetupBrabo));
  connect(actionSetupBrabo, SIGNAL(activated()), this, SLOT(setupBrabo()));

  ///// Setup->Geometry Optimization
  actionSetupRelax = new QAction(IconSets::getIconSet(IconSets::SetupRelax), tr("Geometry &Optimization..."), Key_F4, this);
  actionSetupRelax->setToolTip(tr("Setup geometry optimization"));
  actionSetupRelax->setWhatsThis(actionText(tr("Geometry Optimization Setup"), tr("Allows setting up all aspects of the geometry optimization procedure."),
                                 tr("An interface is provided for easily changing the common options, but experts can use the "
                                    "'Extra' table for providing new or other options not covered. "
                                    "From the settings an input file will be generated which can be checked before accepting the changes. "
                                    "Even during the run of a calculation all settings can be changed. They will come into effect "
                                    "the next  time an optimization step is started."),
                                 IconSets::SetupRelax));
  connect(actionSetupRelax, SIGNAL(activated()), this, SLOT(setupRelax()));

  ///// Setup->Frequencies
  actionSetupFreq = new QAction(IconSets::getIconSet(IconSets::SetupFreq), tr("&Frequencies..."), Key_F5, this);
  actionSetupFreq->setText(tr("Setup frequencies"));
  connect(actionSetupFreq, SIGNAL(activated()), this, SLOT(setupFreq()));

  ///// Setup->Crystal
  actionSetupBuur = new QAction(IconSets::getIconSet(IconSets::SetupBuur), tr("&Crystal..."), Key_F6, this);  
  actionSetupBuur->setText(tr("Setup crystal"));
  connect(actionSetupBuur, SIGNAL(activated()), this, SLOT(setupBuur()));

  ///// Run->Start
  actionRunStart = new QAction(IconSets::getIconSet(IconSets::Start), tr("S&tart"), Key_F9, this);
  actionRunStart->setToolTip(tr("Start the calculation"));
  actionRunStart->setToggleAction(true);
  actionRunStart->setWhatsThis(actionText(tr("Start Calculation"), tr("Starts a calculation."),
                               tr("A running calculation that is paused can be continued in the same way. "),
                               IconSets::Start));                                            
  connect(actionRunStart, SIGNAL(activated()), this, SLOT(runStart()));

  ///// Run->Pause
  actionRunPause = new QAction(IconSets::getIconSet(IconSets::Pause), tr("&Pause"), Key_F10, this);
  actionRunPause->setToolTip(tr("Pause the calculation"));
  actionRunPause->setToggleAction(true);
  actionRunPause->setWhatsThis(actionText(tr("Pause Calculation"), tr("Pauses a running calculation."),
                               tr("When the calculation is already paused, it can be continued with this command. "
                                  "Only when the calculation is neither running nor paused, this command is not available. "
                                  "When pausing a calculation, it will be effectively paused when the current step "
                                  "(energy evaluation, charge calculation, optimization, etc.) has ended. Single Point "
                                  "Energy calculations cannot be paused as they consist of only one step."),
                               IconSets::Pause));
  connect(actionRunPause, SIGNAL(activated()), this, SLOT(runPause()));

  ///// Run->Stop
  actionRunStop = new QAction(IconSets::getIconSet(IconSets::Stop), tr("&Stop..."), Key_F11, this);
  actionRunStop->setToolTip(tr("Stop the calculation"));
  actionRunStop->setWhatsThis(actionText(tr("Stop Calculation"), tr("Stops a running calculation."),
                              tr("The question will be asked whether to stop immediately or after the current step "
                                 "has ended. Stopping immediately will kill the running process."),
                              IconSets::Stop));
  connect(actionRunStop, SIGNAL(activated()), this, SLOT(runStop()));

  ///// Run->Write
  actionRunWrite = new QAction(IconSets::getIconSet(IconSets::Write), tr("&Write input"), 0, this);
  actionRunWrite->setToolTip(tr("Write the input files"));
  actionRunWrite->setWhatsThis(actionText(tr("Write Input Files"), tr("Writes all input files."),
                               tr("Although all needed input files are written at the start of a calculation and "
                                  "everytime the setup is being changed during the run, the input file for the evaluation "
                                  "of energy and forces is not written. This command can thus be used to get this and "
                                  "other files without having to start a calculation. They can then be used to start the "
                                  "calculation outside of the program, for example."),
                               IconSets::Write));
  connect(actionRunWrite, SIGNAL(activated()), this, SLOT(runWrite()));        

  ///// Run->Clean
  actionRunClean = new QAction(IconSets::getIconSet(IconSets::Clean), tr("&Clean directory..."), 0, this);
  actionRunClean->setToolTip(tr("Clean the calculation directory"));
  actionRunClean->setWhatsThis(actionText(tr("Clean Directory"), tr("Cleans up the calculation directory."),
                               tr("All files pertaining to the calculation will be removed, including (saved) in- and output files, "
                                  "binary files, starting vectors, fortran files and others."),
                               IconSets::Clean));
  connect(actionRunClean, SIGNAL(activated()), this, SLOT(runClean()));
  
  ///// Run->View Results
  actionResultsViewOutput = new QAction(IconSets::getIconSet(IconSets::Outputs), tr("&View results..."), Key_F12, this);
  actionResultsViewOutput->setToolTip(tr("View the output file(s)"));
  actionResultsViewOutput->setWhatsThis(actionText(tr("View Results"), tr("Shows the output file(s)."),
                                        tr("A table will be shown with all output files that were found in the calculation "
                                           "directory. They can subsequently be visualized."),
                                        IconSets::Outputs));
  connect(actionResultsViewOutput, SIGNAL(activated()), this, SLOT(resultsViewOutput()));

  ///// Tools->Show Density Map
  actionToolsPlotMap = new QAction(QIconSet(), tr("Show &density map..."), 0, this);
  actionToolsPlotMap->setToolTip(tr("Show a 2D density map"));
  actionToolsPlotMap->setWhatsThis(actionText(tr("Show Density Map"), tr("Shows generated density maps."),
                                   tr("A window will be shown allowing loading and analysis of density map files "
                                      "(extension .map.1). A number of visualization options are available and the "
                                      "resulting image can be exported to a file.")));
  connect(actionToolsPlotMap, SIGNAL(activated()), this, SLOT(toolsPlotMap()));

  ///// Tools->Show 3D orbitals
  actionToolsOrbitals = new QAction(QIconSet(), tr("Show &orbitals..."), 0, this);
  actionToolsOrbitals->setToolTip(tr("Show 3D orbitals"));
  actionToolsOrbitals->setWhatsThis(actionText(tr("Show Orbitals"), tr("Shows orbitals of hydrogen-like atoms."),
                                    tr("These orbitals are calculated analytically and shown in a 3D scene much like "
                                       "the 3D scene of a calculation (and allowing similar manipulations). "
                                       "Quantum numbers, colors and rendering types can be adjusted. ")));
  connect(actionToolsOrbitals, SIGNAL(activated()), this, SLOT(toolsOrbitals()));  

  ///// Help->Contents
  actionHelp = new QAction(IconSets::getIconSet(IconSets::Help), tr("Contents..."), Qt::Key_F1, this);
  actionHelp->setToolTip(tr("Shows useful info"));
  actionHelp->setWhatsThis(actionText(tr("Contents of Help"), tr("Shows a brief explanation of the program and the use of the context-sensitive help system."),
                           QString::null, IconSets::Help));
  connect(actionHelp, SIGNAL(activated()), this, SLOT(helpHelp()));

  ///// Help->Context Sensitive Help
  actionWhatsThis = new QAction(IconSets::getIconSet(IconSets::WhatsThis), tr("Context sensitive help..."), Qt::SHIFT + Qt::Key_F1, this);
  actionWhatsThis->setToolTip(tr("Shows context sensitive help"));
  actionWhatsThis->setWhatsThis(actionText(tr("Context Sensitive Help"), tr("Well, if you are reading this, you already know how this thing works!."),
                           QString::null, IconSets::WhatsThis));
  connect(actionWhatsThis, SIGNAL(activated()), this, SLOT(helpWhatsThis()));

  ///// Help->Credits
  actionCredits = new QAction(QIconSet(), tr("Credits..."), 0, this);
  actionCredits->setToolTip(tr("Shows some credits"));
  actionCredits->setWhatsThis(actionText(tr("Credits"), tr("Shows a window where some credit is given to the developers of the building blocks of this program.")));
  connect(actionCredits, SIGNAL(activated()), this, SLOT(helpCredits()));

  ///// Help->About
  actionAbout = new QAction(QIconSet(), tr("About..."), 0, this);
  actionAbout->setToolTip(tr("Shows an About box"));
  actionAbout->setWhatsThis(actionText(tr("About"), tr("Gives some info about") + " " + Version::appName + ".",
                            tr("It is the same window you see flashing by when starting the application")));
  connect(actionAbout, SIGNAL(activated()), this, SLOT(helpAbout()));
}

///// initMenuBar /////////////////////////////////////////////////////////////
void Xbrabo::initMenuBar()
/// Creates the menu.
{  
  ///// fileMenu
  QPopupMenu* fileMenu = new QPopupMenu();
  actionFileNew->addTo(fileMenu);
  actionFileOpen->addTo(fileMenu);
  fileMenu->insertSeparator();
  actionFileSave->addTo(fileMenu);
  actionFileSaveAs->addTo(fileMenu);
  actionFileClose->addTo(fileMenu);
  fileMenu->insertSeparator();
  actionFileQuit->addTo(fileMenu);

  ///// editMenu
  QPopupMenu* editMenu=new QPopupMenu();
  actionEditCut->addTo(editMenu);
  actionEditCopy->addTo(editMenu);
  actionEditPaste->addTo(editMenu);
  editMenu->insertSeparator();
  actionEditPrefs->addTo(editMenu);

  ///// viewMenu
  QPopupMenu* viewMenu = new QPopupMenu();
  viewMenu->setCheckable(true);
    QPopupMenu* toolbarMenu = new QPopupMenu(viewMenu);
    toolbarMenu->setCheckable(true);
    actionViewToolBarStandard->addTo(toolbarMenu);
    actionViewToolBarCalculation->addTo(toolbarMenu);
    actionViewToolBarCoordinates->addTo(toolbarMenu);
  viewMenu->insertItem(tr("&Toolbars"), toolbarMenu);
  actionViewStatusBar->addTo(viewMenu);
  actionViewTaskBar->addTo(viewMenu);

  ///// moleculeMenu
  QPopupMenu* moleculeMenu = new QPopupMenu();
  actionMoleculeReadCoordinates->addTo(moleculeMenu);
  actionMoleculeSaveCoordinates->addTo(moleculeMenu);
  actionMoleculeAddAtom->addTo(moleculeMenu);
  actionMoleculeDeleteSelection->addTo(moleculeMenu);
  QPopupMenu* alterMenu = new QPopupMenu();
    actionMoleculeAlterCartesian->addTo(alterMenu);
    actionMoleculeAlterInternal->addTo(alterMenu);
    moleculeMenu->insertItem(tr("A&lter"), alterMenu);
  QPopupMenu* selectMenu = new QPopupMenu();
    actionMoleculeSelectAll->addTo(selectMenu);
    actionMoleculeSelectNone->addTo(selectMenu);
    moleculeMenu->insertItem(tr("&Select"), selectMenu);
  actionMoleculeSelection->addTo(moleculeMenu);
  moleculeMenu->insertSeparator();
  actionMoleculeDisplayMode->addTo(moleculeMenu);
  actionMoleculeDensity->addTo(moleculeMenu);
  actionMoleculeAnimate->addTo(moleculeMenu);
  //actionMoleculeFPS->addTo(moleculeMenu);
  actionMoleculeImage->addTo(moleculeMenu);  
  QPopupMenu* resetMenu = new QPopupMenu();
    actionMoleculeCenterView->addTo(resetMenu);
    actionMoleculeResetOrientation->addTo(resetMenu);
    actionMoleculeZoomFit->addTo(resetMenu);
    actionMoleculeResetView->addTo(resetMenu);    
    moleculeMenu->insertItem(tr("&Reset"), resetMenu);

  ///// setupMenu
  QPopupMenu* setupMenu = new QPopupMenu();
  actionSetupGlobal->addTo(setupMenu);
  setupMenu->insertSeparator();
  actionSetupBrabo->addTo(setupMenu);
  actionSetupRelax->addTo(setupMenu);
  //actionSetupFreq->addTo(setupMenu);
  //actionSetupBuur->addTo(setupMenu);  

  ///// runMenu
  QPopupMenu* runMenu = new QPopupMenu();
  actionRunStart->addTo(runMenu);
  actionRunPause->addTo(runMenu);
  actionRunStop->addTo(runMenu);
  actionRunWrite->addTo(runMenu);
  actionRunClean->addTo(runMenu);
  runMenu->insertSeparator();
  actionResultsViewOutput->addTo(runMenu);
  
  ///// resultsMenu
  //resultsMenu = new QPopupMenu();
  //actionResultsViewOutput->addTo(resultsMenu);

  ///// toolsMenu
  QPopupMenu* toolsMenu = new QPopupMenu();
  actionToolsPlotMap->addTo(toolsMenu);
  actionToolsOrbitals->addTo(toolsMenu);
    
  ///// helpMenu
  QPopupMenu* helpMenu = new QPopupMenu();
  actionHelp->addTo(helpMenu);
  actionWhatsThis->addTo(helpMenu);
  actionCredits->addTo(helpMenu);
  actionAbout->addTo(helpMenu);

  ///// menuBar
  menuBar()->insertItem(tr("&File"), fileMenu);
  menuBar()->insertItem(tr("&Edit"), editMenu);
  menuBar()->insertItem(tr("&View"), viewMenu);
  menuBar()->insertItem(tr("&Molecule"), moleculeMenu);
  menuBar()->insertItem(tr("&Setup"), setupMenu);
  menuBar()->insertItem(tr("&Run"), runMenu);
  //menuBar()->insertItem(tr("Res&ults"), resultsMenu);
  menuBar()->insertItem(tr("&Tools"), toolsMenu);
  menuBar()->insertItem(tr("&Window"), windowMenu());
  menuBar()->insertItem(tr("&Help"), helpMenu);

  ///// other
  setMenuForSDIModeSysButtons( menuBar());
}

///// initToolBar /////////////////////////////////////////////////////////////
void Xbrabo::initToolBars()
/// Creates the toolbars.
{  
  ///// ToolBarStandard
  ToolBarStandard = new QToolBar(this);
  ToolBarStandard->hide();
  ToolBarStandard->setLabel(tr("Standard"));
  actionFileNew->addTo(ToolBarStandard);
  actionFileOpen->addTo(ToolBarStandard);
  actionFileSave->addTo(ToolBarStandard);
  ToolBarStandard->addSeparator();
  actionEditCut->addTo(ToolBarStandard);
  actionEditCopy->addTo(ToolBarStandard);
  actionEditPaste->addTo(ToolBarStandard);
  ToolBarStandard->addSeparator();
  actionEditPrefs->addTo(ToolBarStandard);  
  ToolBarStandard->addSeparator();
  actionHelp->addTo(ToolBarStandard);
  actionWhatsThis->addTo(ToolBarStandard);

  ///// ToolBarCalculation
  ToolBarCalculation = new QToolBar(this);
  ToolBarCalculation->hide();
  ToolBarCalculation->setLabel(tr("Calculation"));
  actionSetupGlobal->addTo(ToolBarCalculation);
  ToolBarCalculation->addSeparator();
  actionSetupBrabo->addTo(ToolBarCalculation);
  actionSetupRelax->addTo(ToolBarCalculation);
  //actionSetupFreq->addTo(ToolBarCalculation);
  //actionSetupBuur->addTo(ToolBarCalculation);  
  ToolBarCalculation->addSeparator();
  actionRunStart->addTo(ToolBarCalculation);
  actionRunPause->addTo(ToolBarCalculation);
  actionRunStop->addTo(ToolBarCalculation);
  ToolBarCalculation->addSeparator();
  actionResultsViewOutput->addTo(ToolBarCalculation);
  
  ///// ToolBarCoordinates
  ToolBarCoordinates = new QToolBar(this);
  ToolBarCoordinates->hide();
  ToolBarCoordinates->setLabel(tr("Molecule"));
  actionMoleculeReadCoordinates->addTo(ToolBarCoordinates);
  actionMoleculeSaveCoordinates->addTo(ToolBarCoordinates);
  actionMoleculeSelection->addTo(ToolBarCoordinates);
  actionMoleculeAnimate->addTo(ToolBarCoordinates);
  actionMoleculeImage->addTo(ToolBarCoordinates);

  ///// show all toolbars
  ToolBarStandard->show();
  ToolBarCalculation->show();
  ToolBarCoordinates->show();

  ///// set the label for the taskbar
  m_pTaskBar->setLabel(tr("Taskbar"));

  ///// make sure the mainwindow is updated in Toplevel Mode when toolbars are changed
  //connect(ToolBarStandard, SIGNAL(orientationChanged(Orientation)), this, SLOT(fixToplevelModeHeight()));
  connect(ToolBarStandard, SIGNAL(placeChanged(QDockWindow::Place)), this, SLOT(fixToplevelModeHeight()));
  connect(ToolBarStandard, SIGNAL(visibilityChanged(bool)), this, SLOT(fixToplevelModeHeight()));
  //connect(ToolBarCalculation, SIGNAL(orientationChanged(Orientation)), this, SLOT(fixToplevelModeHeight()));
  connect(ToolBarCalculation, SIGNAL(placeChanged(QDockWindow::Place)), this, SLOT(fixToplevelModeHeight()));
  connect(ToolBarCalculation, SIGNAL(visibilityChanged(bool)), this, SLOT(fixToplevelModeHeight()));
  //connect(ToolBarCoordinates, SIGNAL(orientationChanged(Orientation)), this, SLOT(fixToplevelModeHeight()));
  connect(ToolBarCoordinates, SIGNAL(placeChanged(QDockWindow::Place)), this, SLOT(fixToplevelModeHeight()));
  connect(ToolBarCoordinates, SIGNAL(visibilityChanged(bool)), this, SLOT(fixToplevelModeHeight()));
  //connect(m_pTaskBar, SIGNAL(orientationChanged(Orientation)), this, SLOT(fixToplevelModeHeight()));
  connect(m_pTaskBar, SIGNAL(placeChanged(QDockWindow::Place)), this, SLOT(fixToplevelModeHeight()));
  connect(m_pTaskBar, SIGNAL(visibilityChanged(bool)), this, SLOT(fixToplevelModeHeight()));
}

///// updatePreferences ///////////////////////////////////////////////////////
void Xbrabo::updatePreferences()
/// Updates the preferences of all calculations. This is done
/// by calling static functions of BraboBase, GLView and GLSimpleMoleculeView.
{  
  ///// BraboBase
  BraboBase::setPreferredBasisset(editPreferences->preferredBasisset());

  ///// GlView
  GLView::setParameters(editPreferences->getGLBaseParameters());

  ///// GLSimpleMoleculeView
  GLSimpleMoleculeView::setParameters(editPreferences->getGLMoleculeParameters());
}

///// updateToolbarsInfo //////////////////////////////////////////////////////
void Xbrabo::updateToolbarsInfo()
/// Updates editPreferences with info about the toolbars.
{
  QString result;
  QTextStream stream(&result, IO_WriteOnly);
  stream << *this;
  editPreferences->setToolbarsInfo(result, statusBar()->isVisible());
}

///// restoreToolbars /////////////////////////////////////////////////////////
void Xbrabo::restoreToolbars()
/// Restores the toolbars. Should be called
/// when Xbrabo is visible, otherwise the toolbarMenu cannot be updated.
{
  QString toolbars;
  bool tempstatus;
  editPreferences->getToolbarsInfo(tempstatus, toolbars);
  
  if(!toolbars.isEmpty())
  {
    QTextStream stream(&toolbars, IO_ReadOnly);
    stream >> *this;
    if(tempstatus)
      statusBar()->show();
    else
      statusBar()->hide();    
  }
  actionViewToolBarStandard->setOn(ToolBarStandard->isVisible());
  actionViewToolBarCalculation->setOn(ToolBarCalculation->isVisible());
  actionViewToolBarCoordinates->setOn(ToolBarCoordinates->isVisible());
  actionViewStatusBar->setOn(statusBar()->isVisible());
  actionViewTaskBar->setOn(m_pTaskBar->isVisible());
  fixToplevelModeHeight();
}

///// actionText //////////////////////////////////////////////////////////////
QString Xbrabo::actionText(const QString title, const QString brief, const QString details, const IconSets::IconSetID iconID)
/// Constructs a What's This text for actions with proper richtext formatting.
/// \arg title: The title of the action
/// \arg brief: A brief one line description
/// \arg details: A longer detailed description (optional)
/// \arg iconID: translates to the name of a pixmap in the default mimesource factory (optional) 
{
  QString result = "<big><b>" + title + "</b></big> <hr>"; // keep the space before <hr> until this bug is solved (never for Qt pre 3.3.6)
  if(iconID != IconSets::LastIcon)
    result += "<img source=\"" + IconSets::factoryName(iconID) + "\" align=\"right\"/>";
  result += "<br>" + brief;
  if(!details.isEmpty())
    result += "<br>" + details;
  return result;
}

