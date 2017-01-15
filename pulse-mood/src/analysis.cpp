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

#include "analysis.h"

#include <cmath>
#include <algorithm>
#include <cstring>

#include <QDebug>
#include <stdio.h>

Analysis::Analysis(QObject* parent, const int fftSize, const int nBands) :
  QObject(parent),
  fftSize(fftSize),
  nBands(nBands)
{
  bands = new double[nBands];
  delay = new int[nBands];
  logScale = new double[nBands + 1];
  
  // initialize logarithmic scale
  for (int i = 0; i <= nBands; i++) {
    logScale[i] = pow(fftSize / 2, (double) i / nBands) - 0.5;
  }
  
  qDebug() << "LOG SCALE:";
  for (int i = 0; i <= nBands; i++) {
    printf("%f ", logScale[i]);
  }
  printf("\n");
  
  // initialize bands
  std::memset(bands, 0, nBands * sizeof(double));
  std::memset(delay, 0, nBands * sizeof(int));
}

Analysis::~Analysis()
{
  delete[] bands;
  delete[] delay;
  delete[] logScale;
}

void Analysis::updateBands(const double* fftData)
{
  //QMutexLocker(&mutex);
  
  for (int i = 0; i < nBands; i++) {
    int a = ceil(logScale[i]);
    int b = floor(logScale[i+1]);
    double n = 0; // accumulator
    double x = 0;
    
    if (b < a) {
      n += fftData[b] * (logScale[i + 1] - logScale[i]);
    } else {
      if (a > 1) {
        n += fftData[a - 1] * (a - logScale[i]);
      }
      while (a < b) {
        n += fftData[a];
        a++;
      }
      if (b < fftSize / 2) {
        n += fftData[b] * (logScale[i + 1] - b);
      }
    }
    
    //x = 60 + 80 * log10(n / 200);
    x = 50 + 60 * log10(n / 120);
    bands[i] = std::max(0.0, std::min(x, 250.0));
    
//     if (n > 0) {
//       // 40dB range clamp
//       x = 20 * log10(n);
//       x = std::max(0.0, std::min(x, 40.0));
//     }
//       
//     bands[i] -= std::max(0, VIS_FALLOFF - delay[i]);
//     if (bands[i] < 0) {
//       bands[i] = 0;
//     }
//     if (delay[i] > 0) {
//       delay[i]--;
//     }
//     if (x > bands[i]) {
//       bands[i] = x;
//       delay[i] = VIS_DELAY;
//     }
    
//     double beatAvg = 0.0;
//     double beatLocal = 0.0;
//     for (int i = 0; i < PEAK_HISTORY_SIZE - 1; i++) {
//       beatHistory[i] = beatHistory[i+1];
//       beatAvg += beatHistory[i] / PEAK_HISTORY_SIZE;
//       if (i > PEAK_HISTORY_SIZE - PEAK_HISTORY_LOCAL) {
//         beatLocal += beatHistory[i] / PEAK_HISTORY_LOCAL;
//       }
//     }
//     beatHistory[PEAK_HISTORY_SIZE - 1] = bands[BEAT_BAND];
//     beatAvg += bands[BEAT_BAND] / PEAK_HISTORY_SIZE;
//     beatLocal += bands[BEAT_BAND] / PEAK_HISTORY_LOCAL;
//     
//     beatFactor = beatLocal / beatAvg;
  }
}

void Analysis::updatePeak(const double peak_)
{
  peak = peak_;
  smoothPeak = 0.0;
  averagePeak = 0.0;
  
  for (int i = 0; i < PEAK_HISTORY_SIZE - 1; i++) {
   peakHistory[i] = peakHistory[i+1]; 
   averagePeak += peakHistory[i] / PEAK_HISTORY_SIZE;
   if (i > PEAK_HISTORY_SIZE - PEAK_HISTORY_LOCAL) {
     smoothPeak += peakHistory[i] / PEAK_HISTORY_LOCAL;
   }
  }
  peakHistory[PEAK_HISTORY_SIZE - 1] = peak_;
  averagePeak += peak_ / PEAK_HISTORY_SIZE;
  smoothPeak += peak_ / PEAK_HISTORY_LOCAL;
  
  beatFactor = smoothPeak / averagePeak;
}

double Analysis::getPeak() const
{
  return peak;
}

double Analysis::getSmoothPeak() const
{
  return smoothPeak;
}

double Analysis::getAveragePeak() const
{
  return averagePeak;
}

double Analysis::getBeatFactor() const
{
  return beatFactor;
}

const double * Analysis::getBands() const
{
  return bands;
}

void Analysis::debugPrint()
{
  printf("BANDS:\n");
  for (int i = 0; i < nBands; i++) {
    printf("%.2f ", bands[i]);
  }
  printf("\n");
}

#include "analysis.moc"
