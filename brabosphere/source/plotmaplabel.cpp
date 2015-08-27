/***************************************************************************
                       plotmaplabel.cpp  -  description
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

///// Comments ////////////////////////////////////////////////////////////////
/*!
  \class PlotMapLabel
  \brief This class does the drawing of a 2D density map including isodensity lines.

  The input is provided by a parent PlotMapBase class.
*/
/// \file
/// Contains the implementation of the class PlotMapLabel

///// Header files ////////////////////////////////////////////////////////////

// Qt header files
#include <qpainter.h>
#include <qpixmap.h>

// Xbrabo header files
#include "plotmaplabel.h"

///////////////////////////////////////////////////////////////////////////////
///// Public Member Functions                                             /////
///////////////////////////////////////////////////////////////////////////////

///// constructor /////////////////////////////////////////////////////////////
PlotMapLabel::PlotMapLabel(QWidget* parent, const char* name, WFlags fl) : QLabel(parent, name, fl),
  densityPixmap(NULL),
  drawDrag(false)
/// The default constructor.
{
}

///// destructor //////////////////////////////////////////////////////////////
PlotMapLabel::~PlotMapLabel()
/// The default destructor.
{
  if(densityPixmap)
    delete densityPixmap;
}

///// setDrawStyle ////////////////////////////////////////////////////////////
void PlotMapLabel::setDrawStyle(const unsigned int style, const bool cubic)
/// Sets the draw style. Possible values for \c style are: 
/// \arg 0 : raw values
/// \arg 1 : isolines
/// \arg 2 : polygons
/// \arg 3 : isolines and polygons
{
  if(style > 3)
    return;

  drawStyle = style;
  cubicInterpolation = cubic;
}

///// setDensityPixmap ////////////////////////////////////////////////////////
void PlotMapLabel::setDensityPixmap(const QPixmap& pixmap)
/// Sets the pixmap containing the raw density values.
{
  if(!densityPixmap)
    densityPixmap = new QPixmap(pixmap);
  else
    *densityPixmap = pixmap;
}

///// setCrosses //////////////////////////////////////////////////////////////
void PlotMapLabel::setCrosses(const vector<Point3D<double> >& centers)
/// Sets the list of positions of the atoms to be drawn as crosses.
{
  if(centers.empty())
    return;

  crosses.clear();
  crosses.reserve(centers.size());
  crosses.assign(centers.begin(), centers.end());
}

///// setGrid /////////////////////////////////////////////////////////////////
void PlotMapLabel::setGrid(const Point3D<unsigned int>& pointDimension, const Point3D<double>& pointDelta)
/// Sets the parameters for the source density
/// to be used as a reference while scaling. These include the number of gridpoints
/// in each direction and the cell size.
{
  numPoints = pointDimension;
  delta = pointDelta;
}

///// setIsoCurves ////////////////////////////////////////////////////////////
void PlotMapLabel::setIsoCurves(const vector< vector<Point3D<double> >* >& curves, const vector<QColor>& lineColors)
/// Sets the isocurves to be drawn.
{
  isoCurves.clear();
  isoCurves.reserve(curves.size());
  isoCurves.assign(curves.begin(), curves.end());

  //isoCurveColors.clear();
  //isoCurveColors.reserve(colors.size());
  //isoCurveColors.assign(colors.begin(), colors.end());

  isoLineColors.clear();
  isoLineColors.reserve(lineColors.size());
  isoLineColors.assign(lineColors.begin(), lineColors.end());
}

///// setPolygons /////////////////////////////////////////////////////////////
void PlotMapLabel::setPolygons(const vector< vector<Point3D<double> > >& polygons, const vector<QColor>& colors)
/// Sets the polygons to be drawn.
{
  isoPolygons.clear();
  isoPolygons.reserve(polygons.size());
  isoPolygons.assign(polygons.begin(), polygons.end());

  /*qDebug("the contents of isoPolygons in PlotMapLabel:");
  for(unsigned int polygon = 0; polygon < isoPolygons.size(); polygon++)
  {
    qDebug(" for polygon %d with %d points",polygon, isoPolygons[polygon].size());
    for(unsigned int i = 0; i < isoPolygons[polygon].size(); i++)
      qDebug("  point %d = (%f, %f)",i,isoPolygons[polygon][i].x(),isoPolygons[polygon][i].y()); 
  }*/

  isoPolygonColors.clear();
  isoPolygonColors.reserve(colors.size());
  isoPolygonColors.assign(colors.begin(), colors.end());
}

///// setColors ///////////////////////////////////////////////////////////////
void PlotMapLabel::setColors(const QColor& zero, const QColor& atom)
/// Sets the primary drawing colors.
{
  zeroColor = zero;
  crossColor = atom;
}

///// startDrag /////////////////////////////////////////////////////////////////
void PlotMapLabel::startDrag(const QPoint& pos)
/// Sets the starting position for a drag.
{
  ///// do not start if out of bounds
  if(pos.x() < 0 || pos.y() < 0 || pos.x() > width() || pos.y() > height())
    return;

  startPosition = pos;
  endPosition = pos;
  drawDrag = true;
}

///// moveDrag /////////////////////////////////////////////////////////////////
void PlotMapLabel::moveDrag(const QPoint& pos)
/// Sets the ending position for a drag.
{
  if(!drawDrag)
    return;

  endPosition = pos;
  update();
}

///// endDrag /////////////////////////////////////////////////////////////////
void PlotMapLabel::endDrag()
/// Signals dragging has ended .
{
  drawDrag = false;
  update();
}

///////////////////////////////////////////////////////////////////////////////
///// Protected Member Functions                                          /////
///////////////////////////////////////////////////////////////////////////////

///// drawContents ////////////////////////////////////////////////////////////
void PlotMapLabel::drawContents(QPainter* )
/// Reimplemented from QLabel::drawContents.
/// Does the actual drawing of the density points, isolines, polygons, crosses
/// and selection box. 
{  
  ///// frequently used values
  const QRect drawRect = contentsRect();
  ///// draws using the complete frame
  //const double scaleX = static_cast<double>(drawRect.width()) / numPoints.x();
  //const double scaleY = static_cast<double>(drawRect.height()) / numPoints.y();
  ///// draws while keeping the aspect ratio constant
  const double aspectRatio = static_cast<double>(numPoints.x()) / numPoints.y();
  double scaleX, scaleY;
  if(aspectRatio > static_cast<double>(drawRect.width()) / drawRect.height())
  {
    // the drawing will be done in the top part of the frame
    scaleX = static_cast<double>(drawRect.width()) / numPoints.x(); // scale to the width of the frame
    scaleY = static_cast<double>(drawRect.width()) / aspectRatio / numPoints.y(); // scale according to the aspect ratio
  }
  else
  {
    // the drawing will be done in the left part of the frame
    scaleX = static_cast<double>(drawRect.height()) * aspectRatio / numPoints.x(); // scale to the aspect ratio
    scaleY = static_cast<double>(drawRect.height()) / numPoints.y(); // scale according to the heigth of the frame
  }
  const unsigned int maxPoints = static_cast<unsigned int>(scaleX + scaleY); // the number of points to interpolate between to fixed points

  ///// draw everything to a local pixmap and then bitblt it to the QPainter
  QPixmap pm(drawRect.width(), drawRect.height());
  QPainter paint;
  paint.begin(&pm);

  ///// draw the density pixmap permitting scaling smaller than the size of the grid
  // always fill the widget with the zeroColor
  paint.fillRect(drawRect, QBrush(zeroColor));
  if(drawStyle == 0 && densityPixmap)
  {
    QRect pixmapRect(drawRect);
    // define the rectangle to draw the pixmap in
    if(aspectRatio > static_cast<double>(drawRect.width()) / drawRect.height())
      pixmapRect.setHeight(drawRect.width() / aspectRatio);
    else
      pixmapRect.setWidth(drawRect.height() * aspectRatio);
    paint.drawPixmap(pixmapRect, *densityPixmap); // autoscaling
  }
  
  ///// draw the grid
  if(false)
  {
    paint.setPen(QColor(0, 255, 0));
    for(unsigned int y = 0; y < numPoints.y(); y++)
    {
      for(unsigned int x = 0; x < numPoints.x(); x++)
        paint.drawPoint(static_cast<int>((x+0.5)*scaleX), static_cast<int>((y+0.5)*scaleY));
    }
  }

  if(drawStyle > 1)
  {
    ///// draw polygons
    for(unsigned int polygon = 0; polygon < isoPolygons.size(); polygon++)
    {
      paint.setBrush(isoPolygonColors[polygon]);
      paint.setPen(Qt::NoPen);
      paint.drawPolygon(pointCurve(isoPolygons[polygon], cubicInterpolation, scaleX, scaleY, maxPoints));
      /*
      QPointArray pa(isoPolygons[polygon].size() - 1);
      for(unsigned int i = 0; i < isoPolygons[polygon].size() - 1; i++)
      {
        if(!cubicInterpolation)
        {
          int x = static_cast<int>((isoPolygons[polygon][i].x()+0.5) * scaleX);
          int y = static_cast<int>((isoPolygons[polygon][i].y()+0.5) * scaleY);
          pa.setPoint(i, QPoint(x, y));
        }
        else
        {

        }
      }
      paint.drawPolygon(pa);
      */
    }
  }

  if(drawStyle == 1 || drawStyle == 3)
  {
    ///// draw isolines  
    if(scaleX <= 1.0 && scaleY <= 1.0)
    {
      // no interpolation needed if the scale is smaller than 1
      for(unsigned int curve = 0; curve < isoCurves.size(); curve++)
      {
        paint.setPen(QPen(isoLineColors[curve]));
        for(unsigned int i = 0; i < isoCurves[curve]->size() - 1; i++) // never draw the last point as it is either identical to 
        {                                                               // the first or the previous point
          int x = static_cast<int>((isoCurves[curve]->operator[](i).x()+0.5) * scaleX);
          int y = static_cast<int>((isoCurves[curve]->operator[](i).y()+0.5) * scaleY);
          paint.drawPoint(x, y);
        }
      }
    }
    else
    {
      for(unsigned int curve = 0; curve < isoCurves.size(); curve++)
      {
        paint.setPen(QPen(isoLineColors[curve]));
        paint.drawPolyline(pointCurve(*isoCurves[curve], cubicInterpolation, scaleX, scaleY, maxPoints));
      }
    }
    /*
    else if(!cubicInterpolation)
    {
      // use linear interpolation
      for(unsigned int curve = 0; curve < isoCurves.size(); curve++)
      {
        paint.setPen(QPen(isoLineColors[curve], 1));
        if(isoCurves[curve]->size() > 1)
        {
          int oldx = static_cast<int>((isoCurves[curve]->operator[](0).x()+0.5) * scaleX);
          int oldy = static_cast<int>((isoCurves[curve]->operator[](0).y()+0.5) * scaleY);
          for(unsigned int i = 1; i < isoCurves[curve]->size(); i++)
          {
            int newx = static_cast<int>((isoCurves[curve]->operator[](i).x()+0.5) * scaleX);
            int newy = static_cast<int>((isoCurves[curve]->operator[](i).y()+0.5) * scaleY);
            paint.drawLine(oldx, oldy, newx, newy);
            oldx = newx;
            oldy = newy;
          }
        }
      }
    }
    else
    {
      // use cubic interpolation
      for(unsigned int curve = 0; curve < isoCurves.size(); curve++)
      {
        if(isoCurves[curve]->size() > 3)
        {
          paint.setPen(isoLineColors[curve]);
          for(unsigned int i = 1; i < isoCurves[curve]->size() - 2; i++)
          {
            //qDebug("drawing spline between (%f, %f) and (%f, %f)",isoCurves[curve]->operator[](i).x(),isoCurves[curve]->operator[](i).y(),
            //                                                      isoCurves[curve]->operator[](i+1).x(),isoCurves[curve]->operator[](i+1).y());
            Point3D<double> controlPoint1((isoCurves[curve]->operator[](i-1).x()+0.5) * scaleX, (isoCurves[curve]->operator[](i-1).y()+0.5) * scaleY, 0.0);
            Point3D<double> startPoint((isoCurves[curve]->operator[](i).x()+0.5) * scaleX, (isoCurves[curve]->operator[](i).y()+0.5) * scaleY, 0.0);
            Point3D<double> endPoint((isoCurves[curve]->operator[](i+1).x()+0.5) * scaleX, (isoCurves[curve]->operator[](i+1).y()+0.5) * scaleY, 0.0);
            Point3D<double> controlPoint2((isoCurves[curve]->operator[](i+2).x()+0.5) * scaleX, (isoCurves[curve]->operator[](i+2).y()+0.5) * scaleY, 0.0);
            paint.drawPoints(cardinalSpline(controlPoint1, startPoint, endPoint, controlPoint2, maxPoints));
          }
          // if the last point of the curve is identical to the first point, the curve 
          // is circular and two extra segments should be drawn
          if(isoCurves[curve]->operator[](0).id() == isoCurves[curve]->operator[](isoCurves[curve]->size() - 1).id())
          {
            const unsigned int lastIndex = isoCurves[curve]->size() - 1;
            // between last and first points
            //qDebug("drawing spline between (%f, %f) and (%f, %f)",isoCurves[curve]->operator[](lastIndex-1).x(),isoCurves[curve]->operator[](lastIndex-1).y(),
            //                                                      isoCurves[curve]->operator[](0).x(),isoCurves[curve]->operator[](0).y());
            Point3D<double> controlPoint1((isoCurves[curve]->operator[](lastIndex - 2).x()+0.5) * scaleX, (isoCurves[curve]->operator[](lastIndex - 2).y()+0.5) * scaleY, 0.0);
            Point3D<double> startPoint((isoCurves[curve]->operator[](lastIndex - 1).x()+0.5) * scaleX, (isoCurves[curve]->operator[](lastIndex - 1).y()+0.5) * scaleY, 0.0);
            Point3D<double> endPoint((isoCurves[curve]->operator[](0).x()+0.5) * scaleX, (isoCurves[curve]->operator[](0).y()+0.5) * scaleY, 0.0);
            Point3D<double> controlPoint2((isoCurves[curve]->operator[](1).x()+0.5) * scaleX, (isoCurves[curve]->operator[](1).y()+0.5) * scaleY, 0.0);
            paint.drawPoints(cardinalSpline(controlPoint1, startPoint, endPoint, controlPoint2, maxPoints));
            // between first and second points
            //qDebug("drawing spline between (%f, %f) and (%f, %f)",isoCurves[curve]->operator[](0).x(),isoCurves[curve]->operator[](0).y(),
            //                                                      isoCurves[curve]->operator[](1).x(),isoCurves[curve]->operator[](1).y());
            controlPoint1 = startPoint;
            startPoint = endPoint;
            endPoint = controlPoint2;
            controlPoint2.setValues((isoCurves[curve]->operator[](2).x()+0.5) * scaleX, (isoCurves[curve]->operator[](2).y()+0.5) * scaleY, 0.0);
            paint.drawPoints(cardinalSpline(controlPoint1, startPoint, endPoint, controlPoint2, maxPoints));
          }
        }
      }
    }
    */
  }

  ///// draw the crosses
  if(!crosses.empty())
  {
    // scale from the grid size to the current window size
    // scale to the complete frame
    //double scaleX = 1.0 / delta.x() / numPoints.x() * drawRect.width();
    //double scaleY = 1.0 / delta.y() / numPoints.y() * drawRect.height();
    // scale keeping the aspect ratio => WARNING: scaleX and scaleY are changed here !!!
    scaleX *= 1.0 / delta.x();
    scaleY *= 1.0 / delta.y();
    paint.setPen(crossColor);
    for(unsigned int i = 0; i < crosses.size(); i++)
    {
      ///// paint the atomic positions (already referenced to the origin)
      int centerX = static_cast<int>(crosses[i].x()*scaleX);
      //int centerY = contentsRect().height() - static_cast<int>(crosses[i].y()*scaleY);
      int centerY = static_cast<int>((numPoints.y()*delta.y() - crosses[i].y())*scaleY);
       paint.drawLine(centerX - 5, centerY, centerX + 5, centerY);
      paint.drawLine(centerX, centerY - 5, centerX, centerY + 5);
    }
  }

  ///// draw the dragging rectangle in XOR mode (always the last item to draw)
  if(drawDrag)
  {
    //qDebug("drawing a rect from (%d, %d) to (%d, %d)", startPosition.x(),startPosition.y(),endPosition.x(),endPosition.y()); 
    //paint.setRasterOp(Qt::XorROP);
    paint.setBrush(Qt::NoBrush);
    paint.setPen(QPen(crossColor, 0, Qt::DashLine));
    paint.drawRect(QRect(startPosition, endPosition));
    //paint.setRasterOp(Qt::CopyROP);
  }

  ///// end of painting to the pixmap
  paint.end();

  ///// do the bitblt to the screen
  bitBlt(this, 0, 0, &pm);
}

///////////////////////////////////////////////////////////////////////////////
///// Private Member Functions                                            /////
///////////////////////////////////////////////////////////////////////////////

///// pointCurve //////////////////////////////////////////////////////////////
QPointArray PlotMapLabel::pointCurve(const vector<Point3D<double> >& curve, const bool cubicInterpolation, const double scaleX, const double scaleY, const unsigned int numInterpolated)
/// Calculates the points of the specified isodensity curve.
/// Curves can either be linearly or cubically interpolated. 
{
  QPointArray result;

  if(cubicInterpolation && curve.size() > 3)
  {
    // Cubic interpolation needs at least 4 points 
    for(unsigned int i = 1; i < curve.size() - 2; i++)
    {
      Point3D<double> controlPoint1((curve[i-1].x()+0.5) * scaleX, (curve[i-1].y()+0.5) * scaleY, 0.0);
      Point3D<double> startPoint(   (curve[i].x()+0.5)   * scaleX, (curve[i].y()+0.5)   * scaleY, 0.0);
      Point3D<double> endPoint(     (curve[i+1].x()+0.5) * scaleX, (curve[i+1].y()+0.5) * scaleY, 0.0);
      Point3D<double> controlPoint2((curve[i+2].x()+0.5) * scaleX, (curve[i+2].y()+0.5) * scaleY, 0.0);
      if(curve[i-1].x() < Point3D<double>::TOLERANCE && curve[i-1].y() < Point3D<double>::TOLERANCE ||
         curve[i].x()   < Point3D<double>::TOLERANCE && curve[i].y()   < Point3D<double>::TOLERANCE ||
         curve[i+1].x() < Point3D<double>::TOLERANCE && curve[i+1].y() < Point3D<double>::TOLERANCE ||
         curve[i+2].x() < Point3D<double>::TOLERANCE && curve[i+2].y() < Point3D<double>::TOLERANCE)
        // never interpolate lines that go through (0,0) as they control Areas
        result.putPoints(result.size(), 1, static_cast<int>(startPoint.x()), static_cast<int>(startPoint.y()),
                                           static_cast<int>(endPoint.x()), static_cast<int>(endPoint.y())); // might result in some double additions 
      else
        result.putPoints(result.size(), numInterpolated, cardinalSpline(controlPoint1, startPoint, endPoint, controlPoint2, numInterpolated));
    }
    // if the last point of the curve is identical to the first point, the curve 
    // is circular and two extra segments should be drawn
    const unsigned int lastIndex = curve.size() - 1;
    if(curve[0].id() == curve[lastIndex].id())
    {
      // between last and first points
      Point3D<double> controlPoint1((curve[lastIndex - 2].x()+0.5) * scaleX, (curve[lastIndex - 2].y()+0.5) * scaleY, 0.0);
      Point3D<double> startPoint(   (curve[lastIndex - 1].x()+0.5) * scaleX, (curve[lastIndex - 1].y()+0.5) * scaleY, 0.0);
      Point3D<double> endPoint(     (curve[0].x()+0.5)             * scaleX, (curve[0].y()+0.5)             * scaleY, 0.0);
      Point3D<double> controlPoint2((curve[1].x()+0.5)             * scaleX, (curve[1].y()+0.5)             * scaleY, 0.0);
      if(curve[lastIndex - 2].x()   < Point3D<double>::TOLERANCE && curve[lastIndex - 2].y()   < Point3D<double>::TOLERANCE ||
         curve[lastIndex - 1].x()   < Point3D<double>::TOLERANCE && curve[lastIndex - 1].y()   < Point3D<double>::TOLERANCE ||
         curve[0].x() < Point3D<double>::TOLERANCE && curve[0].y() < Point3D<double>::TOLERANCE ||
         curve[1].x() < Point3D<double>::TOLERANCE && curve[1].y() < Point3D<double>::TOLERANCE)
        // never interpolate lines that go through (0,0) as they control Areas
        result.putPoints(result.size(), 1, static_cast<int>(startPoint.x()), static_cast<int>(startPoint.y()),
                                           static_cast<int>(endPoint.x()), static_cast<int>(endPoint.y())); // might result in some double additions 
      else
        result.putPoints(result.size(), numInterpolated, cardinalSpline(controlPoint1, startPoint, endPoint, controlPoint2, numInterpolated));
      // between first and second points
      controlPoint1 = startPoint;
      startPoint = endPoint;
      endPoint = controlPoint2;
      controlPoint2.setValues((curve[2].x()+0.5) * scaleX, (curve[2].y()+0.5) * scaleY, 0.0);
      if(curve[lastIndex - 1].x()   < Point3D<double>::TOLERANCE && curve[lastIndex - 1].y()   < Point3D<double>::TOLERANCE ||
         curve[0].x() < Point3D<double>::TOLERANCE && curve[0].y() < Point3D<double>::TOLERANCE ||
         curve[1].x() < Point3D<double>::TOLERANCE && curve[1].y() < Point3D<double>::TOLERANCE ||
         curve[2].x() < Point3D<double>::TOLERANCE && curve[2].y() < Point3D<double>::TOLERANCE)
        // never interpolate lines that go through (0,0) as they control Areas
        result.putPoints(result.size(), 1, static_cast<int>(startPoint.x()), static_cast<int>(startPoint.y()),
                                           static_cast<int>(endPoint.x()), static_cast<int>(endPoint.y())); // might result in some double additions 
      else
        result.putPoints(result.size(), numInterpolated, cardinalSpline(controlPoint1, startPoint, endPoint, controlPoint2, numInterpolated));
    }
  }
  else if(curve.size() > 1)
  {
    // Linear interpolation
    result.resize(curve.size());
    for(unsigned int i = 0; i < curve.size(); i++)
      result.setPoint(i, static_cast<int>((curve[i].x()+0.5) * scaleX), static_cast<int>((curve[i].y()+0.5) * scaleY));
  }
  return result;
}

///// cardinalSpline //////////////////////////////////////////////////////////
QPointArray PlotMapLabel::cardinalSpline(const Point3D<double>& controlPoint1, const Point3D<double>& startPoint, const Point3D<double>& endPoint, const Point3D<double>& controlPoint2, const unsigned int numInterpolated)
/// Calculates the points for a cardinal spline 
/// defined by the 4 given points (2D only). The hardcoded constant \c t can be 
/// changed to use different types of Catmull-Rom splines.
{
  const double t = 0.0; // defines the type of the Catmull-Rom Spline => 0.0 is a Cardinal Spline
                        //                                            => 1.0 is a polyline
  const double s = (1.0 - t)/2.0; 

  //int oldx = 100000;
  //int oldy = 100000;
  QPointArray result(numInterpolated);
  //unsigned int numPoints = 0;
  for(unsigned int i = 0; i < numInterpolated; i++)
  {
    const double value = static_cast<double>(i)/numInterpolated;
    const double value2 = value*value;
    const double value3 = value2*value;
    const int x = static_cast<int>(   (    -s*controlPoint1.x() + (2.0 - s)*startPoint.x() +     (s - 2.0)*endPoint.x() + s*controlPoint2.x() ) * value3
                                    + ( 2.0*s*controlPoint1.x() + (s - 3.0)*startPoint.x() + (3.0 - 2.0*s)*endPoint.x() - s*controlPoint2.x() ) * value2
                                    + (    -s*controlPoint1.x()                            +             s*endPoint.x()                       ) * value
                                                                +           startPoint.x()  
                                  );
    const int y = static_cast<int>(   (    -s*controlPoint1.y() + (2.0 - s)*startPoint.y() +     (s - 2.0)*endPoint.y() + s*controlPoint2.y() ) * value3
                                    + ( 2.0*s*controlPoint1.y() + (s - 3.0)*startPoint.y() + (3.0 - 2.0*s)*endPoint.y() - s*controlPoint2.y() ) * value2
                                    + (    -s*controlPoint1.y()                            +             s*endPoint.y()                       ) * value
                                                                +           startPoint.y()  
                                  );
    result.setPoint(i, x, y);
    //if(x != oldx && y!= oldy)
    //{
    //  result.resize(numPoints+1, QGArray::SpeedOptim);
    //  result.setPoint(numPoints++,QPoint(x,y));
    //  oldx = x;
    //  oldy = y;
    //}
  }
  return result;
}

