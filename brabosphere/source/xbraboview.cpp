/***************************************************************************
                        xbraboview.cpp  -  description
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
  \class XbraboView
  \brief This class is the MDI view widget which also holds all data pertaining a
         calculation.

*/
/// \file
/// Contains the implementation of the class XbraboView

///// Header files ////////////////////////////////////////////////////////////

// C++ header files
#include <cmath> 

// Qt header files
#include <qapplication.h>
#include <qcheckbox.h>
#include <qcombobox.h>
#include <qcursor.h>
#include <qdir.h>
#include <qdom.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlistview.h>
#include <qmessagebox.h>
#include <qpixmap.h>
#include <qpopupmenu.h>
#include <qprogressbar.h>
#include <qnamespace.h>
#include <qrect.h>
#include <qsplitter.h>
#include <qstringlist.h>
#include <qtextedit.h>
#include <qtimer.h>
#include <qwhatsthis.h>

// Xbrabo header files
#include "atomset.h"
#include "brabobase.h"
#include "calculation.h"
#include "crdfactory.h"
#include "domutils.h"
#include "glmoleculeview.h"
#include "globalbase.h"
#include "iconsets.h"
#include "moleculepropertieswidget.h"
#include "outputchooserwidget.h"
#include "paths.h"
#include "relaxbase.h"
#include "statustext.h"
#include "textviewwidget.h"
#include "xbraboview.h"
#include "version.h"

#if defined(USE_KMDI) || defined(USE_KMDI_DLL)
#define QextMdiChildView KMdiChildView
#endif

///////////////////////////////////////////////////////////////////////////////
///// Public Members                                                      /////
///////////////////////////////////////////////////////////////////////////////

///// Constructor /////////////////////////////////////////////////////////////
XbraboView::XbraboView(QWidget* mainWin, QWidget* parent, QString title, const char* name, WFlags f) : QextMdiChildView( title, parent, name, f),
  globalSetup(0),
  braboSetup(0),
  relaxSetup(0),
  calcDate(QDateTime::currentDateTime(Qt::UTC).toString(Qt::ISODate)),
  calculation(0),
  mainWindow(mainWin)
/// The default constructor.
/// \warning \c parent is not Xbrabo, but QextMdiChildArea or 0 (can dynamically change!).
/// => pointer to Xbrabo passed using mainWin
{
  calcCounter++;
  calcDefaultName = tr("unnamed") + QString::number(calcCounter);
  calcFileName = calcDefaultName;

  ///// Set the GLWidgets format
  //QGLFormat f;
  //f.setAlpha(true);  // needed for antialiasing
  //QGLFormat::setDefaultFormat(f);

  ///// Initialize the atoms
  atoms = new AtomSet();
  
  ///// Construct widget layout
  BigLayout = new QVBoxLayout(this,10);
    Splitter = new QSplitter(this, 0);
    Splitter->setOrientation(QSplitter::Vertical);
    Splitter->setOpaqueResize(true);
      MoleculeView = new GLMoleculeView(atoms, Splitter);
      MoleculeView->setSizePolicy(QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding, false));
      QWidget* LayoutWidget = new QWidget(Splitter);
      StatusLayout = new QVBoxLayout(LayoutWidget,0);
        TextLabelStatus = new QLabel(LayoutWidget);
        TextLabelStatus->setText(tr("Status"));
        TextEditStatus = new StatusText(LayoutWidget);
        StatusLayout->addWidget(TextLabelStatus);
        StatusLayout->addWidget(TextEditStatus);
    BigLayout->addWidget(Splitter);
    ProgressLayout = new QHBoxLayout(BigLayout);
      TextLabelProgress = new QLabel(this);
      TextLabelProgress->setText(tr("Progress"));
      ProgressBar = new QProgressBar(this);
      ProgressBar->setCenterIndicator(true);
      ProgressLayout->addWidget(TextLabelProgress);
      ProgressLayout->addWidget(ProgressBar);
    // default size is 400x400
    MoleculeView->resize(MoleculeView->width(),350);
    MoleculeView->resetView();

  ///// Connections
  connect(TextEditStatus, SIGNAL(rightButtonClicked()),this, SLOT(popup()));
#if defined(USE_KMDI) || defined(USE_KMDI_DLL)
  connect(this, SIGNAL(gotFocus(KMdiChildView*)), this, SIGNAL(changed()));
#else
  connect(this, SIGNAL(gotFocus(QextMdiChildView*)), this, SIGNAL(changed()));
#endif
  connect(MoleculeView, SIGNAL(modified()), this, SLOT(setModified()));
  connect(MoleculeView, SIGNAL(changed()), this, SIGNAL(changed()));
  
  ///// What's This
  QWhatsThis::add(MoleculeView, 
    tr("<p>Shows the molecular system in 3D. Pressing the right mouse button "
       "opens a context menu containing the most frequently used actions from the "
       "Molecule menu.</p>"
       "<p>The scene can be manipulated with the mouse and the keyboard. Here follows "
       "a list of possible manipulations and ways to invoke them. Directions are indicated "
       "either by the arrow keys or by dragging the mouse in the desired direction while "
       "pressing the left mouse button.</p>"
       "<ul>"
       "<li>Rotate around the X-axis: <em>Up</em> and <em>Down</em>"
       "<li>Rotate around the Y-axis: <em>Left</em> and <em>Right</em>"
       "<li>Rotate around the Z-axis: <em>Shift+Left</em> and <em>Shift+Right</em>"
       "<li>Zoom: <em>Shift+Up</em> and <em>Shift+Down</em> or using the scrollwheel of the mouse"
       "<li>Translate horizontally: <em>Ctrl+Left</em> and <em>Ctrl+Right</em>"
       "<li>Translate vertically: <em>Ctrl+Up</em> and <em>Ctrl+Down</em>"       
       "</ul>"
       "<p>Atoms can be selected/deselected by clicking on them with the left mouse button. When a "
       "selection is present, all actions shown above can be applied to the selection only. This can "
       "be done either by holding the <em>Alt</em> modifier or selecting the <em>Manipulate Selection</em> "
       "button/menu-entry. Using the scrollwheel of the mouse will still zoom the entire system.</p>"
       "<p>When a bond, angle or torsion angle is selected, the internal coordinate (whose value is always visible) "
       "can be altered by using <em>Ctrl+Shift+Left</em> and <em>Ctrl+Shift+Right</em>.</p>"
      ));
  QWhatsThis::add(TextEditStatus,
    tr("<p>Shows a log of informational messages mostly when a calculation is running. Results presented "
       "here will not be saved but are always accessible through other means. Data can, however, be "
       "copied using the standard <em>Copy</em> action.</p><p>The size of this box can be changed by dragging "
       "the bar between this box and the 3D view above (not visible in all skins).</p>"));
  QWhatsThis::add(ProgressBar,
    tr("Indicates the progress of a running calculation. It will be reset shortly after a calculation has finished."));

  ///// Other 
  setModified(false);
}

///// Destructor //////////////////////////////////////////////////////////////
XbraboView::~XbraboView()
/// The default destructor.
{
  delete atoms;
}

///// calculationType /////////////////////////////////////////////////////////
unsigned int XbraboView::calculationType() const
/// Returns the type of the calculation.
/// \arg GlobalBase::SINGLE_POINT_ENERGY
/// \arg GlobalBase::ENERGY_AND_FORCES
/// \arg GlobalBase::GEOMETRY_OPTIMIZATION
/// \arg GlobalBase::FREQUENCIES
{  
  if(globalSetup == 0)
    return GlobalBase::SinglePointEnergy;

  return globalSetup->calculationType();
}

///// buurType ////////////////////////////////////////////////////////////////
unsigned int XbraboView::buurType() const
/// Returns the type of crystal environment used.
/// \arg GlobalBase::NO_BUUR
/// \arg GlobalBase::PC
/// \arg GlobalBase::SM
{  
  if(globalSetup == 0)
    return GlobalBase::NoBuur;
  
  return globalSetup->buurType();
}

///// isModified //////////////////////////////////////////////////////////////
bool XbraboView::isModified() const
/// Returns true if the calculation has been modified and should be saved.
{  
  return calcModified;
}

/*//// isAnimating /////////////////////////////////////////////////////////////
bool XbraboView::isAnimating() const
/// Returns true if the molecule is animating.
{
  return MoleculeView->isAnimating();
}*/

///// name ////////////////////////////////////////////////////////////////////
QString XbraboView::name() const
/// Returns the name of the calculation.
{
  if(globalSetup == 0)
    return calcDefaultName;

  return globalSetup->name();
}

///// fileName ////////////////////////////////////////////////////////////////
QString XbraboView::fileName() const
/// Returns the filename of the calculation.
{
  return calcFileName;
}

///// directory ///////////////////////////////////////////////////////////////
QString XbraboView::directory() const
/// Returns the directory of the calculation.
{
  if(globalSetup == 0)
    return QDir::convertSeparators(QDir::homeDirPath()) + QDir::separator() + calcDefaultName;

  return globalSetup->directory();
}

///// isRunning ///////////////////////////////////////////////////////////////
bool XbraboView::isRunning() const
/// Returns true if the calculation is running.
{
  if(calculation == 0)
    return false;
  
  return calculation->isRunning();
}

///// isPaused ////////////////////////////////////////////////////////////////
bool XbraboView::isPaused() const
/// Returns true if the calculation is paused.
{
  if(calculation == 0)
    return false;
  
  return calculation->isPaused();
}

/*//// hasSelection ////////////////////////////////////////////////////////////
bool XbraboView::hasSelection() const
/// Returns true if some atoms are selected
{ 
  return MoleculeView->selectedAtoms() > 0;
}*/

///// cut /////////////////////////////////////////////////////////////////////
void XbraboView::cut()
/// Deletes the marked text and puts it on the clipboard. It is currently only
/// connected to the cut facility of StatusText. As that is 
/// read-only, nothing is actually cut, only put on the clipboard.
{ 
  TextEditStatus->cut();
}

///// copy ////////////////////////////////////////////////////////////////////
void XbraboView::copy()
/// Copies the marked text to the clipboard. It is currently only connected to
/// the copy facility of StatusText.
{
  TextEditStatus->copy();
}

///// paste ///////////////////////////////////////////////////////////////////
void XbraboView::paste()
/// Pastes the contents from the clipboard. It is currently only
/// connected to the paste facility of StatusText. As that is 
/// read-only, nothing actually happens.
{
  TextEditStatus->paste();
}

///// moleculeView ////////////////////////////////////////////////////////////
GLMoleculeView* XbraboView::moleculeView() const
/// Returns a pointer to the GLMoleculeView class. 
{
  return MoleculeView;
}

///// loadCML /////////////////////////////////////////////////////////////////
bool XbraboView::loadCML(const QDomDocument* doc)
/// Loads all calculation data from a QDomDocument. It returns the success status.
{  
  ///// Check for the CML root element
  QDomElement root = doc->documentElement();
  if(root.tagName() != "cml")
  {
    QMessageBox::warning(this, Version::appName, tr("The selected file is a regular CML file"), QMessageBox::Ok, QMessageBox::NoButton);
    return false;
  }
  ///// Check the namespaceURI
  if(root.attribute("xmlns:" + DomUtils::ns) != DomUtils::uriDict10)
  {
    //qDebug("attribute list for <cml>:");
    //for(int i = 0; i < root.attributes().count(); i++)
    //  qDebug("attribute 1: "+root.attributes().item(i).nodeName()+" "+root.attributes().item(i).nodeValue());
    QMessageBox::warning(this, Version::appName, tr("The selected file does not contain the correct version indicator."), QMessageBox::Ok, QMessageBox::NoButton);
    return false;
  }
  ///// Get the creation date
  QDomNode metaNode = root.namedItem("metadataList");
  if(!metaNode.isNull())
  {
    QDomNode childNode = metaNode.firstChild();
    while(!childNode.isNull())
    {
      if(childNode.isElement() && childNode.toElement().tagName() == "metadata" && childNode.toElement().attribute("name") == "dc:date")
      {
        calcDate = childNode.toElement().attribute("content");
        break;
      }
      childNode = childNode.nextSibling();
    }
  }

  TextEditStatus->append(tr("Loading data"));
  ///// Let the different classes load their needed info
  ///// coordinates
  QDomElement section = root.namedItem("molecule").toElement();
  if(section.isNull())
    TextEditStatus->append("<font color=red>" + tr("  ...no coordinates found while loading") + "</font>");
  else
  {
    TextEditStatus->append(tr("  ...loading coordinates"));
    atoms->loadCML(&section);
  }
  ///// Brabosphere related section are stored in a list
  root = root.namedItem("parameterList").toElement();
  if(root.attribute("dictRef") != DomUtils::ns + ":settings")
  {
    TextEditStatus->append("<font color=red>" + tr("  ...no calculation related information found while loading") + "</font>");  
  }
  else
  {
    section = root.firstChild().toElement();
    while(!section.isNull() && section.tagName() == "parameterList")
    {
      if(section.attribute("dictRef") == DomUtils::ns + ":geometry")
      {
        ///// geometry
        {
          TextEditStatus->append(tr("  ...setting geometry"));  
          loadCMLLocal(&section);
        }
      }
      else if(section.attribute("dictRef") == DomUtils::ns + ":view")
      {
        ///// view
        {
          TextEditStatus->append(tr("  ...loading view data"));  
          MoleculeView->loadCML(&section);
        }
      }
      else if(section.attribute("dictRef") == DomUtils::ns + ":global")
      {
        ///// global
        {
          TextEditStatus->append(tr("  ...loading Global data"));  
          initGlobalSetup();
          globalSetup->loadCML(&section);
        }
      }
      else if(section.attribute("dictRef") == DomUtils::ns + ":energy_and_forces")
      {
        ///// brabo
        {
          TextEditStatus->append(tr("  ...loading Energy & Forces data"));  
          initBraboSetup();
          braboSetup->loadCML(&section);
        }
      }
      else if(section.attribute("dictRef") == DomUtils::ns + ":geometry_optimization")
      {
        ///// relax
        {
          TextEditStatus->append(tr("  ...loading Geometry Optimization data"));  
          initRelaxSetup();
          relaxSetup->loadCML(&section);
        }
      }
      else if(section.attribute("dictRef") == DomUtils::ns + ":calculation")
      {
        ///// calculation
        {
          TextEditStatus->append(tr("  ...loading Calculation data"));  
          bool ok = initCalculation();
          calculation->loadCML(&section);
          if(!ok)
            calculation->stop();
        }
      }
      section = section.nextSibling().toElement();
    }
    TextEditStatus->append(tr("Loading done"));  
  }
  
  updateBraboSetup();
  updateRelaxSetup();
  setModified(false);
  return true;
}

///// saveCML /////////////////////////////////////////////////////////////////
QDomDocument* XbraboView::saveCML()
/// Saves the calculation to a QDomDocument.
{  
  QDomDocument* doc = new QDomDocument();
  ///// add the <?xml> line
  QDomProcessingInstruction instr = doc->createProcessingInstruction("xml","version=\"1.0\" encoding=\"UTF-8\"");
  doc->appendChild(instr);
  ///// add the <!DOCTYPE> line -> this was only used for CML1
  //QDomImplementation impl;
  //QDomDocumentType type = impl.createDocumentType("cml",0, "cml.dtd");
  //doc->appendChild(type);  
  ///// add the <cml> element          
  QDomElement root = doc->createElement("cml");
  root.setAttribute("xmlns", DomUtils::uriNSCML);
  root.setAttribute("xmlns:" + DomUtils::nsCMLM, DomUtils::uriDictCMLM);
  root.setAttribute("xmlns:" + DomUtils::ns, DomUtils::uriDict10);
  root.setAttribute("xmlns:" + DomUtils::nsXSD, DomUtils::uriDictXSD);
  root.setAttribute("xmlns:" + DomUtils::nsAtomic, DomUtils::uriDictAtomic);
  root.setAttribute("xmlns:" + DomUtils::nsSI, DomUtils::uriDictSI);
  root.setAttribute("xmlns:" + DomUtils::nsDC, DomUtils::uriNSDC);

  doc->appendChild(root);
  ///// add the meta data
  QDomElement childNode = doc->createElement("metadataList");
  childNode.setAttribute("title", "Generated by " + Version::appName);
  root.appendChild(childNode);
  QDomElement grandChildNode = doc->createElement("metadata");
  grandChildNode.setAttribute("name", DomUtils::nsDC + ":creator");
  grandChildNode.setAttribute("content", Version::appName + " " + Version::appVersion);
  childNode.appendChild(grandChildNode);
  grandChildNode = doc->createElement("metadata");
  childNode.appendChild(grandChildNode);
  grandChildNode.setAttribute("name", DomUtils::nsDC + ":description");
  grandChildNode.setAttribute("content", Version::appName + " Calculation File");
  grandChildNode = doc->createElement("metadata");
  grandChildNode.setAttribute("content", calcDate);
  grandChildNode.setAttribute("name", DomUtils::nsDC + ":date");
  childNode.appendChild(grandChildNode);
  ///// add the molecular data
  childNode = doc->createElement("molecule");
  root.appendChild(childNode);  
  atoms->saveCML(&childNode);
  ///// add the rest of the data in a parameter list
  childNode = doc->createElement("parameterList");
  childNode.setAttribute("dictRef", DomUtils::ns + ":settings");
  childNode.setAttribute("title", Version::appName + " specific data");
  root.appendChild(childNode);
  ///// add the positional data
  grandChildNode = doc->createElement("parameterList");
  grandChildNode.setAttribute("dictRef", DomUtils::ns + ":geometry");
  childNode.appendChild(grandChildNode);
  saveCMLLocal(&grandChildNode);
  ///// add the GL viewer data
  grandChildNode = doc->createElement("parameterList");
  grandChildNode.setAttribute("dictRef", DomUtils::ns + ":view");
  childNode.appendChild(grandChildNode);
  MoleculeView->saveCML(&grandChildNode);
  ///// add the keyword data
  if(globalSetup != 0)
  {
    grandChildNode = doc->createElement("parameterList");
    grandChildNode.setAttribute("dictRef", DomUtils::ns + ":global");
    childNode.appendChild(grandChildNode);
    globalSetup->saveCML(&grandChildNode);
  }
  if(braboSetup != 0)
  {
    grandChildNode = doc->createElement("parameterList");
    grandChildNode.setAttribute("dictRef", DomUtils::ns + ":energy_and_forces");
    childNode.appendChild(grandChildNode);
    braboSetup->saveCML(&grandChildNode);
  }
  if(relaxSetup != 0)
  {
    grandChildNode = doc->createElement("parameterList");
    grandChildNode.setAttribute("dictRef", DomUtils::ns + ":geometry_optimization");
    childNode.appendChild(grandChildNode);
    relaxSetup->saveCML(&grandChildNode);
  }
  ///// add the calculation data
  if(calculation != 0)
  {
    grandChildNode = doc->createElement("parameterList");
    grandChildNode.setAttribute("dictRef", DomUtils::ns + ":calculation");
    childNode.appendChild(grandChildNode);
    calculation->saveCML(&grandChildNode);
  }
  return doc;
}

///// setFileName /////////////////////////////////////////////////////////////
void XbraboView::setFileName(const QString filename)
/// Sets the filename of the calculation.
{
  calcFileName = filename;
  updateCaptions();
}

///////////////////////////////////////////////////////////////////////////////
///// Public Slots                                                        /////
///////////////////////////////////////////////////////////////////////////////

///// setModified /////////////////////////////////////////////////////////////
void XbraboView::setModified(bool state)
/// Sets the 'modified' status of the calculation.
/// Defaults to true if no argument is provided.
{
  calcModified = state;
  if(!state)
  {
    // setting the calculation is not being modified is only done after a 
    // save has occured
    MoleculeView->setModified(false);
    if(calculation != 0)
      calculation->setModified(false);
  }
  updateCaptions();
  emit changed();
}

///// moleculeReadCoordinates /////////////////////////////////////////////////
void XbraboView::moleculeReadCoordinates()
/// Reads coordinates into the AtomSet from a file.
{
  QString filename;
  unsigned int result = CrdFactory::readFromFile(atoms, filename);
  switch(result)
  {
    case CrdFactory::OK:
      setModified();
      MoleculeView->updateAtomSet(true); // does a complete reset
      if(calculation != 0)
        calculation->setContinuable(false);
      TextEditStatus->append(tr("New coordinates read: ") + QString::number(atoms->count()) + tr(" atoms"));
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

///// moleculeSaveCoordinates /////////////////////////////////////////////////
void XbraboView::moleculeSaveCoordinates()
/// Saves the coordinates from the AtomSet into a file.
{
  unsigned short int result = CrdFactory::writeToFile(atoms);
  switch(result)
  {
    case CrdFactory::UnknownExtension:
      QMessageBox::warning(this, tr("Unknown format"), tr("The file has an unknown extension"), QMessageBox::Ok, QMessageBox::NoButton);
      break;
    case CrdFactory::ErrorOpen:
      QMessageBox::warning(this, tr("Error opening file"), tr("The file could not be opened for writing"), QMessageBox::Ok, QMessageBox::NoButton);
      break;
    case CrdFactory::ErrorWrite:
      QMessageBox::warning(this, tr("Error writing file"), tr("The file could not be written"), QMessageBox::Ok, QMessageBox::NoButton);
      break;
  }
}

///// moleculeFPS /////////////////////////////////////////////////////////////
void XbraboView::moleculeFPS()
/// Calculates the Frames-Per-Second for the current parameters.
{
  TextEditStatus->append(tr("Frames per second: ") + QString::number(MoleculeView->calculateFPS()));
}

///// setupGlobal /////////////////////////////////////////////////////////////
void XbraboView::setupGlobal()
/// Sets up the global options.
{  
  initGlobalSetup();
  if(globalSetup->exec() == QDialog::Accepted)
  {
    ///// OK was pressed so something must have changed
    setModified();
    updateBraboSetup();
    updateRelaxSetup();
    updateCaptions();
  }
}

///// setupBrabo //////////////////////////////////////////////////////////////
void XbraboView::setupBrabo()
/// Sets up the Brabo options.
{  
  initBraboSetup();
  if(braboSetup->exec() == QDialog::Accepted)
  {
    setModified();
    if(calculation != 0 && calculation->isRunning() && globalSetup->calculationType() != GlobalBase::SinglePointEnergy
      && globalSetup->calculationType() != GlobalBase::EnergyAndForces)
    {
      bool ok;
      QStringList basissetList = braboSetup->basissets(ok);
      if(!ok)
        return; // don't apply the changes if the basissets are not available for all atoms
      QString atdens = braboSetup->generateAtdens(ok);
      if(!ok)
        return; // same for the atomic density basisset

      ///// update the calculation with the new input files
      bool prefer;
      unsigned int size1, size2;
      QString startVector = braboSetup->startVector(prefer, size1, size2); // zero sizes for size1 and size2 imply a problem with
                                                                           // reading the basisset files.
      if(size1 == 0)
        return;
      calculation->setBraboInput(braboSetup->generateInput(BraboBase::BRABO), basissetList, startVector, prefer, size1, size2);
      calculation->setStockInput(braboSetup->generateInput(BraboBase::STOCK), atdens);      
    }
  }
}

///// setupRelax //////////////////////////////////////////////////////////////
void XbraboView::setupRelax()
/// Sets up the Relax options.
{
  initRelaxSetup();
  if(relaxSetup->exec() == QDialog::Accepted)
  {
    setModified();
    if(calculation != 0 && calculation->isRunning() && globalSetup->calculationType() == GlobalBase::GeometryOptimization)
    {
      ///// no restrictions for a new input file for relax
      std::vector<unsigned int> steps;
      std::vector<double> factors;
      relaxSetup->scaleFactors(steps, factors);
      calculation->setRelaxInput(relaxSetup->generateInput(RelaxBase::AFF), relaxSetup->generateInput(RelaxBase::MAFF), 
                                 relaxSetup->inputGenerationFrequency(), relaxSetup->maxSteps(), steps, factors);
    }
  }
}

///// setupFreq ///////////////////////////////////////////////////////////////
void XbraboView::setupFreq()
/// Sets up the Distor & Forkon options. Not implemented yet.
{
}

///// setupBuur ///////////////////////////////////////////////////////////////
void XbraboView::setupBuur()
/// Sets up the Buur options. Not implemented yet.
{
}

///// start ///////////////////////////////////////////////////////////////////
void XbraboView::start()
/// Starts the calculation.
{
  if(calculation != 0 && calculation->isRunning())
  {      // succesful unpausing
    if(calculation->isPaused())
        TextEditStatus->append("<font color=blue>" + tr("Calculation unpaused") + "</font>");
    calculation->start(); // this handles the error messages or un-pausing
    emit changed();
    return;
  }


  ///// setup the Calculation class ///
  if(!initCalculation()) // does initial setup
    return;

  ///// Try to start the calculation
  if(calculation->start())
    globalSetup->allowChanges(false);
  else
    return;

  ///// cosmetic setup
  TextEditStatus->append("<font color=blue>" + tr("Calculation started") + "</font>");
  QString tempItem = " * " + tr("Type") + ": ";
  unsigned int calcType = globalSetup->calculationType();
  switch(calcType)
  {
    case GlobalBase::SinglePointEnergy:    tempItem += tr("Single Point Energy");
                                           break;
    case GlobalBase::EnergyAndForces:      tempItem += tr("Energy & Forces");
                                           break;
    case GlobalBase::GeometryOptimization: tempItem += tr("Geometry Optimization");
                                           break;
  }
  TextEditStatus->append(tempItem);
  TextEditStatus->append(" * " + tr("Method") + ": " + braboSetup->method());
  if(calcType == GlobalBase::GeometryOptimization)
    TextEditStatus->append(" * " + tr("Optimization step") + " 1");
  TextEditStatus->append("   - " + tr("Calculating energy"));  

  // progressbar  
  ProgressBar->reset();
  unsigned int numSteps = braboSetup->maxIterations(); // never zero
  if(calcType != GlobalBase::SinglePointEnergy)
    numSteps *= 2;
  if(calcType == GlobalBase::GeometryOptimization)
  {
    unsigned int calcMaxSteps = relaxSetup->maxSteps();
    if(calcMaxSteps == 0)
      numSteps *= 3*atoms->count(); // starting value ~ minimal number of IC's
    else
      numSteps *= calcMaxSteps;
  }
  ProgressBar->setTotalSteps(numSteps);
  lastProgress = 0;
  ProgressBar->setProgress(lastProgress);
  emit changed();
}

///// pause ///////////////////////////////////////////////////////////////////
void XbraboView::pause()
/// Pauses the calculation.
{  
  if(calculation != 0)
  {
    if(calculation->pause())
    {
      // succesful (un)pausing
      if(calculation->isPaused())
        TextEditStatus->append("<font color=blue>" + tr("Calculation paused") + "</font>" +  tr("(effective after the current step)"));
      else
        TextEditStatus->append("<font color=blue>" + tr("Calculation unpaused") + "</font>");
      emit changed();
    }
  }
}

///// stop ////////////////////////////////////////////////////////////////////
void XbraboView::stop()
/// Stops a running calculation.
{  
  if(calculation != 0)
  {
    calculation->stop();
    emit changed();
  }
}

///// writeInput //////////////////////////////////////////////////////////////
void XbraboView::writeInput()
/// Writes the needed input files for a calculation.
{
  if(!initCalculation())
    return;

  calculation->writeInput();

  /*
  ///// BRABO input file
  initGlobalSetup();
  initBraboSetup();
  QFile file(globalSetup->directory() + QDir::separator() + globalSetup->name() + ".inp");
  qDebug("writing file "+file.name());
  if(!file.open(IO_WriteOnly))
    return;
  QTextStream stream(&file);
  QStringList input = braboSetup->generateInput(BraboBase::BRABO);
  for(QStringList::Iterator it = input.begin(); it != input.end(); it++)
    stream << *it << "\n";
  file.close();
  */
}

///// cleanCalculation ////////////////////////////////////////////////////////
void XbraboView::cleanCalculation()
/// Cleans the calculation directory
{
  if(calculation == 0 || calculation->isRunning())
    return;
  if(QMessageBox::information(this, "Clean up calculation", "This will remove all files pertaining to the calculation from the calculation directory", "Clean", "Cancel", QString::null, 0, 1) == 0)
    calculation->clean();
}

///// viewOutput //////////////////////////////////////////////////////////////
void XbraboView::viewOutput()
/// Shows the output from the calculation.
{  
  ///// create an output widget
  OutputChooserWidget* outputChooser = new OutputChooserWidget(this, 0, true, 0);
  outputChooser->ListView->setSorting(-1);
  QPixmap pixmapPresent = IconSets::getPixmap(IconSets::OK);

  if(calculation)
  {
    ///// get the outputs for the current cycle
    QListViewItem* item = new QListViewItem(outputChooser->ListView);
    item->setText(0, tr("last"));
    if(!calculation->braboOutput().isEmpty())
      item->setPixmap(1, pixmapPresent);
    if(!calculation->stockOutput().isEmpty())
      item->setPixmap(2, pixmapPresent);
    if(!calculation->relaxOutput().isEmpty())
      item->setPixmap(3, pixmapPresent);
    if(!calculation->affOutput().isEmpty())
      item->setPixmap(4, pixmapPresent);
  
    ///// get the outputs for the saved cycles
    vector<unsigned int> cycles;
    vector<bool> savedBrabo, savedStock, savedRelax, savedAFF;
    calculation->getAvailableOutputs(cycles, savedBrabo, savedStock, savedRelax, savedAFF);
    for(unsigned int i = 0; i < cycles.size(); i++)
    {
      item = new QListViewItem(outputChooser->ListView, item);
      item->setText(0, QString::number(cycles[i]));
      if(savedBrabo[i])
        item->setPixmap(1, pixmapPresent);
      if(savedStock[i])
        item->setPixmap(2, pixmapPresent);
      if(savedRelax[i])
        item->setPixmap(3, pixmapPresent);
      if(savedAFF[i])
        item->setPixmap(4, pixmapPresent);
    }
  }

  ///// execute the dialog and get the info by signals
  connect(outputChooser->ListView, SIGNAL(clicked(QListViewItem*, const QPoint&, int)), SLOT(showOutput(QListViewItem*, const QPoint&, int)));
  connect(outputChooser->PushButtonClose, SIGNAL(clicked()), outputChooser, SLOT(close()));
  outputChooser->exec();
  delete outputChooser;
}

///// updatePVMHosts //////////////////////////////////////////////////////////
void XbraboView::updatePVMHosts(const QStringList& hosts)
/// Updates the PVM host list for BraboBase.
{  
  if(!braboSetup)
    hostListPVM = hosts;
  else
    braboSetup->setPVMHosts(hosts);
}


///////////////////////////////////////////////////////////////////////////////
///// Protected Member Functions                                          /////
///////////////////////////////////////////////////////////////////////////////

///// keyPressEvent ///////////////////////////////////////////////////////////
void XbraboView::keyPressEvent(QKeyEvent* e)
/// Overridden from QextMdiChildFrm::keyPressEvent.
/// Sends unhandled key events to the main window.
{
  if(isTopLevel())
    // No parent so transfer it manually while changing the type from
    // QEvent::KeyPress to QEvent::QAccel. This might not be a very wise thing
    // to do as the docs state a QKeyEvent can only have a type of
    // QEvent::KeyPress or QEvent::KeyRelease.
    qApp->sendEvent(mainWindow, new QKeyEvent(QEvent::Accel, e->key(), e->ascii(), e->state(), e->text(), e->isAutoRepeat(), e->count()));

  e->ignore(); // automatic propagation to the parent widget when it's not NULL (mainwindow)
}

///// mouseReleaseEvent ///////////////////////////////////////////////////////
void XbraboView::mouseReleaseEvent(QMouseEvent* e)
/// Overridden from QextMdiChildFrm::mouseReleaseEvent.
/// Handles left mouse button releases. This will popup a contextmenu.
{
  if(e->button() == QMouseEvent::RightButton)
    popup();
  // eat all other release events
}


///////////////////////////////////////////////////////////////////////////////
///// Private Slots                                                       /////
///////////////////////////////////////////////////////////////////////////////

///// popup ///////////////////////////////////////////////////////////////////
void XbraboView::popup()
/// Shows a popupmenu with menuoptions.
{
  ///// build the menu
  QPopupMenu* popup = new QPopupMenu(0,0);
  popup->insertItem(IconSets::getIconSet(IconSets::MoleculeRead), tr("Read coordinates..."), this, SLOT(moleculeReadCoordinates()));
  popup->insertItem(tr("Add atoms..."), MoleculeView, SLOT(addAtoms()));
  const int ID_MOLECULE_DELETE = popup->insertItem(tr("Delete selected atoms"), MoleculeView, SLOT(deleteSelectedAtoms()));
  popup->insertSeparator();
  const int ID_ALTER_CARTESIAN = popup->insertItem(tr("Alter cartesian coordinates"), MoleculeView, SLOT(alterCartesian()));
  const int ID_ALTER_INTERNAL = popup->insertItem(tr("Alter internal coordinate"), MoleculeView, SLOT(alterInternal()));    
  popup->insertSeparator();
  popup->insertItem(tr("Select all"), MoleculeView, SLOT(selectAll()));
  popup->insertItem(tr("Select none"), MoleculeView, SLOT(unselectAll()));
  popup->insertSeparator();
  popup->insertItem(IconSets::getIconSet(IconSets::SetupGlobal), tr("Setup global"),this, SLOT(setupGlobal()));
  popup->insertItem(IconSets::getIconSet(IconSets::SetupBrabo), tr("Setup energy && Forces"),this, SLOT(setupBrabo()));
  const int ID_SETUP_RELAX = popup->insertItem(IconSets::getIconSet(IconSets::SetupRelax), tr("Setup geometry optimization"),this, SLOT(setupRelax()));
  popup->insertSeparator();
  const int ID_RUN_START = popup->insertItem(IconSets::getIconSet(IconSets::Start), tr("Start"), this, SLOT(start()));
  const int ID_RUN_PAUSE = popup->insertItem(IconSets::getIconSet(IconSets::Pause), tr("Pause"), this, SLOT(pause()));
  const int ID_RUN_STOP = popup->insertItem(IconSets::getIconSet(IconSets::Stop), tr("Stop"), this, SLOT(stop()));
  popup->insertItem(IconSets::getIconSet(IconSets::Outputs), tr("View output"), this, SLOT(viewOutput()));

  /*
  ///// submenu reset
  QPopupMenu* popupReset = new QPopupMenu(popup, 0);
  popupReset->insertItem(tr("Translation"), MoleculeView, SLOT(centerView()));
  popupReset->insertItem(tr("Orientation"), MoleculeView, SLOT(resetOrientation()));
  popupReset->insertItem(tr("Zoom"), MoleculeView, SLOT(zoomFit()));
  popupReset->insertItem(tr("View"), MoleculeView, SLOT(resetView()));
  ///// submenu select
  QPopupMenu* popupSelect = new QPopupMenu(popup, 0);
  popupSelect->insertItem(tr("All"), MoleculeView, SLOT(selectAll()));
  popupSelect->insertItem(tr("None"), MoleculeView, SLOT(unselectAll()));
  ///// submenu alter
  QPopupMenu* popupAlter = new QPopupMenu(popup, 0);
  const int ID_ALTER_CARTESIAN = popupAlter->insertItem(tr("Cartesian coordinates"), MoleculeView, SLOT(alterCartesian()));
  const int ID_ALTER_INTERNAL = popupAlter->insertItem(tr("Internal coordinate"), MoleculeView, SLOT(alterInternal()));    
  ///// submenu setup
  QPopupMenu* popupSetup = new QPopupMenu(popup, 0);
  popupSetup->insertItem(IconSets::getIconSet(IconSets::SetupGlobal), tr("Global"),this, SLOT(setupGlobal()));
  popupSetup->insertItem(IconSets::getIconSet(IconSets::SetupBrabo), tr("Energy && Forces"),this, SLOT(setupBrabo()));
  const int ID_SETUP_RELAX = popupSetup->insertItem(IconSets::getIconSet(IconSets::SetupRelax), tr("Geometry Optimization"),this, SLOT(setupRelax()));
  const int ID_SETUP_FREQ = popupSetup->insertItem(IconSets::getIconSet(IconSets::SetupFreq), tr("Frequencies"),this, SLOT(setupFreq()));
  const int ID_SETUP_BUUR = popupSetup->insertItem(IconSets::getIconSet(IconSets::SetupBuur), tr("Crystal"),this, SLOT(setupBuur()));
  ///// submenu run
  QPopupMenu* popupRun = new QPopupMenu(popup, 0);
  const int ID_RUN_START = popupRun->insertItem(IconSets::getIconSet(IconSets::Start), tr("Start"), this, SLOT(start()));
  const int ID_RUN_PAUSE = popupRun->insertItem(IconSets::getIconSet(IconSets::Pause), tr("Pause"), this, SLOT(pause()));
  const int ID_RUN_STOP = popupRun->insertItem(IconSets::getIconSet(IconSets::Stop), tr("Stop"), this, SLOT(stop()));
  popupRun->insertItem(IconSets::getIconSet(IconSets::Outputs), tr("View output"), this, SLOT(viewOutput()));
  ///// menu                                     
  popup->insertItem(IconSets::getIconSet(IconSets::MoleculeRead), tr("Read coordinates..."), this, SLOT(moleculeReadCoordinates()));
  popup->insertItem(tr("Change isosurfaces..."), MoleculeView, SLOT(showDensity()));
  popup->insertItem(tr("Change display mode..."), this, SLOT(showProperties()), 0);
  popup->insertItem(tr("Reset"), popupReset);
  popup->insertItem(tr("Select"), popupSelect);
  popup->insertItem(tr("Alter"), popupAlter);
  popup->insertItem(tr("Add atoms..."), MoleculeView, SLOT(addAtoms()));
  const int ID_MOLECULE_DELETE = popup->insertItem(tr("Delete selected atoms"), MoleculeView, SLOT(deleteSelectedAtoms()));
  // don't add the animate action as it doesn't update the mainmenu action
  //const int ID_MOLECULE_ANIMATION = popup->insertItem(IconSets::getIconSet(IconSets::MoleculeAnimate), tr("Toggle animation"), this, SLOT(moleculeAnimate()));
  popup->insertItem(tr("Calculate FPS"), this, SLOT(moleculeFPS()));
  popup->insertItem(IconSets::getIconSet(IconSets::Image), tr("Save image"), MoleculeView, SLOT(saveImage()));
  popup->insertSeparator();
  popup->insertItem(tr("Setup"), popupSetup);
  popup->insertItem(tr("Run"), popupRun);
  */

  ///// disable items for Alter menu
  if(MoleculeView->selectedAtoms() == 0 || isRunning())
  {
    popup->setItemEnabled(ID_ALTER_CARTESIAN, false);
    popup->setItemEnabled(ID_MOLECULE_DELETE, false);
  }
  if(MoleculeView->selectedAtoms() < 2 || MoleculeView->selectedAtoms() > 4)
    popup->setItemEnabled(ID_ALTER_INTERNAL, false);
    
  ///// disable items for Setup menu
  if(globalSetup == 0 || globalSetup->calculationType() != GlobalBase::GeometryOptimization)
    popup->setItemEnabled(ID_SETUP_RELAX, false);
  //if(globalSetup == 0 || globalSetup->calculationType() != GlobalBase::Frequencies)
  //  popup->setItemEnabled(ID_SETUP_FREQ, false);
  //if(globalSetup == 0 || globalSetup->buurType() == GlobalBase::NoBuur)
  //  popup->setItemEnabled(ID_SETUP_BUUR, false);

  ///// disable items for Run menu
  if(isRunning())
    popup->setItemEnabled(ID_RUN_START, false);
  else
  {
    popup->setItemEnabled(ID_RUN_PAUSE, false);
    popup->setItemEnabled(ID_RUN_STOP, false);
  }
 
  ///// execute the menu  
  popup->exec(QCursor::pos());
  delete popup;
}

///// updateIteration /////////////////////////////////////////////////////////
void XbraboView::updateIteration(unsigned int iteration, double energy)
/// Updates the status window with a new iteration and energy
{
  // only show each iteration when not optimizing
  if(globalSetup->calculationType() == GlobalBase::SinglePointEnergy || globalSetup->calculationType() == GlobalBase::EnergyAndForces)
    TextEditStatus->append("       " + tr("Iteration") + " " + QString::number(iteration) + " : " + tr("Energy") + " = " + QString::number(energy,'f',8));
    //ListBoxStatus->insertItem("       " + tr("Iteration") + " " + QString::number(iteration) + " : " + tr("Energy") + " = " + QString::number(energy,'f',8));
  // always add the final energy
  if(iteration == 50)
  {
    //ListBoxStatus->insertItem("   - " + tr("Total energy") + " = " + QString::number(energy,'f',8));
    TextEditStatus->append("   - " + tr("Total energy") + " = " + QString::number(energy,'f',8));
    if(globalSetup->calculationType() != GlobalBase::SinglePointEnergy)
      TextEditStatus->append("   - " + tr("Calculating forces"));
      //ListBoxStatus->insertItem("   - " + tr("Calculating forces"));
  }

  //ListBoxStatus->setCurrentItem(ListBoxStatus->count()-1);
  lastProgress++;
  ProgressBar->setProgress(lastProgress);
}

///// updateCycle /////////////////////////////////////////////////////////////
void XbraboView::updateCycle(unsigned int cycle)
/// Updates the status window with a new cycle
{
  bool ref1, ref2, ref3, ref4, ref5;
  calculation->getRefinementParameters(ref1, ref2, ref3, ref4, ref5);
  QString refResult = "   - " + tr("Refinement parameters") + ":";
  refResult += ref1 ? " +" : " -";
  refResult += ref2 ? " +" : " -";
  refResult += ref3 ? " +" : " -";
  refResult += ref4 ? " +" : " -";
  refResult += ref5 ? " +" : " -";
  TextEditStatus->append(refResult);
  TextEditStatus->append(" * " + tr("Optimization step") + " " + QString::number(cycle));
  TextEditStatus->append("   - " + tr("Calculating energy"));
  lastProgress += braboSetup->maxIterations(); // assume calculation of forces takes as long as energy
  if(lastProgress >= static_cast<unsigned int>(ProgressBar->totalSteps()))
    ProgressBar->setTotalSteps(lastProgress + 2* braboSetup->maxIterations());
  ProgressBar->setProgress(lastProgress); 
}

/*//// updateCharges ///////////////////////////////////////////////////////////
void XbraboView::updateCharges()
/// Updates the OpenGL window with new charges
{
  if(MoleculeView->isShowingCharges(AtomSet::Stockholder))
  {
    MoleculeView->updateGL();
    QString text = "   - " + tr("Updated stockholder charges");
    QString scf = atoms->chargesSCF(AtomSet::Stockholder);
    QString density = atoms->chargesDensity(AtomSet::Stockholder);
    if(!scf.isEmpty())
    {
      text += " " + tr("calculated using") + " " + scf;
      if(!density.isEmpty())
        text += tr(" from the ") + density;
    }
    ListBoxStatus->insertItem(text);
  }
}*/

///// cleanupCalculation //////////////////////////////////////////////////////
void XbraboView::cleanupCalculation(unsigned int error)
/// Does the cleanup after a calculation has ended. The type of error is passed.
{
  qDebug("XbraboView::cleanupCalculation() called");

  switch(error)
  {
    case Calculation::NoError:
      {
        if(globalSetup->calculationType() == GlobalBase::GeometryOptimization)
        {
          TextEditStatus->append("   - " + tr("Refinement parameters") + ": + + + + +");
          TextEditStatus->append(" * " + tr("Structure optimized"));
        }
        TextEditStatus->append("<font color=green>" + tr("Calculation finished successfully") + "</font>");
      }
      break;
    case Calculation::UndefinedError:
      TextEditStatus->append("<font color=red>" + tr("Calculation finished with errors") + "</font>");
      break;
    case Calculation::NoConvergence:
      TextEditStatus->append("<font color=red>" + tr("Calculation finished with errors: no convergence") + "</font>");
      break;
    case Calculation::CloseNuclei:
      TextEditStatus->append("<font color=red>" + tr("Calculation finished with errors: nuclei too close") + "</font>");
      break;
    case Calculation::MaxCyclesExceeded:
      TextEditStatus->append("<font color=red>" + tr("Calculation finished with errors: maximum number of optimization cycles reached") + "</font>");
      break;
    case Calculation::ManualStop:
      TextEditStatus->append("<font color=red>" + tr("Calculation stopped immediately") + "</font>");
      break;
  }

  if(globalSetup->calculationType() == GlobalBase::GeometryOptimization)
    calculation->setContinuable(true);
  ProgressBar->setProgress(ProgressBar->totalSteps());
  QTimer::singleShot(5*1000, ProgressBar, SLOT(reset())); // reset the progressbar after 5 seconds

  globalSetup->allowChanges(true);
  emit changed();
}

///// showOutput //////////////////////////////////////////////////////////////
void XbraboView::showOutput(QListViewItem* item, const QPoint&, int column)
/// Shows the output corresponding to the item clicked on.
{ 
  ///// out of bounds checks
  if(item == 0 || column < 1 || column > 4 || calculation == 0)
    return;

  ///// check whether an output file should be present
  if(item->pixmap(column) == 0)
    return;

  ///// determine which output to show
  unsigned int cycle;
  if(item->text(0) == tr("last"))
    cycle = 0;
  else
    cycle = item->text(0).toUInt();
  
  ///// load the file into a QStringList
  QStringList contents;
  int fileWidth = 81;
  switch(column)
  {
    case 1: // it's a Brabo output file
            contents = calculation->braboOutput(cycle);
            fileWidth = 111;
            break;
    case 2: // it's a Stock output file
            contents = calculation->stockOutput(cycle);
            fileWidth = 97;
            break;
    case 3: // it's a Relax output file
            contents = calculation->relaxOutput(cycle);
            fileWidth = 131;
            break;
    case 4: //it's an AFF file
            contents = calculation->affOutput(cycle);
            break;
  }
  if(contents.isEmpty())
    return;

  ///// show the contents in a viewer widget
  TextViewWidget* outputViewer = new TextViewWidget(this, 0, true, Qt::WDestructiveClose);
  outputViewer->TextEdit->setTextFormat(Qt::LogText); // default is PlainText
  QFontMetrics metrics(outputViewer->TextEdit->currentFont());
  int newWidth = fileWidth*metrics.width(" ") + 2*outputViewer->layout()->margin() + outputViewer->TextEdit->verticalScrollBar()->sliderRect().width();
  outputViewer->resize(newWidth, outputViewer->height()); 

  ///// fill the QTextEdit
  switch(column)
  {
    case 1: // it's a Brabo output file
            if(cycle == 0)
              outputViewer->setCaption(tr("Showing Brabo output from last cycle"));
            else
              outputViewer->setCaption(tr("Showing Brabo output from cycle") + " " + QString::number(cycle));
            outputViewer->TextEdit->setText(contents.join("\n"));      
            break;

      /*
      for(QStringList::iterator it = contents.begin(); it != contents.end(); it++)
      {
        QString line = *it;
        //line.replace("&", "&amp;");
        //line.replace("<", "&lt;");
        //line.replace(">", "&gt;");

        ///// colors:
        ///// RED          : errors
        ///// GREEN        : input
        ///// BLUE         : results (darkbue for text, blue for numbers)
        ///// BLACK        : rest
        ///// YELLOW       :
        ///// ORANGE       :
        ///// MAGENTA      :
        ///// CYAN         :
        ///// GREY         : sections (>>>>>) + formatting (*****)
        if(line.left(15) == " TOTAL ENERGY =")
        {
          line = "<font color=darkblue>" + line.mid(0, 15)  + "</font>"
               + "<font color=blue>"     + line.mid(15, 20) + "</font>"
               + "<font color=darkblue>" + line.mid(35, 8)  + "</font>";
          outputViewer->TextEdit->append(line);
          line = *(++it); // total energy in other units
          line = "<font color=blue>"     + line.mid(0, 25)  + "</font>"
               + "<font color=darkblue>" + line.mid(25, 7)  + "</font>"
               + "<font color=blue>"     + line.mid(32, 19) + "</font>"
               + "<font color=darkblue>" + line.mid(51, 3)  + "</font>"
               + "<font color=blue>"     + line.mid(54, 18) + "</font>"
               + "<font color=darkblue>" + line.mid(72, 9)  + "</font>"
               + "<font color=blue>"     + line.mid(81, 15) + "</font>"
               + "<font color=darkblue>" + line.mid(96, 7)  + "</font>";
          outputViewer->TextEdit->append(line);
          line = *(++it); // total energy in other units (2)
          line = "<font color=blue>"     + line.mid(0, 25)  + "</font>"
               + "<font color=darkblue>" + line.mid(25, 7)  + "</font>"
               + "<font color=blue>"     + line.mid(32, 19) + "</font>"
               + "<font color=darkblue>" + line.mid(51, 5)  + "</font>"
               + "<font color=blue>"     + line.mid(56, 19) + "</font>"
               + "<font color=darkblue>" + line.mid(75, 6)  + "</font>"
               + "<font color=blue>"     + line.mid(81, 19) + "</font>"
               + "<font color=darkblue>" + line.mid(100, 3) + "</font>";
          outputViewer->TextEdit->append(line);
          line = *(++it); // empty line
          outputViewer->TextEdit->append(line);
          line = *(++it); // 2e and 1e electron energy
          line = "<font color=darkblue>" + line.mid(0, 21)  + "</font>"
               + "<font color=blue>"     + line.mid(21, 16) + "</font>"
               + "<font color=darkblue>" + line.mid(37, 22) + "</font>"
               + "<font color=blue>"     + line.mid(59, 17) + "</font>";
          outputViewer->TextEdit->append(line);
          line = *(++it); // kinetic and core-attraction energy
          line = "<font color=darkblue>" + line.mid(0, 21)  + "</font>"
               + "<font color=blue>"     + line.mid(21, 16) + "</font>"
               + "<font color=darkblue>" + line.mid(37, 22) + "</font>"
               + "<font color=blue>"     + line.mid(59, 17) + "</font>";
          outputViewer->TextEdit->append(line);
          line = *(++it); // virial and nuclear energy
          line = "<font color=darkblue>" + line.mid(0, 21)  + "</font>"
               + "<font color=blue>"     + line.mid(21, 16) + "</font>"
               + "<font color=darkblue>" + line.mid(37, 22) + "</font>"
               + "<font color=blue>"     + line.mid(59, 17) + "</font>";
          outputViewer->TextEdit->append(line);
          line = *(++it); // empty line
          outputViewer->TextEdit->append(line);
          line = *(++it); // XYZ
          line = "<font color=darkblue>" + line + "</font>";
          outputViewer->TextEdit->append(line);
          line = *(++it); // dipole
          line = "<font color=darkblue>" + line.mid(0, 10)  + "</font>"
               + "<font color=blue>"     + line.mid(10, 45) + "</font>"
               + "<font color=darkblue>" + line.mid(55, 3)  + "</font>";
          outputViewer->TextEdit->append(line);
          line = *(++it); // dipole (2)
          line = "<font color=blue>"     + line.mid(0, 55)  + "</font>"
               + "<font color=darkblue>" + line.mid(55, 6)  + "</font>";
          outputViewer->TextEdit->append(line);
          line = *(++it); // dipole (3)
          line = "<font color=blue>"     + line.mid(0, 55)  + "</font>"
               + "<font color=darkblue>" + line.mid(55, 5)  + "</font>";
          outputViewer->TextEdit->append(line);
          line = *(++it); // empty line
          outputViewer->TextEdit->append(line);
          line = *(++it); // |dipole|
          line = "<font color=darkblue>" + line.mid(0, 11)  + "</font>"
               + "<font color=blue>"     + line.mid(11, 15) + "</font>"
               + "<font color=darkblue>" + line.mid(26, 3)  + "</font>"
               + "<font color=blue>"     + line.mid(29, 15) + "</font>"
               + "<font color=darkblue>" + line.mid(44, 6)  + "</font>"
               + "<font color=blue>"     + line.mid(50, 16) + "</font>"
               + "<font color=darkblue>" + line.mid(66, 5)  + "</font>";   
          outputViewer->TextEdit->append(line);                            
        }
        else if(line.left(10) == "**********")
        {
          line = "<font color=darkgray>" + line + "</font>";
          outputViewer->TextEdit->append(line);
        }
        else if(line.left(5) == " >>>>>")
        {
          line = "<font color=darkgray>" + line + "</font>";
          outputViewer->TextEdit->append(line);
        }
        else                      
          outputViewer->TextEdit->append(line);
      }
      break;
      */
    case 2: // it's a Stock output file
            if(cycle == 0)
              outputViewer->setCaption(tr("Showing Stock output from last cycle"));
            else
              outputViewer->setCaption(tr("Showing Stock output from cycle") + " " + QString::number(cycle));
            outputViewer->TextEdit->setText(contents.join("\n"));      
            break;
    case 3: // it's a Relax output file      
            if(cycle == 0)
              outputViewer->setCaption(tr("Showing Relax output from last cycle"));
            else
              outputViewer->setCaption(tr("Showing Relax output from cycle") + " " + QString::number(cycle));
            outputViewer->TextEdit->setText(contents.join("\n"));      
            break;
    case 4: //it's an AFF file
            if(cycle == 0)
              outputViewer->setCaption(tr("Showing AFF file from last cycle"));
            else
              outputViewer->setCaption(tr("Showing AFF file from cycle") + " " + QString::number(cycle));
            outputViewer->TextEdit->setText(contents.join("\n"));      
            break;      
  }

  ///// show it from the top
  //outputViewer->TextEdit->ensureVisible(1, 1); // not needed on Windows apparently
  //outputViewer->TextEdit->setCursorPosition(1,1);
  //outputViewer->TextEdit->ensureCursorVisible();
  outputViewer->exec(); // will delete itself when closed
}

///// showProperties //////////////////////////////////////////////////////////
void XbraboView::showProperties()
/// Changes which properties are shown in the OpenGL window. This is done through
/// a MoleculePropertiesWidget.
{
  MoleculePropertiesWidget* properties = new MoleculePropertiesWidget(this, 0, true, 0);
  properties->ComboBoxRenderingType->setCurrentItem(MoleculeView->displayStyle(GLSimpleMoleculeView::Molecule));
  properties->ComboBoxForces->setCurrentItem(MoleculeView->displayStyle(GLSimpleMoleculeView::Forces));
  properties->CheckBoxElement->setChecked(MoleculeView->isShowingElements());
  properties->CheckBoxNumber->setChecked(MoleculeView->isShowingNumbers());
  if(MoleculeView->isShowingCharges(AtomSet::Mulliken))
    properties->ComboBoxCharge->setCurrentItem(AtomSet::Mulliken); 
  else if(MoleculeView->isShowingCharges(AtomSet::Stockholder))
    properties->ComboBoxCharge->setCurrentItem(AtomSet::Stockholder);
  else
    properties->ComboBoxCharge->setCurrentItem(AtomSet::None);

  if(properties->exec() == QDialog::Accepted)
  {
    MoleculeView->setDisplayStyle(GLSimpleMoleculeView::Molecule, properties->ComboBoxRenderingType->currentItem());
    MoleculeView->setDisplayStyle(GLSimpleMoleculeView::Forces, properties->ComboBoxForces->currentItem());
    MoleculeView->setLabels(properties->CheckBoxElement->isChecked(),
                            properties->CheckBoxNumber->isChecked(),
                            properties->ComboBoxCharge->currentItem());
    MoleculeView->updateGL();
  }
  delete properties;
}

///////////////////////////////////////////////////////////////////////////////
///// Private Member Functions                                            /////
///////////////////////////////////////////////////////////////////////////////

///// loadCMLLocal ////////////////////////////////////////////////////////////
void XbraboView::loadCMLLocal(const QDomElement* root)
/// Loads the positions and sizes of the XbraboView widgets from a QDomElement.
{  
  int positionX = 0, positionY = 0, sizeWidth = 400, sizeHeight = 400;
  QValueList<int> splitterSizes;
  const QString prefix = "geometry_";
  QDomNode childNode = root->firstChild();
  while(!childNode.isNull())
  {
    if(childNode.isElement() && childNode.nodeName() == "parameter")
    {
      if(DomUtils::dictEntry(childNode, prefix + "position-x"))
        DomUtils::readNode(&childNode, &positionX);
      else if(DomUtils::dictEntry(childNode, prefix + "position-y"))
        DomUtils::readNode(&childNode, &positionY);
      else if(DomUtils::dictEntry(childNode, prefix + "width"))
        DomUtils::readNode(&childNode, &sizeWidth);
      else if(DomUtils::dictEntry(childNode, prefix + "height"))
        DomUtils::readNode(&childNode, &sizeHeight);  
      else if(DomUtils::dictEntry(childNode, prefix + "splitter_sizes"))
        DomUtils::readNode(&childNode, &splitterSizes);
    }
    childNode = childNode.nextSibling();
  }
  QPoint position(positionX, positionY);
  QSize size(sizeWidth, sizeHeight);
  resize(size);
  move(position);
  if(splitterSizes.size() != 0)
    Splitter->setSizes(splitterSizes);
}

///// saveCMLLocal ////////////////////////////////////////////////////////////
void XbraboView::saveCMLLocal(QDomElement* root)
/// Saves the positions and sizes of the XbraboView widgets to a QDomElement.
{  
  ///// XbraboView geometry (use QextMDI functions)
  QRect geometry = internalGeometry();  
  const QString prefix = "geometry_";
  DomUtils::makeNode(root, geometry.x(), prefix + "position-x");
  DomUtils::makeNode(root, geometry.y(), prefix + "position-y");
  DomUtils::makeNode(root, geometry.width(), prefix + "width");
  DomUtils::makeNode(root, geometry.height(), prefix + "height");
  
  QValueList<int> list = Splitter->sizes();
  DomUtils::makeNode(root, list, prefix + "splitter_sizes");
}

///// updateCaptions //////////////////////////////////////////////////////////
void XbraboView::updateCaptions()
/// Updates the captions for the widget and taskbar.
{
  QFileInfo fi = QFileInfo(calcFileName);  
  QString tempString = fi.baseName(true);
  if(globalSetup != 0 && !globalSetup->description().isEmpty())
    tempString += QString(" - ") + globalSetup->description();
  if(isModified())
    tempString += tr(" [modified]");
  setCaption(tempString);
  setTabCaption(fi.baseName(true));
}

///// updateBraboSetup ////////////////////////////////////////////////////////
void XbraboView::updateBraboSetup()
/// Updates braboSetup according to globalSetup.
{
  if(braboSetup == 0)
    return;

  braboSetup->setForces(globalSetup->calculationType() != GlobalBase::SinglePointEnergy);
  braboSetup->setDescription(globalSetup->description());
  braboSetup->setName(globalSetup->name());
  braboSetup->setExtendedFormat(globalSetup->extendedFormat());
}

///// updateRelaxSetup ////////////////////////////////////////////////////////
void XbraboView::updateRelaxSetup()
/// Updates relaxSetup according to globalSetup.
{
  if(relaxSetup == 0)
    return;

  relaxSetup->setName(globalSetup->name());
  relaxSetup->setDescription(globalSetup->description());
  relaxSetup->setDir(globalSetup->directory());
  relaxSetup->setExtendedFormat(globalSetup->extendedFormat());
}

///// makeDirCurrent //////////////////////////////////////////////////////////
bool XbraboView::makeDirCurrent(const QString dir, const QString title)
/// Makes \c dir the current directory.
/// Shows a messagebox with the error and \c title when something goes wrong.
{
  QDir workDir = dir;
  if(!workDir.exists())
  {
    if(!workDir.mkdir(workDir.path()))
    {
      QMessageBox::warning(this, title, tr("Unable to create the directory ") + workDir.path());
      return false;
    }
  }
  if(!QDir::setCurrent(workDir.path()))
  {
    QMessageBox::warning(this, title, tr("Unable to change to the directory ") + workDir.path());
    return false;
  }
  return true;
}

///// initGlobalSetup /////////////////////////////////////////////////////////
void XbraboView::initGlobalSetup()
/// Construct the GlobalBase widget \c globalSetup if needed.
{
  if(globalSetup != 0)
    return;
  
  globalSetup = new GlobalBase(this, 0, true, 0);
  globalSetup->setDefaultName(calcDefaultName);
  globalSetup->setIcon(QPixmap::fromMimeSource("SetupGlobalNormal"));
}

///// initBraboSetup //////////////////////////////////////////////////////////
void XbraboView::initBraboSetup()
/// Construct the BraboBase widget \c braboSetup if needed. It also calls
/// initGlobalSetup as it needs data from it.
{
  if(braboSetup != 0)
    return;
  
  initGlobalSetup();
  braboSetup = new BraboBase(atoms, this, 0, true, 0);
  //braboSetup->setIcon(QPixmap::fromMimeSource("SetupBraboNormal")); // doesn't work apparently 
  updateBraboSetup();
  if(!hostListPVM.isEmpty())
    updatePVMHosts(hostListPVM);
}

///// initRelaxSetup //////////////////////////////////////////////////////////
void XbraboView::initRelaxSetup()
/// Construct the RelaxBase widget \c relaxSetup if needed. It also calls
/// initGlobalSetup as it needs data from it.
{
  if(relaxSetup != 0)
    return;

  initGlobalSetup();
  relaxSetup = new RelaxBase(atoms, this, 0, true, 0);
  relaxSetup->setIcon(QPixmap::fromMimeSource("SetupRelaxNormal"));
  updateRelaxSetup();
}

///// initCalculation /////////////////////////////////////////////////////////
bool XbraboView::initCalculation()
/// Construct the Calculation if needed and does an initial setup. This function
/// is to be called by start(), loadCML and writeInput. It returns false if any error occurs.
{
  if(calculation == 0)
  {
    calculation = new Calculation(atoms, this);
    connect(calculation, SIGNAL(newIteration(unsigned int, double)), this, SLOT(updateIteration(unsigned int, double)));
    connect(calculation, SIGNAL(newCycle(unsigned int)), this, SLOT(updateCycle(unsigned int)));
    connect(calculation, SIGNAL(updated()), MoleculeView, SLOT(updateGL()));
    connect(calculation, SIGNAL(finished(unsigned int)), this, SLOT(cleanupCalculation(unsigned int)));
    connect(calculation, SIGNAL(modified()), this, SIGNAL(changed()));
  }
  
  ///// check for the presence of any coordinates
  if(atoms->count() == 0)
  {
    QMessageBox::warning(this, tr("Initialize calculation"), tr("There are no atoms present"));
    return false;
  }  

  ///// GlobalBase related parameters (should be given before those of BraboBase)
  initBraboSetup(); // also inits globalSetup  
  calculation->setCalculationType(globalSetup->calculationType(), globalSetup->buurType());
  calculation->setParameters(globalSetup->name(), globalSetup->directory(), globalSetup->extendedFormat());
  calculation->setBackup(1, true, true, true, true, true); // default until interface is made (in class GlobalBase)

  ///// BraboBase related parameters
  bool ok;
  QStringList basissetList = braboSetup->basissets(ok);
  if(!ok)
    return false;
  QString atdens = braboSetup->generateAtdens(ok);
  if(!ok)
    return false;
  bool prefer;
  unsigned int size1, size2;
  QString startVector = braboSetup->startVector(prefer, size1, size2);
  if(size1 == 0)
    return false;
  calculation->setBraboInput(braboSetup->generateInput(BraboBase::BRABO), basissetList, startVector, prefer, size1, size2);
  calculation->setStockInput(braboSetup->generateInput(BraboBase::STOCK), atdens);
  
  ///// RelaxBase related parameters
  if(globalSetup->calculationType() == GlobalBase::GeometryOptimization)
  {
    initRelaxSetup();
    std::vector<unsigned int> steps;
    std::vector<double> factors;
    relaxSetup->scaleFactors(steps, factors);
    qDebug("XbraboView::start:");
    for(unsigned int i = 0; i < steps.size(); i++)
      qDebug(" %d, step %d, factor %f", i, steps[i], factors[i]);

    calculation->setRelaxInput(relaxSetup->generateInput(RelaxBase::AFF), relaxSetup->generateInput(RelaxBase::MAFF), 
                               relaxSetup->inputGenerationFrequency(), relaxSetup->maxSteps(), steps, factors);
  }
  return true;
}

///// updateAtomSet ///////////////////////////////////////////////////////////
void XbraboView::updateAtomSet()
/// Does the necessary updating when the AtomSet has been changed. This means 
/// new coordinates have been read from file or atoms have been added/deleted.
{
  if(calculation != 0)
    calculation->setContinuable(false);

  // possibly update the SpinBox limits in RelaxBase when needed. ATM it is done 
  // each time the dialog is opened. 
}

///////////////////////////////////////////////////////////////////////////////
///// Static variables                                                    /////
///////////////////////////////////////////////////////////////////////////////

unsigned int XbraboView::calcCounter = 0;

