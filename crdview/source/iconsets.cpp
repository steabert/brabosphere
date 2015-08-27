/***************************************************************************
                         iconsets.cpp  -  description
                             -------------------
    begin                : Tue Oct 18 2005
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
  This is a simpler version of Brabosphere's implementation.
*/

/// \file
/// Contains the implementation of the class IconSets.

///// Header files ////////////////////////////////////////////////////////////

// Qt header files
#include <qimage.h>
#include <qmime.h>

// CrdView header files
#include "iconsets.h"
#include "icons-shared.h"

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

  if(id == Open)
  {
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(OpenNormal)), QIconSet::Automatic);
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
  else if(id == Prefs)
  {
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(PrefsNormal)), QIconSet::Automatic);
    result.setPixmap(QPixmap::fromMimeSource(factoryNames.at(PrefsActive)), QIconSet::Large, QIconSet::Active);
    result.setPixmap(QPixmap(QPixmap::fromMimeSource(factoryNames.at(PrefsActive)).convertToImage().smoothScale(22, 22)), QIconSet::Small, QIconSet::Active);
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

  return result;
}

///// factoryName /////////////////////////////////////////////////////////////
QString IconSets::factoryName(const IconSetID id)
/// Returns the string with which the default version ('normal'version) of the icon set 
/// can be accessed through the default QMimeSourceFactory.
{
  return iconNames[id];
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
  // Open
  initPixmap((unsigned char*)open_normal_data, 32, 32, true, OpenNormal, "OpenNormal", Open);
  initPixmap((unsigned char*)open_active_data, 32, 32, true, OpenActive, "OpenActive");
  // Save
  initPixmap((unsigned char*)save_normal_data, 32, 32, true, SaveNormal, "SaveNormal", Save);
  initPixmap((unsigned char*)save_disabled_data, 32, 32, true, SaveDisabled, "SaveDisabled");
  initPixmap((unsigned char*)save_active_data, 32, 32, true, SaveActive, "SaveActive");
  // Prefs
  initPixmap((unsigned char*)prefs_normal_data, 32, 32, true, PrefsNormal, "PrefsNormal", Prefs);
  initPixmap((unsigned char*)prefs_active_data, 32, 32, true, PrefsActive, "PrefsActive");
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

