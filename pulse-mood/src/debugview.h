/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright 2017  <copyright holder> <email>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
 */

#ifndef DEBUGVIEW_H
#define DEBUGVIEW_H

#include <QWidget>

#include "analysis.h"

class DebugView : public QWidget
{
    Q_OBJECT
    
  public:
    DebugView(const Analysis* analysis, const size_t nBands);
    ~DebugView();
    
  protected:
    void paintEvent(QPaintEvent *event);
  

  private:
    const Analysis *analysis;
    const size_t nBands;
};

#endif // DEBUGVIEW_H
