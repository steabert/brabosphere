/***************************************************************************
                        calculation.cpp  -  description
                             -------------------
    begin                : Thu April 28 2005
    copyright            : (C) 2005-2006 by Ben Swerts
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
  \class Calculation
  \brief This class handles running a Brabo calculation.

  It assembles and writes the needed input files and calls the right programs
  at the right times or sets up a script file to do the work. Results are read
  and signalled to the host class XbraboView.

  Single Point Energy / Energy & Forces
  -------------------------------------
   - brabo (molecule.inp, basisset checks)
  [- stock (molecule.stin, molecule.atdens) ]
  [- achar (stdin) ]
  [- buur (molecule.bin) ]

  Geometry Optimization
  ---------------------
   - brabo (molecule.inp, basisset checks)
  [- stock (molecule.stin, molecule.atdens) ]
  [- achar (stdin) ]
  [- buur (molecule.bin) ]
   - relax (molecule.aff, stdin)
  [- maff (stdin) ]
  [- cnvrtaff (stdin) ]

*/
/// \file
/// Contains the implementation of the class Calculation

///// Header files ////////////////////////////////////////////////////////////

// C++ header files
#include <cassert>
#include <iostream>

// Qt header files
#include <qapplication.h>
#include <qdir.h>
#include <qdom.h>
#include <qfile.h>
#include <qmessagebox.h>
#include <qprocess.h>
#include <qtimer.h>
#include <qurloperator.h>
#include <qwidget.h>

// Xbrabo header files
#include "atomset.h"
#include "calculation.h"
#include "crdfactory.h"
#include "domutils.h"
#include "globalbase.h"
#include "paths.h"

///////////////////////////////////////////////////////////////////////////////
///// Public Member Functions                                             /////
///////////////////////////////////////////////////////////////////////////////

///// constructor /////////////////////////////////////////////////////////////
Calculation::Calculation(AtomSet* atomset, QWidget* parent, const char* name) : QObject(parent, name),
atoms(atomset),
parentWidget(parent),
braboInput(QStringList()),
stockInput(QStringList()),
relaxInput(QStringList()),
basissetList(QStringList()),
startVectorFile(QString::null),
atdensFile(QString::null),
affUpdateFreq(0),
calculationType(0),
crystalType(0),
updateBraboInput(true),
updateStockInput(false),
updateRelaxInput(false),
doBasissetCheck(true),
copyAtdens(false),
calcRunning(false),
calcPaused(false),
calcName(QString::null),
calcDir(QString::null),
calcProcess(0),
stopRequested(false),
backupFrequency(0),
calcModified(false),
calcError(NoError),
calcContinuable(false)
/// The default constructor.
{
  assert(atomset != 0);
}

///// destructor //////////////////////////////////////////////////////////////
Calculation::~Calculation()
/// The default destructor.
{

}

///// start ///////////////////////////////////////////////////////////////////
bool Calculation::start()
/// Starts the calculation. If certain prerequisites are not fulfilled, 
/// this function returns false and the calculation will not be started.
{
  ///// check whether the calculation is already started.
  if(calcRunning)
  {
    if(!calcPaused)
    {
      //QMessageBox::warning(parentWidget, tr("Start Calculation") , tr("Only 1 calculation can be run at a time"));
      return false;
    }
    else
    {
      calcPaused = false;
      doNextStep();
      return true;
    }
  }
  ///// check for the presence of a BRABO input file => programming error if it is not
  assert(!braboInput.isEmpty());
  ///// check for the presence of a Relax input file -> programming error if it is not
  if(calculationType == GlobalBase::GeometryOptimization)
    assert(!relaxInput.isEmpty());
  ///// verify the existence of the basisset files in basissetList
  if(doBasissetCheck && !checkBasissets())
    return false;

  ///// verify the locations of the executables
  if(!checkExecutables())
    return false;

  ///// switch to the calculation directory
  if(!makeDirCurrent(calcDir, tr("Start Calculation")))
    return false;

  ///// copy the start vector to the calculation directory if needed
  if(!copyStartVector())
    return false;

  ///// write the starting coordinates if needed
  bool calcContinue = false;
  QFile fileCRD(calcName + ".crd");
  if(calculationType == GlobalBase::GeometryOptimization && calcContinuable && fileCRD.exists())
  {
    // the optimization can possibly be continued from the current or previous cycle
    calcContinue = QMessageBox::question(parentWidget, tr("Geometry optimization"), 
      tr("The geometry optimization can possibly be continued."), //\nYou can choose to try to continue from the current cycle\nor the previous one."), 
      tr("Current cycle"), tr("Restart")) == 0;
  }
  if(!calcContinue && CrdFactory::writeToFile(atoms, calcName + ".crd", calcXF) != CrdFactory::OK)
  {
    QMessageBox::warning(parentWidget, tr("Start Calculation"), tr("Unable to write the initial coordinates"));
    return false;
  }

  ///// delete the .c00 if needed
  if(calculationType == GlobalBase::GeometryOptimization)
  {
    QFile optcrd(calcName + ".c00");
    if(optcrd.exists())
    {
      if(!optcrd.remove())
      {
        QMessageBox::warning(parentWidget, tr("Start calculation"), tr("The existing .c00 file could not be removed."));
        return false;
      }
    }
  }
  
  ///// delete saved outputs from previous calculations with the same name
  ///// don't bomb on errors, though
  QDir dir(calcDir);
  QStringList files = dir.entryList(calcName + "_*"); // all files named molecule_* 
  QFile file;
  for(QStringList::Iterator it = files.begin(); it != files.end(); it++)
  {
    file.setName(*it);
    file.remove();
  }

  ///// everything is OK, so setup the execution order of the programs
  // NOTE: after BRABO, STOCK and (MAFF,RELAX) can execute in parallel
  // NOTE: Always add all steps and decide whether or not to run them in doNextStep
  calculationSteps.clear();
  calculationSteps.push_back(BRABO);
  calculationSteps.push_back(STOCK);
  calculationSteps.push_back(UPDATE);
  if(calculationType == GlobalBase::GeometryOptimization)
  {
    calculationSteps.push_back(MAFF);
    calculationSteps.push_back(CNVRTAFF);
    calculationSteps.push_back(RELAX);
  }
  // add this step last
  calculationSteps.push_back(NEW_CYCLE);

  stopRequested = false;  
  calcRunning = true;
  currentCycle = 0;
  calcError = NoError;

  // start the calculation by invoking the dispatcher
  doNextStep();
  return true;
}

///// pause ///////////////////////////////////////////////////////////////////
bool Calculation::pause()
/// Pauses the calculation. If the calculation is running, it will be paused 
/// before the start of the next program or step. This function will return 
/// false if the calculation consists of only one step or when the calculation
/// is not running.
{
  // return if the calculation is not running
  if(!calcRunning)
    return false;

  // start if calculation is already paused
  if(calcPaused)
    return start();

  // return if only 1 step is performed
  if(calculationSteps.size() == 2) 
    return false;

  calcPaused = true;
  setModified();
  return true;
}

///// stop ////////////////////////////////////////////////////////////////////
bool Calculation::stop()
/// Stops the calculation. A dialog is popped up asking whether the calculation
/// should be stopped immediately or at the start of the next program or step.
/// It returns false of the calculation is not running or the dialog is cancelled.
/// \note Maybe show the dialog in XbraboView and add an argument to stop:
/// bool stop(const bool kill = false);
{
  if(!calcRunning)
    return false;
  
  if(calcPaused) // for loading a paused calculation from disk
  {
    calcRunning = false;
    if(calcError == NoError)
      calcError = UndefinedError;
    doNextStep();
    return true;
  }

  int result = QMessageBox::information(parentWidget, tr("Stop calculation"), tr("Please specify how the calculation should be stopped."), 
               tr("After the current step"), tr("Immediately"), tr("Cancel"), 0, 2);
  switch(result)
  {
    case 0: // stop the calculation after the current step has ended
            stopRequested = true; 
            setModified();
            break;
    case 1: // stop the calculation immediately
            if(calcProcess != 0)
              calcProcess->kill();
            calcRunning = false;
            if(calcError == NoError)
              calcError = ManualStop;
            doNextStep();
            break;
    case 2: // cancel stopping
            return false;
  }
  return true;
}

///// setModified /////////////////////////////////////////////////////////////
void Calculation::setModified(const bool status)
/// Sets the 'modified' status of the calculation
{
  if(status && !calcModified)
    emit modified();
  calcModified = status;
}

///// setBraboInput ///////////////////////////////////////////////////////////
void Calculation::setBraboInput(const QStringList stdInput, const QStringList basissets, const QString startVector, const bool preferStartVector, const unsigned int startVectorSize1, const unsigned int startVectorSize2)
/// Updates the contents of the standard input for BRABO. The new input
/// is used for the next invocation of the program, also when the calculation
/// is already running. If \c basissets is different from the existing one, the list of basisset files
/// has changed and should be checked for existence before the next BRABO invocation.
/// If the calculation is not running and \c startVector is not empty, this file will be copied
/// to the calculation directory at the beginning of the calculation.
/// If preferStartVector is set, the existing file will be preferred over the new given file.
/// startVectorSize1 and 2 define possible file sizes of valid starting vectors. 
{
  ///// Do not update when the calculation is running and BRABO will only be called once.
  ///// When the calculation is restarted, start() will always call this routine again.
  if(calcRunning && (calculationType == GlobalBase::SinglePointEnergy || calculationType == GlobalBase::EnergyAndForces))
    return;

  ///// update the brabo input file
  if(!stdInput.isEmpty())
  {
    braboInput = stdInput;
    updateBraboInput = true;
  }

  ///// update the basisset list
  if(basissets != basissetList && !basissets.isEmpty())
  {
    basissetList = basissets;
    doBasissetCheck = true;
  }

  ///// update the starting vector
  // - if the calculation is not running, check the validity of any existing startvector
  //   with the right name if preferStartVector = true, else use the provided startvector
  //   (or none if none is specified)
  // - if the calculation already started, only update the startvector if the basisset
  //   changed. 
  qDebug("setBraboInput: calcRunning = %d, prefer = %d, size1 = %d, size2 = %d",calcRunning, preferStartVector, startVectorSize1, startVectorSize2);
  qDebug("               startVector = " + startVector + ", startVectorFile = " + startVectorFile);
  if(!calcRunning)
  {
    if(preferStartVector)
    {
      QFileInfo fi(calcDir + QDir::separator() + calcName + ".sta");
      if(fi.size() == startVectorSize1 || fi.size() == startVectorSize2) // will fail if no filename like this exists
        startVectorFile = calcDir + QDir::separator() + calcName + ".sta";
      else
      {
        fi.setFile(startVector);
        qDebug("size of " + startVector + " = %d", fi.size());
        if(fi.size() == startVectorSize1 || fi.size() == startVectorSize2) // will always return false if startVector is empty
          startVectorFile = startVector; 
        else
          startVectorFile = QString::null; // do not use a starting vector at the next step (initial step here)
      }
    }
    else
    {
      QFileInfo fi(startVector);
      if(fi.size() == startVectorSize1 || fi.size() == startVectorSize2)
        startVectorFile = startVector;
      else
        startVectorFile = QString::null;
    }
  }
  else if(doBasissetCheck)
  {
    QFileInfo fi(startVectorFile);
    if(fi.size() != startVectorSize1 && fi.size() != startVectorSize2)
    {
      // the calculation is running, the basis set has changed and the
      // current starting vector cannot be used anymore.
      // WARNING: this does not work when the calculation is doing the first BRABO 
      //          call without a starting vector and before the first iteration of
      //          the SCF has been completed (thus before a starting vector has been 
      //          written). In that case startVectorFile is still empty.
      fi.setFile(startVector);
      if(fi.size() == startVectorSize1 || fi.size() == startVectorSize2)
        startVectorFile = startVector; // use the given start vector 
      else
        startVectorFile = QString::null; // regenerate the starting vector    
    }
  }
  qDebug("chosen startvector = " + startVectorFile);
}

///// setStockInput ///////////////////////////////////////////////////////////
void Calculation::setStockInput(const QStringList stin, const QString atdens)
/// Updates the contents of the input file for STOCK. The new input
/// is used for the next invocation of the program, also when the calculation
/// is already running. If \c atdens is not empty, it will be copied to
/// the calculation directory too.
{
  if(!stin.isEmpty())
  {
    stockInput = stin;
    updateStockInput = true;
  }

  if(!atdens.isEmpty())
  {
    atdensFile = atdens;
    copyAtdens = true;
  }
}

///// setRelaxInput ///////////////////////////////////////////////////////////
void Calculation::setRelaxInput(const QStringList aff, const QStringList maff, const unsigned int updateFreq, const unsigned int maxSteps, const std::vector<unsigned int>& steps, const std::vector<double>& factors)
/// Updates the contents of the input file for RELAX. The new input
/// is used for the next invocation of the program, also when the calculation
/// is already running. 
/// \param[in] aff : The AFF header file
/// \param[in] maff: The input for running MAFF. This can contain input to generate
///                  BMAT or GBMA input. If it is empty, MAFF should never be called.
/// \param[in] updateFreq: if MAFF is to be called (maff not empty) this variable
///                        indicates the frequency with which the AFF should be 
///                        regenerated. If zero MAFF should only be called once
///                        at the beginning of the calculation.
/// \param[in] maxSteps: the maximum number of optimization cycles. If zero go on forever.
/// \param[in] steps: Contains info for the generation of the scale factor.
/// \param[in] factors: Contains info for the generation of the scale factor.
{
  if(!aff.isEmpty())
  {
    relaxInput = aff;
    updateRelaxInput = true;
    maffInput = maff;
  }
  affUpdateFreq = updateFreq;
  if(!steps.empty() && steps.size() == factors.size())
  {
    scaleSteps.clear();
    scaleSteps.reserve(steps.size());
    scaleSteps.assign(steps.begin(), steps.end());
    scaleFactors.clear();
    scaleFactors.reserve(factors.size());
    scaleFactors.assign(factors.begin(), factors.end());
  }
  calcMaxCycle = maxSteps;
}

///// setCalculationType //////////////////////////////////////////////////////
void Calculation::setCalculationType(const unsigned int type, const unsigned int buur)
/// Sets the type of calculation to be performed. 
/// \param[in] type : Possibilities are Single
/// Point Energy (& Forces), Geometry Optimization and Frequencies. This type
/// cannot be changed while a calculation is running.
/// \param[in] buur : Possibilities are None, PC and SM.
{
  if(!calcRunning)
  {
    calculationType = type;
    crystalType = buur;
  }
}

///// setParameters ///////////////////////////////////////////////////////////
void Calculation::setParameters(const QString name, const QString dir, const bool format)
/// Sets the global calculation parameters.
{
  calcName = name;
  calcDir = dir;
  calcXF = format;
}

///// setBackup ///////////////////////////////////////////////////////////////
void Calculation::setBackup(const unsigned int freq, const bool brabo, const bool stock, const bool relax, const bool aff, const bool crd)
/// Sets the backup frequency and which outputs should be backed up at one step.
{
  backupFrequency = freq;
  if(backupFrequency != 0)
  {
    backupBrabo = brabo;
    backupStock = stock;
    backupRelax = relax;
    backupAFF = aff;
    backupCRD = crd;
  }
  setModified();
}

///// setContinuable //////////////////////////////////////////////////////////
void Calculation::setContinuable(const bool status)
/// Updates whether an optimization can be continued after it has been stopped.
{
  calcContinuable = status;
}

///// isRunning ///////////////////////////////////////////////////////////////
bool Calculation::isRunning() const
/// Returns whether or not the calculation is running.
{
  return calcRunning;
}

///// isPaused ////////////////////////////////////////////////////////////////
bool Calculation::isPaused() const
/// Returns whether or not the calculation is paused.
{
  return calcPaused;
}

///// getRefinedParameters ////////////////////////////////////////////////////
void Calculation::getRefinementParameters(bool& largestCart, bool& magnCart, bool& largestInt, bool& magnInt, bool& largestShift)
/// Returns the 5 refinement criteria for the latest optimization cycle. 
/// The parameters are read from the calcName.aou file. If it is incomplete
/// false is returned for each criterium.
{
  largestCart = false;
  magnCart = false;
  largestInt = false;
  magnInt = false;
  largestShift = false;

  QFile file(calcDir + QDir::separator() + calcName + ".aou");
  if(!file.open(IO_ReadOnly))
    return;
  QTextStream stream(&file);
  while(!stream.atEnd())
  {
    QString line = stream.readLine();
    if(line.contains("OK"))
    {
      if(line.contains("LARGEST CARTESIAN FORCE"))
        largestCart = true;
      else if(line.contains("MAGNIT. CART. FORCE VEC"))
        magnCart = true;
      else if(line.contains("LARGEST INTERNAL FORCE"))
        largestInt = true;
      else if(line.contains("MAGNIT. INT. FORCE VEC"))
        magnInt = true;
      else if(line.contains("LARGEST SHIFT"))
        largestShift = true;
    }
  }
}

///// getAvailableOutputs /////////////////////////////////////////////////////
void Calculation::getAvailableOutputs(std::vector<unsigned int>& cycles, std::vector<bool>& brabo, std::vector<bool>& stock, std::vector<bool>& relax, std::vector<bool>& aff)
/// Returns the cycles for which outputs are available.
{
  cycles.clear();
  brabo.clear();
  stock.clear();
  relax.clear();
  aff.clear();

  bool out, stou, aou, aff2;
  QFile file;
  QString prefix = calcDir + QDir::separator() + calcName + "_";
  for(unsigned int i = 1; i <= currentCycle; i++)
  {
    QString prefixCycle = prefix + QString::number(i);
    file.setName(prefixCycle + ".out");
    out = file.exists();
    file.setName(prefixCycle + ".stou");
    stou = file.exists();
    file.setName(prefixCycle + ".aou");
    aou = file.exists();
    file.setName(prefixCycle + ".aff");
    aff2 = file.exists();
    if(out || stou || aou || aff2)
    {
      // at least one output exists so add a cycle
      cycles.push_back(i);
      brabo.push_back(out);
      stock.push_back(stou);
      relax.push_back(aou);
      aff.push_back(aff2);
    }
  }
}

///// braboOutput /////////////////////////////////////////////////////////////
QStringList Calculation::braboOutput(const unsigned int step)
/// Returns the output from Brabo for a certain step. If the step is zero, the latest
/// output is returned (possibly not saved).
{
  return output(".out", step);
}

///// stockOutput /////////////////////////////////////////////////////////////
QStringList Calculation::stockOutput(const unsigned int step)
/// Returns the output from Stock for a certain step. If the step is zero, the latest
/// output is returned (possibly not saved).
{
  return output(".stou", step);
}

///// relaxOutput /////////////////////////////////////////////////////////////
QStringList Calculation::relaxOutput(const unsigned int step)
/// Returns the output from Relax for a certain step. If the step is zero, the latest
/// output is returned (possibly not saved).
{
  return output(".aou", step);
}

///// affOutput ///////////////////////////////////////////////////////////////
QStringList Calculation::affOutput(const unsigned int step)
/// Returns the AFF file for a certain step. If the step is zero, the latest
/// version is returned (possibly not saved).
{
  return output(".aff", step);
}

///// loadCML /////////////////////////////////////////////////////////////////
void Calculation::loadCML(const QDomElement* root)
/// Loads the settings from file
{
  const QString prefix = "calculation_";
  QDomNode childNode = root->firstChild();
  while(!childNode.isNull())
  {
    if(childNode.isElement() && childNode.toElement().tagName() == "parameter")
    {
      if(DomUtils::dictEntry(childNode, prefix + "starting_vector"))
        DomUtils::readNode(&childNode, &startVectorFile);
      else if(DomUtils::dictEntry(childNode, prefix + "update_energy_and_forces"))
        DomUtils::readNode(&childNode, &updateBraboInput);
      else if(DomUtils::dictEntry(childNode, prefix + "update_stockholder"))
        DomUtils::readNode(&childNode, &updateStockInput);
      else if(DomUtils::dictEntry(childNode, prefix + "update_geometry_optimization"))
        DomUtils::readNode(&childNode, &updateRelaxInput);
      else if(DomUtils::dictEntry(childNode, prefix + "check_basissets"))
        DomUtils::readNode(&childNode, &doBasissetCheck);
      else if(DomUtils::dictEntry(childNode, prefix + "running"))
        DomUtils::readNode(&childNode, &calcRunning);
      else if(DomUtils::dictEntry(childNode, prefix + "paused"))
        DomUtils::readNode(&childNode, &calcPaused);
      else if(DomUtils::dictEntry(childNode, prefix + "current_cycle"))
        DomUtils::readNode(&childNode, &currentCycle);
      else if(DomUtils::dictEntry(childNode, prefix + "error"))
        DomUtils::readNode(&childNode, &calcError);
      else if(DomUtils::dictEntry(childNode, prefix + "continuable"))
        DomUtils::readNode(&childNode, &calcContinuable);
      else if(DomUtils::dictEntry(childNode, prefix + "steps"))
      {
        std::vector<unsigned int> steps;
        DomUtils::readNode(&childNode, &steps);
        calculationSteps.clear();
        for(unsigned int i = 0; i < steps.size(); i++)
        {
          switch(steps[i])
          {
            case NEW_CYCLE: calculationSteps.push_back(NEW_CYCLE);
                            break;
            case ACHAR:     calculationSteps.push_back(ACHAR);
                            break;
            case BRABO:     calculationSteps.push_back(BRABO);
                            break;
            case BUUR:      calculationSteps.push_back(BUUR);
                            break;
            case CNVRTAFF:  calculationSteps.push_back(CNVRTAFF);
                            break;
            case DISTOR:    calculationSteps.push_back(DISTOR);
                            break;
            case FORKON:    calculationSteps.push_back(FORKON);
                            break;
            case MAFF:      calculationSteps.push_back(MAFF);
                            break;
            case RELAX:     calculationSteps.push_back(RELAX);
                            break;
            case STOCK:     calculationSteps.push_back(STOCK);
                            break;
            case UPDATE:    calculationSteps.push_back(UPDATE);
                            break;
          }
        }
      }
    }
    childNode = childNode.nextSibling();
  }
}

///// saveCML /////////////////////////////////////////////////////////////////
void Calculation::saveCML(QDomElement* root)
/// Saves the settings to file 
{
  const QString prefix = "calculation_";
  DomUtils::makeNode(root, startVectorFile, prefix + "starting_vector");
  DomUtils::makeNode(root, updateBraboInput, prefix + "update_energy_and_forces");
  DomUtils::makeNode(root, updateStockInput, prefix + "update_stockholder");
  DomUtils::makeNode(root, updateRelaxInput, prefix + "update_geometry_optimization");
  DomUtils::makeNode(root, doBasissetCheck, prefix + "check_basissets");
  DomUtils::makeNode(root, calcRunning, prefix + "running");
  DomUtils::makeNode(root, calcPaused, prefix + "paused");
  DomUtils::makeNode(root, currentCycle, prefix + "current_cycle");
  DomUtils::makeNode(root, calcError, prefix + "error");
  DomUtils::makeNode(root, calcContinuable, prefix + "continuable");
  std::vector<unsigned int> calcSteps;
  for(std::list<CalculationStep>::iterator it = calculationSteps.begin(); it != calculationSteps.end(); it++)
    calcSteps.push_back(static_cast<unsigned int>(*it));
  DomUtils::makeNode(root, calcSteps, prefix + "steps");
}

///// writeInput //////////////////////////////////////////////////////////////
void Calculation::writeInput()
/// Writes all input files. This is only needed in case the calculation is to
/// be run outside of this program. When this function is called, all input
/// should already be present in the proper QStringLists.
{
  ///// switch to the calculation directory
  if(!makeDirCurrent(calcDir, tr("Write all input files")))
    return;

  ///// the Brabo input file
  QFile fileInp(calcName + ".inp");
  if(!fileInp.open(IO_WriteOnly))
  {
    QMessageBox::warning(parentWidget, tr("Write all input files"), tr("Unable to write the file") +
      "\n" + fileInp.name());
    return;
  }
  QTextStream streamInp(&fileInp);
  streamInp << QString(braboInput.join("\n")+"\n");
  fileInp.close();
  if(calcRunning)
    return; // when a calculation is running, all other files are already present

  ///// the coordinates
  if(CrdFactory::writeToFile(atoms, calcName + ".crd", calcXF) != CrdFactory::OK)
  {
    QMessageBox::warning(parentWidget, tr("Write all input files"), tr("Unable to write the file") + "\n" + calcName + ".crd");
    return;
  }

  ///// the starting vector
  if(!copyStartVector())
    return;

  ///// the Stock input file
  if(!stockInput.isEmpty())
  {
    QFile file(calcName + ".stin");
    if(!file.open(IO_WriteOnly))
    {
      QMessageBox::warning(parentWidget, tr("Write all input files"), tr("Unable to write the file") +
      "\n" + file.name());
      return;
    }
    QTextStream stream(&file);
    stream << QString(stockInput.join("\n")+"\n");
    fileInp.close();
  }

  ///// the MAFF input file
  if(!maffInput.isEmpty())
  {
    QFile file(calcName + ".maffinp");
    if(!file.open(IO_WriteOnly))
    {
      QMessageBox::warning(parentWidget, tr("Write all input files"), tr("Unable to write the file") +
      "\n" + file.name());
      return;
    }
    QTextStream stream(&file);
    stream << QString(maffInput.join("\n")+"\n");
    fileInp.close();
  }

  ///// the AFF (header) file
  if(!relaxInput.isEmpty())
  {
    QString extension = ".aff";
    if(!maffInput.isEmpty())
      extension += "1";
    QFile file(calcName + extension);
    if(!file.open(IO_WriteOnly))
    {
      QMessageBox::warning(parentWidget, tr("Write all input files"), tr("Unable to write the file") +
      "\n" + file.name());
      return;
    }
    QTextStream stream(&file);
    stream << QString(relaxInput.join("\n")+"\n");
    fileInp.close();
  }
}

///// clean ///////////////////////////////////////////////////////////////////
void Calculation::clean()
/// Cleans up the calculation directory
{
  if(calcRunning)
    return;

  QDir dir(calcDir);
  if(!dir.exists())
    return; // certainly clean

  ///// switch to the existing calculation directory
  if(!makeDirCurrent(calcDir, tr("Clean calculation")))
    return;

  ///// build a list of files to remove
  QStringList files = dir.entryList(calcName + ".*"); // all in and output files
  files += dir.entryList(calcName + "_*"); // all saved outputs
  files += dir.entryList("fort.*"); // all fortran unnnamed unit files
  QFile file;
  ///// remove them
  for(QStringList::Iterator it = files.begin(); it != files.end(); it++)
  {
    file.setName(*it);
    file.remove();
  }
  ///// remove the tmp subdirectory and its files
  dir.setPath(calcDir + QDir::separator() + "tmp");
  if(!dir.exists())
    return;
  dir.setCurrent(dir.path());
  files = dir.entryList(calcName + ".*"); // all in and output files
  files += dir.entryList("fort.*"); // all fortran unnnamed unit files
  for(QStringList::Iterator it = files.begin(); it != files.end(); it++)
  {
    file.setName(*it);
    file.remove();
  }  
  dir.cdUp();
  dir.rmdir(calcDir + QDir::separator() + "tmp"); // will only be removed if it is empty 
}

///////////////////////////////////////////////////////////////////////////////
///// Private Slots                                                       /////
///////////////////////////////////////////////////////////////////////////////

///// finishBrabo /////////////////////////////////////////////////////////////
void Calculation::finishBrabo()
/// Does the cleanup of a BRABO run.
{
  assert(calcProcess != 0);

  if(!calcRunning)
    return;

  if(calcProcess->isRunning())
  {
    calcProcess->tryTerminate();
    QTimer::singleShot(1000, this, SLOT(finishBrabo));
    return;
  }
  
  ///// at this point the process is definitely not running anymore
  bool noError = calcProcess->normalExit();
  delete calcProcess;
  calcProcess = 0;

  ///// handle errors
  if(!noError)
  {
    if(calculationType == GlobalBase::SinglePointEnergy)
      QMessageBox::warning(parentWidget, tr("Energy"), tr("A problem occured during the calculation of the energy"));
    else
      QMessageBox::warning(parentWidget, tr("Energy & Forces"), tr("A problem occured during the calculation of the energy & forces"));    
    calcRunning = false;
    if(calcError == NoError)
      calcError = UndefinedError;
  }
  
  ///// start the next step
  doNextStep();
}

///// readStdOutBrabo /////////////////////////////////////////////////////////
void Calculation::readStdOutBrabo()
/// Reads the output from Brabo.
{
  assert(calcProcess != 0);

  if(!calcRunning)
    return;
  
  while(calcProcess->canReadLineStdout())
  {
    QString line = calcProcess->readLineStdout();
    //qDebug("stdout: " + line);
    if(line.length() == 75 && line.left(30) != "                              ")
    {
      bool oki, oke;
      unsigned int iteration = line.left(4).toUInt(&oki);
      double energy = line.mid(4,16).toDouble(&oke);
      if(oki && oke)
        emit newIteration(iteration, energy);
      if(oki && oke && startVectorFile.isEmpty())
        startVectorFile = calcDir + QDir::separator() + calcName + ".sta"; // makes sure this variable is filled as soon as the actual file exists
    }
    if(calcError == NoError && line.left(58) == "                                        NO CONVERGENCE !!!")
      calcError = NoConvergence; // the only error that can be deducted from stdout
  }
  
/*      // keep the first error that is encountered
      if(line == "        ERROR DETECTED        ERROR DETECTED        ERROR DETECTED        ERROR DETECTED        ERROR DETECTED")
        calcError = UndefinedError;
      else if(line.left(7) == " NUCLEI" && line.mid(17,10) == "TOO CLOSE,")
        calcError = CloseNuclei;
      qDebug("left7 = |" + line.left(7) + "|, mid = |" + line.mid(17,10) + "|");
*/
}

///// finishStock /////////////////////////////////////////////////////////////
void Calculation::finishStock()
/// Does the cleanup of a STOCK run.
{
  assert(calcProcess != 0);

  if(!calcRunning)
    return;

  if(calcProcess->isRunning())
  {
    calcProcess->tryTerminate();
    QTimer::singleShot(1000, this, SLOT(finishStock));
    return;
  }
  
  ///// at this point the process is definitely not running anymore
  bool noError = calcProcess->normalExit();
  delete calcProcess;
  calcProcess = 0;

  ///// handle errors
  if(!noError)
  {
    QMessageBox::warning(parentWidget, tr("Calculation of stockholder charges"), tr("A problem occured during the calculation of the stockholder charges"));    
    calcRunning = false;
    if(calcError == NoError)
      calcError = UndefinedError;
    doNextStep();
    return;
  }

  ///// add the contents of the fort.7 file (containing the charges) to the punch file
  QFile inputFile(calcDir + QDir::separator() + "fort.7");
  QFile appendFile(calcDir + QDir::separator() + calcName + ".pun");
  if(!inputFile.open(IO_ReadOnly) || !appendFile.open(IO_WriteOnly | IO_Append))
  {
    QMessageBox::warning(parentWidget, tr("Calculation of stockholder charges"), tr("A problem occured during the calculation of the stockholder charges"));    
    calcRunning = false;
    if(calcError == NoError)
      calcError = UndefinedError;
    doNextStep();
    return;
  }
  appendFile.writeBlock(inputFile.readAll());
  inputFile.close();
  appendFile.close();

  doNextStep();
}

///// finishMAFF //////////////////////////////////////////////////////////////
void Calculation::finishMAFF()
/// Does the cleanup of a MAFF run.
{
  assert(calcProcess != 0);

  if(!calcRunning)
    return;

  if(calcProcess->isRunning())
  {
    calcProcess->tryTerminate();
    QTimer::singleShot(1000, this, SLOT(finishMAFF));
    return;
  }
  
  ///// at this point the process is definitely not running anymore
  bool noError = calcProcess->normalExit();
  delete calcProcess;
  calcProcess = 0;

  ///// handle errors
  if(!noError)
  {
    QMessageBox::warning(parentWidget, tr("Updating the internal coordinates"), tr("A problem occured during the generation of new internal coordinates"));    
    calcRunning = false;
    if(calcError == NoError)
      calcError = UndefinedError;
  }

  ///// nothing more to to than run the next step (CNVRTAFF or fail)
  doNextStep();
}

///// finishCnvrtAFF //////////////////////////////////////////////////////////
void Calculation::finishCnvrtAFF()
/// Does the cleanup of a CNVRTAFF run.
{
  assert(calcProcess != 0);

  if(!calcRunning)
    return;

  if(calcProcess->isRunning())
  {
    calcProcess->tryTerminate();
    QTimer::singleShot(1000, this, SLOT(finishCnvrtAFF));
    return;
  }
  
  ///// at this point the process is definitely not running anymore
  bool noError = calcProcess->normalExit();
  delete calcProcess;
  calcProcess = 0;

  ///// handle errors
  if(!noError)
  {
    QMessageBox::warning(parentWidget, tr("Updating the internal coordinates"), tr("A problem occured during the conversion of the new internal coordinates"));    
    calcRunning = false;
    if(calcError == NoError)
      calcError = UndefinedError;
    doNextStep();
    return;
  }

  ///// combine the contents of the generated AFF file with the AFF header file
  QFile fileNewAFF(calcDir + QDir::separator() + calcName + ".aff_new");
  QFile fileAFF(calcDir + QDir::separator() + calcName + ".aff");
  if(!fileNewAFF.open(IO_ReadOnly) || !fileAFF.open(IO_WriteOnly | IO_Translate)) // suddenly Relax wants files with crlf on Windows?
  {
    QMessageBox::warning(parentWidget, tr("Updating the internal coordinates"), tr("A problem occured during combining the new internal coordinates"));    
    calcRunning = false;
    if(calcError == NoError)
      calcError = UndefinedError;
    doNextStep();
    return;
  }
  QTextStream streamNewAFF(&fileNewAFF);
  QTextStream streamAFF(&fileAFF);
  ///// write the header
  streamAFF << QString(relaxInput.join("\n")+"\n");
  ///// read the correct part from the generated AFF file (after GBMA/BMAT)
  QString line;
  while(!line.contains("GBMA", false) && !line.contains("BMAT", false))
    line = streamNewAFF.readLine();
  streamAFF << streamNewAFF.read(); // add the remainder of the file
  fileNewAFF.close();
  fileAFF.close();

  ///// next step
  doNextStep();
}

///// finishRelax /////////////////////////////////////////////////////////////
void Calculation::finishRelax()
/// Does the cleanup of a RELAX run. It fills the AtomSet \c newAtoms for updating 
/// after the forces are calculated for them.
{
  assert(calcProcess != 0);

  if(!calcRunning)
    return;

  if(calcProcess->isRunning())
  {
    calcProcess->tryTerminate();
    QTimer::singleShot(1000, this, SLOT(finishRelax));
    return;
  }
  
  ///// at this point the process is definitely not running anymore
  bool noError = calcProcess->normalExit();
  delete calcProcess;
  calcProcess = 0;

  qDebug("finishRelax: noError = %d", noError);
  ///// handle errors
  if(!noError)
  {
    QMessageBox::warning(parentWidget, tr("Geometry optimization"), tr("A problem occured during the calculation of new coordinates"));    
    calcRunning = false;
    if(calcError == NoError)
      calcError = UndefinedError;
    doNextStep();
    return;
  }

  ///// check if the molecule is refined
  QFile fileC00(calcDir + QDir::separator() + calcName + ".c00");
  if(fileC00.exists())
  {
    // read the coordinates
    unsigned int result = CrdFactory::readFromFile(atoms, fileC00.name());
    if(result != CrdFactory::OK)
    {
      QMessageBox::warning(parentWidget, tr("Run calculation"), tr("A problem occured reading the optimized coordinates."));
      if(calcError == NoError)
        calcError = UndefinedError;
    }
    else
      calcError = NoError;
    calcRunning = false;
    doNextStep();
    return;
  }

  ///// check whether an .ncr has been written
  QFile fileNCR(calcDir + QDir::separator() + calcName + ".ncr");
  if(!fileNCR.exists())
  {
    QMessageBox::warning(parentWidget, tr("Run calculation"), tr("A problem occured reading the new coordinates\n(the coordinate file does not exist)."));
    calcRunning = false;
    if(calcError == NoError)
      calcError = UndefinedError;
    doNextStep();
    return;
  }

  ///// move the ncr file to the crd file
  QFile fileCrd(calcDir + QDir::separator() + calcName + ".crd");
  if(!fileCrd.remove()) // this is suddenly needed otherwise the rename won't work on Windows
  {
    QMessageBox::warning(parentWidget, tr("Run calculation"), tr("A problem occured updating the coordinates."));
    calcRunning  = false;
    if(calcError == NoError)
      calcError = UndefinedError;
    doNextStep();
    return;
  }
  QUrlOperator local(calcDir);
  const QNetworkOperation* operation = local.rename(calcName + ".ncr", calcName + ".crd");
  while((operation->state() == QNetworkProtocol::StWaiting) || (operation->state() == QNetworkProtocol::StInProgress))
    qApp->processEvents(); // shouldn't do much as renaming should be very fast as opposed to copying
  if(operation->state() != QNetworkProtocol::StDone)
  {
    qDebug("operation->state() = %d",operation->state());
    qDebug(" error code = %d",operation->errorCode());
    qDebug("error string = "+operation->protocolDetail());
    QMessageBox::warning(parentWidget, tr("Run calculation"), tr("A problem occured updating the coordinates."));
    calcRunning = false;
    if(calcError == NoError)
      calcError = UndefinedError;
  }

  doNextStep();
}

///// readStdOutAll ///////////////////////////////////////////////////////////
void Calculation::readStdOutAll()
/// Reads the output from the running process without checking or parsing.
{
  assert(calcProcess != 0);

  if(!calcRunning)
    return;
  
  while(calcProcess->canReadLineStdout())
    calcProcess->readLineStdout();
}

///////////////////////////////////////////////////////////////////////////////
///// Private Member Functions                                            /////
///////////////////////////////////////////////////////////////////////////////

///// checkBasissets //////////////////////////////////////////////////////////
bool Calculation::checkBasissets()
/// Checks whether all basissets given in \c basissetList exist.
{
  ///// build a list of missing basisset files
  QStringList missingFiles;
  for(QStringList::iterator it = basissetList.begin(); it != basissetList.end(); it++)
  {
    QFile file(*it);
    if(!file.exists())
      missingFiles += *it;
  }
  ///// return OK if all files are present
  if(missingFiles.isEmpty())
  {
    doBasissetCheck = false;
    return true;
  }
  
  ///// return false if some files are missing
  QMessageBox::warning(parentWidget, tr("Check basis sets") , tr("The following basis set files could not be found:") 
                       + "\n\n - " + missingFiles.join("\n - "));
  return false;
}

///// checkExecutables ////////////////////////////////////////////////////////
bool Calculation::checkExecutables()
/// Checks whether all needed executables are present 
{
  ///// build a list of missing executables
  QStringList missingExecutables;
  QFile file(Paths::brabo);
  if(!file.exists())
    missingExecutables += "Brabo (" + Paths::brabo  +")";
  if(!stockInput.isEmpty())
  {
    file.setName(Paths::relax);
    if(!file.exists())
      missingExecutables += "Stock (" + Paths::stock  +")";
  }
  if(calculationType == GlobalBase::GeometryOptimization)
  {
    file.setName(Paths::relax);
    if(!file.exists())
      missingExecutables += "Relax (" + Paths::relax  +")";
    if(!maffInput.isEmpty())
    {
      file.setName(Paths::maff);
      if(!file.exists())
        missingExecutables += "Maff (" + Paths::maff  +")";
      file.setName(Paths::cnvrtaff);
      if(!file.exists())
        missingExecutables += "Cnvrtaff (" + Paths::cnvrtaff +")";
    }
  }
  ///// return OK if all files are present
  if(missingExecutables.isEmpty())
    return true;

  ///// return false if some files are missing
  QMessageBox::warning(parentWidget, tr("Check executables") , tr("The following executable files could not be found:") 
                       + "\n\n - " + missingExecutables.join("\n - ") + "\n\n" + tr("Please check the Preferences."));
  return false;
}

///// makeDirCurrent //////////////////////////////////////////////////////////
bool Calculation::makeDirCurrent(const QString dir, const QString title)
/// Makes \c dir the current directory. If it doesn't exist, it will be created.
/// Shows a messagebox with the error and \c title when something goes wrong.
{
  QDir workDir = dir;
  if(!workDir.exists())
  {
    if(!workDir.mkdir(workDir.path()))
    {
      QMessageBox::warning(parentWidget, title, tr("Unable to create the directory ") + workDir.path());
      return false;
    }
  }
  if(!QDir::setCurrent(workDir.path()))
  {
    QMessageBox::warning(parentWidget, title, tr("Unable to change to the directory ") + workDir.path());
    return false;
  }
  return true;
}

///// copyStartvector /////////////////////////////////////////////////////////
bool Calculation::copyStartVector()
/// Copies the starting vector to the calculation directory.
{
  ///// return if no starting vector is to be used
  if(startVectorFile.isEmpty())
    return true;

  ///// return if the starting vector is already in the directory
  if(startVectorFile == calcDir + QDir::separator() + calcName + ".sta")
    return true;

  ///// copy the starting vector
  qDebug("copying startvector");
  if(!copyFile(startVectorFile, calcDir + QDir::separator() + calcName + ".sta"))
  {
    QMessageBox::warning(parentWidget, tr("Copy starting vector"), tr("Unable to copy the starting vector \n") 
                         + startVectorFile + tr(" to the calculation directory"));
    return false;
  }
  return true;
}

///// doNextStep //////////////////////////////////////////////////////////////
void Calculation::doNextStep()
/// Starts the next step of a calculation. It reads it from the first element
/// of \c calculationSteps and calls the appropriate startup function. 
{
  qDebug("doNextStep: calcPaused = %d, calcRunning = %d, stopRequested = %d",
                      calcPaused, calcRunning, stopRequested);

  // do nothing if the calculation is paused
  if(calcPaused)
    return;

  setModified();

  // check whether the calculation has finished successfully or an error occured
  if(!calcRunning || stopRequested)
  {
    runCleanup();
    return;
  }

  // get the current step
  CalculationStep currentStep = calculationSteps.front();
  // take it from the front and insert it at the back for the next call of doNextStep
  calculationSteps.pop_front();
  calculationSteps.push_back(currentStep);

  // do the appropriate action 
  const unsigned int thisStep = currentStep;
  switch(thisStep) // switch doesn't work with enums
  {
    case NEW_CYCLE: // start of a new cycle
                    qDebug("doNextStep: currentStep = NEW_CYCLE");
                    if(calculationType == GlobalBase::SinglePointEnergy || calculationType == GlobalBase::EnergyAndForces)
                      calcRunning = false;
                    else if(currentCycle == calcMaxCycle)
                    {
                      calcRunning = false;
                      if(calcError == NoError)
                        calcError = MaxCyclesExceeded;
                    }
                    else
                    {
                      // backup the output files
                      backupOutputs();
                      // start a new optimization cycle
                      currentCycle++;
                      emit newCycle(currentCycle+1);
                      doNextStep();
                      return;
                    }
                    break;

    case BRABO:     // run BRABO     
                    qDebug("doNextStep: currentStep = BRABO");
                    runBrabo();
                    break;

    case CNVRTAFF:  // run CNVRTAFF
                    qDebug("doNextStep: currentStep = CNVRTAFF");
                    if(!maffInput.isEmpty() && ((affUpdateFreq == 0 && currentCycle == 0) || (affUpdateFreq != 0 && (currentCycle % affUpdateFreq == 0) || updateRelaxInput)))
                      // only run CnvrtAFF/MAFF if
                      // - there is an input file for maff (if not, no automatic generation of IC's is requested
                      // - it's the first cyle and only 1 AFF file is to be made (when affUpdateFreq == 0)
                      // - the AFF file should be regenerated every affUpdateFreq cycles
                      // - automatic regeneration and a change in the AFF header file
                      runCnvrtAFF();
                    else
                    {
                      doNextStep();
                      return;
                    }
                    break;

    case MAFF:      // run MAFF
                    qDebug("doNextStep: currentStep = MAFF");
                    if(!maffInput.isEmpty() && ((affUpdateFreq == 0 && currentCycle == 0) || (affUpdateFreq != 0 && (currentCycle % affUpdateFreq == 0) || updateRelaxInput)))
                      // only run CnvrtAFF/MAFF if
                      // - there is an input file for maff (if not, no automatic generation of IC's is requested
                      // - it's the first cyle and only 1 AFF file is to be made (when affUpdateFreq == 0)
                      // - the AFF file should be regenerated every affUpdateFreq cycles
                      // - automatic generation and a change in the AFF header file
                      runMAFF();
                    else
                    {
                      doNextStep();
                      return;
                    }
                    break;

    case RELAX:     // run RELAX
                    qDebug("doNextStep: currentStep = RELAX");
                    runRelax();
                    break;
         
    case STOCK:     // run STOCK
                    qDebug("doNextStep: currentStep = STOCK");
                    if(stockInput.isEmpty())
                    {
                      doNextStep();
                      return;
                    }
                    else
                      runStock(); // only run it when these charges are requested
                    break;

    case UPDATE:    // run UPDATE
                    qDebug("doNextStep: currentStep = UPDATE");
                    runUpdate();
                    return; // never call runCleanup after runUpdate (runCleanup never changes calcRunning)
                    break;
  }
  // do a cleanup if something went wrong or the end of the calculation, 
  // signalled by setting calcRunning to false
  if(!calcRunning)
    runCleanup();
}

///// runBrabo ////////////////////////////////////////////////////////////////
void Calculation::runBrabo()
/// Starts a BRABO run. The latest input is always used in case of Xbrabo-control
/// but it might have to be updated for other runtypes.
{
  assert(calcProcess == 0);

  if(updateBraboInput)
  {
    // update this input if it should exist as a physical file on disk

    updateBraboInput = false;
  }

  ///// remove the line 'STAR        99.' from the input file
  ///// if no starting vector is present
  QString stdInput;
  if(startVectorFile.isEmpty())
  {
    for(QStringList::Iterator it = braboInput.begin(); it != braboInput.end(); it++)
    {
      if((*it).left(4) != "star")
        stdInput += *it + "\n";
    }
  }
  else
    stdInput = braboInput.join("\n")+"\n";

  ///// copy the starting vector to the calculation directory
  ///// this only does something during the calculation when it is changed due to a different basis set
  if(!copyStartVector())
  {
    calcRunning = false;
    if(calcError == NoError)
      calcError = UndefinedError;
    doNextStep();
    return;
  }

  ///// start BRABO
  calcProcess = new QProcess(this);
  calcProcess->setWorkingDirectory(calcDir);
  calcProcess->addArgument(Paths::brabo);
  connect(calcProcess, SIGNAL(processExited()), this, SLOT(finishBrabo()));
  connect(calcProcess, SIGNAL(readyReadStdout()), this, SLOT(readStdOutBrabo()));
  calcProcess->launch(stdInput);
}

///// runStock ////////////////////////////////////////////////////////////////
void Calculation::runStock()
/// Starts a STOCK run.
{
  assert(calcProcess == 0);

  if(updateStockInput)
  {
    ///// write the stin file
    QFile file(calcDir + QDir::separator() + calcName + ".stin");
    if(!file.open(IO_WriteOnly))
    {
      QMessageBox::warning(parentWidget, tr("Start the calculation of stockholder charges"), tr("Unable to update the file") +
        "\n" + file.name());
      calcRunning = false;
      if(calcError == NoError)
        calcError = UndefinedError;
      doNextStep();
      return;
    }
    QTextStream stream(&file);
    stream << QString(stockInput.join("\n")+"\n");
    file.close();

    ///// write the atdens file if it is not empty
    if(!atdensFile.isEmpty())
    {
      file.setName(calcDir + QDir::separator() + calcName + ".atdens");
      if(!file.open(IO_WriteOnly | IO_Translate))
      {
        QMessageBox::warning(parentWidget, tr("Start the calculation of stockholder charges"), tr("Unable to update the file") +
          "\n" + file.name());
        calcRunning = false;
        if(calcError == NoError)
          calcError = UndefinedError;
        doNextStep();
        return;
      }
      stream.setDevice(&file);
      stream << atdensFile;
      file.close();
      atdensFile = QString::null;
    }
    updateStockInput = false;
  }

  calcProcess = new QProcess(this);
  calcProcess->setWorkingDirectory(calcDir);
  calcProcess->addArgument(Paths::stock);
  connect(calcProcess, SIGNAL(processExited()), this, SLOT(finishStock()));
  connect(calcProcess, SIGNAL(readyReadStdout()), this, SLOT(readStdOutAll()));
  calcProcess->launch(calcName + "\n");
}

///// runMAFF /////////////////////////////////////////////////////////////////
void Calculation::runMAFF()
/// Starts a MAFF run. 
{
  assert(calcProcess == 0);
  assert(!maffInput.isEmpty());

  calcProcess = new QProcess(this);
  calcProcess->setWorkingDirectory(calcDir);
  calcProcess->addArgument(Paths::maff);
  connect(calcProcess, SIGNAL(processExited()), this, SLOT(finishMAFF()));
  connect(calcProcess, SIGNAL(readyReadStdout()), this, SLOT(readStdOutAll()));
  calcProcess->launch(maffInput.join("\n")+"\n"); 
}

///// runCnvrtAFF /////////////////////////////////////////////////////////////
void Calculation::runCnvrtAFF()
/// Starts a CNVRTAFF run. 
{
  assert(calcProcess == 0);

  updateRelaxInput = false; // should be disabled here because if in runMAFF this 
                            // routine wouldn't be called and if not at all
                            // an update in runRelax would overwrite stuff.

  calcProcess = new QProcess(this);
  calcProcess->setWorkingDirectory(calcDir);
  calcProcess->addArgument(Paths::cnvrtaff);
  connect(calcProcess, SIGNAL(processExited()), this, SLOT(finishCnvrtAFF()));
  connect(calcProcess, SIGNAL(readyReadStdout()), this, SLOT(readStdOutAll()));
  calcProcess->launch(calcName + "\n");
}

///// runRelax ////////////////////////////////////////////////////////////////
void Calculation::runRelax()
/// Starts a RELAX run. 
{
  assert(calcProcess == 0);

  if(updateRelaxInput)
  {
    ///// update the AFF file in case of updateAFFFreq = 0
    QFile file(calcDir + QDir::separator() + calcName + ".aff");
    if(!file.open(IO_WriteOnly))
    {
      QMessageBox::warning(parentWidget, tr("Updating the internal coordinates"), tr("A problem occured writing the new internal coordinates"));    
      calcRunning = false;
      if(calcError == NoError)
        calcError = UndefinedError;
      doNextStep();
      return;
    }
    QTextStream stream(&file);
    stream << QString(relaxInput.join("\n") + "\n");
    file.close();    
    updateRelaxInput = false;
  }

  calcProcess = new QProcess(this);
  calcProcess->setWorkingDirectory(calcDir);
  calcProcess->addArgument(Paths::relax);
  connect(calcProcess, SIGNAL(processExited()), this, SLOT(finishRelax()));
  connect(calcProcess, SIGNAL(readyReadStdout()), this, SLOT(readStdOutAll()));
  calcProcess->launch(calcName + "\n" + QString::number(currentCycle+1) + "\nn\n" + QString::number(scaleFactor(currentCycle)) +"\n");
}

///// runCleanup ////////////////////////////////////////////////////////////////
void Calculation::runCleanup()
/// Wraps up a calculation
{
  qDebug("Calculation::runCleanup() called");

  if(calcError == NoError)
    setContinuable(false); // when the structure is optimized, it should never be continued

  emit finished(calcError);
}

///// runUpdate ///////////////////////////////////////////////////////////////
void Calculation::runUpdate()
/// Commits changes to the AtomSet. It updates the coordinates in case of the
/// !first step of a geometry optimization and possibly updates the forces,
/// Mulliken and/or Stockholder charges.
{
  ///// update the coordinates in case of a geometry optimization
  if(calculationType == GlobalBase::GeometryOptimization && currentCycle != 0)
  {
    ///// read the crd file
    QFile file(calcDir + QDir::separator() + calcName + ".crd");
    if(!file.exists() || CrdFactory::readFromFile(atoms, file.name()) != CrdFactory::OK)
      QMessageBox::warning(parentWidget, tr("Run calculation"), tr("The view could not be updated with the new coordinates."));
  }
  ///// update the corresponding properties after the new coordinates are read 
  ///// to prevent them from being overwritten

  ///// update the forces from the punch file
  if(calculationType != GlobalBase::SinglePointEnergy)
  {
    ///// read the forces
    QFile file(calcDir + QDir::separator() + calcName + ".pun");    
    if(!file.exists() || CrdFactory::readForces(atoms, file.name()) != CrdFactory::OK)
    {
      QMessageBox::warning(parentWidget, tr("Run calculation"), tr("The view could not be updated with the new forces."));
      qDebug("filename: "+file.name());
      qDebug("return value: %d", CrdFactory::readForces(atoms, file.name())); 
    }
  }

  ///// update the Mulliken charges
  atoms->setCharges(loadFromPunch("MULL", atoms->count(), 10, 8), AtomSet::Mulliken);

  ///// update the Stockholder charges
  atoms->setCharges(loadFromPunch("STOC", atoms->count(), 10, 8), AtomSet::Stockholder);

  emit updated();
  doNextStep();
}

///// scaleFactor /////////////////////////////////////////////////////////////
double Calculation::scaleFactor(const unsigned int step)
/// Returns the scale factor corresponding to the optimization step.
{
  double result;
  unsigned int iterStep = 0;
  result = scaleFactors[0];
  for(unsigned int i = 0; i < scaleSteps.size(); i++)
  {
    iterStep += scaleSteps[i];
    if(step <= iterStep)
      result = scaleFactors[i];
    else
      break;
  }
  qDebug("Calculation::scaleFactor returns %f", result);
  for(unsigned int i = 0; i < scaleSteps.size(); i++)
    qDebug(" %d: step %d with factor %f",i,scaleSteps[i],scaleFactors[i]);

  return result;
}

///// backupOutputs ///////////////////////////////////////////////////////////
void Calculation::backupOutputs()
/// Makes a backup of output files depending on the backup frequency. If an error
/// is encountered, this function will not interrupt the calculation in any way.
{
  if(backupFrequency == 0 || currentCycle % backupFrequency != 0)
    return;

  if(backupBrabo)
    copyFile(calcDir + QDir::separator() + calcName + ".out", 
             calcDir + QDir::separator() + calcName + "_" + QString::number(currentCycle+1) + ".out");
  if(backupStock && !stockInput.isEmpty())
    copyFile(calcDir + QDir::separator() + calcName + ".stou", 
             calcDir + QDir::separator() + calcName + "_" + QString::number(currentCycle+1) + ".stou");
  if(backupRelax && calculationType == GlobalBase::GeometryOptimization)
    copyFile(calcDir + QDir::separator() + calcName + ".aou", 
             calcDir + QDir::separator() + calcName + "_" + QString::number(currentCycle+1) + ".aou");
  if(backupAFF && calculationType == GlobalBase::GeometryOptimization)
    copyFile(calcDir + QDir::separator() + calcName + ".aff", 
             calcDir + QDir::separator() + calcName + "_" + QString::number(currentCycle+1) + ".aff");
  if(backupCRD)
    copyFile(calcDir + QDir::separator() + calcName + ".crd", 
             calcDir + QDir::separator() + calcName + "_" + QString::number(currentCycle+1) + ".crd");
}

///// copyFile ////////////////////////////////////////////////////////////////
bool Calculation::copyFile(const QString source, const QString destination)
/// Does a blocking copy of \c source to \c destination. It returns whether
/// it succeeded. As it reads the complete file in memory, this is not useful
/// for large files.
{
  QFile inputFile(source);
  QFile outputFile(destination);
  if(!inputFile.open(IO_ReadOnly) || !outputFile.open(IO_WriteOnly))
    return false;
  
  outputFile.writeBlock(inputFile.readAll());
  inputFile.close();
  outputFile.close();
  return true;
}

///// output //////////////////////////////////////////////////////////////////
QStringList Calculation::output(const QString extension, const unsigned int step)
/// Returns the output from a file with the given extension for a certain step. 
/// If the step is zero, the latest output is returned (possibly not saved).
{
  QStringList result;
  QString fileName;
  if(step == 0)
    fileName = calcName + extension;
  else
    fileName = calcName + "_" + QString::number(step) + extension;

  QFile file(calcDir + QDir::separator() + fileName);
  if(file.open(IO_ReadOnly))
  {
    QTextStream stream(&file);
    //while(!stream.atEnd())
    //  result += stream.readLine();
    result = QStringList::split("\n", stream.read(), true);
  }
  return result;
}

///// loadFromPunch ///////////////////////////////////////////////////////////
std::vector<double> Calculation::loadFromPunch(const QString code, const unsigned int numValues, const unsigned int fieldSize, const unsigned int fieldsPerLine)
/// Reads data from the punch file in fixed format.
/// \param[in] code: Starts reading from the file after the line '****'+code.
/// \param[in] numValues: The number of values to read.
/// \param[in] fieldSize: The number of characters assigned to a value.
/// \param[in] fieldsPerLine: The number of values to read from 1 line.
{
  std::vector<double> result;

  ///// open the punch file
  QFile file(calcDir + QDir::separator() + calcName + ".pun");
  if(!file.open(IO_ReadOnly))
    return result; // An empty result indicates failure

  qDebug("reading data from " + file.name());
  qDebug("looking for code |****" + code+ "|");

  //// position after the correct code
  QTextStream stream(&file);
  int codeLength = 4 + code.length();  
  while(stream.readLine().left(codeLength) != QString("****" + code) && !stream.atEnd())
    ;
  
  ///// return if the code cannot be found
  if(stream.atEnd())
  {
    result.clear();
    return result;
  }
  
  ///// read the full lines and process them
  ///// all complete lines
  QString line;
  for(unsigned int i = 0; i < numValues/fieldsPerLine; i++)
  {
    line = stream.readLine(); // contains fieldsPerLine values
    for(unsigned int j = 0; j < 8; j++)
      result.push_back(line.mid(j*fieldSize,fieldSize).toDouble());

    if(stream.atEnd())
    {
      result.clear();
      return result;
    }
  }
  ///// last incomplete line
  line = stream.readLine(); 
  for(unsigned int j = 0; j < numValues % fieldsPerLine; j++)
    result.push_back(line.mid(j*fieldSize,fieldSize).toDouble());

  assert(result.size() == numValues);
  return result;
}

