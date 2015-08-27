/***************************************************************************
                         basisset.cpp  -  description
                             -------------------
    begin                : Mon Jun 30 2003
    copyright            : (C) 2003-2006 by Ben Swerts
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

/*!
  \class Basisset
  \brief Provides an interface to the basissets available in the
         program.

  It is a utility class composed of static members only. On the first invocation
  of any function the necessary basisset data are loaded. Basissets can be 
  converted between number and string representations.

*/
/// \file
/// Contains the implementation of the class Basisset

///// Header files ////////////////////////////////////////////////////////////

// C++ heqder files
#include <cassert>

// Qt header files
#include <qstring.h>
#include <qstringlist.h>

// Xbrabo header files
#include <basisset.h>


///////////////////////////////////////////////////////////////////////////////
///// Public Member Functions                                             /////
///////////////////////////////////////////////////////////////////////////////

///// basisToNum //////////////////////////////////////////////////////////////
unsigned int Basisset::basisToNum(const QString& basis)
/// Returns the index corresponding to the given basisset name. 
/// The range of the index is 0 - (numBasissets-1).
{
  initBasissets(); // checks itself whether the call was needed

  assert(basToNum.find(basis) != basToNum.end());
  
  return basToNum[basis];
}

///// numToBasis //////////////////////////////////////////////////////////////
QString Basisset::numToBasis(const unsigned int basis)
/// Returns the basisset name corresponding to the given index. 
/// The range of  the index should be 0 - (numBasissets-1).
{
  initBasissets(); // checks itself whether the call was needed

  assert(basis < numBasissets);
  
  return numToBas[basis];
}

///// numToBasisDir ///////////////////////////////////////////////////////////
QString Basisset::numToBasisDir(const unsigned int basis)
/// Returns the basisset directory corresponding to the given index. 
/// The range of  the index should be 0 - (numBasissets-1).
{
  initBasissets(); // checks itself whether the call was needed

  assert(basis < numBasissets);

  return numToDir[basis];
}

///// extension ///////////////////////////////////////////////////////////////
QString Basisset::extension()
/// Returns the extension used for the basisset files.
{
  return QString("basis");
}

///// maxBasissets ////////////////////////////////////////////////////////////
unsigned int Basisset::maxBasissets()
/// Returns the number of available basissets.
{
  initBasissets(); // checks itself whether the call was needed

  return numBasissets;
}

///// contractedFunctions /////////////////////////////////////////////////////
unsigned int Basisset::contractedFunctions(const unsigned int basis, const unsigned int atom)
/// Returns the number of contracted function for the given basis set and atom type.
/// Each basis set has a fixed number of functions per region of the periodic table.
/// With only 54 atom types (as from AtomSet) this means storing 9 numbers per basis set
/// -> 1-2, 3-10, 11-18, 19-20, 21-30, 31-36, 37-38, 39-48, 49-54.
{
  initBasissets(); // checks itself whether the call was needed

  if(atom < 1 || basis >= numBasissets)
    return 0;
  else if(atom < 3)
    return ncf[basis][0];
  else if(atom < 11)
    return ncf[basis][1];
  else if(atom < 19)
    return ncf[basis][2];
  else if(atom < 21)
    return ncf[basis][3];
  else if(atom < 31)
    return ncf[basis][4];
  else if(atom < 37)
    return ncf[basis][5];
  else if(atom < 39)
    return ncf[basis][6];
  else if(atom < 49)
    return ncf[basis][7];
  else if(atom < 55)
    return ncf[basis][8];

  return 0; // if atom > 54, so also for unknown atoms
}

///////////////////////////////////////////////////////////////////////////////
///// Private Member Functions                                            /////
///////////////////////////////////////////////////////////////////////////////

///// constructor /////////////////////////////////////////////////////////////
Basisset::Basisset()
/// The default constructor. Made private for this utility class
{

}

///// destructor //////////////////////////////////////////////////////////////
Basisset::~Basisset()
/// The default destructor.
{

}

///// initBasissets ///////////////////////////////////////////////////////////
void Basisset::initBasissets()
/// Initializes the static member variables. This function is called at the
/// start of each static public member function.
{  
  if(isInitialized)
    return;
  isInitialized = true;

  ///// Basis sets
  numToBas.clear();
  numToDir.clear();
  basToNum.clear();
  ncf.clear();

  QString basis, basdir;
  std::vector<unsigned int> periods(9);
  numBasissets = 0;
  basis = "3-21G";              basdir = "3-21G";              numToBas += basis; numToDir += basdir; basToNum[basis] = numBasissets++;
    periods[0] =  2; periods[1] =  9; periods[2] = 13; periods[3] = 17; periods[4] = 29; 
    periods[5] = 23; periods[6] = 27; periods[7] = 39; periods[8] = 33; ncf.push_back(periods);
  basis = "3-21G*";             basdir = "3-21Gp";             numToBas += basis; numToDir += basdir; basToNum[basis] = numBasissets++;
    periods[0] =  2; periods[1] =  9; periods[2] = 19; periods[3] = 23; periods[4] = 29; 
    periods[5] = 29; periods[6] = 33; periods[7] = 39; periods[8] = 39; ncf.push_back(periods);
  basis = "3-21G**";            basdir = "3-21Gpp";            numToBas += basis; numToDir += basdir; basToNum[basis] = numBasissets++;
    periods[0] =  5; periods[1] =  9; periods[2] = 19; periods[3] = 23; periods[4] = 29; 
    periods[5] = 29; periods[6] = 33; periods[7] = 39; periods[8] = 39; ncf.push_back(periods);
  basis = "3-21+G";             basdir = "3-21+G";             numToBas += basis; numToDir += basdir; basToNum[basis] = numBasissets++;
    periods[0] =  2; periods[1] = 13; periods[2] = 17; periods[3] =  0; periods[4] =  0; 
    periods[5] =  0; periods[6] =  0; periods[7] =  0; periods[8] =  0; ncf.push_back(periods);
  basis = "3-21+G*";            basdir = "3-21+Gp";            numToBas += basis; numToDir += basdir; basToNum[basis] = numBasissets++;
    periods[0] =  2; periods[1] = 13; periods[2] = 23; periods[3] =  0; periods[4] =  0; 
    periods[5] =  0; periods[6] =  0; periods[7] =  0; periods[8] =  0; ncf.push_back(periods);
  basis = "3-21+G**";           basdir = "3-21+Gpp";           numToBas += basis; numToDir += basdir; basToNum[basis] = numBasissets++;
    periods[0] =  5; periods[1] = 13; periods[2] = 23; periods[3] =  0; periods[4] =  0; 
    periods[5] =  0; periods[6] =  0; periods[7] =  0; periods[8] =  0; ncf.push_back(periods);
  basis = "6-31G";              basdir = "6-31G";              numToBas += basis; numToDir += basdir; basToNum[basis] = numBasissets++;
    periods[0] =  2; periods[1] =  9; periods[2] = 13; periods[3] = 17; periods[4] = 29; 
    periods[5] = 24; periods[6] =  0; periods[7] =  0; periods[8] =  0; ncf.push_back(periods);
  basis = "6-31G*";             basdir = "6-31Gp";             numToBas += basis; numToDir += basdir; basToNum[basis] = numBasissets++;
    periods[0] =  2; periods[1] = 15; periods[2] = 19; periods[3] = 23; periods[4] = 36; 
    periods[5] = 30; periods[6] =  0; periods[7] =  0; periods[8] =  0; ncf.push_back(periods);
  basis = "6-31G**";            basdir = "6-31Gpp";            numToBas += basis; numToDir += basdir; basToNum[basis] = numBasissets++;
    periods[0] =  5; periods[1] = 15; periods[2] = 19; periods[3] = 23; periods[4] = 36; 
    periods[5] = 30; periods[6] =  0; periods[7] =  0; periods[8] =  0; ncf.push_back(periods);
  basis = "6-31+G";             basdir = "6-31+G";             numToBas += basis; numToDir += basdir; basToNum[basis] = numBasissets++;
    periods[0] =  2; periods[1] = 13; periods[2] = 17; periods[3] = 21; periods[4] = 42; 
    periods[5] = 28; periods[6] =  0; periods[7] =  0; periods[8] =  0; ncf.push_back(periods);
  basis = "6-31+G*";            basdir = "6-31+Gp";            numToBas += basis; numToDir += basdir; basToNum[basis] = numBasissets++;
    periods[0] =  2; periods[1] = 19; periods[2] = 23; periods[3] = 27; periods[4] = 49; 
    periods[5] = 34; periods[6] =  0; periods[7] =  0; periods[8] =  0; ncf.push_back(periods);
  basis = "6-31+G**";           basdir = "6-31+Gpp";           numToBas += basis; numToDir += basdir; basToNum[basis] = numBasissets++;
    periods[0] =  5; periods[1] = 19; periods[2] = 23; periods[3] = 27; periods[4] = 49; 
    periods[5] = 34; periods[6] =  0; periods[7] =  0; periods[8] =  0; ncf.push_back(periods);
  basis = "6-31++G";            basdir = "6-31++G";            numToBas += basis; numToDir += basdir; basToNum[basis] = numBasissets++;
    periods[0] =  3; periods[1] = 13; periods[2] = 17; periods[3] = 21; periods[4] = 42; 
    periods[5] = 28; periods[6] =  0; periods[7] =  0; periods[8] =  0; ncf.push_back(periods);
  basis = "6-31++G*";           basdir = "6-31++Gp";           numToBas += basis; numToDir += basdir; basToNum[basis] = numBasissets++;
    periods[0] =  3; periods[1] = 19; periods[2] = 23; periods[3] = 27; periods[4] = 49; 
    periods[5] = 34; periods[6] =  0; periods[7] =  0; periods[8] =  0; ncf.push_back(periods);
  basis = "6-31++G**";          basdir = "6-31++Gpp";          numToBas += basis; numToDir += basdir; basToNum[basis] = numBasissets++;
    periods[0] =  6; periods[1] = 19; periods[2] = 23; periods[3] = 27; periods[4] = 49; 
    periods[5] = 34; periods[6] =  0; periods[7] =  0; periods[8] =  0; ncf.push_back(periods);
  basis = "6-311G";             basdir = "6-311G";             numToBas += basis; numToDir += basdir; basToNum[basis] = numBasissets++;
    periods[0] =  3; periods[1] = 13; periods[2] = 21; periods[3] = 34; periods[4] = 39; 
    periods[5] = 39; periods[6] =  0; periods[7] =  0; periods[8] =  0; ncf.push_back(periods);
  basis = "6-311G*";            basdir = "6-311Gp";            numToBas += basis; numToDir += basdir; basToNum[basis] = numBasissets++;
    periods[0] =  3; periods[1] = 18; periods[2] = 26; periods[3] = 39; periods[4] = 46; 
    periods[5] = 44; periods[6] =  0; periods[7] =  0; periods[8] =  0; ncf.push_back(periods);
  basis = "6-311G**";           basdir = "6-311Gpp";           numToBas += basis; numToDir += basdir; basToNum[basis] = numBasissets++;
    periods[0] =  6; periods[1] = 18; periods[2] = 26; periods[3] = 39; periods[4] = 46; 
    periods[5] = 44; periods[6] =  0; periods[7] =  0; periods[8] =  0; ncf.push_back(periods);
  basis = "6-311+G";            basdir = "6-311+G";            numToBas += basis; numToDir += basdir; basToNum[basis] = numBasissets++;
    periods[0] =  3; periods[1] = 17; periods[2] = 25; periods[3] = 38; periods[4] = 51; 
    periods[5] = 43; periods[6] =  0; periods[7] =  0; periods[8] =  0; ncf.push_back(periods);
  basis = "6-311+G*";           basdir = "6-311+Gp";           numToBas += basis; numToDir += basdir; basToNum[basis] = numBasissets++;
    periods[0] =  3; periods[1] = 22; periods[2] = 30; periods[3] = 43; periods[4] = 58; 
    periods[5] = 48; periods[6] =  0; periods[7] =  0; periods[8] =  0; ncf.push_back(periods);
  basis = "6-311+G**";          basdir = "6-311+Gpp";          numToBas += basis; numToDir += basdir; basToNum[basis] = numBasissets++;
    periods[0] =  6; periods[1] = 22; periods[2] = 30; periods[3] = 43; periods[4] = 58; 
    periods[5] = 48; periods[6] =  0; periods[7] =  0; periods[8] =  0; ncf.push_back(periods);
  basis = "6-311++G";           basdir = "6-311++G";           numToBas += basis; numToDir += basdir; basToNum[basis] = numBasissets++;
    periods[0] =  4; periods[1] = 17; periods[2] = 25; periods[3] = 38; periods[4] = 51; 
    periods[5] = 43; periods[6] =  0; periods[7] =  0; periods[8] =  0; ncf.push_back(periods);
  basis = "6-311++G*";          basdir = "6-311++Gp";          numToBas += basis; numToDir += basdir; basToNum[basis] = numBasissets++;
    periods[0] =  4; periods[1] = 22; periods[2] = 30; periods[3] = 43; periods[4] = 58; 
    periods[5] = 48; periods[6] =  0; periods[7] =  0; periods[8] =  0; ncf.push_back(periods);
  basis = "6-311++G**";         basdir = "6-311++Gpp";         numToBas += basis; numToDir += basdir; basToNum[basis] = numBasissets++;
    periods[0] =  7; periods[1] = 22; periods[2] = 30; periods[3] = 43; periods[4] = 58; 
    periods[5] = 48; periods[6] =  0; periods[7] =  0; periods[8] =  0; ncf.push_back(periods);
  basis = "Scaled STO-3G";      basdir = "Scaled_STO-3G";      numToBas += basis; numToDir += basdir; basToNum[basis] = numBasissets++;
    periods[0] =  1; periods[1] =  5; periods[2] =  9; periods[3] = 13; periods[4] = 18; 
    periods[5] = 18; periods[6] = 22; periods[7] = 27; periods[8] = 27; ncf.push_back(periods);
  basis = "Scaled STO-3G*";     basdir = "Scaled_STO-3Gp";     numToBas += basis; numToDir += basdir; basToNum[basis] = numBasissets++;
    periods[0] =  1; periods[1] =  5; periods[2] = 14; periods[3] = 18; periods[4] = 23; 
    periods[5] = 23; periods[6] = 27; periods[7] = 32; periods[8] = 32; ncf.push_back(periods);
  basis = "Scaled 4-31G";       basdir = "Scaled_4-31G";       numToBas += basis; numToDir += basdir; basToNum[basis] = numBasissets++;
    periods[0] =  2; periods[1] =  9; periods[2] =  0; periods[3] =  0; periods[4] =  0; 
    periods[5] =  0; periods[6] =  0; periods[7] =  0; periods[8] =  0; ncf.push_back(periods);
  basis = "Scaled 4-31G*";      basdir = "Scaled_4-31Gp";      numToBas += basis; numToDir += basdir; basToNum[basis] = numBasissets++;
    periods[0] =  2; periods[1] = 15; periods[2] =  0; periods[3] =  0; periods[4] =  0; 
    periods[5] =  0; periods[6] =  0; periods[7] =  0; periods[8] =  0; ncf.push_back(periods);
  basis = "Scaled 4-31G**";     basdir = "Scaled_4-31Gpp";     numToBas += basis; numToDir += basdir; basToNum[basis] = numBasissets++;
    periods[0] =  5; periods[1] = 15; periods[2] =  0; periods[3] =  0; periods[4] =  0; 
    periods[5] =  0; periods[6] =  0; periods[7] =  0; periods[8] =  0; ncf.push_back(periods);
  basis = "Scaled 6-21G";       basdir = "Scaled_6-21G";       numToBas += basis; numToDir += basdir; basToNum[basis] = numBasissets++;
    periods[0] =  2; periods[1] =  9; periods[2] = 13; periods[3] =  0; periods[4] =  0; 
    periods[5] =  0; periods[6] =  0; periods[7] =  0; periods[8] =  0; ncf.push_back(periods);
  basis = "Scaled 6-21G*";      basdir = "Scaled_6-21Gp";      numToBas += basis; numToDir += basdir; basToNum[basis] = numBasissets++;
    periods[0] =  2; periods[1] =  9; periods[2] = 13; periods[3] =  0; periods[4] =  0; 
    periods[5] =  0; periods[6] =  0; periods[7] =  0; periods[8] =  0; ncf.push_back(periods);
  basis = "Scaled 6-21G**";     basdir = "Scaled_6-21Gpp";     numToBas += basis; numToDir += basdir; basToNum[basis] = numBasissets++;
    periods[0] =  5; periods[1] =  9; periods[2] = 13; periods[3] =  0; periods[4] =  0; 
    periods[5] =  0; periods[6] =  0; periods[7] =  0; periods[8] =  0; ncf.push_back(periods);
  basis = "Scaled 6-31G";       basdir = "Scaled_6-31G";       numToBas += basis; numToDir += basdir; basToNum[basis] = numBasissets++;
    periods[0] =  2; periods[1] =  9; periods[2] = 13; periods[3] = 17; periods[4] = 29; 
    periods[5] = 24; periods[6] =  0; periods[7] =  0; periods[8] =  0; ncf.push_back(periods);
  basis = "Scaled 6-31G*";      basdir = "Scaled_6-31Gp";      numToBas += basis; numToDir += basdir; basToNum[basis] = numBasissets++;
    periods[0] =  2; periods[1] = 15; periods[2] = 19; periods[3] = 23; periods[4] = 36; 
    periods[5] = 30; periods[6] =  0; periods[7] =  0; periods[8] =  0; ncf.push_back(periods);
  basis = "Scaled 6-31G**";     basdir = "Scaled_6-31Gpp";     numToBas += basis; numToDir += basdir; basToNum[basis] = numBasissets++;
    periods[0] =  5; periods[1] = 15; periods[2] = 19; periods[3] = 23; periods[4] = 36; 
    periods[5] = 30; periods[6] =  0; periods[7] =  0; periods[8] =  0; ncf.push_back(periods);
  basis = "Scaled 6-31+G";      basdir = "Scaled_6-31+G";      numToBas += basis; numToDir += basdir; basToNum[basis] = numBasissets++;
    periods[0] =  2; periods[1] = 13; periods[2] = 17; periods[3] = 21; periods[4] = 42; 
    periods[5] = 28; periods[6] =  0; periods[7] =  0; periods[8] =  0; ncf.push_back(periods);
  basis = "Scaled 6-31+G*";     basdir = "Scaled_6-31+Gp";     numToBas += basis; numToDir += basdir; basToNum[basis] = numBasissets++;
    periods[0] =  2; periods[1] = 19; periods[2] = 23; periods[3] = 27; periods[4] = 49; 
    periods[5] = 34; periods[6] =  0; periods[7] =  0; periods[8] =  0; ncf.push_back(periods);
  basis = "Scaled 6-31+G**";    basdir = "Scaled_6-31+Gpp";    numToBas += basis; numToDir += basdir; basToNum[basis] = numBasissets++;
    periods[0] =  5; periods[1] = 19; periods[2] = 23; periods[3] = 27; periods[4] = 49; 
    periods[5] = 34; periods[6] =  0; periods[7] =  0; periods[8] =  0; ncf.push_back(periods);
  basis = "Scaled 6-31++G";     basdir = "Scaled_6-31++G";     numToBas += basis; numToDir += basdir; basToNum[basis] = numBasissets++;
    periods[0] =  3; periods[1] = 13; periods[2] = 17; periods[3] = 21; periods[4] = 42; 
    periods[5] = 28; periods[6] =  0; periods[7] =  0; periods[8] =  0; ncf.push_back(periods);
  basis = "Scaled 6-31++G*";    basdir = "Scaled_6-31++Gp";    numToBas += basis; numToDir += basdir; basToNum[basis] = numBasissets++;
    periods[0] =  3; periods[1] = 19; periods[2] = 23; periods[3] = 27; periods[4] = 49; 
    periods[5] = 34; periods[6] =  0; periods[7] =  0; periods[8] =  0; ncf.push_back(periods);
  basis = "Scaled 6-31++G**";   basdir = "Scaled_6-31++Gpp";   numToBas += basis; numToDir += basdir; basToNum[basis] = numBasissets++;
    periods[0] =  6; periods[1] = 19; periods[2] = 23; periods[3] = 27; periods[4] = 49; 
    periods[5] = 34; periods[6] =  0; periods[7] =  0; periods[8] =  0; ncf.push_back(periods);
}

///////////////////////////////////////////////////////////////////////////////
///// Static Member Data                                                  /////
///////////////////////////////////////////////////////////////////////////////

bool Basisset::isInitialized = false; ///< Holds the initialisation status of the class.
QStringList Basisset::numToBas = QStringList(); ///< The list of basisset names. Maps basisset indices to names
QStringList Basisset::numToDir = QStringList(); ///< The list of basisset directories.
QMap <QString, unsigned int> Basisset::basToNum = QMap<QString, unsigned int>(); ///< Maps basisset names to indices.
unsigned int Basisset::numBasissets = 0; ///< The number of basissets present
std::vector < std::vector<unsigned int> > Basisset::ncf; ///< The number of contracted functions for each basis set and part of row in the periodic table.

