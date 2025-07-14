#include "ScreenCapture.h"
#include <QPixmap>
#include <QScreen>
#include <QGuiApplication>
#include <QDebug>

#ifdef _WIN32
#include <windows.h>
#include <dwmapi.h>
#endif

ScreenCapture::ScreenCapture(QObject* parent)
    : QObject(parent)
    , m_captureTimer(new QTimer(this))
    , m_targetScreen(QGuiApplication::primaryScreen())
    , m_currentMode(BASELINE)
    , m_isCapturing(false)
{
    connect(m_captureTimer, &QTimer::timeout, this, &ScreenCapture::captureFrame);
}

ScreenCapture::~ScreenCapture()
{
    stopCapture();
}

void ScreenCapture::startCapture()
{
    if (m_isCapturing) return;

    qDebug() << "Starting screen capture at" << m_currentMode << "FPS";
    setCaptureMode(BASELINE);
    m_captureTimer->start(1000 / m_currentMode); // Convert FPS to interval
    m_isCapturing = true;
}

void ScreenCapture::stopCapture()
{
    if (!m_isCapturing) return;

    qDebug() << "Stopping screen capture";
    m_captureTimer->stop();
    m_isCapturing = false;
}

void ScreenCapture::setCaptureRate(int fps)
{
    if (fps < 1) fps = 1;
    if (fps > 30) fps = 30;

    if (m_isCapturing) {
        m_captureTimer->setInterval(1000 / fps);
    }
}

void ScreenCapture::setCaptureMode(CaptureMode mode)
{
    m_currentMode = mode;
    if (m_isCapturing) {
        m_captureTimer->setInterval(1000 / mode);
        qDebug() << "Capture mode changed to" << mode << "FPS";
    }
}

void ScreenCapture::captureFrame()
{
    QPixmap screenshot = captureScreen();
    if (!screenshot.isNull()) {
        emit frameReady(screenshot);
    }
    else {
        emit captureError("Failed to capture screen");
    }
}

QPixmap ScreenCapture::captureScreen()
{
    // For now, capture the entire primary screen
    // Later we'll focus on CS2 window only
    QRect screenGeometry = m_targetScreen->geometry();
    QPixmap screenshot = m_targetScreen->grabWindow(0,
        screenGeometry.x(), screenGeometry.y(),
        screenGeometry.width(), screenGeometry.height());

    return screenshot;
}

QRect ScreenCapture::findCS2Window()
{
    // TODO: Implement CS2 window detection
    // For now, return full screen
    return m_targetScreen->geometry();
}