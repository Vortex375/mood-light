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

constexpr std::array<int, 5> Analysis::BEAT_BANDS;
constexpr std::array<double, 5> Analysis::BEAT_THRESHOLD;
constexpr std::array<int, 24> Analysis::BARK_BANDS;

Analysis::Analysis(QObject* parent, const int fftSize, const int nBands) :
  QObject(parent),
  fft(fftSize),
  beatFFT(PEAK_HISTORY_SIZE),
  fftSize(fftSize),
  nBands(nBands)
{
  bands = new double[nBands];
  logScale = new float[nBands + 1];
  barkTable = new int[fftSize / 2];
  
  // initialize logarithmic scale
  for (int i = 0; i <= nBands; i++) {
    logScale[i] = pow(fftSize / 2, (float) i / nBands) - 0.5;
  }

  // initialize bark table
  int barkBand = 0;
  for (int i = 0; i < fftSize / 2; i++) {
    double freq = (double) SAMPLE_RATE / fftSize * i;
    while (barkBand < BARK_BANDS.size() - 1 && freq >= BARK_BANDS[barkBand]) {
      barkBand++;
    }
    qDebug() << SAMPLE_RATE << "/" << fftSize << "*" <<i << "=" << freq << "->" << barkBand;
    barkTable[i] = barkBand;
  }
  
  qDebug() << "LOG SCALE:";
  for (int i = 0; i <= nBands; i++) {
    printf("%f ", logScale[i]);
  }
  printf("\n");
  qDebug() << "BARK TABLE:";
  for (int i = 0; i < fftSize / 2; i++) {
    printf("%d ", barkTable[i]);
  }
  printf("\n");
  
  // initialize bands
  std::memset(bands, 0, nBands * sizeof(double));
  std::memset(beatBandsFactor, 0, BEAT_BANDS.size() * sizeof(double));
  std::memset(triSpectrumHistory, 0, 3 * PEAK_HISTORY_SIZE * sizeof(double));
  beatFactor = 1.0;
  lockOnFactor = 1.0;
  lockOnBand = -1;
  lockOnIntensity = 0;
}

Analysis::~Analysis()
{
  delete[] bands;
  delete[] logScale;
}

void Analysis::update(const double *audioData)
{
  fft.pushDataFiltered(audioData);
  fft.execute();

  updateBands(fft.getData());
  updateTriSpectrum(fft.getData());
}

void Analysis::updatePeak(const double peak_)
{
  peak = peak_ * peak_;
  smoothPeak = 0.0;
  averagePeak = 0.0;

  for (int i = 0; i < PEAK_HISTORY_SIZE - 1; i++) {
   peakHistory[i] = peakHistory[i+1];
   averagePeak += peakHistory[i] / PEAK_HISTORY_SIZE;
   if (i >= PEAK_HISTORY_SIZE - PEAK_HISTORY_LOCAL) {
     smoothPeak += peakHistory[i] / PEAK_HISTORY_LOCAL;
   }
  }
  peakHistory[PEAK_HISTORY_SIZE - 1] = peak;
  averagePeak += peak / PEAK_HISTORY_SIZE;
  smoothPeak += peak / PEAK_HISTORY_LOCAL;

  averagePeak = sqrt(averagePeak);
  smoothPeak = sqrt(smoothPeak);

  beatFFT.pushDataFiltered(peakHistory);
  beatFFT.execute();
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

    x = 50 + 60 * log10(n / 120);
    bands[i] = std::max(0.0, std::min(x, 250.0));
  }

  //beatFactor = 1.0;
  for (int i = 0; i < BEAT_BANDS.size(); i++) {
    double avg = 0.0;
    double localAvg = 0.0;
    for (int j = 0; j < BEAT_HISTORY_SIZE - 1; j++) {
      beatHistory[i][j] = beatHistory[i][j+1];
      avg += beatHistory[i][j] / BEAT_HISTORY_SIZE;
      if (j >= BEAT_HISTORY_SIZE - BEAT_HISTORY_LOCAL) {
        localAvg += beatHistory[i][j] / BEAT_HISTORY_LOCAL;
      }
    }
    beatHistory[i][BEAT_HISTORY_SIZE - 1] = pow(bands[BEAT_BANDS[i]] / 100, 2);
    avg += beatHistory[i][BEAT_HISTORY_SIZE - 1] / BEAT_HISTORY_SIZE;
    localAvg += beatHistory[i][BEAT_HISTORY_SIZE - 1] / BEAT_HISTORY_LOCAL;
    beatAverage[i] = avg;
    /*if (avg > 0.2 || localAvg > 0.2) { // mask small local maxima
      beatBandsFactor[i] = (beatBandsFactor[i] + std::max(1.0, localAvg / avg)) / 2;
    } else {
      beatBandsFactor[i] = 1.0;
    }*/
    if (avg > 0.2 || localAvg > 0.2) {
      beatBandsFactor[i] = std::max(1.0, localAvg / avg);
    } else {
      beatBandsFactor[i] = 1.0;
    }
  }
  updateBeatFactor();
//  if (newPeak > beatFactor && newPeak > BEAT_THRESHOLD) {
//    beatFactor = newPeak;
//  } else if (beatFactor > 1.0) {
//    // decay
//    beatFactor = beatFactor - (beatFactor - 1) / 2;
//  }
}

void Analysis::updateBeatFactor() {
  int newBand = -1;
  double newFactor = 0.0;
  for (int i = 0; i < BEAT_BANDS.size(); i++) {
    if (beatBandsFactor[i] - BEAT_THRESHOLD[i] > newFactor + (0.02/(beatAverage[i]*beatAverage[i]))) {
      newFactor = beatBandsFactor[i] - 1;
      newBand = i;
    }
  }
  newFactor += 1;

  if (lockOnBand < 0 && newBand >= 0) { // new
    beatFactor = newFactor;
    lockOnFactor = newFactor;
    lockOnIntensity = bands[BEAT_BANDS[newBand]];
    lockOnBand = newBand;
    qDebug() << "BEAT: NEW" << "\t" << "BAND:" << BEAT_BANDS[lockOnBand] << "LOCK:" << lockOnIntensity << "\t" << beatFactor;
  } else if (newBand >= 0
          && bands[BEAT_BANDS[newBand]] > lockOnIntensity * 1.1
          && newFactor > beatFactor) { // switch band
    beatFactor = newFactor;
    lockOnFactor = newFactor;
    lockOnIntensity = bands[BEAT_BANDS[newBand]];;
    lockOnBand = newBand;
    qDebug() << "BEAT: SWITCH" << "\t" << "BAND:" << BEAT_BANDS[lockOnBand] << "LOCK:" << lockOnIntensity << "\t"
             << beatFactor;
  } else if (newBand >= 0 && newBand == lockOnBand
          && newFactor > beatFactor
          //&& newFactor > lockOnFactor * 0.9
          && bands[BEAT_BANDS[newBand]] > lockOnIntensity * 0.9) {
    beatFactor = newFactor;
    if (newFactor > lockOnFactor) lockOnFactor = newFactor;
    lockOnIntensity = bands[BEAT_BANDS[newBand]];
    qDebug() << "BEAT: REFILL" << "\t" << "BAND:" << BEAT_BANDS[lockOnBand] << "LOCK:" << lockOnIntensity << "\t" << beatFactor;
  } else if (lockOnBand >= 0) { // decay
    qDebug() << "BEAT: DECAY" << "\t" << "BAND:" << BEAT_BANDS[lockOnBand] << "LOCK:" << lockOnIntensity << "\t" << beatFactor;
    beatFactor = beatFactor - (beatFactor - 1) / 6;
    lockOnIntensity = lockOnIntensity * 0.98;
    if (beatFactor < 1.01) {
      qDebug() << "BEAT: FREE";
      beatFactor = 1.0;
      lockOnFactor = 1.0;
      lockOnIntensity = 0;
      lockOnBand = -1;
    }
  }
}



void Analysis::updateTriSpectrum(const double *fftData) {
  std::memset(barkBands, 0, BARK_BANDS.size() * sizeof(double));

  for (int i = 0; i < fftSize / 2; i++) {
    barkBands[barkTable[i]] += fftData[i] / (fftSize / 2);
  }

  double tri[3] = {0.0, 0.0, 0.0};
  for (int i = 0; i < BARK_BANDS.size(); i++) {
    tri[i * 3 / BARK_BANDS.size()] += barkBands[i] * barkBands[i];
    //tri[i * 3 / BARK_BANDS.size()] += std::max(0.0, 50 + 60 * log10(barkBands[i] / 120));
  }

  double avg[3] = {0.0, 0.0, 0.0};
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < PEAK_HISTORY_SIZE - 1; j++) {
      triSpectrumHistory[i][j] = triSpectrumHistory[i][j + 1];
      avg[i] += triSpectrumHistory[i][j] / PEAK_HISTORY_SIZE;
    }
    triSpectrumHistory[i][PEAK_HISTORY_SIZE - 1] = sqrt(tri[i]);
    avg[i] += sqrt(tri[i]) / PEAK_HISTORY_SIZE;
  }

  // normalize
  double max = std::max(avg[0], std::max(avg[1], avg[2]));
  if (max > 0) {
    for (int i = 0; i < 3; i++) {
      avg[i] /= max;
      triSpectrum[i] = avg[i];
    }
  } else {
    for (int i = 0; i < 3; i++) {
      triSpectrum[i] = 0.0;
    }
  }
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
  return 1 + (beatFactor - 1) * pow((lockOnIntensity) / 100, 2);
}

const double * Analysis::getBands() const
{
  return bands;
}

const double *Analysis::getBeatBandsAverage() const {
  return beatAverage;
}

const double *Analysis::getBeatBandsFactor() const {
  return beatBandsFactor;
}

const double *Analysis::getTriSpectrum() const {
  return triSpectrum;
}

int Analysis::getBeatLockOnBand() const {
  return lockOnBand;
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
