/***************************************************************************
                         plotmapbase.h  -  description
                             -------------------
    begin                : Tue Jul 22 2003
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

/// \file
/// Contains the declaration of the class PlotMapBase

#ifndef PLOTMAPBASE_H
#define PLOTMAPBASE_H

///// Forward class declarations & header files ///////////////////////////////

///// C++ forward class declarations
#include <map>
#include <vector>
using std::map;
using std::vector;

///// Qt forward class declarations
class QColor;
class QImage;
class QPoint;
class QString;

///// Base class header file
#include <plotmapwidget.h>
class PlotMapExtensionWidget;
class PlotMapLabel;
#include "point3d.h"
 
///// class PlotMapBase ////////////////////////////////////////////////////////
class PlotMapBase : public PlotMapWidget
{
  Q_OBJECT

  public:
    PlotMapBase(QWidget* parent = 0, const char* name = 0, bool modal = FALSE, WFlags fl = 0);       // constructor
    ~PlotMapBase();                     // destructor

    bool loadMapFile(const QString filename, const bool noerrors = false);      // Loads a map file with a specified name

  public slots:  
    bool loadMapFile();                 // Loads a map file and asks for a filename

  signals:
    void optionsChanged();              // signal which fires when optiosn have changed
    
  protected:
    void mousePressEvent(QMouseEvent* e);         // handles mouse press events
    void mouseReleaseEvent(QMouseEvent* e);       // handles mouse release events
    void mouseMoveEvent(QMouseEvent* e);          // handles the mouse move events for the pixmap
    //void paintEvent(QPaintEvent* e);    // handles repaints of the widget

  private slots:
    void saveImage();                   // saves the image
    void showOptions();                 // shows the options widget
    void applyOptions();                // applies the options
    void autoApply(bool state);         // turns on/off auto applying of options
    void resetOptions();                // resets the options to their defaults
       
  private:
    void makeConnections();             // Sets up all permanent connections
    void init();                        // Initializes the dialog
    void updateImage();                 // Updates the image from the current settings
    void updatePixmap();                // update the pixmap from density values
    void updateCrosses();               // update the atoms to be drawn as crosses
    void updateIsoLines();              // update the points making up the isolines
    QColor plotColor(const QColor foregroundColor, const QColor backgroundColor, const double opaqueness);    // returns the foreground with opaqueness on the backgroundColor
    bool onPixmap(const QPoint position);         // returns whether the position is on the pixmaplabel
    QPoint mapToImage(const QPoint position);     // returns the image-pixel-position of position
    Point3D<double> interpolate2D(const Point3D<double>& point1, const Point3D<double>& point2, const double density1, const double density2, const double isoLevel);       // calculates the interpolated point  
    bool hasPoint(const unsigned int segment, const unsigned int level, const vector<map<unsigned int, unsigned int>* >& segmentMaps); // returns true if segment has a Point3D for level
    Point3D<double> getPoint(const unsigned int segment, const unsigned int level, const vector<map<unsigned int, unsigned int>* >& segmentMaps, const vector< vector<Point3D<double> >* >& segments); // returns the point correspondig to a segment
    void startSearch(const unsigned int segment, const unsigned int level, const bool circular, const vector<map<unsigned int, unsigned int>* >& segmentMaps, vector<bool>& segmentVisited, const vector< vector<Point3D<double> >* >& segments); // starts a search for an isoline
    bool getNeighbours(const unsigned int currentSegment, const unsigned int previousSegment, unsigned int& segment1, unsigned int& segment2, unsigned int& segment3); // returns the neighbouring segments (false if edge)
    bool isHorizontal(const unsigned int segment); // returns true if a segment is horizontal
    void continuePolygon(const unsigned int startCurve, const unsigned int endCurve, unsigned int currentCurve, vector<Point3D<double> >& isoPolygon, vector<bool>& curveAdded, const Point3D<double>& startPoint, const bool fromStart, const double isoLevel); // recursively completes a polygon
    void nextPolygonCCWT(const unsigned int startCurve, const unsigned int endCurve, vector<Point3D<double> >& isoPolygon, vector<bool>& curveAdded, const Point3D<double>& startPoint, const Point3D<double>& refPoint, const double isoLevel); // search for the next curve to add in the CCW direction on the top row
    void nextPolygonCWT(const unsigned int startCurve, const unsigned int endCurve, vector<Point3D<double> >& isoPolygon, vector<bool>& curveAdded, const Point3D<double>& startPoint, const Point3D<double>& refPoint, const double isoLevel); // search for the next curve to add in the CW direction on the top row
    void nextPolygonCCWR(const unsigned int startCurve, const unsigned int endCurve, vector<Point3D<double> >& isoPolygon, vector<bool>& curveAdded, const Point3D<double>& startPoint, const Point3D<double>& refPoint, const double isoLevel); // search for the next curve to add in the CCW direction on the right column
    void nextPolygonCWR(const unsigned int startCurve, const unsigned int endCurve, vector<Point3D<double> >& isoPolygon, vector<bool>& curveAdded, const Point3D<double>& startPoint, const Point3D<double>& refPoint, const double isoLevel); // search for the next curve to add in the CW direction on the right column
    void nextPolygonCCWB(const unsigned int startCurve, const unsigned int endCurve, vector<Point3D<double> >& isoPolygon, vector<bool>& curveAdded, const Point3D<double>& startPoint, const Point3D<double>& refPoint, const double isoLevel); // search for the next curve to add in the CCW direction on the bottom row
    void nextPolygonCWB(const unsigned int startCurve, const unsigned int endCurve, vector<Point3D<double> >& isoPolygon, vector<bool>& curveAdded, const Point3D<double>& startPoint, const Point3D<double>& refPoint, const double isoLevel); // search for the next curve to add in the CW direction on the bottom row
    void nextPolygonCCWL(const unsigned int startCurve, const unsigned int endCurve, vector<Point3D<double> >& isoPolygon, vector<bool>& curveAdded, const Point3D<double>& startPoint, const Point3D<double>& refPoint, const double isoLevel); // search for the next curve to add in the CCW direction on the left column
    void nextPolygonCWL(const unsigned int startCurve, const unsigned int endCurve, vector<Point3D<double> >& isoPolygon, vector<bool>& curveAdded, const Point3D<double>& startPoint, const Point3D<double>& refPoint, const double isoLevel); // search for the next curve to add in the CW direction on the left column
    QRect labelRect() const;            // return the inner geometry of plotLabel with respect to the widget

    template<typename T> void limitRange(const T& min, T& value, const T& max);
                
    ///// private member data
    Point3D<unsigned int> numPoints;    ///< The number of grid points in 2 directions.
    unsigned int numAtoms;              ///< The number of atoms.
    vector< vector<double> > points;    ///< The values of the grid points: points[numPointsY][NumPointsX].
    vector< Point3D<double> > coords;   ///< The coordinates of the atoms.
    PlotMapExtensionWidget* options;    ///< The widget that allows changing the options.
    QPoint mousePosition;               ///< Holds the start position of a mouse drag.
    PlotMapLabel* plotLabel;            ///< Shows the resulting map.
    bool loadInProgress;                ///< Is set to true if the points array is being updated.
    Point3D<double> origin, delta;      ///< The origin and cell size for the 3D grid. 
    double maxValue, minValue;          ///< The maximum and minimum values of the grid.
    vector< vector<Point3D<double> >* > isoCurves;///< A data structure containing all isodensity curves.
    vector<QColor> isoCurveColors;      ///< A vector containing the color of each curve.
    
    ///// static private constants
    static const double AUTOANG;        ///< Conversion factor atomic units -> angstrom.
};

#endif

