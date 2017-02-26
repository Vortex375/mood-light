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

DebugView::DebugView(const Analysis * analysis) : QWidget(),
  analysis(analysis),
  intensitySlider(Qt::Horizontal, this)
{
  setFixedSize(QSize(500, 500));
  intensitySlider.setRange(0, 100);
  intensitySlider.setValue(20);
  intensitySlider.setGeometry(400, 485, 100, 15);
}

DebugView::~DebugView()
{
  
}

void DebugView::paintEvent(QPaintEvent *event)
{
  int fps = 0;
  if (lastRedraw.isValid() && lastRedraw.elapsed() > 0) {
    fps = 1000 / lastRedraw.elapsed();
  }
  lastRedraw.start();

  const double *bands = analysis->getBands();
  const double *beatBandAverage = analysis->getBeatBandsAverage();
  const double *beatBandsFactor = analysis->getBeatBandsFactor();
  const double *triSpectrum = analysis->getTriSpectrum();
  double maxBeatFactor = analysis->getBeatFactor();
  int lockOnBand = analysis->getBeatLockOnBand();
  double peak = analysis->getSmoothPeak();
  double avgPeak = analysis->getAveragePeak();
//  double beatFactor = analysis->getBeatFactor();
  
//  if (beatFactor < 1.0) beatFactor = 1.0;
  
  QPainter painter(this);

  painter.drawText(0, 10, QString("FPS: %1").arg(fps));

  painter.fillRect(5, 150, 10, 100, Qt::lightGray);
  painter.fillRect(5, 250 - avgPeak * 100, 10, avgPeak * 100, Qt::darkGray);
  painter.setOpacity(0.6);
  painter.fillRect(5, 250 - peak * 100, 10, peak * 100, Qt::red);
  painter.setOpacity(1.0);

  for (int i = 0; i < Analysis::BEAT_BANDS.size(); i++) {
    double val = beatBandAverage[i];
    painter.fillRect(i * 20 + 25, 250 - (val * 250), 10, val * 250, Qt::darkGray);
  }

  painter.setOpacity(0.6);
  for (int i = 0; i < Analysis::N_BANDS; i++) {
    double val = bands[i];
    painter.fillRect(i * 20 + 25, 250 - (val * 250), 10, val * 250, Qt::blue);

    painter.drawText(i * 20 + 25, 260, QString::number(i));
  }
  painter.setOpacity(1.0);

  for (int i = 0; i < Analysis::BEAT_BANDS.size(); i++) {
    double beatFactor = beatBandsFactor[i] ;
    if (beatFactor > Analysis::BEAT_THRESHOLD[i])
      painter.fillRect(200 - beatFactor * 30, 270 + 15 * i, beatFactor * 60, 10, Qt::green);
    else
      painter.fillRect(200 - beatFactor * 30, 270 + 15 * i, beatFactor * 60, 10, Qt::red);
    painter.drawText(200, 280 + 15 * i, QString::number(Analysis::BEAT_BANDS[i]));
  }
  painter.fillRect(200 - maxBeatFactor * 30, 270 + 15 * Analysis::BEAT_BANDS.size(), maxBeatFactor * 60, 10, Qt::darkYellow);
  painter.drawText(200, 280 + 15 * Analysis::BEAT_BANDS.size(), lockOnBand < 0 ? "-" : QString::number(Analysis::BEAT_BANDS[lockOnBand]));

  QColor triColor[] = {Qt::red, Qt::green, Qt::blue};
  for (int i = 0; i < 3; i++) {
    painter.fillRect(i * 20 + 5, 470 - triSpectrum[i] * 100, 10, triSpectrum[i] * 100, triColor[i]);
  }
  QColor finalColor((int) (255 *triSpectrum[0]),
                    (int) (255 *triSpectrum[1]),
                    (int) (255 *triSpectrum[2]));
  painter.fillRect(5, 475, 55, 20, finalColor);

//  if (beatFactor > 1.3)
//    painter.fillRect(200 - beatFactor * 30, 270, beatFactor * 60, 10, Qt::green);
//  else
//    painter.fillRect(200 - beatFactor * 30, 270, beatFactor * 60, 10, Qt::red);
  
  painter.setPen(Qt::red);
  painter.drawLine(0, 250, 400, 250);

  painter.end();
}

double DebugView::getMaxIntensity() {
  return (double) intensitySlider.value() / 100;
}
