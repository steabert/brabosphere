/***************************************************************************
                     densityloadthread.h  -  description
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

/// \file
/// Contains the declaration of the class DensityLoadThread.

#ifndef DENSITYLOADTHREAD_H
#define DENSITYLOADTHREAD_H

///// Forward class declarations & header files ///////////////////////////////

// STL includes
#include <vector>

// Qt forward class declarations
class QTextStream;

// Xbrabo forward class declarations
class DensityBase;

// Base class header files
#include <qthread.h>

///// class DensityLoadThread /////////////////////////////////////////////////
class DensityLoadThread : public QThread
{
  public:
    ///// constructor/destructor
    DensityLoadThread(std::vector<double>* densityPoints, QTextStream* stream, DensityBase* densityDialog, const unsigned int numSkipValues, const unsigned int totalPoints);       // constructor
    ~DensityLoadThread();               // destructor

    ///// pure virtuals
    virtual void run();                 // reimplementation of this pure virtual does the actual work
    
    ///// other public member functions
    void stop();                        // requests stopping the thread
    bool success();                     // returns true if everything loaded succesfully

  private:
    ///// private member data
    std::vector<double>* data;          ///< The pointer to the recipient for the data.
    QTextStream* textStream;            ///< The pointer to the datastream.
    unsigned int numSkip;               ///< The number of values to skip at each read.
    unsigned int numValues;             ///< The total number of values to read. 
    bool stopRequested;                 ///< Is set to true if the thread should be stopped.
    DensityBase* parent;                ///< The widget which should get notifications.
    unsigned int progress;              ///< Used to transfer the progress to the parent dialog.
};

#endif

