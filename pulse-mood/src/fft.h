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

#ifndef FFT_H
#define FFT_H

#include <fftw3.h>

#define HAMMING_ALPHA 0.54
#define HAMMING_BETA  0.46

class FFT
{
  public:
    FFT(const int fftSize);
    ~FFT();

    void pushData(const double *newData);
    void pushDataFiltered(const double *newData); // push new input data, filtered by Hamming window
    void execute();
    
    const double* getData(); // return (fftSize / 2) real-valued DFT results
    
    void debugPrint();
    
  private:
    const int fftSize;
    
    fftw_plan plan;
    fftw_complex *data;
    double *hamming;
};

#endif // FFT_H
