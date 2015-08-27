/***************************************************************************
                           utils.h  -  description
                             -------------------
    begin                : Wed Jul 31 2002
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
/// Contains the declarations for the functions implemented in utils.cpp.
#ifndef UTILS_H
#define UTILS_H

///// Forward class declarations & header files ///////////////////////////////

// STL header files
#include <vector>
using std::vector;

///// Qt forward class declarations
class QCheckBox;
class QString;
class QStringList;
class QTable;


///// Functions ///////////////////////////////////////////////////////////////

///// input files
QString inputLine(const QString keyword, const double a, const double b = 0.0, const double c = 0.0);         // returns a string ready to be written to an input file
QString inputLine(const QString keyword, const int a, const int b = 0, const int c = 0);  // overloaded
QString inputLine(const QString keyword, const int a, const double b);// overloaded
QString validatedSYMMLine(QCheckBox* bx, QCheckBox* by, QCheckBox* bz, QCheckBox* bxy, QCheckBox* bxz, QCheckBox* byz, QCheckBox* bxyz);    // returns a string containing a valid SYMM line based on the checkboxes

///// limiting
template<typename T> void limitRange(const T& min, T& value, const T& max);

///// QTable manipulators
void resetTable(QTable* table);         // resets a table to an empty 3-row table
void saveTable(const QTable* table, unsigned int& numLines, vector<unsigned int>& hPos, vector<unsigned int>& vPos, QStringList& contents); // saves the contents of the table
void restoreTable(QTable* table, const unsigned int& numLines, const vector<unsigned int>& hPos, const vector<unsigned int>& vPos, const QStringList& contents);// restores the table
void checkTableOverflow(QTable* table, const int row, const int col);
void addTableRow(QTable* table);        // adds a row to a table
bool removeTableRow(QTable* table);     // removes the selected row(s) from a table
bool clearTableSelection(QTable* table);// clears the selected cells in a table
QStringList tableContents(const QTable* table);   // returns the contents of a QTable in BRABO input format
bool emptyTableRow(const QTable* table, const int row);     // returns true if the row is empty
int firstEmptyTableRow(QTable* table);  // returns the first empty row

#endif

