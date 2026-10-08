#include "MainWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QWidget>
#include <QGroupBox>
#include <QDebug>
#include <QTextEdit>
#include <QCheckBox>
#include <QSpinBox>
#include <QDesktopServices>
#include <QUrl>
#include <QStandardPaths>
#include <QSplitter>
#include <QTabWidget>
#include <QTime>
#include <QTextCursor>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_screenCapture(new ScreenCapture(this))
    , m_ocrProcessor(new OCRProcessor(this))
    , m_economyEngine(new EconomyEngine(this))
    , m_isScanning(false)
    , m_frameCount(0)
{
    setupUI();
    
    // Connect screen capture signals
    connect(m_screenCapture, &ScreenCapture::frameReady, this, &MainWindow::onFrameReady);
    connect(m_screenCapture, &ScreenCapture::captureError, this, &MainWindow::onCaptureError);
    
    // Connect OCR processor signals
    connect(m_ocrProcessor, &OCRProcessor::gameDataReady, this, &MainWindow::onGameDataReady);
    connect(m_ocrProcessor, &OCRProcessor::processingError, this, &MainWindow::onOCRError);
    connect(m_ocrProcessor, &OCRProcessor::debugMessage, this, &MainWindow::onDebugMessage);
    connect(m_ocrProcessor, &OCRProcessor::calibrationNeeded, this, &MainWindow::onCalibrationNeeded);
    
    // Connect economy engine signals
    connect(m_economyEngine, &EconomyEngine::recommendationReady, this, &MainWindow::onRecommendationReady);
    
    // Connect button signals
    connect(m_startButton, &QPushButton::clicked, this, &MainWindow::startScanning);
    connect(m_stopButton, &QPushButton::clicked, this, &MainWindow::stopScanning);
    
    // Connect debug controls
    connect(m_debugModeCheckbox, &QCheckBox::toggled, this, &MainWindow::toggleDebugMode);
    connect(m_saveDebugButton, &QPushButton::clicked, this, &MainWindow::saveDebugFrame);
    connect(m_openDebugFolderButton, &QPushButton::clicked, this, &MainWindow::openDebugFolder);
    connect(m_calibrateButton, &QPushButton::clicked, this, &MainWindow::startCalibration);
    connect(m_adjustMoneyButton, &QPushButton::clicked, this, &MainWindow::adjustMoneyRegion);
    connect(m_adjustScoreButton, &QPushButton::clicked, this, &MainWindow::adjustScoreboardRegion);
    
    // Connect region adjustment spinboxes
    connect(m_moneyXSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::adjustMoneyRegion);
    connect(m_moneyYSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::adjustMoneyRegion);
    connect(m_moneyWSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::adjustMoneyRegion);
    connect(m_moneyHSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::adjustMoneyRegion);
    
    // Initialize debug mode
    m_debugModeCheckbox->setChecked(true);
    toggleDebugMode();
    
    // Show current regions
    QRect moneyRegion = m_ocrProcessor->getMoneyRegion();
    m_moneyXSpin->setValue(moneyRegion.x());
    m_moneyYSpin->setValue(moneyRegion.y());
    m_moneyWSpin->setValue(moneyRegion.width());
    m_moneyHSpin->setValue(moneyRegion.height());
}

void MainWindow::setupUI()
{
    auto *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    // Create main splitter
    auto *mainSplitter = new QSplitter(Qt::Horizontal, this);
    auto *mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->addWidget(mainSplitter);
    
    // Left side - main interface
    auto *leftWidget = new QWidget();
    auto *leftLayout = new QVBoxLayout(leftWidget);
    
    // Title
    auto *titleLabel = new QLabel("CSCAN - CS2 Economy Helper", this);
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold;");
    titleLabel->setAlignment(Qt::AlignCenter);
    leftLayout->addWidget(titleLabel);
    
    // Status and controls
    auto *controlLayout = new QHBoxLayout();
    m_statusLabel = new QLabel("Status: Ready - Click 'Start Game' when in match", this);
    m_startButton = new QPushButton("Start Game", this);
    m_stopButton = new QPushButton("Stop Scan", this);
    m_stopButton->setEnabled(false);
    
    controlLayout->addWidget(m_statusLabel);
    controlLayout->addStretch();
    controlLayout->addWidget(m_startButton);
    controlLayout->addWidget(m_stopButton);
    leftLayout->addLayout(controlLayout);
    
    // Screen preview group
    m_previewGroup = new QGroupBox("Screen Capture Preview", this);
    auto *previewLayout = new QVBoxLayout(m_previewGroup);
    m_previewLabel = new QLabel("No capture active", this);
    m_previewLabel->setMinimumSize(400, 200);
    m_previewLabel->setStyleSheet("border: 1px solid gray; background-color: #f0f0f0;");
    m_previewLabel->setAlignment(Qt::AlignCenter);
    m_previewLabel->setScaledContents(true);
    previewLayout->addWidget(m_previewLabel);
    leftLayout->addWidget(m_previewGroup);
    
    // Game data group (enhanced with state info)
    m_dataGroup = new QGroupBox("Detected CS2 Data", this);
    auto *dataLayout = new QVBoxLayout(m_dataGroup);
    m_moneyLabel = new QLabel("Money: $0", this);
    m_scoreLabel = new QLabel("Score: CT 0 - 0 T", this);
    m_sideLabel = new QLabel("Side: Unknown", this);
    m_gameStateLabel = new QLabel("State: Unknown", this);
    
    m_moneyLabel->setStyleSheet("font-size: 14px; font-weight: bold;");
    m_scoreLabel->setStyleSheet("font-size: 14px; font-weight: bold;");
    m_sideLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: blue;");
    m_gameStateLabel->setStyleSheet("font-size: 12px; color: purple;");
    
    dataLayout->addWidget(m_moneyLabel);
    dataLayout->addWidget(m_scoreLabel);
    dataLayout->addWidget(m_sideLabel);
    dataLayout->addWidget(m_gameStateLabel);
    leftLayout->addWidget(m_dataGroup);
    
    // Enhanced suggestion group
    m_suggestionGroup = new QGroupBox("Economy Recommendation", this);
    auto *suggestionLayout = new QVBoxLayout(m_suggestionGroup);
    
    m_suggestionLabel = new QLabel("Start scanning to get economy suggestions...", this);
    m_suggestionLabel->setStyleSheet("color: gray; font-style: italic; font-size: 12px;");
    
    m_weaponLabel = new QLabel("Weapons: None", this);
    m_weaponLabel->setStyleSheet("font-size: 13px; font-weight: bold; color: #2E8B57;");
    
    m_utilityLabel = new QLabel("Utility: None", this);
    m_utilityLabel->setStyleSheet("font-size: 13px; color: #4169E1;");
    
    m_strategyLabel = new QLabel("Strategy: None", this);
    m_strategyLabel->setStyleSheet("font-size: 13px; color: #FF4500; font-style: italic;");
    
    suggestionLayout->addWidget(m_suggestionLabel);
    suggestionLayout->addWidget(m_weaponLabel);
    suggestionLayout->addWidget(m_utilityLabel);
    suggestionLayout->addWidget(m_strategyLabel);
    
    leftLayout->addWidget(m_suggestionGroup);
    
    mainSplitter->addWidget(leftWidget);
    
    // Right side - debug interface
    setupDebugControls();
    
    setWindowTitle("CSCAN - CS2 Economy Helper (Debug Mode)");
    setMinimumSize(1000, 800);
    resize(1200, 900);
    
    // Set splitter proportions
    mainSplitter->setSizes({700, 500});
}

void MainWindow::setupDebugControls()
{
    auto *rightWidget = new QWidget();
    auto *rightLayout = new QVBoxLayout(rightWidget);
    
    // Debug controls group
    m_debugGroup = new QGroupBox("Debug Controls", this);
    auto *debugControlLayout = new QVBoxLayout(m_debugGroup);
    
    // Debug mode toggle
    m_debugModeCheckbox = new QCheckBox("Enable Debug Mode", this);
    debugControlLayout->addWidget(m_debugModeCheckbox);
    
    // Debug action buttons
    auto *debugButtonLayout = new QHBoxLayout();
    m_saveDebugButton = new QPushButton("Save Debug Frame", this);
    m_openDebugFolderButton = new QPushButton("Open Debug Folder", this);
    m_calibrateButton = new QPushButton("Start Calibration", this);
    
    debugButtonLayout->addWidget(m_saveDebugButton);
    debugButtonLayout->addWidget(m_openDebugFolderButton);
    debugButtonLayout->addWidget(m_calibrateButton);
    debugControlLayout->addLayout(debugButtonLayout);
    
    rightLayout->addWidget(m_debugGroup);
    
    // Region adjustment group
    auto *regionGroup = new QGroupBox("Money Region Adjustment", this);
    auto *regionLayout = new QVBoxLayout(regionGroup);
    
    // Current region info
    m_regionInfoLabel = new QLabel("Money Region: (1650, 50, 200, 40)", this);
    m_regionInfoLabel->setStyleSheet("font-weight: bold; color: #0066CC;");
    regionLayout->addWidget(m_regionInfoLabel);
    
    // Spinboxes for region adjustment
    auto *spinboxLayout = new QGridLayout();
    
    spinboxLayout->addWidget(new QLabel("X:"), 0, 0);
    m_moneyXSpin = new QSpinBox(this);
    m_moneyXSpin->setRange(0, 3840);
    m_moneyXSpin->setSingleStep(10);
    spinboxLayout->addWidget(m_moneyXSpin, 0, 1);
    
    spinboxLayout->addWidget(new QLabel("Y:"), 0, 2);
    m_moneyYSpin = new QSpinBox(this);
    m_moneyYSpin->setRange(0, 2160);
    m_moneyYSpin->setSingleStep(10);
    spinboxLayout->addWidget(m_moneyYSpin, 0, 3);
    
    spinboxLayout->addWidget(new QLabel("W:"), 1, 0);
    m_moneyWSpin = new QSpinBox(this);
    m_moneyWSpin->setRange(50, 500);
    m_moneyWSpin->setSingleStep(10);
    spinboxLayout->addWidget(m_moneyWSpin, 1, 1);
    
    spinboxLayout->addWidget(new QLabel("H:"), 1, 2);
    m_moneyHSpin = new QSpinBox(this);
    m_moneyHSpin->setRange(20, 200);
    m_moneyHSpin->setSingleStep(5);
    spinboxLayout->addWidget(m_moneyHSpin, 1, 3);
    
    regionLayout->addLayout(spinboxLayout);
    
    // Region adjustment buttons
    auto *regionButtonLayout = new QHBoxLayout();
    m_adjustMoneyButton = new QPushButton("Apply Money Region", this);
    m_adjustScoreButton = new QPushButton("Adjust Scoreboard", this);
    
    regionButtonLayout->addWidget(m_adjustMoneyButton);
    regionButtonLayout->addWidget(m_adjustScoreButton);
    regionLayout->addLayout(regionButtonLayout);
    
    rightLayout->addWidget(regionGroup);
    
    // Debug info display
    m_debugInfoLabel = new QLabel("Debug Info: Ready", this);
    m_debugInfoLabel->setStyleSheet("font-size: 11px; color: #666; padding: 5px;");
    m_debugInfoLabel->setWordWrap(true);
    rightLayout->addWidget(m_debugInfoLabel);
    
    // Debug output
    auto *outputGroup = new QGroupBox("Debug Output", this);
    auto *outputLayout = new QVBoxLayout(outputGroup);
    
    m_debugOutput = new QTextEdit(this);
    m_debugOutput->setMaximumHeight(300);  // Limit height instead of block count
    m_debugOutput->setStyleSheet("font-family: 'Courier New'; font-size: 9px;");
    outputLayout->addWidget(m_debugOutput);
    
    rightLayout->addWidget(outputGroup);
    
    // Add to main splitter
    auto *mainSplitter = qobject_cast<QSplitter*>(centralWidget()->layout()->itemAt(0)->widget());
    if (mainSplitter) {
        mainSplitter->addWidget(rightWidget);
    }
}

void MainWindow::startScanning()
{
    m_isScanning = true;
    m_frameCount = 0;
    
    m_startButton->setEnabled(false);
    m_stopButton->setEnabled(true);
    m_statusLabel->setText("Status: ● SCANNING");
    m_previewLabel->setText("Starting capture...");
    
    m_screenCapture->startCapture();
    
    // Clear debug output
    m_debugOutput->clear();
    m_debugOutput->append("=== SCANNING STARTED ===");
    
    qDebug() << "Started scanning with OCR processing and economy engine";
}

void MainWindow::stopScanning()
{
    m_isScanning = false;
    
    m_startButton->setEnabled(true);
    m_stopButton->setEnabled(false);
    m_statusLabel->setText("Status: Ready - Click 'Start Game' when in match");
    m_previewLabel->setText("No capture active");
    
    m_screenCapture->stopCapture();
    
    m_debugOutput->append("=== SCANNING STOPPED ===");
    qDebug() << "Stopped scanning";
}

void MainWindow::onFrameReady(const QPixmap &frame)
{
    m_frameCount++;
    
    // Update preview
    QPixmap scaledFrame = frame.scaled(m_previewLabel->size(), 
        Qt::KeepAspectRatio, Qt::SmoothTransformation);
    m_previewLabel->setPixmap(scaledFrame);
    
    // Process frame for OCR every few frames
    if (m_frameCount % 3 == 0) {
        m_ocrProcessor->processFrame(frame);
    }
    
    // Update status with frame count
    m_statusLabel->setText(QString("Status: ● SCANNING (Frames: %1)").arg(m_frameCount));
}

void MainWindow::onCaptureError(const QString &error)
{
    qDebug() << "Capture error:" << error;
    m_statusLabel->setText("Status: ERROR - " + error);
    m_debugOutput->append(QString("CAPTURE ERROR: %1").arg(error));
}

void MainWindow::onGameDataReady(const GameData &data)
{
    // Update detected data display
    m_moneyLabel->setText(QString("Money: $%1").arg(data.money));
    m_scoreLabel->setText(QString("Score: CT %1 - %2 T").arg(data.ctScore).arg(data.tScore));
    m_sideLabel->setText(QString("Side: %1").arg(data.side));
    m_gameStateLabel->setText(formatGameStateInfo(data));
    
    // Update debug info
    m_lastDebugInfo = data.debugInfo;
    m_debugInfoLabel->setText(QString("Debug: %1").arg(data.debugInfo));
    
    // Change text color based on data validity and state
    if (data.dataValid && !data.isSpectating) {
        m_moneyLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: green;");
        m_sideLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: blue;");
        
        // Generate economy recommendation only for valid player data
        BuyRecommendation recommendation = m_economyEngine->generateRecommendation(data);
        updateRecommendationDisplay(recommendation);
    } else {
        m_moneyLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: orange;");
        m_sideLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: orange;");
        
        if (data.isSpectating) {
            m_suggestionLabel->setText("SPECTATING - No recommendations");
            m_weaponLabel->setText("(Showing teammate's data)");
            m_utilityLabel->setText("");
            m_strategyLabel->setText("");
        }
    }
    
    qDebug() << "Game data updated - Money:" << data.money << "Valid:" << data.dataValid 
             << "Spectating:" << data.isSpectating;
}

void MainWindow::onOCRError(const QString &error)
{
    qDebug() << "OCR error:" << error;
    m_moneyLabel->setText("Money: OCR Error");
    m_moneyLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: red;");
    m_debugOutput->append(QString("OCR ERROR: %1").arg(error));
}

void MainWindow::onRecommendationReady(const BuyRecommendation &recommendation)
{
    updateRecommendationDisplay(recommendation);
}

void MainWindow::onDebugMessage(const QString &message)
{
    QString timestamp = QTime::currentTime().toString("hh:mm:ss");
    m_debugOutput->append(QString("[%1] %2").arg(timestamp).arg(message));
    
    // Auto-scroll to bottom
    m_debugOutput->moveCursor(QTextCursor::End);
}

void MainWindow::onCalibrationNeeded(const QString &message)
{
    m_debugOutput->append(QString("CALIBRATION: %1").arg(message));
    m_statusLabel->setText(QString("CALIBRATION: %1").arg(message));
}

QString MainWindow::formatGameStateInfo(const GameData &data)
{
    QString stateText;
    
    switch (data.gameState) {
    case GameState::IN_ROUND:
        stateText = "In Round";
        break;
    case GameState::BUY_PHASE:
        stateText = "Buy Phase";
        break;
    case GameState::ROUND_END:
        stateText = "Round End";
        break;
    case GameState::SPECTATING:
        stateText = "Spectating";
        break;
    case GameState::DEAD:
        stateText = "Dead";
        break;
    default:
        stateText = "Unknown";
        break;
    }
    
    if (data.isSpectating) {
        stateText += " (SPECTATING)";
    }
    if (!data.isAlive) {
        stateText += " (DEAD)";
    }
    
    return QString("State: %1").arg(stateText);
}

void MainWindow::updateRecommendationDisplay(const BuyRecommendation &recommendation)
{
    QString suggestionType;
    switch (recommendation.decision) {
    case BuyDecision::FULL_BUY:
        suggestionType = "FULL BUY ROUND";
        break;
    case BuyDecision::FORCE_BUY:
        suggestionType = "FORCE BUY";
        break;
    case BuyDecision::ECO_ROUND:
        suggestionType = "ECO ROUND";
        break;
    case BuyDecision::ANTI_ECO:
        suggestionType = "ANTI-ECO";
        break;
    case BuyDecision::PISTOL_ARMOR:
        suggestionType = "PISTOL + ARMOR";
        break;
    default:
        suggestionType = "SAVE ROUND";
        break;
    }
    
    QString suggestionText = QString("%1 ($%2)")
                            .arg(suggestionType)
                            .arg(recommendation.totalCost);
    m_suggestionLabel->setText(suggestionText);
    m_suggestionLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #8B0000;");
    
    if (!recommendation.weapons.isEmpty()) {
        QString weaponText = "Weapons: " + recommendation.weapons.join(", ");
        m_weaponLabel->setText(weaponText);
    } else {
        m_weaponLabel->setText("Weapons: Default pistol");
    }
    
    if (!recommendation.utility.isEmpty()) {
        QString utilityText = "Utility: " + recommendation.utility.join(", ");
        m_utilityLabel->setText(utilityText);
    } else {
        m_utilityLabel->setText("Utility: None");
    }
    
    if (!recommendation.armor.isEmpty()) {
        QString armorText = " + " + recommendation.armor.join(", ");
        m_weaponLabel->setText(m_weaponLabel->text() + armorText);
    }
    
    QString strategyText = QString("Strategy: %1").arg(recommendation.strategy);
    m_strategyLabel->setText(strategyText);
    
    qDebug() << "Updated recommendation display:" << suggestionType 
             << "Cost:" << recommendation.totalCost;
}

void MainWindow::toggleDebugMode()
{
    bool enabled = m_debugModeCheckbox->isChecked();
    m_ocrProcessor->setDebugMode(enabled);
    
    if (enabled) {
        m_debugOutput->append("Debug mode ENABLED");
    } else {
        m_debugOutput->append("Debug mode DISABLED");
    }
}

void MainWindow::saveDebugFrame()
{
    if (m_isScanning && !m_previewLabel->pixmap().isNull()) {
        m_ocrProcessor->saveDebugFrame(m_previewLabel->pixmap());
        m_debugOutput->append("Debug frame saved to Documents/CSCAN_Debug/");
    }
}

void MainWindow::openDebugFolder()
{
    QString debugDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/CSCAN_Debug";
    QDesktopServices::openUrl(QUrl::fromLocalFile(debugDir));
}

void MainWindow::startCalibration()
{
    m_ocrProcessor->startCalibration();
    m_debugOutput->append("CALIBRATION MODE: Manual region adjustment activated");
    m_debugOutput->append("Use the spinboxes above to adjust the money region coordinates");
}

void MainWindow::adjustMoneyRegion()
{
    QRect newRegion(m_moneyXSpin->value(), m_moneyYSpin->value(),
                   m_moneyWSpin->value(), m_moneyHSpin->value());
    
    m_ocrProcessor->setMoneyRegion(newRegion);
    m_regionInfoLabel->setText(QString("Money Region: (%1, %2, %3, %4)")
                              .arg(newRegion.x()).arg(newRegion.y())
                              .arg(newRegion.width()).arg(newRegion.height()));
    
    m_debugOutput->append(QString("Money region adjusted to: %1,%2,%3,%4")
                         .arg(newRegion.x()).arg(newRegion.y())
                         .arg(newRegion.width()).arg(newRegion.height()));
}

void MainWindow::adjustScoreboardRegion()
{
    // For now, just log that this feature could be added
    m_debugOutput->append("Scoreboard region adjustment - Feature can be added similar to money region");
}
