#ifndef PAHELPER_H
#define PAHELPER_H

#include <QCoreApplication>
#include <QHash>

extern "C" {
#include <pulse/error.h>
#include <pulse/glib-mainloop.h>
#include <pulse/context.h>
#include <pulse/introspect.h>
#include <pulse/subscribe.h>
}

#include "reader.h"

static const char* SINK_NAME = "ubidoo_rtp_sink";
static const char* PLAYBACK_NAME = "ubidoo_rtp_playback";

class PAHelper : public QCoreApplication
{
    Q_OBJECT

public:
    
    PAHelper(int argc, char ** argv);
    ~PAHelper();

public slots:
    void shutdown();
    
    void startStream();
    void stopStream();
    
    void handleInput(QString input);
    
private:
    // callbacks for pulseaudio async API
    static void paServerInfoCallback(pa_context *c, const pa_server_info *i, void *userdata);
    static void paStateCallback(pa_context *content, void *userdata);
    static void paSinkInfoCallback(pa_context *c, const pa_sink_info *i, int eol, void *userdata);
    static void paLoadSinkModuleCallback(pa_context *c, uint32_t idx, void *userdata);
    static void paPrepareMoveCallback(pa_context *c, const pa_sink_input_info *i, int eol, void *userdata);
    static void paPrepareRestoreCallback(pa_context *c, const pa_sink_input_info *i, int eol, void *userdata);
    static void paNewSinkInputCallback(pa_context *c, const pa_sink_input_info *i, int eol, void *userdata);
    
    static void paContextSubscribeCallback(pa_context *c, pa_subscription_event_type_t t, uint32_t idx, void *userdata);
    
    void findSink();
    void createSink();
    
    bool paConnected;
    int paSinkModuleIdx;
    int paStreamModuleIdx;
    int paSinkIndex;
    char* paFallbackSinkName;
    bool foundSink;
    bool createdSink;
    bool streamActive;
    
    QHash<int, int> streamRestoreMap;
    
    pa_context* paContext;
    pa_glib_mainloop* paMainloop;
    Reader reader;
};

#endif // PAHELPER_H
