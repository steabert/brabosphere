/***************************************************************************
                          relaxbase.h  -  description
                             -------------------
    begin                : Mon Jun 16 2003
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
/// Contains the declaration of the class RelaxBase.

#ifndef RELAXBASE_H
#define RELAXBASE_H

///// Forward class declarations & header files ///////////////////////////////

// C++ forward class declarations
#include <vector>
using std::vector;

// Qt forward class declarations
class QDomElement;
class QProcess;
class QString;

// Xbrabo forward class declarations
class AtomSet;

// Base class header file
#include "relaxwidget.h"


///// class RelaxBase /////////////////////////////////////////////////////////
class RelaxBase : public RelaxWidget
{
  Q_OBJECT

  public:
    ///// constructor/destructor
    RelaxBase(AtomSet* atomset, QWidget* parent = 0, const char* name = 0, bool modal = FALSE, WFlags fl = 0);// constructor
    ~RelaxBase();                       // destructor

    ///// public enums
    enum Input{AFF, MAFF};              ///< Types of input.
    
    ///// public member functions for changing data
    void setName(const QString name);   // sets the name (filename prefix)
    void setDescription(const QString description);         // sets the description
    void setDir(const QString dir);     // sets the directory
    void setExtendedFormat(const bool state);     // sets extended format for coordinates & forces

    ///// public member function for retrieving data
    unsigned int inputGenerationFrequency();      // frequency with which the internal coordinates should be regenerated
    unsigned int maxSteps();            // the limit for the number of optimization steps
    void scaleFactors(vector<unsigned int>& steps, vector<double>& factors);    // the list of scalefactors
    
    ///// public member functions for indirect IO
    QStringList generateInput(const Input type);         // generate input for a specific program
    void loadCML(const QDomElement* root);        // loads widget appearances from file
    void saveCML(QDomElement* root);    // saves widget appearances to file 

  public slots:
    void reset();                       // resets all widgets to their default values

  protected:
    void showEvent(QShowEvent* e);      // occurs each time the dialog is shown
  
  protected slots:
    void accept();                      // is called when the changes are accepted (OK clicked)
    void reject();                      // is called when the changes are rejected (Cancel or X clicked)
    
  private slots:
    ///// actions
    void showPreview();                 // initializes showing a preview of the input file
    void showPreview2();                // does the actual showing 
    void readInputFile();               // reads the contents of an existing AFF into the widgets
    ///// global
    void setChanged(bool state = true); // sets the 'changed' status of the dialog
    void selectWidget(QListViewItem*);  // shows a widget from the widgetstack depending on the listview item
    ///// basic
    void enableAutogenWidgets();                  // enabled/disables the Autogen widgets
    void enableSFACWidgets(bool state = true);    // enables/disables the SFAC widgets depending on RadioButtonSFAC{1|2}
    void newScaleFactor();                        // adds a scale factor to the list
    void deleteScaleFactor();                     // deletes a scale factor from the list
    ///// internal coordinates
    void updateICEdit();                          // updates the widgets with the contents of the selected IC
    void updateICList();                          // updates ListViewIC with the contents of the widgets
    void updateICSpecifics();                     // updates the specific widgets
    void updateICType();                          // updates the type of the internal coordinate for all parts
    void newIC();                                 // creates a new internal coordinate
    void newICPart();                             // creates a new part for an internal coordinate
    void deleteIC();                              // deletes the current internal coordinate
    void clearIC();                               // clears the IC list
    void genIC();                                 // initiates the generation all internal coordinates
    void genIC2();                                // generates all internal coordinates
    void moveICUp();                              // moves an internal coordinate up
    void moveICDown();                            // moves an internal coordinate down
    ///// cartesian coordinate - other
    void newFixedAtom();                          // adds an atom to the fixed atoms list
    void deleteFixedAtom();                       // removes an atom from the fixed atoms list
    void checkFIXABoxes();                        // makes sure at least 1 box stays checked
    void enableTROTWidgets(bool state = true);    // enables/disables the TROT widgets depending on CheckBoxTROT
    ///// advanced - other
    void newExtraBond();                          // adds a bond to the XBON list
    void deleteExtraBond();                       // removes a bond from the XBON list
    ///// debug
    void enableUPDTWidgets(bool state = true);    // enables/disables the UPDT widgets depending on CheckBoxUPDT
    void enableSHRTWidgets(bool state = true);    // enables/disables the SHRt widgets depending on CHeckBoxSHRT
    ///// extra
    void checkOverflow(int row, int col);         // spans a cell over multiple columns if needed
    void addExtraRow();                 // adds a row to Table
    void removeExtraRow();              // removes a row from Table
    void clearSelection();              // clears the selected cells in Table
    
  private:
    ///// private enums
    enum Categories{BASIC = 0, INTERNAL = 1, CARTESIAN = 2, ADVANCED = 3, DEBUG1 = 4, EXTRA = 5,
                    CARTESIAN_SYMMETRY = 6, CARTESIAN_OTHER = 7,
                    ADVANCED_THRESHOLDS = 8, ADVANCED_OTHER = 9};     ///< The list of categories.
    enum Type{PreviewAFF, ICListAFF};   ///< Types of AFF generation
                                            
    ///// private member functions
    void makeConnections();             // sets up all permanent connections
    void init();                        // initializes the dialog
    void saveWidgets();                 // saves the contents of the widgets to the data struct
    void restoreWidgets();              // restores the widgets from the data struct
    QString icRepresentation(const unsigned int atom1, const unsigned int atom2, const unsigned int atom3 = 0, const int atom4 = 0);        // builds an internal coordinate from 4 atoms
    void getAtomsFromIC(const QString textIC, unsigned int& atom1, unsigned int& atom2, unsigned int& atom3, int& atom4);         // returns the atom numbers from a textual representation of an internal coordinate
    void renumberICs();                 // renumbers all internal coordinates
    QStringList generateAFFHeader();    // generates input for the first part of an .aff file
    QStringList generateBmatInput();    // generates input for Maff for generating BMAT internal coordinates
    bool makeDirCurrent(const QString dir, const QString title);      // makes a directory current
    void fillICListFromAFF(const QStringList aff);// adds the internal coordinates found in aff to the IC list
    bool startMAFF(const unsigned int slot);      // initiates the generation of an AFF file using MAFF
    QStringList readMAFFGeneratedICs(); // reads the output from a MAFF run and cleans up

    ///// private structs
    struct ICData
    /// A utility struct containing all data to reconstruct an internal coordinate
    {
      unsigned int          number;     ///< The number in the list (only for root items).
      bool                  rootItem;   ///< = TRUE if it's a root item.
      QString               type;       ///< Denotes STRETCH|BEND|TORSION|OUT-OF-PLANE|LINEAR BEND 1|LINEAR BEND2 (only for root items).
      QString               fc;         ///< Force constant (only for root items).
      bool                  fix;        ///< = TRUE if the IC should be fixed (only for root items).
      bool                  maximize;   ///< = TRUE if the IC should be maximized (only for root items).
      QString               refValue;   ///< The reference value if any (only for root items).
      QString               weight;     ///< The weight of the partial IC.
      unsigned int          atom1;      ///< The first atom defining the partial IC.
      unsigned int          atom2;      ///< The second atom defining the partial IC.
      unsigned int          atom3;      ///< The third atom defining the partial IC.
      unsigned int          atom4;      ///< The fourth atom defining the partial IC.
    };
    struct WidgetData
    /// A struct representing the data from all widgets
    {                                                 // WIDGET                     KEYWORD(S)
      ///// Basic
      unsigned int          type;                 ///< ComboBoxType               GBMA/BMAT
      bool                  autoGenerate;         ///< CheckBoxAutogen            none
      bool                  autoRegen;            ///< CheckBoxRegen              none
      unsigned int          autoGenFreq;          ///< SpinBoxGenFreq             none
      bool                  autoHBonds;           ///< CheckBoxHBonds             none
      bool                  useOneScaleFactor;    ///< RadioButtonSFAC{1|2}       none
      QString               sfacValue1;           ///< LineEditSFAC1              none
      QString               sfacValue2;           ///< LineEditSFAC2              none
      unsigned int          sfacNumSteps;         ///< SpinBoxSFAC                none
      QStringList           listValues;           ///< ListViewSFAC               none
      vector<unsigned int>  listNumSteps;         ///< ListViewSFAC               none
      unsigned int          numDIIS;              ///< SpinBoxMAXD                MAXD
      bool                  printShifts;          ///< CheckBoxSUMM               SUMM
      //bool                  useDiagonals;         ///< CheckBoxFDIA               FDIA
      unsigned int          numSteps;             ///< SpinBoxSteps               none

      ///// Internal coordinates
      vector<ICData>        ic;                   ///< ListViewIC                 K/FIXC/MAXI/TORO

      ///// Cartesian coordinates - Symmetry
      bool                  checkSymmetry;        ///< CheckBoxSYMM               SYMM
      bool                  checkSymmX;           ///< CheckBoxSYMMx              SYMM      
      bool                  checkSymmY;           ///< CheckBoxSYMMy              SYMM
      bool                  checkSymmZ;           ///< CheckBoxSYMMz              SYMM
      bool                  checkSymmXY;          ///< CheckBoxSYMMxy             SYMM
      bool                  checkSymmXZ;          ///< CheckBoxSYMMxz             SYMM
      bool                  checkSymmYZ;          ///< CheckBoxSYMMyz             SYMM
      bool                  checkSymmXYZ;         ///< CheckBoxSYMMxyz            SYMM

      ///// Cartesian coordinates - Other
      unsigned int          fixAtom;              ///< SpinBoxFIXA                FIXA
      bool                  fixX;                 ///< CheckBoxFIXA1              FIXA
      bool                  fixY;                 ///< CheckBoxFIXA2              FIXA
      bool                  fixZ;                 ///< CheckBoxFIXA3              FIXA      
      vector<unsigned int>  listFixedAtoms;       ///< ListViewFIXA               FIXA
      QStringList           listFixedXYZ;         ///< ListViewFIXA               FIXA
      bool                  optExternal;          ///< CheckBoxTROT               TROT
      unsigned int          optExternalAngle;     ///< ComboBoxTROT               TROT
      QString               optExternalA;         ///< LineEditXTFK1              XTFK
      QString               optExternalB;         ///< LineEditXTFK2              XTFK
      QString               optExternalC;         ///< LineEditXTFK3              XTFK
      QString               optExternalAlpha;     ///< LineEditXTFK4              XTFK
      QString               optExternalBeta;      ///< LineEditXTFK5              XTFK
      QString               optExternalGamma;     ///< LineEditXTFK6              XTFK
      bool                  mirrorImage;          ///< CheckBoxSIGN               SIGN
      bool                  writeXYZ;             ///< CheckBoxANIM               ANIM
      
      ///// Advanced - Thresholds
      QString               refineThre1;          ///< LineEditREFT1              REFT
      QString               refineThre2;          ///< LineEditREFT2              REFT
      QString               refineThre3;          ///< LineEditREFT3              REFT
      QString               refineThre4;          ///< LineEditREFT4              REFT
      QString               refineThre5;          ///< LineEditREFT5              REFT
      unsigned int          maxIter;              ///< SpinBoxITER                ITER
      QString               iterThre1;            ///< LineEditTHRE1              THRE
      QString               iterThre2;            ///< LineEditTHRE2              THRE
      QString               iterThre3;            ///< LineEditTHRE3              THRE
      QString               iterThre4;            ///< LineEditTHRE4              THRE

      ///// Advanced - Other
      unsigned int          xbonAtom1;            ///< SpinBoxXBON1               XBON
      unsigned int          xbonAtom2;            ///< SpinBoxXBON2               XBON
      vector<unsigned int>  listXbonAtoms1;       ///< ListViewXBON               XBON
      vector<unsigned int>  listXbonAtoms2;       ///< ListViewXBON               XBON
      unsigned int          massType;             ///< RadioButtonMASS{1|2|3}     MASS/AVMA
      bool                  setLowFreqs;          ///< CheckBoxFRET               FRET
      QString               lowFreqs;             ///< LineEditFRET               FRET
      bool                  orden;                ///< CheckBoxORDA               ORDA      
      
      ///// Debug
      bool                  goon;                 ///< CheckBoxGOON               GOON
      bool                  printDebug;           ///< CheckBoxLONG               LONG
      bool                  update;               ///< CheckBoxUPDT               UPDT
      unsigned int          updateType;           ///< ComboBoxUPDT               UPDT
      QString               updateScaleFactor;    ///< LineEditUPDT               UPDT
      bool                  compact;              ///< CheckBoxSHRT               SHRT
      QString               compactBonds;         ///< LineEditSHRT1              SHRT
      QString               compactAngles;        ///< LineEditSHRT2              SHRT
      QString               compactTorsions;      ///< LineEditSHRT3              SHRT

      ///// Extra
      unsigned int          numLines;             ///< Table
      vector<unsigned int>  hPos;                 ///< Table
      vector<unsigned int>  vPos;                 ///< Table
      QStringList           contents;             ///< Table                                                  
    };
    struct XMLData
    /// A struct representing the XML dictRef values for all the data from WidgetData.
    /// All members have the same name but are QString's.
    {                                                 
      ///// Basic
      QString type;
      QString autoGenerate;
      QString autoRegen;
      QString autoGenFreq;
      QString autoHBonds;
      QString useOneScaleFactor;
      QString sfacValue1;
      QString sfacValue2;
      QString sfacNumSteps;
      QString listValues;
      QString listNumSteps;
      QString numDIIS;
      QString printShifts;
      //QString useDiagonals;
      QString numSteps;

      ///// Internal coordinates
      QString ic_number;
      QString ic_rootItem;
      QString ic_type;
      QString ic_fc;
      QString ic_fix;
      QString ic_maximize;
      QString ic_refValue;
      QString ic_weight;
      QString ic_atom1;
      QString ic_atom2;
      QString ic_atom3;
      QString ic_atom4;

      ///// Cartesian coordinates - Symmetry
      QString checkSymmetry;
      QString checkSymmX;     
      QString checkSymmY;
      QString checkSymmZ;
      QString checkSymmXY;
      QString checkSymmXZ;
      QString checkSymmYZ;
      QString checkSymmXYZ;

      ///// Cartesian coordinates - Other
      QString fixAtom;
      QString fixX;
      QString fixY;
      QString fixZ;      
      QString listFixedAtoms;
      QString listFixedXYZ;
      QString optExternal;
      QString optExternalAngle;
      QString optExternalA;
      QString optExternalB;
      QString optExternalC;
      QString optExternalAlpha;
      QString optExternalBeta;
      QString optExternalGamma; 
      QString mirrorImage;
      QString writeXYZ;
      
      ///// Advanced - Thresholds
      QString refineThre1;
      QString refineThre2;
      QString refineThre3;
      QString refineThre4;
      QString refineThre5;
      QString maxIter;
      QString iterThre1;
      QString iterThre2;
      QString iterThre3;
      QString iterThre4;

      ///// Advanced - Other
      QString xbonAtom1;
      QString xbonAtom2;
      QString listXbonAtoms1;
      QString listXbonAtoms2;
      QString massType;
      QString setLowFreqs;
      QString lowFreqs;
      QString orden;      
      
      ///// Debug
      QString goon;
      QString printDebug;
      QString update;
      QString updateType;
      QString updateScaleFactor;
      QString compact;
      QString compactBonds;
      QString compactAngles;
      QString compactTorsions;

      ///// Extra
      QString numLines;
      QString hPos;
      QString vPos;
      QString contents;                                                 
    };

    ///// private data
    WidgetData data;                    ///< Contains the status of all widgets.
    static const XMLData xml;           ///< Contains the XML dictRef values for all WidgetData.
    AtomSet* atoms;                     ///< Pointer to the AtomSet passed in through the constructor.
    vector<QString> category;           ///< Contains translatable names for the items in ListViewCategory.
    bool widgetChanged;                 ///< = TRUE if the dialog has been changed and needs saving.
    QString calcName;                   ///< Contains the filename prefix (maybe change to pointer to XbraboView QString.
    QString calcDescription;            ///< Contains the description.
    QString calcDir;                    ///< Contains the directory.
    bool calcXF;                        ///< = TRUE if the calculation is to be done in extended format.
    QProcess* maffProcess;              ///< The process for running MAFF
};
    
#endif

