#include "pahelper.h"
#include <QDebug>
#include <QCoreApplication>
#include <QApplication>
#include <QTimer>

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include "pulseaudio.h"
#include "analysis.h"
#include "debugview.h"
#include "serial.h"

// redirect qDebug() and friends to stderr
void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
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

static QCoreApplication *instance = NULL;

void signal_callback_handler(int signum) {
    if (instance) {
        instance->quit();
    }
}

#define FRAME_SIZE      2048
#define FRAME_DURATION    20 // 50 FPS
#define ANALYSIS_BANDS    20

int main(int argc, char **argv) {
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

    Serial serial(&app);
    PulseAudio pulseAudio(&app, FRAME_SIZE);
    Analysis analysis(&app, FRAME_SIZE, ANALYSIS_BANDS);
    DebugView debugView(&analysis, ANALYSIS_BANDS);

    QColor baseColor(255, 180, 107);
    baseColor = baseColor.toHsv();

    QTimer frameTimer;
    frameTimer.setInterval(FRAME_DURATION);
    app.connect(&frameTimer, &QTimer::timeout, [&]() {
        //pulseAudio.debugPrint();
        //printf("PEAK: %f\n", pulseAudio.getPeak());
        analysis.update(pulseAudio.getData());
        analysis.updatePeak(pulseAudio.getPeak());
        //const double* bands = analysis.getBands();

        //printf("BEAT: %f\n", analysis.getBeatFactor());
        //analysis.debugPrint();
        debugView.update();

        // write to
        int h,s,v;
        baseColor.getHsv(&h, &s, &v);
        v = std::max(0, std::min(255, (int) (v * analysis.getAveragePeak())));
        QColor currentColor = QColor::fromHsv(h, s, v);
        //qDebug() << "H:" << h << "S:" << s << "V:" << v;
        currentColor.getRgb(&h, &s, &v); // misusing hsv as rgb
        uint32_t colorValue = ((h & 0xFF) << 24) | ((s & 0xFF) << 16) | ((v & 0xFF) << 8);

        serial.writePixelValue(colorValue);
    });


    qDebug() << "Starting main loop...";

    debugView.show();
    frameTimer.start();

    return app.exec();
}
