#include "pahelper.h"
#include <QDebug>
#include <QCoreApplication>
#include <QApplication>

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include "fft.h"
#include "pulseaudio.h"
#include "analysis.h"
#include "debugview.h"

// redirect qDebug() and friends to stderr
void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QByteArray localMsg = msg.toLocal8Bit();
    switch (type) {
    case QtDebugMsg:
        fprintf(stderr, "Debug: %s\n", localMsg.constData());
        break;
#if QT_VERSION >= 0x050500
    case QtInfoMsg:
        fprintf(stderr, "Info: %s\n", localMsg.constData());
        break;
#endif
    case QtWarningMsg:
        fprintf(stderr, "Warning: %s\n", localMsg.constData());
        break;
    case QtCriticalMsg:
        fprintf(stderr, "Critical: %s\n", localMsg.constData());
        break;
    case QtFatalMsg:
        fprintf(stderr, "Fatal: %s\n", localMsg.constData());
        abort();
    }
}

static QCoreApplication* instance = NULL;

void signal_callback_handler(int signum)
{
    if (instance) {
        instance->quit();
    }
}

int main(int argc, char** argv) {
    // register signal traps
    // this is so we can clean up and remove the pulseaudio sink we created
    // before terminating the program
    signal(SIGINT, signal_callback_handler);
    signal(SIGTERM, signal_callback_handler);
    
    // register message handler to redirect all output to stderr
    qInstallMessageHandler(myMessageOutput);
    
    // initialize main application
    QApplication app(argc, argv);
    instance = &app;
    
    //FFT fft(256);
    //fft.debugPrint();
    
    PulseAudio pulseAudio(&app, 2048);
    FFT fft(2048);
    Analysis analysis(&app, 2048, 20);
    DebugView debugView(&analysis, 20);
    
    app.connect(&pulseAudio, &PulseAudio::haveData, [&]() {
      //pulseAudio.debugPrint();
      printf("PEAK: %f\n", pulseAudio.getPeak());
      const double* data = pulseAudio.getData();
      fft.pushDataFiltered(data);
      fft.execute();
      //fft.debugPrint();
      const double* fftData = fft.getData();
      analysis.updatePeak(pulseAudio.getPeak());
      analysis.updateBands(fftData);
      //const double* bands = analysis.getBands();
      
      printf("BEAT: %f\n", analysis.getBeatFactor());
      analysis.debugPrint();
      debugView.update();
    });
    
    
    qDebug() << "Starting main loop...";
    
    debugView.show();
    
    return app.exec();
}
