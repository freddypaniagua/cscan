#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QTextEdit>
#include "ScreenCapture.h"
#include "OCRProcessor.h"
#include "EconomyEngine.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow() = default;

private slots:
    void startScanning();
    void stopScanning();
    void onFrameReady(const QPixmap& frame);
    void onCaptureError(const QString& error);
    void onGameDataReady(const GameData& data);
    void onOCRError(const QString& error);
    void onRecommendationReady(const BuyRecommendation& recommendation);

private:
    void setupUI();
    void updateRecommendationDisplay(const BuyRecommendation& recommendation);

    // Core components
    ScreenCapture* m_screenCapture;
    OCRProcessor* m_ocrProcessor;
    EconomyEngine* m_economyEngine;

    // UI components
    QPushButton* m_startButton;
    QPushButton* m_stopButton;
    QLabel* m_statusLabel;
    QGroupBox* m_previewGroup;
    QLabel* m_previewLabel;
    QGroupBox* m_dataGroup;
    QLabel* m_moneyLabel;
    QLabel* m_scoreLabel;
    QLabel* m_sideLabel;
    QGroupBox* m_suggestionGroup;
    QLabel* m_suggestionLabel;
    QLabel* m_weaponLabel;
    QLabel* m_utilityLabel;
    QLabel* m_strategyLabel;

    bool m_isScanning;
    int m_frameCount;
};

#endif // MAINWINDOW_H