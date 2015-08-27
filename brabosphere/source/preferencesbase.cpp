/***************************************************************************
                      preferencesbase.cpp  -  description
                             -------------------
    begin                : Sat Aug 10 2002
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
  \class PreferencesBase
  \brief This class allows setup of the preferences.

  It is instantiation during startup of the program and then reads the settings 
  from an ini file or the registry, providing defaults if no settings can be found.

*/
/// \file
/// Contains the implementation of the class PreferencesBase.

///// Header files ////////////////////////////////////////////////////////////

// C++ header files
#include <cmath>

// Qt header files
#include <qapplication.h>
#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qcombobox.h>
#include <qfiledialog.h>
#include <qgl.h>
#include <qiconview.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qlistview.h>
#include <qmessagebox.h>
#include <qpoint.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qsettings.h>
#include <qsize.h>
#include <qslider.h>
#include <qspinbox.h>
#include <qstring.h>
#include <qstyle.h>
#include <qstylefactory.h>
#include <qvalidator.h>
#include <qwidgetstack.h>

// QextMDI header files
#if defined(USE_KMDI) || defined(USE_KMDI_DLL)
#  include<kmdidefines.h>
#  include <kmdimainfrm.h>
#  define QextMdiMainFrm KMdiMainFrm
#  define QextMdi KMdi
#else
#  include <qextmdidefines.h>
#  include <qextmdimainfrm.h>
#endif

// Xbrabo header files
#include "atomset.h"
#include "basisset.h"
#include "colorbutton.h"
#include "iconsets.h"
#include "latin1validator.h"
#include "paths.h"
#include "preferencesbase.h"
#include "version.h"


///////////////////////////////////////////////////////////////////////////////
///// Public Member Functions                                             /////
///////////////////////////////////////////////////////////////////////////////

///// constructor /////////////////////////////////////////////////////////////
PreferencesBase::PreferencesBase(QWidget* parent, const char* name, bool modal, WFlags fl) : PreferencesWidget(parent,name, modal, fl)
/// The default constructor.
{      
  mainWindow = dynamic_cast<QextMdiMainFrm*>(parent); // save the pointer to Xbrabo (main window)
  makeConnections();
  init();
  // STOCK on Windows chokes when a path name is used for the F11= keyword (compiler related)
#ifdef WIN32
  RadioButtonBin1->setChecked(true);
  RadioButtonBin2->setDisabled(true);
  LineEditBin->setDisabled(true);
  ToolButtonBin->setDisabled(true);
#endif
}

///// destructor //////////////////////////////////////////////////////////////
PreferencesBase::~PreferencesBase()
/// The default destructor.
{
  saveSettings(); // needed for mainwindow geometry
}

///// preferredBasisset ///////////////////////////////////////////////////////
unsigned int PreferencesBase::preferredBasisset() const
/// Returns the index of the basisset used as the preferred basisset.
{  
  return ComboBoxBasis->currentItem();
}

///// useBinDirectory /////////////////////////////////////////////////////////
bool PreferencesBase::useBinDirectory() const
/// Returns true if the .11 should be written to
/// a special directory (different from the calculation directory).
{  
  return RadioButtonBin2->isChecked();
}

///// getGLBaseParameters /////////////////////////////////////////////////////
GLBaseParameters PreferencesBase::getGLBaseParameters() const
/// Returns a struct containing all OpenGL parameters used in GLView.
{  
  GLBaseParameters result;
  switch(data.lightPosition)
  {
    case 0: result.lightPositionX = -1.0;
            result.lightPositionY =  1.0;
            break;
    case 1: result.lightPositionX =  0.0;
            result.lightPositionY =  1.0;
            break;
    case 2: result.lightPositionX =  1.0;
            result.lightPositionY =  1.0;
            break;
    case 3: result.lightPositionX = -1.0;
            result.lightPositionY =  0.0;
            break;
    case 4: result.lightPositionX =  0.0;
            result.lightPositionY =  0.0;
            break;
    case 5: result.lightPositionX =  1.0;
            result.lightPositionY =  0.0;
            break;
    case 6: result.lightPositionX = -1.0;
            result.lightPositionY = -1.0;
            break;
    case 7: result.lightPositionX =  0.0;
            result.lightPositionY = -1.0;
            break;
    case 8: result.lightPositionX =  1.0;
            result.lightPositionY = -1.0;
            break;            
  }
  result.lightPositionZ = 1.0;
  result.lightColor = data.lightColor;
  result.materialSpecular = data.materialSpecular;
  result.materialShininess = data.materialShininess;
  result.backgroundColor = data.colorBackgroundGL;
  result.antialias = data.antialias;
  result.smoothShading = data.smoothShading;
  result.depthCue = data.depthCue;
  result.perspectiveProjection = data.perspectiveProjection;
    
  return result;
}

///// getGLMoleculeParameters /////////////////////////////////////////////////
GLMoleculeParameters PreferencesBase::getGLMoleculeParameters() const
/// Returns a struct containing all OpenGL parameters used in GLSimpleMoleculeView.
{  
  GLMoleculeParameters result;

  if(data.quality < 11)
    result.quality = data.quality;
  else
    result.quality = static_cast<int>(10.0 * pow(1.1, static_cast<int>(data.quality) - 10));
  result.sizeLines = static_cast<GLfloat>(data.sizeLines)*lineWidthGranularity;
  result.sizeBonds = data.sizeBonds.toFloat()/2.0; // diameter -> radius
  result.sizeForces = data.sizeForces.toFloat()/2.0; // diameter -> radius
  result.defaultMoleculeStyle = data.styleMolecule;
  result.defaultForcesStyle = data.styleForces;
  result.fastRenderLimit = data.fastRenderLimit;
  result.showElements = data.showElements;
  result.showNumbers = data.showNumbers;
  result.colorLabels = data.colorLabels;
  result.colorICs = data.colorICs;
  result.colorSelections = data.colorSelections;
  result.opacitySelections = data.opacitySelections;
  result.colorForces = data.colorForces;
  result.forcesOneColor = data.forcesOneColor;
  result.opacityForces = data.opacityForces;

  return result;
}

///// getPVMHosts /////////////////////////////////////////////////////////////
QStringList PreferencesBase::getPVMHosts() const
/// Returns the list of available PVM hosts.
{  
  return data.pvmHosts;
}

///// setToolbarsInfo /////////////////////////////////////////////////////////
void PreferencesBase::setToolbarsInfo(const QString& info, const bool status)
/// Sets the info needed to restore the toolbars.
{
  toolbarsInfo = info;
  toolbarsStatus = status;
}

///// getToolbarsInfo /////////////////////////////////////////////////////////
void PreferencesBase::getToolbarsInfo(bool& status, QString& info) const
/// Returns the info needed to restore the toolbars.
{
  info = toolbarsInfo;
  status = toolbarsStatus;
}


///////////////////////////////////////////////////////////////////////////////
///// Public Slots                                                        /////
///////////////////////////////////////////////////////////////////////////////

///// loadSettings ////////////////////////////////////////////////////////////
void PreferencesBase::loadSettings()
/// Loads the program settings from file. On linux, this file
/// is $HOME/.qt/brabosphererc while on Windows they are read from the registry. 
/// It also sets up the geometry of the widgets.
{
#ifdef Q_OS_WIN32
  const QString appDirBrabo  = "C:\\Program Files\\" + Version::appCompany + "\\BRABO";
  const QString defaultExtension = ".exe";
  const QString basisDir = "C:\\Basissets";
  const QString binDir = "C:\\Temp";
  const QString unixPrefix = "/";
#else
  const QString appDirBrabo  = "/usr/local/bin/brabo";
  const QString defaultExtension = ".x";
  const QString basisDir = "/usr/local/share/" + Version::appName.lower() + "/basissets";
  const QString binDir = "/tmp";
  const QString unixPrefix = "/" + Version::appName.lower() + "/";
#endif
      
  QSettings settings;
  settings.setPath(Version::appCompany, Version::appName.lower(), QSettings::User);

  QString prefix = unixPrefix + "preferences/";

  ///// Preferences entries ///////////
  unsigned int settingsVersion = settings.readNumEntry(unixPrefix + "version", 0);
  
  ///// Paths
  data.path              = settings.readEntry(prefix + "path", appDirBrabo + QDir::separator());
  data.extension         = settings.readEntry(prefix + "extension", defaultExtension);
  data.executables = settings.readListEntry(prefix + "executables");
  if(static_cast<int>(data.executables.size()) != ListViewExecutables->childCount())
  {
    ///// supply default values for data.executables
    LineEditPath->setText(data.path);
    LineEditExtension->setText(data.extension);
    updateAllExecutables(); // fills ListViewExecutables with values generated from data.path and data.extension
    saveWidgets(); // saves these values to data.executables
  }                         
  data.binDir            = settings.readEntry(prefix + "bin_dir", binDir);
  data.binInCalcDir      = settings.readBoolEntry(prefix + "bin_in_calc_dir", true);
  data.basissetDir       = settings.readEntry(prefix + "basisset_dir", basisDir);
  data.basisset          = settings.readNumEntry(prefix + "basisset", Basisset::basisToNum("6-31G"));
  ///// Molecule
  data.styleMolecule     = settings.readNumEntry(prefix + "style_molecule", 3); // Ball & Stick
  data.styleForces       = settings.readNumEntry(prefix + "style_forces", 2); // Tubes
  data.fastRenderLimit   = settings.readNumEntry(prefix + "fast_render_limit", 1000);
  data.showElements      = settings.readBoolEntry(prefix + "show_elements", false);
  data.showNumbers       = settings.readBoolEntry(prefix + "show_numbers", true);
  data.sizeLines         = settings.readNumEntry(prefix + "size_lines", static_cast<int>((minLineWidthGL > 1.0f ? minLineWidthGL : 1.0f)/lineWidthGranularity)); // max(1.0, minLineWidthGL) 
  data.sizeBonds         = settings.readEntry(prefix + "size_bonds", QString::number(AtomSet::vanderWaals(1)/2.0));
  data.sizeForces        = settings.readEntry(prefix + "size_forces", QString::number(AtomSet::vanderWaals(1)/2.0*1.1));
  data.colorLabels       = settings.readNumEntry(prefix + "color_labels", QColor(0, 255, 0).rgb()); // green
  data.colorICs          = settings.readNumEntry(prefix + "color_internal_coordinates", QColor(0, 255, 255).rgb()); // cyan
  data.colorBackgroundGL = settings.readNumEntry(prefix + "color_background_gl", 0); // black
  data.colorSelections   = settings.readNumEntry(prefix + "color_selections", QColor(255, 255, 0).rgb()); // yellow
  data.opacitySelections = settings.readNumEntry(prefix + "opacity_selections", 50);
  data.colorForces       = settings.readNumEntry(prefix + "color_forces", QColor(255, 255, 0).rgb()); //yellow
  data.forcesOneColor    = settings.readBoolEntry(prefix + "color_force_type", false); // atom color
  data.opacityForces     = settings.readNumEntry(prefix + "opacity_forces", 100);

  ///// Visuals
  data.backgroundType    = settings.readNumEntry(prefix + "background_type", 0); // default
  data.backgroundImage   = settings.readEntry(prefix + "background_image", QString::null);
  data.backgroundColor   = settings.readNumEntry(prefix + "background_color", QColor(224, 224, 224).rgb()); // Light grey
  data.styleApplication  = settings.readNumEntry(prefix + "style", 0); // Startup style
  
  ///// OpenGL
  data.lightPosition     = settings.readNumEntry(prefix + "light_position", 2);
  data.lightColor        = settings.readNumEntry(prefix + "light_color", QColor(255,255,255).rgb());
  data.materialSpecular  = settings.readNumEntry(prefix + "material_specular", 80);
  data.materialShininess = settings.readNumEntry(prefix + "material_shininess", 100);
  data.antialias         = settings.readBoolEntry(prefix + "antialias", true);
  data.smoothShading     = settings.readBoolEntry(prefix + "smooth_shading", true);
  data.depthCue          = settings.readBoolEntry(prefix + "depth_cue", true);
  data.quality           = settings.readNumEntry(prefix + "quality", 22); // 31 slices
  data.perspectiveProjection = settings.readBoolEntry(prefix + "perspective_projection", true);
  ///// PVM
  data.pvmHosts          = settings.readListEntry(prefix + "pvm_hosts");

  restoreWidgets();
  widgetChanged = false;

  ///// Xbrabo entries not read in Xbrabo::loadGeometry()
  ///// toolbars
  toolbarsInfo = settings.readEntry(unixPrefix + "toolbars/other", QString::null);
  toolbarsStatus = settings.readBoolEntry(unixPrefix + "toolbars/statusbar", true);

  ///// save the settings if this is a first run
  if(settingsVersion == 0)
    saveSettings();
}

///// saveSettings ////////////////////////////////////////////////////////////
void PreferencesBase::saveSettings()
/// Saves the program settings to file. ATM only called from
/// the destructor, but possibly useful as a public slot.  
/// The following stuff is or should be saved:
/// \arg all PreferencesBase widgets
/// \arg mainwindow geometry/mdimode/recent file list/
/// \arg toolbar geometry/visibility/
/// \arg statusbar geometry/visibility/
/// \arg taskbar geometry/visibility/
{
  QSettings settings;
  settings.setPath(Version::appCompany, Version::appName.lower(), QSettings::User);

#ifdef Q_OS_WIN32
  QString unixPrefix = "/";
#else
  QString unixPrefix = "/" + Version::appName.lower() + "/";
#endif

  ///// Version
  settings.writeEntry(unixPrefix + "version", static_cast<int>(SettingsVersion));
  ///// Preferences entries ///////////
  QString prefix = unixPrefix + "preferences/";
  ///// Paths
  settings.writeEntry(prefix + "path", data.path);
  settings.writeEntry(prefix + "extension", data.extension);
  settings.writeEntry(prefix + "executables", data.executables);
  settings.writeEntry(prefix + "bin_in_calc_dir", data.binInCalcDir);
  settings.writeEntry(prefix + "bin_dir", data.binDir);
  settings.writeEntry(prefix + "basisset_dir", data.basissetDir);
  settings.writeEntry(prefix + "basisset", static_cast<int>(data.basisset));
  ///// Molecule
  settings.writeEntry(prefix + "style_molecule", static_cast<int>(data.styleMolecule)); 
  settings.writeEntry(prefix + "style_forces", static_cast<int>(data.styleForces)); 
  settings.writeEntry(prefix + "fast_render_limit", data.fastRenderLimit);
  settings.writeEntry(prefix + "show_elements", data.showElements);
  settings.writeEntry(prefix + "show_numbers", data.showNumbers);
  settings.writeEntry(prefix + "size_lines", data.sizeLines);
  settings.writeEntry(prefix + "size_bonds", data.sizeBonds);
  settings.writeEntry(prefix + "size_forces", data.sizeForces);
  settings.writeEntry(prefix + "color_labels", static_cast<int>(data.colorLabels));
  settings.writeEntry(prefix + "color_internal_coordinates", static_cast<int>(data.colorICs));
  settings.writeEntry(prefix + "color_background_gl", static_cast<int>(data.colorBackgroundGL));
  settings.writeEntry(prefix + "color_selections", static_cast<int>(data.colorSelections));
  settings.writeEntry(prefix + "opacity_selections", static_cast<int>(data.opacitySelections));
  settings.writeEntry(prefix + "color_forces", static_cast<int>(data.colorForces));
  settings.writeEntry(prefix + "color_force_type", data.forcesOneColor);
  settings.writeEntry(prefix + "opacity_forces", static_cast<int>(data.opacityForces));
  ///// Visuals
  settings.writeEntry(prefix + "background_type", static_cast<int>(data.backgroundType));
  settings.writeEntry(prefix + "background_image", data.backgroundImage);
  settings.writeEntry(prefix + "background_color", static_cast<int>(data.backgroundColor));
  settings.writeEntry(prefix + "style", static_cast<int>(data.styleApplication));
  ///// OpenGL
  settings.writeEntry(prefix + "light_position", static_cast<int>(data.lightPosition));
  settings.writeEntry(prefix + "light_color", static_cast<int>(data.lightColor));
  settings.writeEntry(prefix + "material_specular", static_cast<int>(data.materialSpecular));
  settings.writeEntry(prefix + "material_shininess", static_cast<int>(data.materialShininess));
  settings.writeEntry(prefix + "antialias", data.antialias);
  settings.writeEntry(prefix + "smooth_shading", data.smoothShading);
  settings.writeEntry(prefix + "depth_cue", data.depthCue);
  settings.writeEntry(prefix + "quality", static_cast<int>(data.quality));
  settings.writeEntry(prefix + "perspective_projection", data.perspectiveProjection); 
  ///// PVM
  settings.writeEntry(prefix + "pvm_hosts", data.pvmHosts);
    
  ///// Xbrabo entries ////////////////
  prefix = unixPrefix + "geometry/";
  settings.writeEntry(prefix + "x", mainWindow->pos().x());
  settings.writeEntry(prefix + "y", mainWindow->pos().y());
  settings.writeEntry(prefix + "width", mainWindow->size().width());
  settings.writeEntry(prefix + "height", mainWindow->size().height());
  settings.writeEntry(prefix + "maximized", mainWindow->isMaximized());
  settings.writeEntry(unixPrefix + "mdi_mode", mainWindow->mdiMode());
  settings.writeEntry(unixPrefix + "toolbars/other", toolbarsInfo);
  settings.writeEntry(unixPrefix + "toolbars/statusbar", toolbarsStatus);
  //settings.writeEntry("/recent", QStringList recentFiles);
}

///// updateVisuals ///////////////////////////////////////////////////////////
void PreferencesBase::updateVisuals()
/// Updates the background image & color of the mainwindow and sets the style.
{ 
  switch(data.backgroundType)
  {
    case 0: // default
            mainWindow->setBackgroundPixmap(IconSets::getPixmap(IconSets::Background));
            break;
    case 1: // image
            {
              QPixmap pm(data.backgroundImage);
              if(!pm.isNull())
                mainWindow->setBackgroundPixmap(pm);
              else
                mainWindow->setBackgroundPixmap(IconSets::getPixmap(IconSets::Background));
            }
            break;
    case 2: // single color
            mainWindow->setBackgroundColor(data.backgroundColor);
  }

  ///// style      
  updateStyle();
}


///////////////////////////////////////////////////////////////////////////////
///// Protected Slots                                                     /////
///////////////////////////////////////////////////////////////////////////////

///// accept //////////////////////////////////////////////////////////////////
void PreferencesBase::accept()
/// Overridden version of PreferencesWidget::accept().
/// Sets the accept/reject status of the widget depending on whether
/// any actual changes have been made.
{  
  if(widgetChanged)
  {
    widgetChanged = false;
    saveWidgets();
    saveSettings(); // so CrdView's can be updated without restarting Brabosphere
    updatePaths();
    // Check whether the pvm host list has changed
    if(pvmHostsChanged == true)
    {
      pvmHostsChanged = false;
      emit newPVMHosts(data.pvmHosts);
    }
    PreferencesWidget::accept();
  }
  else
    PreferencesWidget::reject();
}

///// reject //////////////////////////////////////////////////////////////////
void PreferencesBase::reject()
/// Overridden version of PreferencesWidget::reject().
/// Rejects any possible changes made.
{  
  if(widgetChanged)
  {
    widgetChanged = false;
    restoreWidgets();
  }
  PreferencesWidget::reject();
}


///////////////////////////////////////////////////////////////////////////////
///// Private Slots                                                       /////
///////////////////////////////////////////////////////////////////////////////

///// changed /////////////////////////////////////////////////////////////////
void PreferencesBase::changed()
/// Sets the 'changed' status of PreferencesBase.
{  
  widgetChanged = true;
}

///// selectWidget ////////////////////////////////////////////////////////////
void PreferencesBase::selectWidget(QIconViewItem* item)
/// Shows a widget from WidgetStackCategory depending on the iconview item.
{  
  if(item->text() == tr("BRABO"))
    WidgetStackCategory->raiseWidget(0);
  else if(item->text() == tr("Visuals"))
    WidgetStackCategory->raiseWidget(1);
  else if(item->text() == tr("OpenGL"))
    WidgetStackCategory->raiseWidget(2);
  else if(item->text() == tr("PVM"))
    WidgetStackCategory->raiseWidget(3);
  else if(item->text() == tr("Molecule"))
    WidgetStackCategory->raiseWidget(4);
}

///// changeExecutable ////////////////////////////////////////////////////////
void PreferencesBase::changeExecutable()
/// Updates LabelProgram & LineEditExecutable with the
/// currently selected item in ListViewExecutables.
{  
  QListViewItem* item = ListViewExecutables->selectedItem();
  if(item == 0)
    return;

  LabelProgram->setText(item->text(0));
  LineEditExecutable->blockSignals(true); // setting the text doesn't change the listview
  LineEditExecutable->setText(item->text(1));
  LineEditExecutable->blockSignals(false);
}

///// updateExecutable ////////////////////////////////////////////////////////
void PreferencesBase::updateExecutable(const QString& text)
/// Updates the current item of ListViewExecutables with \c text.
{  
  QListViewItem* item = ListViewExecutables->selectedItem();
  if(item == 0)
    return;

  item->setText(1, text);
}

///// updateAllExecutables ////////////////////////////////////////////////////
void PreferencesBase::updateAllExecutables()
/// Renames all items of ListViewExecutables according to the
/// pattern path+name+extension.
{  
  ListViewExecutables->findItem("ABC",0)->setText(1,LineEditPath->text()+"abc"+LineEditExtension->text());
  ListViewExecutables->findItem("Achar",0)->setText(1,LineEditPath->text()+"achar"+LineEditExtension->text());
  ListViewExecutables->findItem("AtomSCF",0)->setText(1,LineEditPath->text()+"atomscf"+LineEditExtension->text());
  ListViewExecutables->findItem("AtomSCF_inp",0)->setText(1,LineEditPath->text()+"atomscf_inp"+LineEditExtension->text());
  ListViewExecutables->findItem("AtomSCF_o2d",0)->setText(1,LineEditPath->text()+"atomscf_o2d"+LineEditExtension->text());
  ListViewExecutables->findItem("B112gfchk",0)->setText(1,LineEditPath->text()+"b112gfchk"+LineEditExtension->text());
  ListViewExecutables->findItem("Bios2ped",0)->setText(1,LineEditPath->text()+"bios2ped"+LineEditExtension->text());
  ListViewExecutables->findItem("Brabo",0)->setText(1,LineEditPath->text()+"brabo"+LineEditExtension->text());
  ListViewExecutables->findItem("Buur",0)->setText(1,LineEditPath->text()+"buur"+LineEditExtension->text());
  ListViewExecutables->findItem("Cnvrtaff",0)->setText(1,LineEditPath->text()+"cnvrtaff"+LineEditExtension->text());
  ListViewExecutables->findItem("Crd2gauss",0)->setText(1,LineEditPath->text()+"crd2gauss"+LineEditExtension->text());
  ListViewExecutables->findItem("Crd2xyz",0)->setText(1,LineEditPath->text()+"crd2xyz"+LineEditExtension->text());
  ListViewExecutables->findItem("Diffcrd",0)->setText(1,LineEditPath->text()+"diffcrd"+LineEditExtension->text());
  ListViewExecutables->findItem("Distor",0)->setText(1,LineEditPath->text()+"distor"+LineEditExtension->text());
  ListViewExecutables->findItem("Forkon",0)->setText(1,LineEditPath->text()+"forkon"+LineEditExtension->text());
  ListViewExecutables->findItem("Frex",0)->setText(1,LineEditPath->text()+"frex"+LineEditExtension->text());
  ListViewExecutables->findItem("Gar2ped",0)->setText(1,LineEditPath->text()+"gar2ped"+LineEditExtension->text());
  ListViewExecutables->findItem("Geom",0)->setText(1,LineEditPath->text()+"geom"+LineEditExtension->text());
  ListViewExecutables->findItem("Gfchk2b11",0)->setText(1,LineEditPath->text()+"gfchk2b11"+LineEditExtension->text());
  ListViewExecutables->findItem("Gfchk2crd",0)->setText(1,LineEditPath->text()+"gfchk2crd"+LineEditExtension->text());
  ListViewExecutables->findItem("Gfchk2pun",0)->setText(1,LineEditPath->text()+"gfchk2pun"+LineEditExtension->text());
  ListViewExecutables->findItem("HKL",0)->setText(1,LineEditPath->text()+"hkl"+LineEditExtension->text());
  ListViewExecutables->findItem("Log2crd",0)->setText(1,LineEditPath->text()+"log2crd"+LineEditExtension->text());
  ListViewExecutables->findItem("Maff",0)->setText(1,LineEditPath->text()+"maff"+LineEditExtension->text());
  ListViewExecutables->findItem("Makeden",0)->setText(1,LineEditPath->text()+"makeden"+LineEditExtension->text());
  ListViewExecutables->findItem("Makexit",0)->setText(1,LineEditPath->text()+"makexit"+LineEditExtension->text());
  ListViewExecutables->findItem("Molsplit",0)->setText(1,LineEditPath->text()+"molsplit"+LineEditExtension->text());
  ListViewExecutables->findItem("Out2aff",0)->setText(1,LineEditPath->text()+"out2aff"+LineEditExtension->text());
  ListViewExecutables->findItem("Potdicht",0)->setText(1,LineEditPath->text()+"potdicht"+LineEditExtension->text());
  ListViewExecutables->findItem("Pullarc",0)->setText(1,LineEditPath->text()+"pullarc"+LineEditExtension->text());
  ListViewExecutables->findItem("Refine",0)->setText(1,LineEditPath->text()+"refine"+LineEditExtension->text());
  ListViewExecutables->findItem("Relax",0)->setText(1,LineEditPath->text()+"relax"+LineEditExtension->text());
  ListViewExecutables->findItem("Ring",0)->setText(1,LineEditPath->text()+"ring"+LineEditExtension->text());
  ListViewExecutables->findItem("Spfmap",0)->setText(1,LineEditPath->text()+"spfmap"+LineEditExtension->text());
  ListViewExecutables->findItem("Startvec",0)->setText(1,LineEditPath->text()+"startvec"+LineEditExtension->text());
  ListViewExecutables->findItem("Stock",0)->setText(1,LineEditPath->text()+"stock"+LineEditExtension->text());
  ListViewExecutables->findItem("Symm",0)->setText(1,LineEditPath->text()+"symm"+LineEditExtension->text());
  ListViewExecutables->findItem("Table",0)->setText(1,LineEditPath->text()+"table"+LineEditExtension->text());
  ListViewExecutables->findItem("Xyz2crd",0)->setText(1,LineEditPath->text()+"xyz2crd"+LineEditExtension->text());
  
  changeExecutable(); //update LabelProgram & LineEditExecutable
}

///// selectBinDir ////////////////////////////////////////////////////////////
void PreferencesBase::selectBinDir()
/// Selects a new directory for LineEditBin using a standard filedialog.
{  
  QString dirname = QFileDialog::getExistingDirectory(LineEditBin->text(),this, 0, tr("Choose a directory"));
  if(!dirname.isNull())
    LineEditBin->setText(QDir::convertSeparators(dirname));
}

///// selectExecutable ////////////////////////////////////////////////////////
void PreferencesBase::selectExecutable()
/// Selects an executable for LineEditExecutable using a standard filedialog.
{  
  QString filename = QFileDialog::getOpenFileName(LineEditExecutable->text(), QString::null, this, 0, tr("Choose an executable"));
  if(!filename.isNull())
    LineEditExecutable->setText(QDir::convertSeparators(filename));
}

///// selectBasisDir //////////////////////////////////////////////////////////
void PreferencesBase::selectBasisDir()
/// Selects a new directory for LineEditBasis using a standard filedialog.
{  
  QString dirname = QFileDialog::getExistingDirectory(LineEditBasis->text(),this, 0, tr("Choose a directory"));
  if(!dirname.isNull())
    LineEditBasis->setText(QDir::convertSeparators(dirname));
}

///// selectBackground ////////////////////////////////////////////////////////
void PreferencesBase::selectBackground()
/// Selects a background image for LineEditBackground using a standard filedialog.
{
  QString filename = QFileDialog::getOpenFileName(LineEditBackground->text(), QString::null, this, 0, tr("Choose an image"));
  if(!filename.isNull())
    LineEditBackground->setText(filename);
}

///// updateLineEditBondSizeLines /////////////////////////////////////////////
void PreferencesBase::updateLineEditBondSizeLines()
/// Updates LineEditBondSizeLines according to SliderBondSizeLines.
{
  LineEditBondSizeLines->setText(QString::number(SliderBondSizeLines->value()*lineWidthGranularity));
}

///// updateLineEditBondSizeTubes /////////////////////////////////////////////
void PreferencesBase::updateLineEditBondSizeTubes()
/// Updates LineEditBondSizeTubes according to SliderBondSizeTubes.
{  
  LineEditBondSizeTubes->blockSignals(true);
  LineEditBondSizeTubes->setText(QString::number(SliderBondSizeTubes->value()*0.01));
  LineEditBondSizeTubes->blockSignals(false);
}

///// updateSliderBondSizeTubes ///////////////////////////////////////////////
void PreferencesBase::updateSliderBondSizeTubes()
/// Updates SliderBondSizeTubes according to LineEditBondSizeTubes.
{  
  SliderBondSizeTubes->blockSignals(true);
  SliderBondSizeTubes->setValue(static_cast<int>(LineEditBondSizeTubes->text().toFloat()/0.01));
  SliderBondSizeTubes->blockSignals(false);
}

///// updateLineEditForceSizeTubes ////////////////////////////////////////////
void PreferencesBase::updateLineEditForceSizeTubes()
/// Updates LineEditForceSizeTubes according to SliderForceSizeTubes.
{  
  LineEditForceSizeTubes->blockSignals(true);
  LineEditForceSizeTubes->setText(QString::number(SliderForceSizeTubes->value()*0.01));
  LineEditForceSizeTubes->blockSignals(false);
}

///// updateSliderForceSizeTubes //////////////////////////////////////////////
void PreferencesBase::updateSliderForceSizeTubes()
/// Updates SliderForceSizeTubes according to LineEditForceSizeTubes.
{  
  SliderForceSizeTubes->blockSignals(true);
  SliderForceSizeTubes->setValue(static_cast<int>(LineEditForceSizeTubes->text().toFloat()/0.01));
  SliderForceSizeTubes->blockSignals(false);
}

///// updateOpacitySelection //////////////////////////////////////////////////
void PreferencesBase::updateOpacitySelection()
/// Updates TextLabelSelection according to SliderSelectionOpacity.
{  
  if(SliderSelectionOpacity->value() == 100)
    TextLabelSelection->setText(QString::number(SliderSelectionOpacity->value()) + " %");
  else
    TextLabelSelection->setText(" " + QString::number(SliderSelectionOpacity->value()) + " %");
}

///// updateOpacityForces /////////////////////////////////////////////////////
void PreferencesBase::updateOpacityForces()
/// Updates TextLabelForces according to SliderForcesOpacity.
{  
  if(SliderForceOpacity->value() == 100)
    TextLabelForce->setText(QString::number(SliderForceOpacity->value()) + " %");
  else
    TextLabelForce->setText(" " + QString::number(SliderForceOpacity->value()) + " %");
}

///// updateColorButtonForce //////////////////////////////////////////////////
void PreferencesBase::updateColorButtonForce()
/// Enables/disables ColorButtonForce according to the selected colring type.
{  
  ColorButtonForce->setEnabled(ComboBoxForceColor->currentItem() == 1);
}

///// changePVMHost ///////////////////////////////////////////////////////////
void PreferencesBase::changePVMHost()
/// Updates LineEditPVMHost with the selected item in ListViewPVMHosts.
{  
  LineEditPVMHosts->blockSignals(true); // so setting the text doesn't change the listview
  if(ListViewPVMHosts->selectedItem() != 0)
  {
    LineEditPVMHosts->setEnabled(true);
    PushButtonPVMHostsDelete->setEnabled(true);
    LineEditPVMHosts->setText(ListViewPVMHosts->selectedItem()->text(0));
  }
  else
  {
    LineEditPVMHosts->setEnabled(false);
    PushButtonPVMHostsDelete->setEnabled(false);
    LineEditPVMHosts->setText(QString::null);
  }
  LineEditPVMHosts->blockSignals(false);
}

///// updatePVMHost ///////////////////////////////////////////////////////////
void PreferencesBase::updatePVMHost(const QString& text)
/// Updates the current item of ListViewPVMHosts with text.
{  
  if(ListViewPVMHosts->selectedItem() != 0)
  {
    ListViewPVMHosts->selectedItem()->setText(0, text);
    ListViewPVMHosts->sort();
  }
}

///// newPVMHost //////////////////////////////////////////////////////////////
void PreferencesBase::newPVMHost()
/// Adds a new host to ListViewPVMHosts with the name 'new host'.
{  
  QListViewItem* item =  new QListViewItem(ListViewPVMHosts,tr("host.domain"));
  ListViewPVMHosts->setSelected(item, true);
}

///// deletePVMHost ///////////////////////////////////////////////////////////
void PreferencesBase::deletePVMHost()
/// Removes the currently selected host of ListViewPVMHosts.
{  
  if(ListViewPVMHosts->selectedItem() != 0)
  {
    delete ListViewPVMHosts->selectedItem();
    changed();
    changedPVM();
    ListViewPVMHosts->setSelected(ListViewPVMHosts->firstChild(), true);
    changePVMHost();
  }
}

///// changedPVM //////////////////////////////////////////////////////////////
void PreferencesBase::changedPVM()
/// Indicates that the PVM host list has changed.
{
  pvmHostsChanged = true;
}


///////////////////////////////////////////////////////////////////////////////
///// Private Member Functions                                            /////
///////////////////////////////////////////////////////////////////////////////

///// makeConnections /////////////////////////////////////////////////////////
void PreferencesBase::makeConnections()
/// Sets up all permanent coonections.
{  
  ///// Connections for IconView-WidgetStack /////
  connect(IconViewCategory, SIGNAL(selectionChanged(QIconViewItem*)), this, SLOT(selectWidget(QIconViewItem*)));
  
  ///// connections for buttons /////
  connect(ButtonOK, SIGNAL(clicked()), this, SLOT(accept()));
  connect(ButtonCancel, SIGNAL(clicked()), this, SLOT(reject()));
  
  ///// connections for other widgets /////
  ///// Paths
  connect(ListViewExecutables, SIGNAL(selectionChanged()), this, SLOT(changeExecutable()));
  connect(LineEditExecutable, SIGNAL(textChanged(const QString&)), this, SLOT(updateExecutable(const QString&)));
  connect(PushButtonRename, SIGNAL(clicked()), this, SLOT(updateAllExecutables()));
  connect(ToolButtonBin, SIGNAL(clicked()), this, SLOT(selectBinDir()));
  connect(ToolButtonExecutable, SIGNAL(clicked()), this, SLOT(selectExecutable()));
  connect(ToolButtonBasis, SIGNAL(clicked()), this, SLOT(selectBasisDir()));
  ///// Molecule
  connect(SliderBondSizeLines, SIGNAL(valueChanged(int)), this, SLOT(updateLineEditBondSizeLines()));
  connect(SliderBondSizeTubes, SIGNAL(valueChanged(int)), this, SLOT(updateLineEditBondSizeTubes()));
  connect(LineEditBondSizeTubes, SIGNAL(textChanged(const QString&)), this, SLOT(updateSliderBondSizeTubes()));
  connect(SliderForceSizeTubes, SIGNAL(valueChanged(int)), this, SLOT(updateLineEditForceSizeTubes()));
  connect(LineEditForceSizeTubes, SIGNAL(textChanged(const QString&)), this, SLOT(updateSliderForceSizeTubes()));
  connect(SliderSelectionOpacity, SIGNAL(valueChanged(int)), this, SLOT(updateOpacitySelection()));
  connect(SliderForceOpacity, SIGNAL(valueChanged(int)), this, SLOT(updateOpacityForces()));
  connect(ComboBoxForceColor, SIGNAL(activated(int)), this, SLOT(updateColorButtonForce()));
  ///// Visuals
  connect(ToolButtonBackground, SIGNAL(clicked()), this, SLOT(selectBackground()));
  ///// PVM
  connect(ListViewPVMHosts, SIGNAL(selectionChanged()), this, SLOT(changePVMHost()));
  connect(LineEditPVMHosts, SIGNAL(textChanged(const QString&)), this, SLOT(updatePVMHost(const QString&)));
  connect(PushButtonPVMHostsNew, SIGNAL(clicked()), this, SLOT(newPVMHost()));
  connect(PushButtonPVMHostsDelete, SIGNAL(clicked()), this, SLOT(deletePVMHost()));
  
  ///// connections for changes ///////
  ///// Paths
  connect(LineEditExecutable, SIGNAL(textChanged(const QString&)), this, SLOT(changed())); // no appropiate signal for ListViewExecutable
  connect(LineEditPath, SIGNAL(textChanged(const QString&)), this, SLOT(changed()));
  connect(LineEditExtension, SIGNAL(textChanged(const QString&)), this, SLOT(changed()));
  connect(PushButtonRename, SIGNAL(clicked()), this, SLOT(changed()));
  connect(ButtonGroupBin, SIGNAL(clicked(int)), this, SLOT(changed()));
  connect(LineEditBin, SIGNAL(textChanged(const QString&)), this, SLOT(changed()));
  connect(LineEditBasis, SIGNAL(textChanged(const QString&)), this, SLOT(changed()));
  connect(ComboBoxBasis, SIGNAL(activated(int)), this, SLOT(changed()));
  ///// Molecule
  connect(ComboBoxMolecule, SIGNAL(activated(int)), this, SLOT(changed()));
  connect(ComboBoxForces, SIGNAL(activated(int)), this, SLOT(changed()));
  connect(SpinBoxFastRender, SIGNAL(valueChanged(int)), this, SLOT(changed()));
  connect(CheckBoxElement, SIGNAL(clicked()), this, SLOT(changed()));
  connect(CheckBoxNumber, SIGNAL(clicked()), this, SLOT(changed()));
  connect(SliderBondSizeLines, SIGNAL(valueChanged(int)), this, SLOT(changed()));
  connect(SliderBondSizeTubes, SIGNAL(valueChanged(int)), this, SLOT(changed()));
  connect(LineEditBondSizeTubes, SIGNAL(textChanged(const QString&)), this, SLOT(changed()));
  connect(SliderForceSizeTubes, SIGNAL(valueChanged(int)), this, SLOT(changed()));
  connect(LineEditForceSizeTubes, SIGNAL(textChanged(const QString&)), this, SLOT(changed()));
  connect(ColorButtonLabel, SIGNAL(newColor(QColor*)), this, SLOT(changed()));
  connect(ColorButtonIC, SIGNAL(newColor(QColor*)), this, SLOT(changed()));
  connect(ColorButtonBackgroundGL, SIGNAL(newColor(QColor*)), this, SLOT(changed()));
  connect(ColorButtonSelection, SIGNAL(newColor(QColor*)), this, SLOT(changed()));
  connect(SliderSelectionOpacity, SIGNAL(valueChanged(int)), this, SLOT(changed()));
  connect(ColorButtonForce, SIGNAL(newColor(QColor*)), this, SLOT(changed()));
  connect(ComboBoxForceColor, SIGNAL(activated(int)), this, SLOT(changed()));
  connect(SliderForceOpacity, SIGNAL(valueChanged(int)), this, SLOT(changed()));
  ///// Visuals
  connect(ButtonGroupBackground, SIGNAL(clicked(int)), this, SLOT(changed()));
  connect(LineEditBackground, SIGNAL(textChanged(const QString&)), this, SLOT(changed()));
  connect(ColorButtonBackground, SIGNAL(newColor(QColor*)), this, SLOT(changed()));
  connect(ComboBoxStyle, SIGNAL(activated(int)), this, SLOT(changed()));
  ///// OpenGL
  connect(SliderQuality, SIGNAL(valueChanged(int)), this, SLOT(changed()));
  connect(CheckBoxAA, SIGNAL(clicked()), this, SLOT(changed()));
  connect(CheckBoxSmooth, SIGNAL(clicked()), this, SLOT(changed()));
  connect(CheckBoxDepthCue, SIGNAL(clicked()), this, SLOT(changed()));
  connect(ButtonGroupLightPosition, SIGNAL(clicked(int)), this, SLOT(changed()));
  connect(ColorButtonLight, SIGNAL(newColor(QColor*)), this, SLOT(changed()));
  connect(SliderSpecular, SIGNAL(valueChanged(int)), this, SLOT(changed()));
  connect(SliderShininess, SIGNAL(valueChanged(int)), this, SLOT(changed()));
  connect(ButtonGroupProjection, SIGNAL(clicked(int)), this, SLOT(changed()));
  ///// PVM
  connect(LineEditPVMHosts, SIGNAL(textChanged(const QString&)), this, SLOT(changed()));
  connect(LineEditPVMHosts, SIGNAL(textChanged(const QString&)), this, SLOT(changedPVM()));
  connect(PushButtonPVMHostsNew, SIGNAL(clicked()), this, SLOT(changed()));
  connect(PushButtonPVMHostsNew, SIGNAL(clicked()), this, SLOT(changedPVM()));
}

///// init ////////////////////////////////////////////////////////////////////
void PreferencesBase::init()
/// Initializes the widget. Called once from the constructor.
{  
  ///// select the first icon in the iconview
  IconViewCategory->setSelected(IconViewCategory->firstItem(), true);  
  ///// select the first item in the listview
  ListViewExecutables->setSelected(ListViewExecutables->firstChild(), true);
  ///// fill ComboBoxBasis with the available basissets
  ComboBoxBasis->clear();
  for(unsigned int i = 0; i < Basisset::maxBasissets(); i++)
    ComboBoxBasis->insertItem(Basisset::numToBasis(i));
  ///// set the preferredbasisset to  6-31G
  ComboBoxBasis->setCurrentItem(Basisset::basisToNum("6-31G"));
  ///// get the available styles
  QStringList styleList = QStyleFactory::keys();
  styleList.sort();
  ///// fill ComboBoxStyle with the available styles
  ComboBoxStyle->blockSignals(true);
  ComboBoxStyle->clear();
  ComboBoxStyle->insertItem(tr("Startup style") + " (" + QApplication::style().name() + ")");
  for(QStringList::iterator it = styleList.begin(); it != styleList.end(); it++)
    ComboBoxStyle->insertItem(*it);
  ComboBoxStyle->blockSignals(false);
  ///// save the startup style name
  startupStyleName = QApplication::style().name();

  ///// set the maximum values for the bond size sliders
  initOpenGL();
  SliderBondSizeLines->setMinValue(static_cast<int>(minLineWidthGL/lineWidthGranularity));
  SliderBondSizeLines->setMaxValue(static_cast<int>(maxLineWidthGL/lineWidthGranularity));
  SliderBondSizeTubes->setMaxValue(static_cast<int>(AtomSet::vanderWaals(1)/0.01)); // maxsize is H-diameter
  SliderForceSizeTubes->setMaxValue(static_cast<int>(AtomSet::vanderWaals(1)/0.01)); 
  
  ///// set a validator on the PVM hosts input
  QRegExp rx("[-a-zA-Z0-9\\.]+");
  LineEditPVMHosts->setValidator(new QRegExpValidator(rx, this));
  ///// set a validator on the temp and basisset directories
  Latin1Validator* v = new Latin1Validator(this);
  LineEditBin->setValidator(v);
  LineEditBasis->setValidator(v);
  v = 0;

  ///// load the settings
  loadSettings();
  updateColorButtonForce();

  ///// assign the icons
  for(QIconViewItem* item = IconViewCategory->firstItem(); item; item = item->nextItem())
  {
    if(item->text() == tr("BRABO"))
      item->setPixmap(IconSets::getPixmap(IconSets::BRABO));
    else if(item->text() == tr("Molecule"))
      item->setPixmap(IconSets::getPixmap(IconSets::Molecule));
    else if(item->text() == tr("Visuals"))
      item->setPixmap(IconSets::getPixmap(IconSets::Visuals));
    else if(item->text() == tr("OpenGL"))
      item->setPixmap(IconSets::getPixmap(IconSets::OpenGL));
    else if(item->text() == tr("PVM"))
      item->setPixmap(IconSets::getPixmap(IconSets::PVM));
  }
  ToolButtonExecutable->setIconSet(IconSets::getIconSet(IconSets::Open));
  ToolButtonBin->setIconSet(IconSets::getIconSet(IconSets::Open));
  ToolButtonBasis->setIconSet(IconSets::getIconSet(IconSets::Open));
  ToolButtonBackground->setIconSet(IconSets::getIconSet(IconSets::Open));

  resize(1,1); 
  changeExecutable();
  updatePaths();
  widgetChanged = false;
  pvmHostsChanged = false;
}

///// initOpenGL //////////////////////////////////////////////////////////////
void PreferencesBase::initOpenGL()
/// Determines the capabilities of the current OpenGL implementation.
/// ATM: minimum/maximum linewidth, linewidth granularity
/// in the future: stereo mode, etc.
/// Called once from the constructor.
{
  ///// create an OpenGL widget
  QGLWidget* testOpenGL = new QGLWidget();
  testOpenGL->makeCurrent();

  ///// Get the linewidth parameters
  GLfloat lwrange[] = {0.0, 0.0};
  GLfloat lwgran[] = {0.0};
  glGetFloatv(GL_LINE_WIDTH_RANGE, lwrange);
  glGetFloatv(GL_LINE_WIDTH_GRANULARITY, lwgran);
  minLineWidthGL = lwrange[0];
  maxLineWidthGL = lwrange[1];
  lineWidthGranularity = lwgran[0];

  /*
  qDebug("Renderer: %s",(const char*)glGetString(GL_RENDERER));
  qDebug("Vendor: %s",(const char*)glGetString(GL_VENDOR));
  qDebug("Version: %s",(const char*)glGetString(GL_VERSION));
  qDebug("Extensions: %s",(const char*)glGetString(GL_EXTENSIONS));
  //*/   

  ///// destroy the OpenGL widget
  delete testOpenGL;

}

///// saveWidgets /////////////////////////////////////////////////////////////
void PreferencesBase::saveWidgets()
/// Saves the status of the widgets to the struct data.
{
  ///// Paths
  data.executables.clear();
  QListViewItem* item = ListViewExecutables->firstChild();
  while(item != 0)
  {
    data.executables += item->text(1);
    item = item->nextSibling();
  }
  data.path = LineEditPath->text();
  data.extension = LineEditExtension->text();
  data.binInCalcDir = RadioButtonBin1->isChecked();
  data.binDir = LineEditBin->text();
  data.basissetDir = LineEditBasis->text();
  data.basisset = ComboBoxBasis->currentItem();
  
  ///// Molecule
  data.styleMolecule = ComboBoxMolecule->currentItem();
  data.styleForces = ComboBoxForces->currentItem();
  data.fastRenderLimit = SpinBoxFastRender->value();
  data.showElements = CheckBoxElement->isChecked();
  data.showNumbers = CheckBoxNumber->isChecked();
  data.sizeLines = SliderBondSizeLines->value();
  data.sizeBonds = LineEditBondSizeTubes->text();
  data.sizeForces = LineEditForceSizeTubes->text();
  data.colorLabels = ColorButtonLabel->color().rgb();
  data.colorICs = ColorButtonIC->color().rgb();
  data.colorBackgroundGL = ColorButtonBackgroundGL->color().rgb();
  data.colorSelections = ColorButtonSelection->color().rgb();
  data.colorForces = ColorButtonForce->color().rgb();
  data.opacitySelections = SliderSelectionOpacity->value();
  data.opacityForces = SliderForceOpacity->value();
  data.forcesOneColor = ComboBoxForceColor->currentItem() == 1;

  ///// Visuals
  data.backgroundType = ButtonGroupBackground->selectedId();
  data.backgroundImage = LineEditBackground->text();
  data.backgroundColor = ColorButtonBackground->color().rgb();
  data.styleApplication = ComboBoxStyle->currentItem();

  ///// OpenGL
  data.quality = SliderQuality->value();
  data.antialias = CheckBoxAA->isChecked();
  data.smoothShading = CheckBoxSmooth->isChecked();
  data.depthCue = CheckBoxDepthCue->isChecked();
  /*for(unsigned int i = 0; i < 9; i++)
  {
    if(ButtonGroupLightPosition->find(i)->isOn())
    {
      data.lightPosition = i;
      break;
    }
  }*/
  data.lightPosition = ButtonGroupLightPosition->selectedId();
  data.lightColor = ColorButtonLight->color().rgb();
  data.materialSpecular = SliderSpecular->value();
  data.materialShininess = SliderShininess->value();
  data.perspectiveProjection = RadioButtonPerspective->isChecked();

  ///// PVM
  data.pvmHosts.clear();
  item = ListViewPVMHosts->firstChild();
  while(item != 0)
  {
    data.pvmHosts += item->text(0);
    item = item->nextSibling();
  }

}

///// restoreWidgets //////////////////////////////////////////////////////////
void PreferencesBase::restoreWidgets()
/// Restores the status of the widgets from the contents of the struct data.
{  
  ///// Paths
  QListViewItem* item = ListViewExecutables->firstChild();
  {
    for(QStringList::Iterator it = data.executables.begin(); it != data.executables.end(); it++)
    {
      if(item == 0)
        break;  // more values than ListViewItems
      item->setText(1,*it);
      item = item->nextSibling();
    }  
  }
  LineEditPath->setText(data.path);
  LineEditExtension->setText(data.extension);
  RadioButtonBin1->setChecked(data.binInCalcDir);
  RadioButtonBin2->setChecked(!data.binInCalcDir);
  LineEditBin->setText(data.binDir);
  LineEditBasis->setText(data.basissetDir);
  ComboBoxBasis->setCurrentItem(data.basisset);
  
  ///// Molecule
  ComboBoxMolecule->setCurrentItem(data.styleMolecule);
  ComboBoxForces->setCurrentItem(data.styleForces);
  SpinBoxFastRender->setValue(data.fastRenderLimit);
  CheckBoxElement->setChecked(data.showElements);
  CheckBoxNumber->setChecked(data.showNumbers);
  SliderBondSizeLines->setValue(data.sizeLines);
    updateLineEditBondSizeLines(); // needed?
  LineEditBondSizeTubes->setText(data.sizeBonds);
    updateSliderBondSizeTubes();
  LineEditForceSizeTubes->setText(data.sizeForces);
    updateSliderForceSizeTubes();
  ColorButtonLabel->setColor(data.colorLabels);
  ColorButtonIC->setColor(data.colorICs);
  ColorButtonBackgroundGL->setColor(data.colorBackgroundGL);
  ColorButtonSelection->setColor(data.colorSelections);
  ColorButtonForce->setColor(data.colorForces);
  SliderSelectionOpacity->setValue(data.opacitySelections);
  SliderForceOpacity->setValue(data.opacityForces);
  ComboBoxForceColor->setCurrentItem(data.forcesOneColor ? 1 : 0);

  ///// Visuals
  ButtonGroupBackground->setButton(data.backgroundType);
  LineEditBackground->setText(data.backgroundImage);
  ColorButtonBackground->setColor(data.backgroundColor);
  ComboBoxStyle->setCurrentItem(data.styleApplication);

  ///// OpenGL
  SliderQuality->setValue(data.quality);
  CheckBoxAA->setChecked(data.antialias);
  CheckBoxSmooth->setChecked(data.smoothShading);
  CheckBoxDepthCue->setChecked(data.depthCue);
  ButtonGroupLightPosition->setButton(data.lightPosition);
  ColorButtonLight->setColor(QColor(data.lightColor));
  SliderSpecular->setValue(data.materialSpecular);
  SliderShininess->setValue(data.materialShininess);
  ButtonGroupProjection->setButton(data.perspectiveProjection ? 0 : 1);

  ///// PVM
  ListViewPVMHosts->clear();
  { for(QStringList::Iterator it = data.pvmHosts.begin(); it != data.pvmHosts.end(); it++)
      new QListViewItem(ListViewPVMHosts, *it); }
  if(!data.pvmHosts.isEmpty())
    ListViewPVMHosts->setSelected(ListViewPVMHosts->firstChild(), true);  
  changePVMHost();
}

///// updateStyle /////////////////////////////////////////////////////////////
void PreferencesBase::updateStyle()
/// Updates the style of the application according
/// to the current item of ComboBoxStyle.
{  
  qDebug("Calling PreferencesBase::updateStyle");
  qDebug("currentStyle = " + QString(QApplication::style().name()));    
  if(ComboBoxStyle->currentItem() == 0)
  {
    if(QString(QApplication::style().name()).lower() != startupStyleName.lower())
    {
      qDebug("About to revert to the startup style");      
      QApplication::setStyle(startupStyleName);
    }
  }
  else
  {
    qDebug("About to change the style to "+ComboBoxStyle->currentText());
    QApplication::setStyle(ComboBoxStyle->currentText());
  }
}

///// updatePaths /////////////////////////////////////////////////////////////
void PreferencesBase::updatePaths()
/// Updates the data of the class Paths.
{
  Paths::abc         = QDir::convertSeparators(ListViewExecutables->findItem("ABC", 0)->text(1));
  Paths::achar       = QDir::convertSeparators(ListViewExecutables->findItem("Achar",0)->text(1));
  Paths::atomscf     = QDir::convertSeparators(ListViewExecutables->findItem("AtomSCF",0)->text(1));
  Paths::atomscf_inp = QDir::convertSeparators(ListViewExecutables->findItem("AtomSCF_inp",0)->text(1));
  Paths::atomscf_o2d = QDir::convertSeparators(ListViewExecutables->findItem("AtomSCF_o2d",0)->text(1));
  Paths::b112gfchk   = QDir::convertSeparators(ListViewExecutables->findItem("B112gfchk",0)->text(1));
  Paths::bios2ped    = QDir::convertSeparators(ListViewExecutables->findItem("Bios2ped",0)->text(1));
  Paths::brabo       = QDir::convertSeparators(ListViewExecutables->findItem("Brabo",0)->text(1));
  Paths::buur        = QDir::convertSeparators(ListViewExecutables->findItem("Buur",0)->text(1));
  Paths::cnvrtaff    = QDir::convertSeparators(ListViewExecutables->findItem("Cnvrtaff",0)->text(1));
  Paths::crd2gauss   = QDir::convertSeparators(ListViewExecutables->findItem("Crd2gauss",0)->text(1));
  Paths::crd2xyz     = QDir::convertSeparators(ListViewExecutables->findItem("Crd2xyz",0)->text(1));
  Paths::diffcrd     = QDir::convertSeparators(ListViewExecutables->findItem("Diffcrd",0)->text(1));
  Paths::distor      = QDir::convertSeparators(ListViewExecutables->findItem("Distor",0)->text(1));
  Paths::forkon      = QDir::convertSeparators(ListViewExecutables->findItem("Forkon",0)->text(1));
  Paths::frex        = QDir::convertSeparators(ListViewExecutables->findItem("Frex",0)->text(1));
  Paths::gar2ped     = QDir::convertSeparators(ListViewExecutables->findItem("Gar2ped",0)->text(1));
  Paths::geom        = QDir::convertSeparators(ListViewExecutables->findItem("Geom",0)->text(1));
  Paths::gfchk2b11   = QDir::convertSeparators(ListViewExecutables->findItem("Gfchk2b11",0)->text(1));
  Paths::gfchk2crd   = QDir::convertSeparators(ListViewExecutables->findItem("Gfchk2crd",0)->text(1));
  Paths::gfchk2pun   = QDir::convertSeparators(ListViewExecutables->findItem("Gfchk2pun",0)->text(1));
  Paths::hkl         = QDir::convertSeparators(ListViewExecutables->findItem("HKL",0)->text(1));
  Paths::log2crd     = QDir::convertSeparators(ListViewExecutables->findItem("Log2crd",0)->text(1));
  Paths::maff        = QDir::convertSeparators(ListViewExecutables->findItem("Maff",0)->text(1));
  Paths::makeden     = QDir::convertSeparators(ListViewExecutables->findItem("Makeden",0)->text(1));
  Paths::makexit     = QDir::convertSeparators(ListViewExecutables->findItem("Makexit",0)->text(1));
  Paths::molsplit    = QDir::convertSeparators(ListViewExecutables->findItem("Molsplit",0)->text(1));
  Paths::out2aff     = QDir::convertSeparators(ListViewExecutables->findItem("Out2aff",0)->text(1));
  Paths::potdicht    = QDir::convertSeparators(ListViewExecutables->findItem("Potdicht",0)->text(1));
  Paths::pullarc     = QDir::convertSeparators(ListViewExecutables->findItem("Pullarc",0)->text(1));
  Paths::refine      = QDir::convertSeparators(ListViewExecutables->findItem("Refine",0)->text(1));
  Paths::relax       = QDir::convertSeparators(ListViewExecutables->findItem("Relax",0)->text(1));
  Paths::ring        = QDir::convertSeparators(ListViewExecutables->findItem("Ring",0)->text(1));
  Paths::spfmap      = QDir::convertSeparators(ListViewExecutables->findItem("Spfmap",0)->text(1));
  Paths::startvec    = QDir::convertSeparators(ListViewExecutables->findItem("Startvec",0)->text(1));
  Paths::stock       = QDir::convertSeparators(ListViewExecutables->findItem("Stock",0)->text(1));
  Paths::symm        = QDir::convertSeparators(ListViewExecutables->findItem("Symm",0)->text(1));
  Paths::table       = QDir::convertSeparators(ListViewExecutables->findItem("Table",0)->text(1));
  Paths::xyz2crd     = QDir::convertSeparators(ListViewExecutables->findItem("Xyz2crd",0)->text(1));

  if(RadioButtonBin2->isChecked())
    Paths::bin = LineEditBin->text();
  else
    Paths::bin = QString::null;  
  Paths::basisset = LineEditBasis->text();

  ///// further process the directories to simplify them as much as possible
  ///// (most importantly remove final {back}slashes)
  Paths::bin = QDir::convertSeparators(QDir::cleanDirPath(Paths::bin));
  Paths::basisset = QDir::convertSeparators(QDir::cleanDirPath(Paths::basisset));
}

