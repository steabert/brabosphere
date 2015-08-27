/***************************************************************************
                        globalbase.cpp  -  description
                             -------------------
    begin                : Thu Aug 8 2002
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
  \class GlobalBase
  \brief This class allows setup of the global calculation options.

  It contains the implementation for the widget GlobalWidget.
*/
/// \file
/// Contains the implementation of the class GlobalBase.

///// Header files ////////////////////////////////////////////////////////////

// C++ header files
#include <cassert>

// Qt header files
#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qcombobox.h>
#include <qdir.h>
#include <qdom.h>
#include <qfiledialog.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qstring.h>
#include <qtoolbutton.h>

// Xbrabo header files
#include "domutils.h"
#include "globalbase.h"
#include "iconsets.h"
#include "latin1validator.h"


///////////////////////////////////////////////////////////////////////////////
///// Public Member Functions                                             /////
///////////////////////////////////////////////////////////////////////////////

///// constructor /////////////////////////////////////////////////////////////
GlobalBase::GlobalBase(QWidget* parent, const char* name, bool modal, WFlags fl) : GlobalWidget(parent, name, modal, fl)
/// The default constructor.
{
  ButtonGroupRunType->hide(); // will have to be remodeled completely for a future version
  CheckBoxBuur->hide();
  ComboBoxBuur->hide();
  ToolButtonDir->setIconSet(IconSets::getIconSet(IconSets::Open));
  resize(minimumSize());
  makeConnections();
  //init(); // 
}

///// destructor //////////////////////////////////////////////////////////////
GlobalBase::~GlobalBase()
/// The default destructor.
{

}

///// setDefaultName //////////////////////////////////////////////////////////
void GlobalBase::setDefaultName(const QString name)
/// Sets the default calculation name.
{
  defaultName = name;
  init();
}

///// calculationType /////////////////////////////////////////////////////////
unsigned int GlobalBase::calculationType() const
/// Returns the type of calculation.
/// \arg GlobalBase::SinglePointEnergy
/// \arg GlobalBase::EnergyAndForces
/// \arg GlobalBase::GeometryOptimization
/// \arg GlobalBase::Frequencies
{  
  return data.type;
}

///// buurType ////////////////////////////////////////////////////////////////
unsigned int GlobalBase::buurType() const
/// Returns the type of crystal calculation.
/// \arg GlobalBase::NoBuur
/// \arg GlobalBase::PC
/// \arg GlobalBase::SM
{    
  if(!data.useBuur)
    return NoBuur;
  else
    return data.buurType + 1;
}

///// extendedFormat //////////////////////////////////////////////////////////
bool GlobalBase::extendedFormat() const
/// Returns true if extended format is to be used.
{  
  return data.useXF;
}

///// description /////////////////////////////////////////////////////////////
QString GlobalBase::description() const
/// Returns the description for the calculation.
{  
  return data.description;
}

///// name ////////////////////////////////////////////////////////////////////
QString GlobalBase::name() const
/// Returns the name of the calculation.
{  
  return data.name;
}

///// directory ///////////////////////////////////////////////////////////////
QString GlobalBase::directory() const
/// Returns the directory in which the calculation will be run.
{  
  return data.directory;
}

///// runType /////////////////////////////////////////////////////////////////
unsigned int GlobalBase::runType() const
/// Returns the type of run.
/// \arg GlobalBase::Xbrabo : under control of the program
/// \arg GlobalBase::Script : using a script file
/// \arg GlobalBase::Queue  : submitting a script file to a queue
{  
  return data.runType;
}

///// queue ///////////////////////////////////////////////////////////////////
unsigned int GlobalBase::queue() const
/// Returns the queue to which the script file should be submitted.
{  
  return data.queue;
}

///// allowChanges ////////////////////////////////////////////////////////////
void GlobalBase::allowChanges(const bool status)
/// Allows or prohibits changing every option. During a calculation most options
/// may not be changed (type, format, queue, etc.).
{  
  GroupBoxType->setEnabled(status);
  LineEditName->setEnabled(status);
  LineEditDir->setEnabled(status);
  ToolButtonDir->setEnabled(status);
}

///// loadCML /////////////////////////////////////////////////////////////////
void GlobalBase::loadCML(const QDomElement* root)
/// Reads the widget data from a QDomElement.
{  
  ///// This function will normally never be called while the dialog is showing
  ///// => the data struct is always up to date with the contents of the widgets
  assert(!isVisible());

  const QString prefix = "global_";
  QDomNode childNode = root->firstChild();
  while(!childNode.isNull())
  {
    if(childNode.isElement() && childNode.toElement().tagName() == "parameter")
    {
      if(DomUtils::dictEntry(childNode, prefix + "calculation_type"))
        DomUtils::readNode(&childNode, &data.type);
      //else if(childNode.toElement().attribute("dictRef").contains("use_buur")
      //  DomUtils::readNode(&childNode, &data.useBuur);
      //else if(childNode.toElement().attribute("dictRef").contains("buur_type")
      //  DomUtils::readNode(&childNode, &data.buurType);
      else if(DomUtils::dictEntry(childNode, prefix + "use_extended_format"))
        DomUtils::readNode(&childNode, &data.useXF);
      else if(DomUtils::dictEntry(childNode, prefix + "description"))
        DomUtils::readNode(&childNode, &data.description);
      else if(DomUtils::dictEntry(childNode, prefix + "name"))
        DomUtils::readNode(&childNode, &data.name);
      else if(DomUtils::dictEntry(childNode, prefix + "directory"))
        DomUtils::readNode(&childNode, &data.directory);
      //if(childNode.toElement().attribute("dictRef").contains("run_type")
      //  DomUtils::readNode(&childNode, &data.runType);
      //if(childNode.toElement().attribute("dictRef").contains("queue")
      //  DomUtils::readNode(&childNode, &data.queue);
    }
    childNode = childNode.nextSibling();
  }
  restoreWidgets(); 
}

///// saveCML /////////////////////////////////////////////////////////////////
void GlobalBase::saveCML(QDomElement* root)
/// Saves the widget data to a QDomElement.
{  
  ///// This function will normally never be called while the dialog is showing
  ///// => the data struct is always up to date with the contents of the widgets
  assert(!isVisible());

  ///// stuff to be saved: data(struct),...
  const QString prefix = "global_";
  DomUtils::makeNode(root, data.type, prefix + "calculation_type");
  //DomUtils::makeNode(root, data.useBuur, "use_buur");
  //DomUtils::makeNode(root, data.buurType, "buur_type");
  DomUtils::makeNode(root, data.useXF, prefix + "use_extended_format");
  DomUtils::makeNode(root, data.description, prefix + "description");
  DomUtils::makeNode(root, data.name, prefix + "name");
  DomUtils::makeNode(root, data.directory, prefix + "directory");
  //DomUtils::makeNode(root, data.runType, "run_type");
  //DomUtils::makeNode(root, data.queue, "queue");
  
}

///////////////////////////////////////////////////////////////////////////////
///// Public Slots                                                        /////
///////////////////////////////////////////////////////////////////////////////

///// reset ///////////////////////////////////////////////////////////////////
void GlobalBase::reset()
/// Resets all widgets to their default values.
{  
  LineEditDesc->clear();
  
  if(!GroupBoxType->isEnabled())
    return; // when a calculation is running, onyl the description can be reset.

  ComboBoxType->setCurrentItem(SinglePointEnergy);
  //ComboBoxType->setCurrentItem(GeometryOptimization); //implementing RelaxBase
  
  CheckBoxBuur->setChecked(false); // should disable ComboBoxBuur
  ComboBoxBuur->setCurrentItem(NoBuur);
  CheckBoxXF->setChecked(true);

  LineEditName->setText(defaultName);
  LineEditDir->setText(QDir::convertSeparators(QDir::homeDirPath()) + QDir::separator() + defaultName);
  
  RadioButtonRun1->setChecked(true); // change this to buttongroup behaviour when implemented
  ComboBoxQueue->setCurrentItem(0);
}


///////////////////////////////////////////////////////////////////////////////
///// Protected Slots                                                     /////
///////////////////////////////////////////////////////////////////////////////

///// accept //////////////////////////////////////////////////////////////////
void GlobalBase::accept()
/// Overridden from GlobalWidget::accept().
{  
  ///// make the given directory absolute
  //QDir workDir = LineEditDir->text();
  //LineEditDir->setText(workDir.absPath());

  if(widgetChanged)
  {
    setChanged(false);
    saveWidgets();
    GlobalWidget::accept();
  }
  else
    GlobalWidget::reject();
}

///// reject //////////////////////////////////////////////////////////////////
void GlobalBase::reject()
/// Overriden from GlobalWidget::reject().
{  
  if(widgetChanged)
  {
    setChanged(false);
    restoreWidgets();
  }
  GlobalWidget::reject();
}


///////////////////////////////////////////////////////////////////////////////
///// Private Slots                                                       /////
///////////////////////////////////////////////////////////////////////////////

///// setChanged //////////////////////////////////////////////////////////////
void GlobalBase::setChanged(const bool state)
/// Sets the 'changed' property of the dialog.
/// Defaults to true if no argument is provided.
{
  widgetChanged = state;  
}
 
///// correctType /////////////////////////////////////////////////////////////
void GlobalBase::correctType(int index)
/// Corrects the calculation type, because only Single Point
/// Energy and Energy & Forces are implemented ATM.
{  
  if(index > 2)
    ComboBoxType->setCurrentItem(2);
}

///// chooseDir ///////////////////////////////////////////////////////////////
void GlobalBase::chooseDir()
/// Chooses a directory using a dialog.
{  
  QString newDir = QFileDialog::getExistingDirectory(LineEditDir->text(),this,0,tr("Choose a directory"));
  if(!newDir.isNull())
    LineEditDir->setText(QDir::convertSeparators(newDir)); 
}


///////////////////////////////////////////////////////////////////////////////
///// Private Member Functions                                            /////
///////////////////////////////////////////////////////////////////////////////

///// makeConnections /////////////////////////////////////////////////////////
void GlobalBase::makeConnections()
/// Sets up the permanent connections. Called once from the constructor.
{
  ///// connections for Buttons
  connect(ButtonOK, SIGNAL(clicked()), this, SLOT(accept()));
  connect(ButtonCancel, SIGNAL(clicked()), this, SLOT(reject()));
  connect(ButtonReset, SIGNAL(clicked()), this, SLOT(reset()));
  ///// connections for all options
  connect(CheckBoxBuur, SIGNAL(toggled(bool)), ComboBoxBuur, SLOT(setEnabled(bool)));
  connect(ToolButtonDir, SIGNAL(clicked()), this, SLOT(chooseDir()));
  connect(RadioButtonRun3, SIGNAL(toggled(bool)), ComboBoxQueue, SLOT(setEnabled(bool)));
  ///// connections for imcomplete implementation
  connect(ComboBoxType, SIGNAL(activated(int)), this, SLOT(correctType(int)));
  ///// connections for changes
  connect(ComboBoxType, SIGNAL(activated(int)), this, SLOT(setChanged()));
  connect(CheckBoxBuur, SIGNAL(clicked()), this, SLOT(setChanged()));
  connect(ComboBoxBuur, SIGNAL(activated(int)), this, SLOT(setChanged()));
  connect(CheckBoxXF, SIGNAL(clicked()), this, SLOT(setChanged()));
  connect(LineEditDesc, SIGNAL(textChanged(const QString&)), this, SLOT(setChanged()));
  connect(LineEditName, SIGNAL(textChanged(const QString&)), this, SLOT(setChanged()));
  connect(LineEditDir, SIGNAL(textChanged(const QString&)), this, SLOT(setChanged()));
  connect(ButtonGroupRunType, SIGNAL(clicked(int)), this, SLOT(setChanged()));
  connect(ComboBoxQueue, SIGNAL(activated(int)), this, SLOT(setChanged()));
}
  
///// init ////////////////////////////////////////////////////////////////////
void GlobalBase::init()
/// Initializes the dialog. Called once from the constructor.
{ 
  // validate strings that are sent to BRABO programs not understanding unicode
  LineEditName->setValidator(new Latin1Validator(this));
  // set default values
  reset();
  saveWidgets();
  widgetChanged = false;
}

///// saveWidgets /////////////////////////////////////////////////////////////
void GlobalBase::saveWidgets()
/// Saves the status of the widgets to a data struct.
{  
  data.type = ComboBoxType->currentItem();
  data.useBuur = CheckBoxBuur->isOn();
  data.buurType = ComboBoxBuur->currentItem();
  data.useXF = CheckBoxXF->isOn();

  data.description = LineEditDesc->text();
  data.name = LineEditName->text();
  data.directory = LineEditDir->text();

  // replace the following with a buttongroup implementation
  if(RadioButtonRun1->isChecked())
    data.runType = 0;
  if(RadioButtonRun2->isChecked())
    data.runType = 1;
  if(RadioButtonRun3->isChecked())
    data.runType = 3;
  data.queue = ComboBoxQueue->currentItem();
}

///// restoreWidgets //////////////////////////////////////////////////////////
void GlobalBase::restoreWidgets()
/// Restores the status of the widgets from the data struct.
{  
  ComboBoxType->setCurrentItem(data.type);
  CheckBoxBuur->setChecked(data.useBuur);
  ComboBoxBuur->setCurrentItem(data.buurType);
  CheckBoxXF->setChecked(data.useXF);

  LineEditDesc->setText(data.description);
  LineEditName->setText(data.name);
  LineEditDir->setText(data.directory);

  // replace the following with a buttongroup implementation
  switch(data.runType)
  {
    case 0: RadioButtonRun1->setChecked(true);
            break;
    case 1: RadioButtonRun2->setChecked(true);
            break;
    case 2: RadioButtonRun3->setChecked(true);
            break;
  }
  ComboBoxQueue->setCurrentItem(data.queue);
}

