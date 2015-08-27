/***************************************************************************
                        plotmaplabel.h  -  description
                             -------------------
    begin                : Tue Mar 29 2005
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
/// Contains the declaration of the class PlotMapLabel

#ifndef PLOTMAPLABEL_H
#define PLOTMAPLABEL_H

///// Forward class declarations & header files ///////////////////////////////

// STL header files
#include <vector>
using std::vector;

// Qt forward class declarations
#include <qcolor.h>
class QPixmap;
#include <qpointarray.h>

// Xbrabo header files
#include "point3d.h"

// Base class header file
#include <qlabel.h>

///// class PlotMapLabel //////////////////////////////////////////////////////
class PlotMapLabel : public QLabel
{
  public:
    ///// constructor/destructor
    PlotMapLabel(QWidget* parent, const char* name = 0, WFlags fl = 0);         // constructor
    ~PlotMapLabel();                    // destructor

    ///// public member functions
    void setDrawStyle(const unsigned int style, const bool cubic);  // sets the drawing style
    void setDensityPixmap(const QPixmap& pixmap); // sets the pixmap containing density values
    void setCrosses(const vector<Point3D<double> >& centers);        // sets the positions of the atoms to be shown and their color
    void setGrid(const Point3D<unsigned int>& pointDimension, const Point3D<double>& pointDelta);         // sets the parameters defining the grid
    void setIsoCurves(const vector< vector<Point3D<double> >* >& curves, const vector<QColor>& lineColors); // ordered points making up isocurves
    void setPolygons(const vector< vector<Point3D<double> > >& polygons, const vector<QColor>& colors); // sets the data for the drawing of polygons
    void setColors(const QColor& zero, const QColor& atom); // sets the primary colors use for drawing 
    void startDrag(const QPoint& pos);  // indicates a drag is started at pos (labelcoordinates)
    void moveDrag(const QPoint& pos);   // indicates actual dragging
    void endDrag();                     // indicates dragging has ended

  protected:
    ///// protected member functions
    virtual void drawContents(QPainter* p);       // does the actual drawing

  private:
    ///// private member functions
    QPointArray pointCurve(const vector<Point3D<double> >& curve, const bool cubicInterpolation, const double scaleX, const double scaleY, const unsigned int numInterpolated = 0);  // Returns the polygon with the specified curve and linearly or cubically interpolated
    QPointArray cardinalSpline(const Point3D<double>& controlPoint1, const Point3D<double>& startPoint, const Point3D<double>& endPoint, const Point3D<double>& controlPoint2, const unsigned int numInterpolated); // calculate the point for a cardinal spline defined by the 4 given points

    ///// private member data
    QPixmap* densityPixmap;             ///< A local copy of the pixmap containing density values.
    vector<Point3D<double> > crosses;   ///< The atomic positions to be drawn as crosses.
    Point3D<unsigned int> numPoints;    ///< A Point3D containing the number of points in 2 directions.
    Point3D<double> delta;              ///< A Point3D containing the cell lengths in  2 directions.
    QPoint startPosition;               ///< Starting position for a mousedrag rectangle.
    QPoint endPosition;                 ///< Ending position for a mousedrag rectangle.
    bool drawDrag;                      ///< = true if a dragging rectangle should be drawn.
    vector< vector<Point3D<double> >* > isoCurves; ///< Contains the data for drawing the isolines.
    //vector<QColor> isoCurveColors;      
    vector<QColor> isoLineColors;       ///< Contains the color of each isoline.
    QColor crossColor;                  ///< The color the crosses should be drawn in.
    QColor zeroColor;                   ///< The background color.
    unsigned int drawStyle;             ///< The drawing style.
    bool cubicInterpolation;            ///< Interpolation type. Linear or cubic interpolation.
    vector< vector<Point3D<double> > > isoPolygons;         ///< Contains the data for drawing the polygons
    vector<QColor> isoPolygonColors;    ///< Contains the color of each polygon.
};

#endif

