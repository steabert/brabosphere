/***************************************************************************
                         iconsets.cpp  -  description
                             -------------------
    begin                : Thu Jun 23 2005
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
  \class IconSets
  \brief Stores all icons used in the program.

  It is a utility class composed of static members only. Upon the first invocation
  all icon sets are initialized for retrieval in various parts of the program.
  Additionally it can store other pixmaps. All data are to be retrieved
  using enums.
*/

/// \file 
/// Contains the implementation of the class IconSets.

///// Header files ////////////////////////////////////////////////////////////

// Qt header files
#include <qbitmap.h>
#include <qimage.h>
#include <qmime.h>

// Xbrabo header files
#include "iconsets.h"

// Xbrabo pixmap header files
#include "background.h"
#include "icons.h"
#include "icons-shared.h"
#include "pixmaps.h"
#include "splash.h"

///////////////////////////////////////////////////////////////////////////////
///// Public Member Functions                                             /////
///////////////////////////////////////////////////////////////////////////////

///// Destructor //////////////////////////////////////////////////////////////
IconSets::~IconSets()
/// The default destructor 
{
}

///// getIconSet //////////////////////////////////////////////////////////////
QIconSet IconSets::getIconSet(const IconSetID id)
/// Returns the icon set associated with the given id.
{
  initialize();
  QIconSet result;

  if(id == New)
  {
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(NewNormal)), QIconSet::Automatic, QIconSet::Normal);
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(NewDisabled)), QIconSet::Automatic, QIconSet::Disabled);
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(NewActive)), QIconSet::Large, QIconSet::Active);
    result.setPixmap(QPixmap(QPixmap::fromMimeSource(factoryNames.at(NewActive)).convertToImage().smoothScale(22, 22)), QIconSet::Small, QIconSet::Active);
  }
  else if(id == Open)
  {
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(OpenNormal)), QIconSet::Automatic, QIconSet::Normal);
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(OpenDisabled)), QIconSet::Automatic, QIconSet::Disabled);
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(OpenActive)), QIconSet::Large, QIconSet::Active);
    result.setPixmap(QPixmap(QPixmap::fromMimeSource(factoryNames.at(OpenActive)).convertToImage().smoothScale(22, 22)), QIconSet::Small, QIconSet::Active);
  }
  else if(id == Save)
  {
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(SaveNormal)), QIconSet::Automatic, QIconSet::Normal);
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(SaveDisabled)), QIconSet::Automatic, QIconSet::Disabled);
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(SaveActive)), QIconSet::Large, QIconSet::Active);
    result.setPixmap(QPixmap(QPixmap::fromMimeSource(factoryNames.at(SaveActive)).convertToImage().smoothScale(22, 22)), QIconSet::Small, QIconSet::Active);
  }
  else if(id == Cut)
  {
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(CutNormal)), QIconSet::Automatic, QIconSet::Normal);
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(CutDisabled)), QIconSet::Automatic, QIconSet::Disabled);
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(CutActive)), QIconSet::Large, QIconSet::Active);
    result.setPixmap(QPixmap(QPixmap::fromMimeSource(factoryNames.at(CutActive)).convertToImage().smoothScale(22, 22)), QIconSet::Small, QIconSet::Active);
  }
  else if(id == Copy)
  {
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(CopyNormal)), QIconSet::Automatic, QIconSet::Normal);
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(CopyDisabled)), QIconSet::Automatic, QIconSet::Disabled);
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(CopyActive)), QIconSet::Large, QIconSet::Active);
    result.setPixmap(QPixmap(QPixmap::fromMimeSource(factoryNames.at(CopyActive)).convertToImage().smoothScale(22, 22)), QIconSet::Small, QIconSet::Active);
  }
  else if(id == Paste)
  {
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(PasteNormal)), QIconSet::Automatic, QIconSet::Normal);
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(PasteDisabled)), QIconSet::Automatic, QIconSet::Disabled);
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(PasteActive)), QIconSet::Large, QIconSet::Active);
    result.setPixmap(QPixmap(QPixmap::fromMimeSource(factoryNames.at(PasteActive)).convertToImage().smoothScale(22, 22)), QIconSet::Small, QIconSet::Active);
  }
  else if(id == Prefs)
  {
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(PrefsNormal)), QIconSet::Automatic);
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(PrefsActive)), QIconSet::Large, QIconSet::Active);
    result.setPixmap(QPixmap(QPixmap::fromMimeSource(factoryNames.at(PrefsActive)).convertToImage().smoothScale(22, 22)), QIconSet::Small, QIconSet::Active);
  }
  else if(id == MoleculeRead)
  {
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(MoleculeReadNormal)), QIconSet::Automatic, QIconSet::Normal);
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(MoleculeReadDisabled)), QIconSet::Automatic, QIconSet::Disabled);
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(MoleculeReadActive)), QIconSet::Large, QIconSet::Active);
    result.setPixmap(QPixmap(QPixmap::fromMimeSource(factoryNames.at(MoleculeReadActive)).convertToImage().smoothScale(22, 22)), QIconSet::Small, QIconSet::Active);
  }
  else if(id == MoleculeAnimate)
  {
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(MoleculeAnimateNormal)), QIconSet::Automatic, QIconSet::Normal);
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(MoleculeAnimateDisabled)), QIconSet::Automatic, QIconSet::Disabled);
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(MoleculeAnimateActive)), QIconSet::Large, QIconSet::Active);
    result.setPixmap(QPixmap(QPixmap::fromMimeSource(factoryNames.at(MoleculeAnimateActive)).convertToImage().smoothScale(22, 22)), QIconSet::Small, QIconSet::Active);
  }
  else if(id == MoleculeSave)
  {
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(MoleculeSaveNormal)), QIconSet::Automatic, QIconSet::Normal);
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(MoleculeSaveDisabled)), QIconSet::Automatic, QIconSet::Disabled);
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(MoleculeSaveActive)), QIconSet::Large, QIconSet::Active);
    result.setPixmap(QPixmap(QPixmap::fromMimeSource(factoryNames.at(MoleculeSaveActive)).convertToImage().smoothScale(22, 22)), QIconSet::Small, QIconSet::Active);
  }
  else if(id == MoleculeSelection)
  {
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(MoleculeSelectionNormal)), QIconSet::Automatic, QIconSet::Normal);
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(MoleculeSelectionDisabled)), QIconSet::Automatic, QIconSet::Disabled);
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(MoleculeSelectionActive)), QIconSet::Large, QIconSet::Active);
    result.setPixmap(QPixmap(QPixmap::fromMimeSource(factoryNames.at(MoleculeSelectionActive)).convertToImage().smoothScale(22, 22)), QIconSet::Small, QIconSet::Active);
  }
  else if(id == SetupGlobal)
  {
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(SetupGlobalNormal)), QIconSet::Automatic, QIconSet::Normal);
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(SetupGlobalDisabled)), QIconSet::Automatic, QIconSet::Disabled);
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(SetupGlobalActive)), QIconSet::Large, QIconSet::Active);
    result.setPixmap(QPixmap(QPixmap::fromMimeSource(factoryNames.at(SetupGlobalActive)).convertToImage().smoothScale(22, 22)), QIconSet::Small, QIconSet::Active);
  }
  else if(id == SetupBrabo)
  {
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(SetupBraboNormal)), QIconSet::Automatic, QIconSet::Normal);
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(SetupBraboDisabled)), QIconSet::Automatic, QIconSet::Disabled);
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(SetupBraboActive)), QIconSet::Large, QIconSet::Active);
    result.setPixmap(QPixmap(QPixmap::fromMimeSource(factoryNames.at(SetupBraboActive)).convertToImage().smoothScale(22, 22)), QIconSet::Small, QIconSet::Active);
  }
  else if(id == SetupRelax)
  {
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(SetupRelaxNormal)), QIconSet::Automatic, QIconSet::Normal);
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(SetupRelaxDisabled)), QIconSet::Automatic, QIconSet::Disabled);
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(SetupRelaxActive)), QIconSet::Large, QIconSet::Active);
    result.setPixmap(QPixmap(QPixmap::fromMimeSource(factoryNames.at(SetupRelaxActive)).convertToImage().smoothScale(22, 22)), QIconSet::Small, QIconSet::Active);
  }
  else if(id == SetupFreq)
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(SetupFreqNormal)), QIconSet::Automatic);
  else if(id == SetupBuur)
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(SetupBuurNormal)), QIconSet::Automatic);
  else if(id == Start)
  {
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(StartNormal)), QIconSet::Automatic, QIconSet::Normal);
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(StartDisabled)), QIconSet::Automatic, QIconSet::Disabled);
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(StartActive)), QIconSet::Large, QIconSet::Active);
    result.setPixmap(QPixmap(QPixmap::fromMimeSource(factoryNames.at(StartActive)).convertToImage().smoothScale(22, 22)), QIconSet::Small, QIconSet::Active);
  }
  else if(id == Pause)
  {
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(PauseNormal)), QIconSet::Automatic, QIconSet::Normal);
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(PauseDisabled)), QIconSet::Automatic, QIconSet::Disabled);
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(PauseActive)), QIconSet::Large, QIconSet::Active);
    result.setPixmap(QPixmap(QPixmap::fromMimeSource(factoryNames.at(PauseActive)).convertToImage().smoothScale(22, 22)), QIconSet::Small, QIconSet::Active);
  }
  else if(id == Stop)
  {
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(StopNormal)), QIconSet::Automatic, QIconSet::Normal);
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(StopDisabled)), QIconSet::Automatic, QIconSet::Disabled);
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(StopActive)), QIconSet::Large, QIconSet::Active);
    result.setPixmap(QPixmap(QPixmap::fromMimeSource(factoryNames.at(StopActive)).convertToImage().smoothScale(22, 22)), QIconSet::Small, QIconSet::Active);
  }
  else if(id == Write)
  {
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(WriteNormal)), QIconSet::Automatic, QIconSet::Normal);
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(WriteDisabled)), QIconSet::Automatic, QIconSet::Disabled);
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(WriteActive)), QIconSet::Large, QIconSet::Active);
    result.setPixmap(QPixmap(QPixmap::fromMimeSource(factoryNames.at(WriteActive)).convertToImage().smoothScale(22, 22)), QIconSet::Small, QIconSet::Active);
  }
  else if(id == Clean)
  {
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(CleanNormal)), QIconSet::Automatic, QIconSet::Normal);
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(CleanDisabled)), QIconSet::Automatic, QIconSet::Disabled);
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(CleanActive)), QIconSet::Large, QIconSet::Active);
    result.setPixmap(QPixmap(QPixmap::fromMimeSource(factoryNames.at(CleanActive)).convertToImage().smoothScale(22, 22)), QIconSet::Small, QIconSet::Active);
  }
  else if(id == Outputs)
  {
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(OutputsNormal)), QIconSet::Automatic, QIconSet::Normal);
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(OutputsDisabled)), QIconSet::Automatic, QIconSet::Disabled);
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(OutputsActive)), QIconSet::Large, QIconSet::Active);
    result.setPixmap(QPixmap(QPixmap::fromMimeSource(factoryNames.at(OutputsActive)).convertToImage().smoothScale(22, 22)), QIconSet::Small, QIconSet::Active);
  }
  else if(id == ArrowLeft)
  {
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(ArrowLeftNormal)), QIconSet::Automatic, QIconSet::Normal);
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(ArrowLeftDisabled)), QIconSet::Automatic, QIconSet::Disabled);
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(ArrowLeftActive)), QIconSet::Large, QIconSet::Active);
    result.setPixmap(QPixmap(QPixmap::fromMimeSource(factoryNames.at(ArrowLeftActive)).convertToImage().smoothScale(22, 22)), QIconSet::Small, QIconSet::Active);
  }
  else if(id == ArrowRight)
  {
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(ArrowRightNormal)), QIconSet::Automatic, QIconSet::Normal);
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(ArrowRightDisabled)), QIconSet::Automatic, QIconSet::Disabled);
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(ArrowRightActive)), QIconSet::Large, QIconSet::Active);
    result.setPixmap(QPixmap(QPixmap::fromMimeSource(factoryNames.at(ArrowRightActive)).convertToImage().smoothScale(22, 22)), QIconSet::Small, QIconSet::Active);
  }
  else if(id == ArrowUp)
  {
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(ArrowUpNormal)), QIconSet::Automatic, QIconSet::Normal);
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(ArrowUpDisabled)), QIconSet::Automatic, QIconSet::Disabled);
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(ArrowUpActive)), QIconSet::Large, QIconSet::Active);
    result.setPixmap(QPixmap(QPixmap::fromMimeSource(factoryNames.at(ArrowUpActive)).convertToImage().smoothScale(22, 22)), QIconSet::Small, QIconSet::Active);
  }
  else if(id == ArrowDown)
  {
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(ArrowDownNormal)), QIconSet::Automatic, QIconSet::Normal);
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(ArrowDownDisabled)), QIconSet::Automatic, QIconSet::Disabled);
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(ArrowDownActive)), QIconSet::Large, QIconSet::Active);
    result.setPixmap(QPixmap(QPixmap::fromMimeSource(factoryNames.at(ArrowDownActive)).convertToImage().smoothScale(22, 22)), QIconSet::Small, QIconSet::Active);
  }
  else if(id == Clear)
  {
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(ClearNormal)), QIconSet::Automatic, QIconSet::Normal);
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(ClearDisabled)), QIconSet::Automatic, QIconSet::Disabled);
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(ClearActive)), QIconSet::Large, QIconSet::Active);
    result.setPixmap(QPixmap(QPixmap::fromMimeSource(factoryNames.at(ClearActive)).convertToImage().smoothScale(22, 22)), QIconSet::Small, QIconSet::Active);
 }
  else if(id == Image)
  {
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(ImageNormal)), QIconSet::Automatic, QIconSet::Normal);
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(ImageDisabled)), QIconSet::Automatic, QIconSet::Disabled);
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(ImageActive)), QIconSet::Large, QIconSet::Active);
    result.setPixmap(QPixmap(QPixmap::fromMimeSource(factoryNames.at(ImageActive)).convertToImage().smoothScale(22, 22)), QIconSet::Small, QIconSet::Active);
  }
  else if(id == Help)
  {
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(HelpNormal)), QIconSet::Automatic);
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(HelpActive)), QIconSet::Large, QIconSet::Active);
    result.setPixmap(QPixmap(QPixmap::fromMimeSource(factoryNames.at(HelpActive)).convertToImage().smoothScale(22, 22)), QIconSet::Small, QIconSet::Active);
  }
  else if(id == WhatsThis)
  {
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(WhatsThisNormal)), QIconSet::Automatic);
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(WhatsThisActive)), QIconSet::Large, QIconSet::Active);
    result.setPixmap(QPixmap(QPixmap::fromMimeSource(factoryNames.at(WhatsThisActive)).convertToImage().smoothScale(22, 22)), QIconSet::Small, QIconSet::Active);
  }
  else if(id == NewPart)
  {
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(NewPartNormal)), QIconSet::Automatic, QIconSet::Normal);
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(NewPartDisabled)), QIconSet::Automatic, QIconSet::Disabled);
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(NewPartActive)), QIconSet::Large, QIconSet::Active);
    result.setPixmap(QPixmap(QPixmap::fromMimeSource(factoryNames.at(NewPartActive)).convertToImage().smoothScale(22, 22)), QIconSet::Small, QIconSet::Active);
  }
  else if(id == Generate)
  {
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(GenerateNormal)), QIconSet::Automatic, QIconSet::Normal);
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(GenerateDisabled)), QIconSet::Automatic, QIconSet::Disabled);
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(GenerateActive)), QIconSet::Large, QIconSet::Active);
    result.setPixmap(QPixmap(QPixmap::fromMimeSource(factoryNames.at(GenerateActive)).convertToImage().smoothScale(22, 22)), QIconSet::Small, QIconSet::Active);
  }
  
  return result;
}

///// factoryName /////////////////////////////////////////////////////////////
QString IconSets::factoryName(const IconSetID id)
/// Returns the string with which the default version ('normal'version) of the icon set 
/// can be accessed through the default QMimeSourceFactory.
{
  return iconNames[id];
}

///// getPixmap ///////////////////////////////////////////////////////////////
QPixmap IconSets::getPixmap(const PixmapID id)
/// Returns the pixmap associated with the given id.
{
  initialize(); 
  return QPixmap::fromMimeSource(factoryNames.at(id + Background2));   
}

///// getSplash ///////////////////////////////////////////////////////////////
QPixmap IconSets::getSplash()
/// Returns the pixmap used for the splash screen. It is not stored in the 
/// QMimeSourceFactory as it should load quickly on startup.
{
  // recreate the image
  QImage image((unsigned char*)splash_data, 567, 386, 32, 0, 0, QImage::BigEndian);
  image.setAlphaBuffer(true);

  // create a pixmap with a mask
  QPixmap result;
  result.convertFromImage(image);
  if(!result.mask())
  {
    QBitmap bm;
    if(image.hasAlphaBuffer())
      bm = image.createAlphaMask();
    else
      bm = image.createHeuristicMask();
    result.setMask(bm);
  }
  return result;
}

///////////////////////////////////////////////////////////////////////////////
///// Private Member Functions                                            /////
///////////////////////////////////////////////////////////////////////////////

///// Constructor /////////////////////////////////////////////////////////////
IconSets::IconSets()
/// The default constructor. Made private to inhibit instantiations of this class.
{
}

///// Initialize //////////////////////////////////////////////////////////////
void IconSets::initialize()
/// Initializes the static data of the class. In practice it fills the default
/// QMimeSourceFactory with data from the included qembed'ed files.
/// QIconSet's are constructed from these data when requested. This allows the
/// factory to be filed with different versions of an icon.
{
  if(isInitialized)
    return;
  isInitialized = true;

  ///// IconSet pixmaps
  // New
  initPixmap((unsigned char*)new_normal_data, 32, 32, true, NewNormal, "NewNormal", New);
  initPixmap((unsigned char*)new_disabled_data, 32, 32, true, NewDisabled, "NewDisabled");
  initPixmap((unsigned char*)new_active_data, 32, 32, true, NewActive, "NewActive");
  // Open
  initPixmap((unsigned char*)open_normal_data, 32, 32, true, OpenNormal, "OpenNormal", Open);
  initPixmap((unsigned char*)open_disabled_data, 32, 32, true, OpenDisabled, "OpenDisabled");
  initPixmap((unsigned char*)open_active_data, 32, 32, true, OpenActive, "OpenActive");
  // Save
  initPixmap((unsigned char*)save_normal_data, 32, 32, true, SaveNormal, "SaveNormal", Save);
  initPixmap((unsigned char*)save_disabled_data, 32, 32, true, SaveDisabled, "SaveDisabled");
  initPixmap((unsigned char*)save_active_data, 32, 32, true, SaveActive, "SaveActive");
  // Cut
  initPixmap((unsigned char*)cut_normal_data, 32, 32, true, CutNormal, "CutNormal", Cut);
  initPixmap((unsigned char*)cut_disabled_data, 32, 32, true, CutDisabled, "CutDisabled");
  initPixmap((unsigned char*)cut_active_data, 32, 32, true, CutActive, "CutActive");
  // Copy
  initPixmap((unsigned char*)copy_normal_data, 32, 32, true, CopyNormal, "CopyNormal", Copy);
  initPixmap((unsigned char*)copy_disabled_data, 32, 32, true, CopyDisabled, "CopyDisabled");
  initPixmap((unsigned char*)copy_active_data, 32, 32, true, CopyActive, "CopyActive");
  // Paste
  initPixmap((unsigned char*)paste_normal_data, 32, 32, true, PasteNormal, "PasteNormal", Paste);
  initPixmap((unsigned char*)paste_disabled_data, 32, 32, true, PasteDisabled, "PasteDisabled");
  initPixmap((unsigned char*)paste_active_data, 32, 32, true, PasteActive, "PasteActive");
  // Prefs
  initPixmap((unsigned char*)prefs_normal_data, 32, 32, true, PrefsNormal, "PrefsNormal", Prefs);
  initPixmap((unsigned char*)prefs_active_data, 32, 32, true, PrefsActive, "PrefsActive");
  // MoleculeRead
  initPixmap((unsigned char*)molecule_read_normal_data, 32, 32, true, MoleculeReadNormal, "MoleculeReadNormal", MoleculeRead);
  initPixmap((unsigned char*)molecule_read_disabled_data, 32, 32, true, MoleculeReadDisabled, "MoleculeReadDisabled");
  initPixmap((unsigned char*)molecule_read_active_data, 32, 32, true, MoleculeReadActive, "MoleculeReadActive");
  // MoleculeAnimate
  initPixmap((unsigned char*)molecule_animate_normal_data, 32, 32, true, MoleculeAnimateNormal, "MoleculeAnimateNormal", MoleculeAnimate);
  initPixmap((unsigned char*)molecule_animate_disabled_data, 32, 32, true, MoleculeAnimateDisabled, "MoleculeAnimateDisabled");
  initPixmap((unsigned char*)molecule_animate_active_data, 32, 32, true, MoleculeAnimateActive, "MoleculeAnimateActive");
  // MoleculeSave
  initPixmap((unsigned char*)molecule_save_normal_data, 32, 32, true, MoleculeSaveNormal, "MoleculeSaveNormal", MoleculeSave);
  initPixmap((unsigned char*)molecule_save_disabled_data, 32, 32, true, MoleculeSaveDisabled, "MoleculeSaveDisabled");
  initPixmap((unsigned char*)molecule_save_active_data, 32, 32, true, MoleculeSaveActive, "MoleculeSaveActive");
  // MoleculeSelection
  initPixmap((unsigned char*)selection_normal_data, 32, 32, true, MoleculeSelectionNormal, "MoleculeSelectionNormal", MoleculeSelection);
  initPixmap((unsigned char*)selection_disabled_data, 32, 32, true, MoleculeSelectionDisabled, "MoleculeSelectionDisabled");
  initPixmap((unsigned char*)selection_active_data, 32, 32, true, MoleculeSelectionActive, "MoleculeSelectionActive");
  // SetupGlobal
  initPixmap((unsigned char*)setup_global_normal_data, 32, 32, true, SetupGlobalNormal, "SetupGlobalNormal", SetupGlobal);
  initPixmap((unsigned char*)setup_global_disabled_data, 32, 32, true, SetupGlobalDisabled, "SetupGlobalDisabled");
  initPixmap((unsigned char*)setup_global_active_data, 32, 32, true, SetupGlobalActive, "SetupGlobalActive");
  // SetupBrabo
  initPixmap((unsigned char*)setup_brabo_normal_data, 32, 32, true, SetupBraboNormal, "SetupBraboNormal", SetupBrabo);
  initPixmap((unsigned char*)setup_brabo_disabled_data, 32, 32, true, SetupBraboDisabled, "SetupBraboDisabled");
  initPixmap((unsigned char*)setup_brabo_active_data, 32, 32, true, SetupBraboActive, "SetupBraboActive");
  // SetupRelax
  initPixmap((unsigned char*)setup_relax_normal_data, 32, 32, true, SetupRelaxNormal, "SetupRelaxNormal", SetupRelax);
  initPixmap((unsigned char*)setup_relax_disabled_data, 32, 32, true, SetupRelaxDisabled, "SetupRelaxDisabled");
  initPixmap((unsigned char*)setup_relax_active_data, 32, 32, true, SetupRelaxActive, "SetupRelaxActive");
  // SetupFreq
  initPixmap((unsigned char*)setup_freq_data, 22, 22, true, SetupFreqNormal, "SetupFreqNormal");
  // SetupBuur
  initPixmap((unsigned char*)setup_buur_data, 22, 22, true, SetupBuurNormal, "SetupBuurNormal");
  // Start
  initPixmap((unsigned char*)start_normal_data, 32, 32, true, StartNormal, "StartNormal", Start);
  initPixmap((unsigned char*)start_disabled_data, 32, 32, true, StartDisabled, "StartDisabled");
  initPixmap((unsigned char*)start_active_data, 32, 32, true, StartActive, "StartActive");
  // Pause
  initPixmap((unsigned char*)pause_normal_data, 32, 32, true, PauseNormal, "PauseNormal", Pause);
  initPixmap((unsigned char*)pause_disabled_data, 32, 32, true, PauseDisabled, "PauseDisabled");
  initPixmap((unsigned char*)pause_active_data, 32, 32, true, PauseActive, "PauseActive");
  // Stop
  initPixmap((unsigned char*)stop_normal_data, 32, 32, true, StopNormal, "StopNormal", Stop);
  initPixmap((unsigned char*)stop_disabled_data, 32, 32, true, StopDisabled, "StopDisabled");
  initPixmap((unsigned char*)stop_active_data, 32, 32, true, StopActive, "StopActive");
  // Write
  initPixmap((unsigned char*)write_normal_data, 32, 32, true, WriteNormal, "WriteNormal", Write);
  initPixmap((unsigned char*)write_disabled_data, 32, 32, true, WriteDisabled, "WriteDisabled");
  initPixmap((unsigned char*)write_active_data, 32, 32, true, WriteActive, "WriteActive");
  // Clean
  initPixmap((unsigned char*)clean_normal_data, 32, 32, true, CleanNormal, "CleanNormal", Clean);
  initPixmap((unsigned char*)clean_disabled_data, 32, 32, true, CleanDisabled, "CleanDisabled");
  initPixmap((unsigned char*)clean_active_data, 32, 32, true, CleanActive, "CleanActive");
  // Outputs
  initPixmap((unsigned char*)outputs_normal_data, 32, 32, true, OutputsNormal, "OutputsNormal", Outputs);
  initPixmap((unsigned char*)outputs_disabled_data, 32, 32, true, OutputsDisabled, "OutputsDisabled");
  initPixmap((unsigned char*)outputs_active_data, 32, 32, true, OutputsActive, "OutputsActive");
  // Arrow Left
  initPixmap((unsigned char*)arrow_left_normal_data, 32, 32, true, ArrowLeftNormal, "ArrowLeftNormal", ArrowLeft);
  initPixmap((unsigned char*)arrow_left_disabled_data, 32, 32, true, ArrowLeftDisabled, "ArrowLeftDisabled");
  initPixmap((unsigned char*)arrow_left_active_data, 32, 32, true, ArrowLeftActive, "ArrowLeftActive");
  // Arrow Right
  initPixmap((unsigned char*)arrow_right_normal_data, 32, 32, true, ArrowRightNormal, "ArrowRightNormal", ArrowRight);
  initPixmap((unsigned char*)arrow_right_disabled_data, 32, 32, true, ArrowRightDisabled, "ArrowRightDisabled");
  initPixmap((unsigned char*)arrow_right_active_data, 32, 32, true, ArrowRightActive, "ArrowRightActive");
  // Arrow Up
  initPixmap((unsigned char*)arrow_up_normal_data, 32, 32, true, ArrowUpNormal, "ArrowUpNormal", ArrowUp);
  initPixmap((unsigned char*)arrow_up_disabled_data, 32, 32, true, ArrowUpDisabled, "ArrowUpDisabled");
  initPixmap((unsigned char*)arrow_up_active_data, 32, 32, true, ArrowUpActive, "ArrowUpActive");
  // Arrow Down
  initPixmap((unsigned char*)arrow_down_normal_data, 32, 32, true, ArrowDownNormal, "ArrowDownNormal", ArrowDown);
  initPixmap((unsigned char*)arrow_down_disabled_data, 32, 32, true, ArrowDownDisabled, "ArrowDownDisabled");
  initPixmap((unsigned char*)arrow_down_active_data, 32, 32, true, ArrowDownActive, "ArrowDownActive");
  // Clear
  initPixmap((unsigned char*)clear_normal_data, 32, 32, true, ClearNormal, "ClearNormal", Clear);
  initPixmap((unsigned char*)clear_disabled_data, 32, 32, true, ClearDisabled, "ClearDisabled");
  initPixmap((unsigned char*)clear_active_data, 32, 32, true, ClearActive, "ClearActive");
  // Image
  initPixmap((unsigned char*)image_normal_data, 32, 32, true, ImageNormal, "ImageNormal", Image);
  initPixmap((unsigned char*)image_disabled_data, 32, 32, true, ImageDisabled, "ImageDisabled");
  initPixmap((unsigned char*)image_active_data, 32, 32, true, ImageActive, "ImageActive");
  // Help
  initPixmap((unsigned char*)help_normal_data, 32, 32, true, HelpNormal, "HelpNormal", Help);
  initPixmap((unsigned char*)help_active_data, 32, 32, true, HelpActive, "HelpActive");
  // What's This
  initPixmap((unsigned char*)whatsthis_normal_data, 32, 32, true, WhatsThisNormal, "WhatsThisNormal", WhatsThis);
  initPixmap((unsigned char*)whatsthis_active_data, 32, 32, true, WhatsThisActive, "WhatsThisActive");
  // New Part
  initPixmap((unsigned char*)newpart_normal_data, 32, 32, true, NewPartNormal, "NewPartNormal", NewPart);
  initPixmap((unsigned char*)newpart_disabled_data, 32, 32, true, NewPartDisabled, "NewPartDisabled");
  initPixmap((unsigned char*)newpart_active_data, 32, 32, true, NewPartActive, "NewPartActive");
  // Generate
  initPixmap((unsigned char*)generate_normal_data, 32, 32, true, GenerateNormal, "GenerateNormal", Generate);
  initPixmap((unsigned char*)generate_disabled_data, 32, 32, true, GenerateDisabled, "GenerateDisabled");
  initPixmap((unsigned char*)generate_active_data, 32, 32, true, GenerateActive, "GenerateActive");

  ///// Other pixmaps
  // Background
  initPixmap((unsigned char*)background_data, 512, 512, false, Background2, "Background");
  // Symmetry operation C2x
  initPixmap((unsigned char*)symm_c2x_data, 96, 96, true, SymmYZ2, "SymmYZ");
  // Symmetry operation C2y
  initPixmap((unsigned char*)symm_c2y_data, 96, 96, true, SymmXZ2, "SymmXZ");
  // Symmetry operation C2z
  initPixmap((unsigned char*)symm_c2z_data, 96, 96, true, SymmXY2, "SymmXY");
  // Symmetry operation SigmaXY
  initPixmap((unsigned char*)symm_sxy_data, 96, 96, true, SymmZ2, "SymmZ");
  // Symmetry operation SigmaXZ
  initPixmap((unsigned char*)symm_sxz_data, 96, 96, true, SymmY2, "SymmY");
  // Symmetry operation SigmaYZ
  initPixmap((unsigned char*)symm_syz_data, 96, 96, true, SymmX2, "SymmX");
  // Symmetry operation i
  initPixmap((unsigned char*)symm_i_data, 96, 96, true, SymmXYZ2, "SymmXYZ");
  // Stretch
  initPixmap((unsigned char*)stretch_data, 96, 96, true, Stretch2, "Stretch");
  // Bend
  initPixmap((unsigned char*)bend_data, 96, 96, true, Bend2, "Bend"); 
  // Torsion
  initPixmap((unsigned char*)torsion_data, 96, 96, true, Torsion2, "Torsion");
  // Out Of Plane
  initPixmap((unsigned char*)outofplane_data, 96, 96, true, OutOfPlane2, "OutOfPlane");
  // Linear Bend
  initPixmap((unsigned char*)linearbend_data, 96, 96, true, LinearBend2, "LinearBend"); 
  // OK
  initPixmap((unsigned char*)ok_data, 16, 16, true, OK2, "OK");
  // BRABO
  initPixmap((unsigned char*)brabo_data, 43, 48, true, BRABO2, "BRABO");
  // Molecule
  initPixmap((unsigned char*)molecule_data, 64, 48, true, Molecule2, "Molecule");
  // Visuals
  initPixmap((unsigned char*)visuals_data, 48, 48, false, Visuals2, "Visuals");
  // OpenGL
  initPixmap((unsigned char*)opengl_data, 93, 48, true, OpenGL2, "OpenGL");
  // PVM
  initPixmap((unsigned char*)pvm_data, 81, 48, true, PVM2, "PVM");
}

///// initPixmap //////////////////////////////////////////////////////////////
void IconSets::initPixmap(unsigned char* data, const int sizeX, const int sizeY, const bool alpha, const FactoryID id, const QString idString, const IconSetID iconID)
/// Stores one pixmap in the default QMimeSourceFactory and sets up the retrieval
/// array
{
  factoryNames.at(id) = idString;
  
  QImage image(data, sizeX, sizeY, 32, 0, 0, QImage::BigEndian);
  image.setAlphaBuffer(alpha);
  QMimeSourceFactory::defaultFactory()->setPixmap(idString, QPixmap(image));

  if(iconID != LastIcon)
   iconNames.at(iconID) = idString;
}

///////////////////////////////////////////////////////////////////////////////
///// Static Variables                                                    /////
///////////////////////////////////////////////////////////////////////////////

bool IconSets::isInitialized = false; ///< Holds the initialisation status of the class.
std::vector<QString> IconSets::iconNames = std::vector<QString>(LastIcon);      ///< Holds the names of the normal versions of the icons in the default QMimeSourceFactory
std::vector<QString> IconSets::factoryNames = std::vector<QString>(LastFactory); ///< Holds the names of the pixmaps in the default QMimeSourceFactory

