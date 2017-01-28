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

#include "fft.h"

#include <iostream>
#include <cmath>
#include <stdio.h>
#include <string.h>

#include <QDebug>

FFT::FFT(const int fftSize) : 
  fftSize(fftSize)
{
  // init FFTW
  data = new fftw_complex[fftSize];
  plan = fftw_plan_dft_r2c_1d(fftSize, (double *) data, data, 0);
  
  // pre-compute hamming window
  hamming = new double[fftSize];
  for (int i = 0; i < fftSize; i++) {
    hamming[i] = HAMMING_ALPHA - HAMMING_BETA * cos(2 * M_PI * i / (fftSize - 1));
  }
}

FFT::~FFT()
{
  fftw_destroy_plan(plan);
  fftw_cleanup();
  delete[] data;
}


void FFT::pushData(const double *newData) {
  memcpy(data, newData, sizeof(double) * fftSize);
}

void FFT::pushDataFiltered(const double* newData)
{
  double *inData = (double *) data;
  
  for (int i = 0; i < fftSize; i++) {
    // multiply input data with hamming window
    inData[i] = newData[i] * hamming[i];
  }
}

void FFT::execute()
{
  // execute FFT
  fftw_execute(plan);
  
  // use absolute value of result (convert to real)
  double *outData = (double *) data; // re-interpret data array as double
  for (int i = 0; i < fftSize; i++) {
    double *val = (double *) (data + i);
    outData[i] = hypot(val[0], val[1]);
  }
}

const double* FFT::getData()
{
  return (const double *) data;
}

void FFT::debugPrint()
{
  printf("FFT Output:\n");
  const double *result = getData();
  for (int i = 0; i < fftSize / 2; i++) {
    printf("%f ", result[i]);
  }
  printf("\n");
}

