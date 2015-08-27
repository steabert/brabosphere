/***************************************************************************
                          brabobase.h  -  description
                             -------------------
    begin                : Sun Jul 28 2002
    copyright            : (C) 2002-2006 by Ben Swerts
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
/// Contains the declaration of the class BraboBase.

#ifndef BRABOBASE_H
#define BRABOBASE_H

///// Forward class declarations & header files ///////////////////////////////

// STL header files
#include <vector>
using std::vector;

// Qt forward class declarations
class QDomElement;
class QPixmap;
class QString;

// Xbrabo forward class declarations
class AtomSet;

// Base class header file
#include "brabowidget.h"


///// class BraboBase /////////////////////////////////////////////////////////
class BraboBase : public BraboWidget
{
  Q_OBJECT

  public:
    ///// constructor/destructor
    BraboBase(AtomSet* atoms, QWidget* parent = 0, const char* name = 0, bool modal = FALSE, WFlags fl = 0);  // constructor
    ~BraboBase();                       // destructor

    ///// public enums
    enum Programs{BRABO, STOCK};

    ///// public member functions for changing data
    void setForces(const bool state);   // sets whether forces are to be calculated
    void setDescription(const QString description);         // sets the description
    void setName(const QString name);   // sets the name (filename prefix)
    void setExtendedFormat(const bool state);     // sets extended format for coordinates & forces
    void setPVMHosts(const QStringList& hosts);   // sets the available PVM hosts

    ///// public member function for retrieving data
    QString method() const;              // returns the method
    
    ///// public member functions for preparing the calculation
    QStringList basissets(bool& ok);    // checks whether all atoms have a basisset assigned and returns the list of basisset files
    QString startVector(bool& prefer, unsigned int& size1, unsigned int& size2);// returns the startvector
    unsigned int maxIterations() const; // returns the maximum number of iterations
            
    //// public member functions for indirect IO
    QStringList generateInput(unsigned int program);        // fills a QStringList with the input file
    QString generateAtdens(bool& ok);   // generate a QString with the appropriate .atdens file
    void loadCML(const QDomElement* root);        // loads widget appearances from file
    void saveCML(QDomElement* root);    // saves widget appearances to file 

    ///// static public member functions
    static void setPreferredBasisset(const unsigned int basisset);    // sets the preferred basisset
      
  public slots:
    void reset();                       // resets all widgets

  protected slots:
    void accept();                      // is called when the changes are accepted (OK clicked)
    void reject();                      // is called when the changes are rejected (Cancel or X clicked)

  private slots:
    ///// actions
    void showPreview();                 // shows a preview of the input file
    void readInputFile();               // reads an existing input file into the widgets
    ///// global
    void changed();                     // sets the 'changed' status of the widget
    void selectWidget(QListViewItem*);  // shows a widget from the widgetstack depending on the listview item
    ///// basic method
    void updateSCFMethodWidgets(int index);       // updates widgets according to the SCF method index
    void updateSCFTypeWidgets(int index);         // updates widgets according to the SCF type index
    void selectStartvector();           // selects an existing startvector
    ///// basic basisset
    void oneBasisset(bool on);          // toggles between 1/multiple basissets
    void addBasisset();                 // adds a basisset to the list
    void removeBasisset();              // removes a basisset from the list
    ///// SCF convergence    
    void toggleDIIS(bool on);           // toggles DIIS options on/off
    //// PVM
    void addPVMHost();                  // adds a PVM host to the NOTH list
    void removePVMHost();               // removes a PVM host from the NOTH list
    ///// debug
    void adjustPrintSCF();              // adjusts the PrintSCF checkboxes
    ///// extra
    void checkOverflowMain(int row, int col);     // spans a cel over multiple columns if the field is longer than 10 chars (TableMain)
    void checkOverflowPVM(int row, int col);      // spans a cel over multiple columns if the field is longer than 10 chars (TablePVM)
    void checkOverflowINTE(int row, int col);     // spans a cel over multiple columns if the field is longer than 10 chars (TableINTE)
    void checkOverflowSCF(int row, int col);      // spans a cel over multiple columns if the field is longer than 10 chars (TableSCF)
    void checkOverflowFORC(int row, int col);     // spans a cel over multiple columns if the field is longer than 10 chars (TableFORC)
    void addRowMain();                  // adds a row to TableMain
    void addRowPVM();                   // adds a row to TablePVM
    void addRowINTE();                  // adds a row to TableINTE
    void addRowSCF();                   // adds a row to TableSCF
    void addRowFORC();                  // adds a row to TableFORC    
    void removeRowMain();               // removes a row from TableMain
    void removeRowPVM();                // removes a row from TablePVM
    void removeRowINTE();               // removes a row from TableINTE
    void removeRowSCF();                // removes a row from TableSCF
    void removeRowFORC();               // removes a row from TableFORC
    void clearSelectionMain();          // clears the selected cells in TableMain
    void clearSelectionPVM();           // clears the selected cells in TablePVM
    void clearSelectionINTE();          // clears the selected cells in TableINTE
    void clearSelectionSCF();           // clears the selected cells in TableSCF
    void clearSelectionFORC();          // clears the selected cells in TableFORC    

  private:
    ///// private enums
    enum Categories{BASIC = 0, ADVANCED, PROPERTIES, SCFCONVERGENCE, PVM, DEBUG1, EXTRA,
                    ADVANCED_METHOD, ADVANCED_SYMMETRY, ADVANCED_OTHER,
                    PROPERTIES_CHARGES, PROPERTIES_OTHER, 
                    EXTRA_MAIN, EXTRA_PVM, EXTRA_INTE, EXTRA_SCF, EXTRA_FORC};

    ///// private member functions
    void makeConnections();             // sets up all permanent connections
    void init();                        // initializes the dialog
    void fillComboBoxes();              // fills the comboboxes 
    void saveWidgets();                 // saves widget appearances
    void restoreWidgets();              // restores widget appearances
    QStringList parsedNUCLlines();      // returns a stringlist containing 'NUCL     x' lines
    void buildAtdens();                 // generates .atdens files from the basis set files

    ///// private structs
    struct WidgetData
    /// A struct for saving the contents of all BraboWidget widgets. 
    /// Private to the class BraboBase.
    {                                                 //// WIDGET                     KEYWORD(S)

      ///// Basic Method
      unsigned int          SCFMethod;            ///< ComboBoxSCFMethod          SCF/UHF/MP2E
      unsigned int          SCFType;              ///< ComboBoxSCFType            MIA/DRCT/TWOE
      bool                  useStartvector;       ///< CheckBoxSTAR               STAR
      QString               startvector;          ///< LineEditSTAR               F99=
      bool                  preferStartvector;    ///< CheckBoxSTAR2              F99=
      int                   charge;               ///< SpinBoxCHAR                CHAR

      ///// Basic Basis set
      bool                  useOneBasisset;       ///< RadioButtonBAS{1|2}        GBAS/ABAS
      unsigned int          basisset1;            ///< ComboBoxBAS1               GBAS
      unsigned int          basissetAtom;         ///< ComboBoxBAS2               ABAS
      unsigned int          basisset2;            ///< ComboBoxBAS3               ABAS
      vector<unsigned int>  listAtoms;            ///< ListViewBAS                ABAS
      vector<unsigned int>  listBasissets;        ///< ListViewBAS                ABAS

      ///// Advanced Method
      unsigned int          MP2Type;              ///< RadioButtonMP2E{1|2|3}     MP2E          (0, 1, 2)
      unsigned int          MP2ExcludeOcc;        ///< SpinBoxMP2E1               MP2E
      unsigned int          MP2ExcludeVirt;       ///< SpinBoxMP2E2               MP2E
      bool                  MP2Density;           ///< CheckBoxMP2D               MP2D
      bool                  useField;             ///< CheckBoxFIEL               FIEL
      QString               fieldX;               ///< LineEditFIELx              FIEL
      QString               fieldY;               ///< LineEditFIELy              FIEL
      QString               fieldZ;               ///< LineEditFIELz              FIEL
      bool                  useSCRF;              ///< CheckBoxSCRF               SCRF
      QString               SCRFEpsilon;          ///< LineEditSCRF1              SCRF
      QString               SCRFRadius;           ///< LineEditSCRF2              SCRF

      ///// Advanced Symmetry
      bool                  useSymmetry;          ///< CheckBoxSYMM               SYMM
      bool                  useSymmAuto;          ///< RadioButtonSYMM{1|2}       SYMM
      bool                  useSymmX;             ///< CheckBoxSYMMx              SYMM
      bool                  useSymmY;             ///< CheckBoxSYMMy              SYMM
      bool                  useSymmZ;             ///< CheckBoxSYMMz              SYMM
      bool                  useSymmXY;            ///< CheckBoxSYMMxy             SYMM
      bool                  useSymmYZ;            ///< CheckBoxSYMMyz             SYMM
      bool                  useSymmXZ;            ///< CheckBoxSYMMxz             SYMM
      bool                  useSymmXYZ;           ///< CheckBoxSYMMxyz            SYMM

      ///// Advanced Other
      QString               valueIpol;            ///< LineEditIPOL               IPOL
      bool                  useSkpc;              ///< CheckBoxSKPC               SKPC
      bool                  printGeop;            ///< CheckBoxGEOP               GEOP
      bool                  printWF;              ///< CheckBoxPRWF               PRWF
      bool                  printDM;              ///< CheckBoxPRDM               PRDM
      bool                  useDST;               ///< CheckBoxDST                DS=T
      bool                  useACD;               ///< CheckBoxACD                AC=D
      bool                  saveF11;              ///< CheckBoxF11                F11=
      bool                  useFato;              ///< CheckBoxFATO               FATO
      unsigned int          numFato;              ///< SpinBoxFATO                FATO
      bool                  useMIAForce;          ///< CheckBoxMIAForc            MIA

      ///// Properties Charges
      bool                  useMulliken;          ///< CheckBoxMULL1              MULL
      bool                  MullikenNoOverlap;    ///< CheckBoxMULL2              MULL
      bool                  MullikenEachIter;     ///< CheckBoxMULL3              MUL1
      bool                  useStock;             ///< CheckBoxSTOCK              stock
      unsigned int          stockType;            ///< RadioButtonStockDENS{1|2|3|4|5}  stock HOMO/LUMO/TRDE/UHF
      bool                  stockTotalDensity;    ///< ComboBoxStockMOLD          stock MOLD
      unsigned int          stockTransition1;     ///< SpinBoxStockTRDE1          stock TRDE
      unsigned int          stockTransition2;     ///< SpinBoxStockTRDE2          stock TRDE
      bool                  useStockElmo;         ///< CheckBoxStockELMO          stock ELMO
      bool                  useStockEpar;         ///< CheckBoxStockEPAR          stock EPAR

      ///// Properties Other
      bool                  useBoys;              ///< CheckBoxLOCA               LOCA
      unsigned int          numBoysIter;          ///< SpinBoxLOCA                LOCA
      QString               BoysThreshold;        ///< LineEditLOCA               LOCA
      QString               listNucl;             ///< LineEditNUCL               NUCL
      bool                  useElmo;              ///< CheckBoxELMO               ELMO
      bool                  useExit;              ///< CheckBoxEXIT               EXIT
      bool                  useJab;               ///< CheckBoxJAB                JAB

      ///// SCF Convergence
      int                   numIter;              ///< SpinBoxITER                ITER
      bool                  useJacobi;            ///< CheckBoxJACO               JACO
      bool                  useDIIS;              ///< CheckBoxDIIS               DIIS
      QString               DIISThre;             ///< LineEditDIIS               DIIS
      int                   numDIISCoeff;         ///< SpinBoxDIIS                DIIS
      QString               valueLvsh;            ///< LineEditLVSH               LVSH
      bool                  useDlvs;              ///< CheckBoxDLVS               DLVS
      QString               thresholdINTEa;       ///< LineEditThreINTEa          THRE
      QString               thresholdINTEb;       ///< LineEditThreINTEb          THRE
      QString               thresholdMIAa;        ///< LineEditThreSCF1a          THRE
      QString               thresholdMIAb;        ///< LineEditThreSCf1b          THRE
      QString               thresholdSCFa;        ///< LineEditThreSCF2a          THRE
      QString               thresholdSCFb;        ///< LineEditThreSCF2b          THRE
      QString               thresholdOverlapa;    ///< LineEditSthrSCFa           STHR
      QString               thresholdOverlapb;    ///< LineEditSthrSCFb           STHR
      bool                  useVarThreMIA;        ///< CheckBoxVthrSCF            VTHR

      ///// PVM
      bool                  usePVM;               ///< CheckBoxPVM                PVM
      QStringList           PVMExcludeHosts;      ///< ListBoxNOTH1               NOTH
      unsigned int          PVMNumShells;         ///< SpinBoxNCST                NCST
      unsigned int          PVMNumTasks;          ///< SpinBoxNTAS                NTAS
      bool                  notOnMaster;          ///< CheckBoxNOSE               NOSE
      int                   valueNice;            ///< SpinBoxNICE                NICE
      bool                  usePacked;            ///< CheckBoxPACK               PACK

      ///// Debug
      bool                  printDebugPVM;        ///< CheckBoxDebugPVM           PRIN
      bool                  printDebugFORC;       ///< CheckBoxDebugFORC          PRIN
      bool                  goonINTE;             ///< CheckBoxGoonINTE           GOON
      bool                  goonSCF;              ///< CheckBoxGoonSCF            GOON
      bool                  printIntegrals;       ///< CheckBoxIntINTE            PRIN
      bool                  printShells;          ///< CheckBoxShellsINTE         LONG
      unsigned int          levelPrintSCF;        ///< CheckBoxPrintSCF{1|2|3}    PRIN
      unsigned int          numVirtual;           ///< SpinBoxPrintSCFNumVirtual  PRIN

      ///// Extra
      unsigned int          numLinesExtraMain;    ///< TableMain{1|2}   Each table element is saved
      unsigned int          numLinesExtraPVM;     ///< TablePVM{1|2}    by its column (hPos), row
      unsigned int          numLinesExtraINTE;    ///< TableINTE{1|2}   (vPos) and contents.
      unsigned int          numLinesExtraSCF;     ///< TableSCF{1|2}    The total number of lines
      unsigned int          numLinesExtraFORC;    ///< TableFORC{1|2}   of the table is also saved.
      vector<unsigned int>  hPosExtraMain;        ///< TableMain{1|2}
      vector<unsigned int>  hPosExtraPVM;         ///< TablePVM{1|2}
      vector<unsigned int>  hPosExtraINTE;        ///< TableINTE{1|2}
      vector<unsigned int>  hPosExtraSCF;         ///< TableSCF{1|2}
      vector<unsigned int>  hPosExtraFORC;        ///< TableFORC{1|2}
      vector<unsigned int>  vPosExtraMain;        ///< TableMain{1|2}
      vector<unsigned int>  vPosExtraPVM;         ///< TablePVM{1|2}
      vector<unsigned int>  vPosExtraINTE;        ///< TableINTE{1|2}
      vector<unsigned int>  vPosExtraSCF;         ///< TableSCF{1|2}
      vector<unsigned int>  vPosExtraFORC;        ///< TableFORC{1|2}
      QStringList           contentsExtraMain;    ///< TableMain{1|2}
      QStringList           contentsExtraPVM;     ///< TablePVM{1|2}
      QStringList           contentsExtraINTE;    ///< TableINTE{1|2}
      QStringList           contentsExtraSCF;     ///< TableSCF{1|2}
      QStringList           contentsExtraFORC;    ///< TableFORC{1|2}
    };
    struct XMLData
    /// A struct representing the XML dictRef values for all the data from WidgetData.
    /// All members have the same name but are QString's.
    {
      ///// Basic Method
      QString SCFMethod;
      QString SCFType;
      QString useStartvector;
      QString startvector; 
      QString preferStartvector;
      QString charge;
      ///// Basic Basis set
      QString useOneBasisset;
      QString basisset1;
      QString basissetAtom;
      QString basisset2;
      QString listAtoms;
      QString listBasissets;
      ///// Advanced Method
      QString MP2Type;
      QString MP2ExcludeOcc;
      QString MP2ExcludeVirt;
      QString MP2Density; 
      QString useField;
      QString fieldX;
      QString fieldY;
      QString fieldZ; 
      QString useSCRF;
      QString SCRFEpsilon;
      QString SCRFRadius;
      ///// Advanced Symmetry
      QString useSymmetry;
      QString useSymmAuto;
      QString useSymmX;
      QString useSymmY; 
      QString useSymmZ;
      QString useSymmXY;
      QString useSymmYZ;
      QString useSymmXZ;
      QString useSymmXYZ;
      ///// Advanced Other
      QString valueIpol;
      QString useSkpc;
      QString printGeop; 
      QString printWF;
      QString printDM;
      QString useDST;
      QString useACD;
      QString saveF11;
      QString useFato;
      QString numFato;
      QString useMIAForce;
      ///// Properties Charges
      QString useMulliken;
      QString MullikenNoOverlap;
      QString MullikenEachIter;
      QString useStock;
      QString stockType;
      QString stockTotalDensity;
      QString stockTransition1;
      QString stockTransition2;
      QString useStockElmo;
      QString useStockEpar;
      ///// Properties Other
      QString useBoys;
      QString numBoysIter;
      QString BoysThreshold;
      QString listNucl;
      QString useElmo;
      QString useExit;
      QString useJab;
      ///// SCF Convergence
      QString numIter;
      QString useJacobi;
      QString useDIIS;
      QString DIISThre;
      QString numDIISCoeff;
      QString valueLvsh; 
      QString useDlvs; 
      QString thresholdINTEa;
      QString thresholdINTEb;
      QString thresholdMIAa;
      QString thresholdMIAb;
      QString thresholdSCFa; 
      QString thresholdSCFb;
      QString thresholdOverlapa;
      QString thresholdOverlapb;
      QString useVarThreMIA;
      ///// PVM
      QString usePVM;
      QString PVMExcludeHosts;
      QString PVMNumShells;
      QString PVMNumTasks;
      QString notOnMaster;
      QString valueNice;
      QString usePacked;
      ///// Debug
      QString printDebugPVM;
      QString printDebugFORC;
      QString goonINTE;
      QString goonSCF;
      QString printIntegrals;
      QString printShells;
      QString levelPrintSCF;
      QString numVirtual;
      ///// Extra
      QString numLinesExtraMain;
      QString numLinesExtraPVM;
      QString numLinesExtraINTE; 
      QString numLinesExtraSCF;
      QString numLinesExtraFORC;
      QString hPosExtraMain;
      QString hPosExtraPVM;
      QString hPosExtraINTE;
      QString hPosExtraSCF;
      QString hPosExtraFORC;
      QString vPosExtraMain;
      QString vPosExtraPVM;
      QString vPosExtraINTE;
      QString vPosExtraSCF;
      QString vPosExtraFORC;
      QString contentsExtraMain;
      QString contentsExtraPVM;
      QString contentsExtraINTE;
      QString contentsExtraSCF;
      QString contentsExtraFORC;
    };

    ///// private member data
    // externally set
    bool calcForces;                    ///< = true if forces should be calculated
    QString calcDescription;            ///< contains the description
    QString calcName;                   ///< contains the filename prefix
    //QString calcDir;                    ///< contains the directory
    bool calcXF;                        ///< = true if extended format is to be used
    // internally set
    WidgetData data;                    ///< contains the status of all widgets
    bool widgetChanged;                 ///< is set when any of the widgets change
    vector<QString> category;           ///< Contains translatable names for the items in ListViewCategories
    AtomSet* atoms;                     ///< A pointer to the AtomSet 

    ///// static private member data
    static QStringList pvmHosts;        // contains the PVM host list
    static unsigned int preferredBasisset;        // contains the preferred basisset 
    static const XMLData xml;           ///< Contains the XML dictRef values for all WidgetData.
};

#endif

