/***************************************************************************
                        plotmapbase.cpp  -  description
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

///// Comments ////////////////////////////////////////////////////////////////
/*!
  \class PlotMapBase
  \brief This class shows 2D density maps from files created with potdicht.

  It's a replacement for spfmap with added analysis possibilities. This class 
  provides the functionality for PlotMapWidget. The actual rendering is done in
  PlotMapLabel. The optionwidgets are provided by PlotMapExtensionWidget.

*/
/// \file
/// Contains the implementation of the class PlotMapBase

///// Header files ////////////////////////////////////////////////////////////

// STL header files
#include <map>

// C++ header files
#include <cmath>

// Qt header files
#include <qapplication.h>
#include <qcheckbox.h>
#include <qcolor.h>
#include <qcombobox.h>
#include <qcursor.h>
#include <qfile.h>
#include <qfiledialog.h>
#include <qfileinfo.h>
#include <qimage.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qmessagebox.h>
#include <qnamespace.h>
#include <qpainter.h>
#include <qpoint.h>
#include <qprogressdialog.h>
#include <qpushbutton.h>
#include <qspinbox.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qtextstream.h>

// Xbrabo header files
#include "colorbutton.h"
#include "plotmapbase.h"
#include "plotmapextensionwidget.h"
#include "plotmaplabel.h"
#include "version.h"

///////////////////////////////////////////////////////////////////////////////
///// Public Member Functions                                             /////
///////////////////////////////////////////////////////////////////////////////

///// constructor /////////////////////////////////////////////////////////////
PlotMapBase::PlotMapBase(QWidget* parent, const char* name, bool modal, WFlags fl) : PlotMapWidget(parent, name, modal, fl),
  maxValue(1.0),
  minValue(-1.0),
  loadInProgress(false)
/// The default constructor.
{
  options = new PlotMapExtensionWidget(this);
  options->hide();
  PlotMapWidgetLayout->addWidget(options);
  ///// keep this widget as small as possible
  options->setFixedHeight(options->height());
  options->setMaximumHeight(options->height());
  
  ///// initialize a new PlotMapLabel
  plotLabel = new PlotMapLabel(this, 0, Qt::WNoAutoErase);
  plotLabel->setBackgroundMode(Qt::NoBackground);
  plotLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  plotLabel->setFrameShape(QFrame::StyledPanel);
  plotLabel->setFrameShadow(QFrame::Sunken); 
  plotLabel->setLineWidth(1);
  plotLabel->setMouseTracking(true);
  PixmapLayout->addWidget(plotLabel);

  ///// further initialisation
  makeConnections();
  init();
}

///// destructor //////////////////////////////////////////////////////////////
PlotMapBase::~PlotMapBase()
/// The default destructor.
{

}

///// loadMapFile /////////////////////////////////////////////////////////////
bool PlotMapBase::loadMapFile(const QString filename, const bool noerrors)
/// Tries to load a map file with the specified name.
/// If noerrors = true, no error-messages will be shown.
{
  ///// open the file
  QFile mapFile(filename);
  if(!mapFile.exists())
  {
    if(!noerrors)
      QMessageBox::warning(this, "Loading map file", "The map file does not exist");
    return false;
  }
  if(!mapFile.open(IO_ReadOnly))
  {
    if(!noerrors)
      QMessageBox::warning(this, "Loading map file", "The map file could not ne opened");
    return false;
  }

  ///// busy-wait, but show progress
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  loadInProgress = true;
  QTextStream stream(&mapFile);

  // read the sizes
  stream.readLine(); // ignore the first line
  QString line = stream.readLine();
  numPoints.setValues(line.mid(0, 5).toUInt(), line.mid(75, 5).toUInt(), 0);
  origin.setValues(line.mid(5,10).toDouble() * AUTOANG, line.mid(65,10).toDouble() * AUTOANG, 0.0);
  delta.setValues(line.mid(15,10).toDouble() * AUTOANG, line.mid(55,10).toDouble() * AUTOANG, 0.0);
  QProgressDialog progress(tr("Loading the data..."), tr("Cancel"), numPoints.y() + numPoints.y()/20, this, 0, true);
  if(isVisible())
    progress.setCancelButton(0); // no cancelling when rereading
      
  // read the points
  points.clear();
  for(unsigned int j = 0; j < numPoints.y(); j++) // y's
  {
    progress.setProgress(j);
    qApp->processEvents();
    if(progress.wasCancelled())
    {
      points.clear();
      coords.clear();
      numPoints.setValues(0, 0, 0);
      numAtoms = 0;
      loadInProgress = false;
     QApplication::restoreOverrideCursor();  
     return false;
    }
    double tempvar;
    vector<double> tempvector;
    for(unsigned int i = 0; i < numPoints.x(); i++) // x's
    {
      stream >> tempvar;
      tempvector.push_back(tempvar);
    }
    points.push_back(tempvector);
  }
  ///// points[y][x] !!! /////

  loadInProgress = false;
  if(stream.atEnd())
  {
    QApplication::restoreOverrideCursor(); 
    QMessageBox::warning(this, tr("Loading map file"), tr("Premature end of file encountered"));
    return false; // premature end of file
  }

  // read the coordinates
  stream >> numAtoms;
  coords.clear();
  coords.reserve(numAtoms);
  Point3D<double> atom;
  for(unsigned int i = 0; i < numAtoms; i ++)
  {
    line = stream.readLine();
    atom.setValues(line.left(10).toDouble() * AUTOANG, line.mid(10,10).toDouble() * AUTOANG, line.mid(20,10).toDouble() * AUTOANG);
    coords.push_back(atom);
  }
  
  mapFile.close();
  plotLabel->setGrid(numPoints, delta);
  resize(width(), numPoints.y() + 6); // approximately scale plotLabel to its native size
  //plotLabel->resize(numPoints.x(), numPoints.y()); // has no effect

  ///// calculate the maximum and minimum values
  maxValue = 0.0;
  minValue = 0.0;
  for(unsigned int j = 0; j < numPoints.y(); j++)
  {
    progress.setProgress(numPoints.y() + j/20);
    for(unsigned int i = 0; i < numPoints.x(); i++)
    {
      if(points[j][i] > maxValue)
        maxValue = points[j][i];
      else if(points[j][i] < minValue)
        minValue = points[j][i];
    }
  }
 
  progress.setProgress(progress.totalSteps());
  
  ///// update the caption
  setCaption(Version::appName + " - " + tr("Visualizing density map") + " (" + mapFile.name() + ")");

  ///// update the statistics
  //TextLabelFilename->setText(filename);
  TextLabelGridSizeX->setText(QString::number(numPoints.x()));  
  TextLabelGridSizeY->setText(QString::number(numPoints.y()));
  TextLabelMaxPos->setText(QString::number(maxValue));
  TextLabelMaxNeg->setText(QString::number(minValue));

  ///// update the options
  options->LineEditMaxPos->setText(QString::number(maxValue));
  options->LineEditMaxNeg->setText(QString::number(minValue));
  
  ///// fix the width of the left column
  TextLabelCurrentCoordinate->setText("(" + QString::number(numPoints.x()) + ", " + QString::number(numPoints.y()) + ")"); // largest possible width
  TextLabelCurrentCoordinate->setFixedWidth(TextLabelCurrentCoordinate->width());
  
  ///// update the current grid point
  TextLabelCurrentCoordinate->setText("none");
  TextLabelCurrentValue->setText("none");
  TextLabelCurrentCartesian->setText("none");
  
  ///// resize the whole thing so the label is at the size of the grid
  resize(width() - plotLabel->contentsRect().width() + numPoints.x(), height() - plotLabel->contentsRect().height() + numPoints.y());

  ///// show the image
  updateImage();

  QApplication::restoreOverrideCursor();  
  return true;
}


///////////////////////////////////////////////////////////////////////////////
///// Public Slots                                                        /////
///////////////////////////////////////////////////////////////////////////////

///// loadMapFile (overloaded) ////////////////////////////////////////////////
bool PlotMapBase::loadMapFile()
/// Tries to load a map file. 
{
  QString filename = QFileDialog::getOpenFileName(QString::null, "*.map.1", this, 0, tr("Choose a map file to open"));

  if(filename.isEmpty())
    return false;  

  return loadMapFile(filename);
}


///////////////////////////////////////////////////////////////////////////////
///// Protected Member Functions                                          /////
///////////////////////////////////////////////////////////////////////////////

///// mousePressEvent /////////////////////////////////////////////////////////
void PlotMapBase::mousePressEvent(QMouseEvent* e)
/// Overridden from PlotMapWidget::mousePressEvent. Initiates mousedrags.
{
  mousePosition = e->pos();
  if(onPixmap(mousePosition))
    plotLabel->startDrag(mousePosition - plotLabel->pos());
}

///// mouseReleaseEvent ///////////////////////////////////////////////////////
void PlotMapBase::mouseReleaseEvent(QMouseEvent* e)
/// Overridden from PlotMapWidget::mouseReleaseEvent. Handles mousedrags and
/// shows statistics for the selected box.
{
  QPoint mouseEndPosition = e->pos();

  if(onPixmap(mousePosition) && onPixmap(mouseEndPosition))
  {
    QPoint point1 = mapToImage(mousePosition);
    QPoint point2 = mapToImage(mouseEndPosition);
    int minX = point1.x() > point2.x() ? point2.x() : point1.x();
    int maxX = point1.x() > point2.x() ? point1.x() : point2.x();
    int minY = point1.y() > point2.y() ? point2.y() : point1.y();
    int maxY = point1.y() > point2.y() ? point1.y() : point2.y();
     
    ///// calculate some values
    double localMaxPosValue = 0.0;
    double localMinPosValue = maxValue;
    double localMaxNegValue = 0.0;
    double localMinNegValue = minValue;
    for(int j = minY; j <= maxY; j++)
    {
      for(int i = minX; i <= maxX; i++)
      {
        double value = points[j][i];
        if(value > 0.0)
        {
          if(value > localMaxPosValue)
            localMaxPosValue = value;
          if(value < localMinPosValue)
            localMinPosValue = value;
        }
        else if(value < 0.0)
        {
          if(value < localMaxNegValue)
            localMaxNegValue = value;
          if(value > localMinNegValue)
            localMinNegValue = value;
        }
        else
          localMinPosValue = localMinNegValue = 0.0; 
      }
    }  
    QString value1, value2, value3, value4;
    value1.setNum(localMaxPosValue,'f',5);
    value2.setNum(localMinPosValue,'f',5);
    value3.setNum(localMaxNegValue,'f',5);
    value4.setNum(localMinNegValue,'f',5);
    if(localMaxPosValue == localMaxNegValue)
    {
      ///// all values are zero
      value2 = "0.00000";
      value4 = "0.00000";
    }
    else if((localMaxPosValue == 0.0) && (localMinNegValue != 0.0))
    {
      ///// no positive values found
      value1 = tr("none");
      value2 = tr("none");
    }  
    else if((localMaxNegValue == 0.0) && (localMinPosValue != 0.0))
    {
      ///// no negative values found
      value3 = tr("none");
      value4 = tr("none");
    }  
      
    ///// show the values
    QString stats = tr("The selected area has the following statistics:")+"\n";
    stats += tr("Maximum positive value: ") + value1 + "\n";
    stats += tr("Minimum positive value: ") + value2 + "\n";
    stats += tr("Maximum negative value: ") + value3 + "\n";
    stats += tr("Minimum negative value: ") + value4;
    QMessageBox::information(this, tr("Statistics from selection"), stats);

  }
  plotLabel->endDrag();
  mousePosition.setX(0);
  mousePosition.setY(0);
  mouseEndPosition.setX(0);
  mouseEndPosition.setY(0);
}


///// mouseMoveEvent //////////////////////////////////////////////////////////
void PlotMapBase::mouseMoveEvent(QMouseEvent* e)
/// Overridden from PlotMapWidget::mouseMoveEvent. Handles mousedrags in progress
/// and updates statistics.
{
  if(loadInProgress)
    return; // this routine reads from the points array but that one is being updated in loadMapFile (re-entry because of qapp->processEvents)

  ///// send drag messages to plotLabel
  if(mousePosition.x() && mousePosition.y())
    plotLabel->moveDrag(e->pos() - plotLabel->pos());

  ///// calculate the position on the pixmaplabel
  if(onPixmap(e->pos()))
  {
    ///// the grid point
    QPoint position = mapToImage(e->pos());
    TextLabelCurrentCoordinate->setText("(" + QString::number(position.x() + 1) + ", " + QString::number(position.y() + 1) + ")");

    ///// the value
    QString value0;
    value0.setNum(points[position.y()][position.x()],'f', 5);
    TextLabelCurrentValue->setText(value0);
    ///// the corresponding cartesian coordinate
    position = e->pos() - plotLabel->pos();
    /*
    int fwidth = plotLabel->frameWidth();
    QRect rect = plotLabel->geometry();
    rect.setLeft(rect.left() + fwidth);
    rect.setRight(rect.right() - fwidth);
    rect.setTop(rect.top() + fwidth);
    rect.setBottom(rect.bottom() - fwidth);
    // rect = plotLabel->contentsRect() ???
    double cx = origin.x() + delta.x()*double(numPoints.x())*double(position.x() - fwidth)/double(rect.width());
    double cy = origin.y() + delta.y()*double(numPoints.y())*(1.0 - double(position.y() - fwidth)/double(rect.height()));
    */
    QRect rect = plotLabel->contentsRect(); // if only the size is needed, contentsRect is always correct
    double cx = origin.x() + delta.x()*double(numPoints.x())*double(position.x())/double(rect.width());
    double cy = origin.y() + delta.y()*double(numPoints.y())*(1.0 - double(position.y())/double(rect.height()));

    QString value1, value2;
    value1.setNum(cx,'f',4);
    value2.setNum(cy,'f',4);
    TextLabelCurrentCartesian->setText("(" + value1 + ", " + value2 + ")");
  }
  else
  {
    TextLabelCurrentCoordinate->setText("none");
    TextLabelCurrentValue->setText("none");
    TextLabelCurrentCartesian->setText("none");
  }
}

///////////////////////////////////////////////////////////////////////////////
///// Private Slots                                                       /////
///////////////////////////////////////////////////////////////////////////////

///// saveImage /////////////////////////////////////////////////////////////
void PlotMapBase::saveImage()
/// Saves the image to a file.
{
  ///// get the available output formats  
  QStringList saveFormats = QImage::outputFormatList();
  QStringList::Iterator it = saveFormats.begin();
  while(it != saveFormats.end())
  {
    if((*it) == "JPEG")
      *it = "JPEG (*.jpg)";
    else  
      *it += " (*."+(*it).lower()+")";  // 'BMP' -> 'BMP (*.bmp)'
    //qDebug( *it );
    it++;
  }

  ///// set up and show the dialog
  QFileDialog saveDialog("", QString::null, this, 0, true);  
  saveDialog.setFilters(saveFormats);
  saveDialog.setSelectedFilter("PNG (*.png)");
  saveDialog.setCaption(tr("Choose a filename and format"));
  saveDialog.setMode(QFileDialog::AnyFile);
  if(saveDialog.exec() != QDialog::Accepted)
    return;
  
  ///// get the filename and the desired extension/filter
  QString filename = saveDialog.selectedFile();
  if(filename.isEmpty())
    return;

  QString extension = saveDialog.selectedFilter();
  extension = extension.mid(extension.find("."));
  extension = extension.remove(")");
  //qDebug("extension: "+extension);

  if(filename.contains(extension) == 0)
    filename += extension;

  QString format = saveDialog.selectedFilter();
  format = format.left(format.find(" "));
  
  QPixmap pm = QPixmap::grabWidget(plotLabel);
  QImage image = pm.convertToImage();
  if(!image.save(filename, format))
    QMessageBox::warning(this, tr("Save image"), tr("An error occured. Image is not saved")); 
}

///// showOptions /////////////////////////////////////////////////////////////
void PlotMapBase::showOptions()
/// Shows the options widget.
{
  if(options->isVisible())
  {
    int newHeight = plotLabel->height() + 2*plotLabel->pos().y();
    QSize oldsize(width(), newHeight); // keep width the same
    options->hide();
    qApp->processEvents();
    resize(oldsize);
    PushButtonOptions->setText(tr(">>> Show Options"));
  }
  else
  {
    int newHeight = plotLabel->height() + 2*plotLabel->pos().y();
    options->show();
    newHeight += options->height() + 6; // WHY 6 ?????
    resize(width(), newHeight); // keep new width the same
    PushButtonOptions->setText(tr("<<< Hide Options"));
   }
}

///// applyOptions ////////////////////////////////////////////////////////////
void PlotMapBase::applyOptions()
/// Applies the options to the image.
{
  updateImage();
}

///// autoApply ///////////////////////////////////////////////////////////////
void PlotMapBase::autoApply(bool state)
/// Turns automatic application of options on/off.
{
  if(state)
  {
    connect(this, SIGNAL(optionsChanged()), this, SLOT(applyOptions()));
    applyOptions();
  }  
  else
    disconnect(this, SIGNAL(optionsChanged()), this, SLOT(applyOptions()));
}

///// resetOptions ////////////////////////////////////////////////////////////
void PlotMapBase::resetOptions()
/// Resets the options to their defaults.
{
  options->LineEditMaxPos->setText(QString::number(maxValue));
  options->ColorButtonPos->setColor(QColor(0, 0, 255));
  options->SpinBoxPos->setValue(10);

  options->LineEditMaxNeg->setText(QString::number(minValue));
  options->ColorButtonNeg->setColor(QColor(255, 0, 0));
  options->SpinBoxNeg->setValue(10);
  
  options->CheckBoxAtoms->setChecked(true);
  options->LineEditAtoms->setText("0.1");
  options->ColorButtonAtoms->setColor(QColor(0, 0, 0));

  options->ColorButtonZero->setColor(QColor(255, 255, 255));

  options->ComboBoxStyle->setCurrentItem(2); // Areas
  options->ComboBoxInterpolation->setCurrentItem(0); // Linear interpolation

  updateImage();
}


///////////////////////////////////////////////////////////////////////////////
///// Private Member Functions                                            /////
///////////////////////////////////////////////////////////////////////////////

///// makeConnections /////////////////////////////////////////////////////////
void PlotMapBase::makeConnections()
/// Sets up all permanent connections. Called once from the constructor.
{
  ///// main buttons
  connect(PushButtonLoadMap, SIGNAL(clicked()), this, SLOT(loadMapFile()));
  connect(PushButtonSaveImage, SIGNAL(clicked()), this, SLOT(saveImage()));
  connect(PushButtonOptions, SIGNAL(clicked()), this, SLOT(showOptions()));
  connect(PushButtonClose, SIGNAL(clicked()), this, SLOT(close())); // with WDestructiveClose

  ///// options buttons
  connect(options->PushButtonApply, SIGNAL(clicked()), this, SLOT(applyOptions()));
  connect(options->CheckBoxApply, SIGNAL(toggled(bool)), this, SLOT(autoApply(bool)));
  connect(options->PushButtonDefaults, SIGNAL(clicked()), this, SLOT(resetOptions()));
  //connect(options->PushButtonClose, SIGNAL(clicked()), options, SLOT(hide()));

  ///// auto apply changes for options
  connect(options->LineEditMaxPos, SIGNAL(textChanged(const QString&)), this, SIGNAL(optionsChanged()));
  connect(options->ColorButtonPos, SIGNAL(newColor(QColor*)), this, SIGNAL(optionsChanged()));
  //connect(options->CheckBoxPos, SIGNAL(toggled(bool)), this, SIGNAL(optionsChanged()));
  connect(options->SpinBoxPos, SIGNAL(valueChanged(int)), this, SIGNAL(optionsChanged()));
  connect(options->LineEditMaxNeg, SIGNAL(textChanged(const QString&)), this, SIGNAL(optionsChanged()));
  connect(options->ColorButtonNeg, SIGNAL(newColor(QColor*)), this, SIGNAL(optionsChanged()));
  //connect(options->CheckBoxNeg, SIGNAL(toggled(bool)), this, SIGNAL(optionsChanged()));
  connect(options->SpinBoxNeg, SIGNAL(valueChanged(int)), this, SIGNAL(optionsChanged()));
  connect(options->CheckBoxAtoms, SIGNAL(toggled(bool)), this, SIGNAL(optionsChanged()));
  connect(options->LineEditAtoms, SIGNAL(textChanged(const QString&)), this, SIGNAL(optionsChanged()));
  connect(options->ColorButtonAtoms, SIGNAL(newColor(QColor*)), this, SIGNAL(optionsChanged()));
  connect(options->ColorButtonZero, SIGNAL(newColor(QColor*)), this, SIGNAL(optionsChanged()));  
}

///// init ////////////////////////////////////////////////////////////////////
void PlotMapBase::init()
/// Initializes the dialog. Called once from the constructor.
{
  mousePosition.setX(0);
  mousePosition.setY(0);
  resetOptions(); 
}

///// updateImage /////////////////////////////////////////////////////////////
void PlotMapBase::updateImage()
/// Regenerates the image depending on the options.
{
  if((numPoints.x() == 0) || (numPoints.y() == 0))
    return;

  if(options->ComboBoxStyle->currentItem() == 0)
    updatePixmap();
  else
    updateIsoLines();
  if(options->CheckBoxAtoms->isChecked())
    updateCrosses();

  plotLabel->setDrawStyle(options->ComboBoxStyle->currentItem(), options->ComboBoxInterpolation->currentItem() == 1); 
  plotLabel->setColors(options->ColorButtonZero->color(), options->ColorButtonAtoms->color());
  plotLabel->update();
}

///// updatePixmap ////////////////////////////////////////////////////////////
void PlotMapBase::updatePixmap()
/// Regenerates the pixmap from density values depending on the options.
{
  QPixmap showPixmap(numPoints.x(), numPoints.y());
  QPainter painter(&showPixmap);

  const double maxPlotValue = fabs(options->LineEditMaxPos->text().toDouble());
  const QColor positiveColor = options->ColorButtonPos->color();
  const bool useLevelsPos = options->SpinBoxPos->value() != 255;
  const int numLevelsPos = options->SpinBoxPos->value();  
  const double minPlotValue = - fabs(options->LineEditMaxNeg->text().toDouble());
  const QColor negativeColor = options->ColorButtonNeg->color();
  const bool useLevelsNeg = options->SpinBoxNeg->value() != 255;
  const int numLevelsNeg = options->SpinBoxNeg->value();    
  const QColor backgroundColor = options->ColorButtonZero->color();
      
  for(unsigned int j = 0; j < numPoints.y(); j++)
  {
    for(unsigned int i = 0; i < numPoints.x(); i++)
    {
      ///// calculate the color
      QColor pixelColor;
      if(points[j][i] > 0.0)
      {
        ///// calculate the positive intensity (max intensity = max opaqueness)
        double intensity = 1.0;
        if(!useLevelsPos)
          intensity = points[j][i]/maxPlotValue;
        else
          intensity = floor(points[j][i]*double(numLevelsPos)/maxPlotValue + 0.5)/double(numLevelsPos);
        ///// get the color
        pixelColor = plotColor(positiveColor, backgroundColor, intensity);        
      }
      else
      {
        double intensity = 1.0;
        if(!useLevelsNeg)
          intensity = points[j][i]/minPlotValue;
        else
          intensity = floor(points[j][i]*double(numLevelsNeg)/minPlotValue + 0.5)/double(numLevelsNeg);  
        pixelColor = plotColor(negativeColor, backgroundColor, intensity);        
      }
      // set the pixel to that color
      painter.setPen(pixelColor);
      painter.drawPoint(i, j);
    }
  }
  plotLabel->setDensityPixmap(showPixmap);
}

///// updateCrosses ///////////////////////////////////////////////////////////
void PlotMapBase::updateCrosses()
/// Updates which atoms are displayed as crosses.
{
  const double showAtomsLimit = options->LineEditAtoms->text().toDouble();
      
  vector<Point3D<double> > showCoords;
  showCoords.reserve(numAtoms);
  Point3D<double>newCoord;
  if(options->CheckBoxAtoms->isChecked())
  {
    for(unsigned int i = 0; i < numAtoms; i++)
    {
      if(fabs(coords[i].z()) > showAtomsLimit)
        continue; // do not show atoms too far out of plane

      // substract the origin
      newCoord.setValues(coords[i].x() - origin.x(), coords[i].y() - origin.y(), 0.0);
      showCoords.push_back(newCoord);
    }
  }
  // give them to plotLabel
  plotLabel->setCrosses(showCoords);
}

///// updateIsoLines //////////////////////////////////////////////////////////
void PlotMapBase::updateIsoLines()
/// Updates the list of points with isodensity values. These points are calculated
/// using a procedure related to the Marching Squares but without lookup tables.
/// Two sets of curves are produced: one set used for isolines (one curve for each line)
/// and one set used for polygons (one curve for each isolevel).
/// Points are ordered so the lines can be drawn with drawPolyLine or drawPolygon.
{
  const double maxPlotValue = fabs(options->LineEditMaxPos->text().toDouble());
  const QColor positiveColor = options->ColorButtonPos->color();
  const int numLevelsPos = options->SpinBoxPos->value();  
  const double minPlotValue = - fabs(options->LineEditMaxNeg->text().toDouble());
  const QColor negativeColor = options->ColorButtonNeg->color();
  const int numLevelsNeg = options->SpinBoxNeg->value();    
  const QColor backgroundColor = options->ColorButtonZero->color();

  ///// get the list of isovalues to generate and their corresponding colors
  // values
  vector<double> isoLevels;
  isoLevels.reserve(numLevelsPos+numLevelsNeg);
  for(int i = 1; i <= numLevelsPos; i++) 
    isoLevels.push_back(static_cast<double>(i)/numLevelsPos * maxPlotValue);
  for(int i = 1; i <= numLevelsNeg; i++) 
    isoLevels.push_back(static_cast<double>(i)/numLevelsNeg * minPlotValue);
  // colors
  vector<QColor> isoColors;
  isoColors.reserve(numLevelsPos+numLevelsNeg);
  for(int i = 0; i < numLevelsPos; i++)
    isoColors.push_back(plotColor(positiveColor, backgroundColor, isoLevels[i]/maxPlotValue));
  for(int i = numLevelsPos; i < numLevelsPos+numLevelsNeg; i++)
    isoColors.push_back(plotColor(negativeColor, backgroundColor, isoLevels[i]/minPlotValue));
  // colors for isolines 
  vector<QColor> lineColors;
  lineColors.reserve(numLevelsPos+numLevelsNeg);
  for(int i = 0; i < numLevelsPos; i++)
    lineColors.push_back(positiveColor);
  for(int i = numLevelsPos; i < numLevelsPos+numLevelsNeg; i++)
    lineColors.push_back(negativeColor);

  ///// clear allocated data
  for(unsigned int i = 0; i < isoCurves.size(); i++)
    delete isoCurves[i];
  isoCurves.clear();

  ///// loop over all segments and generate isodensity points for each level
  // the following vector keeps all found isodensity points for each level
  vector< vector<Point3D<double> >* > segments;
  segments.reserve(isoLevels.size());
  // the following maps between segment numbers and indices in the segments vector
  vector<map<unsigned int, unsigned int>* > segmentMaps;
  segmentMaps.reserve(isoLevels.size());
  for(unsigned int i = 0; i < isoLevels.size(); i++)
  {
    vector<Point3D<double> >* newSegment = new vector<Point3D<double> >();
    segments.push_back(newSegment);
    map<unsigned int, unsigned int>* newMap = new map<unsigned int, unsigned int>();
    segmentMaps.push_back(newMap);
  }

  ///// loop over all horizontal segments
  for(unsigned int y = 0; y < numPoints.y(); y++)
  {
    unsigned int currentSegment = y * (2*numPoints.x()-1);
    for(unsigned int x = 0; x < numPoints.x() - 1; x++)
    {
      ///// we're on line y and loop over all points on this line
      ///// we get numPoints.x() - 1 segments to check defined by x and x+1
      for(unsigned int level = 0; level < isoLevels.size(); level++)
      {
        if( (points[y][x] <= isoLevels[level] && points[y][x+1] > isoLevels[level]) ||
            (points[y][x] > isoLevels[level] && points[y][x+1] <= isoLevels[level]) )
        {
          ///// isoLevels[level] passes through this segment so determine the exact point by linear interpolation
          Point3D<double> point1(static_cast<double>(x), static_cast<double>(y), 0.0);
          Point3D<double> point2(static_cast<double>(x+1), static_cast<double>(y), 0.0);
          Point3D<double> result = interpolate2D(point1, point2, points[y][x], points[y][x+1], isoLevels[level]);
          result.setID(currentSegment);
          segments[level]->push_back(result);
          ///// map the segment number to the index in the segments vector
          segmentMaps[level]->operator[](currentSegment) = segments[level]->size() - 1;
        }
      }
      currentSegment++;
    }
  }
  ///// loop over all vertical segments
  for(unsigned int y = 0; y < numPoints.y() - 1; y++)
  {
    unsigned int currentSegment = (2*y + 1)*numPoints.x() - y - 1; //from numPoints.x() - 1 + y * (2*numPoints.x()-1);
    for(unsigned int x = 0; x < numPoints.x(); x++)
    {
      ///// we're dealing with the segments between line y and y+1
      ///// it contains numPoints.x() of these segments
      for(unsigned int level = 0; level < isoLevels.size(); level++)
      {
        if( (points[y][x] <= isoLevels[level] && points[y+1][x] > isoLevels[level]) ||
            (points[y][x] > isoLevels[level] && points[y+1][x] <= isoLevels[level]))
        {
          ///// isoLevels[level] passes through this segment so determine the exact point by linear interpolation
          Point3D<double> point1(static_cast<double>(x), static_cast<double>(y), 0.0);
          Point3D<double> point2(static_cast<double>(x), static_cast<double>(y+1), 0.0);
          Point3D<double> result = interpolate2D(point1, point2, points[y][x], points[y+1][x], isoLevels[level]);
          result.setID(currentSegment);
          segments[level]->push_back(result);
          ///// map the segment number to the index in the segments vector
          segmentMaps[level]->operator[](currentSegment) = segments[level]->size() - 1;
        }
      }
      currentSegment++;
    }
  }

  ///// now extract curves from all data
  const unsigned int totalSegments = (numPoints.x() - 1)*numPoints.y() + numPoints.x()*(numPoints.y() - 1);
  //vector<QColor> isoCurveColors; // holds the color for each curve
  vector<QColor> lineCurveColors;
  vector< vector<Point3D<double> > > isoPolygons; // polygon data for each iso level
  isoPolygons.reserve(numLevelsPos+numLevelsNeg); // should equal the size of isoColors and isoLevels
  for(unsigned int level = 0; level < isoLevels.size(); level++)
  {
    ///// first do all open curves => starting at edge segments
    vector<bool> segmentVisited;
    segmentVisited.reserve(totalSegments);
    for(unsigned int i = 0; i < totalSegments; i++)
      segmentVisited.push_back(false);

    const unsigned int firstCurve = isoCurves.size(); // index of the first curve of this level
    // the top horizontal row
    for(unsigned int segment = 0; segment < numPoints.x() - 1; segment++)
    {
      // find out whether a Point3D exists with this segment ID
      if(!segmentVisited[segment] && hasPoint(segment, level, segmentMaps))
      {
        startSearch(segment, level, false, segmentMaps, segmentVisited, segments);
        //isoCurveColors.push_back(isoColors[level]);
        lineCurveColors.push_back(lineColors[level]);
      }
    }
    // the rightmost column
    for(unsigned int segment = 2*(numPoints.x() - 1); segment < totalSegments; segment += 2*numPoints.x() - 1)
    {
      if(!segmentVisited[segment] && hasPoint(segment, level, segmentMaps))
      {
        startSearch(segment, level, false, segmentMaps, segmentVisited, segments);
        //isoCurveColors.push_back(isoColors[level]);
        lineCurveColors.push_back(lineColors[level]);
     }
    }
    // the bottom horizontal row
    for(unsigned int segment = totalSegments - (numPoints.x() - 1); segment < totalSegments; segment++)
    {
      if(!segmentVisited[segment] && hasPoint(segment, level, segmentMaps))
      {
        startSearch(segment, level, false, segmentMaps, segmentVisited, segments);
        //isoCurveColors.push_back(isoColors[level]);
        lineCurveColors.push_back(lineColors[level]);
      }
    }
    // the leftmost column
    for(unsigned int segment = numPoints.x() - 1; segment < totalSegments; segment += 2*numPoints.x() - 1)
    {
      if(!segmentVisited[segment] && hasPoint(segment, level, segmentMaps))
      {
        startSearch(segment, level, false, segmentMaps, segmentVisited, segments);
        //isoCurveColors.push_back(isoColors[level]);
        lineCurveColors.push_back(lineColors[level]);
     }
    }

    const unsigned int firstCircularCurve = isoCurves.size(); // index of the first curve of this level (only circular ones)
    // all segments (visited ones will be skipped anyway)
    for(unsigned int segment = 0; segment < totalSegments; segment++)
    {
      if(!segmentVisited[segment] && hasPoint(segment, level, segmentMaps))
      {
        startSearch(segment, level, true, segmentMaps, segmentVisited, segments);
        //isoCurveColors.push_back(isoColors[level]);
        lineCurveColors.push_back(lineColors[level]);
      }        
    }
    const unsigned int lastCurve = isoCurves.size(); // index of the last curve of this level
    // combine all the generated curves into 1 polygon
    vector<Point3D<double> > isoPolygon;
    Point3D<double> zeroPoint(0.0, 0.0, 0.0);
    // combine all non-circular curves into 1 by adding them in the correct order and 
    // by possibly adding corner points
    if(firstCurve != firstCircularCurve)
    {
      //qDebug("started with a point at (0,0) for non-circular curves");
      //qDebug("added first curve from (%f,%f) to (%f,%f)",isoCurves[firstCurve]->operator[](0).x(), isoCurves[firstCurve]->operator[](0).y(),
      //       isoCurves[firstCurve]->operator[](isoCurves[firstCurve]->size() - 1).x(),isoCurves[firstCurve]->operator[](isoCurves[firstCurve]->size() - 1).y());                                                   
      vector<bool> curveAdded;
      curveAdded.reserve(firstCircularCurve - firstCurve);
      for(unsigned int i = firstCurve; i < firstCircularCurve; i++)
        curveAdded.push_back(false);
      // always pass by zero
      isoPolygon.push_back(zeroPoint);
      // add the first curve
      isoPolygon.reserve(isoPolygon.size() + isoCurves[firstCurve]->size());
      isoPolygon.insert(isoPolygon.end(),isoCurves[firstCurve]->begin(), isoCurves[firstCurve]->end());
      curveAdded[0] = true;
      // recursively get the next curves until all have been added
      continuePolygon(firstCurve, firstCircularCurve, firstCurve, isoPolygon, curveAdded, isoCurves[firstCurve]->operator[](0), false, isoLevels[level]);
    }
    // add the circular curves
    for(unsigned int i = firstCircularCurve; i < lastCurve; i++)
    {
      // always pass by zero
      isoPolygon.push_back(zeroPoint);
      // add the contents of the curve (completely thus some extra points are added)
      isoPolygon.reserve(isoPolygon.size() + isoCurves[i]->size());
      isoPolygon.insert(isoPolygon.end(),isoCurves[i]->begin(), isoCurves[i]->end());
    }
    //if(firstCurve == lastCurve)
      // add a zero so the polygon is not empty
      isoPolygon.push_back(zeroPoint);

    // add it to the list
    isoPolygons.push_back(isoPolygon);
  }

  //plotLabel->setIsoCurves(isoCurves, isoCurveColors, lineCurveColors);
  plotLabel->setIsoCurves(isoCurves, lineCurveColors);
  plotLabel->setPolygons(isoPolygons, isoColors);
}

///// plotColor /////////////////////////////////////////////////////////////
QColor PlotMapBase::plotColor(const QColor foregroundColor, const QColor backgroundColor, const double opaqueness)
/// Returns a color calculated as the foregroundColor
/// with an opaqueness against the backgroundColor.
{
  double intensity = opaqueness;
  limitRange(0.0, intensity, 1.0);
    
  int newRed = 1000, newGreen = 1000, newBlue = 1000;

  if(backgroundColor.red() == foregroundColor.red())
    newRed = backgroundColor.red();
  else if(backgroundColor.red() < foregroundColor.red())
    newRed = backgroundColor.red() + int(double(foregroundColor.red() - backgroundColor.red())*intensity);
  else
    newRed = foregroundColor.red() + int(double(backgroundColor.red() - foregroundColor.red())*(1.0 - intensity));

  if(backgroundColor.green() == foregroundColor.green())
    newGreen = backgroundColor.green();
  else if(backgroundColor.green() < foregroundColor.green())
    newGreen = backgroundColor.green() + int(double(foregroundColor.green() - backgroundColor.green())*intensity);
  else
    newGreen = foregroundColor.green() + int(double(backgroundColor.green() - foregroundColor.green())*(1.0 - intensity));

  if(backgroundColor.blue() == foregroundColor.blue())
    newBlue = backgroundColor.blue();
  else if(backgroundColor.blue() < foregroundColor.blue())
    newBlue = backgroundColor.blue() + int(double(foregroundColor.blue() - backgroundColor.blue())*intensity);
  else
    newBlue = foregroundColor.blue() + int(double(backgroundColor.blue() - foregroundColor.blue())*(1.0 - intensity));

  //qDebug("backColor:  %d, %d, %d frontColor: %d, %d, %d intensity: %f result: %d, %d, %d",backgroundColor.red(), backgroundColor.green(), backgroundColor.blue(),foregroundColor.red(), foregroundColor.green(), foregroundColor.blue(),intensity, newRed, newGreen, newBlue);
    
  limitRange(0, newRed, 255);
  limitRange(0, newGreen, 255);
  limitRange(0, newBlue, 255);
      
  return QColor(newRed, newGreen, newBlue);
}

///// onPixmap ////////////////////////////////////////////////////////////////
bool PlotMapBase::onPixmap(const QPoint position)
/// Returns true if position is on the actual drawing area.
{
  /*
  QRect rect = plotLabel->geometry();
  int fwidth = plotLabel->frameWidth();
  rect.setLeft(rect.left() + fwidth);
  rect.setRight(rect.right() - fwidth);
  rect.setTop(rect.top() + fwidth);
  rect.setBottom(rect.bottom() - fwidth);
  */
  QRect rect = labelRect();


  return rect.contains(position);
}

///// mapToImage //////////////////////////////////////////////////////////////
QPoint PlotMapBase::mapToImage(const QPoint position)
/// Returns the pixel position in the image
/// corresponding to the point position on the widget.
{
  QRect rect = labelRect();
  QPoint intermediate = position - rect.topLeft();

  return QPoint( intermediate.x()*numPoints.x()/rect.width(), 
                 intermediate.y()*numPoints.y()/rect.height() );

  //int fwidth = plotLabel->frameWidth();
  //QPoint intermediate = position - plotLabel->pos();
  //return QPoint( (intermediate.x() - fwidth)*numPoints.x()/(plotLabel->geometry().width() - 2*fwidth), 
  //               (intermediate.y() - fwidth)*numPoints.y()/(plotLabel->geometry().height() - 2*fwidth) );
}

///// interpolate2D ///////////////////////////////////////////////////////////
Point3D<double> PlotMapBase::interpolate2D(const Point3D<double>& point1, const Point3D<double>& point2, const double density1, const double density2, const double isoLevel)
/// Calculates the interpolated point in 2D between
/// point1 and point2 with density values of density1 and density2 respectively.
/// The target density is isoLevel.
{
  Point3D<double> lowPoint, highPoint;
  double lowDensity, highDensity, mu;
  if(density1 < density2)
  {
    lowPoint = point1;
    highPoint = point2;
    lowDensity = density1;
    highDensity = density2;
  }
  else 
  {
    lowPoint = point2;
    highPoint = point1;
    lowDensity = density2;
    highDensity = density1;
  }

  if(highDensity == lowDensity)
    mu = 0.5;
  else
    mu = (isoLevel - lowDensity)/(highDensity - lowDensity);
  const double x = lowPoint.x() + mu*(highPoint.x() - lowPoint.x());
  const double y = lowPoint.y() + mu*(highPoint.y() - lowPoint.y());

  return Point3D<double>(x, y, 0.0);
}

///// hasPoint ////////////////////////////////////////////////////////////////
bool PlotMapBase::hasPoint(const unsigned int segment, const unsigned int level, const vector<map<unsigned int, unsigned int>* >& segmentMaps)
/// Returns true if segment has a Point3D for isoLevel index level.
{
  //map<unsigned int, unsigned int>::iterator it = segmentMaps[level]->find(segment);
  return segmentMaps[level]->find(segment) != segmentMaps[level]->end();

}

///// getPoint ////////////////////////////////////////////////////////////////
Point3D<double> PlotMapBase::getPoint(const unsigned int segment, const unsigned int level, const vector<map<unsigned int, unsigned int>* >& segmentMaps, const vector< vector<Point3D<double> >* >& segments)
/// Returns the Point3D corresponding to the segment for isoLevel index \c level.
{
  map<unsigned int, unsigned int>::iterator it = segmentMaps[level]->find(segment);
  if(it != segmentMaps[level]->end())
  {
    // verify the returned point has the same ID as the requested segment
    if(segment != segments[level]->at(it->second).id())
    {
      qDebug("asked for point corresponding to segment %d", segment);
      qDebug("returned point with ID %d", segments[level]->at(it->second).id());
    }

    return segments[level]->at(it->second);
  }
  else
    return Point3D<double>();
}

///// startSearch /////////////////////////////////////////////////////////////
void PlotMapBase::startSearch(const unsigned int segment, const unsigned int level, const bool circular, const vector<map<unsigned int, unsigned int>* >& segmentMaps, vector<bool>& segmentVisited, const vector< vector<Point3D<double> >* >& segments)
/// Starts a search for an isoline.
{
  ///// start a new curve
  vector<Point3D<double> >* newCurve = new vector<Point3D<double> >();
  isoCurves.push_back(newCurve);
  ///// add the current point
  Point3D<double> firstPoint = getPoint(segment, level, segmentMaps, segments);
  newCurve->push_back(firstPoint);
  ///// add it a second time if the curve is not circular
  if(!circular)
    newCurve->push_back(firstPoint);
  ///// mark it as visited
  segmentVisited[segment] = true;
  unsigned int oldSegment = segment;
  unsigned int newSegment = segment;
  if(circular)
  {
    // set up a previously visited segment for a circular curve to proceed succesfully
    // go down for horizontal segments and go right for vertical segments by defining
    // the previous segment being 1 smaller than the current segment
    oldSegment--;
  }

  //bool print = false;
  //if(isoCurves.size() == 1)
  //  print = true;

  //qDebug("starting a new curve at segment %d (%f, %f) for level %d ", segment, firstPoint.x(), firstPoint.y(), level);

  Point3D<double> newPoint = firstPoint;
  unsigned int segment1, segment2, segment3;
  while(true)
  {
    ///// find the neighbouring segments with points
    ///// the last added point is newSegment and the previous one is oldSegment
    if(!getNeighbours(newSegment, oldSegment, segment1, segment2, segment3))
    {
      if(circular)
        qDebug("end of curve reached although a circular curve is searched");
      // edge segment
      // the final point of a non-circular curve has been reached
      // so add this point again and return
      newCurve->push_back(newPoint);
      //if(print) qDebug("last point of noncircular curve: point %u: (%f, %f)", newCurve->size() - 1, newPoint.x(), newPoint.y());
      return;           
    }
    else
    {
      // check which of the neighbouring segments have a Point3D
      bool segment1HasPoint = hasPoint(segment1, level, segmentMaps) && !segmentVisited[segment1];
      bool segment2HasPoint = hasPoint(segment2, level, segmentMaps) && !segmentVisited[segment2];
      bool segment3HasPoint = hasPoint(segment3, level, segmentMaps) && !segmentVisited[segment3];

      /*
      qDebug("possible new directions: segment %d, segment %d or segment %d", segment1, segment2, segment3);
      QString test = " have a Point3D associated: ";
      if(hasPoint(segment1, level, segmentMaps))
        test += "segment1 ";
      if(hasPoint(segment2, level, segmentMaps))
        test += "segment2 ";
      if(hasPoint(segment3, level, segmentMaps))
        test += "segment3 ";
      qDebug(test);
      test = " are not visited yet: ";
      if(!segmentVisited[segment1])
        test += "segment1 ";
      if(!segmentVisited[segment2])
        test += "segment2 ";
      if(!segmentVisited[segment3])
        test += "segment3 ";
      qDebug(test);
      */

      if(segment1HasPoint && segment2HasPoint && segment3HasPoint)
      {
        // 3 connecting segments have been found
        // we have a problem because this leaves us with 2
        // choices for the next segment (segment1 or segment3)
        // => choose segment1 as a test
        oldSegment = newSegment;
        newSegment = segment1;
        newPoint = getPoint(newSegment, level, segmentMaps, segments);
        newCurve->push_back(newPoint);
        segmentVisited[newSegment] = true;
        //if(print) qDebug("next point of curve (ambiguous case): point %u: (%f, %f)", newCurve->size() - 1, newPoint.x(), newPoint.y());
      }
      else if(!segment1HasPoint && !segment2HasPoint && !segment3HasPoint)
      {
        // no possibilities left so this must be the end of a circular line
        if(circular)
        {
          //qDebug("last point of circular line. closing with first point");
          newCurve->push_back(firstPoint);
          // while drawing check whether the ID of the first and last points are identical for closed curves
          return;
        }
        else
        {
          // a non-circular curve ended in the middle of the grid => shouldn't happen
          newCurve->push_back(newPoint);
          
          qDebug(" double last point : point %d: (%f, %f)", static_cast<int>(newCurve->size() - 1), newPoint.x(), newPoint.y());
          qDebug("end of non-circular curve reached in the middle of the field");
          qDebug(" current segment = %d, possibly next segments: %d, %d and %d", newSegment, segment1, segment2, segment3);
          qDebug(" point of current segment: (%f, %f)",newPoint.x(), newPoint.y());
          if(isHorizontal(newSegment))
            qDebug(" current segment is horizontal");
          else
            qDebug(" current segment is vertical");
          if(hasPoint(segment1, level, segmentMaps))
            qDebug(" segment1 has a point but is already visited");
          if(hasPoint(segment2, level, segmentMaps))
            qDebug(" segment2 has a point but is already visited");
          if(hasPoint(segment3, level, segmentMaps))
            qDebug(" segment3 has a point but is already visited");
          
          return;
        }
      }
      else
      {
        // only 1 segment is left (2 is impossible as segments are taken away from cells in pairs
        oldSegment = newSegment;
        if(segment1HasPoint)
          newSegment = segment1;
        else if(segment2HasPoint)
          newSegment = segment2;
        else
          newSegment = segment3;
        newPoint = getPoint(newSegment, level, segmentMaps, segments);
        newCurve->push_back(newPoint);
        segmentVisited[newSegment] = true;
        //if(print) qDebug("next point of curve: point %u: (%f, %f)", newCurve->size() - 1, newPoint.x(), newPoint.y());

      }      
    }
  }
}

///// getNeighbours ///////////////////////////////////////////////////////////
bool PlotMapBase::getNeighbours(const unsigned int currentSegment, const unsigned int previousSegment, unsigned int& segment1, unsigned int& segment2, unsigned int& segment3)
/// Returns the neighbours of currentSegment not from the cell containing previousSegment.
{
  ///// find out what kind of segment currentSegment is
  if(currentSegment == previousSegment)
  {
    // this is an edge segment at the start of a curve
    if(isHorizontal(currentSegment))
    {
      // top or bottom row
      if(currentSegment < numPoints.x() - 1)
      {
        // top row segment so return belowleft, below and belowright
        segment1 = currentSegment + numPoints.x() - 1;
        segment2 = currentSegment + 2*numPoints.x() - 1;
        segment3 = currentSegment + numPoints.x();
        return true;
      }
      else
      {
        // bottom row segment so return aboveleft, above and aboveright
        segment1 = currentSegment - numPoints.x();
        segment2 = currentSegment + 1 - 2*numPoints.x();
        segment3 = currentSegment + 1 - numPoints.x();
        return true;
      }
    }
    else
    {
      // first or last column
      if((currentSegment + 1 - numPoints.x()) % numPoints.x() == 0)
      {
        // first column segment so return above right, right, belowright
        segment1 = currentSegment + 1 - numPoints.x();
        segment2 = currentSegment + 1;
        segment3 = currentSegment + numPoints.x();
        return true;
      }
      else
      {
        // last column segment so return above left, left, belowleft
        segment1 = currentSegment - numPoints.x();
        segment2 = currentSegment - 1;
        segment3 = currentSegment + numPoints.x() - 1;
      }
    }
  }
  else
  {
    // the currentSegment is not at the start of a curve 
    // check in which direction should be searched
    if(isHorizontal(currentSegment))
    {
      if(previousSegment < currentSegment)
      {
        // check out the cell below (below left, below, below right
        // first check currentSegment is not an edge
        if(currentSegment >= (2*numPoints.x() - 1)*(numPoints.y() - 1))
          return false;
        else
        {
          segment1 = currentSegment + numPoints.x() - 1;
          segment2 = currentSegment + 2*numPoints.x() - 1;
          segment3 = currentSegment + numPoints.x();
          return true;
        }
      }
      else
      {
        // check out the cell above
        if(currentSegment < numPoints.x())
          return false;
        else
        {
          segment1 = currentSegment - numPoints.x();
          segment2 = currentSegment + 1 - 2*numPoints.x();
          segment3 = currentSegment + 1 - numPoints.x();
          return true;
        }
      }
    }
    else
    {
      // vertical segment
      if(previousSegment == currentSegment - 1 || previousSegment == currentSegment - numPoints.x() || previousSegment == currentSegment + numPoints.x() - 1)
      {
        // check out the right cell
        if((currentSegment - 2*(numPoints.x() - 1)) % (2*numPoints.x() - 1) == 0)
          return false;
        else
        {
          segment1 = currentSegment + 1 - numPoints.x();
          segment2 = currentSegment + 1;
          segment3 = currentSegment + numPoints.x();
          return true;
        }
      }
      else
      {
        // check out the left cell
        if((currentSegment - (numPoints.x() - 1)) % (2*numPoints.x() - 1) == 0)
          return false;
        else
        {
          segment1 = currentSegment - numPoints.x();
          segment2 = currentSegment - 1;
          segment3 = currentSegment + numPoints.x() - 1;
          return true;
        }
      }
    }
  }
  return true;
}

///// isHorizontal ////////////////////////////////////////////////////////////
bool PlotMapBase::isHorizontal(const unsigned int segment)
/// Returns true if a segment is positioned horizontally.
{
  unsigned int currentSegment = 0;
  for(unsigned int y = 0; y < numPoints.y(); y++)
  {
    if(segment >= currentSegment && segment < currentSegment + numPoints.x() - 1)
      return true;
    currentSegment += 2*numPoints.x() - 1;
  }
  return false;
}

///// continuePolygon /////////////////////////////////////////////////////////
void PlotMapBase::continuePolygon(const unsigned int startCurve, const unsigned int endCurve, unsigned int currentCurve, vector<Point3D<double> >& isoPolygon, vector<bool>& curveAdded, const Point3D<double>& startPoint, const bool fromStart, const double isoLevel)
/// Recursively adds edge-curves to form a polygon.
{  
  ///// check where should be continued
  // get the starting point
  Point3D<double> checkPoint;
  if(fromStart)
    checkPoint = isoCurves[currentCurve]->operator[](0);
  else
    checkPoint = isoCurves[currentCurve]->operator[](isoCurves[currentCurve]->size() - 1);
  //qDebug("continuePolygon: checking point (%f, %f)",checkPoint.x(), checkPoint.y());
  //qDebug("because fromStart  = %d",fromStart);
  //qDebug("end points are (%f,%f) and (%f,%f)",isoCurves[currentCurve]->operator[](0).x(),isoCurves[currentCurve]->operator[](0).y(),
  //    isoCurves[currentCurve]->operator[](isoCurves[currentCurve]->size() - 1).x(),isoCurves[currentCurve]->operator[](isoCurves[currentCurve]->size() - 1).y());
  // on which edge?
  if(static_cast<int>(checkPoint.y()) == 0)
  {
    // top row -> go left or right?
    if(fabs(points[0][static_cast<int>(checkPoint.x())]) > fabs(isoLevel))
    {
      // go left -> counterclockwise direction
      //qDebug("searching left on top row");
      nextPolygonCCWT(startCurve, endCurve, isoPolygon, curveAdded, startPoint, checkPoint, isoLevel);
    }
    else
    {
      // go right -> clockwise
      //qDebug("searching right on top row");
      nextPolygonCWT(startCurve, endCurve, isoPolygon, curveAdded, startPoint, checkPoint, isoLevel);
    }
  }
  else if(static_cast<unsigned int>(checkPoint.x()) == numPoints.x() - 1)
  {
    // right column -> go up or down
    if(fabs(points[static_cast<int>(checkPoint.y())][numPoints.x() - 1]) > fabs(isoLevel))
    {
      // go up -> counterclockwise direction
      //qDebug("searching up on right column");
      nextPolygonCCWR(startCurve, endCurve, isoPolygon, curveAdded, startPoint, checkPoint, isoLevel);
    }
    else
    {
      // go down -> clockwise
      //qDebug("searching down on right column");
      nextPolygonCWR(startCurve, endCurve, isoPolygon, curveAdded, startPoint, checkPoint, isoLevel);
    }
  }
  else if(static_cast<unsigned int>(checkPoint.y()) == numPoints.y() - 1)
  {
    /*{
      unsigned int x = static_cast<int>(checkPoint.x());
      unsigned int y = numPoints.y() - 1;
      //qDebug("bottom row, checking left point (%d,%d)=%f vs level %f",x,y,points[y][x],isoLevel); 
      //qDebug("surrounding points: x-1, x, x+1, x+2: %f, %f, %f, %f",points[y][x-1],points[y][x],points[y][x+1],points[y][x+2]);
    }*/
    // bottom row -> go left or right?
    if(fabs(points[numPoints.y() - 1][static_cast<int>(checkPoint.x())]) > fabs(isoLevel))
    {
      // go left -> clockwise direction
      //qDebug("searching left on bottom row");
      nextPolygonCWB(startCurve, endCurve, isoPolygon, curveAdded, startPoint, checkPoint, isoLevel);
    }
    else
    {
      // go right -> counterclockwise
      //qDebug("searching right on bottom row");
      nextPolygonCCWB(startCurve, endCurve, isoPolygon, curveAdded, startPoint, checkPoint, isoLevel);
    }
  }
  else if(static_cast<int>(checkPoint.x()) == 0)
  {
    // left column -> go up or down
    if(fabs(points[static_cast<int>(checkPoint.y())][0]) > fabs(isoLevel))
    {
      // go up -> clockwise direction
      //qDebug("searching up on left column");
      nextPolygonCWL(startCurve, endCurve, isoPolygon, curveAdded, startPoint, checkPoint, isoLevel);
    }
    else
    {
      // go down -> counterclockwise
      //qDebug("searching down on left column");
      nextPolygonCCWL(startCurve, endCurve, isoPolygon, curveAdded, startPoint, checkPoint, isoLevel);
    }
  }
}

///// nextPolygonCCWT /////////////////////////////////////////////////////////
void PlotMapBase::nextPolygonCCWT(const unsigned int startCurve, const unsigned int endCurve, vector<Point3D<double> >& isoPolygon, vector<bool>& curveAdded, const Point3D<double>& startPoint, const Point3D<double>& refPoint, const double isoLevel)
/// Find the next curve in a counterclockwise direction on the top row.
{
  //qDebug("entering nextPolygonCCWT");
  unsigned int nearestCurve = endCurve;
  Point3D<double> nearestPoint;
  bool nearestStart = false;
  for(unsigned int curve = startCurve; curve < endCurve; curve++)
  {
    if(!curveAdded[curve - startCurve])
    {
      Point3D<double> firstPoint = isoCurves[curve]->operator[](0); 
      Point3D<double> lastPoint = isoCurves[curve]->operator[](isoCurves[curve]->size() - 1); 
      if(static_cast<int>(firstPoint.y()) == 0 && firstPoint.x() < refPoint.x())
      {
        // curve found -> check whether it's the nearest
        if(nearestCurve == endCurve || firstPoint.x() > nearestPoint.x())
        {
          // no previous curve specified or nearer than nearestCurve
          nearestCurve = curve;
          nearestPoint = firstPoint;
          nearestStart = true;
        }
      }
      else if(static_cast<int>(lastPoint.y()) == 0 && lastPoint.x() < refPoint.x())
      {
        if(nearestCurve == endCurve || lastPoint.x() > nearestPoint.x())
        {
          nearestCurve = curve;
          nearestPoint = lastPoint;
          nearestStart = false;
        }
      }
    }
  }
  // now the nearest curve and point are defined
  if(nearestCurve == endCurve)
  {
    // no curve found so check whether all curves have been assigned
    bool allVisited = true;
    for(unsigned int i = 0; i < endCurve - startCurve; i++)
    {
      if(!curveAdded[i])
      {
        allVisited = false;
        break;
      }
    }
    if(allVisited)
    {
      // only return if the starting point is on the same edge as the current end
      if(static_cast<int>(startPoint.y()) == 0)
      {
        isoPolygon.push_back(startPoint);
        return;
      }
    }
    // if not add the (0,0) corner point and search down along the left column 
    Point3D<double> zeroPoint(0.0, 0.0, 0.0);
    isoPolygon.push_back(zeroPoint);
    nextPolygonCCWL(startCurve, endCurve, isoPolygon, curveAdded, startPoint, zeroPoint, isoLevel);
  }
  else
  {
    // a new curve is found
    curveAdded[nearestCurve - startCurve] = true;
    isoPolygon.reserve(isoPolygon.size() + isoCurves[nearestCurve]->size());
    if(nearestStart)
    {
      // the new curve should be added from the start
      isoPolygon.insert(isoPolygon.end(),isoCurves[nearestCurve]->begin(), isoCurves[nearestCurve]->end());
    }
    else
    {
      // the new curve should be added from the end
      isoPolygon.insert(isoPolygon.end(),isoCurves[nearestCurve]->rbegin(), isoCurves[nearestCurve]->rend());
    }
    // continue this polygon
    continuePolygon(startCurve, endCurve, nearestCurve, isoPolygon, curveAdded, startPoint, !nearestStart, isoLevel);
  }
}

///// nextPolygonCWT //////////////////////////////////////////////////////////
void PlotMapBase::nextPolygonCWT(const unsigned int startCurve, const unsigned int endCurve, vector<Point3D<double> >& isoPolygon, vector<bool>& curveAdded, const Point3D<double>& startPoint, const Point3D<double>& refPoint, const double isoLevel)
/// Find the next curve in a clockwise direction on the top row.
{  
  //qDebug("entering nextPolygonCWT");
  unsigned int nearestCurve = endCurve;
  Point3D<double> nearestPoint;
  bool nearestStart = false;
  for(unsigned int curve = startCurve; curve < endCurve; curve++)
  {
    if(!curveAdded[curve - startCurve])
    {
      Point3D<double> firstPoint = isoCurves[curve]->operator[](0); 
      Point3D<double> lastPoint = isoCurves[curve]->operator[](isoCurves[curve]->size() - 1); 
      if(static_cast<int>(firstPoint.y()) == 0 && firstPoint.x() > refPoint.x())
      {
        // curve found -> check whether it's the nearest
        if(nearestCurve == endCurve || firstPoint.x() < nearestPoint.x())
        {
          // no previous curve specified or nearer than nearestCurve
          nearestCurve = curve;
          nearestPoint = firstPoint;
          nearestStart = true;
        }
      }
      else if(static_cast<int>(lastPoint.y()) == 0 && lastPoint.x() > refPoint.x())
      {
        if(nearestCurve == endCurve || lastPoint.x() < nearestPoint.x())
        {
          nearestCurve = curve;
          nearestPoint = lastPoint;
          nearestStart = false;
        }
      }
    }
  }
  // now the nearest curve and point are defined
  if(nearestCurve == endCurve)
  {
    // no curve found so check whether all curves have been assigned
    bool allVisited = true;
    for(unsigned int i = 0; i < endCurve - startCurve; i++)
    {
      if(!curveAdded[i])
      {
        allVisited = false;
        break;
      }
    }
    if(allVisited)
    {
      // only return if the starting point is on the same edge as the current end
      if(static_cast<int>(startPoint.y()) == 0)
      {
        isoPolygon.push_back(startPoint);
        return;
      }
    }
    // if not add the (xmax,0) corner point and search down along the right column 
    Point3D<double> cornerPoint(static_cast<double>(numPoints.x() - 1), 0.0, 0.0);
    isoPolygon.push_back(cornerPoint);
    nextPolygonCWR(startCurve, endCurve, isoPolygon, curveAdded, startPoint, cornerPoint, isoLevel);
  }
  else
  {
    // a new curve is found
    curveAdded[nearestCurve - startCurve] = true;
    isoPolygon.reserve(isoPolygon.size() + isoCurves[nearestCurve]->size());
    if(nearestStart)
    {
      // the new curve should be added from the start
      isoPolygon.insert(isoPolygon.end(),isoCurves[nearestCurve]->begin(), isoCurves[nearestCurve]->end());
    }
    else
    {
      // the new curve should be added from the end
      isoPolygon.insert(isoPolygon.end(),isoCurves[nearestCurve]->rbegin(), isoCurves[nearestCurve]->rend());
    }
    // continue this polygon
    continuePolygon(startCurve, endCurve, nearestCurve, isoPolygon, curveAdded, startPoint, !nearestStart, isoLevel);
  }
}

///// nextPolygonCCWR /////////////////////////////////////////////////////////
void PlotMapBase::nextPolygonCCWR(const unsigned int startCurve, const unsigned int endCurve, vector<Point3D<double> >& isoPolygon, vector<bool>& curveAdded, const Point3D<double>& startPoint, const Point3D<double>& refPoint, const double isoLevel)
/// Find the next curve in a counterclockwise direction on the rightmost column.
{  
  //qDebug("entering nextPolygonCCWR");
  unsigned int nearestCurve = endCurve;
  Point3D<double> nearestPoint;
  bool nearestStart = false;
  for(unsigned int curve = startCurve; curve < endCurve; curve++)
  {
    if(!curveAdded[curve - startCurve])
    {
      Point3D<double> firstPoint = isoCurves[curve]->operator[](0); 
      Point3D<double> lastPoint = isoCurves[curve]->operator[](isoCurves[curve]->size() - 1); 
      if(static_cast<unsigned int>(firstPoint.x()) == numPoints.x() - 1 && firstPoint.y() < refPoint.y())
      {
        // curve found -> check whether it's the nearest
        if(nearestCurve == endCurve || firstPoint.y() > nearestPoint.y())
        {
          // no previous curve specified or nearer than nearestCurve
          nearestCurve = curve;
          nearestPoint = firstPoint;
          nearestStart = true;
        }
      }
      else if(static_cast<unsigned int>(lastPoint.x()) == numPoints.x() - 1 && lastPoint.y() < refPoint.y())
      {
        if(nearestCurve == endCurve || lastPoint.y() > nearestPoint.y())
        {
          nearestCurve = curve;
          nearestPoint = lastPoint;
          nearestStart = false;
        }
      }
    }
  }
  // now the nearest curve and point are defined
  if(nearestCurve == endCurve)
  {
    //qDebug("nextPolygonCCWR: no curve found");
    // no curve found so check whether all curves have been assigned
    bool allVisited = true;
    for(unsigned int i = 0; i < endCurve - startCurve; i++)
    {
      if(!curveAdded[i])
      {
        allVisited = false;
        break;
      }
    }
    if(allVisited)
    {
      // only return if the starting point is on the same edge as the current end
      if(static_cast<unsigned int>(startPoint.x()) == numPoints.x() - 1)
      {
        isoPolygon.push_back(startPoint);
        return;
      }
    }
    // if not add the (xmax,0) corner point and search left along the top row
    //qDebug("nextPolygonCCWR: adding corner point and calling nextPolygonCCWT");
    Point3D<double> cornerPoint(static_cast<double>(numPoints.x() - 1), 0.0, 0.0);
    isoPolygon.push_back(cornerPoint);
    nextPolygonCCWT(startCurve, endCurve, isoPolygon, curveAdded, startPoint, cornerPoint, isoLevel);
  }
  else
  {
    //qDebug("nextPolygonCCWR: curve %d added",nearestCurve - startCurve);
    // a new curve is found
    curveAdded[nearestCurve - startCurve] = true;
    isoPolygon.reserve(isoPolygon.size() + isoCurves[nearestCurve]->size());
    if(nearestStart)
    {
      // the new curve should be added from the start
      isoPolygon.insert(isoPolygon.end(),isoCurves[nearestCurve]->begin(), isoCurves[nearestCurve]->end());
    }
    else
    {
      // the new curve should be added from the end
      isoPolygon.insert(isoPolygon.end(),isoCurves[nearestCurve]->rbegin(), isoCurves[nearestCurve]->rend());
    }
    // continue this polygon
    continuePolygon(startCurve, endCurve, nearestCurve, isoPolygon, curveAdded, startPoint, !nearestStart, isoLevel);
  }
}

///// nextPolygonCWR /////////////////////////////////////////////////////////
void PlotMapBase::nextPolygonCWR(const unsigned int startCurve, const unsigned int endCurve, vector<Point3D<double> >& isoPolygon, vector<bool>& curveAdded, const Point3D<double>& startPoint, const Point3D<double>& refPoint, const double isoLevel)
/// Find the next curve in a clockwise direction on the rightmost column.
{  
  //qDebug("entering nextPolygonCWR");
  unsigned int nearestCurve = endCurve;
  Point3D<double> nearestPoint;
  bool nearestStart = false;
  for(unsigned int curve = startCurve; curve < endCurve; curve++)
  {
    if(!curveAdded[curve - startCurve])
    {
      Point3D<double> firstPoint = isoCurves[curve]->operator[](0); 
      Point3D<double> lastPoint = isoCurves[curve]->operator[](isoCurves[curve]->size() - 1); 
      if(static_cast<unsigned int>(firstPoint.x()) == numPoints.x() - 1 && firstPoint.y() > refPoint.y())
      {
        // curve found -> check whether it's the nearest
        if(nearestCurve == endCurve || firstPoint.y() < nearestPoint.y())
        {
          // no previous curve specified or nearer than nearestCurve
          nearestCurve = curve;
          nearestPoint = firstPoint;
          nearestStart = true;
        }
      }
      else if(static_cast<unsigned int>(lastPoint.x()) == numPoints.x() - 1 && lastPoint.y() > refPoint.y())
      {
        if(nearestCurve == endCurve || lastPoint.y() < nearestPoint.y())
        {
          nearestCurve = curve;
          nearestPoint = lastPoint;
          nearestStart = false;
        }
      }
    }
  }
  // now the nearest curve and point are defined
  if(nearestCurve == endCurve)
  {
    // no curve found so check whether all curves have been assigned
    bool allVisited = true;
    for(unsigned int i = 0; i < endCurve - startCurve; i++)
    {
      if(!curveAdded[i])
      {
        allVisited = false;
        break;
      }
    }
    if(allVisited)
    {
      // only return if the starting point is on the same edge as the current end
      if(static_cast<unsigned int>(startPoint.x()) == numPoints.x() - 1)
      {
        isoPolygon.push_back(startPoint);
        return;
      }
    }
    // if not add the (xmax,ymax) corner point and search left along the bottom row
    Point3D<double> cornerPoint(static_cast<double>(numPoints.x() - 1), static_cast<double>(numPoints.y() - 1), 0.0);
    isoPolygon.push_back(cornerPoint);
    nextPolygonCWB(startCurve, endCurve, isoPolygon, curveAdded, startPoint, cornerPoint, isoLevel);
  }
  else
  {
    // a new curve is found
    curveAdded[nearestCurve - startCurve] = true;
    isoPolygon.reserve(isoPolygon.size() + isoCurves[nearestCurve]->size());
    if(nearestStart)
    {
      // the new curve should be added from the start
      isoPolygon.insert(isoPolygon.end(),isoCurves[nearestCurve]->begin(), isoCurves[nearestCurve]->end());
    }
    else
    {
      // the new curve should be added from the end
      isoPolygon.insert(isoPolygon.end(),isoCurves[nearestCurve]->rbegin(), isoCurves[nearestCurve]->rend());
    }
    // continue this polygon
    continuePolygon(startCurve, endCurve, nearestCurve, isoPolygon, curveAdded, startPoint, !nearestStart, isoLevel);
  }
}

///// nextPolygonCCWB /////////////////////////////////////////////////////////
void PlotMapBase::nextPolygonCCWB(const unsigned int startCurve, const unsigned int endCurve, vector<Point3D<double> >& isoPolygon, vector<bool>& curveAdded, const Point3D<double>& startPoint, const Point3D<double>& refPoint, const double isoLevel)
/// Find the next curve in a counterclockwise direction on the bottom row.
{  
  //qDebug("entering nextPolygonCCWB");
  unsigned int nearestCurve = endCurve;
  Point3D<double> nearestPoint;
  bool nearestStart = false;
  for(unsigned int curve = startCurve; curve < endCurve; curve++)
  {
    if(!curveAdded[curve - startCurve])
    {
      Point3D<double> firstPoint = isoCurves[curve]->operator[](0); 
      Point3D<double> lastPoint = isoCurves[curve]->operator[](isoCurves[curve]->size() - 1); 
      if(static_cast<unsigned int>(firstPoint.y()) == numPoints.y() - 1 && firstPoint.x() > refPoint.x())
      {
        // curve found -> check whether it's the nearest
        if(nearestCurve == endCurve || firstPoint.x() < nearestPoint.x())
        {
          // no previous curve specified or nearer than nearestCurve
          nearestCurve = curve;
          nearestPoint = firstPoint;
          nearestStart = true;
        }
      }
      else if(static_cast<unsigned int>(lastPoint.y()) == numPoints.y() - 1 && lastPoint.x() > refPoint.x())
      {
        if(nearestCurve == endCurve || lastPoint.x() > nearestPoint.x())
        {
          nearestCurve = curve;
          nearestPoint = lastPoint;
          nearestStart = false;
        }
      }
    }
  }
  // now the nearest curve and point are defined
  if(nearestCurve == endCurve)
  {
    // no curve found so check whether all curves have been assigned
    bool allVisited = true;
    for(unsigned int i = 0; i < endCurve - startCurve; i++)
    {
      if(!curveAdded[i])
      {
        allVisited = false;
        break;
      }
    }
    if(allVisited)
    {
      // only return if the starting point is on the same edge as the current end
      if(static_cast<unsigned int>(startPoint.y()) == numPoints.y() - 1)
      {
        isoPolygon.push_back(startPoint);
        return;
      }
    }
    // if not add the (xmax,ymax) corner point and search up along the right column 
    Point3D<double> zeroPoint(static_cast<double>(numPoints.x() - 1), static_cast<double>(numPoints.y() - 1), 0.0);
    isoPolygon.push_back(zeroPoint);
    nextPolygonCCWR(startCurve, endCurve, isoPolygon, curveAdded, startPoint, zeroPoint, isoLevel);
  }
  else
  {
    // a new curve is found
    curveAdded[nearestCurve - startCurve] = true;
    isoPolygon.reserve(isoPolygon.size() + isoCurves[nearestCurve]->size());
    if(nearestStart)
    {
      // the new curve should be added from the start
      isoPolygon.insert(isoPolygon.end(),isoCurves[nearestCurve]->begin(), isoCurves[nearestCurve]->end());
    }
    else
    {
      // the new curve should be added from the end
      isoPolygon.insert(isoPolygon.end(),isoCurves[nearestCurve]->rbegin(), isoCurves[nearestCurve]->rend());
    }
    // continue this polygon
    continuePolygon(startCurve, endCurve, nearestCurve, isoPolygon, curveAdded, startPoint, !nearestStart, isoLevel);
  }
}

///// nextPolygonCWB //////////////////////////////////////////////////////////
void PlotMapBase::nextPolygonCWB(const unsigned int startCurve, const unsigned int endCurve, vector<Point3D<double> >& isoPolygon, vector<bool>& curveAdded, const Point3D<double>& startPoint, const Point3D<double>& refPoint, const double isoLevel)
/// Find the next curve in a clockwise direction on the bottom row.
{  
  //qDebug("entering nextPolygonCWB");
  unsigned int nearestCurve = endCurve;
  Point3D<double> nearestPoint;
  bool nearestStart = false;
  for(unsigned int curve = startCurve; curve < endCurve; curve++)
  {
    if(!curveAdded[curve - startCurve])
    {
      Point3D<double> firstPoint = isoCurves[curve]->operator[](0); 
      Point3D<double> lastPoint = isoCurves[curve]->operator[](isoCurves[curve]->size() - 1); 
      if(static_cast<unsigned int>(firstPoint.y()) == numPoints.y() - 1 && firstPoint.x() < refPoint.x())
      {
        // curve found -> check whether it's the nearest
        if(nearestCurve == endCurve || firstPoint.x() > nearestPoint.x())
        {
          // no previous curve specified or nearer than nearestCurve
          nearestCurve = curve;
          nearestPoint = firstPoint;
          nearestStart = true;
        }
      }
      else if(static_cast<unsigned int>(lastPoint.y()) == numPoints.y() - 1 && lastPoint.x() < refPoint.x())
      {
        if(nearestCurve == endCurve || lastPoint.x() > nearestPoint.x())
        {
          nearestCurve = curve;
          nearestPoint = lastPoint;
          nearestStart = false;
        }
      }
    }
  }
  // now the nearest curve and point are defined
  if(nearestCurve == endCurve)
  {
    // no curve found so check whether all curves have been assigned
    bool allVisited = true;
    for(unsigned int i = 0; i < endCurve - startCurve; i++)
    {
      if(!curveAdded[i])
      {
        allVisited = false;
        break;
      }
    }
    if(allVisited)
    {
      // only return if the starting point is on the same edge as the current end
      if(static_cast<unsigned int>(startPoint.y()) == numPoints.y() - 1)
      {
        isoPolygon.push_back(startPoint);
        return;
      }
    }
    // if not add the (0,ymax) corner point and search up along the left column 
    Point3D<double> cornerPoint(0.0, static_cast<double>(numPoints.y() - 1), 0.0);
    isoPolygon.push_back(cornerPoint);
    nextPolygonCWL(startCurve, endCurve, isoPolygon, curveAdded, startPoint, cornerPoint, isoLevel);
  }
  else
  {
    // a new curve is found
    curveAdded[nearestCurve - startCurve] = true;
    isoPolygon.reserve(isoPolygon.size() + isoCurves[nearestCurve]->size());
    if(nearestStart)
    {
      // the new curve should be added from the start
      isoPolygon.insert(isoPolygon.end(),isoCurves[nearestCurve]->begin(), isoCurves[nearestCurve]->end());
    }
    else
    {
      // the new curve should be added from the end
      isoPolygon.insert(isoPolygon.end(),isoCurves[nearestCurve]->rbegin(), isoCurves[nearestCurve]->rend());
    }
    // continue this polygon
    continuePolygon(startCurve, endCurve, nearestCurve, isoPolygon, curveAdded, startPoint, !nearestStart, isoLevel);
  }
}

///// nextPolygonCCWL /////////////////////////////////////////////////////////
void PlotMapBase::nextPolygonCCWL(const unsigned int startCurve, const unsigned int endCurve, vector<Point3D<double> >& isoPolygon, vector<bool>& curveAdded, const Point3D<double>& startPoint, const Point3D<double>& refPoint, const double isoLevel)
/// Find the next curve in a counterclockwise direction on the leftmost column.
{  
  //qDebug("entering nextPolygonCCWL");
  unsigned int nearestCurve = endCurve;
  Point3D<double> nearestPoint;
  bool nearestStart = false;
  for(unsigned int curve = startCurve; curve < endCurve; curve++)
  {
    if(!curveAdded[curve - startCurve])
    {
      Point3D<double> firstPoint = isoCurves[curve]->operator[](0); 
      Point3D<double> lastPoint = isoCurves[curve]->operator[](isoCurves[curve]->size() - 1); 
      if(static_cast<int>(firstPoint.x()) == 0 && firstPoint.y() > refPoint.y())
      {
        // curve found -> check whether it's the nearest
        if(nearestCurve == endCurve || firstPoint.y() < nearestPoint.y())
        {
          // no previous curve specified or nearer than nearestCurve
          nearestCurve = curve;
          nearestPoint = firstPoint;
          nearestStart = true;
        }
      }
      else if(static_cast<int>(lastPoint.x()) == 0 && lastPoint.y() > refPoint.y())
      {
        if(nearestCurve == endCurve || lastPoint.y() < nearestPoint.y())
        {
          nearestCurve = curve;
          nearestPoint = lastPoint;
          nearestStart = false;
        }
      }
    }
  }
  // now the nearest curve and point are defined
  if(nearestCurve == endCurve)
  {
    // no curve found so check whether all curves have been assigned
    bool allVisited = true;
    for(unsigned int i = 0; i < endCurve - startCurve; i++)
    {
      if(!curveAdded[i])
      {
        allVisited = false;
        break;
      }
    }
    if(allVisited)
    {
      // only return if the starting point is on the same edge as the current end
      if(static_cast<int>(startPoint.x()) == 0)
      {
        isoPolygon.push_back(startPoint);
        return;
      }
    }
    // if not add the (0,ymax) corner point and search right along the bottom row
    Point3D<double> cornerPoint(0.0, static_cast<double>(numPoints.y() - 1), 0.0);
    isoPolygon.push_back(cornerPoint);
    nextPolygonCCWB(startCurve, endCurve, isoPolygon, curveAdded, startPoint, cornerPoint, isoLevel);
  }
  else
  {
    //qDebug("nextPolygonCCWL: found a new curve",nearestCurve);
    //qDebug("curve's first point = (%f,%f), last point = (%f,%f)",isoCurves[nearestCurve]->operator[](0).x(),isoCurves[nearestCurve]->operator[](0).y(),
    //  isoCurves[nearestCurve]->operator[](isoCurves[nearestCurve]->size() - 1).x(),isoCurves[nearestCurve]->operator[](isoCurves[nearestCurve]->size() - 1).y());
    // a new curve is found
    curveAdded[nearestCurve - startCurve] = true;
    isoPolygon.reserve(isoPolygon.size() + isoCurves[nearestCurve]->size());
    if(nearestStart)
    {
      //qDebug("adding curve from start to end");
      // the new curve should be added from the start
      isoPolygon.insert(isoPolygon.end(),isoCurves[nearestCurve]->begin(), isoCurves[nearestCurve]->end());
    }
    else
    {
      //qDebug("adding curve from end to start");
      // the new curve should be added from the end
      isoPolygon.insert(isoPolygon.end(),isoCurves[nearestCurve]->rbegin(), isoCurves[nearestCurve]->rend());
    }
    // continue this polygon
    continuePolygon(startCurve, endCurve, nearestCurve, isoPolygon, curveAdded, startPoint, !nearestStart, isoLevel);
  }
}

///// nextPolygonCWL /////////////////////////////////////////////////////////
void PlotMapBase::nextPolygonCWL(const unsigned int startCurve, const unsigned int endCurve, vector<Point3D<double> >& isoPolygon, vector<bool>& curveAdded, const Point3D<double>& startPoint, const Point3D<double>& refPoint, const double isoLevel)
/// Find the next curve in a clockwise direction on the leftmost column.
{  
  //qDebug("entering nextPolygonCWL");
  unsigned int nearestCurve = endCurve;
  Point3D<double> nearestPoint;
  bool nearestStart = false;
  for(unsigned int curve = startCurve; curve < endCurve; curve++)
  {
    if(!curveAdded[curve - startCurve])
    {
      Point3D<double> firstPoint = isoCurves[curve]->operator[](0); 
      Point3D<double> lastPoint = isoCurves[curve]->operator[](isoCurves[curve]->size() - 1); 
      if(static_cast<int>(firstPoint.x()) == 0 && firstPoint.y() < refPoint.y())
      {
        // curve found -> check whether it's the nearest
        if(nearestCurve == endCurve || firstPoint.y() > nearestPoint.y())
        {
          // no previous curve specified or nearer than nearestCurve
          nearestCurve = curve;
          nearestPoint = firstPoint;
          nearestStart = true;
        }
      }
      else if(static_cast<int>(lastPoint.x()) == 0 && lastPoint.y() < refPoint.y())
      {
        if(nearestCurve == endCurve || lastPoint.y() > nearestPoint.y())
        {
          nearestCurve = curve;
          nearestPoint = lastPoint;
          nearestStart = false;
        }
      }
    }
  }
  // now the nearest curve and point are defined
  if(nearestCurve == endCurve)
  {
    // no curve found so check whether all curves have been assigned
    bool allVisited = true;
    for(unsigned int i = 0; i < endCurve - startCurve; i++)
    {
      if(!curveAdded[i])
      {
        allVisited = false;
        break;
      }
    }
    if(allVisited)
    {
      // only return if the starting point is on the same edge as the current end
      if(static_cast<int>(startPoint.x()) == 0)
      {
        isoPolygon.push_back(startPoint);
        return;
      }
    }
    // if not add the (0,0) corner point and search left along the top row
    Point3D<double> cornerPoint(0.0, 0.0, 0.0);
    isoPolygon.push_back(cornerPoint);
    nextPolygonCWT(startCurve, endCurve, isoPolygon, curveAdded, startPoint, cornerPoint, isoLevel);
  }
  else
  {
    // a new curve is found
    curveAdded[nearestCurve - startCurve] = true;
    isoPolygon.reserve(isoPolygon.size() + isoCurves[nearestCurve]->size());
    if(nearestStart)
    {
      // the new curve should be added from the start
      isoPolygon.insert(isoPolygon.end(),isoCurves[nearestCurve]->begin(), isoCurves[nearestCurve]->end());
    }
    else
    {
      // the new curve should be added from the end
      isoPolygon.insert(isoPolygon.end(),isoCurves[nearestCurve]->rbegin(), isoCurves[nearestCurve]->rend());
    }
    // continue this polygon
    continuePolygon(startCurve, endCurve, nearestCurve, isoPolygon, curveAdded, startPoint, !nearestStart, isoLevel);
  }
}

///// labelRect ///////////////////////////////////////////////////////////////
QRect PlotMapBase::labelRect() const
/// Returns the inner geometry of plotLabel with respect to the widget with
/// a provision for the aspect ratio. THe returned QRect is thus the actual 
/// drawing area.
{
  // uncorrected rectangle
  QRect rect(plotLabel->geometry().topLeft() + plotLabel->contentsRect().topLeft(), plotLabel->contentsRect().size());
  
  // aspect ratio correction
  const double aspectRatio = static_cast<double>(numPoints.x()) / numPoints.y();
  if(aspectRatio > static_cast<double>(rect.width()) / rect.height())
    rect.setHeight(rect.width() / aspectRatio);
  else
    rect.setWidth(rect.height() * aspectRatio);

  // the result
  return rect;
}

///// limitRange //////////////////////////////////////////////////////////////
template<typename T> void PlotMapBase::limitRange(const T& min, T& value, const T& max)
/// Limits the value between min and max.
{
  if(min < max)
  {
    if(value < min)
      value = min;
    else if(value > max)
      value = max;
  }
  else
  { // when wrong input is given, still do correct limiting
    if(value < max)
      value = max;
    else if(value > min)
      value = min;
  }
}

///////////////////////////////////////////////////////////////////////////////
///// Static Private Member Data                                          /////
///////////////////////////////////////////////////////////////////////////////
const double PlotMapBase::AUTOANG = 1.0/1.889726342;

