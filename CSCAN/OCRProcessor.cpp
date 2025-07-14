#include "OCRProcessor.h"
#include <QDebug>
#include <QStandardPaths>
#include <QDir>
#include <QRegularExpression>
#include <QRandomGenerator>

// Include Tesseract only if available
#ifdef TESSERACT_AVAILABLE
#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>
#endif

// Define screen regions for 1920x1080 resolution
const QRect OCRProcessor::MONEY_REGION(1650, 50, 200, 40);
const QRect OCRProcessor::SCOREBOARD_REGION(600, 100, 720, 500);
const QRect OCRProcessor::BUY_TIMER_REGION(850, 650, 220, 50);

OCRProcessor::OCRProcessor(QObject* parent)
    : QObject(parent)
    , m_tesseract(nullptr)
{
#ifdef TESSERACT_AVAILABLE
    initializeRealOCR();
#else
    initializeMockOCR();
#endif
}

OCRProcessor::~OCRProcessor()
{
#ifdef TESSERACT_AVAILABLE
    if (m_tesseract) {
        static_cast<tesseract::TessBaseAPI*>(m_tesseract)->End();
        delete static_cast<tesseract::TessBaseAPI*>(m_tesseract);
    }
#endif
}

#ifdef TESSERACT_AVAILABLE
void OCRProcessor::initializeRealOCR()
{
    tesseract::TessBaseAPI* api = new tesseract::TessBaseAPI();

    // Try different initialization paths
    QStringList tessdataPaths = {
        "C:/Program Files/Tesseract-OCR/tessdata",
        "C:/vcpkg/installed/x64-windows/share/tessdata",
        "tessdata"
    };

    bool initialized = false;
    for (const QString& path : tessdataPaths) {
        if (api->Init(path.toLocal8Bit().data(), "eng", tesseract::OEM_LSTM_ONLY) == 0) {
            initialized = true;
            qDebug() << "Tesseract initialized with data path:" << path;
            break;
        }
    }

    if (!initialized) {
        // Try without specifying path (system default)
        if (api->Init(nullptr, "eng", tesseract::OEM_LSTM_ONLY) == 0) {
            initialized = true;
            qDebug() << "Tesseract initialized with system default path";
        }
    }

    if (!initialized) {
        qWarning() << "Failed to initialize Tesseract OCR - falling back to mock mode";
        delete api;
        initializeMockOCR();
        return;
    }

    // Configure for game text recognition
    api->SetPageSegMode(tesseract::PSM_SINGLE_WORD);

    // Whitelist characters commonly found in CS2 UI
    api->SetVariable("tessedit_char_whitelist", "0123456789$,:-CTcts ");

    // Optimize for game text
    api->SetVariable("tessedit_do_invert", "0");
    api->SetVariable("classify_enable_learning", "0");

    m_tesseract = api;
    m_usingRealOCR = true;
    qDebug() << "Real OCR Processor initialized successfully";
}
#endif

void OCRProcessor::initializeMockOCR()
{
    m_tesseract = nullptr;
    m_usingRealOCR = false;
    qDebug() << "Mock OCR Processor initialized (Tesseract not available)";
}

void OCRProcessor::processFrame(const QPixmap& frame)
{
    if (m_usingRealOCR) {
        processFrameReal(frame);
    }
    else {
        processFrameMock(frame);
    }
}

#ifdef TESSERACT_AVAILABLE
void OCRProcessor::processFrameReal(const QPixmap& frame)
{
    if (!m_tesseract) {
        emit processingError("OCR not initialized");
        return;
    }

    GameData data;

    try {
        // Extract money from top-right region
        QString moneyText = extractTextFromRegion(frame, MONEY_REGION);
        data.money = extractMoney(moneyText);

        // Extract side information from scoreboard region
        QString scoreText = extractTextFromRegion(frame, SCOREBOARD_REGION);
        auto scorePair = extractScore(scoreText);
        data.ctScore = scorePair.first;
        data.tScore = scorePair.second;

        // Detect current side
        data.side = detectSide(frame);

        // Simple validation - CS2 money should be 0-16000
        data.dataValid = (data.money >= 0 && data.money <= 16000);

        // Enhanced validation
        if (data.ctScore < 0 || data.tScore < 0 ||
            data.ctScore > 16 || data.tScore > 16) {
            data.dataValid = false;
        }

        qDebug() << "Real OCR Results - Money:" << data.money
            << "Score:" << data.ctScore << "-" << data.tScore
            << "Side:" << data.side << "Valid:" << data.dataValid;

        emit gameDataReady(data);

    }
    catch (const std::exception& e) {
        emit processingError(QString("OCR processing failed: %1").arg(e.what()));
    }
}
#else
void OCRProcessor::processFrameReal(const QPixmap& frame)
{
    // Fallback to mock if Tesseract not available
    processFrameMock(frame);
}
#endif

void OCRProcessor::processFrameMock(const QPixmap& frame)
{
    GameData data;

    // Mock data that simulates realistic CS2 scenarios
    static int mockMoney = 3200;
    static int roundCount = 0;

    roundCount++;

    // Simulate different economy scenarios
    if (roundCount % 10 == 0) {
        // Reset scenario
        mockMoney = QRandomGenerator::global()->bounded(800, 4000);
    }
    else {
        // Normal fluctuation
        int change = QRandomGenerator::global()->bounded(-300, 500);
        mockMoney += change;
    }

    // Keep money in valid CS2 range
    if (mockMoney < 0) mockMoney = 0;
    if (mockMoney > 16000) mockMoney = 16000;

    data.money = mockMoney;
    data.ctScore = 7 + (roundCount / 5) % 8;
    data.tScore = 5 + (roundCount / 7) % 8;
    data.side = (roundCount % 30 < 15) ? "CT" : "T";
    data.dataValid = true;

    // Simulate round events occasionally
    if (roundCount % 8 == 0) {
        data.roundEnded = true;
    }

    if (roundCount % 6 == 0) {
        data.buyPhaseActive = true;
    }

    qDebug() << "Mock OCR - Money:" << data.money << "Score:" << data.ctScore
        << "-" << data.tScore << "Side:" << data.side;

    emit gameDataReady(data);
}

QString OCRProcessor::extractTextFromRegion(const QPixmap& frame, const QRect& region)
{
#ifdef TESSERACT_AVAILABLE
    if (!m_usingRealOCR) return "";

    tesseract::TessBaseAPI* api = static_cast<tesseract::TessBaseAPI*>(m_tesseract);
    if (!api) return "";

    // Extract the region from the frame
    QPixmap regionPixmap = frame.copy(region);
    QImage regionImage = regionPixmap.toImage();

    // Preprocess the image for better OCR
    QImage processedImage = preprocessImage(regionImage);

    // Save debug image (helps with debugging)
    static int debugCounter = 0;
    if (debugCounter++ % 30 == 0) { // Save every 30th frame
        saveDebugImage(processedImage, QString("debug_region_%1.png").arg(debugCounter));
    }

    // Convert QImage to format Tesseract can use
    QImage rgbImage = processedImage.convertToFormat(QImage::Format_RGB888);

    // Set image data in Tesseract
    api->SetImage(rgbImage.bits(), rgbImage.width(), rgbImage.height(),
        3, rgbImage.bytesPerLine());

    // Get text
    char* text = api->GetUTF8Text();
    QString result = QString::fromUtf8(text ? text : "");
    delete[] text;

    return result.trimmed();
#else
    return QString::number(QRandomGenerator::global()->bounded(800, 16000));
#endif
}

QImage OCRProcessor::preprocessImage(const QImage& input)
{
    QImage processed = input;

    // Convert to grayscale for better OCR
    processed = processed.convertToFormat(QImage::Format_Grayscale8);

    // Scale up for better OCR (3x size for small text)
    processed = processed.scaled(processed.size() * 3, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    // Simple contrast enhancement
    // For production, you might want more sophisticated preprocessing

    return processed;
}

QString OCRProcessor::detectSide(const QPixmap& frame)
{
    // Try multiple methods to detect side

    // Method 1: Analyze money region color
    QPixmap moneyRegion = frame.copy(MONEY_REGION);
    QImage moneyImage = moneyRegion.toImage();

    // Analyze dominant color in money region
    int bluePixels = 0, orangePixels = 0, totalPixels = 0;

    for (int y = 0; y < moneyImage.height(); ++y) {
        for (int x = 0; x < moneyImage.width(); ++x) {
            QColor pixel = moneyImage.pixelColor(x, y);
            totalPixels++;

            // Check for blue-ish colors (CT side)
            if (pixel.blue() > pixel.red() && pixel.blue() > pixel.green()) {
                bluePixels++;
            }
            // Check for orange-ish colors (T side)
            else if (pixel.red() > pixel.blue() && pixel.red() > pixel.green() * 1.2) {
                orangePixels++;
            }
        }
    }

    if (bluePixels > orangePixels) {
        return "CT";
    }
    else if (orangePixels > bluePixels) {
        return "T";
    }

    // Method 2: OCR text detection (fallback)
    QString scoreText = extractTextFromRegion(frame, SCOREBOARD_REGION);
    if (scoreText.contains("CT", Qt::CaseInsensitive)) {
        return "CT";
    }
    else if (scoreText.contains("T", Qt::CaseInsensitive)) {
        return "T";
    }

    // Default fallback
    return "Unknown";
}

int OCRProcessor::extractMoney(const QString& text)
{
    // Remove $ and , characters, extract numbers
    QString cleanText = text;
    cleanText.remove(QRegularExpression("[^0-9]")); // Keep only digits

    bool ok;
    int money = cleanText.toInt(&ok);

    // Validate range
    if (!ok || money < 0 || money > 16000) {
        return 0; // Invalid money value
    }

    return money;
}

QPair<int, int> OCRProcessor::extractScore(const QString& text)
{
    // Look for patterns like "CT 7 - 5 T" or "7-5"
    QRegularExpression scorePattern(R"((\d+)\s*[-–]\s*(\d+))");
    QRegularExpressionMatch match = scorePattern.match(text);

    if (match.hasMatch()) {
        bool ok1, ok2;
        int score1 = match.captured(1).toInt(&ok1);
        int score2 = match.captured(2).toInt(&ok2);

        if (ok1 && ok2 && score1 >= 0 && score1 <= 16 && score2 >= 0 && score2 <= 16) {
            return QPair<int, int>(score1, score2);
        }
    }

    return QPair<int, int>(0, 0);
}

bool OCRProcessor::detectRoundEnd(const QPixmap& frame)
{
    // TODO: Implement round end detection
    // Look for "ROUND OVER" text or sudden scoreboard appearance
    return false;
}

bool OCRProcessor::detectBuyPhase(const QPixmap& frame)
{
    // TODO: Implement buy phase detection
    // Look for buy timer or "BUY" text
    return false;
}

void OCRProcessor::saveDebugImage(const QImage& image, const QString& filename)
{
    QString debugDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/CSCAN_Debug";
    QDir().mkpath(debugDir);

    QString fullPath = debugDir + "/" + filename;
    image.save(fullPath);
    qDebug() << "Debug image saved:" << fullPath;
}