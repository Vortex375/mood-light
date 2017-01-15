#include "pahelper.h"

#include <QDebug>
#include <QThread>
#include <iostream>

PAHelper::PAHelper(int argc, char** argv) : QCoreApplication(argc, argv)
{
    // create pulseaudio main loop and context objects
    createdSink = false;
    paConnected = false;
    paSinkModuleIdx = -1;
    paStreamModuleIdx = -1;
    streamActive = false;
        
    // set up connection to pulseaudio
    paMainloop = pa_glib_mainloop_new(NULL);
    paContext = pa_context_new(pa_glib_mainloop_get_api(paMainloop), "ubidoo-pulseaudio-helper");
    pa_context_set_state_callback(paContext, PAHelper::paStateCallback, this);
    // connect to pulseaudio server
    int ret = pa_context_connect(paContext, NULL, PA_CONTEXT_NOFLAGS, NULL);
    if (ret < 0) {
        qWarning() << "pa_context_connect() failed:" << pa_strerror(pa_context_errno(paContext));
        std::exit(1);
    }
    
    // read commands from stdin and exit when stdin is closed
    QThread * readerThread = new QThread();
    reader.moveToThread(readerThread);
    connect(&reader, SIGNAL(data(QString)), this, SLOT(handleInput(QString)));
    connect(&reader, SIGNAL(finished()), this, SLOT(shutdown()));
    
    // reader thread startup and shutdown
    connect(readerThread, SIGNAL(started()), &reader, SLOT(read()));
    connect(&reader, SIGNAL(finished()), readerThread, SLOT(quit()));
    connect(readerThread, SIGNAL(finished()), readerThread, SLOT(deleteLater()));
    
    readerThread->start();
}

PAHelper::~PAHelper()
{
    if (paConnected) {
        pa_context_disconnect(paContext);
    }
    pa_context_unref(paContext);
    pa_glib_mainloop_free(paMainloop);
}

void PAHelper::shutdown()
{
    // unload the modules that we loaded earlier (it's nice to clean up after ourselves)
    if (paConnected) {
        if (paStreamModuleIdx >= 0) {
            pa_context_unload_module(paContext, paStreamModuleIdx, NULL, NULL);
        }
        if (paSinkModuleIdx >= 0) {
            pa_context_unload_module(paContext, paSinkModuleIdx, NULL, NULL);
        }
        
    }
    exit(0);
}

void PAHelper::paServerInfoCallback(pa_context* c, const pa_server_info* i, void* userdata)
{
    PAHelper* that = (PAHelper*) userdata;
    
    that->paFallbackSinkName = strdup(i->default_sink_name);
}


void PAHelper::findSink()
{
    // check if our rtp sink is already available
    foundSink = false;
    pa_context_get_sink_info_list(paContext, PAHelper::paSinkInfoCallback, this);
}

void PAHelper::paSinkInfoCallback(pa_context* c, const pa_sink_info* i, int eol, void* userdata)
{
    PAHelper* that = (PAHelper*) userdata;
    
    if (eol) {
        if (!that->foundSink) {
            if (that->createdSink) {
                // already attempted to create the sink but still unable to find it
                qWarning() << "Failed to create pulseaudio rtp sink.";
                that->exit(1);
                return;
            }
            that->createSink();
        }
        return;
    }
    
    qDebug() << "found sink:" << i->name;
    
    if (strcmp(i->name, SINK_NAME) == 0) {
        that->foundSink = true;
        that->paSinkIndex = i->index;
    }
}

void PAHelper::createSink()
{
    QStringList arguments;
    arguments << "sink_name=" << SINK_NAME;
    arguments << " format=s16be channels=2 rate=44100 sink_properties=device.description=\"UBIDOO_RTP_Multicast_Sink\"";
    QString arg = arguments.join("");
    
    qDebug() << SINK_NAME << " not found. Attempting to create sink...";
    pa_context_load_module(paContext, 
                           "module-null-sink", 
                           arg.toUtf8().constData(),
                           PAHelper::paLoadSinkModuleCallback,
                           this);
                           
}

void PAHelper::paLoadSinkModuleCallback(pa_context* c, uint32_t idx, void* userdata)
{
    PAHelper* that = (PAHelper*) userdata;
    
    qDebug() << "sink created:" << idx;
    that->paSinkModuleIdx = idx;
    that->createdSink = true;
    
    // disable loading of module rtp-send for now
    //that->loadStreamModule();
    that->findSink();
    
}

void PAHelper::startStream()
{
    streamActive = true;
    
    // iterate over all sink inputs and move them to our streaming sink
    pa_context_get_sink_input_info_list(paContext, PAHelper::paPrepareMoveCallback, this);
}

void PAHelper::paPrepareMoveCallback(pa_context* c, const pa_sink_input_info* i, int eol, void* userdata)
{
    PAHelper* that = (PAHelper*) userdata;
    
    if (eol) {
        // all streams moved
        return;
    }
    
    // do not move our rtp playback stream
    if (strcmp(pa_proplist_gets(i->proplist, PA_PROP_APPLICATION_NAME), PLAYBACK_NAME) == 0) {
        return;
    }
    
    // do not move stream if unnecessary
    if (i->sink == that->paSinkIndex) {
        return;
    }
    
    qDebug() << "moving sink input " << i->name << "to sink " << QString::number(that->paSinkIndex);
    
    // store the sink that this sink input was previously connected to,
    // so we can restore it later
    that->streamRestoreMap[i->index] = i->sink;
    pa_context_move_sink_input_by_index(that->paContext, i->index, that->paSinkIndex, NULL, NULL);
}

void PAHelper::stopStream()
{
    streamActive = false;
    
    // move all sink inputs back to their original sinks
    pa_context_get_sink_input_info_list(paContext, PAHelper::paPrepareRestoreCallback, this);
}

void PAHelper::paPrepareRestoreCallback(pa_context* c, const pa_sink_input_info* i, int eol, void* userdata)
{
    PAHelper* that = (PAHelper*) userdata;
    
    if (eol) {
        // all streams restored
        that->streamRestoreMap.clear();
        return;
    }
    
    // only move streams that are currently running on our sink
    if (i->sink != that->paSinkIndex) {
        if (that->streamRestoreMap.contains(i->index)) {
            that->streamRestoreMap.remove(i->index);
        }
        return;
    }
    
    if (that->streamRestoreMap.contains(i->index)) {
        // move sink input back to its original sink
        qDebug() << "moving sink input " << i->name << "to sink " << QString::number(that->streamRestoreMap[i->index]);
        pa_context_move_sink_input_by_index(that->paContext, i->index, that->streamRestoreMap[i->index], NULL, NULL);
    } else {
        // move sink input to default sink
        qDebug() << "moving sink input " << i->name << "to default sink " << that->paFallbackSinkName;
        pa_context_move_sink_input_by_name(that->paContext, i->index, that->paFallbackSinkName, NULL, NULL);
    }
}

void PAHelper::paContextSubscribeCallback(pa_context* c, pa_subscription_event_type_t t, uint32_t idx, void* userdata)
{
    PAHelper* that = (PAHelper*) userdata;
    
    if (t & (PA_SUBSCRIPTION_EVENT_SINK_INPUT | PA_SUBSCRIPTION_EVENT_NEW)) {
        // query information about the new sink input
        pa_context_get_sink_input_info(that->paContext, idx, PAHelper::paNewSinkInputCallback, that);
    }
}

void PAHelper::paNewSinkInputCallback(pa_context* c, const pa_sink_input_info* i, int eol, void* userdata)
{
    PAHelper* that = (PAHelper*) userdata;
    
    if (eol) {
        return;
    }
    
    // a new sink input was created
    std::cout << "sink-input-new \"" << pa_proplist_gets(i->proplist, PA_PROP_APPLICATION_NAME) << "\"" << std::endl;
    
    if (that->streamActive) {
        // move it to ubidoo UBIDOO_RTP_Multicast_Sink
        pa_context_get_sink_input_info(that->paContext, i->index, PAHelper::paPrepareMoveCallback, that);
    } else {
        // make sure it is not on UBIDOO_RTP_Multicast_Sink
        pa_context_get_sink_input_info(that->paContext, i->index, PAHelper::paPrepareRestoreCallback, that);
    }
}


void PAHelper::handleInput(QString input)
{
    //qDebug() << "read from stdin: " << input;
    QStringList args = input.split(" ");
    
    if (args.size() > 0) {
        QString command = args[0].trimmed();
        
        if (command == "start-stream") {
            startStream();
        } else if (command == "stop-stream") {
            stopStream();
        }
    }
}

void PAHelper::paStateCallback(pa_context* context, void* userdata)
{
    PAHelper* that = (PAHelper*) userdata;
    
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
            pa_context_get_server_info(that->paContext, PAHelper::paServerInfoCallback, that);
            
            // subscribe to sink input events (we want to be notified when a new sink input is created,
            // so we can move it to our streaming sink while streaming is active)
            pa_context_set_subscribe_callback(that->paContext, PAHelper::paContextSubscribeCallback, that);
            pa_context_subscribe(that->paContext, PA_SUBSCRIPTION_MASK_ALL, NULL, NULL);
            
            that->findSink();
            break;
        case PA_CONTEXT_FAILED:
            that->paConnected = false;
            qWarning() << "Connection to pulseaudio failed.";
            that->exit(2);
            break;
        case PA_CONTEXT_TERMINATED:
            that->paConnected = false;
            qDebug() << "Connection to pulseaudio closed.";
            that->exit(0);
            break;
    }
}

#include "pahelper.moc"
