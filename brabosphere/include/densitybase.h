/***************************************************************************
                         densitybase.h  -  description
                             -------------------
    begin                : Thu Mar 17 2005
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
/// Contains the declaration of the class DensityBase

#ifndef DENSITYBASE_H
#define DENSITYBASE_H

///// Forward class declarations & header files ///////////////////////////////

// STL includes
#include <vector>

// Xbrabo forward class declarations
class DensityLoadThread;
class IsoSurface;

// Xbrabo includes
#include <point3d.h>

// Base class header files
#include "densitywidget.h"

///// class DensityBase ///////////////////////////////////////////////////////
class DensityBase : public DensityWidget
{
  Q_OBJECT

  public:
    ///// constructor/destructor
    DensityBase(IsoSurface* surface, QWidget* parent = 0, const char* name = 0, bool modal = FALSE, WFlags fl = 0);      // constructor
    ~DensityBase();                     // destructor

    ///// public member function for retrieving data
    bool surfaceVisible(const unsigned int surface);        // returns true if a surface is visible
    QColor surfaceColor(const unsigned int surface);        // returns the color of a surface
    unsigned int surfaceOpacity(const unsigned int surface);// returns the opacity of a surface
    unsigned int surfaceType(const unsigned int surface);   // returns the drawing type of a surface

  signals:
    void newSurface(const unsigned int surface);  // is emitted after a new surface is created
    void updatedSurface(const unsigned int surface);        // is emitted when a surface has changed
    void deletedSurface(const unsigned int surface);        // is emitted when an existing surface is deleted
    void redrawScene();                 // is emitted when something has changed

  public slots:
    void loadDensityA();                // loads a new cube file for density A
    void loadDensityB();                // loads a new cube file for density B
    void addSurface();                  // adds a new surface
    void addSurfacePair();              // adds a pair of surfaces with opposite signs
    void deleteSurface();               // deletes an existing surface
    void updateAll();                   // updates all changes

  protected:
    void customEvent(QCustomEvent* e);  // reimplemented to receive events from loadingThread
    void showEvent(QShowEvent* e);      // reimplemented to keep the colour column fixed after a hide/show cycle
    void hideEvent(QHideEvent* e);      // reimplemented to keep the colour column fixed after a hide/show cycle

  private slots:
    void updateSliderLevel();           // updates SliderLevel from the contents of LineEditLevel
    void updateLineEditLevel();         // updates LineEditLevel from the value of SliderLevel
    void updateListView();              // updates ListViewParameters upon changes in the settings
    void updateSettings();              // updates the settings upon changes in ListViewParameters
    void updateVisibility(QListViewItem* item, const QPoint&, int column); // updates the visibility of a surface
    void updateOperation(const unsigned int density = 0);   // updates the possible operations 
    void updateOpacity();               // updates LabelOpacity with the current opacity value

  private:
    ///// private enums
    enum Columns{COLUMN_VISIBLE, COLUMN_ID, COLUMN_RGB, COLUMN_LEVEL, COLUMN_COLOUR, COLUMN_OPACITY, COLUMN_TYPE}; ///< Indices of each column in the ListView

    ///// private member functions
    void makeConnections();             // sets up all connections
    void loadDensity(const bool densityA);        // loads a density for density A or B
    void updateDensity();               // updates everything after loading has finished
    void updateProgress(const unsigned int progress);       // updates the progressbar for the current loading density
    unsigned int typeToNum(const QString& type);      // translates the type into a number
    void enableWidgets();               // enables/disables the correct widgets depending on the status of the class
    bool identicalGrids();              // returns true if the grids of densityA and B are identical

    ///// private structs
    struct SurfaceProperties            
    /// Contains the properties of a surface.
    {
      bool visible;                     ///< = true if the surface is visible
      double level;                     ///< The isolevel at which the surface is determined
      unsigned int colour;              ///< The colour of the surface
      unsigned int opacity;             ///< The opacity of the surface
      unsigned int type;                ///< The rendering type (solid, wireframe, dots)
      bool deleted;                     ///< = true if the surface was deleted
      bool isNew;                       ///< = true if the surface is a new one
      unsigned int ID;                  ///< The ID of the surface
    };

    ///// private member data
    IsoSurface* isoSurface;             ///< A pointer to the IsoSurface.
    unsigned int idCounter;             ///< A counter for uniquely identifying defined surfaces.
    std::vector<SurfaceProperties> surfaceProperties;       ///< A list of the properties of each defined surface.
    DensityLoadThread* loadingThread;   ///< A thread that does the actual reading of the density points from the cube file.
    std::vector<double> densityPointsA; ///< An array holding the density values for Density A.
    std::vector<double> densityPointsB; ///< An array holding the density values for Density B.
    bool loadingDensityA;               ///< Indicates which density is loading.
    Point3D<float> originA;             ///< Holds the coordinates of the origin of density A.
    Point3D<float> originB;             ///< Holds the coordinates of the origin of density B.
    Point3D<unsigned int> numPointsA;   ///< Holds the number of data points in each direction of density A.
    Point3D<unsigned int> numPointsB;   ///< Holds the number of data points in each direction of density B.
    Point3D<float> deltaA;              ///< Holds the cell lengths of density A.
    Point3D<float> deltaB;              ///< Holds the cell lengths of density B.
    QString newDescription;             ///< Holds the description of the contents of a new density.
    int columnColourWidth;              ///< Holds the right column width for the one containing the colour of the surface.

    ///// static private member data
    static const double deltaLevel;     ///< The minimal change allowed in isoLevels.
};
#endif

