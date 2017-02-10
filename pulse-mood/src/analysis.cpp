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

#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define YEL   "\x1B[33m"
#define BLU   "\x1B[34m"
#define MAG   "\x1B[35m"
#define CYN   "\x1B[36m"
#define WHT   "\x1B[37m"
#define RESET "\x1B[0m"
void beatPrint(const char* color, const char* type, int band, double intensity, double variance, double factor) {
  printf("%sBEAT: %s\tBAND: %d    LOCK: %.5f    VAR: %.5f    FACT: %.5f    RES: %.5f\n",
         color, type, band, intensity, variance, factor, 1 + (factor - 1) * intensity);
}

Analysis::Analysis(QObject* parent, const int fftSize) :
  QObject(parent),
  fft(fftSize),
  beatFFT(PEAK_HISTORY_SIZE),
  fftSize(fftSize)
{
  barkTable = new int[fftSize / 2];

  // initialize bark table
  int barkBand = 0;
  for (int i = 1; i < fftSize / 2; i++) {
    double freq = (double) SAMPLE_RATE / fftSize * i;
    while (barkBand < BARK_BANDS.size() - 1 && freq >= BARK_BANDS[barkBand]) {
      barkBand++;
    }
    qDebug() << SAMPLE_RATE << "/" << fftSize << "*" <<i << "=" << freq << "->" << barkBand;
    barkTable[i] = barkBand;
  }

  printf("\n");
  qDebug() << "BARK TABLE:";
  for (int i = 0; i < fftSize / 2; i++) {
    printf("%d ", barkTable[i]);
  }
  printf("\n");
  
  // initialize bands
  std::memset(beatBandsFactor, 0, BEAT_BANDS.size() * sizeof(double));
  std::memset(triSpectrumHistory, 0, 3 * PEAK_HISTORY_SIZE * sizeof(double));
  beatFactor = 1.0;
  lockOnFactor = 1.0;
  lockOnBand = -1;
  lockOnIntensity = 0;
}

Analysis::~Analysis()
{
  delete[] barkTable;
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
  std::memset(bands, 0, N_BANDS * sizeof(double));
  for (int i = 1; i < fftSize / 2; i++) {
    bands[barkTable[i]] += fftData[i] / (fftSize / 2);
  }

  for (int i = 0; i < BEAT_BANDS.size(); i++) {
    double avg = 0.0;
    double localAvg = 0.0;
    double var = 0.0;

    for (int j = 0; j < BEAT_HISTORY_SIZE - 1; j++) {
      beatHistory[i][j] = beatHistory[i][j+1];
      avg += beatHistory[i][j] / BEAT_HISTORY_SIZE;
      if (j >= BEAT_HISTORY_SIZE - BEAT_HISTORY_LOCAL) {
        localAvg += beatHistory[i][j] / BEAT_HISTORY_LOCAL;
      }
    }

    beatHistory[i][BEAT_HISTORY_SIZE - 1] = bands[BEAT_BANDS[i]];
    avg += bands[BEAT_BANDS[i]] / BEAT_HISTORY_SIZE;
    localAvg += bands[BEAT_BANDS[i]] / BEAT_HISTORY_LOCAL;
    beatAverage[i] = avg;

    for (int j = 0; j < BEAT_HISTORY_SIZE; j++) {
      var += pow(avg - beatHistory[i][j], 2);
    }
    beatVariance[i] = var;

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
  double newFactor = 1.0;
  for (int i = 0; i < BEAT_BANDS.size(); i++) {
    if (beatBandsFactor[i] - BEAT_THRESHOLD[i] > newFactor) {
      newFactor = beatBandsFactor[i] - 1;
      newBand = i;
    }
  }

  if (lockOnBand < 0 && newBand >= 0) { // new
    beatFactor = newFactor;
    lockOnFactor = newFactor;
    lockOnIntensity = bands[BEAT_BANDS[newBand]];
    lockOnBand = newBand;
    beatPrint(RED, "NEW", BEAT_BANDS[lockOnBand], lockOnIntensity, beatVariance[lockOnBand], beatFactor);
  } else if (newBand >= 0
          && bands[BEAT_BANDS[newBand]] > lockOnIntensity * 1.1
          && newFactor > beatFactor) { // switch band
    beatFactor = newFactor;
    lockOnFactor = newFactor;
    lockOnIntensity = bands[BEAT_BANDS[newBand]];
    lockOnBand = newBand;
    beatPrint(YEL, "SWITCH", BEAT_BANDS[lockOnBand], lockOnIntensity, beatVariance[lockOnBand], beatFactor);
  } else if (newBand >= 0 && newBand == lockOnBand
          && newFactor > beatFactor
          //&& newFactor > lockOnFactor * 0.9
          && bands[BEAT_BANDS[newBand]] > lockOnIntensity * 0.9) {
    beatFactor = newFactor;
    if (newFactor > lockOnFactor) lockOnFactor = newFactor;
    lockOnIntensity = bands[BEAT_BANDS[newBand]];
    beatPrint(CYN, "REFILL", BEAT_BANDS[lockOnBand], lockOnIntensity, beatVariance[lockOnBand], beatFactor);
  } else if (lockOnBand >= 0) { // decay
//    beatPrint(BLU, "DECAY", BEAT_BANDS[lockOnBand], lockOnIntensity, beatVariance[lockOnBand], beatFactor);
    beatFactor = beatFactor * 0.9;
    lockOnIntensity = lockOnIntensity * 0.9;
    if (beatFactor < 1.01) {
//      beatPrint(GRN, "FREE", BEAT_BANDS[lockOnBand], lockOnIntensity, beatVariance[lockOnBand], beatFactor);
      beatFactor = 1.0;
      lockOnFactor = 1.0;
      lockOnIntensity = 0;
      lockOnBand = -1;
    }
  }
}

void Analysis::updateTriSpectrum(const double *fftData) {

  double tri[3] = {0.0, 0.0, 0.0};
  for (int i = 0; i < BARK_BANDS.size(); i++) {
    if (i < 5) {
      tri[0] += bands[i] * bands[i];
    } else if (i < 18) {
      tri[1] += bands[i] * bands[i];
    } else {
      tri[2] += bands[i] * bands[i];
    }

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
  return 1 + (beatFactor - 1) * lockOnIntensity;
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
  for (int i = 0; i < N_BANDS; i++) {
    printf("%.2f ", bands[i]);
  }
  printf("\n");
}

#include "analysis.moc"
