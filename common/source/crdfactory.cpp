/***************************************************************************
                        crdfactory.cpp  -  description
                             -------------------
    begin                : Sat Jan 17 2004
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
  \class CrdFactory
  \brief Allows loading and saving of a number of coordinate file formats.

  This class is a utility class providing a unified interface to loading and saving 
coordinates from/to Brabo .crd files and formats supported by the Open Babel
library. This interface eases the use of this library in the Qt framework.
It is a pure utility class, so no instances can be made (private 
constructor) and only static functions are available.

Defines 'USE_OPENBABEL1' and 'USE_OPENBABEL2' are made available. If neither
is defined, the openbabel library is not needed for compilation.

*/
/// \file 
/// Contains the implementation of the class CrdFactory.

///// Header files ////////////////////////////////////////////////////////////

// C++ header files
#include <cmath>
#include <iostream>

// Qt header files
#include <qfiledialog.h>
#include <qmessagebox.h>
#include <qstring.h>
#include <qstringlist.h>

#include <qdatetime.h>

#ifdef USE_OPENBABEL1
 // Open Babel 1.100.2 header files
 #ifdef Q_OS_WIN32
   #include <mol.h>
   #include <molvector.h>
   #include <obutil.h>
   #include <parsmart.h>
   #include <rotor.h>
   #include <binary.h>
 #else
   #include <openbabel/mol.h>
   #include <openbabel/molvector.h>
   #include <openbabel/obutil.h>
   #include <openbabel/parsmart.h>
   #include <openbabel/rotor.h>
   #include <openbabel/binary.h>
 #endif
 using namespace OpenBabel; // to be changed to a number of 'using <class>' lines
#endif
#ifdef USE_OPENBABEL2
  // Open Babel 2.x header files
  #ifdef Q_OS_WIN32
    #include <mol.h>
    #include <obconversion.h>
  #else
    #include <openbabel/mol.h>
    #include <openbabel/obconversion.h>
  #endif
  using namespace OpenBabel;
#endif

// CrdView header files
#include "atomset.h"
#include "crdfactory.h"
#include "version.h"


///////////////////////////////////////////////////////////////////////////////
///// Public Member Functions                                             /////
///////////////////////////////////////////////////////////////////////////////

///// Destructor //////////////////////////////////////////////////////////////
CrdFactory::~CrdFactory()
/// The default destructor.
{

}

///// readFromFile ////////////////////////////////////////////////////////////
unsigned int CrdFactory::readFromFile(AtomSet* atoms, QString filename)
/// Reads the contents of a coordinate file
/// \c filename and fills the AtomSet \c atoms. If the filename is empty, a dialog is
/// shown allowing the choice of one. 
{
  // choose a filename if none was specified
  if(filename.isEmpty())
  {
    QStringList filterList = supportedInputFormats();
    QString fileFilter = filterList.join(";;");
    QString fileTitle = QFileDialog::tr("Choose coordinates");
    filename = QFileDialog::getOpenFileName(QString::null, fileFilter, 0, 0, fileTitle);
    if(filename.isEmpty())
      return Cancelled; 
  }

  // validate the filename
  if(!validInputFormat(filename))    
    return UnknownExtension; 
  
  // check whether the Xbrabo or Open Babel reading routines should be used
  if(braboExtension(filename))
  {
    ///// Brabo format
    return readBraboFile(atoms, filename);
  }
  else if(xmolExtension(filename))
  {
    ///// Xmol format
    return readXmolFile(atoms, filename);
  } 
  else if(gaussianExtension(filename))
  {
    ///// Gaussian format
    return readGaussianFile(atoms, filename);
  }
#ifdef USE_OPENBABEL1
  else
  {
    ///// Open Babel format
    // read the file
    char* fn = qstrdup(filename.latin1()); // put the filename QString into a non-const char*
    io_type fileType = extab.FilenameToType(fn);
    delete fn;
    std::ifstream inFileStream;
    inFileStream.open(filename.latin1());
    if(!inFileStream)
      return ErrorOpen;
    OBMol* mol = new OBMol(fileType);
    if(!OBFileFormat::ReadMolecule(inFileStream, *mol)) // only read the first molecule
    {
      delete mol;
      return ErrorRead;
    }
    // fill the AtomSet
    atoms->clear();
    atoms->reserve(mol->NumAtoms());
    vector<OBNodeBase*>::iterator it;
    for(OBAtom* atom  = mol->BeginAtom(it); atom; atom = mol->NextAtom(it))
      atoms->addAtom(atom->GetX(), atom->GetY(), atom->GetZ(), static_cast<unsigned int>(atom->GetAtomicNum()));
    delete mol;
  }  
#endif
#ifdef USE_OPENBABEL2
  else
  {
    ///// Open Babel format
    QTime timer;
    timer.start();
    // read the file
    OBConversion conv;
    OBFormat* fileFormat = conv.FormatFromExt(filename.latin1());
    if(!fileFormat || !conv.SetInFormat(fileFormat))
      return UnknownExtension;
    // delete fileFormat; ???
    std::ifstream inFileStream;
    inFileStream.open(filename.latin1());
    if(!inFileStream)
      return ErrorOpen;
    OBMol mol;
    if(!conv.Read(&mol, &inFileStream))
      return ErrorRead;
    qDebug("time to read the OpenBabel file: %f seconds", timer.restart()/1000.f);
    // fill the AtomSet
    atoms->clear();
    atoms->reserve(mol.NumAtoms());
    for(OBMolAtomIter atom(mol); atom; atom++)
      atoms->addAtom(atom->x(), atom->y(), atom->z(), atom->GetAtomicNum());
    qDebug("time to fill the AtomSet: %f seconds", timer.restart()/1000.f);
  }  
#endif
  return OK;
}

///// readFromFile (overloaded) ///////////////////////////////////////////////
unsigned int CrdFactory::readFromFile(AtomSet* atoms)
/// Reads the contents of a coordinate file
/// filename and fills the AtomSet. Overloaded version allowing reading a
/// coordinate file when the filename isn't needed by the calling routine.
{  
  QString emptyString;
  return readFromFile(atoms, QString::null);
}

///// writeToFile /////////////////////////////////////////////////////////////
unsigned int CrdFactory::writeToFile(AtomSet* atoms, QString filename, bool extendedFormat)
/// Saves the coordinates in the AtomSet to a file
/// with a (predefined) filename, and for the Brabo format a predefined precision.
/// If filename = QString::null, a filename is asked. If the resulting format is
/// .crd, a precision is asked
{
  if(filename.isEmpty())
  {
    ///// no filename was specified
    QStringList filterList = supportedOutputFormats();
    QString fileFilter = filterList.join(";;");
    QString fileTitle = QFileDialog::tr("Choose a filename");
    QString selectedFilter;
    filename = QFileDialog::getSaveFileName(0, fileFilter, 0, 0, fileTitle, &selectedFilter);
    if(filename.isEmpty())
      return Cancelled;
      
    // fix the extension
    QString selectedExtension = selectedFilter.section(".", -1).section(")",0);
    if(!filename.section(".", -1).contains(selectedExtension))
      filename += "." + selectedExtension;

    // ask the format if it's a Brabo extension
    if(braboExtension(filename))
    {
      extendedFormat = false;
      // check whether the coordinates fit in normal format
      // if not force extended format
      if(atoms->needsExtendedFormat())
        extendedFormat = true;
      else if(QMessageBox::information(0, Version::appName,
                                     QMessageBox::tr("Do you want to save the coordinates in extended or normal format?"),
                                     QMessageBox::tr("Extended format"), QMessageBox::tr("Normal format")) == 0)
        extendedFormat = true;  
    }  
  }
  else
  {
    ///// a filename was specified
    // validate the filename
    if(!validOutputFormat(filename))
      return UnknownExtension; 
  }   

  // check whether the Xbrabo or Open Babel writing routines should be used
  if(braboExtension(filename))
  {
    ///// Brabo format
    return writeBraboFile(atoms, filename, extendedFormat);
  }
#ifdef USE_OPENBABEL1  
  else
  {
    ///// Open Babel format
    // determine the Open Babel file type
    char* fn = qstrdup(filename.latin1()); // put the filename QString into a non-const char*
    io_type fileType = extab.FilenameToType(fn);
    delete fn;   
    // construct an Open Babel molecule and fill it with the contents of the AtomSet
    OBMol* mol = new OBMol(fileType, fileType);
    OBAtom* atom;
    for(unsigned int i = 0; i < atoms->count(); i++)
    {
      atom = mol->NewAtom();
      atom->SetVector(atoms->x(i), atoms->y(i), atoms->z(i));
      atom->SetAtomicNum(atoms->atomicNumber(i));
    }   
    // write the file
    std::ofstream outFileStream;
    outFileStream.open(filename.latin1());
    if(!outFileStream)
      return ErrorOpen;
    if(!OBFileFormat::WriteMolecule(outFileStream, *mol))
    {
      delete mol;
      return ErrorWrite;
    }  
    delete mol;      
  }
#endif  
#ifdef USE_OPENBABEL2  
  else
  {
    ///// Open Babel format
    // determine the Open Babel file type
    OBConversion conv;
    OBFormat* fileFormat = conv.FormatFromExt(filename.latin1());
    if(!fileFormat || !conv.SetOutFormat(fileFormat))
      return UnknownExtension;
    // delete fileFormat; ???
    std::ofstream outFileStream;
    outFileStream.open(filename.latin1());
    if(!outFileStream)
      return ErrorOpen;
    OBMol mol;
    OBAtom* atom;
    for(unsigned int i = 0; i < atoms->count(); i++)
    {
      atom = mol.NewAtom();
      atom->SetVector(atoms->x(i), atoms->y(i), atoms->z(i));
      atom->SetAtomicNum(atoms->atomicNumber(i));
    }   
    if(!conv.Write(&mol, &outFileStream))
      return ErrorWrite;  
  }
#endif
  return OK;
}

///// convert /////////////////////////////////////////////////////////////////
unsigned int CrdFactory::convert(const QString inputFileName, const QString outputFileName, const bool extendedFormat)
/// Converts the coordinates in \c inputFileName to \c outputFileName.
{
  ///// validate the input
  if(!validInputFormat(inputFileName))
    return UnknownExtension;
  if(!validOutputFormat(outputFileName))
    return UnknownExtension;  
  
  //// determine which routine should be used  
  if(braboExtension(inputFileName) || braboExtension(outputFileName) || xmolExtension(inputFileName))
  {
    ///// one of the files is in Brabo format or the input file is in Xmol format, so no shortcuts can be taken
    
    // read the input file into the AtomSet
    AtomSet* atoms = new AtomSet();
    QString tempInput = inputFileName;
    unsigned int result = readFromFile(atoms, tempInput);
    if(result != OK)
      return result;

    // save the AtomSet to the output file
    return writeToFile(atoms, outputFileName, extendedFormat);
       
  }
#ifdef USE_OPENBABEL1  
  else
  {
    ///// both formats can be handled by Open Babel => shortcut
    
    // determine the file types of the in- & output files
    char* inFile = qstrdup(inputFileName.latin1()); 
    io_type inFileType = extab.FilenameToType(inFile);
    delete inFile;
    char* outFile = qstrdup(outputFileName.latin1());
    io_type outFileType = extab.FilenameToType(outFile);
    delete outFile;

    // open both files
    std::ifstream inFileStream;
    inFileStream.open(inputFileName.latin1());
    if(!inFileStream)
      return ErrorOpen;
    std::ofstream outFileStream;
    outFileStream.open(outputFileName.latin1());
    if(!outFileStream)
      return ErrorOpen;      
    
    // construct an OBMol object
    OBMol* mol = new OBMol(inFileType, outFileType);

    // read the coordinates   
    if(!OBFileFormat::ReadMolecule(inFileStream, *mol)) // only read the first molecule
    {
      delete mol;
      return ErrorRead;
    }

    // write the coordinates
    if(!OBFileFormat::WriteMolecule(outFileStream, *mol))
    {
      delete mol;
      return ErrorWrite;
    }

    // delete the OBMol object again
    delete mol;        
  }
#endif  
#ifdef USE_OPENBABEL2 
  else
  {
    ///// both formats can be handled by Open Babel => shortcut
    // open both files
    std::ifstream inFileStream;
    inFileStream.open(inputFileName.latin1());
    if(!inFileStream)
      return ErrorOpen;
    std::ofstream outFileStream;
    outFileStream.open(outputFileName.latin1());
    if(!outFileStream)
      return ErrorOpen;      
    // create a conversion object
    OBConversion conv(&inFileStream, &outFileStream);
    // set the filetypes
    OBFormat* inFormat = conv.FormatFromExt(inputFileName.latin1());
    OBFormat* outFormat = conv.FormatFromExt(outputFileName.latin1());
    if(!inFormat || !outFormat || !conv.SetInAndOutFormats(inFormat, outFormat))
      return UnknownExtension;
    conv.Convert(); 
  }
#endif
  return OK;
}

///// readForces //////////////////////////////////////////////////////////////
unsigned int CrdFactory::readForces(AtomSet* atoms, QString filename)
/// Reads the contents of a forces file
/// \c filename and fills the AtomSet \c atoms.
{ 
  // validate the filename
  if(!validForceInputFormat(filename))
    return UnknownExtension;

  // check whether the Xbrabo or Open Babel reading routines should be used
  if(braboExtension(filename))
  {
    ///// Brabo format
    return readBraboForces(atoms, filename);
  }
   
  return OK;
}

///////////////////////////////////////////////////////////////////////////////
///// Private Member Functions                                            /////
///////////////////////////////////////////////////////////////////////////////

///// Constructor /////////////////////////////////////////////////////////////
CrdFactory::CrdFactory()
/// The default constructor. Made private to inhibit instantiations of this
/// utility class.
{

}

///// supportedInputFormats ///////////////////////////////////////////////////
QStringList CrdFactory::supportedInputFormats()
/// Returns a QStringList containing the
/// supported coordinate input formats, ready for use in a QFileDialog filter.
{
  QStringList result = "Brabo (*.crd *.c00)";
  result += "Gaussian Checkpoint (*.fchk)";
#ifdef USE_OPENBABEL1  
  for(unsigned int i = 0; i < extab.Count(); i++)
  {
    if(extab.IsReadable(i))
    {
      QString inputFormat = ";;";
      inputFormat += extab.GetDescription(i);
      inputFormat += " (*.";
      inputFormat += extab.GetExtension(i);
      inputFormat += ")";
      result += inputFormat;
    }  
  }
#endif
#ifdef USE_OPENBABEL2 
  OBConversion conv; // here only because of initialisation bug in OBConversion::GetNextFormat (v2.0.0)
  //OBConversion::RegisterFormat("alc",this, "chemical/x-alchemy"); // test for initialisation of OBConversion
  
  const char* str = NULL;
  Formatpos pos;
  OBFormat* format;
  while(OBConversion::GetNextFormat(pos, str, format))
  {
    if(!(format->Flags() & NOTREADABLE))
    {
      // description
      QString desc = QString(format->Description());
      desc.truncate(desc.find("\n")); // stop at the first newline
      desc = desc.remove("format", false).stripWhiteSpace(); // remove unnecessary stuff
      // extension
      QString ext = QString(str);
      ext.truncate(ext.find(" "));
      // combined
      result += QString(";;" + desc + " (*." + ext + ")");
    }
  }
#endif
  return result;
}

///// supportedOutputFormats //////////////////////////////////////////////////
QStringList CrdFactory::supportedOutputFormats()
/// Returns a QStringList containing the
/// supported coordinate output formats, ready for use in a QFileDialog filter.
{
  QStringList result = "Brabo (*.crd)";
#ifdef USE_OPENBABEL1  
  for(unsigned int i = 0; i < extab.Count(); i++)
  {
    if(extab.IsWritable(i))
    {
      QString outputFormat = ";;";
      outputFormat += extab.GetDescription(i);
      outputFormat += " (*.";
      outputFormat += extab.GetExtension(i);
      outputFormat += ")";
      result += outputFormat;
    }
  }
#endif  
#ifdef USE_OPENBABEL2  
  const char* str = NULL;
  Formatpos pos;
  OBFormat* format;
  OBConversion conv; // here only because of initialisation bug in OBConversion::GetNextFormat (v2.0.0)
  while(OBConversion::GetNextFormat(pos, str, format))
  {
    if(!(format->Flags() & NOTWRITABLE))
    {
      // description
      QString desc = QString(format->Description());
      desc.truncate(desc.find("\n")); // stop at the first newline
      desc = desc.remove("format", false).stripWhiteSpace(); // remove unnecessary stuff
      // extension
      QString ext = QString(str);
      ext.truncate(ext.find(" "));
      // combined
      result += QString(";;" + desc + " (*." + ext + ")");
    }
  }
#endif
  return result;
}

///// validInputFormat ////////////////////////////////////////////////////////
bool CrdFactory::validInputFormat(const QString filename)
/// Returns true if the contents of the filename can be read.
{
  // check for Brabo formats
  if(braboExtension(filename) || xmolExtension(filename) || gaussianExtension(filename))
    return true;

#ifdef USE_OPENBABEL1    
  // check for Open Babel formats
  char* fn = qstrdup(filename.latin1()); // put the filename QString into a non-const char*
  if(extab.CanReadExtension(fn))
  {
    delete fn;
    return true;
  }  
  delete fn;
#endif
#ifdef USE_OPENBABEL2    
  // check for Open Babel formats
  OBConversion conv; // here only because of initialisation bug in OBConversion::FileFormat (called by FormatFromExt) (v2.0.0)
  OBFormat* format = OBConversion::FormatFromExt(filename.latin1());
  if(format || !(format->Flags() | NOTREADABLE))
    return true;
#endif
  return false;
}

///// validForceInputFormat ///////////////////////////////////////////////////
bool CrdFactory::validForceInputFormat(const QString filename)
/// Returns true if the contents of the filename can be read.
{
  // check for Brabo formats
  if(braboExtension(filename))
    return true;

  return false;
}

///// validOutputFormat ///////////////////////////////////////////////////////
bool CrdFactory::validOutputFormat(const QString filename)
/// Returns true if the filename can be written.
{  
  // check for Brabo formats
  if(braboExtension(filename))
    return true;

#ifdef USE_OPENBABEL1
  // check for Open Babel formats
  char* fn = qstrdup(filename.latin1()); // put the filename QString into a non-const char*
  if(extab.CanWriteExtension(fn))
  {
    delete fn;
    return true;
  }
  delete fn;
#endif
#ifdef USE_OPENBABEL2    
  // check for Open Babel formats
  OBConversion conv; // here only because of initialisation bug in OBConversion::FileFormat (called by FormatFromExt) (v2.0.0)
  OBFormat* format = OBConversion::FormatFromExt(filename.latin1());
  if(format || !(format->Flags() | NOTWRITABLE))
    return true;
#endif
  return false;
}

///// braboExtension //////////////////////////////////////////////////////////
bool CrdFactory::braboExtension(const QString filename)
/// Returns true if the extension of the 
/// given filename corresponds to a Brabo format.
{
  QString extension = filename.section(".", -1).lower();
  if(extension == "crd" || extension == "c00" || extension == "ncr" || extension == "crdcrd" ||
     extension == "pun" || extension == "f00")
    return true;

  return false;
}

///// xmolExtension ///////////////////////////////////////////////////////////
bool CrdFactory::xmolExtension(const QString filename)
/// Returns true if the extension of the 
/// given filename corresponds to a Xmol format.
{
  QString extension = filename.section(".", -1).lower();
  if(extension == "xyz")
    return true;

  return false;
}

///// gaussianExtension ///////////////////////////////////////////////////////
bool CrdFactory::gaussianExtension(const QString filename)
/// Returns true if the extension of the 
/// given filename corresponds to a Gaussian format.
{
  QString extension = filename.section(".", -1).lower();
  if(extension == "fchk")
    return true;

  return false;
}

//// readBraboFile ////////////////////////////////////////////////////////////
unsigned int CrdFactory::readBraboFile(AtomSet* atoms, const QString filename)
/// Reads coordinates from a given Brabo coordinate file.
/// This function does not check the validity of the given filename.
{
  QTime timer;
  timer.start();
  QFile file(filename);
  if(!file.open(IO_ReadOnly))
    return ErrorOpen;

  ///// read a maximum of 100 lines into allLines to check the format
  QTextStream stream(&file);
  QStringList allLines;
  QString line;
  while(!stream.eof() && allLines.size() < 101)
  {
    line = stream.readLine();
    if(line.left(4).lower() == "stop")
      break;
    else if(line.left(1).lower() == "n" && line.mid(1,1) == "=")
      allLines += line;
  }

  ///// determine the format of the coordinates
  unsigned int format = determineCrdFormat(allLines);
  if(format == UnknownFormat) // format not determinable => error out or ask format
    return UnknownFormat;
  
  ///// read the coordinates
  if(format == NormalFormat)
    readCrdAtoms(atoms, allLines, false); // using normal format
  else if(format == ExtendedFormat)
    readCrdAtoms(atoms, allLines, true); // using extended format

  ///// check the units (Angstrom or Bohr)
  vector<unsigned int>* bonds1;
  vector<unsigned int>* bonds2;
  atoms->bonds(bonds1, bonds2);
  double toAngstrom = 1.0;
  if(bonds1->empty() && atoms->count() > 1)
  {
    // rescale all coordinates
    toAngstrom = 0.529177249;
    for(unsigned int i = 0; i < atoms->count(); i++)
    {
      atoms->setX(i,atoms->x(i) * toAngstrom);
      atoms->setY(i,atoms->y(i) * toAngstrom);
      atoms->setZ(i,atoms->z(i) * toAngstrom);
    }
  }

  ///// read the rest of the file and directly fill the AtomSet
  if(line.left(4).lower() != "stop" && !stream.eof())
  {
    double x, y, z, atomNum;
    unsigned int atomicNumber;

    while(!stream.eof())
    {
      line = stream.readLine();
      if(line.left(4).lower() == "stop")
        break;
      else if(line.left(1).lower() == "n" && line.mid(1,1) == "=")
      {
        atomNum = line.mid(10,10).toDouble();
        if(atomNum < 1.0)
          atomicNumber = 0;
        else
        {
          ///// check whether the atomic number has an integer value or not. If not set it as zero.
          if(atomNum - floor(atomNum) > 0.00001)
            atomicNumber = 0;
          else
            atomicNumber = static_cast<unsigned int>(atomNum);
        }
        if(format == NormalFormat)
        {
          x = line.mid(20,10).toDouble() * toAngstrom;
          y = line.mid(30,10).toDouble() * toAngstrom;
          z = line.mid(40,10).toDouble() * toAngstrom;
        }
        else
        {
          x = line.mid(20,20).toDouble() * toAngstrom;
          y = line.mid(40,20).toDouble() * toAngstrom;
          z = line.mid(60,20).toDouble() * toAngstrom;
        }
        atoms->addAtom(x, y, z, atomicNumber);
      }
    }
  }

  qDebug("time to read the BRABO file: %f seconds", timer.restart()/1000.f);
  return OK;
}

//// writeBraboFile ///////////////////////////////////////////////////////////
unsigned int CrdFactory::writeBraboFile(AtomSet* atoms, const QString filename, const bool extendedFormat)
/// Writes coordinates to a file with a specified filename in a specified format.
/// This function does not check the validity of the given filename.
{
  QFile file(filename);
  if(!file.open(IO_WriteOnly))
    return ErrorOpen;

  QTextStream stream(&file);
  QString line;

  for(unsigned int i = 0; i < atoms->count(); i++)
  {
    if(extendedFormat)
      line = QString("N=%1      %2%3%4%5").arg(AtomSet::numToAtom(atoms->atomicNumber(i)), 2)
                                          .arg(static_cast<double>(atoms->atomicNumber(i)), -10, 'f', 1)
                                          .arg(atoms->x(i), 20, 'f', 12)
                                          .arg(atoms->y(i), 20, 'f', 12)
                                          .arg(atoms->z(i), 20, 'f', 12);
    else
      line = QString("N=%1      %2%3%4%5").arg(AtomSet::numToAtom(atoms->atomicNumber(i)), 2)
                                          .arg(static_cast<double>(atoms->atomicNumber(i)), -10, 'f', 1)
                                          .arg(atoms->x(i), 10, 'f', 7)
                                          .arg(atoms->y(i), 10, 'f', 7)
                                          .arg(atoms->z(i), 10, 'f', 7);
    stream << line << "\n";
  }
  stream << "STOP\n";
  file.close();
  return OK;
}

//// readBraboForces //////////////////////////////////////////////////////////
unsigned int CrdFactory::readBraboForces(AtomSet* atoms, const QString filename)
/// Reads forces from a given Brabo forces file.
/// This function does not check the validity of the given filename.
{
  //qDebug("CrdFactory:: readBraboForces - entering");
  QFile file(filename);
  if(!file.open(IO_ReadOnly))
    return ErrorOpen;

  ///// read all lines into allLines and close the file
  QTextStream stream(&file);
  QStringList allLines;
  QString line;
  ///// position the punch file
  while(!stream.eof())
  {
    line = stream.readLine();
    if(line.left(8).lower() == "****forc")
      break;
  }
  while(!stream.eof())    
  {
    line = stream.readLine();  
    if(line.left(4) == "****")
      break;
    allLines += line;
  }
  file.close();

  ///// determine the format of the forces
  unsigned int format = determinePunchFormat(allLines);
  if(format == UnknownFormat) // format not determinable => error out or ask format
    return UnknownFormat;

  ///// read the forces
  if(format == NormalFormat)
    readPunchForces(atoms, allLines, false); // using normal format
  else if(format == ExtendedFormat)
    readPunchForces(atoms, allLines, true); // using extended format

  return OK;
}

///// readCrdAtoms ////////////////////////////////////////////////////////////
void CrdFactory::readCrdAtoms(AtomSet* atoms, QStringList &lines, const bool extendedFormat)
/// Reads atom coordinates from a .crd file in
/// QStringList form with the specified format (extended/normal).
{
  double x, y, z, atomNum;
  unsigned int atomicNumber;
  QString line;

  atoms->clear();
  atoms->reserve(lines.size());
  for(QStringList::iterator it = lines.begin(); it != lines.end(); it++)
  {
    line = *it;
    ///// read the atomic number and set it to zero if not a positive integer
    atomNum = line.mid(10,10).toDouble();
    if(atomNum < 1.0)
     atomicNumber = 0;
    else
    {
      ///// check whether the atomic number has an integer value or not. If not set it as zero.
      if(atomNum - floor(atomNum) > 0.00001)
        atomicNumber = 0;
      else
        atomicNumber = static_cast<unsigned int>(atomNum);
    }
    //atomicNumber = static_cast<unsigned int>(line.mid(10,10).toDouble());
    if(!extendedFormat)
    {
      x = line.mid(20,10).toDouble();
      y = line.mid(30,10).toDouble();
      z = line.mid(40,10).toDouble();
    }
    else
    {
      x = line.mid(20,20).toDouble();
      y = line.mid(40,20).toDouble();
      z = line.mid(60,20).toDouble();
    }
    atoms->addAtom(x, y, z, atomicNumber);
  }
}

///// readPunchForces /////////////////////////////////////////////////////////
void CrdFactory::readPunchForces(AtomSet* atoms, QStringList &lines, const bool extendedFormat)
/// Reads atom forces from a .pun file in
/// QStringList form with the specified format (extended/normal).
{
  double dx, dy, dz;
  QString line;
  unsigned int index = 0;
  
  for(QStringList::iterator it = lines.begin(); it != lines.end(); it++)
  {    
    line = *it;
    if(!extendedFormat)
    {
      dx = line.mid(0,10).toDouble();
      dy = line.mid(10,10).toDouble();
      dz = line.mid(20,10).toDouble();
    }
    else
    {
      dx = line.mid(0,20).toDouble();
      dy = line.mid(20,20).toDouble();
      dz = line.mid(40,20).toDouble();
    }
    atoms->setForces(index++, dx, dy, dz);    
  }
}

///// determineCrdFormat //////////////////////////////////////////////////////
unsigned int CrdFactory::determineCrdFormat(QStringList& lines)
/// Tries to determine the format of the .crd file parsed in lines. 
/// Only check a maximum of 100 lines. Even as little as 10 could be sufficient though.
{
  QString line;
  unsigned int returnVal = UnknownFormat;
  unsigned int linesChecked = 0;

  for(QStringList::iterator it = lines.begin(); it != lines.end(); it++)
  {
    line = *it;
    if(possiblyNormalCrd(line.mid(20,30)))
    {
      if(!possiblyExtendedCrd(line.mid(20,60)))
      {
        if(returnVal != 1)
          returnVal = NormalFormat;
        else
          return UnknownFormat;
      }
    }
    else
    {
      if(possiblyExtendedCrd(line.mid(20,60)))
      {
        if(returnVal != 0)
          returnVal = ExtendedFormat;
        else
          return UnknownFormat;
      }
      else
        return UnknownFormat;
    }
    if(++linesChecked == 100)
      return returnVal;
  }

  return returnVal;
}

///// determinePunchFormat ////////////////////////////////////////////////////
unsigned int CrdFactory::determinePunchFormat(QStringList& lines)
/// Tries to determine the format of the .pun file parsed in lines.
/// Only check the first 100 lines
{
  QString line;
  unsigned int returnVal = UnknownFormat;
  unsigned int linesChecked = 0;

  for(QStringList::iterator it = lines.begin(); it != lines.end(); it++)
  {
    line = *it;
    if(possiblyNormalCrd(line.mid(0,30)))
    {
      if(!possiblyExtendedCrd(line.mid(0,60)))
      {
        if(returnVal != 1)
          returnVal = NormalFormat;
        else
          return UnknownFormat;
      }
    }
    else
    {
      if(possiblyExtendedCrd(line.mid(0,60)))
      {
        if(returnVal != 0)
          returnVal = ExtendedFormat;
        else
          return UnknownFormat;
      }
      else
        return UnknownFormat;
    }
    if(++linesChecked == 100)
      return returnVal;
  }

  return returnVal;
}

///// possiblyNormalCrd ///////////////////////////////////////////////////////
bool CrdFactory::possiblyNormalCrd(const QString line)
/// Returns true if line contains a .crd line that is possibly in normal format.
{
  QString field1 = line.mid(0,10).stripWhiteSpace();
  QString field2 = line.mid(10,10).stripWhiteSpace();
  QString field3 = line.mid(20,10).stripWhiteSpace();

  if(field1.contains(".", false) != 1 && !field1.isEmpty())
    return false;
  if(field2.contains(".", false) != 1 && !field2.isEmpty())
    return false;
  if(field3.contains(".", false) != 1 && !field3.isEmpty())
    return false;

  return true;
}

///// possiblyExtendedCrd /////////////////////////////////////////////////////
bool CrdFactory::possiblyExtendedCrd(const QString line)
/// Returns true if line contains a .crd line that is possibly in extended format.
{
  QString field1 = line.mid(0,20).stripWhiteSpace();
  QString field2 = line.mid(20,20).stripWhiteSpace();
  QString field3 = line.mid(40,20).stripWhiteSpace();

  if(field1.contains(".", false) != 1 && !field1.isEmpty())
    return false;
  if(field2.contains(".", false) != 1 && !field2.isEmpty())
    return false;
  if(field3.contains(".", false) != 1 && !field3.isEmpty())
    return false;

  return true;
}

//// readXmolFile /////////////////////////////////////////////////////////////
unsigned int CrdFactory::readXmolFile(AtomSet* atoms, const QString filename)
/// Reads coordinates from a given Xmol coordinate file. 
/// This function does not check the validity of the given filename.
{
  ///// open the file
  QFile file(filename);
  if(!file.open(IO_ReadOnly))
    return ErrorOpen;

  ///// read the first line: the number of atoms
  QTextStream stream(&file);
  bool convOK;  
  unsigned int natoms = stream.readLine().toUInt(&convOK);
  if(!convOK || natoms == 0)
    return ErrorRead;

  ///// read the second line: contains a description, ignore it ATM
  stream.readLine();

  ///// read all lines
  QStringList allLines;
  for(unsigned int i = 0; i < natoms; i++)
  {
    // read the next line
    QString line = stream.readLine().simplifyWhiteSpace();
    if(line.isEmpty())
      return ErrorRead;

    allLines += line;
  }
  file.close();
  
  ///// check the number of sections in each line (should be 4 or 7, with or without forces)
  for(QStringList::iterator it = allLines.begin(); it != allLines.end(); it++)
  {
    // split it by whitespace
    QStringList sections = QStringList::split(" ", *it);
    if(sections.count() != 4 && sections.count() != 7)
      return ErrorRead;
  }

  ///// add the coordinates to the AtomSet
  atoms->clear();
  atoms->reserve(allLines.size());
  for(QStringList::iterator it = allLines.begin(); it != allLines.end(); it++)
  {
    // split it again by whitespace
    QStringList sections = QStringList::split(" ", *it);
    // only read the coordinates ATM, ignoring the forces
    QStringList::iterator it2 = sections.begin();
    unsigned int atomnum = AtomSet::atomToNum(*it2);
    double x = (*(++it2)).toDouble();
    double y = (*(++it2)).toDouble();
    double z = (*(++it2)).toDouble();
    atoms->addAtom(x, y, z, atomnum);   
  } 
  return OK;
}

//// readGaussianFile /////////////////////////////////////////////////////////
unsigned int CrdFactory::readGaussianFile(AtomSet* atoms, const QString filename)
/// Reads coordinates from a given Gaussian formatted checkpoint file. 
/// This function does not check the validity of the given filename.
{
  ///// open the file
  QFile file(filename);
  if(!file.open(IO_ReadOnly))
    return ErrorOpen;

  ///// search for a line containing 'Number of atoms'
  QTextStream stream(&file);
  QString line;
  while(line.left(15) != "Number of atoms")
    line = stream.readLine();
  bool convOK;
  unsigned int natoms = line.mid(49,12).toUInt(&convOK);
  if(!convOK || natoms == 0)
    return ErrorRead;

  ///// search for a line containing 'Atomic numbers'
  while(line.left(14) != "Atomic numbers")
    line = stream.readLine();
  unsigned int natoms2 = line.mid(49,12).toUInt(&convOK);
  if(!convOK || natoms2 != natoms)
    return ErrorRead;
  
  ///// read the atomic numbers => safe to read one by one
  vector<unsigned int> atomnums;
  unsigned int atomnum;
  for(unsigned int i = 0; i < natoms; i++)
  {
    stream >> atomnum;
	atomnums.push_back(atomnum);
  }

  ///// search for a line containing 'Current cartesian coordinates'
  while(line.left(29) != "Current cartesian coordinates")
    line = stream.readLine();
  unsigned int natoms3 = line.mid(49,12).toUInt(&convOK);
  if(!convOK || natoms3 != 3*natoms)
    return ErrorRead;
  
  ///// read the coordinates (5E16.8) => safe to read one by one
  atoms->clear();
  atoms->reserve(natoms);
  double x, y, z;
  for(unsigned int i = 0; i < natoms; i++)
  {
    stream >> x >> y >> z;
	x *= AUTOANG;
	y *= AUTOANG;
	z *= AUTOANG;
	atoms->addAtom(x, y, z, atomnums[i]);
  }

  return OK;
}

///////////////////////////////////////////////////////////////////////////////
///// Static Variables                                                    /////
///////////////////////////////////////////////////////////////////////////////
const double CrdFactory::AUTOANG = 1.0/1.889726342; ///< A conversion factor from atomic units to Angstrom.

