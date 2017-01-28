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

#ifndef PULSEAUDIO_H
#define PULSEAUDIO_H

#include <QObject>

//#include <chrono>

extern "C" {
#include <pulse/error.h>
#include <pulse/glib-mainloop.h>
#include <pulse/context.h>
#include <pulse/introspect.h>
#include <pulse/subscribe.h>
#include <pulse/stream.h>
#include <pulse/sample.h>
}

class PulseAudio : public QObject
{
  Q_OBJECT
  
  public:
    PulseAudio(QObject* parent, const size_t bufferSize);
    ~PulseAudio();
    
    void startRecording();
    const double* getData() const;
    double getPeak() const;
    
    void debugPrint();
    
signals:
    void haveData();
    
  private:
    const size_t bufferSize;
    double *buffer; // we use double to store audio samples, so we can input directly to FFT
    double peak; // peak sample
    
    pa_context* paContext;
    pa_glib_mainloop* paMainloop;
    bool paConnected;
    
    pa_stream *recordStream;
    bool recording;
    
    // for debug
    size_t paHaveRead;
    
    void paRead();
    
    static void paStateCallback(pa_context *context, void *userdata);
    static void paStreamCallback(pa_stream *stream, void *userdata);
    static void paReadCallback(pa_stream *stream, size_t nBytes, void *userdata);
};

#endif // PULSEAUDIO_H
