#ifndef SCREENCAPTURE_H
#define SCREENCAPTURE_H

#include <QObject>
#include <QTimer>
#include <QPixmap>
#include <QScreen>
#include <QGuiApplication>

class ScreenCapture : public QObject
{
    Q_OBJECT

public:
    explicit ScreenCapture(QObject* parent = nullptr);
    ~ScreenCapture();

    void startCapture();
    void stopCapture();
    void setCaptureRate(int fps);

    // Capture modes for hybrid system
    enum CaptureMode {
        BASELINE = 3,      // 3 FPS - normal scanning
        ROUND_END = 10,    // 10 FPS - round ending
        BUY_PHASE = 6      // 6 FPS - buy phase
    };

    void setCaptureMode(CaptureMode mode);

signals:
    void frameReady(const QPixmap& frame);
    void captureError(const QString& error);

private slots:
    void captureFrame();

private:
    QTimer* m_captureTimer;
    QScreen* m_targetScreen;
    CaptureMode m_currentMode;
    bool m_isCapturing;

    QPixmap captureScreen();
    QRect findCS2Window();
};

#endif // SCREENCAPTURE_H