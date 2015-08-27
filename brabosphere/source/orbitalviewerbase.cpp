/***************************************************************************
                     orbitalviewerbase.cpp  -  description
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

///// Comments ////////////////////////////////////////////////////////////////
/*!
  \class OrbitalViewerBase
  \brief This class is a container for showing and manipulating views or orbitals.

  It provides the functionality for OrbitalViewerWidget and manages both
  GLOrbitalView and OrbitalThread.

*/
/// \file
/// Contains the implementation of the class OrbitalViewerBase

///// Header files ////////////////////////////////////////////////////////////

// Qt header files
#include <qcombobox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qprogressbar.h>
#include <qslider.h>
#include <qspinbox.h>
#include <qtimer.h>
#include <qtoolbutton.h>
#include <qvalidator.h>
#include <qwhatsthis.h>

// Xbrabo header files
#include "atomset.h"
#include "colorbutton.h"
#include "glorbitalview.h"
#include "iconsets.h"
#include "orbitaloptionswidget.h"
#include "orbitalthread.h"
#include "orbitalviewerbase.h"
#include "version.h"

///////////////////////////////////////////////////////////////////////////////
///// Public Members                                                      /////
///////////////////////////////////////////////////////////////////////////////

///// Constructor /////////////////////////////////////////////////////////////
OrbitalViewerBase::OrbitalViewerBase(QWidget* parent, const char* name, bool modal, WFlags fl) : QDialog(parent, name, modal, fl),
  calcThread(0)
/// The default constructor.
{
  // Construct the widget layout
  BigLayout = new QHBoxLayout(this, 10);
    options = new OrbitalOptionsWidget(this);
    BigLayout->addWidget(options);
    view = new GLOrbitalView(this);
    BigLayout->addWidget(view);

  timer = new QTimer(this);
   
  // set the icons for the options widget
  options->ToolButtonUpdate->setIconSet(IconSets::getIconSet(IconSets::Start));
  options->ToolButtonCancel->setIconSet(IconSets::getIconSet(IconSets::Stop));

  // Keep the options as small as possible
  options->setFixedWidth(options->width());
  options->setMaximumWidth(options->width());
  view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  resize(options->width()*3, height());

  // Fill the combobox for atoms
  for(unsigned int i = 1; i <= AtomSet::maxElements; i++)
    options->ComboBoxAtom->insertItem(AtomSet::numToAtom(i));

  // set the initial colors
  options->ColorButtonPositive->setColor(QColor(0, 0, 255));
  options->ColorButtonNegative->setColor(QColor(255, 0, 0));
  
  // set a validator
  options->LineEditProbability->setValidator(new QDoubleValidator(this));

  // do some connections
  connect(options->SpinBoxN, SIGNAL(valueChanged(int)), this, SLOT(adjustL(int)));
  connect(options->SpinBoxL, SIGNAL(valueChanged(int)), this, SLOT(adjustM(int)));
  connect(options->ColorButtonPositive, SIGNAL(newColor(QColor*)), this, SLOT(updateColors()));
  connect(options->ComboBoxType, SIGNAL(activated(int)), this, SLOT(updateTypeOptions(int)));
  connect(options->SliderDots, SIGNAL(valueChanged(int)), options->SpinBoxDots, SLOT(setValue(int)));
  connect(options->SpinBoxDots, SIGNAL(valueChanged(int)), options->SliderDots, SLOT(setValue(int)));  
    
  connect(options->ToolButtonUpdate, SIGNAL(clicked()), this, SLOT(update()));
  connect(options->PushButtonReset, SIGNAL(clicked()), view, SLOT(resetView()));
  connect(options->PushButtonSave, SIGNAL(clicked()), view, SLOT(saveImage()));
  connect(options->ToolButtonCancel, SIGNAL(clicked()), this, SLOT(cancelCalculation()));
  connect(options->PushButtonClose, SIGNAL(clicked()), this, SLOT(close()));

  connect(timer, SIGNAL(timeout()), view, SLOT(updateGL()));  

  //connect(view, SIGNAL(maximumProgress(int)), options->ProgressBar, SLOT(setTotalSteps(int)));
  //connect(view, SIGNAL(currentProgress(int)), options->ProgressBar, SLOT(setProgress(int)));
  
  // What's This
  QWhatsThis::add(view,
    tr("<p>Shows the hydrogen-like orbital in 3D. This view can be manipulated in exactly the "
       "same way as the 3D molecular scene in calculations (read its context sensitive help for "
       "details). This means the orbital can be translated, rotated and zoomed using the mouse "
       "and/or keyboard.</p>"));
  QWhatsThis::add(options->ToolButtonUpdate, tr("Starts calculating the orbital with the requested characteristics."));
  QWhatsThis::add(options->ToolButtonCancel, tr("Stops the calculation in progress."));

  //update the view
  updateTypeOptions(0);
  update();
  setCaption(Version::appName + " - " + tr("Visualizing hydrogen-like orbitals"));
}

///// Destructor //////////////////////////////////////////////////////////////
OrbitalViewerBase::~OrbitalViewerBase()
/// The default destructor.
{
  if(calcThread != 0)
  {
    if(calcThread->running())
    {
      calcThread->stop();
      calcThread->wait();
    }
    delete calcThread;
  }
}

///////////////////////////////////////////////////////////////////////////////
///// Protected Member Functions                                          /////
///////////////////////////////////////////////////////////////////////////////

///// customEvent /////////////////////////////////////////////////////////////
void OrbitalViewerBase::customEvent(QCustomEvent* e)
/// Handles custom events originating from calcThread.
{
  ///// update the loading progress
  if(e->type() == 1001)
    options->ProgressBar->setProgress(*(static_cast<unsigned int*>(e->data())));
  ///// finish up after the thread has ended
  else if(e->type() == 1002)
    finishCalculation();
}

///////////////////////////////////////////////////////////////////////////////
///// Private Slots                                                       /////
///////////////////////////////////////////////////////////////////////////////

///// update //////////////////////////////////////////////////////////////////
void OrbitalViewerBase::update()
/// Updates the view with the values of the widgets.
{
  options->ToolButtonUpdate->setEnabled(false);
  options->ToolButtonCancel->setEnabled(true);
  // update the view
  updateColors();
  // setup the progressbar
  options->ProgressBar->setProgress(0);
  const int resolution = options->SliderResolution->value();
  const int numDots = options->SpinBoxDots->value();
  switch(options->ComboBoxType->currentItem())
  {
    case OrbitalThread::IsoProbability:
    case OrbitalThread::AccumulatedProbability:
    case OrbitalThread::AngularPart: 
      options->ProgressBar->setTotalSteps(resolution*resolution);
      break;
    case OrbitalThread::Density: 
      options->ProgressBar->setTotalSteps(numDots);
      break;
    case OrbitalThread::RadialPart: 
      options->ProgressBar->setTotalSteps(10*resolution);
      break; 
  }

  // start a computation thread
  calcThread = new OrbitalThread(this, view->getMutex(), view->getCoordinates(), 
                                 static_cast<unsigned int>(options->ComboBoxType->currentItem()),
                                 static_cast<unsigned int>(options->ComboBoxAtom->currentItem() + 1),
                                 static_cast<unsigned int>(options->SpinBoxN->value()),
                                 static_cast<unsigned int>(options->SpinBoxL->value()),
                                 static_cast<int>(options->SpinBoxM->value()),
                                 static_cast<float>(options->SliderResolution->value()),
                                 options->LineEditProbability->text().toFloat(),
                                 static_cast<unsigned int>(options->SpinBoxDots->value()));
  calcThread->start(QThread::LowPriority);

  // update the scene every 100 ms
  timer->start(100);
}

///// adjustL /////////////////////////////////////////////////////////////////
void OrbitalViewerBase::adjustL(int newN)
/// Adjusts the value of the orbital quantum number L to lie
/// in the region 1 - N-1.
{
  options->SpinBoxL->setMaxValue(newN - 1);
}

///// adjustM /////////////////////////////////////////////////////////////////
void OrbitalViewerBase::adjustM(int newL)
/// Adjusts the value of the angular momentum quantum number M
/// to lie in the region -L - +L.
{
  options->SpinBoxM->setMinValue(-newL);
  options->SpinBoxM->setMaxValue(newL);
}

///// updateColors ///////////////////////////////////////////////////////////
void OrbitalViewerBase::updateColors()
/// Updates the colors for drawing the positive and negative phases of the orbitals
{
  view->updateColors(options->ColorButtonPositive->color(), options->ColorButtonNegative->color());
}

///// updateTypeOptions ///////////////////////////////////////////////////////
void OrbitalViewerBase::updateTypeOptions(int type)
/// Updates the options to correspond to the chosen type of visualisation.
{
  switch(type)
  {
    case OrbitalThread::IsoProbability:
            options->LabelResolution->setEnabled(true);
            options->SliderResolution->setEnabled(true);
            options->LabelProbability->setEnabled(true);
            options->LineEditProbability->setEnabled(true);
            options->LabelDots->setEnabled(false);
            options->SliderDots->setEnabled(false);
            options->SpinBoxDots->setEnabled(false);
            if(options->LineEditProbability->text().toFloat() > 0.1f) 
              options->LineEditProbability->setText("0.0001");            
            break;
    case OrbitalThread::AccumulatedProbability:
            options->LabelResolution->setEnabled(true);
            options->SliderResolution->setEnabled(true);
            options->LabelProbability->setEnabled(true);
            options->LineEditProbability->setEnabled(true);
            options->LabelDots->setEnabled(false);
            options->SliderDots->setEnabled(false);
            options->SpinBoxDots->setEnabled(false);
            if(options->LineEditProbability->text().toFloat() < 0.1f)
            options->LineEditProbability->setText("0.95");
            break;
    case OrbitalThread::Density:
            options->LabelResolution->setEnabled(false);
            options->SliderResolution->setEnabled(false);
            options->LabelProbability->setEnabled(false);
            options->LineEditProbability->setEnabled(false);
            options->LabelDots->setEnabled(true);
            options->SliderDots->setEnabled(true);
            options->SpinBoxDots->setEnabled(true);
            break;
    case OrbitalThread::RadialPart:
            options->LabelResolution->setEnabled(true);
            options->SliderResolution->setEnabled(true);
            options->LabelProbability->setEnabled(false);
            options->LineEditProbability->setEnabled(false);
            options->LabelDots->setEnabled(false);
            options->SliderDots->setEnabled(false);
            options->SpinBoxDots->setEnabled(false);
            break;
    case OrbitalThread::AngularPart:
            options->LabelResolution->setEnabled(true);
            options->SliderResolution->setEnabled(true);
            options->LabelProbability->setEnabled(false);
            options->LineEditProbability->setEnabled(false);
            options->LabelDots->setEnabled(false);
            options->SliderDots->setEnabled(false);
            options->SpinBoxDots->setEnabled(false);
  }
}

///// cancelCalculation ///////////////////////////////////////////////////////
void OrbitalViewerBase::cancelCalculation()
/// Stops a running calculation.
{
  if(calcThread == 0)
    return;

  calcThread->stop();
}

///////////////////////////////////////////////////////////////////////////////
///// Private Member Functions                                            /////
///////////////////////////////////////////////////////////////////////////////

///// finishCalculation ///////////////////////////////////////////////////////
void OrbitalViewerBase::finishCalculation()
/// Cleans up after a calculation was finished.
{
  ///// clean up the thread
  if(calcThread == 0)
    return;

  if(!calcThread->finished())
    calcThread->wait(); // blocking wait

  view->setMaximumRadius(calcThread->boundingSphereRadius()); // this forces a zoomfit and a redraw

  delete calcThread;
  calcThread = 0;

  ///// update the widget
  options->ToolButtonCancel->setEnabled(false);
  options->ToolButtonUpdate->setEnabled(true);     
  options->ProgressBar->setProgress(0);

  ///// stop auto-updating
  timer->stop();
}

