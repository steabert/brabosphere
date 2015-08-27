/***************************************************************************
                          utils.cpp  -  description
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
/// Contains a number of functions used by different classes.

// C++ header files
#include <cmath>

// Qt header files
#include <qcheckbox.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qtable.h>

// Xbrabo header files
#include "utils.h"


///// inputLine ///////////////////////////////////////////////////////////////
QString inputLine(const QString keyword, const double a, const double b, const double c)
/// Returns a string ready to be written to a Brabo input file. 
/// It will consist of the keyword and up to 3 double values.
{
  QString astr, bstr, cstr;
  astr.setNum(a,'f',10).truncate(10);
  bstr.setNum(b,'f',10).truncate(10);
  cstr.setNum(c,'f',10).truncate(10);

  QString result = keyword + "    ";
  result.truncate(4);
  result += "      ";
  result += astr;
  if(fabs(c) > 0.000000001)
  {
    if(fabs(b) > 0.000000001)
      result += bstr + cstr;
    else
      result += "          " + cstr;
  }
  else if(fabs(b) > 0.000000001)
    result += bstr;

  return result;
}

///// inputLine (overloaded) //////////////////////////////////////////////////
QString inputLine(const QString keyword, const int a, const int b, const int c)
/// \overload
/// Returns a string ready to be written to a Brabo input file. 
/// It will consist of the keyword and up to 3 integer values.
{
  QString astr, bstr, cstr;
  if(a != 0)
  {
    astr.setNum(a);
    astr += ".         ";
    astr.truncate(10);
  }
  else
    astr = "          ";
  bstr.setNum(b);
  bstr += ".         ";
  bstr.truncate(10);
  cstr.setNum(c);
  cstr += ".         ";
  cstr.truncate(10);

  QString result = keyword + "    ";
  result.truncate(4);
  result += "      " + astr;
  if(c != 0)
  {
    if(b != 0)
      result += bstr + cstr;
    else
      result += "          " + cstr;
  }
  else if(b != 0)
    result += bstr;

  return result;
}

///// inputLine (overloaded) //////////////////////////////////////////////////
QString inputLine(const QString keyword, const int a, const double b)
/// \overload
/// Returns a string ready to be written to a Brabo input file.
/// It will consist of the keyword, 1 integer and 1 double value.
{
  QString astr, bstr;
  astr.setNum(a);
  astr += ".         ";
  astr.truncate(10);
  bstr.setNum(b,'f',10).truncate(10);

  QString result = keyword + "    ";
  result.truncate(4);
  result += "      " + astr;
  if(fabs(b) > 0.000000001)
    result += bstr;

  return result;
}

///// validatedSYMMLine ///////////////////////////////////////////////////////
QString validatedSYMMLine(QCheckBox* bx, QCheckBox* by, QCheckBox* bz, QCheckBox* bxy, QCheckBox* bxz, QCheckBox* byz, QCheckBox* bxyz)
/// Returns a string ready to be written to an input file, from the symmetry checkboxes.
/// It should also adapt those checkboxes to reflect the proper configuration.
{
  bool x, y, z, xy, yz, xz, xyz;

  x = bx->isChecked();
  y = by->isChecked();
  z = bz->isChecked();
  xy = bxy->isChecked();
  yz = byz->isChecked();
  xz = bxz->isChecked();
  xyz = bxyz->isChecked();

  if(x && y) xy = true;
  if(y && z) yz = true;
  if(x && z) xz = true;
  if(x && y && z) xyz = true;

  if(x && y && z && xy && yz && xz && xyz)
    return QString("symm      all");

  QString result = "symm      ";
  if(x)   result += "x         ";
  if(y)   result += "y         ";
  if(z)   result += "z         ";
  if(xy)  result += "xy        ";
  if(yz)  result += "yz        ";
  if(xz)  result += "xz        ";
  if(xyz) result += "xyz       ";

  return result;
}

///// limitRange //////////////////////////////////////////////////////////////
template<typename T> void limitRange(const T& min, T& value, const T& max)
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

///// resetTable //////////////////////////////////////////////////////////////
void resetTable(QTable* table)
/// Clears a table and sets the number of rows to 3.
{  
  table->setNumRows(3);
  table->selectCells(0, 0, 2, 7);
  table->clearSelection();
}

///// saveTable ///////////////////////////////////////////////////////////////
void saveTable(const QTable* table, unsigned int& numLines, vector<unsigned int>& hPos, vector<unsigned int>& vPos, QStringList& contents)
/// Saves the contents of a QTable to a data structure for use with
/// BraboBase, RelaxBase, ...
{
  numLines = table->numRows();
  hPos.clear();
  vPos.clear();
  contents.clear();
  for(unsigned int j = 0; j < numLines; j++)
  {
    for(unsigned int i = 0; i < 8; i++)
    {
      QString cellContents = table->text(i,j);
      if(!cellContents.isNull())
      {
        hPos.push_back(i);
        vPos.push_back(j);
        contents += cellContents;
      }
    }
  }  
}

///// restoreTable ///////////////////////////////////////////////////////////////
void restoreTable(QTable*  table, const unsigned int& numLines, const vector<unsigned int>& hPos, const vector<unsigned int>& vPos, const QStringList& contents)
/// Restores a QTable from the contents of a data structure for use with
/// BraboBase, RelaxBase, ...
{
  table->setNumRows(numLines);
  table->selectCells(0, 0, numLines - 1, 7);
  table->clearSelection();
  
  for(unsigned int i = 0; i < hPos.size(); i++)
  {
    if(contents[i].length() > 10)
    {
      int cellSpan = (contents[i].length() -1)/10 + 1;
      table->item(hPos[i], vPos[i])->setSpan(1,cellSpan);
    }
    else if(table->item(hPos[i], vPos[i])->colSpan() != 1)
      table->item(hPos[i], vPos[i])->setSpan(1,1);

    table->setText(hPos[i], vPos[i], contents[i]);
  }
}

///// checkTableOverflow //////////////////////////////////////////////////////
void checkTableOverflow(QTable* table, const int row, const int col)
/// Spans cells of a QTable over multiple columns if the contents exceed 10 characters.
/// For use with BraboBase and RelaxBase.
{  
  ///// determine the cell span
  int cellSpan = 1;
  if(table->text(row, col).length() > 10)
    cellSpan = (table->text(row, col).length() - 1)/10 + 1;
  ///// check if the span overflows the total column span (column can range from 0 to 7)
  if((col + cellSpan) > 8)
    cellSpan = 8 - col;
  ///// set the span
  table->item(row, col)->setSpan(1, cellSpan);
}

///// addTableRow /////////////////////////////////////////////////////////////
void addTableRow(QTable* table)
/// Adds a row to a table when no rows are selected. Inserts a row before a
/// selected row.
{  
  ///// check whether a row is selected => insert a row before it
  for(int row = 0; row < table->numRows(); row++)
  {
    if(table->isRowSelected(row, true))
    {
      table->insertRows(row);
      return;
    }
  }
  ///// no row is selected so add a row at the end
  int numberOfRows = table->numRows();
  numberOfRows++;
  table->setNumRows(numberOfRows);  
}

///// removeTableRow //////////////////////////////////////////////////////////
bool removeTableRow(QTable* table)
/// Removes the selected rows from a QTable. Returns false if no rows were
/// selected.
{
  bool succes = false;
  for(int row = 0; row < table->numRows(); row++)
  {
    if(table->isRowSelected(row, true))
    {
      table->removeRow(row);
      succes = true;
    }
  }
  return succes;
}

///// clearTableSelection /////////////////////////////////////////////////////
bool clearTableSelection(QTable* table)
/// Clears the selected cells in a table. Returns false if no cells were cleared.
{  
  if(table->numSelections() > 0)
  {
    ///// one or more cells are selected
    for(int row = 0; row < table->numRows(); row++)
    {
      for(int col = 0; col < 8; col++)
      {
        if(table->isSelected(row, col))
          table->clearCell(row, col);
      }
    }
    table->clearSelection(true);
    return true;
  }
  // else
  return false;
}

///// tableContents ///////////////////////////////////////////////////////////
QStringList tableContents(const QTable* table)
/// Returns the contents of the non-empty rows of the Table in BRABO fixed
/// format.
{
  QStringList result;

  for(int row = 0; row < table->numRows(); row++)
  {
    if(emptyTableRow(table, row))
      continue;

    QString line;
    for(int col = 0; col < 8; col++)
    {
      if(table->text(row, col).isEmpty())
        line += "          ";
      else if(table->item(row, col)->colSpan() != 1)
      {
        QString lineX0 = table->text(row, col) + "          ";
        lineX0.truncate(10 * table->item(row, col)->colSpan());
        line += lineX0;        
        col += table->item(row, col)->colSpan();
      }
      else
      {
        QString line10 = table->text(row, col) + "          ";
        line10.truncate(10);
        line += line10;
      }
    }
    result += line.latin1();
  }
  return result;
}

///// emptyTableRow ///////////////////////////////////////////////////////////
bool emptyTableRow(const QTable* table, const int row)
/// Returns true if a certain row of a Table is empty.
{
  if(row < 0 || row > table->numRows())
    return false;

  for(int col = 0; col < 8; col++)
  {
    if(!table->text(row, col).isEmpty())
      return false;
  }
  return true;
}

///// firstEmptyTableRow //////////////////////////////////////////////////////
int firstEmptyTableRow(QTable* table)
/// Returns the number of the first empty row of a Table and adds a new row
/// if none are empty.
{
  for(int row = 0; row < table->numRows(); row++)
  {
    if(emptyTableRow(table, row))
      return row;
  }
  addTableRow(table);
  return table->numRows() - 1;
}

