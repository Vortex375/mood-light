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

#ifndef ANALYSIS_H
#define ANALYSIS_H

#include <QObject>
#include <QTime>

#include "fft.h"

#define VIS_DELAY   2
#define VIS_FALLOFF 2

// how many samples to keep for sliding window
// for smoothing (average) or derivative
#define PEAK_HISTORY_SIZE  25               // about 500ms
#define PEAK_HISTORY_LOCAL 4                // about  80ms
#define BEAT_HISTORY_LOCAL 1                // about  40ms

class Analysis : public QObject
{
  Q_OBJECT
  
  public:
    Analysis(QObject *parent, const int fftSize, const int nBands);
    ~Analysis();

    void update(const double *audioData);
    void updatePeak(const double peak);
    const double* getBands() const;
    const double* getBeatBandsAverage() const;
    const double* getBeatBandsFactor() const;
    double getPeak() const;
    double getSmoothPeak() const;
    double getAveragePeak() const;
    double getBeatFactor() const;
    
    void debugPrint();

    constexpr static std::array<int, 4> BEAT_BANDS = {3, 4, 5, 6};
    
  private:
    //QMutex mutex;

    FFT fft;
    FFT beatFFT;
    QTime debugTime;

    const int fftSize;
    const int nBands;
    
    float *logScale;
    double *bands;
    
    double peak;
    double peakHistory[PEAK_HISTORY_SIZE];
    double beatHistory[BEAT_BANDS.size()][PEAK_HISTORY_SIZE];
    double beatAverage[BEAT_BANDS.size()];
    double beatBandsFactor[BEAT_BANDS.size()];
    double averagePeak;
    double smoothPeak;
    double beatFactor;

    void updateBands(const double* fftData);
};

#endif // ANALYSIS_H
