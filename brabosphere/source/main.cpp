/***************************************************************************
                          main.cpp  -  description
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

/*!
  \mainpage The Brabosphere code documentation

  \author Ben Swerts
  \section s_intro Introduction

  These pages contain the documentation for the source code of the Brabosphere
  project. Consequently this is not a user manual but geared towards 
  developers. At the moment I'm the only one working on this project so these 
  pages are probably only useful for myself. On the other hand, for someone who wants
  to study the code (to extract some technique out of it) these pages might come
  in very handy.

  \section s_xbrabo Xbrabo

  The project started under the working title 'Xbrabo'. As some class names and
  other code fragments are based on this name this will never be changed. The code
  will continue to be developed under the working title and only the visible
  interface will show the actual project name.

  \section s_license License

  Copyright &copy; 2002-2006 by Ben Swerts.

  This program is free software; you can redistribute it and/or modify  
  it under the terms of the GNU General Public License as published by  
  the Free Software Foundation; either version 2 of the License, or     
  (at your option) any later version.                                   

*/

/// \file 
/// Your standard C++ main file.

// C++ header files
#include <cstdlib>
#include <ctime>

// Qt header files
#include <qapplication.h>
#include <qfont.h>
#include <qbitmap.h>
#include <qimage.h>
#include <qstring.h>
#include <qtextcodec.h>
#include <qgl.h>

// Xbrabo header files
#include "iconsets.h"
#include "xbrabo.h"

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
/// The starting point of the application.
{
  qInstallMsgHandler(debugHandler); // enables to inhibit all debug messages by a single ifdef
  QApplication app(argc, argv);

  if (!QGLFormat::hasOpenGL())
    qFatal("This system has no working OpenGL support. Exiting...");

  ///// Check whether the splash screen should be shown
  bool showSplash = true;
  for(int i = 1; i < argc; i++)
  {
    if(QString(argv[i]).contains("-nosplash")) 
    {
      showSplash = false;
      break;
    }
  }
  ///// Show a splash screen centered on the desktop
  QWidget* splash;
  if(showSplash)
  {
    //QTime timer;
    //timer.restart();
    splash = new QWidget(0, 0, Qt::WStyle_Customize | Qt::WStyle_NoBorder | Qt::WStyle_StaysOnTop | Qt::WX11BypassWM);
    QPixmap pm(IconSets::getSplash());
    splash->resize(pm.size());
    splash->setBackgroundPixmap(pm);
    if(pm.mask())
      splash->setMask(*pm.mask());
    QRect screenRect = app.desktop()->availableGeometry(app.desktop()->primaryScreen());
    splash->move(screenRect.width()/2 - 309, screenRect.height()/2 - 193);
    splash->show();
  } // all stack-allocated variables are deleted here, before the app starts
  //qDebug("time needed to show the splash screen: %d ms",timer.restart());

  //QTranslator tor( 0 );
  // set the location where your .qm files are in load() below as the last parameter instead of "."
  // for development, use "/" to use the english original as
  // .qm files are stored in the base project directory.
  //tor.load( QString("xbrabo.") + QTextCodec::locale(), "." );
  //app.installTranslator( &tor );

  // the random number generator (needed for calculating hydrogen orbitals)
  srand(time(NULL));

  Xbrabo* xbrabo = new Xbrabo();
  app.setMainWidget(xbrabo);
  if(showSplash)
  {
    xbrabo->show();
    app.processEvents(); // otherwise it will not update the screen
  }
  xbrabo->init(); // this will show the mainwindow being built in case of using the splash screen
  if(showSplash)
    app.processEvents(); 
  ///// load all calculations specified on the command line
  for(int i = 1; i < argc; i++)
  {
    if(QString(argv[i]).right(4) == ".cml")
    {
      xbrabo->fileOpen(QString(argv[i]));
      if(showSplash)
        app.processEvents(); 
    }
  }
  if(showSplash)
    splash->deleteLater();
  //qDebug("time needed to start the application: %d ms",timer.restart());
  return app.exec();
}

