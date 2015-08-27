/***************************************************************************
                     densityloadthread.cpp  -  description
                             -------------------
    begin                : Thu Mar 24 2005
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
  \class DensityLoadThread
  \brief This class loads the density data for the class DensityBase.
*/
/// \file
/// Contains the implementation of the class DensityLoadThread.

///// Header files ////////////////////////////////////////////////////////////

// C++ header files
#include <cassert>

// Qt header files
#include <qapplication.h>
#include <qevent.h>
#include <qfile.h>
#include <qtextstream.h>

// Xbrabo header files
#include "densitybase.h"
#include "densityloadthread.h"

///////////////////////////////////////////////////////////////////////////////
///// Public Member Functions                                             /////
///////////////////////////////////////////////////////////////////////////////

///// Constructor /////////////////////////////////////////////////////////////
DensityLoadThread::DensityLoadThread(std::vector<double>* densityPoints, QTextStream* stream, DensityBase* densityDialog, const unsigned int numSkipValues, const unsigned int totalPoints) : QThread(), 
  data(densityPoints), 
  textStream(stream), 
  numSkip(numSkipValues), 
  numValues(totalPoints),
  stopRequested(false),
  parent(densityDialog) // according to GCC this one should be last to coincide with the declaration order
                        // But now it doe'sn't anymore AFAICS
/// The default constructor.
/// \param[out] densityPoints : the resulting density values read from file.
/// \param[in] stream : the stream connected to an opened file.
/// \param[in] densityDialog : the parent DensityBase widget were messages are sent to.
/// \param[in] numSkipValues : the number of points to skip when reading from the stream.
/// \param[in] totalPoints : the total number of points to read.
{
  assert(data != 0);
  assert(textStream != 0);
  assert(parent != 0);
}

///// Destructor //////////////////////////////////////////////////////////////
DensityLoadThread::~DensityLoadThread()
/// The default destructor.
{

}

///// run /////////////////////////////////////////////////////////////////////
void DensityLoadThread::run()
/// Does the actual reading after the proper parameters
/// have been set. It is run with a call to start().
{  
  ///// get the QFile pointer from the stream
  QFile* file = dynamic_cast<QFile*>(textStream->device());
  assert(file != 0);

  if(numValues == 0)
  {
    delete textStream;
    delete file;
    return;
  }

  data->clear();
  data->reserve(numValues);
  const unsigned int updateFreq = numValues/100;
  double value = 0.0;

  for(unsigned int i = 0; i < numValues; i++)
  {
    // read the next density point
    *textStream >> value;
	  data->push_back(value);
    if(i % updateFreq == 0)
    {
      progress = i;
      QCustomEvent* e = new QCustomEvent(static_cast<QEvent::Type>(1001),&progress);
      QApplication::postEvent(parent, e);
    }
    // skip the next numSkipValues density points from other MO's
    for(unsigned int skip = 0; skip < numSkip; skip++)
      *textStream >> value;
    if(stopRequested || textStream->atEnd())
      break;
  }

  // cleanup if stopped prematurely
  if(data->size() != numValues)
    data->clear();

  // cleanup
  delete textStream;
  delete file;

  // notify the thread has ended
  QCustomEvent* e = new QCustomEvent(static_cast<QEvent::Type>(1002));
  QApplication::postEvent(parent, e);
}

///// stop ////////////////////////////////////////////////////////////////////
void DensityLoadThread::stop()
/// Requests the thread to stop.
{
  stopRequested = true;
}

///// success /////////////////////////////////////////////////////////////////
bool DensityLoadThread::success()
/// Returns whether the desired number of points was succesfully read.  
{
  return data->size() == numValues;
}

