/***************************************************************************
                      orbitalviewerbase.h  -  description
                             -------------------
    begin                : Thu Nov 4 2004
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
/// Contains the declaration of the class OrbitalViewerBase

#ifndef ORBITALVIEWERBASE_H
#define ORBITALVIEWERBASE_H

///// Forward class declarations & header files ///////////////////////////////

///// Qt forward class declarations
class QHBoxLayout;
class QTimer;

///// Xbrabo forward class declarations
class ColorButton;
class GLOrbitalView;
class OrbitalOptionsWidget;
class OrbitalThread;

///// Base class header file
#include <qdialog.h>

///// class OrbitalViewerBase /////////////////////////////////////////////////
class OrbitalViewerBase : public QDialog
{
  Q_OBJECT
  
  public:
    // constructor/destructor
    OrbitalViewerBase(QWidget* parent = 0, const char* name = 0, bool modal = FALSE, WFlags fl = 0);// constructor
    ~OrbitalViewerBase();               // destructor

  protected:
    void customEvent(QCustomEvent* e);  // reimplemented to receive events from calcThread
    
  private slots:
    void update();                      // updates the view with the values of the widgets
    void adjustL(int newN);             // adjust the orbital quantum number to the region 1 - n-1
    void adjustM(int newL);             // adjust the angular momentum quantum number to the region -l - +l
    void updateColors();                // updates the view with new colors
    void updateTypeOptions(int type);   // updates the options to correspond to the chosen type
    void cancelCalculation();           // stops calculating a new orbital
    
  private:
    // private member functions
    void finishCalculation();           // finished up a calculation

    // private member variables
    QHBoxLayout* BigLayout;             ///< All encompassing horizontal layout.
    OrbitalOptionsWidget* options;      ///< Shows the options.
    GLOrbitalView* view;                ///< Shows the orbital.
    ColorButton* ColorButtonPositive;   ///< The pushbutton for choosing the colour of the positive values.
    ColorButton* ColorButtonNegative;   ///< The pushbutton for choosing the colour of the negative values.
    OrbitalThread* calcThread;          ///< The thread doing the calculation.
    QTimer* timer;                      ///< Handles periodic updating of the view during a calculation.
};

#endif

