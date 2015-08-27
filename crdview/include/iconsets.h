/***************************************************************************
                          iconsets.h  -  description
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

/// \file
/// Contains the declaration of the class IconSets

#ifndef ICONSETS_H
#define ICONSETS_H

///// Forward class declarations & header files ///////////////////////////////

// STL headers
#include <vector>

// Qt forward class declarations
#include <qiconset.h>
#include <qpixmap.h>
class QString;

///// class IconSets //////////////////////////////////////////////////////////
class IconSets
{
  public:
    ~IconSets();                        // default destructor

    // public enums
    enum IconSetID{Open = 0, Save, Prefs, Image, Help, WhatsThis, LastIcon};

    // static public member functions
    static QIconSet getIconSet(const IconSetID id);         // returns the iconset associated with the id
    static QString factoryName(const IconSetID id);         // returns the string with which to access the 'normal' version of the icon

  private:
    // private enums
    enum FactoryID{OpenNormal = 0, OpenActive, SaveNormal, SaveDisabled, SaveActive,
                   PrefsNormal, PrefsActive,
                   ImageNormal, ImageDisabled, ImageActive, 
                   HelpNormal, HelpActive, WhatsThisNormal, WhatsThisActive,
                   LastFactory};

    IconSets();                         // private constructor for a static class

    // private static member functions
    static void initialize();           // fills all pixmaps
    static void initPixmap(unsigned char* data, const int sizeX, const int sizeY, const bool alpha, const FactoryID id, const QString idString, const IconSetID iconID = LastIcon); // fills one pixmap

    // private static member data
    static bool isInitialized;          // indicates whether initialize() is called already
    static std::vector<QString> factoryNames;  // the names of all pixmaps stored in the QMimeSourceFactory
    static std::vector<QString> iconNames;     // the names of the normal versions of the icons stored in the QMimeSourceFactory
};

#endif

