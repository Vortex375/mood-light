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

#include "debugview.h"

#include <QPainter>

DebugView::DebugView(const Analysis * analysis, size_t nBands) : QWidget(),
  analysis(analysis),
  nBands(nBands)
{
  setFixedSize(QSize(400, 500));
}

DebugView::~DebugView()
{
  
}

void DebugView::paintEvent(QPaintEvent *event)
{
  const double *bands = analysis->getBands();
  const double *beatBandAverage = analysis->getBeatBandsAverage();
  const double *beatBandsFactor = analysis->getBeatBandsFactor();
  double maxBeatFactor = analysis->getBeatFactor();
  int lockOnBand = analysis->getBeatLockOnBand();
  double peak = analysis->getSmoothPeak();
  double avgPeak = analysis->getAveragePeak();
//  double beatFactor = analysis->getBeatFactor();
  
//  if (beatFactor < 1.0) beatFactor = 1.0;
  
  QPainter painter(this);
  
  painter.fillRect(5, 150, 10, 100, Qt::lightGray);
  painter.fillRect(5, 250 - avgPeak * 100, 10, avgPeak * 100, Qt::darkGray);
  painter.setOpacity(0.6);
  painter.fillRect(5, 250 - peak * 100, 10, peak * 100, Qt::red);
  painter.setOpacity(1.0);

  for (int i = 0; i < Analysis::BEAT_BANDS.size(); i++) {
    double val = beatBandAverage[i] * 100;
    painter.fillRect(Analysis::BEAT_BANDS[i] * 20 + 5, 250 - val, 10, val, Qt::darkGray);
  }

  painter.setOpacity(0.6);
  for (int i = 1; i < nBands; i++) {
    double val = bands[i];
    if (val > 0) {
      painter.fillRect(i * 20 + 5, 250 - val, 10, val, Qt::blue);
    } else {
      painter.fillRect(i * 20 + 5, 250, 10, -val, Qt::blue);
    }
    painter.drawText(i * 20 + 5, 260, QString::number(i));
  }
  painter.setOpacity(1.0);

  for (int i = 0; i < Analysis::BEAT_BANDS.size(); i++) {
    double beatFactor = beatBandsFactor[i] ;
    if (beatFactor > 1.3)
      painter.fillRect(200 - beatFactor * 30, 270 + 15 * i, beatFactor * 60, 10, Qt::green);
    else
      painter.fillRect(200 - beatFactor * 30, 270 + 15 * i, beatFactor * 60, 10, Qt::red);
    painter.drawText(200, 280 + 15 * i, QString::number(Analysis::BEAT_BANDS[i]));
  }
  painter.fillRect(200 - maxBeatFactor * 30, 270 + 15 * Analysis::BEAT_BANDS.size(), maxBeatFactor * 60, 10, Qt::darkYellow);
  painter.drawText(200, 280 + 15 * Analysis::BEAT_BANDS.size(), lockOnBand < 0 ? "-" : QString::number(Analysis::BEAT_BANDS[lockOnBand]));

//  if (beatFactor > 1.3)
//    painter.fillRect(200 - beatFactor * 30, 270, beatFactor * 60, 10, Qt::green);
//  else
//    painter.fillRect(200 - beatFactor * 30, 270, beatFactor * 60, 10, Qt::red);
  
  painter.setPen(Qt::red);
  painter.drawLine(0, 250, 400, 250);
}
