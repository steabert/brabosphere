/***************************************************************************
                         crdview.cpp  -  description
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

///// Comments ////////////////////////////////////////////////////////////////
/*!
  \class CrdView
  \brief This class is a lightweight version of Xbrabo.

  It is to be used solely for viewing and converting
  coordinates. It uses as much code from Xbrabo as possible.

*/
/// \file
/// Contains the implementation of the class CrdView.

///// Header files ////////////////////////////////////////////////////////////

// Qt header files
#include <qaccel.h>
#include <qaction.h>
#include <qapplication.h>
#include <qcheckbox.h>
#include <qcombobox.h>
#include <qfiledialog.h>
#include <qlabel.h>
#include <qmenubar.h>
#include <qmsgbox.h>
#include <qpopupmenu.h>
#include <qsettings.h>
#include <qstatusbar.h>
#include <qtextedit.h>
#include <qtoolbar.h>
#include <qwhatsthis.h>

// CrdView header files
#include "atomset.h"
#include "crdfactory.h"
#include "crdview.h"
#include "iconsets.h"
#include "glsimplemoleculeview.h"
#include "moleculepropertieswidget.h"
#include "textviewwidget.h"
#include "version.h"

///////////////////////////////////////////////////////////////////////////////
///// Public Member Functions                                             /////
///////////////////////////////////////////////////////////////////////////////

///// Constructor /////////////////////////////////////////////////////////////
CrdView::CrdView() : QMainWindow(0, 0, Qt::WType_TopLevel | Qt::WDestructiveClose)
{
  setCaption(tr("CrdView "));

  // create an AtomSet for use with the GLSimpleMoleculeView
  atoms = new AtomSet();

  // create a GLSimpleMoleculeView for displaying the coordinates
  glview = new GLSimpleMoleculeView(atoms, this);
  setCentralWidget(glview);

  initActions(); // needs a valid glview pointer
  initMenuBar();
  initToolBar();
  readSettings();
}

///// Destructor //////////////////////////////////////////////////////////////
CrdView::~CrdView()
{
  delete atoms;
  saveSettings();
}

///// readCoordinates /////////////////////////////////////////////////////////
void CrdView::readCoordinates(QString filename)
{
  ///// Public member function. Loads the given filename for viewing.

  unsigned short int result = CrdFactory::readFromFile(atoms, filename);
  switch(result)
  {
    case CrdFactory::OK:
      glview->updateAtomSet(true);
      setCaption(tr("CrdView") + QString(" - ") + filename);
      break;
    case CrdFactory::UnknownExtension:
      QMessageBox::warning(this, tr("Unknown format"), "The file " + filename + " has an unknown extension", QMessageBox::Ok, QMessageBox::NoButton);
      break;
    case CrdFactory::ErrorOpen:
      QMessageBox::warning(this, tr("Error opening file"), "The file " + filename + " could not be opened for reading", QMessageBox::Ok, QMessageBox::NoButton);
      break;
    case CrdFactory::ErrorRead:
      QMessageBox::warning(this, tr("Parse error"), "The contents of the file " + filename + " could not be parsed correctly", QMessageBox::Ok, QMessageBox::NoButton);
      break;
    case CrdFactory::UnknownFormat:
      QMessageBox::warning(this, tr("Unknown format"), "The format (normal/extended) of the file " + filename + " could not be detected correctly", QMessageBox::Ok, QMessageBox::NoButton);
      break;
  }

}

///////////////////////////////////////////////////////////////////////////////
///// Private slots                                                       /////
///////////////////////////////////////////////////////////////////////////////

///// fileOpen ////////////////////////////////////////////////////////////////
void CrdView::fileOpen()
/// Opens an existing coordinate file.
{
  statusBar()->message(tr("Opening a coordinate file..."));

  readCoordinates();

  statusBar()->message(tr("Loaded coordinates"), 2000);
}

///// fileSave ////////////////////////////////////////////////////////////////
void CrdView::fileSave()
/// Saves the coordinates in another format.
{
  statusBar()->message(tr("Saving coordinates in a new format..."));

  unsigned short int result = CrdFactory::writeToFile(atoms);
  switch(result)
  {
    case CrdFactory::UnknownExtension:
      QMessageBox::warning(this, "Unknown format", "The file has an unknown extension", QMessageBox::Ok, QMessageBox::NoButton);
      break;
    case CrdFactory::ErrorOpen:
      QMessageBox::warning(this, "Error opening file", "The file could not be opened for writing", QMessageBox::Ok, QMessageBox::NoButton);
      break;
    case CrdFactory::ErrorWrite:
      QMessageBox::warning(this, "Error writing file", "The file could not be written", QMessageBox::Ok, QMessageBox::NoButton);
      break;
  }
  statusBar()->clear();
}

///// fileExport //////////////////////////////////////////////////////////////
void CrdView::fileExport()
/// Exports the current view to an image.
{
  statusBar()->message(tr("Exporting view..."));

  glview->saveImage();

  statusBar()->clear();
}

///// filePreferences /////////////////////////////////////////////////////////
void CrdView::filePreferences()
/// Temporarily changes the viewing preferences. Defaults have to be changed
/// in the preferences of Brabosphere.
{
  statusBar()->message(tr("Changing Preferences..."));

  MoleculePropertiesWidget* properties = new MoleculePropertiesWidget(this, 0, true);
  properties->ComboBoxRenderingType->setCurrentItem(glview->displayStyle(GLSimpleMoleculeView::Molecule));
  properties->ComboBoxForces->setCurrentItem(glview->displayStyle(GLSimpleMoleculeView::Forces));
  properties->CheckBoxElement->setChecked(glview->isShowingElements());
  properties->CheckBoxNumber->setChecked(glview->isShowingNumbers());
  properties->LabelCharge->hide();
  properties->ComboBoxCharge->hide();

  if(properties->exec() == QDialog::Accepted)
  {
    glview->setDisplayStyle(GLSimpleMoleculeView::Molecule, properties->ComboBoxRenderingType->currentItem());
    glview->setDisplayStyle(GLSimpleMoleculeView::Forces, properties->ComboBoxForces->currentItem());
    glview->setLabels(properties->CheckBoxElement->isChecked(), properties->CheckBoxNumber->isChecked(), 0); // 0 = no charge
    glview->updateGL();
  }
  delete properties;
  statusBar()->clear();
}

///// viewToolBar /////////////////////////////////////////////////////////////
void CrdView::viewToolBar(bool toggle)
/// Toggles the visibility of the toolbar.
{
  statusBar()->message(tr("Toggling toolbar..."), 1000);

  // turn toolbar on or off
  if (toggle == false)
    ToolBarFile->hide();
  else
    ToolBarFile->show();
}

///// viewStatusBar ///////////////////////////////////////////////////////////
void CrdView::viewStatusBar(bool toggle)
/// Toggles the visibility of the status bar.
{
  statusBar()->message(tr("Toggling statusbar..."), 1000);

  //turn statusbar on or off
  if (toggle == false)
    statusBar()->hide();
  else
    statusBar()->show();
}

///// helpHelp ////////////////////////////////////////////////////////////////
void CrdView::helpHelp()
/// Show some general information
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
    "<ul><li>CrdView provides a single 3D window in which coordinates can be read and displayed. "
    "Almost all preferences have to be changed in Brabosphere. CrdView just uses the same settings. "
    "Only the 'Display Mode' of the coordinates can be changed from the default.</li>"
    "<li>CrdView can act as a graphical front-end for OpenBabel. Not only by loading and saving coordinates, "
    "but also by command line conversion: <b>crdview [-bnf] &lt;input file&gt; &lt;output file&gt;</b>. "
    "The '-bnf' option forces using the normal format for writing a BRABO coordinate file instead of the "
    "default extended format. The in- and output files can be any file format recognized by CrdView.</li>"
    "<li>If CrdView is started with exactly one command line option, it will be interpreted as the coordinate file "
    "to load. This way, the program can be used to display a large number of coordinate file formats with a simple click.</li></ul>"));

  context->TextEdit->setWordWrap(QTextEdit::WidgetWidth);
  QWhatsThis::add(context->TextEdit, "Yes, you found out how to use the context sensitive help!");
  context->TextEdit->setFont(QApplication::font());
  context->setCaption(tr("Contents"));
  context->exec();
  delete context;

  statusBar()->clear();
}

///// helpWhatsThis ///////////////////////////////////////////////////////////
void CrdView::helpWhatsThis()
/// Enters What's This mode
{
  statusBar()->message(tr("Entering What's This mode..."), 1000);

  QWhatsThis::enterWhatsThisMode();
}

///// helpAbout ///////////////////////////////////////////////////////////////
void CrdView::helpAbout()
/// Show some program information.
{
  statusBar()->message(tr("About the program..."));

  QMessageBox::about(this,tr("About..."),
                     tr("CrdView") + "\n" +
                     tr("Version") + " " + Version::appVersion + " (" + tr("build") + " " + Version::appBuild + ")\n" +
                     tr("Part of") + " " + Version::appName + " " + Version::appVersion + "\n" +
                     tr("(c) 2006 by Ben Swerts\n")
                    );
  statusBar()->clear();
}

///////////////////////////////////////////////////////////////////////////////
///// Protected Member Functions                                          /////
///////////////////////////////////////////////////////////////////////////////

/*//// showEvent ///////////////////////////////////////////////////////////////
void CrdView::showEvent (QShowEvent*)
{
  ///// Protected member function. Called on showing the mainwindow

  static bool ok = false;
  if(!ok)
  {
    glview->resetView(false);
    ok = true;
  }
}// */

///////////////////////////////////////////////////////////////////////////////
///// Private Member Functions                                            /////
///////////////////////////////////////////////////////////////////////////////

///// initActions /////////////////////////////////////////////////////////////
void CrdView::initActions()
{
  ///// Private member function. Creates the actions and connects them to the
  ///// relevant slots

  ///// create the actions and connect them to the slots
  ///// File->Open
  actionFileOpen = new QAction(IconSets::getIconSet(IconSets::Open), tr("&Open..."), CTRL+Key_O, this);
  actionFileOpen->setToolTip(tr("Open an existing coordinate file"));
  actionFileOpen->setWhatsThis(actionText(tr("Open File"), tr("Opens an existing coordinate file."),
                               tr("Any supported file format can be read, including Brabosphere calculations."),
                               IconSets::Open));
  connect(actionFileOpen, SIGNAL(activated()), this, SLOT(fileOpen()));

  // File->Save As
  actionFileSave = new QAction(IconSets::getIconSet(IconSets::Save), tr("&Save As..."), CTRL+Key_S, this);
  actionFileSave->setText(tr("Save the current coordinates"));
  actionFileSave->setWhatsThis(actionText(tr("Save File"), tr("Saves the currently shown coordinates."),
                               tr("Any supported file format can be chosen as a target."),
                               IconSets::Save));
  connect(actionFileSave, SIGNAL(activated()), this, SLOT(fileSave()));

  // File->Preferences
  actionFilePreferences = new QAction(IconSets::getIconSet(IconSets::Prefs), tr("P&references..."), 0, this);
  actionFilePreferences->setText(tr("Change the default viewing preferences"));
  actionFilePreferences->setWhatsThis(actionText(tr("Preferences"), tr("Changes the viewing preferences."),
                                      tr("The only view settings that can be changed are the ones corresponding to Display "
                                         "Mode in Brabosphere. They take effect immediately. Other settings will have to "
                                         "be changed in Brabosphere itself."
                                         "CrdView will have to be restarted for those settings to take effect."),
                                      IconSets::Prefs));
  connect(actionFilePreferences, SIGNAL(activated()), this, SLOT(filePreferences()));

  // File->Export
  actionFileExport = new QAction(IconSets::getIconSet(IconSets::Image), tr("&Export..."), 0, this);
  actionFileExport->setText(tr("Export the current view to an image"));
  actionFileExport->setWhatsThis(actionText(tr("Export File"), tr("Exports the current view to an image."),
                                 tr("A number of formats can be chosen from. "
                                    "The size of the image will be determined by the size of the scene's window."),
                                 IconSets::Image));
  connect(actionFileExport, SIGNAL(activated()), this, SLOT(fileExport()));

  // File->Quit
  actionFileQuit = new QAction(QIconSet(), tr("E&xit"), CTRL+Key_Q, this);
  actionFileQuit->setText(tr("Quits the application"));
  actionFileQuit->setWhatsThis(actionText(tr("Exit"), tr("Exits CrdView."),
                               tr("The application will be closed without asking confirmation.")));
  connect(actionFileQuit, SIGNAL(activated()), this, SLOT(close()));

  // View->Toolbar
  actionViewToolBar = new QAction(QIconSet(), tr("Tool&bar"), 0, this);
  actionViewToolBar->setText(tr("Toggles the visibility of the toolbar"));
  actionViewToolBar->setToggleAction(true);
  actionViewToolBar->setWhatsThis(actionText(tr("View Toolbar"), tr("Toggles the visibility of the toolbar."),
                                  tr("This is the toolbar containing shortcuts to selected entries in "
                                     "the File menu.")));
  connect(actionViewToolBar, SIGNAL(toggled(bool)), this, SLOT(viewToolBar(bool)));

  // View->Statusbar
  actionViewStatusBar = new QAction(QIconSet(), tr("&Statusbar"), 0, this);
  actionViewStatusBar->setText(tr("Toggles the visibility of the statusbar"));
  actionViewStatusBar->setToggleAction(true);
  actionViewStatusBar->setWhatsThis(actionText(tr("View Statusbar"), tr("Toggles the visibility of the statusbar."),
                                    tr("This is de toolbar at the bottom of the main window which displays temporary messages.")));
  connect(actionViewStatusBar, SIGNAL(toggled(bool)), this, SLOT(viewStatusBar(bool)));

  // Reset->Translation
  actionCenterView = new QAction(QIconSet(), tr("&Translation"), 0, this);
  actionCenterView->setText(tr("Center the molecule"));
  actionCenterView->setWhatsThis(actionText(tr("Reset Translation"), tr("Restores the center of the molecule to the center of the 3D scene."))); 
  connect(actionCenterView, SIGNAL(activated()), glview, SLOT(centerView()));

  // Reset->Orientation
  actionResetOrientation = new QAction(QIconSet(), tr("&Orientation"), 0, this);
  actionResetOrientation->setText(tr("Reset the orientation of the molecule"));
  actionResetOrientation->setWhatsThis(actionText(tr("Reset Orientation"), tr("Resets the rotational axes of the molecule."),
                                       tr("After the reset, the X-axis will again point to the right, "
                                          "the Y-axis to the top and the Z-axis into the screen.")));
  connect(actionResetOrientation, SIGNAL(activated()), glview, SLOT(resetOrientation()));

  // Reset->Zoom
  actionZoomFit = new QAction(QIconSet(), tr("&Zoom"), 0, this);
  actionZoomFit->setText(tr("Zoom such that the molecule fits in the window"));
  actionZoomFit->setWhatsThis(actionText(tr("Reset Zoom"), tr("Zooms the scene such that the molecule fits exactly in the window.")));
  connect(actionZoomFit, SIGNAL(activated()), glview, SLOT(zoomFit()));

  // Reset->View
  actionResetView = new QAction(QIconSet(), tr("&View"), 0, this);
  actionResetView->setText(tr("Reset translation, orientation and zoom"));
  actionResetView->setWhatsThis(actionText(tr("Reset View"), tr("Resets the translation, orientation and zoom of the molecule.")));
  connect(actionResetView, SIGNAL(activated()), glview, SLOT(resetView()));

  // Select->All
  actionSelectAll = new QAction(QIconSet(), tr("&All"), Qt::CTRL + Qt::Key_A, this);
  actionSelectAll->setText(tr("Select all atoms"));
  actionSelectAll->setWhatsThis(actionText(tr("Select All"), tr("Selects all atoms.")));
  connect(actionSelectAll, SIGNAL(activated()), glview, SLOT(selectAll()));

  // Select->None
  actionSelectNone = new QAction(QIconSet(), tr("&None"), Qt::CTRL + Qt::SHIFT + Qt::Key_A, this);
  actionSelectNone->setText(tr("Deselect all atoms"));
  actionSelectNone->setWhatsThis(actionText(tr("Select None"), tr("Deselects all atoms.")));
  connect(actionSelectNone, SIGNAL(activated()), glview, SLOT(unselectAll()));

  // Help->Contents
  actionHelp = new QAction(IconSets::getIconSet(IconSets::Help), tr("Contents..."), Qt::Key_F1, this);
  actionHelp->setText(tr("Shows useful info"));
  actionHelp->setWhatsThis(actionText(tr("Contents of Help"), tr("Shows a brief explanation of the program and the use of the context-sensitive help system."),
                           QString::null, IconSets::Help));
  connect(actionHelp, SIGNAL(activated()), this, SLOT(helpHelp()));

  // Help->Context sensitive help
  actionWhatsThis = new QAction(IconSets::getIconSet(IconSets::WhatsThis), tr("Context sensitive help..."), Qt::SHIFT + Qt::Key_F1, this);
  actionWhatsThis->setText(tr("Shows context sensitive help"));
  actionWhatsThis->setWhatsThis(actionText(tr("Context Sensitive Help"), tr("Well, if you are reading this, you already know how this thing works!."),
                           QString::null, IconSets::WhatsThis));
  connect(actionWhatsThis, SIGNAL(activated()), this, SLOT(helpWhatsThis()));

  // Help->About
  actionAbout = new QAction(QIconSet(), tr("&About..."), 0, this);
  actionAbout->setText(tr("Shows an About box"));
  actionAbout->setWhatsThis(actionText(tr("About"), tr("Gives some info about CrdView.")));
  connect(actionAbout, SIGNAL(activated()), this, SLOT(helpAbout()));
}

///// initMenuBar /////////////////////////////////////////////////////////////
void CrdView::initMenuBar()
{
  ///// Private member function. Creates the menu.

  // fileMenu
  QPopupMenu* fileMenu = new QPopupMenu();
  actionFileOpen->addTo(fileMenu);
  fileMenu->insertSeparator();
  actionFileSave->addTo(fileMenu);
  actionFileExport->addTo(fileMenu);
  fileMenu->insertSeparator();
  actionFilePreferences->addTo(fileMenu);
  fileMenu->insertSeparator();
  actionFileQuit->addTo(fileMenu);

  // viewMenu
  QPopupMenu* viewMenu = new QPopupMenu();
  viewMenu->setCheckable(true);
  actionViewToolBar->addTo(viewMenu);
  actionViewStatusBar->addTo(viewMenu);

  // resetMenu
  QPopupMenu* resetMenu = new QPopupMenu();
  actionCenterView->addTo(resetMenu);
  actionResetOrientation->addTo(resetMenu);
  actionZoomFit->addTo(resetMenu);
  actionResetView->addTo(resetMenu);

  // selectMenu
  QPopupMenu* selectMenu = new QPopupMenu();
  actionSelectAll->addTo(selectMenu);
  actionSelectNone->addTo(selectMenu);

  // menuBar entry helpMenu
  QPopupMenu* helpMenu=new QPopupMenu();
  actionHelp->addTo(helpMenu);
  actionWhatsThis->addTo(helpMenu);
  actionAbout->addTo(helpMenu);

  // menuBar
  menuBar()->insertItem(tr("&File"), fileMenu);
  menuBar()->insertItem(tr("&View"), viewMenu);
  menuBar()->insertItem(tr("&Reset"), resetMenu);
  menuBar()->insertItem(tr("&Select"), selectMenu);
  menuBar()->insertItem(tr("&Help"), helpMenu);
}

///// initToolBar /////////////////////////////////////////////////////////////
void CrdView::initToolBar()
{
  ///// Private member function. Creates the toolbars

  // ToolBarFile
  ToolBarFile = new QToolBar(this);
  ToolBarFile->setCaption(tr("Standard"));
  actionFileOpen->addTo(ToolBarFile);
  actionFileSave->addTo(ToolBarFile);
  actionFileExport->addTo(ToolBarFile);
  ToolBarFile->addSeparator();
  actionFilePreferences->addTo(ToolBarFile);
  ToolBarFile->addSeparator();
  actionHelp->addTo(ToolBarFile);
  actionWhatsThis->addTo(ToolBarFile);
}

///// readSettings ////////////////////////////////////////////////////////////
void CrdView::readSettings()
/// Reads the settings from the Brabosphere settings file.
{
  ///// determine the OpenGL capabilities regarding the rendering of lines
  glview->makeCurrent();
  GLfloat lwrange[] = {0.0, 0.0};
  GLfloat lwgran[] = {0.0};
  glGetFloatv(GL_LINE_WIDTH_RANGE, lwrange);
  glGetFloatv(GL_LINE_WIDTH_GRANULARITY, lwgran);
  const int defaultLineWidth = static_cast<int>((lwrange[0] > 1.0f ? lwrange[0] : 1.0f)/lwgran[0]);
  qDebug("defaultLineWidth = %d", defaultLineWidth);

  QSettings settings;
  settings.setPath(Version::appCompany, Version::appName.lower(), QSettings::User);

#ifdef Q_OS_WIN32
  const QString unixPrefix = "/";
#else
  const QString unixPrefix = "/" + Version::appName.lower() + "/";
#endif
  ///// Brabosphere settings //////////
  QString prefix = unixPrefix + "preferences/";

  ///// Molecule
  GLBaseParameters glBaseParameters;
  GLMoleculeParameters glMoleculeParameters;
  glMoleculeParameters.defaultMoleculeStyle = settings.readNumEntry(prefix + "style_molecule", 3); // Ball & Stick
  glMoleculeParameters.defaultForcesStyle   = settings.readNumEntry(prefix + "style_forces", 2); // Tubes
  glMoleculeParameters.fastRenderLimit      = settings.readNumEntry(prefix + "fast_render_limit", 1000);
  glMoleculeParameters.showElements         = settings.readBoolEntry(prefix + "show_elements", false);
  glMoleculeParameters.showNumbers          = settings.readBoolEntry(prefix + "show_numbers", true);
  const int lineWidth                       = settings.readNumEntry(prefix + "size_lines", defaultLineWidth); 
  glMoleculeParameters.sizeLines            = static_cast<GLfloat>(lineWidth)*lwgran[0];
  qDebug("read sizeLines = %f",glMoleculeParameters.sizeLines);
  glMoleculeParameters.sizeBonds            = settings.readEntry(prefix + "size_bonds", QString::number(AtomSet::vanderWaals(1)/2.0)).toFloat()/2.0;
  glMoleculeParameters.sizeForces           = settings.readEntry(prefix + "size_forces", QString::number(AtomSet::vanderWaals(1)/2.0*1.1)).toFloat()/2.0;
  glMoleculeParameters.colorLabels          = settings.readNumEntry(prefix + "color_labels", QColor(0, 255, 0).rgb()); // green
  glMoleculeParameters.colorICs             = settings.readNumEntry(prefix + "color_internal_coordinates", QColor(0, 255, 255).rgb()); // cyan
  glBaseParameters.backgroundColor          = settings.readNumEntry(prefix + "color_background_gl", 0); // black
  glMoleculeParameters.colorSelections      = settings.readNumEntry(prefix + "color_selections", QColor(255, 255, 0).rgb()); // yellow
  glMoleculeParameters.opacitySelections    = settings.readNumEntry(prefix + "opacity_selections", 50);
  glMoleculeParameters.colorForces          = settings.readNumEntry(prefix + "color_forces", QColor(255, 255, 0).rgb()); //yellow
  glMoleculeParameters.forcesOneColor       = settings.readBoolEntry(prefix + "color_force_type", false); // atom color
  glMoleculeParameters.opacityForces        = settings.readNumEntry(prefix + "opacity_forces", 100);

  ///// Visuals
  //data.styleApplication  = settings.readNumEntry(prefix + "style", 0); // Startup style

  ///// OpenGL
  switch(settings.readNumEntry(prefix + "light_position", 2))
  {
    case 0: glBaseParameters.lightPositionX = -1.0f;
            glBaseParameters.lightPositionY =  1.0f;
            break;
    case 1: glBaseParameters.lightPositionX =  0.0f;
            glBaseParameters.lightPositionY =  1.0f;
            break;
    case 3: glBaseParameters.lightPositionX = -1.0f;
            glBaseParameters.lightPositionY =  0.0f;
            break;
    case 4: glBaseParameters.lightPositionX =  0.0f;
            glBaseParameters.lightPositionY =  0.0f;
            break;
    case 5: glBaseParameters.lightPositionX =  1.0f;
            glBaseParameters.lightPositionY =  0.0f;
            break;
    case 6: glBaseParameters.lightPositionX = -1.0f;
            glBaseParameters.lightPositionY = -1.0f;
            break;
    case 7: glBaseParameters.lightPositionX =  0.0f;
            glBaseParameters.lightPositionY = -1.0f;
            break;
    case 8: glBaseParameters.lightPositionX =  1.0f;
            glBaseParameters.lightPositionY = -1.0f;
            break;
    case 2:
    default: glBaseParameters.lightPositionX =  1.0f;
             glBaseParameters.lightPositionY =  1.0f;
             break;
  }
  glBaseParameters.lightPositionZ = 1.0f;
  glBaseParameters.lightColor               = settings.readNumEntry(prefix + "light_color", QColor(255,255,255).rgb());
  glBaseParameters.materialSpecular         = settings.readNumEntry(prefix + "material_specular", 80);
  glBaseParameters.materialShininess        = settings.readNumEntry(prefix + "material_shininess", 100);
  glBaseParameters.antialias                = settings.readBoolEntry(prefix + "antialias", true);
  glBaseParameters.smoothShading            = settings.readBoolEntry(prefix + "smooth_shading", true);
  glBaseParameters.depthCue                 = settings.readBoolEntry(prefix + "depth_cue", true);
  glMoleculeParameters.quality              = settings.readNumEntry(prefix + "quality", 22); // 31 slices
  glBaseParameters.perspectiveProjection    = settings.readBoolEntry(prefix + "perspective_projection", true);

  ///// Update
  GLView::setParameters(glBaseParameters);
  GLSimpleMoleculeView::setParameters(glMoleculeParameters);

  ///// CrdView settings //////////////
  prefix = unixPrefix + "crdview/";

  ///// geometry
  int x = settings.readNumEntry(prefix + "geometry/x", -1);
  int y = settings.readNumEntry(prefix + "geometry/y", -1);
  int width = settings.readNumEntry(prefix + "geometry/width", -1);
  int height = settings.readNumEntry(prefix + "geometry/height", -1);

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
  resize(width, height);
  move(x, y);

  ///// toolbars
  QString toolbarsInfo = settings.readEntry(prefix + "toolbars/other", QString::null);
  if(!toolbarsInfo.isEmpty())
  {
    QTextStream stream(&toolbarsInfo, IO_ReadOnly);
    stream >> *this;
  }
  if(settings.readBoolEntry(prefix + "toolbars/statusbar", true))
    statusBar()->show();
  else
    statusBar()->hide();
  actionViewToolBar->setOn(ToolBarFile->isVisible());
  actionViewStatusBar->setOn(statusBar()->isVisible());
}

///// saveSettings ////////////////////////////////////////////////////////////
void CrdView::saveSettings()
/// Saves the CrdView view settings to the Brabosphere settings file.
{
  QSettings settings;
  settings.setPath(Version::appCompany, Version::appName.lower(), QSettings::User);

#ifdef Q_OS_WIN32
  const QString prefix = "/crdview/";
#else
  const QString prefix = "/" + Version::appName.lower() + "/crdview/";
#endif

  settings.writeEntry(prefix + "geometry/x", pos().x());
  settings.writeEntry(prefix + "geometry/y", pos().y());
  settings.writeEntry(prefix + "geometry/width", width());
  settings.writeEntry(prefix + "geometry/height", height());
  QString toolbarsInfo;
  QTextStream stream(&toolbarsInfo, IO_WriteOnly);
  stream << *this;
  settings.writeEntry(prefix + "toolbars/other", toolbarsInfo);
  settings.writeEntry(prefix + "toolbars/statusbar", statusBar()->isVisible());
}

///// actionText //////////////////////////////////////////////////////////////
QString CrdView::actionText(const QString title, const QString brief, const QString details, const IconSets::IconSetID iconID)
/// Constructs a What's This text for actions with proper richtext formatting.
/// \arg title: The title of the action
/// \arg brief: A brief one line description
/// \arg details: A longer detailed description (optional)
/// \arg iconID: translates to the name of a pixmap in the default mimesource factory (optional) 
/// This routine is identical to the one used in the Xbrabo class. Refactoring needed.
{
  QString result = "<big><b>" + title + "</b></big> <hr>"; // keep the space before <hr> until this bug is solved (never for Qt pre 3.3.6)
  if(iconID != IconSets::LastIcon)
    result += "<img source=\"" + IconSets::factoryName(iconID) + "\" align=\"right\"/>";
  result += "<br>" + brief;
  if(!details.isEmpty())
    result += "<br>" + details;
  return result;
}

