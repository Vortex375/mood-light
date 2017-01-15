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

#include "pulseaudio.h"

#include <QDebug>
#include <iostream>
#include <cmath>

#include <string.h>
#include <assert.h>

PulseAudio::PulseAudio(QObject* parent, const size_t bufferSize) :
  QObject(parent),
  bufferSize(bufferSize),
  paConnected(false),
  recording(false)
{
  // init buffer
  buffer = new double[bufferSize]{};
  
  // set up connection to pulseaudio
  paMainloop = pa_glib_mainloop_new(NULL);
  paContext = pa_context_new(pa_glib_mainloop_get_api(paMainloop), "Mood Monitor");
  pa_context_set_state_callback(paContext, PulseAudio::paStateCallback, this);
  // connect to pulseaudio server
  int ret = pa_context_connect(paContext, NULL, PA_CONTEXT_NOFLAGS, NULL);
  if (ret < 0) {
      qWarning() << "pa_context_connect() failed:" << pa_strerror(pa_context_errno(paContext));
      std::exit(1);
  }
  
  recordStream = NULL;
//  lastDebug = std::chrono::system_clock::now();
//   paReadTotal = 0;
//   paReadAttempts = 0;
}

PulseAudio::~PulseAudio()
{
  if (recording) {
    pa_stream_disconnect(recordStream);
  }
  if (recordStream != NULL) {
    pa_stream_unref(recordStream);
    recordStream = NULL;
  }
  
  if (paConnected) {
    pa_context_disconnect(paContext);
  }
  pa_context_unref(paContext);
  pa_glib_mainloop_free(paMainloop);
  
  delete[] buffer;
}

void PulseAudio::startRecording()
{
  pa_sample_spec sampleSpec = {PA_SAMPLE_FLOAT32, 48000, 1};
  pa_channel_map channelMap;
  pa_channel_map_init_mono(&channelMap);
  
  recordStream = pa_stream_new(paContext, "Mood Stream", &sampleSpec, &channelMap);
  pa_stream_set_state_callback(recordStream, PulseAudio::paStreamCallback, this);
  pa_stream_set_read_callback(recordStream, PulseAudio::paReadCallback, this);
  
  int ret = pa_stream_connect_record(recordStream, NULL, NULL, PA_STREAM_NOFLAGS);
  if (ret < 0) {
      qWarning() << "pa_context_connect() failed:" << pa_strerror(pa_context_errno(paContext));
      std::exit(1);
  }
}

void PulseAudio::paRead()
{
  size_t nBytes;
  size_t nSamples;
  const float* data;
  paHaveRead = 0;
  
  while(paHaveRead < bufferSize && pa_stream_readable_size(recordStream) > 0) {
    pa_stream_peek(recordStream, (const void**) &data, &nBytes);
  
    if (nBytes == 0) { // no data available
      break;
    }
    if (data == NULL) { // skip hole in data
      pa_stream_drop(recordStream);
      continue;
    }
    
    nSamples = nBytes / sizeof(float); // sample data is in PA_SAMPLE_FLOAT32 format
    assert(nSamples > 0);
    
    peak = 0.0;
    
    if (nSamples >= bufferSize) {
      //replace whole buffer
      for (int i = 0; i < bufferSize; i++) {
        buffer[i] = data[i];
        if (buffer[i] > peak) peak = buffer[i];
      }
    } else { // nSamples < bufferSize
      // shift contents of buffer making room for the new samples
      for (int i = 0; i < bufferSize - nSamples; i++) {
        buffer[i] = buffer[i+nSamples];
        if (buffer[i] > peak) peak = buffer[i];
      }
      int j = 0;
      for (int i = bufferSize - nSamples; i < bufferSize; i++) {
        buffer[i] = data[j++];
        if (buffer[i] > peak) peak = buffer[i];
      }
      assert(j == nSamples);
    }
    
    paHaveRead += nSamples;
//     paReadTotal += nSamples;
//     paReadAttempts++;
    pa_stream_drop(recordStream);
  }
  //qDebug() << "read" << nBytes << "bytes from pulseaudio.";
  
  emit haveData();
}


void PulseAudio::paReadCallback(pa_stream* stream, size_t nBytes, void* userdata)
{
  PulseAudio* that = (PulseAudio*) userdata;
  that->paRead();
}

void PulseAudio::paStateCallback(pa_context* context, void* userdata)
{
  PulseAudio* that = (PulseAudio*) userdata;
    
  pa_context_state_t state = pa_context_get_state(context);
  
  switch (state) {
    case PA_CONTEXT_UNCONNECTED:
    case PA_CONTEXT_CONNECTING:
    case PA_CONTEXT_AUTHORIZING:
    case PA_CONTEXT_SETTING_NAME:
      that->paConnected = false;
      break;
    case PA_CONTEXT_READY:
      qDebug() << "Successfully connected to pulseaudio";
      that->paConnected = true;
      
      // query information about the server (we are interested in the default sink)
      //pa_context_get_server_info(that->paContext, PAHelper::paServerInfoCallback, that);
      
      // subscribe to sink input events (we want to be notified when a new sink input is created,
      // so we can move it to our streaming sink while streaming is active)
      //pa_context_set_subscribe_callback(that->paContext, PAHelper::paContextSubscribeCallback, that);
      //pa_context_subscribe(that->paContext, PA_SUBSCRIPTION_MASK_ALL, NULL, NULL);
      
      that->startRecording();
      break;
    case PA_CONTEXT_FAILED:
      that->paConnected = false;
      qWarning() << "Connection to pulseaudio failed.";
      std::exit(2);
      break;
    case PA_CONTEXT_TERMINATED:
      that->paConnected = false;
      qDebug() << "Connection to pulseaudio closed.";
      std::exit(2);
      break;
  }
}

void PulseAudio::paStreamCallback(pa_stream* stream, void* userdata)
{
  PulseAudio* that = (PulseAudio*) userdata;
  const pa_sample_spec *sampleSpec;
    
  pa_stream_state_t state = pa_stream_get_state(stream);
  
  switch (state) {
    case PA_STREAM_UNCONNECTED:
    case PA_STREAM_CREATING:
       qDebug() << "Record Stream connecting...";
      that->recording = false;
      break;
    case PA_STREAM_READY:
      sampleSpec = pa_stream_get_sample_spec(stream);
      qDebug() << "Record Stream connected ( format =" << sampleSpec->format << "rate =" << sampleSpec->rate << "channels =" << sampleSpec->channels << ")";
      that->recording = true;
      break;
    case PA_STREAM_FAILED:
      qWarning() << "Record Stream failed.";
      that-> recording = false;
      break;
    case PA_STREAM_TERMINATED:
      qWarning() << "Record Stream closed.";
      that-> recording = false;
      break;
  }
}

const double * PulseAudio::getData() const
{
  return buffer;
}

double PulseAudio::getPeak() const {
  return peak;
}

void PulseAudio::debugPrint()
{
//   std::chrono::time_point<std::chrono::system_clock> now;
//   now = std::chrono::system_clock::now();
//   int elapsed_seconds = std::chrono::duration_cast<std::chrono::seconds>
//                              (now-lastDebug).count();
//   if (elapsed_seconds > 1) {
//     printf("SAMPLES READ: %d (%d)\n", paReadTotal, paReadAttempts);
//     paReadTotal = 0;
//     paReadAttempts = 0;
//     lastDebug = now;
//   }
  
  double max = 0;
  for (int i = 0; i < bufferSize; i++) {
    double val = std::abs(buffer[i]);
    if (val > max) max = val;
    //printf("%f ", buffer[i]);
  }
  printf("AUDIO DATA (peak): %f (%d)\n", max, paHaveRead);
  
}


#include "pulseaudio.moc"
