/***************************************************************************
                         globalbase.h  -  description
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

/// \file
/// Contains the declaration of the class GlobalBase.

#ifndef GLOBALBASE_H
#define GLOBALBASE_H

///// Forward class declarations & header files ///////////////////////////////

///// Qt forward class declarations
class QDomElement;
class QString;

///// Base class header file
#include <globalwidget.h>


///// class GlobalBase ////////////////////////////////////////////////////////
class GlobalBase : public GlobalWidget
{
  Q_OBJECT

  public:
    GlobalBase(QWidget* parent = 0, const char* name = 0, bool modal = FALSE, WFlags fl = 0);       // constructor
    ~GlobalBase();                      // destructor

    ///// public enums
    enum Calculation{SinglePointEnergy, EnergyAndForces, GeometryOptimization, Frequencies};        ///< for function calculationType
    enum Buur{NoBuur, PC, SM};          ///< for function buurType
    enum Run{Xbrabo, Script, Queue};    ///< for function runType

    void setDefaultName(const QString name);      // sets the default calculation name
    unsigned int calculationType() const;         // returns the type of calculation
    unsigned int buurType() const;      // returns the type of crystal
    bool extendedFormat() const;        // returns whether extended format is to be used
    QString description() const;        // returns the description
    QString name() const;               // returns the name
    QString directory() const;          // returns the directory
    unsigned int runType() const;       // returns the type of run
    unsigned int queue() const;         // returns the queue
    void allowChanges(const bool status);         // enables/disables changing every option

    void loadCML(const QDomElement* root);        // loads the widget data from a CML file
    void saveCML(QDomElement* root);    // saves the widget data to a CML file
   
  public slots:
    void reset();                       // resets the widgets

  protected slots:
    void accept();                      // is called when the changes are accepted (OK clicked)
    void reject();                      // is called when the changes are rejected (Cancel or X clicked)

  private slots:
    void setChanged(const bool state = true);     // sets the 'changed' property of the dialog
    void correctType(int index);        // corrects the calculation type for unimplemented choices
    void chooseDir();                   // chooses a working dir
    
  private:
    ///// private member functions
    void makeConnections();             // sets up the permanent connections
    void init();                        // initializes the dialog
    void saveWidgets();                 // saves widgets appearances
    void restoreWidgets();              // restores widgets appearances

    ///// private structs 
    struct WidgetData
    /// Struct local to the class GLobalBase containing the data pertaining to all
    /// widgets.
    {
      unsigned int      type;
      bool              useBuur;
      unsigned int      buurType;
      bool              useXF;
      QString           description;
      QString           name;
      QString           directory;
      unsigned int      runType;
      unsigned int      queue;
    };

    WidgetData data;                    ///< Internal data structure.
    QString defaultName;                ///< Name() when reset is issued.
    bool widgetChanged;                 ///< Holds the 'changed' property of the widget.
};

#endif

