/***************************************************************************
                         calculation.h  -  description
                             -------------------
    begin                : Thu April 28 2005
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
/// Contains the declaration of the class Calculation

#ifndef CALCULATION_H
#define CALCULATION_H

///// Forward class declarations & header files ///////////////////////////////

// STL header files
#include <list>
#include <vector>

// Qt forward class declarations & header files
class QDomElement;
class QProcess;
#include <qstringlist.h>
class QWidget;

// Xbrabo forward class declarations
class AtomSet;

// Base class header file
#include <qobject.h>

///// class Calculation ///////////////////////////////////////////////////////
class Calculation : public QObject
{
  Q_OBJECT

  public:
    // constructor/destructor
    Calculation(AtomSet* atomset, QWidget* parent = 0, const char* name = 0);   // constructor
    ~Calculation();                     // destructor
   
    // public enums
    enum Errors{NoError, UndefinedError, NoConvergence, CloseNuclei, MaxCyclesExceeded, ManualStop}; ///< Possible errors that can occur during the calcualtion

    // public member functions for starting/stopping the calculation
    bool start();                       // starts the calculation
    bool pause();                       // pauses the calculation
    bool stop();                        // stops the calculation (ask whether to stop immediately or after the current step is finished)

    // public member functions for setting up the calculation
    void setModified(const bool status = true);   // sets the 'modified' status of the calculation
    void setBraboInput(const QStringList stdInput, const QStringList basissets, const QString startVector, const bool preferStartVector, const unsigned int startVectorSize1, const unsigned int startVectorSize2);         // sets the BRABO input file, the list of basisets to be used and the place where the starting vector should be copied from
    void setStockInput(const QStringList stin, const QString atdens = QString::null); // sets the STOCK input file and the specific atdens file
    void setRelaxInput(const QStringList aff, const QStringList maff, const unsigned int updateFreq, const unsigned int maxSteps, const std::vector<unsigned int>& steps, const std::vector<double>& factors);    // sets the Relax AFF (header) file, its update frequency and scalefactor info
    void setCalculationType(const unsigned int type, const unsigned int buur); // SPE/OPT/FREQ, NoBuur/PC/SM
    void setParameters(const QString name, const QString dir, const bool format);         // Some more needed parameters
    void setBackup(const unsigned int freq, const bool brabo = false, const bool stock = false, const bool relax = false, const bool aff = false, const bool crd = false);// Sets the backup options
    void setContinuable(const bool status);       // Sets whether an optimization can be continued after having it stopped.

    // public member functions for returning data
    bool isRunning() const;             // returns whether the calculation is running
    bool isPaused() const;              // returns whether the calculation is paused
    void getRefinementParameters(bool& largestCart, bool& magnCart, bool& largestInt, bool& magnInt, bool& largestShift);         // Returns the 5 refinement criteria for the latest cycle
    void getAvailableOutputs(std::vector<unsigned int>& cycles, std::vector<bool>& brabo, std::vector<bool>& stock, std::vector<bool>& relax, std::vector<bool>& aff);    // Returns the cycles for which outputs are available
    QStringList braboOutput(const unsigned int step = 0);   // Returns the contents of the Brabo output file for a step
    QStringList stockOutput(const unsigned int step = 0);   // Returns the contents of the Stock output file for a step
    QStringList relaxOutput(const unsigned int step = 0);   // Returns the contents of the Relax output file for a step
    QStringList affOutput(const unsigned int step = 0);     // Returns the contents of the AFF file for a step

    // public member functions for IO
    void loadCML(const QDomElement* root);        // loads the settings from file
    void saveCML(QDomElement* root);    // saves the settings to file 
    void writeInput();                  // writes all input files
    void clean();                       // cleans up the calculation directory

  signals:
    void newIteration(unsigned int, double);      ///< Is emitted when a new SCF iteration is completed
    void newCycle(unsigned int);        ///< Is emitted when a new optimization cycle is started
    void updated();                     ///< Is emitted when the AtomSet is updatd with new coordinates and properties
    void finished(unsigned int error);  ///< Is emitted when the calculation is finished.
    void modified();                    ///< Is emitted when the calculation is modified and needs to be saved.

  private slots:
    void finishBrabo();                 // Does cleanup of a BRABO run
    void readStdOutBrabo();             // Reads output from the process running BRABO
    void finishStock();                 // Does cleanup of a STOCK run
    void finishMAFF();                  // Does cleanup of a MAFF run
    void finishCnvrtAFF();              // Does cleanup of a CNVRTAFF run
    void finishRelax();                 // Does cleanup of a RELAX run
    void readStdOutAll();               // Reads output from the process without checking the contents

  private:
    // private enums
    enum CalculationStep{NEW_CYCLE, ACHAR, BRABO, BUUR, CNVRTAFF, DISTOR, FORKON, MAFF, RELAX, STOCK, UPDATE};        ///< The different programs that are possibly run. NEW_CYCLE indicates the start of a new cycle, UPDATE the place where the AtomSet is updated

    // private member functions
    bool checkBasissets();              // checks whether all basisset files given in basissetList exist
    bool checkExecutables();            // checks whether all needed executables are present
    bool makeDirCurrent(const QString dir, const QString title);      // switches to the desired directory
    bool copyStartVector();             // copies startVectorFile to the calculation directory
    void doNextStep();                  // starts the next step of the calculation
    void runBrabo();                    // starts a BRABO run
    void runStock();                    // starts a STOCK run
    void runMAFF();                     // starts a MAFF run
    void runCnvrtAFF();                 // starts a CNVRTAFF run
    void runRelax();                    // starts a RELAX run
    void runCleanup();                  // cleanup at the end of a calculation
    void runUpdate();                   // updates the AtomSet with the newly calculated properties (and coordinates)
    double scaleFactor(const unsigned int step);  // returns the scalefactor corresponding to the optimization step
    void backupOutputs();               // does a backup of requested output files
    bool copyFile(const QString source, const QString destination);   // blocking copy
    QStringList output(const QString extension, const unsigned int step = 0);     // Returns the contents of a file with the given extension for a certain step
    std::vector<double> loadFromPunch(const QString code, const unsigned int numValues, const unsigned int fieldSize, const unsigned int fieldsPerLine);        // Reads data from the punch file in fixed format

    // private member data
    AtomSet* atoms;                     ///< The atomset that is to be modified
    QWidget* parentWidget;              ///< needed for showing properly positioned messageboxes
    QStringList braboInput;             ///< The standard input fed to BRABO.
    QStringList stockInput;             ///< The contents of the stdin file for STOCK.
    QStringList relaxInput;             ///< The aff (header) file.
    QStringList maffInput;              ///< The standard input fed to MAFF.
    QStringList basissetList;           ///< The list of basissets used for the calculation.
    QString startVectorFile;            ///< The file containing a starting vector that should be copied to the calculation directory 
    QString atdensFile;                 ///< The contents of the atomic density file needed for STOCK.
    unsigned int affUpdateFreq;         ///< The update frequency for the AFF file (0 = no update).
    unsigned int calculationType;       ///< The type of calculation (SP, GeomOpt, Freq).
    unsigned int crystalType;           ///< The type of crystal to be generated (none, PC, SM)
    bool updateBraboInput;              ///< = true if \c braboInput contains an updated input file
    bool updateStockInput;              ///< = true if \c stockInput contains an updated input file
    bool updateRelaxInput;              ///< = true if \c relaxInput contains an updated input file
    bool doBasissetCheck;               ///< = true if a check should be made whether the basissets listed in \c basissetList exist.
    bool copyAtdens;                    ///< = true if \c atdensFile contains updated atomic densities
    bool calcRunning;                   ///< = true if the calculation is running
    bool calcPaused;                    ///< = true if the calculation is in paused mode
    QString calcName;                   ///< The basename from which all input filenames are constructed
    QString calcDir;                    ///< The calculation directory
    QProcess* calcProcess;              ///< The process used for running all BRABO applications
    unsigned int calcMaxCycle;          ///< The maximum number of optimization cycles
    bool calcXF;                        ///< Normal (false) or Extended format (true) for the CRD file
    unsigned int currentCycle;          ///< The current optimization cycle.
    bool calcSuccess;                   ///< Indicates whether the calculation ended successfully
    std::list<CalculationStep> calculationSteps;  ///< The list of steps that are to be carried out in a calculation
    std::vector<unsigned int> scaleSteps;         ///< Info needed to determine the RELAX scale factor.
    std::vector<double> scaleFactors;             ///< Info needed to determine the RELAX scale factor.
    bool stopRequested;                 ///< A flag indicating a request for stopping the calculation after the current step was made
    unsigned int backupFrequency;       ///< The frequency with which outputs should be backed up. Zero is no backup
    bool backupBrabo;                   ///< If true, the Brabo outputs will be backed up
    bool backupStock;                   ///< If true, the Stock outputs will be backed up
    bool backupRelax;                   ///< If true, the Relax outputs will be backed up
    bool backupAFF;                     ///< If true, the AFF files will be backed up
    bool backupCRD;                     ///< If true, the CRD files will be backed up     
    bool calcModified;                  ///< Contains the 'modified' status of the calculation
    unsigned int calcError;             ///< Contains any error that occurred during the calculation
    bool calcContinuable;               ///< Determines whether an optimization can be continued when it is restarted.
};

#endif

