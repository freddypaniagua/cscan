#include "MainWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QWidget>
#include <QGroupBox>
#include <QDebug>

MainWindow::MainWindow(QWidget* parent)
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

    // Connect economy engine signals
    connect(m_economyEngine, &EconomyEngine::recommendationReady, this, &MainWindow::onRecommendationReady);

    // Connect button signals
    connect(m_startButton, &QPushButton::clicked, this, &MainWindow::startScanning);
    connect(m_stopButton, &QPushButton::clicked, this, &MainWindow::stopScanning);
}

void MainWindow::setupUI()
{
    auto* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    auto* mainLayout = new QVBoxLayout(centralWidget);

    // Title
    auto* titleLabel = new QLabel("CSCAN - CS2 Economy Helper", this);
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold;");
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);

    // Status and controls
    auto* controlLayout = new QHBoxLayout();
    m_statusLabel = new QLabel("Status: Ready - Click 'Start Game' when in match", this);
    m_startButton = new QPushButton("Start Game", this);
    m_stopButton = new QPushButton("Stop Scan", this);
    m_stopButton->setEnabled(false);

    controlLayout->addWidget(m_statusLabel);
    controlLayout->addStretch();
    controlLayout->addWidget(m_startButton);
    controlLayout->addWidget(m_stopButton);
    mainLayout->addLayout(controlLayout);

    // Screen preview group
    m_previewGroup = new QGroupBox("Screen Capture Preview", this);
    auto* previewLayout = new QVBoxLayout(m_previewGroup);
    m_previewLabel = new QLabel("No capture active", this);
    m_previewLabel->setMinimumSize(400, 200);
    m_previewLabel->setStyleSheet("border: 1px solid gray; background-color: #f0f0f0;");
    m_previewLabel->setAlignment(Qt::AlignCenter);
    m_previewLabel->setScaledContents(true);
    previewLayout->addWidget(m_previewLabel);
    mainLayout->addWidget(m_previewGroup);

    // Game data group (enhanced)
    m_dataGroup = new QGroupBox("Detected CS2 Data", this);
    auto* dataLayout = new QVBoxLayout(m_dataGroup);
    m_moneyLabel = new QLabel("Money: $0", this);
    m_scoreLabel = new QLabel("Score: CT 0 - 0 T", this);
    m_sideLabel = new QLabel("Side: Unknown", this);

    m_moneyLabel->setStyleSheet("font-size: 14px; font-weight: bold;");
    m_scoreLabel->setStyleSheet("font-size: 14px; font-weight: bold;");
    m_sideLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: blue;");

    dataLayout->addWidget(m_moneyLabel);
    dataLayout->addWidget(m_scoreLabel);
    dataLayout->addWidget(m_sideLabel);
    mainLayout->addWidget(m_dataGroup);

    // Enhanced suggestion group
    m_suggestionGroup = new QGroupBox("Economy Recommendation", this);
    auto* suggestionLayout = new QVBoxLayout(m_suggestionGroup);

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

    mainLayout->addWidget(m_suggestionGroup);

    setWindowTitle("CSCAN - CS2 Economy Helper");
    setFixedSize(500, 750);
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
    qDebug() << "Stopped scanning";
}

void MainWindow::onFrameReady(const QPixmap& frame)
{
    m_frameCount++;

    // Update preview
    QPixmap scaledFrame = frame.scaled(m_previewLabel->size(),
        Qt::KeepAspectRatio, Qt::SmoothTransformation);
    m_previewLabel->setPixmap(scaledFrame);

    // Process frame for OCR every few frames (don't process every single frame for performance)
    if (m_frameCount % 3 == 0) { // Process every 3rd frame
        m_ocrProcessor->processFrame(frame);
    }

    // Update status with frame count
    m_statusLabel->setText(QString("Status: ● SCANNING (Frames: %1)").arg(m_frameCount));
}

void MainWindow::onCaptureError(const QString& error)
{
    qDebug() << "Capture error:" << error;
    m_statusLabel->setText("Status: ERROR - " + error);
}

void MainWindow::onGameDataReady(const GameData& data)
{
    // Update detected data display
    m_moneyLabel->setText(QString("Money: $%1").arg(data.money));
    m_scoreLabel->setText(QString("Score: CT %1 - %2 T").arg(data.ctScore).arg(data.tScore));
    m_sideLabel->setText(QString("Side: %1").arg(data.side));

    // Change text color based on data validity
    if (data.dataValid) {
        m_moneyLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: green;");
        m_sideLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: blue;");

        // Generate economy recommendation
        BuyRecommendation recommendation = m_economyEngine->generateRecommendation(data);
        updateRecommendationDisplay(recommendation);
    }
    else {
        m_moneyLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: red;");
        m_sideLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: red;");
    }

    qDebug() << "Game data updated - Money:" << data.money << "Valid:" << data.dataValid
        << "Side:" << data.side;
}

void MainWindow::onOCRError(const QString& error)
{
    qDebug() << "OCR error:" << error;
    m_moneyLabel->setText("Money: OCR Error");
    m_moneyLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: red;");
}

void MainWindow::onRecommendationReady(const BuyRecommendation& recommendation)
{
    updateRecommendationDisplay(recommendation);
}

void MainWindow::updateRecommendationDisplay(const BuyRecommendation& recommendation)
{
    // Update suggestion type
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

    // Build main suggestion text
    QString suggestionText = QString("%1 ($%2)")
        .arg(suggestionType)
        .arg(recommendation.totalCost);
    m_suggestionLabel->setText(suggestionText);
    m_suggestionLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #8B0000;");

    // Update weapons
    if (!recommendation.weapons.isEmpty()) {
        QString weaponText = "Weapons: " + recommendation.weapons.join(", ");
        m_weaponLabel->setText(weaponText);
    }
    else {
        m_weaponLabel->setText("Weapons: Default pistol");
    }

    // Update utility
    if (!recommendation.utility.isEmpty()) {
        QString utilityText = "Utility: " + recommendation.utility.join(", ");
        m_utilityLabel->setText(utilityText);
    }
    else {
        m_utilityLabel->setText("Utility: None");
    }

    // Add armor if recommended
    if (!recommendation.armor.isEmpty()) {
        QString armorText = " + " + recommendation.armor.join(", ");
        m_weaponLabel->setText(m_weaponLabel->text() + armorText);
    }

    // Update strategy
    QString strategyText = QString("Strategy: %1").arg(recommendation.strategy);
    m_strategyLabel->setText(strategyText);

    qDebug() << "Updated recommendation display:" << suggestionType
        << "Cost:" << recommendation.totalCost;
}