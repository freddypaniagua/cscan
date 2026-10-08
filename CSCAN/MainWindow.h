#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QTextEdit>
#include <QCheckBox>
#include <QSpinBox>
#include <QComboBox>
#include "ScreenCapture.h"
#include "OCRProcessor.h"
#include "EconomyEngine.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow() = default;

private slots:
    void startScanning();
    void stopScanning();
    void onFrameReady(const QPixmap &frame);
    void onCaptureError(const QString &error);
    void onGameDataReady(const GameData &data);
    void onOCRError(const QString &error);
    void onRecommendationReady(const BuyRecommendation &recommendation);
    void onDebugMessage(const QString &message);
    void onCalibrationNeeded(const QString &message);
    
    // Debug controls
    void toggleDebugMode();
    void saveDebugFrame();
    void openDebugFolder();
    void startCalibration();
    void adjustMoneyRegion();
    void adjustScoreboardRegion();

private:
    void setupUI();
    void setupDebugControls();
    void updateRecommendationDisplay(const BuyRecommendation &recommendation);
    QString formatGameStateInfo(const GameData &data);
    
    // Core components
    ScreenCapture *m_screenCapture;
    OCRProcessor *m_ocrProcessor;
    EconomyEngine *m_economyEngine;
    
    // Main UI components
    QPushButton *m_startButton;
    QPushButton *m_stopButton;
    QLabel *m_statusLabel;
    QGroupBox *m_previewGroup;
    QLabel *m_previewLabel;
    QGroupBox *m_dataGroup;
    QLabel *m_moneyLabel;
    QLabel *m_scoreLabel;
    QLabel *m_sideLabel;
    QLabel *m_gameStateLabel;
    QGroupBox *m_suggestionGroup;
    QLabel *m_suggestionLabel;
    QLabel *m_weaponLabel;
    QLabel *m_utilityLabel;
    QLabel *m_strategyLabel;
    
    // Debug UI components
    QGroupBox *m_debugGroup;
    QTextEdit *m_debugOutput;
    QCheckBox *m_debugModeCheckbox;
    QPushButton *m_saveDebugButton;
    QPushButton *m_openDebugFolderButton;
    QPushButton *m_calibrateButton;
    QPushButton *m_adjustMoneyButton;
    QPushButton *m_adjustScoreButton;
    QLabel *m_debugInfoLabel;
    
    // Region adjustment controls
    QSpinBox *m_moneyXSpin;
    QSpinBox *m_moneyYSpin;
    QSpinBox *m_moneyWSpin;
    QSpinBox *m_moneyHSpin;
    QLabel *m_regionInfoLabel;
    
    bool m_isScanning;
    int m_frameCount;
    QString m_lastDebugInfo;
};

#endif // MAINWINDOW_H
