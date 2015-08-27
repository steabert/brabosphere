/***************************************************************************
                          iconsets.h  -  description
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
    enum IconSetID{New = 0, Open, Save, 
                   Cut, Copy, Paste, Prefs,
                   MoleculeRead, MoleculeAnimate, MoleculeSave, MoleculeSelection,
                   SetupGlobal, SetupBrabo, SetupRelax, SetupFreq, SetupBuur,
                   Start, Pause, Stop, Write, Clean, Outputs,
                   ArrowLeft, ArrowRight, ArrowUp, ArrowDown,
                   Clear, Image, Help, WhatsThis,
                   NewPart, Generate,
                   LastIcon};
    enum PixmapID{Background = 0, SymmX, SymmY, SymmZ, SymmXY, SymmXZ, SymmYZ, SymmXYZ,
                  Stretch, Bend, Torsion, OutOfPlane, LinearBend, 
                  OK, BRABO, Molecule, Visuals, OpenGL, PVM};

    // static public member functions
    static QIconSet getIconSet(const IconSetID id);         // returns the iconset associated with the id
    static QString factoryName(const IconSetID id);         // returns the string with which to access the 'normal' version of the icon
    static QPixmap getPixmap(const PixmapID id);  // returns the pixmap associated with the id
    static QPixmap getSplash();         // returns the pixmap used for the splash screen and about box

  private:
    // private enums
    enum FactoryID{NewNormal = 0, NewDisabled, NewActive, OpenNormal, OpenDisabled, OpenActive, SaveNormal, SaveDisabled, SaveActive,
                   CutNormal, CutDisabled, CutActive, CopyNormal, CopyDisabled, CopyActive, 
                   PasteNormal, PasteDisabled, PasteActive, PrefsNormal, PrefsActive,
                   MoleculeReadNormal, MoleculeReadDisabled, MoleculeReadActive, 
                   MoleculeAnimateNormal, MoleculeAnimateDisabled, MoleculeAnimateActive, 
                   MoleculeSaveNormal, MoleculeSaveDisabled, MoleculeSaveActive, 
                   MoleculeSelectionNormal, MoleculeSelectionDisabled, MoleculeSelectionActive,
                   SetupGlobalNormal, SetupGlobalDisabled, SetupGlobalActive,
                   SetupBraboNormal, SetupBraboDisabled, SetupBraboActive,
                   SetupRelaxNormal, SetupRelaxDisabled, SetupRelaxActive, SetupFreqNormal, SetupBuurNormal,
                   StartNormal, StartDisabled, StartActive, PauseNormal, PauseDisabled, PauseActive, 
                   StopNormal, StopDisabled, StopActive, WriteNormal, WriteDisabled, WriteActive, 
                   CleanNormal, CleanDisabled, CleanActive, OutputsNormal, OutputsDisabled, OutputsActive,
                   ArrowLeftNormal, ArrowLeftDisabled, ArrowLeftActive, ArrowRightNormal, ArrowRightDisabled, ArrowRightActive, 
                   ArrowUpNormal, ArrowUpDisabled, ArrowUpActive, ArrowDownNormal, ArrowDownDisabled, ArrowDownActive,
                   ClearNormal, ClearDisabled, ClearActive, ImageNormal, ImageDisabled, ImageActive, 
                   HelpNormal, HelpActive, WhatsThisNormal, WhatsThisActive,
                   NewPartNormal, NewPartDisabled, NewPartActive, GenerateNormal, GenerateDisabled, GenerateActive,

                   Background2, SymmX2, SymmY2, SymmZ2, SymmXY2, SymmXZ2, SymmYZ2, SymmXYZ2,
                   Stretch2, Bend2, Torsion2, OutOfPlane2, LinearBend2, 
                   OK2, BRABO2, Molecule2, Visuals2, OpenGL2, PVM2,
                   LastFactory};

    IconSets();                         // private constructor for a static class

    // private static member functions
    static void initialize();           // fills all pixmaps
    static void initPixmap(unsigned char* data, const int sizeX, const int sizeY, const bool alpha, const FactoryID id, const QString idString, const IconSetID iconID = LastIcon); // fills one pixmap

    // private static member data
    static bool isInitialized;          //< indicates whether initialize() is called already
    static std::vector<QString> factoryNames;  // the names of all pixmaps stored in the QMimeSourceFactory
    static std::vector<QString> iconNames;     // the names of the normal versions of the icons stored in the QMimeSourceFactory
};

#endif

