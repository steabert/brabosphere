/***************************************************************************
                        brabobase.cpp  -  description
                             -------------------
    begin                : Sun Jul 28 2002
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
  \class BraboBase
  \brief Allows generating an input file for the program Brabo.

  A dialog is presented with all options for Brabo classified in different
  categories. Options for the Stockholder program are also available. 
  All data are stored in the class itself using the private struct WidgetData.
  This class is a subclass of BraboWidget, a class generated from brabowidget.ui.
*/
/// \file
/// Contains the implementation of the class BraboBase.

///// Header files ////////////////////////////////////////////////////////////

// C++ header files
#include <algorithm>
#include <cassert>
#include <cmath>  

// Qt header files
#include <qapplication.h>
#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qcombobox.h>
#include <qdom.h>
#include <qfiledialog.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qlistbox.h>
#include <qlistview.h>
#include <qmessagebox.h>
#include <qpixmap.h>
#include <qprocess.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qspinbox.h>
#include <qstring.h>
#include <qtable.h>
#include <qtextedit.h>
#include <qtoolbutton.h>
#include <qvalidator.h>
#include <qwidgetstack.h>

// Xbrabo header files
#include "atomset.h"
#include "basisset.h"
#include "brabobase.h"
#include "domutils.h"
#include "iconsets.h"
#include "paths.h"
#include "textviewwidget.h"
#include "utils.h"
#include "version.h"


///////////////////////////////////////////////////////////////////////////////
///// Public Member Functions                                             /////
///////////////////////////////////////////////////////////////////////////////

///// Constructor /////////////////////////////////////////////////////////////
BraboBase::BraboBase(AtomSet* atomset, QWidget* parent, const char* name, bool modal, WFlags fl) : BraboWidget(parent, name, modal, fl),
  atoms(atomset)
/// The default constructor.
{
  assert(atomset != 0);
  makeConnections();
  init();
}

///// Destructor //////////////////////////////////////////////////////////////
BraboBase::~BraboBase()
/// The default destructor.
{

}

///// setForces ///////////////////////////////////////////////////////////////
void BraboBase::setForces(const bool state)
/// Sets whether forces should be calculated and
/// updates the status of related widgets.
{  
  if(calcForces != state)
  {
    calcForces = state;
    if(calcForces)
    {
      ///// RMP2 is not available
      if(ComboBoxSCFMethod->currentItem() == 2)
        ComboBoxSCFMethod->setCurrentItem(0);
      ComboBoxSCFMethod->removeItem(2);
      ///// Enable forces
      GroupBoxFORC->setEnabled(true);
      CheckBoxDebugFORC->setEnabled(true);
      GroupBoxExtraFORC->setEnabled(true);      
    }
    else
    {
      ///// RMP2 is available
      ComboBoxSCFMethod->insertItem("RMP2");
      ///// Disable forces
      GroupBoxFORC->setEnabled(false);
      CheckBoxDebugFORC->setEnabled(false);
      GroupBoxExtraFORC->setEnabled(false);
    }
  }
}

///// setDescription //////////////////////////////////////////////////////////
void BraboBase::setDescription(const QString description)
/// Sets the description of the calculation.
{  
  calcDescription = description;
}

///// setName /////////////////////////////////////////////////////////////////
void BraboBase::setName(const QString name)
/// Sets the filename prefix of the calculation.
{  
  calcName = name;
}

///// setExtendedFormat ///////////////////////////////////////////////////////
void BraboBase::setExtendedFormat(const bool state)
/// Sets the format of the coordinates (normal/extended).
{  
  calcXF = state;
}

///// setPVMHosts /////////////////////////////////////////////////////////////
void BraboBase::setPVMHosts(const QStringList& hosts)
/// Sets the available PVM hosts.
{  
  ListBoxNOTH2->clear();
  ListBoxNOTH2->insertStringList(hosts);
}

///// method //////////////////////////////////////////////////////////////////
QString BraboBase::method() const
/// Returns the method in the form Type(Method)//Basisset.
{
  QString result = ComboBoxSCFMethod->currentText() + "(" + ComboBoxSCFType->currentText() + ")//";
  if(RadioButtonBAS1->isChecked())
  {
    ///// one basisset
    result += ComboBoxBAS1->currentText();
  }
  else
  {
    if(ListViewBAS->childCount() != 0)
      result += ListViewBAS->firstChild()->text(1);
  }
  return result;  
}

///// basissets //////////////////////////////////////////////////////////
QStringList BraboBase::basissets(bool& ok)
/// Checks whether every atom has a basisset assigned (status returned by \c ok)
/// and returns a list of basisset files. This list can then be checked by Calculation
/// as the check might have to be done remotely.
{
  assert(data.listAtoms.size() == data.listBasissets.size());

  vector<unsigned int> neededAtoms = atoms->usedAtomicNumbers();
  vector<unsigned int> missingAtoms;
  QStringList result;
  for(vector<unsigned int>::const_iterator it = neededAtoms.begin(); it != neededAtoms.end(); it++)
  {
    if(data.useOneBasisset)
      result += QString(Paths::basisset + QDir::separator() + Basisset::numToBasisDir(data.basisset1) + QDir::separator() + AtomSet::numToAtom(*it).stripWhiteSpace() + "." + Basisset::extension());
    else
    {
      ///// find the index into data.listAtoms equal to the value of *it
      vector<unsigned int>::iterator itBasissets = data.listBasissets.begin();
      vector<unsigned int>::iterator itAtoms;
      for(itAtoms = data.listAtoms.begin(); itAtoms != data.listAtoms.end(); itAtoms++, itBasissets++)
      {
        if(*it == *itAtoms)
          break;
      }
      if(itAtoms == data.listAtoms.end())
      {
        ///// the atom could not be found
        missingAtoms.push_back(*it);
      }
      else
      {
        ///// get the corresponding basisset file
        result += QString(Paths::basisset + QDir::separator() + Basisset::numToBasisDir(*itBasissets) + QDir::separator() + AtomSet::numToAtom(*it).stripWhiteSpace() + "." + Basisset::extension());
      }
    }
  }
  
  if(missingAtoms.empty())
    // all atoms have a basisset
    ok = true;
  else
  {
    // show an error with the missing atoms/basissets
    QString atomList;
    for(vector<unsigned int>::iterator it = missingAtoms.begin(); it != missingAtoms.end(); it++)
      atomList += QString(" " + AtomSet::numToAtom(*it));
    QMessageBox::warning(this, tr("Check basis sets") , tr("The following atom types do not have a basis set assigned:") + "\n" + atomList);
    ok = false;
  }
  return result;
}

///// startVector /////////////////////////////////////////////////////////////
QString BraboBase::startVector(bool& prefer, unsigned int& size1, unsigned int& size2)
/// Returns the starting vector file name. \c prefer is set to true if an existing
/// starting vector with the right size is preferred over the given startvector.
/// Possible correct sizes for the starting vector are returned in \c size1 and \c size2.
/// That is because filesizes are different under *nix and Windows (LF vs. CRLF).
/// The sizes are determined using the following formula:
/// size = NEL x (10 x NCF + ceil(NCF/8)) with NEL = the number of electrons
/// and NCF the number of contracted gaussian functions.
/// Maybe merge this function with basissets() into a calculationInput() function...
{
  ///// set the preference for the start vector
  prefer = data.preferStartvector;

  ///// determine NEL
  unsigned int nel = 0;
  for(unsigned int i = 0; i < atoms->count(); i++)
    nel += atoms->atomicNumber(i);
  nel += data.charge;
  nel /= 2;
  qDebug("number of electrons = %d", nel);

  ///// determine NCF
  vector<unsigned int> usedAtoms = atoms->usedAtomicNumbers();
  vector<unsigned int> ncfAtoms(usedAtoms.size());
  for(unsigned int i = 0; i < usedAtoms.size(); i++)
  {
    // get the basis set number for this atom type (very similar to the code of basissets()
    unsigned int basisNum = 0;
    if(data.useOneBasisset)
      basisNum = data.basisset1;
    else
    {
      vector<unsigned int>::iterator itBasissets = data.listBasissets.begin();
      vector<unsigned int>::iterator itAtoms;
      for(itAtoms = data.listAtoms.begin(); itAtoms != data.listAtoms.end(); itAtoms++, itBasissets++)
      {
        if(usedAtoms[i] == *itAtoms)
          break;
      }
      if(itAtoms == data.listAtoms.end())
      {
        ///// the atom could not be found
        // should never happen whem basissets was called previously, so no messagebox is shown
        size1 = 0;
        size2 = 0;
        return QString::null;
      }
      else
        ///// get the corresponding basisset file
        basisNum = *itBasissets;
    }
    
    // get the NCF for this atom type
    ncfAtoms[i] = Basisset::contractedFunctions(basisNum, usedAtoms[i]);
  }
  // sum all ncf's into NCF
  unsigned int ncf = 0;
  for(unsigned int i = 0; i < atoms->count(); i++)
  {
    // get the ncf for this atom
    vector<unsigned int>::iterator it1 = std::find(usedAtoms.begin(), usedAtoms.end(), atoms->atomicNumber(i)); // should always return something within range
    ncf += ncfAtoms[std::distance(usedAtoms.begin(), it1)]; 
    qDebug("ncf after atom %d is %d", i, ncf);
  }
  
  ///// determine the filesize
  size1 = nel * (10*ncf + static_cast<unsigned int>(ceil(ncf/8.0)));   // *nix
  size2 = nel * (10*ncf + 2*static_cast<unsigned int>(ceil(ncf/8.0))); // Windows
  qDebug("BraboBase::startVector: sizes are %d and %d",size1, size2);

  if(!data.useStartvector)
    return QString::null;

  return data.startvector;
}

///// maxIterations ///////////////////////////////////////////////////////////
unsigned int BraboBase::maxIterations() const
/// Returns the maximum number of SCF-iterations.
{
  if(data.numIter == -1)
    return 50;

  return static_cast<unsigned int>(data.numIter);
}

///// generateInput ///////////////////////////////////////////////////////////
QStringList BraboBase::generateInput(unsigned int program)
/// Generates an input file in a stringlist from the contents of the widgets.
/// Values for the argument \c program :
/// \arg \c BraboBase::BRABO : input for Brabo
/// \arg \c BraboBase::STOCK : input for Stock
{
  QStringList result, extra;
  
  result += "! " + tr("This file has been generated by") + " " + Version::appName + " " + Version::appVersion;
  result += "! " + tr("All changes made in this file will be lost!");

  switch(program)
  {
    case BRABO:
      ///// FXX section ///////////////////
      ///// F02 (coordinates)
      result += "F02=" + calcName + ".crd";
      ///// F04 (output)
      result += "F04=" + calcName + ".out";
      ///// F07 (punchfile)
      result += "F07=" + calcName + ".pun";
      ///// F11 (binary file)
      if(CheckBoxF11->isOn() || CheckBoxSTOCK->isOn())
      {
        if(Paths::bin.isNull())
          result += "F11=" + calcName + ".11";
        else
          result += "F11=" + Paths::bin + QDir::separator() + calcName + ".11";
      }
      ///// F99 (startvector) -> it's always written so save it 
      result += "F99=" + calcName + ".sta";
      
      ///// Main section //////////////////
      ///// MOLE
      result += "mole      " + calcName;
      ///// TEXT
      if(!calcDescription.isEmpty())
        result += "text      " + calcDescription;
      ///// DS=T
      if(CheckBoxDST->isChecked())
        result += "dst";
      ///// AC=D
      if(CheckBoxACD->isChecked())
        result += "acd";
      ///// Extra
      result += tableContents(TableMain);

      ///// PVM section ///////////////////
      if(CheckBoxPVM->isChecked())
      {
        ///// PVM
        result += "PVM";
        for(unsigned int i = 0; i < ListBoxNOTH1->count(); i++)
          result += "noth      " + ListBoxNOTH1->text(i);
        ///// NCST
        if(SpinBoxNCST->value() != 0)
          result += inputLine("ncst", SpinBoxNCST->value());
        ///// NTAS
        if(SpinBoxNTAS->value() != 0)
          result += inputLine("ntas", SpinBoxNTAS->value());
        ///// NOSE
        if(CheckBoxNOSE->isChecked())
          result += "nose";
        ///// NICE
        if(SpinBoxNICE->value() != -1)
          result += inputLine("nice", SpinBoxNICE->value());
        ///// PACK
        if(CheckBoxPACK->isChecked())
          result += "pack";
        ///// PRIN
        if(CheckBoxDebugPVM->isChecked())
          result += "prin";
      }
      ///// Extra
      result += tableContents(TablePVM);

      ///// INTE section //////////////////
      result += "INTE";
      switch (ComboBoxSCFType->currentItem())
      {
        case 0: ///// MIA
                result += "mia";
                break;
        case 1: ///// Direct
                break;
        case 2: ///// Conventional
                result += "twoe";
      }
      ///// ANGS and EXFI always present
      result += "angs";
      result += "exfi";
      ///// XFCR
      if(calcXF)
        result += "xfcr";
      ///// SYMM
      if(CheckBoxSYMM->isChecked())
      {
        if(RadioButtonSYMM1->isOn())
          result += "symm";
        else
          result += validatedSYMMLine(CheckBoxSYMMx, CheckBoxSYMMy, CheckBoxSYMMz, CheckBoxSYMMxy, CheckBoxSYMMxz, CheckBoxSYMMyz, CheckBoxSYMMxyz);
      }
      ///// THRE
      { 
        double threINTE = - log10(LineEditThreINTEa->text().toDouble()*pow(10.0, - LineEditThreINTEb->text().toDouble()));
        if(fabs(threINTE - 8.0) > 0.00001)
          result += inputLine("thre", threINTE);
      }  
      ///// SKPC
      if(CheckBoxSKPC->isChecked())
        result += "skpc";
      ///// GEOP
      if(CheckBoxGEOP->isChecked())
        result += "geop";
      ///// GOON
      if(CheckBoxGoonINTE->isChecked())
        result += "goon";
      ///// NUCL
      result += parsedNUCLlines();
      ///// PRIN
      if(CheckBoxIntINTE->isChecked())
        result += "prin";
      ///// LONG
      if(CheckBoxShellsINTE->isChecked())
        result += "long";      
      ///// XFBA always present
      result += "xfba";
      ///// Basisset (end of INTE section)
      if(RadioButtonBAS1->isChecked())
      {
        QString tempstring = Paths::basisset + QDir::separator() + Basisset::numToBasisDir(ComboBoxBAS1->currentItem());
        result += QString("gbas      %1%2").arg(Basisset::extension(),-10).arg(tempstring);
      }
      else
      {
        QString tempstring;
        QListViewItemIterator it(ListViewBAS);
        for( ; it.current(); ++it)
        {
          tempstring = it.current()->text(0);
          result += "abas      " + tempstring + "        " + Paths::basisset + QDir::separator() + Basisset::numToBasisDir(Basisset::basisToNum(it.current()->text(1))) + QDir::separator() + tempstring.stripWhiteSpace() + "." + Basisset::extension();
        }
      }
      ///// Extra
      result += tableContents(TableINTE);

      ///// SCF section ///////////////////
      result += "SCF";
      switch (ComboBoxSCFMethod->currentItem())
      {
        case 0: ///// RHF
                break;
        case 1: ///// UHF
                result += "uhf";
                break;
        case 2: ///// RMP2            
                if(RadioButtonMP2E1->isOn())
                  result += inputLine("mp2e", 0);
                else if(RadioButtonMP2E2->isOn())
                  result += inputLine("mp2e", -1);
                else
                  result += inputLine("mp2e", SpinBoxMP2E1->value(), SpinBoxMP2E2->value());
                break;
        /*
        case 3: // UMP2
                result += "uhf";
                result += "mp2e";
                break;
        case 4: // RCIS
                result += "cis";
                break;
        case 5: // UCIS
                result += "uhf";
                result += "cis";
                break;
        */
      }
      ///// DRCT
      if(ComboBoxSCFType->currentItem() == 1)
        result += "drct";
      ///// IPOL
      result += inputLine("ipol", LineEditIPOL->text().toDouble());
      ///// STAR -> always add. it will be taken out by the calculation for the first step of an optimization
      result += inputLine("star",0,0,99);
      ///// CHAR
      if(SpinBoxCHAR->value() != 0)
        result += inputLine("char", SpinBoxCHAR->value());
      ///// MP2D
      if((ComboBoxSCFMethod->currentItem() == 2) && CheckBoxMP2D->isChecked())
      {
        if(RadioButtonMP2E1->isOn())
          result += inputLine("mp2d", 0);
        else if(RadioButtonMP2E2->isOn())
          result += inputLine("mp2d", -1);
        else
          result += inputLine("mp2d", SpinBoxMP2E1->value(), SpinBoxMP2E2->value());
      }
      ///// SCRF
      if(CheckBoxSCRF->isChecked())
        result += inputLine("scrf", LineEditSCRF1->text().toDouble(), LineEditSCRF2->text().toDouble());
      ///// FIEL
      if(CheckBoxFIEL->isChecked())
        result += inputLine("fiel", LineEditFIELx->text().toDouble(), LineEditFIELy->text().toDouble(), LineEditFIELz->text().toDouble()); 
      ///// ELMO
      if(CheckBoxELMO->isChecked())
        result += "elmo";           
      ///// MULL and MUL1
      if(CheckBoxMULL1->isChecked())
      {
        if(CheckBoxMULL2->isChecked())
          result += inputLine("mull", 1);
        else
          result += "mull";
        if(CheckBoxMULL3->isChecked())
        {
          if(CheckBoxMULL2->isChecked())
            result += inputLine("mul1", 1);
          else
            result += "mul1";
        }
      }
      ///// LOCA
      if(CheckBoxLOCA->isChecked())
        result += inputLine("loca", SpinBoxLOCA->value(), LineEditLOCA->text().toDouble());
      ///// EXIT
      if(CheckBoxEXIT->isChecked())
        result += "exit";
      ///// JAB
      if(CheckBoxJAB->isChecked())
        result += "jab";
      ///// ITER
      if(SpinBoxITER->value() != -1)
        result += inputLine("iter", SpinBoxITER->value());
      ///// THRE
      {
        QString a, c;
        bool addit;
        double threSCF = - log10(LineEditThreSCF1a->text().toDouble()*pow(10.0, - LineEditThreSCF1b->text().toDouble()));
        if(fabs(threSCF - 8.0) > 0.00001)
        {
          a.setNum(threSCF,'f',10).truncate(10);
          addit = true;
        }
        else
        {
          a = "          ";
          addit = false;
        }        
        threSCF = - log10(LineEditThreSCF2a->text().toDouble()*pow(10.0, - LineEditThreSCF2b->text().toDouble()));
        if(fabs(threSCF + log10(3.0*pow(10.0, -6.0))) > 0.00001)
        {
          c.setNum(threSCF,'f',10).truncate(10);
          addit = true;
        }
        else
          c = "          ";
        if(addit)
          result += "thre      " + a + "          " + c;
      }     
      ///// STHR
      { 
        double threSTHR = - log10(LineEditSthrSCFa->text().toDouble() * pow(10.0, - LineEditSthrSCFb->text().toDouble()));
        if(fabs(threSTHR - 4.0) > 0.00001)
          result += inputLine("sthr",threSTHR);
      }     
      ///// VTHR
      if(CheckBoxVTHR->isChecked())
        result += "vthr";
      ///// DIIS and MAXD
      if(CheckBoxDIIS->isChecked())
        result += inputLine("diis",-1);
      else
      {
        qDebug("generateInput: MAXD = %d",SpinBoxDIIS->value());
        if(fabs(LineEditDIIS->text().toDouble() - 0.03) > 0.00001)
          result += inputLine("diis", LineEditDIIS->text().toDouble());
        if(SpinBoxDIIS->value() != -1)
          result +=inputLine("maxd", SpinBoxDIIS->value());
      }
      ///// GOON
      if(CheckBoxGoonSCF->isChecked())
        result += "goon";
      ///// LVSH and DLVS
      if(fabs(LineEditLVSH->text().toDouble()) > 0.00001)
      {
        if(CheckBoxDLVS->isChecked())
          result += inputLine("dlvs", LineEditLVSH->text().toDouble());
        else
          result +=inputLine("lvsh", LineEditLVSH->text().toDouble());
      }
      ///// PRWF
      if(CheckBoxPRWF->isChecked())
        result += "prwf";
      ///// PRDM
      if(CheckBoxPRDM->isChecked())
        result += "prdm";    
      ///// JACO
      if(CheckBoxJACO->isChecked())
        result += "jaco";
      ///// PRIN
      {
        int b = 0;
        if(CheckBoxPrintSCF3->isChecked())
          b = 3;
        else if(CheckBoxPrintSCF2->isChecked())
          b = 2;
        else if(CheckBoxPrintSCF1->isChecked())
          b = 1;
        if(b != 0)
          result += inputLine("prin", SpinBoxPrintSCF->value(), b);
      }      
      ///// PUNC
      result += "punc";
      ///// Extra
      result += tableContents(TableSCF);

      ///// FORC section //////////////////
      if(calcForces)
      {
        result += "FORC";
        ///// PUNC
        result += "punc";
        ///// XFFO
        if(calcXF)
          result += "xffo";      
        ///// FATO
        if(CheckBoxFATO->isChecked())
          result += inputLine("fato", SpinBoxFATO->value());
        ///// MIA
        if(CheckBoxMIAForc->isChecked())
          result += "mia";
        ///// PRIN  
        if(CheckBoxDebugFORC->isChecked())
          result += "prin";
      }
      ///// Extra
      result += tableContents(TableFORC);

      ///// End ///////////////////////////
      result += "STOP";
      break;

    case STOCK:
      if(!CheckBoxSTOCK->isChecked())
        return QStringList();

      ///// MOLE
      result += "mole      " + calcName;
      ///// F11=
      if(!Paths::bin.isEmpty())
        result += "F11=" + Paths::bin + QDir::separator() + calcName + ".11";
      ///// ATDE
      result += "atde      " + calcName + ".atdens";
      ///// TEXT
      if(!calcDescription.isEmpty())
        result += "text      " + calcDescription;
      ///// HOMD
      if(RadioButtonStockDENS2->isChecked())
        result += "homd";
      ///// LUMD
      else if(RadioButtonStockDENS3->isChecked())
        result += "lumd";
      ///// TRDE
      else if(RadioButtonStockDENS4->isChecked())
        result += inputLine("trde", SpinBoxStockTRDE1->value(), SpinBoxStockTRDE2->value());
      ///// UHF
      if(ComboBoxSCFMethod->currentItem() == 1)
      {
        if(RadioButtonStockDENS5->isChecked())
          result += "uhf      1.";
        else
          result += "uhf";
      }
      //// MOLD
      if(ComboBoxStockMOLD->currentItem() == 1)
        result += "mold";
      //// ELMO
      if(CheckBoxStockELMO->isChecked())
        result += "elmo";
      //// EPAR
      if(CheckBoxStockEPAR->isChecked())
        result += "epar";

      //// End ////////////////////////
      result += "STOP";
      break;
  }
  
  return result;
}

///// generateAtdens //////////////////////////////////////////////////////////
QString BraboBase::generateAtdens(bool& ok)
/// Returns a custom built atdens file for running STOCK for the provided atoms.
/// It works similarly to basissets(). It does not check for missing basis set
/// assignment, which should be handled by basissets(). It still quietly returns
/// false in that case. If the result is an empty string, Stock should not be run
/// (similarly to generateInput with BraboBase::STOCK as argument).
{
  if(!data.useStock)
    return QString::null;

  assert(data.listAtoms.size() == data.listBasissets.size());

  QStringList missingFiles;
  QString result;
  vector<unsigned int> neededAtoms = atoms->usedAtomicNumbers();
  for(vector<unsigned int>::const_iterator it = neededAtoms.begin(); it != neededAtoms.end(); it++)
  {
    QFile source;
    if(data.useOneBasisset)
      source.setName(Paths::basisset + QDir::separator() + Basisset::numToBasisDir(data.basisset1) + QDir::separator() + AtomSet::numToAtom(*it).stripWhiteSpace() + ".atdens");
    else
    {
      ///// find the index into data.listAtoms equal to the value of *it
      vector<unsigned int>::const_iterator itBasissets = data.listBasissets.begin();
      for(vector<unsigned int>::const_iterator itAtoms = data.listAtoms.begin(); itAtoms != data.listAtoms.end(); itAtoms++, itBasissets++)
      {
        if(*it == *itAtoms)
          break;
      }
      if(itBasissets != data.listBasissets.end())
        source.setName(Paths::basisset + QDir::separator() + Basisset::numToBasisDir(*itBasissets) + QDir::separator() + AtomSet::numToAtom(*it).stripWhiteSpace() + ".atdens");
      else
      {
        ok = false;
        return QString::null;
      }
    }
    ///// check whether the atdens file exists
    if(!source.exists() || !source.open(IO_ReadOnly))
      missingFiles += source.name();
    else
    {
      // add the contents of the file to the result;
      QTextStream stream(&source);
      result += stream.read();
    }
  }
  
  if(missingFiles.isEmpty())
    // all atoms have a basisset
    ok = true;
  else
  {
    // show an error with the missing basissets
    QMessageBox::warning(this, tr("Building a custom atomic density basis set file") , tr("The following atomic density basis set files could not be found") + ":\n" + missingFiles.join("\n"));
    ok = false;
    result = QString::null;
  }
  return result;
}

///// loadCML /////////////////////////////////////////////////////////////////
void BraboBase::loadCML(const QDomElement* root)
/// Reads the widgetdata from a QDomElement.
{ 
  ///// This function will normally never be called while the dialog is showing
  ///// => the data struct is always up to date with the contents of the widgets
  assert(!isVisible());

  const QString prefix = "energy_and_forces_";
  QDomNode childNode = root->firstChild();
  while(!childNode.isNull())
  {
    if(childNode.isElement() && childNode.toElement().tagName() == "parameter")
    {
      ///// Basic Method  
      if(DomUtils::dictEntry(childNode, prefix + xml.SCFMethod))
        DomUtils::readNode(&childNode, &data.SCFMethod);
      else if(DomUtils::dictEntry(childNode, prefix + xml.SCFType))
        DomUtils::readNode(&childNode, &data.SCFType);
      else if(DomUtils::dictEntry(childNode, prefix + xml.useStartvector))
        DomUtils::readNode(&childNode, &data.useStartvector);
      else if(DomUtils::dictEntry(childNode, prefix + xml.startvector))
        DomUtils::readNode(&childNode, &data.startvector);
      else if(DomUtils::dictEntry(childNode, prefix + xml.preferStartvector))
        DomUtils::readNode(&childNode, &data.preferStartvector);
      else if(DomUtils::dictEntry(childNode, prefix + xml.charge))
        DomUtils::readNode(&childNode, &data.charge);

      ///// Basic Basis Set
      else if(DomUtils::dictEntry(childNode, prefix + xml.useOneBasisset))
        DomUtils::readNode(&childNode, &data.useOneBasisset);
      else if(DomUtils::dictEntry(childNode, prefix + xml.basisset1))
        DomUtils::readNode(&childNode, &data.basisset1);
      else if(DomUtils::dictEntry(childNode, prefix + xml.basissetAtom))
        DomUtils::readNode(&childNode, &data.basissetAtom);
      else if(DomUtils::dictEntry(childNode, prefix + xml.basisset2))
        DomUtils::readNode(&childNode, &data.basisset2);

      else if(DomUtils::dictEntry(childNode, prefix + xml.listAtoms))
        DomUtils::readNode(&childNode, &data.listAtoms);
      else if(DomUtils::dictEntry(childNode, prefix + xml.listBasissets))
        DomUtils::readNode(&childNode, &data.listBasissets);

      ///// Advanced Method
      else if(DomUtils::dictEntry(childNode, prefix + xml.MP2Type))
        DomUtils::readNode(&childNode, &data.MP2Type);
      else if(DomUtils::dictEntry(childNode, prefix + xml.MP2ExcludeOcc))
        DomUtils::readNode(&childNode, &data.MP2ExcludeOcc);
      else if(DomUtils::dictEntry(childNode, prefix + xml.MP2ExcludeVirt))
        DomUtils::readNode(&childNode, &data.MP2ExcludeVirt);
      else if(DomUtils::dictEntry(childNode, prefix + xml.MP2Density))
        DomUtils::readNode(&childNode, &data.MP2Density);
      else if(DomUtils::dictEntry(childNode, prefix + xml.useField))
        DomUtils::readNode(&childNode, &data.useField);
      else if(DomUtils::dictEntry(childNode, prefix + xml.fieldX))
        DomUtils::readNode(&childNode, &data.fieldX);
      else if(DomUtils::dictEntry(childNode, prefix + xml.fieldY))
        DomUtils::readNode(&childNode, &data.fieldY);
      else if(DomUtils::dictEntry(childNode, prefix + xml.fieldZ))
        DomUtils::readNode(&childNode, &data.fieldZ);
      else if(DomUtils::dictEntry(childNode, prefix + xml.useSCRF))
        DomUtils::readNode(&childNode, &data.useSCRF);
      else if(DomUtils::dictEntry(childNode, prefix + xml.SCRFEpsilon))
        DomUtils::readNode(&childNode, &data.SCRFEpsilon);
      else if(DomUtils::dictEntry(childNode, prefix + xml.SCRFRadius))
        DomUtils::readNode(&childNode, &data.SCRFRadius);

      ///// Advanced Symmetry
      else if(DomUtils::dictEntry(childNode, prefix + xml.useSymmetry))
        DomUtils::readNode(&childNode, &data.useSymmetry);
      else if(DomUtils::dictEntry(childNode, prefix + xml.useSymmAuto))
        DomUtils::readNode(&childNode, &data.useSymmAuto);
      else if(DomUtils::dictEntry(childNode, prefix + xml.useSymmX))
        DomUtils::readNode(&childNode, &data.useSymmX);
      else if(DomUtils::dictEntry(childNode, prefix + xml.useSymmY))
        DomUtils::readNode(&childNode, &data.useSymmY);
      else if(DomUtils::dictEntry(childNode, prefix + xml.useSymmZ))
        DomUtils::readNode(&childNode, &data.useSymmZ);
      else if(DomUtils::dictEntry(childNode, prefix + xml.useSymmXY))
        DomUtils::readNode(&childNode, &data.useSymmXY);
      else if(DomUtils::dictEntry(childNode, prefix + xml.useSymmYZ))
        DomUtils::readNode(&childNode, &data.useSymmYZ);
      else if(DomUtils::dictEntry(childNode, prefix + xml.useSymmXZ))
        DomUtils::readNode(&childNode, &data.useSymmXZ);
      else if(DomUtils::dictEntry(childNode, prefix + xml.useSymmXYZ))
        DomUtils::readNode(&childNode, &data.useSymmXYZ);

      ///// Advanced Other
      else if(DomUtils::dictEntry(childNode, prefix + xml.valueIpol))
        DomUtils::readNode(&childNode, &data.valueIpol);
      else if(DomUtils::dictEntry(childNode, prefix + xml.useSkpc))
        DomUtils::readNode(&childNode, &data.useSkpc);
      else if(DomUtils::dictEntry(childNode, prefix + xml.printGeop))
        DomUtils::readNode(&childNode, &data.printGeop);
      else if(DomUtils::dictEntry(childNode, prefix + xml.printWF))
        DomUtils::readNode(&childNode, &data.printWF);
      else if(DomUtils::dictEntry(childNode, prefix + xml.printDM))
        DomUtils::readNode(&childNode, &data.printDM);
      else if(DomUtils::dictEntry(childNode, prefix + xml.useDST))
        DomUtils::readNode(&childNode, &data.useDST);
      else if(DomUtils::dictEntry(childNode, prefix + xml.useACD))
        DomUtils::readNode(&childNode, &data.useACD);
      else if(DomUtils::dictEntry(childNode, prefix + xml.saveF11))
        DomUtils::readNode(&childNode, &data.saveF11);
      else if(DomUtils::dictEntry(childNode, prefix + xml.useFato))
        DomUtils::readNode(&childNode, &data.useFato);
      else if(DomUtils::dictEntry(childNode, prefix + xml.numFato))
        DomUtils::readNode(&childNode, &data.numFato);
      else if(DomUtils::dictEntry(childNode, prefix + xml.useMIAForce))
        DomUtils::readNode(&childNode, &data.useMIAForce);

      ///// Properties Charges
      else if(DomUtils::dictEntry(childNode, prefix + xml.useMulliken))
        DomUtils::readNode(&childNode, &data.useMulliken);
      else if(DomUtils::dictEntry(childNode, prefix + xml.MullikenNoOverlap))
        DomUtils::readNode(&childNode, &data.MullikenNoOverlap);
      else if(DomUtils::dictEntry(childNode, prefix + xml.MullikenEachIter))
        DomUtils::readNode(&childNode, &data.MullikenEachIter);
      else if(DomUtils::dictEntry(childNode, prefix + xml.useStock))
        DomUtils::readNode(&childNode, &data.useStock);
      else if(DomUtils::dictEntry(childNode, prefix + xml.stockType))
        DomUtils::readNode(&childNode, &data.stockType);
      else if(DomUtils::dictEntry(childNode, prefix + xml.stockTotalDensity))
        DomUtils::readNode(&childNode, &data.stockTotalDensity);
      else if(DomUtils::dictEntry(childNode, prefix + xml.stockTransition1))
        DomUtils::readNode(&childNode, &data.stockTransition1);
      else if(DomUtils::dictEntry(childNode, prefix + xml.stockTransition2))
        DomUtils::readNode(&childNode, &data.stockTransition2);
      else if(DomUtils::dictEntry(childNode, prefix + xml.useStockElmo))
        DomUtils::readNode(&childNode, &data.useStockElmo);
      else if(DomUtils::dictEntry(childNode, prefix + xml.useStockEpar))
        DomUtils::readNode(&childNode, &data.useStockEpar);

      ///// Properties Other
      else if(DomUtils::dictEntry(childNode, prefix + xml.useBoys))
        DomUtils::readNode(&childNode, &data.useBoys);
      else if(DomUtils::dictEntry(childNode, prefix + xml.numBoysIter))
        DomUtils::readNode(&childNode, &data.numBoysIter);
      else if(DomUtils::dictEntry(childNode, prefix + xml.BoysThreshold))
        DomUtils::readNode(&childNode, &data.BoysThreshold);
      else if(DomUtils::dictEntry(childNode, prefix + xml.listNucl))
        DomUtils::readNode(&childNode, &data.listNucl);
      else if(DomUtils::dictEntry(childNode, prefix + xml.useElmo))
        DomUtils::readNode(&childNode, &data.useElmo);
      else if(DomUtils::dictEntry(childNode, prefix + xml.useExit))
        DomUtils::readNode(&childNode, &data.useExit);
      else if(DomUtils::dictEntry(childNode, prefix + xml.useJab))
        DomUtils::readNode(&childNode, &data.useJab);

      ///// SCF Convergence
      else if(DomUtils::dictEntry(childNode, prefix + xml.numIter))
        DomUtils::readNode(&childNode, &data.numIter);
      else if(DomUtils::dictEntry(childNode, prefix + xml.useJacobi))
        DomUtils::readNode(&childNode, &data.useJacobi);
      else if(DomUtils::dictEntry(childNode, prefix + xml.useDIIS))
        DomUtils::readNode(&childNode, &data.useDIIS);
      else if(DomUtils::dictEntry(childNode, prefix + xml.DIISThre))
        DomUtils::readNode(&childNode, &data.DIISThre);
      else if(DomUtils::dictEntry(childNode, prefix + xml.numDIISCoeff))
        DomUtils::readNode(&childNode, &data.numDIISCoeff);
      else if(DomUtils::dictEntry(childNode, prefix + xml.valueLvsh))
        DomUtils::readNode(&childNode, &data.valueLvsh);
      else if(DomUtils::dictEntry(childNode, prefix + xml.useDlvs))
        DomUtils::readNode(&childNode, &data.useDlvs);
      else if(DomUtils::dictEntry(childNode, prefix + xml.thresholdINTEa))
        DomUtils::readNode(&childNode, &data.thresholdINTEa);
      else if(DomUtils::dictEntry(childNode, prefix + xml.thresholdINTEb))
        DomUtils::readNode(&childNode, &data.thresholdINTEb);
      else if(DomUtils::dictEntry(childNode, prefix + xml.thresholdMIAa))
        DomUtils::readNode(&childNode, &data.thresholdMIAa);
      else if(DomUtils::dictEntry(childNode, prefix + xml.thresholdMIAb))
        DomUtils::readNode(&childNode, &data.thresholdMIAb);
      else if(DomUtils::dictEntry(childNode, prefix + xml.thresholdSCFa))
        DomUtils::readNode(&childNode, &data.thresholdSCFa);
      else if(DomUtils::dictEntry(childNode, prefix + xml.thresholdSCFb))
        DomUtils::readNode(&childNode, &data.thresholdSCFb);
      else if(DomUtils::dictEntry(childNode, prefix + xml.thresholdOverlapa))
        DomUtils::readNode(&childNode, &data.thresholdOverlapa);
      else if(DomUtils::dictEntry(childNode, prefix + xml.thresholdOverlapb))
        DomUtils::readNode(&childNode, &data.thresholdOverlapb);
      else if(DomUtils::dictEntry(childNode, prefix + xml.useVarThreMIA))
        DomUtils::readNode(&childNode, &data.useVarThreMIA);

      ///// PVM
      else if(DomUtils::dictEntry(childNode, prefix + xml.usePVM))
        DomUtils::readNode(&childNode, &data.usePVM);
      else if(DomUtils::dictEntry(childNode, prefix + xml.PVMNumShells))
        DomUtils::readNode(&childNode, &data.PVMNumShells);
      else if(DomUtils::dictEntry(childNode, prefix + xml.PVMNumTasks))
        DomUtils::readNode(&childNode, &data.PVMNumTasks);
      else if(DomUtils::dictEntry(childNode, prefix + xml.notOnMaster))
        DomUtils::readNode(&childNode, &data.notOnMaster);
      else if(DomUtils::dictEntry(childNode, prefix + xml.valueNice))
        DomUtils::readNode(&childNode, &data.valueNice);
      else if(DomUtils::dictEntry(childNode, prefix + xml.usePacked))
        DomUtils::readNode(&childNode, &data.usePacked);

      else if(DomUtils::dictEntry(childNode, prefix + xml.PVMExcludeHosts))
        DomUtils::readNode(&childNode, &data.PVMExcludeHosts);

      ///// Debug
      else if(DomUtils::dictEntry(childNode, prefix + xml.printDebugPVM))
        DomUtils::readNode(&childNode, &data.printDebugPVM);
      else if(DomUtils::dictEntry(childNode, prefix + xml.printDebugFORC))
        DomUtils::readNode(&childNode, &data.printDebugFORC);
      else if(DomUtils::dictEntry(childNode, prefix + xml.goonINTE))
        DomUtils::readNode(&childNode, &data.goonINTE);
      else if(DomUtils::dictEntry(childNode, prefix + xml.goonSCF))
        DomUtils::readNode(&childNode, &data.goonSCF);
      else if(DomUtils::dictEntry(childNode, prefix + xml.printIntegrals))
        DomUtils::readNode(&childNode, &data.printIntegrals);
      else if(DomUtils::dictEntry(childNode, prefix + xml.printShells))
        DomUtils::readNode(&childNode, &data.printShells);
      else if(DomUtils::dictEntry(childNode, prefix + xml.levelPrintSCF))
        DomUtils::readNode(&childNode, &data.levelPrintSCF);
      else if(DomUtils::dictEntry(childNode, prefix + xml.numVirtual))
        DomUtils::readNode(&childNode, &data.numVirtual);

      ///// Extra
      else if(DomUtils::dictEntry(childNode, prefix + xml.numLinesExtraMain))
        DomUtils::readNode(&childNode, &data.numLinesExtraMain);
      else if(DomUtils::dictEntry(childNode, prefix + xml.numLinesExtraPVM))
        DomUtils::readNode(&childNode, &data.numLinesExtraPVM);
      else if(DomUtils::dictEntry(childNode, prefix + xml.numLinesExtraINTE))
        DomUtils::readNode(&childNode, &data.numLinesExtraINTE);
      else if(DomUtils::dictEntry(childNode, prefix + xml.numLinesExtraSCF))
        DomUtils::readNode(&childNode, &data.numLinesExtraSCF);
      else if(DomUtils::dictEntry(childNode, prefix + xml.numLinesExtraFORC))
        DomUtils::readNode(&childNode, &data.numLinesExtraFORC);       

      else if(DomUtils::dictEntry(childNode, prefix + xml.hPosExtraMain))
        DomUtils::readNode(&childNode, &data.hPosExtraMain);
      else if(DomUtils::dictEntry(childNode, prefix + xml.hPosExtraPVM))
        DomUtils::readNode(&childNode, &data.hPosExtraPVM);
      else if(DomUtils::dictEntry(childNode, prefix + xml.hPosExtraINTE))
        DomUtils::readNode(&childNode, &data.hPosExtraINTE);
      else if(DomUtils::dictEntry(childNode, prefix + xml.hPosExtraSCF))
        DomUtils::readNode(&childNode, &data.hPosExtraSCF);
      else if(DomUtils::dictEntry(childNode, prefix + xml.hPosExtraFORC))
        DomUtils::readNode(&childNode, &data.hPosExtraFORC);
      else if(DomUtils::dictEntry(childNode, prefix + xml.vPosExtraMain))
        DomUtils::readNode(&childNode, &data.vPosExtraMain);
      else if(DomUtils::dictEntry(childNode, prefix + xml.vPosExtraPVM))
        DomUtils::readNode(&childNode, &data.vPosExtraPVM);
      else if(DomUtils::dictEntry(childNode, prefix + xml.vPosExtraINTE))
        DomUtils::readNode(&childNode, &data.vPosExtraINTE);
      else if(DomUtils::dictEntry(childNode, prefix + xml.vPosExtraSCF))
        DomUtils::readNode(&childNode, &data.vPosExtraSCF);
      else if(DomUtils::dictEntry(childNode, prefix + xml.vPosExtraFORC))
        DomUtils::readNode(&childNode, &data.vPosExtraFORC);
      else if(DomUtils::dictEntry(childNode, prefix + xml.contentsExtraMain))
          DomUtils::readNode(&childNode, &data.contentsExtraMain);
      else if(DomUtils::dictEntry(childNode, prefix + xml.contentsExtraPVM))
          DomUtils::readNode(&childNode, &data.contentsExtraPVM);
      else if(DomUtils::dictEntry(childNode, prefix + xml.contentsExtraINTE))
          DomUtils::readNode(&childNode, &data.contentsExtraINTE);
      else if(DomUtils::dictEntry(childNode, prefix + xml.contentsExtraSCF))
          DomUtils::readNode(&childNode, &data.contentsExtraSCF);
      else if(DomUtils::dictEntry(childNode, prefix + xml.contentsExtraFORC))
          DomUtils::readNode(&childNode, &data.contentsExtraFORC);      
    }
    childNode = childNode.nextSibling();
  }
  restoreWidgets();
  widgetChanged = false;
}

///// saveCML /////////////////////////////////////////////////////////////////
void BraboBase::saveCML(QDomElement* root)
/// Saves the widgetdata to a QDomElement.
{  
  ///// This function will normally never be called while the dialog is showing
  ///// => the data struct is always up to date with the contents of the widgets
  assert(!isVisible());
  
  const QString prefix = "energy_and_forces_";
  ///// Basic Method
  DomUtils::makeNode(root, data.SCFMethod, prefix +  xml.SCFMethod);
  DomUtils::makeNode(root, data.SCFType, prefix +  xml.SCFType);
  DomUtils::makeNode(root, data.useStartvector, prefix +  xml.useStartvector);
  DomUtils::makeNode(root, data.startvector, prefix +  xml.startvector);
  DomUtils::makeNode(root, data.preferStartvector, prefix +  xml.preferStartvector);
  DomUtils::makeNode(root, data.charge, prefix +  xml.charge);

  ///// Basic Basis Set
  DomUtils::makeNode(root, data.useOneBasisset, prefix +  xml.useOneBasisset);
  DomUtils::makeNode(root, data.basisset1, prefix +  xml.basisset1);
  DomUtils::makeNode(root, data.basissetAtom, prefix +  xml.basissetAtom);
  DomUtils::makeNode(root, data.basisset2, prefix +  xml.basisset2);
  DomUtils::makeNode(root, data.listAtoms, prefix +  xml.listAtoms);
  DomUtils::makeNode(root, data.listBasissets, prefix +  xml.listBasissets);

  ///// Advanced Method
  DomUtils::makeNode(root, data.MP2Type, prefix +  xml.MP2Type);
  DomUtils::makeNode(root, data.MP2ExcludeOcc, prefix +  xml.MP2ExcludeOcc);
  DomUtils::makeNode(root, data.MP2ExcludeVirt, prefix +  xml.MP2ExcludeVirt);
  DomUtils::makeNode(root, data.MP2Density, prefix +  xml.MP2Density);
  DomUtils::makeNode(root, data.useField, prefix +  xml.useField);
  DomUtils::makeNode(root, data.fieldX, prefix +  xml.fieldX);
  DomUtils::makeNode(root, data.fieldY, prefix +  xml.fieldY);
  DomUtils::makeNode(root, data.fieldZ, prefix +  xml.fieldZ);
  DomUtils::makeNode(root, data.useSCRF, prefix +  xml.useSCRF);
  DomUtils::makeNode(root, data.SCRFEpsilon, prefix +  xml.SCRFEpsilon);
  DomUtils::makeNode(root, data.SCRFRadius, prefix +  xml.SCRFRadius);

  ///// Advanced Symmetry
  DomUtils::makeNode(root, data.useSymmetry, prefix +  xml.useSymmetry);
  DomUtils::makeNode(root, data.useSymmAuto, prefix +  xml.useSymmAuto);
  DomUtils::makeNode(root, data.useSymmX, prefix +  xml.useSymmX);
  DomUtils::makeNode(root, data.useSymmY, prefix +  xml.useSymmY);
  DomUtils::makeNode(root, data.useSymmZ, prefix +  xml.useSymmZ);
  DomUtils::makeNode(root, data.useSymmXY, prefix +  xml.useSymmXY);
  DomUtils::makeNode(root, data.useSymmYZ, prefix +  xml.useSymmYZ);
  DomUtils::makeNode(root, data.useSymmXZ, prefix +  xml.useSymmXZ);
  DomUtils::makeNode(root, data.useSymmXYZ, prefix +  xml.useSymmXYZ);

  ///// Advanced Other
  DomUtils::makeNode(root, data.valueIpol, prefix +  xml.valueIpol);
  DomUtils::makeNode(root, data.useSkpc, prefix +  xml.useSkpc);
  DomUtils::makeNode(root, data.printGeop, prefix +  xml.printGeop);
  DomUtils::makeNode(root, data.printWF, prefix +  xml.printWF);
  DomUtils::makeNode(root, data.printDM, prefix +  xml.printDM);
  DomUtils::makeNode(root, data.useDST, prefix +  xml.useDST);
  DomUtils::makeNode(root, data.useACD, prefix +  xml.useACD);
  DomUtils::makeNode(root, data.saveF11, prefix +  xml.saveF11);
  DomUtils::makeNode(root, data.useFato, prefix +  xml.useFato);
  DomUtils::makeNode(root, data.numFato, prefix +  xml.numFato);
  DomUtils::makeNode(root, data.useMIAForce, prefix +  xml.useMIAForce);

  ///// Properties Charges
  DomUtils::makeNode(root, data.useMulliken, prefix +  xml.useMulliken);
  DomUtils::makeNode(root, data.MullikenNoOverlap, prefix +  xml.MullikenNoOverlap);
  DomUtils::makeNode(root, data.MullikenEachIter, prefix +  xml.MullikenEachIter);
  DomUtils::makeNode(root, data.useStock, prefix +  xml.useStock);
  DomUtils::makeNode(root, data.stockType, prefix +  xml.stockType);
  DomUtils::makeNode(root, data.stockTotalDensity, prefix +  xml.stockTotalDensity);
  DomUtils::makeNode(root, data.stockTransition1, prefix +  xml.stockTransition1);
  DomUtils::makeNode(root, data.stockTransition2, prefix +  xml.stockTransition2);
  DomUtils::makeNode(root, data.useStockElmo, prefix +  xml.useStockElmo);
  DomUtils::makeNode(root, data.useStockEpar, prefix +  xml.useStockEpar);

  ///// Properties Other
  DomUtils::makeNode(root, data.useBoys, prefix +  xml.useBoys);
  DomUtils::makeNode(root, data.numBoysIter, prefix +  xml.numBoysIter);
  DomUtils::makeNode(root, data.BoysThreshold, prefix +  xml.BoysThreshold);
  DomUtils::makeNode(root, data.listNucl, prefix +  xml.listNucl);
  DomUtils::makeNode(root, data.useElmo, prefix +  xml.useElmo);
  DomUtils::makeNode(root, data.useExit, prefix +  xml.useExit);
  DomUtils::makeNode(root, data.useJab, prefix +  xml.useJab);

  ///// SCF Convergence
  DomUtils::makeNode(root, data.numIter, prefix +  xml.numIter);
  DomUtils::makeNode(root, data.useJacobi, prefix +  xml.useJacobi);
  DomUtils::makeNode(root, data.useDIIS, prefix +  xml.useDIIS);
  DomUtils::makeNode(root, data.DIISThre, prefix +  xml.DIISThre);
  DomUtils::makeNode(root, data.numDIISCoeff, prefix +  xml.numDIISCoeff);
  DomUtils::makeNode(root, data.valueLvsh, prefix +  xml.valueLvsh);
  DomUtils::makeNode(root, data.useDlvs, prefix +  xml.useDlvs);
  DomUtils::makeNode(root, data.thresholdINTEa, prefix +  xml.thresholdINTEa);
  DomUtils::makeNode(root, data.thresholdINTEb, prefix +  xml.thresholdINTEb);
  DomUtils::makeNode(root, data.thresholdMIAa, prefix +  xml.thresholdMIAa);
  DomUtils::makeNode(root, data.thresholdMIAb, prefix +  xml.thresholdMIAb);
  DomUtils::makeNode(root, data.thresholdSCFa, prefix +  xml.thresholdSCFa);
  DomUtils::makeNode(root, data.thresholdSCFb, prefix +  xml.thresholdSCFb);
  DomUtils::makeNode(root, data.thresholdOverlapa, prefix +  xml.thresholdOverlapa);
  DomUtils::makeNode(root, data.thresholdOverlapb, prefix +  xml.thresholdOverlapb);
  DomUtils::makeNode(root, data.useVarThreMIA, prefix +  xml.useVarThreMIA);

  ///// PVM
  DomUtils::makeNode(root, data.usePVM, prefix +  xml.usePVM);
  DomUtils::makeNode(root, data.PVMExcludeHosts, prefix +  xml.PVMExcludeHosts);
  DomUtils::makeNode(root, data.PVMNumShells, prefix +  xml.PVMNumShells);
  DomUtils::makeNode(root, data.PVMNumTasks, prefix +  xml.PVMNumTasks);
  DomUtils::makeNode(root, data.notOnMaster, prefix +  xml.notOnMaster);
  DomUtils::makeNode(root, data.valueNice, prefix +  xml.valueNice);
  DomUtils::makeNode(root, data.usePacked, prefix +  xml.usePacked);

  ///// Debug
  DomUtils::makeNode(root, data.printDebugPVM, prefix +  xml.printDebugPVM);
  DomUtils::makeNode(root, data.printDebugFORC, prefix +  xml.printDebugFORC);
  DomUtils::makeNode(root, data.goonINTE, prefix +  xml.goonINTE);
  DomUtils::makeNode(root, data.goonSCF, prefix +  xml.goonSCF);
  DomUtils::makeNode(root, data.printIntegrals, prefix +  xml.printIntegrals);
  DomUtils::makeNode(root, data.printShells, prefix +  xml.printShells);
  DomUtils::makeNode(root, data.levelPrintSCF, prefix +  xml.levelPrintSCF);
  DomUtils::makeNode(root, data.numVirtual, prefix +  xml.numVirtual);

  ///// Extra
  DomUtils::makeNode(root, data.numLinesExtraMain, prefix +  xml.numLinesExtraMain);
  DomUtils::makeNode(root, data.numLinesExtraPVM, prefix +  xml.numLinesExtraPVM);
  DomUtils::makeNode(root, data.numLinesExtraINTE, prefix +  xml.numLinesExtraINTE);
  DomUtils::makeNode(root, data.numLinesExtraSCF, prefix +  xml.numLinesExtraSCF);
  DomUtils::makeNode(root, data.numLinesExtraFORC, prefix +  xml.numLinesExtraFORC);
  DomUtils::makeNode(root, data.hPosExtraMain, prefix +  xml.hPosExtraMain);
  DomUtils::makeNode(root, data.hPosExtraPVM, prefix +  xml.hPosExtraPVM);
  DomUtils::makeNode(root, data.hPosExtraINTE, prefix +  xml.hPosExtraINTE);
  DomUtils::makeNode(root, data.hPosExtraSCF, prefix +  xml.hPosExtraSCF);
  DomUtils::makeNode(root, data.hPosExtraFORC, prefix +  xml.hPosExtraFORC);
  DomUtils::makeNode(root, data.vPosExtraMain, prefix +  xml.vPosExtraMain);
  DomUtils::makeNode(root, data.vPosExtraPVM, prefix +  xml.vPosExtraPVM);
  DomUtils::makeNode(root, data.vPosExtraINTE, prefix +  xml.vPosExtraINTE);
  DomUtils::makeNode(root, data.vPosExtraSCF, prefix +  xml.vPosExtraSCF);
  DomUtils::makeNode(root, data.vPosExtraFORC, prefix +  xml.vPosExtraFORC);
  DomUtils::makeNode(root, data.contentsExtraMain, prefix +  xml.contentsExtraMain);
  DomUtils::makeNode(root, data.contentsExtraPVM, prefix +  xml.contentsExtraPVM);
  DomUtils::makeNode(root, data.contentsExtraINTE, prefix +  xml.contentsExtraINTE);
  DomUtils::makeNode(root, data.contentsExtraSCF, prefix +  xml.contentsExtraSCF);
  DomUtils::makeNode(root, data.contentsExtraFORC, prefix +  xml.contentsExtraFORC);
}

///// setPrefferedBasisset ////////////////////////////////////////////////////
void BraboBase::setPreferredBasisset(const unsigned int basisset)
/// Sets the preferred basisset.
{  
  preferredBasisset = basisset;
}


///////////////////////////////////////////////////////////////////////////////
///// Public Slots                                                        /////
///////////////////////////////////////////////////////////////////////////////

///// reset ///////////////////////////////////////////////////////////////////
void BraboBase::reset()
/// Resets all widgets to their default values determined by
/// calcForces = TRUE / FALSE.  
{
  
  ///// Basic Method //////////////////
  ComboBoxSCFMethod->setCurrentItem(0); 
  ComboBoxSCFType->setCurrentItem(0); 
  CheckBoxSTAR->setChecked(false); 
  CheckBoxSTAR2->setChecked(true); 
  LineEditSTAR->clear(); 
  SpinBoxCHAR->setValue(0); 

  ///// Basic Basisset ////////////////
  RadioButtonBAS1->setChecked(true); 
  RadioButtonBAS2->setChecked(false); 
  ComboBoxBAS1->setCurrentItem(preferredBasisset);
  ComboBoxBAS2->setCurrentItem(0); 
  ComboBoxBAS3->setCurrentItem(preferredBasisset);
  ListViewBAS->clear(); 

  ///// Advanced Method ///////////////
  ButtonGroupMP2->setButton(0); 
  SpinBoxMP2E1->setValue(1);
  SpinBoxMP2E2->setValue(0);
  CheckBoxMP2D->setChecked(false); 
  CheckBoxFIEL->setChecked(false);
  LineEditFIELx->clear();
  LineEditFIELy->clear();
  LineEditFIELz->clear();
  CheckBoxSCRF->setChecked(false);
  LineEditSCRF1->clear();
  LineEditSCRF2->clear();

  ///// Advanced Symmetry /////////////
  CheckBoxSYMM->setChecked(false);
  RadioButtonSYMM1->setChecked(true);
  RadioButtonSYMM2->setChecked(false);
  CheckBoxSYMMx->setChecked(false);
  CheckBoxSYMMy->setChecked(false);
  CheckBoxSYMMz->setChecked(false);
  CheckBoxSYMMxy->setChecked(false);
  CheckBoxSYMMxz->setChecked(false);
  CheckBoxSYMMyz->setChecked(false);
  CheckBoxSYMMxyz->setChecked(false);

  ///// Advanced Other ////////////////
  LineEditIPOL->setText("0.75");
  CheckBoxSKPC->setChecked(false);
  CheckBoxGEOP->setChecked(false);
  CheckBoxPRWF->setChecked(false);
  CheckBoxPRDM->setChecked(false);
  CheckBoxDST->setChecked(false);
  CheckBoxACD->setChecked(false);
  CheckBoxF11->setChecked(false);
  CheckBoxFATO->setChecked(false);
  SpinBoxFATO->setValue(0);
  CheckBoxMIAForc->setChecked(false);

  ///// Properties Charges ////////////
  CheckBoxMULL1->setChecked(false);
  CheckBoxMULL2->setChecked(false);
  CheckBoxMULL3->setChecked(false);
  CheckBoxSTOCK->setChecked(false);
  ButtonGroupSTOCK->setButton(0);
  SpinBoxStockTRDE1->setValue(1);
  SpinBoxStockTRDE2->setValue(1);
  ComboBoxStockMOLD->setCurrentItem(0);
  CheckBoxStockELMO->setChecked(false);
  CheckBoxStockEPAR->setChecked(false);

  ///// Properties Other //////////////
  CheckBoxLOCA->setChecked(false);
  SpinBoxLOCA->setValue(0);
  LineEditLOCA->setText("0.05");
  LineEditNUCL->clear();
  CheckBoxELMO->setChecked(false);
  CheckBoxEXIT->setChecked(false);
  CheckBoxJAB->setChecked(false);

  ///// SCF Convergence ///////////////
  SpinBoxITER->setValue(-1);
  CheckBoxJACO->setChecked(false);
  CheckBoxDIIS->setChecked(false);
  LineEditDIIS->setText("0.03");
  SpinBoxDIIS->setValue(-1);
  LineEditLVSH->clear();
  CheckBoxDLVS->setChecked(false);
  LineEditThreINTEa->setText("1.0");
  LineEditThreINTEb->setText("8.0");
  LineEditThreSCF1a->setText("1.0");
  LineEditThreSCF1b->setText("8.0");
  LineEditThreSCF2a->setText("3.0");
  LineEditThreSCF2b->setText("6.0");
  LineEditSthrSCFa->setText("1.0");
  LineEditSthrSCFb->setText("4.0");
  CheckBoxVTHR->setChecked(false);

  ///// PVM ///////////////////////////
  CheckBoxPVM->setChecked(false);
  ListBoxNOTH1->clear();
  SpinBoxNCST->setValue(0);
  SpinBoxNTAS->setValue(0);
  CheckBoxNOSE->setChecked(false); 
  CheckBoxPACK->setChecked(false);

  ///// Debug /////////////////////////
  CheckBoxDebugPVM->setChecked(false);
  CheckBoxDebugFORC->setChecked(false);
  CheckBoxGoonINTE->setChecked(false);
  CheckBoxGoonSCF->setChecked(false);
  CheckBoxIntINTE->setChecked(false);
  CheckBoxShellsINTE->setChecked(false);
  CheckBoxPrintSCF1->setChecked(false);
  CheckBoxPrintSCF2->setChecked(false);
  CheckBoxPrintSCF3->setChecked(false);
  SpinBoxPrintSCF->setValue(0);

  ///// Extra /////////////////////////
  resetTable(TableMain);
  resetTable(TablePVM);
  resetTable(TableINTE);
  resetTable(TableSCF);
  resetTable(TableFORC);
}


///////////////////////////////////////////////////////////////////////////////
///// Protected Slots                                                     /////
///////////////////////////////////////////////////////////////////////////////

///// accept //////////////////////////////////////////////////////////////////
void BraboBase::accept()
/// Overridden from BraboWidget::accept(). Saves the current status of the
/// widgets to the WidgetData struct.  
{
  ///// To check whether any widget has changed, check
  /////   QComboBox->activated(int index) [signal]
  /////   QLineEdit->textChanged(const QString &text) [signal]
  /////   QCheckBox->clicked() [signal]
  /////   QButtonGroup->clicked() [signal] (for radiobuttons)
  /////   QPushButton->clicked() [signal] (for ListViewBAS and ListBoxNOTH1)
  /////   QSpinBox->valueChanged(int value) [signal]
  /////   QTable->valueChanged(int row, int col) [signal]
  if(widgetChanged)
  {
    widgetChanged = false;
    saveWidgets();
    BraboWidget::accept();
  }
  else
    BraboWidget::reject();
}

///// reject //////////////////////////////////////////////////////////////////
void BraboBase::reject()
/// Overridden from BraboWidget::reject(). Restores the widgets from the 
/// WidgetData struct.  
{
  if(widgetChanged)
  {
    widgetChanged = false;
    restoreWidgets();
  }
  BraboWidget::reject();
}


///////////////////////////////////////////////////////////////////////////////
///// Private Slots                                                       /////
///////////////////////////////////////////////////////////////////////////////

///// showPreview /////////////////////////////////////////////////////////////
void BraboBase::showPreview()
/// Shows a preview from the widgets.  
{ 
  // prepare the widget
  TextViewWidget* preview = new TextViewWidget(this, 0, true, 0);
  if(CheckBoxSTOCK->isChecked())
    preview->setCaption(tr("Preview input files ") + calcName + ".inp, " + calcName + ".stin");
  else
    preview->setCaption(tr("Preview input file ") + calcName + ".inp");
  preview->TextEdit->setTextFormat(Qt::LogText);
  QFontMetrics metrics(preview->TextEdit->currentFont());
  int newWidth = 81*metrics.width(" ") + 2*preview->layout()->margin() + preview->TextEdit->verticalScrollBar()->sliderRect().width(); // 20 = 2 x layoutMargin
  preview->resize(newWidth, preview->height()); 

  // generate the text
  QStringList input = generateInput(BRABO);
  if(CheckBoxSTOCK->isChecked())
  {
    input += "________________________________________________________________________________";
    input += "";
    input += generateInput(STOCK);
  }

  // load the text and show it
  preview->TextEdit->setText(input.join("\n"));      
  preview->exec();
  delete preview;
}

///// readInputFile ///////////////////////////////////////////////////////////
void BraboBase::readInputFile()
/// Reads a Brabo input file into the widgets.  
{
  // sometimes misused for building the atomic density basis set files.
  //buildAtdens();

  ///// select an input file
  QString filename = QFileDialog::getOpenFileName(QString::null, "*.inp", this, 0, tr("Choose a BRABO input file"));
  if(filename.isEmpty())
    return;

  ///// read its contents
  QFile file(filename);
  if(!file.open(IO_ReadOnly))
    return;
  QTextStream stream(&file);
  QString line;
  QStringList input;
  while(!stream.atEnd() && line.left(4).lower() != "stop")
  {
    line = stream.readLine();
    if(line.left(1) != "!" && !line.stripWhiteSpace().isEmpty())
      input += line;
  }

  ///// starting configuration
  reset();

  ///// parse the contents of the file
  // 0: Main
  // 1: PVM
  // 2: INTE
  // 3: SCF
  // 4: FORC
  unsigned int section = 0;
  for(QStringList::Iterator it = input.begin(); it != input.end(); it++)
  {
    line = (*it);
    QString key = line.left(4).lower() + " ";
    key.truncate(4);

    ///// update the widgets
    if(key == "dst ")
      CheckBoxDST->setChecked(true);
    else if(key == "acd ")
      CheckBoxACD->setChecked(true);
    else if(key == "pvm ")
    {
      section = 1;
      CheckBoxPVM->setChecked(true);
    }
    else if(key == "prin" || section == 1)
      CheckBoxDebugPVM->setChecked(true);
    else if(key == "noth")
      ; // ignore
    else if(key == "ncst")
      SpinBoxNCST->setValue(line.mid(10,10).toInt());
    else if(key == "ntas")
      SpinBoxNTAS->setValue(line.mid(10,10).toInt());
    else if(key == "nose")
      CheckBoxNOSE->setChecked(true);
    else if(key == "nice")
      SpinBoxNICE->setValue(line.mid(10,10).toInt());
    else if(key == "pack")
      CheckBoxPACK->setChecked(true);
    else if(key == "inte")    
      section = 2;
    else if(key == "mia ")
      ComboBoxSCFType->setCurrentItem(0);
    else if(key == "twoe")
      ComboBoxSCFType->setCurrentItem(2);
    else if(key == "angs" || key == "exfi" || key == "xfcr")
      ; // ignore
    else if(key == "symm")
    {
      CheckBoxSYMM->setChecked(true);
      if(line.mid(10).isEmpty())
        RadioButtonSYMM1->setChecked(true);
      else
      {
        RadioButtonSYMM1->setChecked(false);
        for(unsigned int i = 0; i < 7; i++)
        {
          QString symmType = line.mid(10+10*i,10).stripWhiteSpace();
          if(symmType == "all")
          {
            CheckBoxSYMMx->setChecked(true);
            CheckBoxSYMMy->setChecked(true);
            CheckBoxSYMMz->setChecked(true);
            CheckBoxSYMMxy->setChecked(true);
            CheckBoxSYMMxz->setChecked(true);
            CheckBoxSYMMyz->setChecked(true);
            CheckBoxSYMMxyz->setChecked(true);
            break;
          }
          else if(symmType == "x")
            CheckBoxSYMMx->setChecked(true);
          else if(symmType == "y")
            CheckBoxSYMMy->setChecked(true);
          else if(symmType == "z")
            CheckBoxSYMMz->setChecked(true);
          else if(symmType == "xy")
            CheckBoxSYMMxy->setChecked(true);
          else if(symmType == "xz")
            CheckBoxSYMMxz->setChecked(true);
          else if(symmType == "yz")
            CheckBoxSYMMyz->setChecked(true);
          else if(symmType == "xyz")
            CheckBoxSYMMxyz->setChecked(true);
        }
      }
    }
    else if(key == "thre" && section == 2)
    {
      double thre = line.mid(10,10).toDouble();
      double threA = log10(floor(thre) - thre);
      int threB = static_cast<int>(thre); // maybe change to floor() ? 
      LineEditThreINTEa->setText(QString::number(threA));
      LineEditThreINTEb->setText(QString::number(threB));
    }
    else if(key == "skpc")
      CheckBoxSKPC->setChecked(true);
    else if(key == "geop")
      CheckBoxGEOP->setChecked(true);
    else if(key == "goon" && section == 2)
      CheckBoxGoonINTE->setChecked(true);
    else if(key == "nucl")
    {
      if(!LineEditNUCL->text().isEmpty())
        LineEditNUCL->setText(",");
      LineEditNUCL->setText(LineEditNUCL->text() + QString::number(line.mid(10,10).toUInt()));
    }
    else if(key == "prin" || section == 2)
      CheckBoxIntINTE->setChecked(true);
    else if(key == "long")
      CheckBoxShellsINTE->setChecked(true);
    else if(key == "xfba" || key == "gbas" || key == "abas")
      ; // ignore
    else if(key == "scf ")
      section = 3;
    else if(key == "uhf ")
      ComboBoxSCFMethod->setCurrentItem(1);
    else if(key == "mp2e")
    {
      ComboBoxSCFMethod->setCurrentItem(2);
      int mp2type = line.mid(10,10).toInt();
      switch(mp2type)
      {
        case  0: RadioButtonMP2E1->setChecked(true);
                 break;
        case -1: RadioButtonMP2E2->setChecked(true);
                 break;
        default: RadioButtonMP2E3->setChecked(true);
                 SpinBoxMP2E1->setValue(mp2type);
                 SpinBoxMP2E2->setValue(line.mid(20,10).toInt());
      }
    }
    else if(key == "drct")
      ComboBoxSCFType->setCurrentItem(1);
    else if(key == "ipol")
      LineEditIPOL->setText(QString::number(line.mid(10,10).toDouble()));
    else if(key == "star")
      ; // ignore
    else if(key == "char")
      SpinBoxCHAR->setValue(line.mid(10,10).toInt());
    else if(key == "mp2d")
    {
      CheckBoxMP2D->setChecked(true);
      int mp2type = line.mid(10,10).toInt();
      switch(mp2type)
      {
        case  0: RadioButtonMP2E1->setChecked(true);
                 break;
        case -1: RadioButtonMP2E2->setChecked(true);
                 break;
        default: RadioButtonMP2E3->setChecked(true);
                 SpinBoxMP2E1->setValue(mp2type);
                 SpinBoxMP2E2->setValue(line.mid(20,10).toInt());
      }
    }
    else if(key == "scrf")
    {
      LineEditSCRF1->setText(QString::number(line.mid(10,10).toDouble()));
      LineEditSCRF2->setText(QString::number(line.mid(20,10).toDouble()));
    }
    else if(key == "fiel")
    {
      CheckBoxFIEL->setChecked(true);
      LineEditFIELx->setText(QString::number(line.mid(10,10).toDouble()));
      LineEditFIELy->setText(QString::number(line.mid(20,10).toDouble()));
      LineEditFIELz->setText(QString::number(line.mid(30,10).toDouble()));
    }
    else if(key == "elmo")
      CheckBoxELMO->setChecked(true);
    else if(key == "mull")
    {
      CheckBoxMULL1->setChecked(true);
      if(line.mid(10,10).toUInt() == 1)
        CheckBoxMULL2->setChecked(true);
    }
    else if(key == "mul1")
    {
      CheckBoxMULL3->setChecked(true);
      if(line.mid(10,10).toUInt() == 1)
        CheckBoxMULL2->setChecked(true);
    }
    else if(key == "loca")
    {
      CheckBoxLOCA->setChecked(true);
      SpinBoxLOCA->setValue(line.mid(10,10).toUInt());
      LineEditLOCA->setText(QString::number(line.mid(20,10).toDouble()));
    }
    else if(key == "exit")
      CheckBoxEXIT->setChecked(true);
    else if(key == "jab ")
      CheckBoxJAB->setChecked(true);
    else if(key == "iter")
      SpinBoxITER->setValue(line.mid(10,10).toUInt());
    else if(key == "thre" && section == 3)
    {
      double thre1 = line.mid(10,10).toDouble();
      double thre2 = line.mid(20,10).toDouble();
      if(thre1 >= 1.0)
      {
        double threA = log10(floor(thre1) - thre1);
        int threB = static_cast<int>(thre1); // maybe change to floor? 
        LineEditThreSCF1a->setText(QString::number(threA));
        LineEditThreSCF1b->setText(QString::number(threB));
      }
      if(thre2 >= 1.0)
      {
        double threA = log10(floor(thre2) - thre2);
        int threB = static_cast<int>(thre2); // maybe change to floor? 
        LineEditThreSCF2a->setText(QString::number(threA));
        LineEditThreSCF2b->setText(QString::number(threB));
      }
    }
    else if(key == "sthr")
    {
      double thre = line.mid(10,10).toDouble();
      if(thre >= 1.0)
      {
        double threA = log10(floor(thre) - thre);
        int threB = static_cast<int>(thre); // maybe change to floor? 
        LineEditSthrSCFa->setText(QString::number(threA));
        LineEditSthrSCFb->setText(QString::number(threB));
      }
    }
    else if(key == "vhtr")
      CheckBoxVTHR->setChecked(true);
    else if(key == "diis")
    {
      double num = line.mid(10,10).toDouble();
      if(int(num) == -1)
        CheckBoxDIIS->setChecked(true);
      else
        LineEditDIIS->setText(QString::number(num));
    }
    else if(key == "maxd")
      SpinBoxDIIS->setValue(line.mid(10,10).toUInt());
    else if(key == "goon" && section == 3)
      CheckBoxGoonSCF->setChecked(true);
    else if(key == "lvsh")
      LineEditLVSH->setText(QString::number(line.mid(10,10).toDouble()));
    else if(key == "dlvs")
    {
      CheckBoxDLVS->setChecked(true);
      LineEditLVSH->setText(QString::number(line.mid(10,10).toDouble()));
    }
    else if(key == "prwf")
      CheckBoxPRWF->setChecked(true);
    else if(key == "prdm")
      CheckBoxPRDM->setChecked(true);
    else if(key == "jaco")
      CheckBoxJACO->setChecked(true);
    else if(key == "prin" && section == 3)
    {
      SpinBoxPrintSCF->setValue(line.mid(10,10).toUInt());
      switch(line.mid(20,10).toUInt())
      {
        case 3: CheckBoxPrintSCF3->setChecked(true);
        case 2: CheckBoxPrintSCF2->setChecked(true);
        case 1: CheckBoxPrintSCF1->setChecked(true);
      }
    }
    else if(key == "punc" && section == 3)
      ; // ignore
    else if(key == "forc")
      section = 4;
    else if(key == "punc" && section == 4)
      ; // ignore
    else if(key == "xffo")
      ; // ignore
    else if(key == "fato")
    {
      CheckBoxFATO->setChecked(true);
      SpinBoxFATO->setValue(line.mid(10,10).toUInt());
    }
    else if(key == "prin" || section == 4)
      CheckBoxDebugFORC->setChecked(true);
    else if(key.left(1) == "f" && key.mid(3,1) == "=")
      ; // ignore
    else
    {
      QStringList texts;
      for(unsigned int i = 0; i < 8; i++)
        texts += line.mid(i*10,10);
      int row;
      switch(section)
      {
        case 0: row = firstEmptyTableRow(TableMain);
                for(unsigned int i = 0; i < 8; i++)
                {
                  if(!(*(texts.at(i))).isEmpty())
                    TableMain->setText(row, i, *(texts.at(i)));
                }
                break;
        case 1: row = firstEmptyTableRow(TablePVM);
                for(unsigned int i = 0; i < 8; i++)
                {
                  if(!(*(texts.at(i))).isEmpty())
                    TablePVM->setText(row, i, *(texts.at(i)));
                }
                break;
        case 2: row = firstEmptyTableRow(TableINTE);
                for(unsigned int i = 0; i < 8; i++)
                {
                  if(!(*(texts.at(i))).isEmpty())
                    TableINTE->setText(row, i, *(texts.at(i)));
                }
                break;
        case 3: row = firstEmptyTableRow(TableSCF);
                for(unsigned int i = 0; i < 8; i++)
                {
                  if(!(*(texts.at(i))).isEmpty())
                    TableSCF->setText(row, i, *(texts.at(i)));
                }
                break;
        case 4: row = firstEmptyTableRow(TableFORC);
                for(unsigned int i = 0; i < 8; i++)
                {
                  if(!(*(texts.at(i))).isEmpty())
                    TableFORC->setText(row, i, *(texts.at(i)));
                }
                break;
      }
    }
  }
}

///// changed /////////////////////////////////////////////////////////////
void BraboBase::changed()
/// Indicates at least one widget was altered.  
{
  widgetChanged = true;
}

///// selectWidget ////////////////////////////////////////////////////////////
void BraboBase::selectWidget(QListViewItem* newItem)
/// Shows a widget from WidgetStackCategory depending on newItem.  
{  
  if(newItem->childCount() != 0)
  {
    ///// this is a root item with children, so select the first child (this should retrigger this slot)
    newItem->setOpen(true);
    ListViewCategory->setSelected(newItem->firstChild(), true);
    return;
  }
  
  ///// show a widget from the widgetstack depending on the selected listview item
  if(newItem->parent() == 0)
  {
    ///// this is a root item
    if(newItem->text(0) == category[BASIC])
      WidgetStackCategory->raiseWidget(0);
    else if(newItem->text(0) == category[SCFCONVERGENCE])
      WidgetStackCategory->raiseWidget(7);
    else if(newItem->text(0) == category[PVM])
      WidgetStackCategory->raiseWidget(9);
    else if(newItem->text(0) == category[DEBUG1])
      WidgetStackCategory->raiseWidget(10);
  }
  else
  {
    ///// this is a child item
    //if(newItem->parent()->text(0) == category[BASIC])
    //{
    //  if(newItem->text(0) == category[BASIC_METHOD])
    //    WidgetStackCategory->raiseWidget(0);
    //  else if(newItem->text(0) == category[BASIC_BASISSET])
    //    WidgetStackCategory->raiseWidget(1);
    //}
    if(newItem->parent()->text(0) == category[ADVANCED])
    {
      if(newItem->text(0) == category[ADVANCED_METHOD])
        WidgetStackCategory->raiseWidget(2);
      else if(newItem->text(0) == category[ADVANCED_SYMMETRY])
        WidgetStackCategory->raiseWidget(3);
      else if(newItem->text(0) == category[ADVANCED_OTHER])
        WidgetStackCategory->raiseWidget(4);
    }
    else if(newItem->parent()->text(0) == category[PROPERTIES])
    {
      if(newItem->text(0) == category[PROPERTIES_CHARGES])
        WidgetStackCategory->raiseWidget(5);
      else if(newItem->text(0) == category[PROPERTIES_OTHER])
        WidgetStackCategory->raiseWidget(6);
    }
    else if(newItem->parent()->text(0) == category[EXTRA])
    {
      if(newItem->text(0) == category[EXTRA_MAIN])
        WidgetStackCategory->raiseWidget(11);
      else if(newItem->text(0) == category[EXTRA_PVM])
        WidgetStackCategory->raiseWidget(12);
      else if(newItem->text(0) == category[EXTRA_INTE])
        WidgetStackCategory->raiseWidget(13);
      else if(newItem->text(0) == category[EXTRA_SCF])
        WidgetStackCategory->raiseWidget(14);
      else if(newItem->text(0) == category[EXTRA_FORC])
        WidgetStackCategory->raiseWidget(15);
    }
  }
}

///// updateSCFMethodWidgets //////////////////////////////////////////////////
void BraboBase::updateSCFMethodWidgets(int index)
/// Updates widgets according to the currently selected SCF method.  
{  
  switch(index)
  {
    case 0: ///// RHF
      GroupBoxMP2->setEnabled(false);
      GroupBoxFIEL->setEnabled(true);
      if(ComboBoxSCFType->currentItem() == 2)
        GroupBoxSCRF->setEnabled(true);
      else
        GroupBoxSCRF->setEnabled(false);
      ButtonGroupSYMM->setEnabled(true);
      RadioButtonStockDENS5->setEnabled(false);
      break;
    case 1: ///// UHF
      GroupBoxMP2->setEnabled(false);
      GroupBoxFIEL->setEnabled(false);
      GroupBoxSCRF->setEnabled(false);
      RadioButtonStockDENS5->setEnabled(true);
      if(ComboBoxSCFType->currentItem() == 0)
        ButtonGroupSYMM->setEnabled(true);
      else
        ButtonGroupSYMM->setEnabled(false);
      break;
    case 2: ///// RMP2 (only if Single Point Energy)
      GroupBoxMP2->setEnabled(true);
      GroupBoxFIEL->setEnabled(true);
      if(ComboBoxSCFType->currentItem() == 2)
        GroupBoxSCRF->setEnabled(true);
      else
        GroupBoxSCRF->setEnabled(false);
      ButtonGroupSYMM->setEnabled(true);
      RadioButtonStockDENS5->setEnabled(false);
  }
}

///// updateSCFTypeWidgets ////////////////////////////////////////////////////
void BraboBase::updateSCFTypeWidgets(int index)
/// Updates widgets according to the currently selected SCF type.  
{
  switch(index)
  {
    case 0: ///// MIA
      GroupBoxSCRF->setEnabled(false);
      if(calcForces && CheckBoxMIAForc->isChecked())
      {
        CheckBoxPVM->setChecked(false);
        CheckBoxPVM->setEnabled(false);
      }
      else
        CheckBoxPVM->setEnabled(true);
      CheckBoxDST->setEnabled(true);
      CheckBoxACD->setEnabled(true);
      CheckBoxMIAForc->setEnabled(true);
      LabelThreMIA1->setEnabled(true);
      LineEditThreSCF1a->setEnabled(true);
      LabelThreMIA2->setEnabled(true);
      LineEditThreSCF1b->setEnabled(true);
      CheckBoxVTHR->setEnabled(true);
      break;
    case 1: ///// Direct
      GroupBoxSCRF->setEnabled(false);
      CheckBoxPVM->setEnabled(true);
      CheckBoxDST->setEnabled(true);
      CheckBoxACD->setEnabled(true);
      LabelThreMIA1->setEnabled(false);
      LineEditThreSCF1a->setEnabled(false);
      LabelThreMIA2->setEnabled(false);
      LineEditThreSCF1b->setEnabled(false);
      CheckBoxVTHR->setEnabled(false);
      break;
    case 2: ///// Conventional
      CheckBoxMIAForc->setEnabled(false);
      if(ComboBoxSCFMethod->currentItem() != 1)
        GroupBoxSCRF->setEnabled(true);
      else
        GroupBoxSCRF->setEnabled(false);
      CheckBoxPVM->setChecked(false);
      CheckBoxPVM->setEnabled(false);
      CheckBoxDST->setEnabled(false);
      CheckBoxACD->setEnabled(false);
      LabelThreMIA1->setEnabled(false);
      LineEditThreSCF1a->setEnabled(false);
      LabelThreMIA2->setEnabled(false);
      LineEditThreSCF1b->setEnabled(false);
      CheckBoxVTHR->setEnabled(false);
  }
}

///// selectStartvector ///////////////////////////////////////////////////////
void BraboBase::selectStartvector()
/// Selects a file containing a startvector to be used in the calculation.  
{  
  QString filename = QFileDialog::getOpenFileName(LineEditSTAR->text(),tr("Startvectors (*.sta)"),this, 0, tr("Choose a startvector"));
  if(!filename.isNull())
    LineEditSTAR->setText(QDir::convertSeparators(filename));
}

///// oneBasisset /////////////////////////////////////////////////////////////
void BraboBase::oneBasisset(bool on)
/// Enables/disables certain widgets depending on the selection
/// of 1 basisset (true) or multiple basissets (false).  
{
  ComboBoxBAS1->setEnabled(on);
  TextLabelBAS1->setEnabled(!on);
  TextLabelBAS2->setEnabled(!on);
  ComboBoxBAS2->setEnabled(!on);
  ComboBoxBAS3->setEnabled(!on);
  ToolButtonBAS1->setEnabled(!on);
  ToolButtonBAS2->setEnabled(!on);
  ListViewBAS->setEnabled(!on);
}

///// addBasisset /////////////////////////////////////////////////////////////
void BraboBase::addBasisset()
/// Adds a basisset/atom combination to ListViewBAS, but only
/// if the atom type isn't listed yet.  
{
  ///// check for duplicate atoms
  bool dupefound = false;
  QListViewItemIterator it(ListViewBAS);
  for( ; it.current(); ++it)
  {
    if(it.current()->text(0) == ComboBoxBAS2->currentText())
    {
      dupefound = true;
      break;
    }
  }
  ///// add if unique
  if(!dupefound)
  {
    (void) new QListViewItem(ListViewBAS,ComboBoxBAS2->currentText(),ComboBoxBAS3->currentText());
    widgetChanged = true;
  }
}

///// removeBasisset //////////////////////////////////////////////////////////
void BraboBase::removeBasisset()
/// Removes the currently selected line from ListViewBAS.
{
  if(ListViewBAS->currentItem() != 0)
  {
    delete ListViewBAS->currentItem();
    widgetChanged = true;
  }
}

///// toggleDIIS //////////////////////////////////////////////////////////////
void BraboBase::toggleDIIS(bool on)
/// Enables/disables the DIIS options.
{
  LineEditDIIS->setEnabled(!on);
  SpinBoxDIIS->setEnabled(!on);
  LabelDIIS1->setEnabled(!on);
  LabelDIIS2->setEnabled(!on);  
}

///// addPVMHost //////////////////////////////////////////////////////////////
void BraboBase::addPVMHost()
/// Adds the currently selected host from ListBoxNOTH2 to
/// ListBoxNOTH1. Checks for duplicates. ListBoxNOTH2 is never changed.
{
  ///// read the host
  QString host = ListBoxNOTH2->currentText();
  if(host.isNull())
    return;

  ///// check for duplicates
  if(ListBoxNOTH1->findItem(host, Qt::ExactMatch) != 0)
    return;

  ///// add the host
  ListBoxNOTH1->insertItem(host);
}

///// removePVMHost ///////////////////////////////////////////////////////////
void BraboBase::removePVMHost()
/// Removes the currently selected host from ListBoxNOTH1.
{
  ListBoxNOTH1->removeItem(ListBoxNOTH1->currentItem());
}

///// adjustPrintSCF //////////////////////////////////////////////////////////
void BraboBase::adjustPrintSCF()
/// Adjusts the Print SCF Level checkboxes. Checking a level
/// should check all lower levels too.
{
  if(CheckBoxPrintSCF3->isChecked())
    CheckBoxPrintSCF2->setChecked(true);
  if(CheckBoxPrintSCF2->isChecked())
    CheckBoxPrintSCF1->setChecked(true);
  if(CheckBoxPrintSCF1->isChecked())
  {
    SpinBoxPrintSCF->setEnabled(true);
    LabelPrintSCF2->setEnabled(true);
  }
  else
  {
    SpinBoxPrintSCF->setEnabled(false);
    LabelPrintSCF2->setEnabled(false);
  }
}

///// checkOverflowMain ///////////////////////////////////////////////////////
void BraboBase::checkOverflowMain(int row, int col)
/// Spans cell(row, col) over multiple columns if it contains
/// more than 10 characters.
/// => For TableMain
{
  checkTableOverflow(TableMain, row, col);
}

///// checkOverflowPVM ////////////////////////////////////////////////////////
void BraboBase::checkOverflowPVM(int row, int col)
/// Spans cell(row, col) over multiple columns if it contains
/// more than 10 characters.
/// => For TablePVM
{
  checkTableOverflow(TablePVM, row, col);
}

///// checkOverflowINTE ///////////////////////////////////////////////////////
void BraboBase::checkOverflowINTE(int row, int col)
/// Spans cell(row, col) over multiple columns if it contains
/// more than 10 characters.
/// => For TableINTE
{
  checkTableOverflow(TableINTE, row, col);
}

///// checkOverflowSCF ////////////////////////////////////////////////////////
void BraboBase::checkOverflowSCF(int row, int col)
/// Spans cell(row, col) over multiple columns if it contains
/// more than 10 characters.
/// => For TableINTE
{
  checkTableOverflow(TableSCF, row, col);
}

///// checkOverflowFORC ///////////////////////////////////////////////////////
void BraboBase::checkOverflowFORC(int row, int col)
/// Spans cell(row, col) over multiple columns if it contains
/// more than 10 characters.
/// => For TableFORC
{
  checkTableOverflow(TableFORC, row, col);
}

///// addRowMain //////////////////////////////////////////////////////////////
void BraboBase::addRowMain()
/// If a row is selected, inserts a row before the selection.
/// If no row is selected, adds a row at the end.
/// => For TableMain
{
  addTableRow(TableMain);
}

///// addRowPVM ///////////////////////////////////////////////////////////////
void BraboBase::addRowPVM()
/// If a row is selected, inserts a row before the selection.
/// If no row is selected, adds a row at the end.
/// => For TablePVM
{
  addTableRow(TablePVM);
}

///// addRowINTE //////////////////////////////////////////////////////////////
void BraboBase::addRowINTE()
/// If a row is selected, inserts a row before the selection.
/// If no row is selected, adds a row at the end.
/// => For TableINTE
{
  addTableRow(TableINTE);
}

///// addRowSCF ///////////////////////////////////////////////////////////////
void BraboBase::addRowSCF()
/// If a row is selected, inserts a row before the selection.
/// If no row is selected, adds a row at the end.
/// => For TableSCF
{
  addTableRow(TableSCF);
}

///// addRowFORC //////////////////////////////////////////////////////////////
void BraboBase::addRowFORC()
/// If a row is selected, inserts a row before the selection.
/// If no row is selected, adds a row at the end.
/// => For TableFORC
{
  addTableRow(TableFORC);
}

///// removeRowMain ///////////////////////////////////////////////////////////
void BraboBase::removeRowMain()
/// Removes the selected rows from TableMain.
{
  if(removeTableRow(TableMain))
    changed();
}

///// removeRowPVM ////////////////////////////////////////////////////////////
void BraboBase::removeRowPVM()
/// Removes the selected rows from TablePVM.
{
  if(removeTableRow(TablePVM))
    changed();
}

///// removeRowINTE ///////////////////////////////////////////////////////////
void BraboBase::removeRowINTE()
/// Removes the selected rows from TableINTE.
{
  if(removeTableRow(TableINTE))
    changed();
}

///// removeRowSCF ////////////////////////////////////////////////////////////
void BraboBase::removeRowSCF()
/// Removes the selected rows from TableSCF.
{
  if(removeTableRow(TableSCF))
    changed();
}

///// removeRowFORC ///////////////////////////////////////////////////////////
void BraboBase::removeRowFORC()
/// Removes the selected rows from TableFORC.
{
  if(removeTableRow(TableFORC))
    changed();
}

///// clearSelectionMain //////////////////////////////////////////////////////
void BraboBase::clearSelectionMain()
/// Clears the selected cells in TableMain.
{
  if(clearTableSelection(TableMain))
    changed();  
}

///// clearSelectionPVM ///////////////////////////////////////////////////////
void BraboBase::clearSelectionPVM()
/// Clears the selected cells in TablePVM.
{
  if(clearTableSelection(TablePVM))
    changed();  
}

///// clearSelectionINTE //////////////////////////////////////////////////////
void BraboBase::clearSelectionINTE()
/// Clears the selected cells in TableINTE.
{
  if(clearTableSelection(TableINTE))
    changed();  
}

///// clearSelectionSCF ///////////////////////////////////////////////////////
void BraboBase::clearSelectionSCF()
/// Clears the selected cells in TableSCF.
{
  if(clearTableSelection(TableSCF))
    changed();  
}

///// clearSelectionFORC //////////////////////////////////////////////////////
void BraboBase::clearSelectionFORC()
/// Clears the selected cells in TableFORC.
{
  if(clearTableSelection(TableFORC))
    changed();  
}


///////////////////////////////////////////////////////////////////////////////
///// Private Member Functions                                            /////
///////////////////////////////////////////////////////////////////////////////

///// makeConnections /////////////////////////////////////////////////////////
void BraboBase::makeConnections()
/// Sets up the permanent connections. Called once from the constructor.
{
  ///// connections for ListView-WidgetStack
  connect(ListViewCategory, SIGNAL(selectionChanged(QListViewItem*)), this, SLOT(selectWidget(QListViewItem*)));

  ///// connections for buttons
  ///// bottom ones
  connect(ButtonOK, SIGNAL(clicked()), this, SLOT(accept()));
  connect(ButtonCancel, SIGNAL(clicked()), this, SLOT(reject()));
  connect(ButtonPreview, SIGNAL(clicked()), this, SLOT(showPreview()));
  connect(ButtonRead, SIGNAL(clicked()), this, SLOT(readInputFile()));
  connect(ButtonReset, SIGNAL(clicked()), this, SLOT(reset()));
  ///// Basic Method
  connect(ToolButtonSTAR, SIGNAL(clicked()), this, SLOT(selectStartvector()));
  ///// Basic Basisset
  connect(ToolButtonBAS1, SIGNAL(clicked()), this, SLOT(addBasisset()));
  connect(ToolButtonBAS2, SIGNAL(clicked()), this, SLOT(removeBasisset()));
  ///// PVM
  connect(ToolButtonNOTHadd, SIGNAL(clicked()), this, SLOT(addPVMHost()));
  connect(ToolButtonNOTHremove, SIGNAL(clicked()), this, SLOT(removePVMHost()));

  ///// Connections for enabling/disabling widgets
  ///// Basic Method
  connect(ComboBoxSCFMethod, SIGNAL(activated(int)), this, SLOT(updateSCFMethodWidgets(int)));
  connect(ComboBoxSCFType, SIGNAL(activated(int)), this, SLOT(updateSCFTypeWidgets(int)));
  ///// Basic Basisset
  connect(RadioButtonBAS1, SIGNAL(toggled(bool)),this,SLOT(oneBasisset(bool)));
  ///// Advanced Method
  connect(CheckBoxFIEL, SIGNAL(toggled(bool)), LineEditFIELx, SLOT(setEnabled(bool)));
  connect(CheckBoxFIEL, SIGNAL(toggled(bool)), LineEditFIELy, SLOT(setEnabled(bool)));
  connect(CheckBoxFIEL, SIGNAL(toggled(bool)), LineEditFIELz, SLOT(setEnabled(bool)));
  connect(CheckBoxFIEL, SIGNAL(toggled(bool)), LabelFIELx, SLOT(setEnabled(bool)));
  connect(CheckBoxFIEL, SIGNAL(toggled(bool)), LabelFIELy, SLOT(setEnabled(bool)));
  connect(CheckBoxFIEL, SIGNAL(toggled(bool)), LabelFIELz, SLOT(setEnabled(bool)));
  connect(CheckBoxSCRF, SIGNAL(toggled(bool)), LineEditSCRF1, SLOT(setEnabled(bool)));
  connect(CheckBoxSCRF, SIGNAL(toggled(bool)), LineEditSCRF2, SLOT(setEnabled(bool)));
  connect(CheckBoxSCRF, SIGNAL(toggled(bool)), LabelSCRF1, SLOT(setEnabled(bool)));
  connect(CheckBoxSCRF, SIGNAL(toggled(bool)), LabelSCRF2, SLOT(setEnabled(bool)));
  ///// Advanced Symmetry
  connect(CheckBoxSYMM, SIGNAL(toggled(bool)), RadioButtonSYMM1, SLOT(setEnabled(bool)));
  connect(CheckBoxSYMM, SIGNAL(toggled(bool)), RadioButtonSYMM2, SLOT(setEnabled(bool)));
  connect(CheckBoxSYMM, SIGNAL(toggled(bool)), GroupBoxSYMM, SLOT(setEnabled(bool)));
  connect(RadioButtonSYMM2, SIGNAL(toggled(bool)), CheckBoxSYMMx, SLOT(setEnabled(bool)));
  connect(RadioButtonSYMM2, SIGNAL(toggled(bool)), CheckBoxSYMMy, SLOT(setEnabled(bool)));
  connect(RadioButtonSYMM2, SIGNAL(toggled(bool)), CheckBoxSYMMz, SLOT(setEnabled(bool)));
  connect(RadioButtonSYMM2, SIGNAL(toggled(bool)), CheckBoxSYMMxy, SLOT(setEnabled(bool)));
  connect(RadioButtonSYMM2, SIGNAL(toggled(bool)), CheckBoxSYMMxz, SLOT(setEnabled(bool)));
  connect(RadioButtonSYMM2, SIGNAL(toggled(bool)), CheckBoxSYMMyz, SLOT(setEnabled(bool)));
  connect(RadioButtonSYMM2, SIGNAL(toggled(bool)), CheckBoxSYMMxyz, SLOT(setEnabled(bool)));  
  ///// Properties Charges
  connect(CheckBoxMULL1, SIGNAL(toggled(bool)), CheckBoxMULL2, SLOT(setEnabled(bool)));
  connect(CheckBoxMULL1, SIGNAL(toggled(bool)), CheckBoxMULL3, SLOT(setEnabled(bool)));
  connect(CheckBoxSTOCK, SIGNAL(toggled(bool)), ButtonGroupSTOCK, SLOT(setEnabled(bool)));
  connect(CheckBoxSTOCK, SIGNAL(toggled(bool)), GroupBoxSTOCK, SLOT(setEnabled(bool)));
  connect(RadioButtonStockDENS4, SIGNAL(toggled(bool)), SpinBoxStockTRDE1, SLOT(setEnabled(bool)));
  connect(RadioButtonStockDENS4, SIGNAL(toggled(bool)), SpinBoxStockTRDE2, SLOT(setEnabled(bool)));
  connect(RadioButtonStockDENS4, SIGNAL(toggled(bool)), LabelStockTRDE, SLOT(setEnabled(bool)));  
  ///// Properties Other
  connect(CheckBoxLOCA, SIGNAL(toggled(bool)), SpinBoxLOCA, SLOT(setEnabled(bool)));
  connect(CheckBoxLOCA, SIGNAL(toggled(bool)), LineEditLOCA, SLOT(setEnabled(bool)));
  connect(CheckBoxLOCA, SIGNAL(toggled(bool)), LabelLOCA1, SLOT(setEnabled(bool)));
  connect(CheckBoxLOCA, SIGNAL(toggled(bool)), LabelLOCA2, SLOT(setEnabled(bool)));
  // SCF Convergence
  connect(CheckBoxDIIS, SIGNAL(toggled(bool)), this, SLOT(toggleDIIS(bool)));
  ///// PVM
  connect(CheckBoxPVM, SIGNAL(toggled(bool)), GroupBoxPVM1, SLOT(setEnabled(bool)));
  connect(CheckBoxPVM, SIGNAL(toggled(bool)), GroupBoxPVM2, SLOT(setEnabled(bool)));
  connect(CheckBoxPVM, SIGNAL(toggled(bool)), GroupBoxPVM3, SLOT(setEnabled(bool)));
  ///// Debug
  connect(CheckBoxPrintSCF1, SIGNAL(clicked()), this, SLOT(adjustPrintSCF()));
  connect(CheckBoxPrintSCF2, SIGNAL(clicked()), this, SLOT(adjustPrintSCF()));
  connect(CheckBoxPrintSCF3, SIGNAL(clicked()), this, SLOT(adjustPrintSCF()));
  ///// Extra PVM
  connect(CheckBoxPVM, SIGNAL(toggled(bool)), GroupBoxExtraPVM, SLOT(setEnabled(bool)));  
  
  ///// connections for changes
  ///// Basic Method
  connect(ComboBoxSCFMethod, SIGNAL(activated(int)), this, SLOT(changed()));
  connect(ComboBoxSCFType, SIGNAL(activated(int)), this, SLOT(changed()));
  connect(CheckBoxSTAR, SIGNAL(clicked()), this, SLOT(changed()));
  connect(CheckBoxSTAR2, SIGNAL(clicked()), this, SLOT(changed()));
  connect(LineEditSTAR, SIGNAL(textChanged(const QString&)), this, SLOT(changed()));
  connect(SpinBoxCHAR, SIGNAL(valueChanged(int)), this, SLOT(changed()));
  ///// Basic Basisset
  connect(RadioButtonBAS1, SIGNAL(clicked()), this, SLOT(changed()));
  connect(RadioButtonBAS2, SIGNAL(clicked()), this, SLOT(changed()));
  connect(ComboBoxBAS1, SIGNAL(activated(int)), this, SLOT(changed()));
  connect(ComboBoxBAS2, SIGNAL(activated(int)), this, SLOT(changed()));
  connect(ComboBoxBAS3, SIGNAL(activated(int)), this, SLOT(changed()));
  connect(ToolButtonBAS1, SIGNAL(clicked()), this, SLOT(changed()));
  connect(ToolButtonBAS2, SIGNAL(clicked()), this, SLOT(changed()));
  ///// Advanced Method
  connect(ButtonGroupMP2, SIGNAL(clicked(int)), this, SLOT(changed()));
  connect(SpinBoxMP2E1, SIGNAL(valueChanged(int)), this, SLOT(changed()));
  connect(SpinBoxMP2E2, SIGNAL(valueChanged(int)), this, SLOT(changed()));
  connect(CheckBoxMP2D, SIGNAL(clicked()), this, SLOT(changed()));
  connect(CheckBoxFIEL, SIGNAL(clicked()), this, SLOT(changed()));
  connect(LineEditFIELx, SIGNAL(textChanged(const QString&)), this, SLOT(changed()));
  connect(LineEditFIELy, SIGNAL(textChanged(const QString&)), this, SLOT(changed()));
  connect(LineEditFIELz, SIGNAL(textChanged(const QString&)), this, SLOT(changed()));
  connect(CheckBoxSCRF, SIGNAL(clicked()), this, SLOT(changed()));
  connect(LineEditSCRF1, SIGNAL(textChanged(const QString&)), this, SLOT(changed()));
  connect(LineEditSCRF2, SIGNAL(textChanged(const QString&)), this, SLOT(changed()));
  ///// Advanced Symmetry
  connect(CheckBoxSYMM, SIGNAL(clicked()), this, SLOT(changed()));
  connect(ButtonGroupSYMM, SIGNAL(clicked(int)), this, SLOT(changed()));
  connect(CheckBoxSYMMx, SIGNAL(clicked()), this, SLOT(changed()));
  connect(CheckBoxSYMMy, SIGNAL(clicked()), this, SLOT(changed()));
  connect(CheckBoxSYMMz, SIGNAL(clicked()), this, SLOT(changed()));
  connect(CheckBoxSYMMxy, SIGNAL(clicked()), this, SLOT(changed()));
  connect(CheckBoxSYMMyz, SIGNAL(clicked()), this, SLOT(changed()));
  connect(CheckBoxSYMMxz, SIGNAL(clicked()), this, SLOT(changed()));
  connect(CheckBoxSYMMxyz, SIGNAL(clicked()), this, SLOT(changed()));
  ///// Advanced Other
  connect(LineEditIPOL, SIGNAL(textChanged(const QString&)), this, SLOT(changed()));
  connect(CheckBoxSKPC, SIGNAL(clicked()), this, SLOT(changed()));
  connect(CheckBoxGEOP, SIGNAL(clicked()), this, SLOT(changed()));
  connect(CheckBoxPRWF, SIGNAL(clicked()), this, SLOT(changed()));
  connect(CheckBoxPRDM, SIGNAL(clicked()), this, SLOT(changed()));
  connect(CheckBoxDST, SIGNAL(clicked()), this, SLOT(changed()));
  connect(CheckBoxACD, SIGNAL(clicked()), this, SLOT(changed()));
  connect(CheckBoxF11, SIGNAL(clicked()), this, SLOT(changed()));
  connect(CheckBoxFATO, SIGNAL(clicked()), this, SLOT(changed()));
  connect(SpinBoxFATO, SIGNAL(valueChanged(int)), this, SLOT(changed()));
  connect(CheckBoxMIAForc, SIGNAL(clicked()), this, SLOT(changed()));
  ///// Properties Charges
  connect(CheckBoxMULL1, SIGNAL(clicked()), this, SLOT(changed()));
  connect(CheckBoxMULL2, SIGNAL(clicked()), this, SLOT(changed()));
  connect(CheckBoxMULL3, SIGNAL(clicked()), this, SLOT(changed()));
  connect(CheckBoxSTOCK, SIGNAL(clicked()), this, SLOT(changed()));
  connect(ButtonGroupSTOCK, SIGNAL(clicked(int)), this, SLOT(changed()));
  connect(SpinBoxStockTRDE1, SIGNAL(valueChanged(int)), this, SLOT(changed()));
  connect(SpinBoxStockTRDE2, SIGNAL(valueChanged(int)), this, SLOT(changed()));
  connect(ComboBoxStockMOLD, SIGNAL(activated(int)), this, SLOT(changed()));
  connect(CheckBoxStockELMO, SIGNAL(clicked()), this, SLOT(changed()));
  connect(CheckBoxStockEPAR, SIGNAL(clicked()), this, SLOT(changed()));
  ///// Properties Other
  connect(CheckBoxLOCA, SIGNAL(clicked()), this, SLOT(changed()));
  connect(SpinBoxLOCA, SIGNAL(valueChanged(int)), this, SLOT(changed()));
  connect(LineEditLOCA, SIGNAL(textChanged(const QString&)), this, SLOT(changed()));
  connect(LineEditNUCL, SIGNAL(textChanged(const QString&)), this, SLOT(changed()));
  connect(CheckBoxELMO, SIGNAL(clicked()), this, SLOT(changed()));
  connect(CheckBoxEXIT, SIGNAL(clicked()), this, SLOT(changed()));
  connect(CheckBoxJAB, SIGNAL(clicked()), this, SLOT(changed()));
  ///// SCF Convergence
  connect(SpinBoxITER, SIGNAL(valueChanged(int)), this, SLOT(changed()));
  connect(CheckBoxJACO, SIGNAL(clicked()), this, SLOT(changed()));
  connect(CheckBoxDIIS, SIGNAL(clicked()), this, SLOT(changed()));
  connect(LineEditDIIS, SIGNAL(textChanged(const QString&)), this, SLOT(changed()));
  connect(SpinBoxDIIS, SIGNAL(valueChanged(int)), this, SLOT(changed()));
  connect(LineEditLVSH, SIGNAL(textChanged(const QString&)), this, SLOT(changed()));
  connect(CheckBoxDLVS, SIGNAL(clicked()), this, SLOT(changed()));
  connect(LineEditThreINTEa, SIGNAL(textChanged(const QString&)), this, SLOT(changed()));
  connect(LineEditThreINTEb, SIGNAL(textChanged(const QString&)), this, SLOT(changed()));
  connect(LineEditThreSCF1a, SIGNAL(textChanged(const QString&)), this, SLOT(changed()));
  connect(LineEditThreSCF1b, SIGNAL(textChanged(const QString&)), this, SLOT(changed()));
  connect(LineEditThreSCF2a, SIGNAL(textChanged(const QString&)), this, SLOT(changed()));
  connect(LineEditThreSCF2b, SIGNAL(textChanged(const QString&)), this, SLOT(changed()));
  connect(LineEditSthrSCFa, SIGNAL(textChanged(const QString&)), this, SLOT(changed()));
  connect(LineEditSthrSCFb, SIGNAL(textChanged(const QString&)), this, SLOT(changed()));
  connect(CheckBoxVTHR, SIGNAL(clicked()), this, SLOT(changed()));
  ///// PVM
  connect(CheckBoxPVM, SIGNAL(clicked()), this, SLOT(changed()));
  connect(ToolButtonNOTHadd, SIGNAL(clicked()), this, SLOT(changed()));
  connect(ToolButtonNOTHremove, SIGNAL(clicked()), this, SLOT(changed()));
  connect(SpinBoxNCST, SIGNAL(valueChanged(int)), this, SLOT(changed()));
  connect(SpinBoxNTAS, SIGNAL(valueChanged(int)), this, SLOT(changed()));
  connect(CheckBoxNOSE, SIGNAL(clicked()), this, SLOT(changed()));
  connect(SpinBoxNICE, SIGNAL(valueChanged(int)), this, SLOT(changed()));
  connect(CheckBoxPACK, SIGNAL(clicked()), this, SLOT(changed()));
  ///// Debug
  connect(CheckBoxDebugPVM, SIGNAL(clicked()), this, SLOT(changed()));
  connect(CheckBoxDebugFORC, SIGNAL(clicked()), this, SLOT(changed()));
  connect(CheckBoxGoonINTE, SIGNAL(clicked()), this, SLOT(changed()));
  connect(CheckBoxGoonSCF, SIGNAL(clicked()), this, SLOT(changed()));
  connect(CheckBoxIntINTE, SIGNAL(clicked()), this, SLOT(changed()));
  connect(CheckBoxShellsINTE, SIGNAL(clicked()), this, SLOT(changed()));
  connect(CheckBoxPrintSCF1, SIGNAL(clicked()), this, SLOT(changed()));
  connect(CheckBoxPrintSCF2, SIGNAL(clicked()), this, SLOT(changed()));
  connect(CheckBoxPrintSCF3, SIGNAL(clicked()), this, SLOT(changed()));
  connect(SpinBoxPrintSCF, SIGNAL(valueChanged(int)), this, SLOT(changed()));
  ///// Extra Main
  connect(TableMain, SIGNAL(valueChanged(int, int)), this, SLOT(changed()));
  connect(TablePVM, SIGNAL(valueChanged(int, int)), this, SLOT(changed()));
  connect(TableINTE, SIGNAL(valueChanged(int, int)), this, SLOT(changed()));
  connect(TableSCF, SIGNAL(valueChanged(int, int)), this, SLOT(changed()));
  connect(TableFORC, SIGNAL(valueChanged(int, int)), this, SLOT(changed()));
  connect(ToolButtonAddMain, SIGNAL(clicked()), this, SLOT(changed()));
  connect(ToolButtonAddPVM, SIGNAL(clicked()), this, SLOT(changed()));
  connect(ToolButtonAddINTE, SIGNAL(clicked()), this, SLOT(changed()));
  connect(ToolButtonAddSCF, SIGNAL(clicked()), this, SLOT(changed()));
  connect(ToolButtonAddFORC, SIGNAL(clicked()), this, SLOT(changed()));
  // Remove's and Clear's changes are handled in the slots removeRow and clearSelection

  ///// connections for the Tables
  connect(TableMain, SIGNAL(valueChanged(int, int)), this, SLOT(checkOverflowMain(int, int)));
  connect(TablePVM, SIGNAL(valueChanged(int, int)), this, SLOT(checkOverflowPVM(int, int)));
  connect(TableINTE, SIGNAL(valueChanged(int, int)), this, SLOT(checkOverflowINTE(int, int)));
  connect(TableSCF, SIGNAL(valueChanged(int, int)), this, SLOT(checkOverflowSCF(int, int)));
  connect(TableFORC, SIGNAL(valueChanged(int, int)), this, SLOT(checkOverflowFORC(int, int)));
  connect(ToolButtonAddMain, SIGNAL(clicked()), this, SLOT(addRowMain()));
  connect(ToolButtonAddPVM, SIGNAL(clicked()), this, SLOT(addRowPVM()));
  connect(ToolButtonAddINTE, SIGNAL(clicked()), this, SLOT(addRowINTE()));
  connect(ToolButtonAddSCF, SIGNAL(clicked()), this, SLOT(addRowSCF()));
  connect(ToolButtonAddFORC, SIGNAL(clicked()), this, SLOT(addRowFORC()));
  connect(ToolButtonRemoveMain, SIGNAL(clicked()), this, SLOT(removeRowMain()));
  connect(ToolButtonRemovePVM, SIGNAL(clicked()), this, SLOT(removeRowPVM()));
  connect(ToolButtonRemoveINTE, SIGNAL(clicked()), this, SLOT(removeRowINTE()));
  connect(ToolButtonRemoveSCF, SIGNAL(clicked()), this, SLOT(removeRowSCF()));
  connect(ToolButtonRemoveFORC, SIGNAL(clicked()), this, SLOT(removeRowFORC()));
  connect(ToolButtonClearMain, SIGNAL(clicked()), this, SLOT(clearSelectionMain()));
  connect(ToolButtonClearPVM, SIGNAL(clicked()), this, SLOT(clearSelectionPVM()));
  connect(ToolButtonClearINTE, SIGNAL(clicked()), this, SLOT(clearSelectionINTE()));
  connect(ToolButtonClearSCF, SIGNAL(clicked()), this, SLOT(clearSelectionSCF()));
  connect(ToolButtonClearFORC, SIGNAL(clicked()), this, SLOT(clearSelectionFORC()));
}

///// init ////////////////////////////////////////////////////////////////////
void BraboBase::init()
/// Initializes the dialog. Called once from the 
/// constructor. Connections should be up.
{
  ///// these values should be redefined before the first exec() of this dialog
  calcForces = true; // so the call to setForces does something
  setForces(false);
  setDescription(tr("not specified"));
  setName(tr("not specified"));
  setExtendedFormat(false);
  
  ///// fill the category array (the order of the strings must coincide with the order in the enum Categories
  category.clear();
  //TODO: setsize to the desired final size
  category.push_back(tr("Basic"));          // root
  category.push_back(tr("Advanced"));
  category.push_back(tr("Properties"));
  category.push_back(tr("SCF Convergence"));
  category.push_back(tr("PVM"));
  category.push_back(tr("Debug"));
  category.push_back(tr("Extra"));
  //category.push_back(tr("Method"));         // Basic
  //category.push_back(tr("Basis Set"));
  category.push_back(tr("Method"));         // Advanced
  category.push_back(tr("Symmetry"));
  category.push_back(tr("Other"));
  category.push_back(tr("Charges"));        // Properties
  category.push_back(tr("Other"));
  category.push_back(tr("Main"));           // Extra
  category.push_back(tr("PVM"));
  category.push_back(tr("INTE"));
  category.push_back(tr("SCF"));
  category.push_back(tr("FORC"));

  ///// setup ListViewCategory (setSorting can't be set in Designer, so items are always sorted)
  ListViewCategory->clear();
  ListViewCategory->setSorting(-1);
  ListViewCategory->header()->hide();
  ///// add all items in reverse order as they are added at the top
  QListViewItem* rootItem;
  ///// Extra
  rootItem = new QListViewItem(ListViewCategory, category[EXTRA]);
    new QListViewItem(rootItem, category[EXTRA_FORC]);
    new QListViewItem(rootItem, category[EXTRA_SCF]);
    new QListViewItem(rootItem, category[EXTRA_INTE]);
    new QListViewItem(rootItem, category[EXTRA_PVM]);
    new QListViewItem(rootItem, category[EXTRA_MAIN]);
  ///// Debug
  new QListViewItem(ListViewCategory, category[DEBUG1]);
  ///// PVM
  new QListViewItem(ListViewCategory, category[PVM]);
  ///// SCF Convergence
  new QListViewItem(ListViewCategory, category[SCFCONVERGENCE]);
  ///// Properties
  rootItem = new QListViewItem(ListViewCategory, category[PROPERTIES]);
    new QListViewItem(rootItem, category[PROPERTIES_OTHER]);
    new QListViewItem(rootItem, category[PROPERTIES_CHARGES]);
  ///// Advanced
  rootItem = new QListViewItem(ListViewCategory, category[ADVANCED]);
    new QListViewItem(rootItem, category[ADVANCED_OTHER]);
    new QListViewItem(rootItem, category[ADVANCED_SYMMETRY]);
    new QListViewItem(rootItem, category[ADVANCED_METHOD]);
  ///// Basic
  rootItem = new QListViewItem(ListViewCategory, category[BASIC]);
  ListViewCategory->setSelected(rootItem, true);
  //rootItem = new QListViewItem(ListViewCategory, category[BASIC]);
  //  new QListViewItem(rootItem, category[BASIC_BASISSET]);
  //  new QListViewItem(rootItem, category[BASIC_METHOD]);
  //  rootItem->setOpen(true);
  //  ///// select Method (signals are not connected yet)
  //  ListViewCategory->setSelected(rootItem->itemBelow(), true);
  ///// Done
  rootItem = 0;
  ///// calculate the needed width of the listview
  QListViewItemIterator it(ListViewCategory);
  int minWidth = 0;
  while(it.current())
  {
    int itemWidth = it.current()->width(fontMetrics(), ListViewCategory, 0);
    if(itemWidth > minWidth)
      minWidth = itemWidth;
    it++;
  }
  ListViewCategory->setFixedWidth(minWidth + 2*ListViewCategory->treeStepSize());

  ///// add validators to LineEdit widgets
  ///// a double value which should fit into a 10 character field
  // positive or negative
  QDoubleValidator* v = new QDoubleValidator(-99999999.0,999999999.,9,this);
  LineEditFIELx->setValidator(v);
  LineEditFIELy->setValidator(v);
  LineEditFIELz->setValidator(v);
  LineEditLVSH->setValidator(v);
  // positive only
  v = new QDoubleValidator(0.0,999999999.,9,this);
  LineEditSCRF1->setValidator(v);
  LineEditSCRF2->setValidator(v);
  LineEditIPOL->setValidator(v);
  LineEditLOCA->setValidator(v);
  LineEditDIIS->setValidator(v);
  LineEditThreINTEa->setValidator(v);
  LineEditThreINTEb->setValidator(v);
  LineEditThreSCF1a->setValidator(v);
  LineEditThreSCF1b->setValidator(v);
  LineEditThreSCF2a->setValidator(v);
  LineEditThreSCF2b->setValidator(v);
  LineEditSthrSCFa->setValidator(v);
  LineEditSthrSCFb->setValidator(v);
  v = 0;  
  
  ///// set icons
  ToolButtonSTAR->setIconSet(IconSets::getIconSet(IconSets::Open));
  ToolButtonBAS1->setIconSet(IconSets::getIconSet(IconSets::ArrowDown));
  ToolButtonBAS2->setIconSet(IconSets::getIconSet(IconSets::ArrowUp));
  ToolButtonNOTHadd->setIconSet(IconSets::getIconSet(IconSets::ArrowLeft));
  ToolButtonNOTHremove->setIconSet(IconSets::getIconSet(IconSets::ArrowRight));
  ToolButtonAddMain->setIconSet(IconSets::getIconSet(IconSets::New));
  ToolButtonRemoveMain->setIconSet(IconSets::getIconSet(IconSets::Cut));
  ToolButtonClearMain->setIconSet(IconSets::getIconSet(IconSets::Clear));
  ToolButtonAddPVM->setIconSet(IconSets::getIconSet(IconSets::New));
  ToolButtonRemovePVM->setIconSet(IconSets::getIconSet(IconSets::Cut));
  ToolButtonClearPVM->setIconSet(IconSets::getIconSet(IconSets::Clear));
  ToolButtonAddINTE->setIconSet(IconSets::getIconSet(IconSets::New));
  ToolButtonRemoveINTE->setIconSet(IconSets::getIconSet(IconSets::Cut));
  ToolButtonClearINTE->setIconSet(IconSets::getIconSet(IconSets::Clear));
  ToolButtonAddSCF->setIconSet(IconSets::getIconSet(IconSets::New));
  ToolButtonRemoveSCF->setIconSet(IconSets::getIconSet(IconSets::Cut));
  ToolButtonClearSCF->setIconSet(IconSets::getIconSet(IconSets::Clear));
  ToolButtonAddFORC->setIconSet(IconSets::getIconSet(IconSets::New));
  ToolButtonRemoveFORC->setIconSet(IconSets::getIconSet(IconSets::Cut));
  ToolButtonClearFORC->setIconSet(IconSets::getIconSet(IconSets::Clear));
  ///// assign pixmaps
  CheckBoxSYMMx->setPixmap(IconSets::getPixmap(IconSets::SymmX));
  CheckBoxSYMMy->setPixmap(IconSets::getPixmap(IconSets::SymmY));
  CheckBoxSYMMz->setPixmap(IconSets::getPixmap(IconSets::SymmZ));
  CheckBoxSYMMxy->setPixmap(IconSets::getPixmap(IconSets::SymmXY));
  CheckBoxSYMMxz->setPixmap(IconSets::getPixmap(IconSets::SymmXZ));
  CheckBoxSYMMyz->setPixmap(IconSets::getPixmap(IconSets::SymmYZ));
  CheckBoxSYMMxyz->setPixmap(IconSets::getPixmap(IconSets::SymmXYZ));
  
  ///// load comboboxes with atom types and basissets
  fillComboBoxes();
  ///// disable the PVM widgets (cannot be enabled again if disabled in designer)
  GroupBoxPVM1->setEnabled(false);
  GroupBoxPVM2->setEnabled(false);
  GroupBoxPVM3->setEnabled(false);  
  ///// make the dialog as small as possible
  resize(1,1);
  ///// set widget and data to defaults
  reset();
  saveWidgets();
  widgetChanged = false;
}

///// fillComboBoxes //////////////////////////////////////////////////////////
void BraboBase::fillComboBoxes()
/// Fills the comboboxes with available basissets and atom types. Called once from init.
{
  ///// fill ComboBoxBAS1 with basissets
  ComboBoxBAS1->clear();
  { for(unsigned int i = 0; i < Basisset::maxBasissets(); i++)
      ComboBoxBAS1->insertItem(Basisset::numToBasis(i)); }
  ///// fill ComboBoxBAS2 with atom types
  ComboBoxBAS2->clear();
  { for(unsigned int i = 1; i <= AtomSet::maxElements; i++)
      ComboBoxBAS2->insertItem(AtomSet::numToAtom(i)); }
  ///// fill ComboBoxBAS3 with basissets
  ComboBoxBAS3->clear();
  { for(unsigned int i = 0; i < Basisset::maxBasissets(); i++)
      ComboBoxBAS3->insertItem(Basisset::numToBasis(i)); }
}

///// saveWidgets /////////////////////////////////////////////////////////////
void BraboBase::saveWidgets()
/// Saves the status of the widgets to a WidgetData struct.
{
  ///// Basic Method
  data.SCFMethod = ComboBoxSCFMethod->currentItem();
  data.SCFType = ComboBoxSCFType->currentItem();
  data.useStartvector = CheckBoxSTAR->isOn();
  if(data.useStartvector)
    data.startvector = LineEditSTAR->text();
  data.preferStartvector = CheckBoxSTAR2->isOn();
  data.charge = SpinBoxCHAR->value();

  ///// Basic Basisset
  data.useOneBasisset = RadioButtonBAS1->isChecked();
  data.basisset1 = ComboBoxBAS1->currentItem();
  data.basissetAtom = ComboBoxBAS2->currentItem();
  data.basisset2 = ComboBoxBAS3->currentItem();
  data.listAtoms.clear();
  data.listBasissets.clear();
  QListViewItemIterator it(ListViewBAS);
  for( ; it.current(); ++it)
  {
    data.listAtoms.push_back(AtomSet::atomToNum(it.current()->text(0)));
    data.listBasissets.push_back(Basisset::basisToNum(it.current()->text(1)));
  }

  ///// Advanced Method
  if(RadioButtonMP2E1->isChecked())
    data.MP2Type = 0;
  else if(RadioButtonMP2E2->isChecked())
    data.MP2Type = 1;
  else if(RadioButtonMP2E3->isChecked())
    data.MP2Type = 2;
  data.MP2ExcludeOcc = SpinBoxMP2E1->value();
  data.MP2ExcludeVirt = SpinBoxMP2E1->value();
  data.MP2Density = CheckBoxMP2D->isChecked();
  data.useField = CheckBoxFIEL->isChecked();
  data.fieldX = LineEditFIELx->text();
  data.fieldY = LineEditFIELy->text();
  data.fieldZ = LineEditFIELz->text();
  data.useSCRF = CheckBoxSCRF->isChecked();
  data.SCRFEpsilon = LineEditSCRF1->text();
  data.SCRFRadius = LineEditSCRF2->text();

  ///// Advanced Symmetry
  data.useSymmetry = CheckBoxSYMM->isChecked();
  data.useSymmAuto = RadioButtonSYMM1->isChecked();
  data.useSymmX = CheckBoxSYMMx->isChecked();
  data.useSymmY = CheckBoxSYMMy->isChecked();
  data.useSymmZ = CheckBoxSYMMz->isChecked();
  data.useSymmXY = CheckBoxSYMMxy->isChecked();
  data.useSymmYZ = CheckBoxSYMMyz->isChecked();
  data.useSymmXZ = CheckBoxSYMMxz->isChecked();
  data.useSymmXYZ = CheckBoxSYMMxyz->isChecked();

  ///// Advanced Other
  data.valueIpol = LineEditIPOL->text();
  data.useSkpc = CheckBoxSKPC->isChecked();
  data.printGeop = CheckBoxGEOP->isChecked();
  data.printWF = CheckBoxPRWF->isChecked();
  data.printDM = CheckBoxPRDM->isChecked();
  data.useDST = CheckBoxDST->isChecked();
  data.useACD = CheckBoxACD->isChecked();
  data.saveF11 = CheckBoxF11->isChecked();
  data.useFato = CheckBoxFATO->isChecked();
  data.numFato = SpinBoxFATO->value();
  data.useMIAForce = CheckBoxMIAForc->isChecked();

  ///// Properties Charges
  data.useMulliken = CheckBoxMULL1->isChecked();
  data.MullikenNoOverlap = CheckBoxMULL2->isChecked();
  data.MullikenEachIter = CheckBoxMULL3->isChecked();
  data.useStock = CheckBoxSTOCK->isChecked();
  if(RadioButtonStockDENS1->isChecked())
    data.stockType = 0;
  else if(RadioButtonStockDENS2->isChecked())
    data.stockType = 1;
  else if(RadioButtonStockDENS3->isChecked())
    data.stockType = 2;
  else if(RadioButtonStockDENS4->isChecked())
    data.stockType = 3;
  else if(RadioButtonStockDENS5->isChecked())
    data.stockType = 4;
  data.stockTotalDensity = ComboBoxStockMOLD->currentItem() && 1;
  data.stockTransition1 = SpinBoxStockTRDE1->value();
  data.stockTransition2 = SpinBoxStockTRDE2->value();
  data.useStockElmo = CheckBoxStockELMO->isChecked();
  data.useStockEpar = CheckBoxStockEPAR->isChecked();

  ///// Properties Other
  data.useBoys = CheckBoxLOCA->isChecked();
  data.numBoysIter = SpinBoxLOCA->value();
  data.BoysThreshold = LineEditLOCA->text();
  data.listNucl = LineEditNUCL->text();
  data.useElmo = CheckBoxELMO->isChecked();
  data.useExit = CheckBoxEXIT->isChecked();
  data.useJab = CheckBoxJAB->isChecked();

  ///// SCF Convergence
  data.numIter = SpinBoxITER->value();
  data.useJacobi = CheckBoxJACO->isChecked();
  data.useDIIS = !CheckBoxDIIS->isChecked();
  data.DIISThre = LineEditDIIS->text();
  data.numDIISCoeff = SpinBoxDIIS->value();
  data.valueLvsh = LineEditLVSH->text();
  data.useDlvs = CheckBoxDLVS->isChecked();
  data.thresholdINTEa = LineEditThreINTEa->text();
  data.thresholdINTEb = LineEditThreINTEb->text();
  data.thresholdMIAa = LineEditThreSCF1a->text();
  data.thresholdMIAb = LineEditThreSCF1b->text();
  data.thresholdSCFa = LineEditThreSCF2a->text();
  data.thresholdSCFb = LineEditThreSCF2b->text();
  data.thresholdOverlapa = LineEditSthrSCFa->text();
  data.thresholdOverlapb = LineEditSthrSCFb->text();
  data.useVarThreMIA = CheckBoxVTHR->isChecked();

  ///// PVM
  data.usePVM = CheckBoxPVM->isChecked();
  data.PVMExcludeHosts.clear();
  for(unsigned int i = 0; i<ListBoxNOTH1->count(); i++)
    data.PVMExcludeHosts += ListBoxNOTH1->text(i);
  data.PVMNumShells = SpinBoxNCST->value();
  data.PVMNumTasks = SpinBoxNTAS->value();
  data.notOnMaster = CheckBoxNOSE->isChecked();
  data.valueNice = SpinBoxNICE->value();
  data.usePacked = CheckBoxPACK->isChecked();

  ///// Debug
  data.printDebugPVM = CheckBoxDebugPVM->isChecked();
  data.printDebugFORC = CheckBoxDebugFORC->isChecked();
  data.goonINTE = CheckBoxGoonINTE->isChecked();
  data.goonSCF = CheckBoxGoonSCF->isChecked();
  data.printIntegrals = CheckBoxIntINTE->isChecked();
  data.printShells = CheckBoxShellsINTE->isChecked();
  if(CheckBoxPrintSCF3->isChecked())
    data.levelPrintSCF = 3;
  else if(CheckBoxPrintSCF2->isChecked())
    data.levelPrintSCF = 2;
  else if(CheckBoxPrintSCF1->isChecked())
    data.levelPrintSCF = 1;
  else
    data.levelPrintSCF = 0;
  data.numVirtual = SpinBoxPrintSCF->value();

  ///// Extra
  saveTable(TableMain, data.numLinesExtraMain, data.hPosExtraMain, data.vPosExtraMain, data.contentsExtraMain);
  saveTable(TablePVM,  data.numLinesExtraPVM,  data.hPosExtraPVM,  data.vPosExtraPVM,  data.contentsExtraPVM);
  saveTable(TableINTE, data.numLinesExtraINTE, data.hPosExtraINTE, data.vPosExtraINTE, data.contentsExtraINTE);
  saveTable(TableSCF,  data.numLinesExtraSCF,  data.hPosExtraSCF,  data.vPosExtraSCF,  data.contentsExtraSCF);
  saveTable(TableFORC, data.numLinesExtraFORC, data.hPosExtraFORC, data.vPosExtraFORC, data.contentsExtraFORC);
}

///// restoreWidgets //////////////////////////////////////////////////////////
void BraboBase::restoreWidgets()
/// Restores the status of the widgets from a WidgetData struct.
{
  ///// Basic Method
  ComboBoxSCFMethod->setCurrentItem(data.SCFMethod);
  ComboBoxSCFType->setCurrentItem(data.SCFType);
  CheckBoxSTAR->setChecked(data.useStartvector);
  if(data.useStartvector)
    LineEditSTAR->setText(data.startvector);
  CheckBoxSTAR2->setChecked(data.preferStartvector);
  SpinBoxCHAR->setValue(data.charge);

  ///// Basic Basisset
  RadioButtonBAS1->setChecked(data.useOneBasisset);
  ComboBoxBAS1->setCurrentItem(data.basisset1);
  ComboBoxBAS2->setCurrentItem(data.basissetAtom);
  ComboBoxBAS3->setCurrentItem(data.basisset2);
  ListViewBAS->clear();
  for (unsigned int i = 0; i < data.listAtoms.size(); i++)
    new QListViewItem(ListViewBAS, AtomSet::numToAtom(data.listAtoms[i]), Basisset::numToBasis(data.listBasissets[i]));

  ///// Advanced Method
  switch(data.MP2Type)
  {
    case 0: RadioButtonMP2E1->setChecked(true);
            break;
    case 1: RadioButtonMP2E2->setChecked(true);
            break;
    case 2: RadioButtonMP2E3->setChecked(true);
  }
  SpinBoxMP2E1->setValue(data.MP2ExcludeOcc);
  SpinBoxMP2E2->setValue(data.MP2ExcludeVirt);
  CheckBoxMP2D->setChecked(data.MP2Density);
  CheckBoxFIEL->setChecked(data.useField);
  LineEditFIELx->setText(data.fieldX);
  LineEditFIELy->setText(data.fieldY);
  LineEditFIELz->setText(data.fieldZ);
  CheckBoxSCRF->setChecked(data.useSCRF);
  LineEditSCRF1->setText(data.SCRFEpsilon);
  LineEditSCRF2->setText(data.SCRFRadius);

  ///// Advanced Symmetry
  CheckBoxSYMM->setChecked(data.useSymmetry);
  RadioButtonSYMM1->setChecked(data.useSymmAuto);
  CheckBoxSYMMx->setChecked(data.useSymmX);
  CheckBoxSYMMy->setChecked(data.useSymmY);
  CheckBoxSYMMz->setChecked(data.useSymmZ);
  CheckBoxSYMMxy->setChecked(data.useSymmXY);
  CheckBoxSYMMyz->setChecked(data.useSymmYZ);
  CheckBoxSYMMxz->setChecked(data.useSymmXZ);
  CheckBoxSYMMxyz->setChecked(data.useSymmXYZ);

  ///// Advanced Other
  LineEditIPOL->setText(data.valueIpol);
  CheckBoxSKPC->setChecked(data.useSkpc);
  CheckBoxGEOP->setChecked(data.printGeop);
  CheckBoxPRWF->setChecked(data.printWF);
  CheckBoxPRDM->setChecked(data.printDM);
  CheckBoxDST->setChecked(data.useDST);
  CheckBoxACD->setChecked(data.useACD);
  CheckBoxF11->setChecked(data.saveF11);
  CheckBoxFATO->setChecked(data.useFato);
  SpinBoxFATO->setValue(data.numFato);
  CheckBoxMIAForc->setChecked(data.useMIAForce);

  ///// Properties Charges
  CheckBoxMULL1->setChecked(data.useMulliken);
  CheckBoxMULL2->setChecked(data.MullikenNoOverlap);
  CheckBoxMULL3->setChecked(data.MullikenEachIter);
  CheckBoxSTOCK->setChecked(data.useStock);
  switch(data.stockType)
  {
    case 0: RadioButtonStockDENS1->setChecked(true);
            break;
    case 1: RadioButtonStockDENS2->setChecked(true);
            break;
    case 2: RadioButtonStockDENS3->setChecked(true);
            break;
    case 3: RadioButtonStockDENS4->setChecked(true);
            break;
    case 4: RadioButtonStockDENS5->setChecked(true);
  }
  if(data.stockTotalDensity)
    ComboBoxStockMOLD->setCurrentItem(1);
  else
    ComboBoxStockMOLD->setCurrentItem(0);
  SpinBoxStockTRDE1->setValue(data.stockTransition1);
  SpinBoxStockTRDE2->setValue(data.stockTransition2);
  CheckBoxStockELMO->setChecked(data.useStockElmo);
  CheckBoxStockEPAR->setChecked(data.useStockEpar);

  ///// Properties Other
  CheckBoxLOCA->setChecked(data.useBoys);
  SpinBoxLOCA->setValue(data.numBoysIter);
  LineEditLOCA->setText(data.BoysThreshold);
  LineEditNUCL->setText(data.listNucl);
  CheckBoxELMO->setChecked(data.useElmo);
  CheckBoxEXIT->setChecked(data.useExit);
  CheckBoxJAB->setChecked(data.useJab);

  ///// SCF Convergence
  SpinBoxITER->setValue(data.numIter);
  CheckBoxJACO->setChecked(data.useJacobi);
  CheckBoxDIIS->setChecked(!data.useDIIS);
  LineEditDIIS->setText(data.DIISThre);
  SpinBoxDIIS->setValue(data.numDIISCoeff);
  LineEditLVSH->setText(data.valueLvsh);
  CheckBoxDLVS->setChecked(data.useDlvs);
  LineEditThreINTEa->setText(data.thresholdINTEa);
  LineEditThreINTEb->setText(data.thresholdINTEb);
  LineEditThreSCF1a->setText(data.thresholdMIAa);
  LineEditThreSCF1b->setText(data.thresholdMIAb);
  LineEditThreSCF2a->setText(data.thresholdSCFa);
  LineEditThreSCF2b->setText(data.thresholdSCFb);
  LineEditSthrSCFa->setText(data.thresholdOverlapa);
  LineEditSthrSCFb->setText(data.thresholdOverlapb);
  CheckBoxVTHR->setChecked(data.useVarThreMIA);

  ///// PVM
  CheckBoxPVM->setChecked(data.usePVM);
  ListBoxNOTH1->clear();
  for(QStringList::Iterator it = data.PVMExcludeHosts.begin(); it != data.PVMExcludeHosts.end(); ++it)
    ListBoxNOTH1->insertItem(*it);
  SpinBoxNCST->setValue(data.PVMNumShells);
  SpinBoxNTAS->setValue(data.PVMNumTasks);
  CheckBoxNOSE->setChecked(data.notOnMaster);
  SpinBoxNICE->setValue(data.valueNice);
  CheckBoxPACK->setChecked(data.usePacked);

  ///// Debug
  CheckBoxDebugPVM->setChecked(data.printDebugPVM);
  CheckBoxDebugFORC->setChecked(data.printDebugFORC);
  CheckBoxGoonINTE->setChecked(data.goonINTE);
  CheckBoxGoonSCF->setChecked(data.goonSCF);
  CheckBoxIntINTE->setChecked(data.printIntegrals);
  CheckBoxShellsINTE->setChecked(data.printShells);
  CheckBoxPrintSCF1->setChecked(data.levelPrintSCF > 0);
  CheckBoxPrintSCF2->setChecked(data.levelPrintSCF > 1);
  CheckBoxPrintSCF3->setChecked(data.levelPrintSCF > 2);
  SpinBoxPrintSCF->setValue(data.numVirtual);

  ///// Extra
  restoreTable(TableMain, data.numLinesExtraMain, data.hPosExtraMain, data.vPosExtraMain, data.contentsExtraMain);
  restoreTable(TablePVM,  data.numLinesExtraPVM,  data.hPosExtraPVM,  data.vPosExtraPVM,  data.contentsExtraPVM);
  restoreTable(TableINTE, data.numLinesExtraINTE, data.hPosExtraINTE, data.vPosExtraINTE, data.contentsExtraINTE);
  restoreTable(TableSCF,  data.numLinesExtraSCF,  data.hPosExtraSCF,  data.vPosExtraSCF,  data.contentsExtraSCF);
  restoreTable(TableFORC, data.numLinesExtraFORC, data.hPosExtraFORC, data.vPosExtraFORC, data.contentsExtraFORC);
}

///// parsedNUCLlines /////////////////////////////////////////////////////////
QStringList BraboBase::parsedNUCLlines()
/// Returns a stringlist containing input lines
/// for Brabo constructed from the NUCL options.
{
  QStringList result;
  QStringList tempList = QStringList::split(",", LineEditNUCL->text()); 
  
  for(QStringList::Iterator it = tempList.begin(); it != tempList.end(); it++)
  {
    QString entry = *it;
    if(entry.contains("-"))
    {
      ///// this string is of the form 'a-b', so generate multiple entries
      int startValue = entry.section("-", 0, 0, QString::SectionSkipEmpty).toInt();
      int endValue = entry.section("-", 1, 1, QString::SectionSkipEmpty).toInt();
      if(startValue < 1)
        startValue = 1;
      if(endValue < startValue)
        endValue = startValue;
      for(int i = startValue; i <= endValue; i++)
        result += inputLine("nucl", i);
    }
    else
    {
      ///// this string contains a single value
      int value = entry.toInt();
      if(value > 0)
        result += inputLine("nucl", value);
    }
  }
  return result;
}

///// buildAtdens /////////////////////////////////////////////////////////////
void BraboBase::buildAtdens()
/// Generates atomic density basis set files for each atom of each basis set using 
/// atomic SCF. 
/// \warning This routine is only to be used during development and never during
///          normal operation. Maybe move it into a separate application...
{
  qDebug("*** generating atomic density basis set files ***");
  ///// fill a vector with the atomic numbers that AtomSCF is configured for.
  vector<unsigned int> atomSCFAtoms;
  for(unsigned int i = 1; i <= 20; i++) 
    atomSCFAtoms.push_back(i);
  atomSCFAtoms.push_back(27);
  for(unsigned int i = 31; i <= 35; i++) 
    atomSCFAtoms.push_back(i);
  atomSCFAtoms.push_back(45);
  atomSCFAtoms.push_back(53);
  
  ///// create a working directory
  QDir workDir("F:\\temp\\atdens"); // Windows
  //QDir workDir("/tmp/atdens"); // *nix
  if(!workDir.exists())
    workDir.mkdir(workDir.path());
  if(!QDir::setCurrent(workDir.path()))
  {
    qDebug("couldn't change into directory "+workDir.path());
    return;
  }

  ///// create a process for running the calculations
  QProcess* process = new QProcess(this);
  process->setWorkingDirectory(workDir);
  ///// loop over all basis sets
  for(unsigned int basis = 0; basis < Basisset::maxBasissets(); basis++)
  //unsigned int basis = 0; // 3-21G
  {
    ///// build the basisset directories
    QString basisDir = Basisset::numToBasisDir(basis);
    QString basisDirBase = basisDir;
    basisDirBase.remove("p"); // same basis without polarisation. might have to be changed when new basis sets are added
    basisDir = Paths::basisset + QDir::separator() + basisDir;
    basisDirBase = Paths::basisset + QDir::separator() + basisDirBase;

    qDebug("building the atdens files for basis set "+Basisset::numToBasis(basis));
    qDebug("full basis dir = "+basisDir);
    qDebug("core basis dir = "+basisDirBase);

    ///// loop over all atoms for AtomSCF
    for(vector<unsigned int>::iterator it = atomSCFAtoms.begin(); it != atomSCFAtoms.end(); it++)
    {
      unsigned int atom = *it;
      QString basisFile = AtomSet::numToAtom(atom).stripWhiteSpace() + "." + Basisset::extension();

      ///// check whether the basis set file exists
      QFile basisFileCheck(basisDir + QDir::separator() + basisFile);
      if(!basisFileCheck.exists())
        continue;

      ///// check whether the atdens file already exists
      QFile atdens(basisDir + QDir::separator() + AtomSet::numToAtom(atom).stripWhiteSpace() + ".atdens");
      if(atdens.exists())
        continue;

      qDebug("  current atomic number = %d with basisset "+basisFile, atom);
      ///// generate standard input for AtomSCF_inp
      QString stdInput = "y\n" + basisFile + "\n" + QString::number(atom) + "\n";
      stdInput += basisDirBase + QDir::separator() + basisFile + "\ny\n";
      ///// run AtomSCF_inp
      process->clearArguments();
      process->addArgument(Paths::atomscf_inp);
      process->launch(stdInput);
      qDebug("    running atomscf_inp");
      while(process->isRunning())
        qApp->processEvents();
      if(!process->normalExit())
        continue; // try next atom

      ///// read the input for AtomSCF
      qDebug("    reading fort.1");
      QFile file1("fort.1"); // assume the working directory hasn't changed
      if(!file1.open(IO_ReadOnly))
        continue;
      QByteArray fort1 = file1.readAll();
      file1.close();
      ///// run AtomSCF
      process->clearArguments();
      process->addArgument(Paths::atomscf);
      process->launch(fort1);
      qDebug("    running atomscf");
      while(process->isRunning())
        qApp->processEvents();
      if(!process->normalExit())
        continue; // try next atom
      ///// save stdout to file fort.2
      qDebug("    reading fort.2");
      QByteArray fort2 = process->readStdout();
      QFile file2("fort.2");
      if(!file2.open(IO_WriteOnly))
        continue;
      file2.writeBlock(fort2);
      file2.close();

      ///// generate standard input for AtomSCF_o2d
      stdInput = "fort.2\n" + basisFile + "\n" + basisDir + "\nn\n"; // n = (no) extended format for the atdens file, but doesn't have any influence yet
      ///// run AtomSCF_o2d
      process->clearArguments();
      process->addArgument(Paths::atomscf_o2d);
      process->launch(stdInput);
      qDebug("    running atomscf_o2d");
      while(process->isRunning())
        qApp->processEvents();
      if(!process->normalExit())
        continue; // try next atom
      
      ///// copy the fort.9 to the desired atdens file
      QFile file9("fort.9");
      qDebug("    copying fort.9 to "+atdens.name());
      if(!file9.open(IO_ReadOnly) || !atdens.open(IO_WriteOnly))
        continue;
      atdens.writeBlock(file9.readAll());
      file9.close();
      atdens.close();
    }

  }
  qDebug("*** finished ***");
}

///////////////////////////////////////////////////////////////////////////////
///// Static Variables                                                    /////
///////////////////////////////////////////////////////////////////////////////

QStringList BraboBase::pvmHosts = QStringList(); ///< The list of available PVM hosts
unsigned int BraboBase::preferredBasisset = 0; ///< The preferred basisset
const BraboBase::XMLData BraboBase::xml = {
  ///// Basic Method
  "scf_method",
  "scf_type",
  "use_start_vector",
  "start_vector_file",
  "start_vector_file",
  "charge",
  ///// Basic Basis Set
  "use_one_basis_set",
  "basis_set_1",
  "basis_set_atom",
  "basis_set_2",
  "list_atoms",
  "list_basissets",
  ///// Advanced Method
  "mp2_type",
  "mp2_exclude_occupied",
  "mp2_exclude_virtual",
  "mp2_density",
  "use_field",
  "field_x",
  "field_y",
  "field_z",
  "use_scrf",
  "scrf_epsilon",
  "scrf_radius",
  ///// Advanced Symmetry
  "use_symmetry",
  "symmetry_auto",
  "symmetry_x__",
  "symmetry_y__",
  "symmetry_z__",
  "symmetry_xy_",
  "symmetry_yz_",
  "symmetry_xz_",
  "symmetry_xyz",
  ///// Advanced Other
  "ipol",
  "use_skpc",
  "print_geom",
  "print_wf",
  "print_dm",
  "use_dst",
  "use_acd",
  "save_f11",
  "use_fato",
  "fato_number",
  "use_mia_force",
  ///// Properties Charges
  "use_mulliken",
  "mulliken_no_overlap",
  "mulliken_each_iter",
  "use_stock",
  "stock_type",
  "stock_total_density",
  "stock_transition_1",
  "stock_transition_2",
  "use_stock_elmo",
  "use_stock_epar",
  ///// Properties Other
  "use_boys",
  "boys_iter",
  "boys_threshold",
  "nucl",
  "use_elmo",
  "use_exit",
  "use_jab",
  ///// SCF Convergence
  "scf_iter",
  "use_jacobi",
  "use_diis",
  "diis_thre",
  "diis_coeff",
  "lvsh",
  "use_dlvs",
  "thre_inte_a",
  "thre__inte_b",
  "thre_mia_a",
  "thre_mia_b",
  "thre_scf_a",
  "thre_scf_b",
  "thre_overlap_a",
  "thre_overlap_b",
  "use_vthr_mia",
  ///// PVM
  "use_pvm",
  "pvm_exclude_hosts",
  "pvm_shells",
  "pvm_tasks",
  "pvm_not_on_master",
  "pvm_nice",
  "pvm_packed",
  ///// Debug
  "debug_pvm",
  "debug_force",
  "goon_inte",
  "goon_scf",
  "print_integrals",
  "print_shells",
  "scf_print_level",
  "scf_print_virtual",
  ///// Extra
  "extra_main_lines",
  "extra_pvm_lines",
  "extra_inte_lines",
  "extra_scf_lines",
  "extra_forc_lines",
  "extra_main_hpos",
  "extra_pvm_hpos",
  "extra_inte_hpos",
  "extra_scf_hpos",
  "extra_forc_hpos",
  "extra_main_vpos",
  "extra_pvm_vpos",
  "extra_inte_vpos",
  "extra_scf_vpos",
  "extra_forc_vpos",
  "extra_main_contents",
  "extra_pvm_contents",
  "extra_inte_contents",
  "extra_scf_contents",
  "extra_forc_contents"
};


