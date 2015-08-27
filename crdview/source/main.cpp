/***************************************************************************
                          main.cpp  -  description
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

/// \file
/// Your standard C++ main file.

///// Header files ////////////////////////////////////////////////////////////

// Qt header files
#include <qapplication.h>
#include <qfont.h>
#include <qstring.h>
#include <qtextcodec.h>
#include <qtranslator.h>

// CrdView header files
#include "crdfactory.h"
#include "crdview.h"

///// debugHandler ////////////////////////////////////////////////////////////
static void debugHandler(QtMsgType type, const char* message)
/// Custom handler for debugging, warning and fatal messages.
/// The message itself is already formatted by the respective qDebug, qWarning
/// and QFatal functions.
{
  switch (type) 
  {
#ifndef QT_NO_DEBUG // only print out debug and warning messages in debug builds
    case QtDebugMsg:   fprintf(stdout, "%s\n", message);
                       break;
    case QtWarningMsg: fprintf(stderr, "Warning: %s\n", message);
                       break;
#endif
    case QtFatalMsg:   fprintf(stderr, "FATAL: %s\n", message);
                       fprintf(stderr, "Please note the exact message when reporting this.\n");
                       fprintf(stderr, "The application will now shut down.\n");
                       abort();
  }
}

///// main ////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[])
{
  // Initialize the Qt application object.
  qInstallMsgHandler(debugHandler); // enables to inhibit all debug messages by a single ifdef
  QApplication a(argc, argv);

  ///// now the Qt options have been removed from the argument list, so read
  ///// in the possible input and output filenames
  ///// USAGE: crdview [ <Qt options> ] [ -bnf ] [ <input file> [ <output file> ]]


  // read the rest of the command line into a QStringList
  QStringList argList;
  bool extendedFormat = true;
  for(int i = 1; i <= argc; i++)
  {
    QString argument = argv[i];
    if(argument.isEmpty())
      break;
    if(argument.left(1) != "-")
      argList += argument;
    else if(argument.lower() = "-bnf") // brabo normal format (extended is the default for writing)
      extendedFormat = false;
  }
  // check whether a conversion is needed
  if(argList.count() >= 2)
  {
    QString inputFileName = *(argList.begin());
    QString outputFileName = *(++argList.begin());
    qDebug("input & out filenames for conversion: |"+inputFileName+"|"+outputFileName+"|");
    int result = CrdFactory::convert(inputFileName, outputFileName, extendedFormat);
    switch(result)
    {
      case 1: break;
    }
    exit(0); // only conversion, no GUI
  }

  // check for OpenGL support after any conversion has finished
  if (!QGLFormat::hasOpenGL())
  {
    qWarning("This system has no working OpenGL support. Exiting...");
    return -1;
  }


  //QTranslator tor( 0 );
  // set the location where your .qm files are in load() below as the last parameter instead of "."
  // for development, use "/" to use the english original as
  // .qm files are stored in the base project directory.
  //tor.load( QString("crdview.") + QTextCodec::locale(), "." );
  //a.installTranslator( &tor );

  CrdView* crdview = new CrdView();
  a.setMainWidget(crdview);

  // preload coordinates if a filename was given on the command line
  if(argList.count() == 1)
  {
    qDebug("preloading: " + QString(*(argList.begin())));
    crdview->readCoordinates(*(argList.begin()));
  }

  // Let the show begin!
  crdview->show();
  return a.exec();
}

