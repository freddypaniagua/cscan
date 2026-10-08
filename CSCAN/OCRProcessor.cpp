#include "OCRProcessor.h"
#include <QDebug>
#include <QSet>
#include <QColor>
#include <QStandardPaths>
#include <QDir>
#include <QRegularExpression>
#include <QRandomGenerator>
#include <QGuiApplication>
#include <QScreen>
#include <QDateTime>

// Include Tesseract only if available
#ifdef TESSERACT_AVAILABLE
#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>
#endif

OCRProcessor::OCRProcessor(QObject *parent)
    : QObject(parent)
    , m_tesseract(nullptr)
    , m_debugMode(true)  // Start with debug enabled
    , m_calibrationMode(false)
    , m_settings(new QSettings("CSCAN", "OCRSettings", this))
{
    initializeCalibratedRegions();
    loadCalibration();
    
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

void OCRProcessor::initializeCalibratedRegions()
{
    // CALIBRATED REGIONS based on your exact screenshots
    
    // Money region (bottom-left) - where $400, $950, $14200 appears
    m_moneyRegion = QRect(20, 850, 120, 35);
    
    // Buy phase money region (top-left of buy menu) - where $9700, $4550 appears
    m_buyMoneyRegion = QRect(200, 150, 120, 35);
    
    // Score region (top-center) - where "1 0", "7 8", "5 5" appears
    m_scoreboardRegion = QRect(680, 30, 80, 45);
    
    // Side symbol region (bottom-center) - where CT/T symbols appear
    m_sideSymbolRegion = QRect(680, 845, 80, 40);
    
    // Buy timer region (center-top) - where "Buy Time Remaining 00:33" appears
    m_buyTimerRegion = QRect(450, 140, 300, 25);
    
    // SPECTATOR BAR REGION (PRIMARY DETECTION) - where player info overlay appears
    m_spectatorBarRegion = QRect(400, 760, 500, 60);
    
    // Chat region (left side) - for backup spectator detection
    m_chatRegion = QRect(20, 580, 400, 200);
    
    // Regions to avoid (so we don't OCR wrong numbers)
    m_healthRegion = QRect(145, 850, 80, 35);      // Health: "82", "100"
    m_armorRegion = QRect(225, 850, 80, 35);       // Armor: "100"
    m_ammoRegion = QRect(770, 845, 60, 40);        // Ammo: "4 90"
    
    logDebugInfo("Initialized calibrated regions for your CS2 setup");
    logDebugInfo(QString("Money region: %1,%2,%3,%4").arg(m_moneyRegion.x()).arg(m_moneyRegion.y())
                .arg(m_moneyRegion.width()).arg(m_moneyRegion.height()));
    logDebugInfo(QString("Spectator bar region: %1,%2,%3,%4").arg(m_spectatorBarRegion.x())
                .arg(m_spectatorBarRegion.y()).arg(m_spectatorBarRegion.width()).arg(m_spectatorBarRegion.height()));
}

#ifdef TESSERACT_AVAILABLE
void OCRProcessor::initializeRealOCR()
{
    tesseract::TessBaseAPI* api = new tesseract::TessBaseAPI();
    
    QStringList tessdataPaths = {
        "C:/vcpkg/installed/x64-windows/share/tessdata",
        "C:/Program Files/Tesseract-OCR/tessdata",
        "tessdata"
    };
    
    bool initialized = false;
    for (const QString& path : tessdataPaths) {
        if (api->Init(path.toLocal8Bit().data(), "eng", tesseract::OEM_LSTM_ONLY) == 0) {
            initialized = true;
            logDebugInfo(QString("Tesseract initialized with data path: %1").arg(path));
            break;
        }
    }
    
    if (!initialized) {
        if (api->Init(nullptr, "eng", tesseract::OEM_LSTM_ONLY) == 0) {
            initialized = true;
            logDebugInfo("Tesseract initialized with system default path");
        }
    }
    
    if (!initialized) {
        logDebugInfo("Failed to initialize Tesseract OCR - falling back to mock mode");
        delete api;
        initializeMockOCR();
        return;
    }
    
    api->SetPageSegMode(tesseract::PSM_SINGLE_WORD);
    api->SetVariable("tessedit_char_whitelist", "0123456789$,:-CTcts ");
    api->SetVariable("tessedit_do_invert", "0");
    api->SetVariable("classify_enable_learning", "0");
    
    m_tesseract = api;
    m_usingRealOCR = true;
    logDebugInfo("Real OCR Processor initialized successfully");
}
#else
void OCRProcessor::initializeRealOCR()
{
    initializeMockOCR();
}
#endif

void OCRProcessor::initializeMockOCR()
{
    m_tesseract = nullptr;
    m_usingRealOCR = false;
    logDebugInfo("Mock OCR Processor initialized (Tesseract not available)");
}

void OCRProcessor::processFrame(const QPixmap &frame)
{
    if (m_debugMode && QRandomGenerator::global()->bounded(100) < 5) {  // Save debug frame 5% of the time
        saveDebugFrame(frame);
    }
    
    if (m_usingRealOCR) {
        processFrameReal(frame);
    } else {
        processFrameMock(frame);
    }
}

#ifdef TESSERACT_AVAILABLE
void OCRProcessor::processFrameReal(const QPixmap &frame)
{
    if (!m_tesseract) {
        emit processingError("OCR not initialized");
        return;
    }
    
    GameData data;
    data.debugInfo = "OCR Debug: ";
    
    try {
        // STEP 1: Detect game state (MOST IMPORTANT)
        data.gameState = detectGameState(frame);
        data.isSpectating = detectSpectatorMode(frame);
        data.isAlive = !data.isSpectating;  // If spectating, definitely not alive
        
        data.debugInfo += QString("State:%1 Spectating:%2 Alive:%3 ")
                         .arg((int)data.gameState)
                         .arg(data.isSpectating ? "YES" : "NO")
                         .arg(data.isAlive ? "YES" : "NO");
        
        // STEP 2: Process money ONLY if not spectating
        if (!data.isSpectating) {
            if (data.gameState == GameState::BUY_PHASE) {
                processBuyPhaseMoney(frame, data);
            } else if (data.gameState == GameState::IN_ROUND) {
                processPlayerMoney(frame, data);
            } else {
                data.money = 0;  // Don't process money in other states
                data.debugInfo += "Money skipped (wrong state) ";
            }
        } else {
            skipSpectatorMoney(data);
        }
        
        // STEP 3: Always try to read score (works in all states)
        QString scoreText = extractTextFromRegion(frame, m_scoreboardRegion, "Score");
        auto scorePair = extractScore(scoreText);
        data.ctScore = scorePair.first;
        data.tScore = scorePair.second;
        data.debugInfo += QString("ScoreText:'%1' CT:%2 T:%3 ").arg(scoreText).arg(data.ctScore).arg(data.tScore);
        
        // STEP 4: Detect side (only if not spectating)
        if (!data.isSpectating) {
            data.side = detectSide(frame);
            data.debugInfo += QString("Side:%1 ").arg(data.side);
        } else {
            data.side = "Unknown";
            data.debugInfo += "Side:Unknown(spectating) ";
        }
        
        // STEP 5: Validation
        data.dataValid = isValidGameData(data);
        data.debugInfo += QString("Valid:%1").arg(data.dataValid ? "YES" : "NO");
        
        logDebugInfo(data.debugInfo);
        emit gameDataReady(data);
        
    } catch (const std::exception& e) {
        emit processingError(QString("OCR processing failed: %1").arg(e.what()));
    }
}
#else
void OCRProcessor::processFrameReal(const QPixmap &frame)
{
    processFrameMock(frame);
}
#endif

void OCRProcessor::processFrameMock(const QPixmap &frame)
{
    GameData data;
    static int mockMoney = 3200;
    static int roundCount = 0;
    static bool mockSpectating = false;
    
    roundCount++;
    
    // Simulate spectator mode occasionally (every 25 rounds)
    if (roundCount % 25 == 0) {
        mockSpectating = !mockSpectating;
    }
    
    data.isSpectating = mockSpectating;
    data.isAlive = !mockSpectating;
    data.gameState = mockSpectating ? GameState::SPECTATING : GameState::IN_ROUND;
    
    if (!mockSpectating) {
        // Simulate realistic money changes
        if (roundCount % 10 == 0) {
            mockMoney = QRandomGenerator::global()->bounded(800, 4000);
        } else {
            int change = QRandomGenerator::global()->bounded(-300, 500);
            mockMoney += change;
        }
        
        if (mockMoney < 0) mockMoney = 0;
        if (mockMoney > 16000) mockMoney = 16000;
        
        data.money = mockMoney;
    } else {
        // Simulate teammate money when spectating
        data.money = QRandomGenerator::global()->bounded(1000, 8000);
    }
    
    data.ctScore = 7 + (roundCount / 5) % 8;
    data.tScore = 5 + (roundCount / 7) % 8;
    data.side = (roundCount % 30 < 15) ? "CT" : "T";
    data.dataValid = !mockSpectating;  // Mock data invalid when spectating
    
    data.debugInfo = QString("MOCK - Spectating:%1 Money:%2 State:%3")
                    .arg(data.isSpectating ? "YES" : "NO")
                    .arg(data.money)
                    .arg((int)data.gameState);
    
    logDebugInfo(data.debugInfo);
    emit gameDataReady(data);
}

GameState OCRProcessor::detectGameState(const QPixmap &frame)
{
    // Check spectator mode FIRST (highest priority)
    if (detectSpectatorMode(frame)) {
        return GameState::SPECTATING;
    }
    
    // Check buy phase
    if (detectBuyPhase(frame)) {
        return GameState::BUY_PHASE;
    }
    
    // Check round end (could add this later)
    if (detectRoundEnd(frame)) {
        return GameState::ROUND_END;
    }
    
    // Default to in-round
    return GameState::IN_ROUND;
}

bool OCRProcessor::detectSpectatorMode(const QPixmap &frame)
{
    // PRIMARY DETECTION: Look for spectator bar overlay
    QImage spectatorBar = frame.copy(m_spectatorBarRegion).toImage();
    bool hasOverlay = hasSpectatorOverlay(spectatorBar);
    
    if (hasOverlay) {
        if (m_debugMode) {
            logDebugInfo("SPECTATOR detected via bottom overlay bar");
        }
        return true;
    }
    
    // BACKUP DETECTION: Check chat for death messages
    bool chatIndicates = detectSpectatorFromChat(frame);
    if (chatIndicates) {
        if (m_debugMode) {
            logDebugInfo("SPECTATOR detected via chat death messages");
        }
        return true;
    }
    
    return false;
}

bool OCRProcessor::hasSpectatorOverlay(const QImage &spectatorBar)
{
    // The spectator bar has much more complex content than just CT/T symbol
    // Look for:
    // 1. Player profile pictures (circular patterns)
    // 2. Player names (text)  
    // 3. Weapon icons (detailed graphics)
    // 4. Much more visual complexity than simple CT/T symbol
    
    return hasComplexContent(spectatorBar);
}

bool OCRProcessor::hasComplexContent(const QImage &region)
{
    if (region.isNull() || region.width() < 50 || region.height() < 20) {
        return false;
    }
    
    // More specific detection for spectator bar
    // Look for player profile picture patterns and text density
    
    // Count different color zones
    QSet<QRgb> uniqueColors;
    int textPixels = 0;  // Dark pixels that could be text
    int totalPixels = 0;
    
    for (int y = 0; y < region.height(); ++y) {
        for (int x = 0; x < region.width(); ++x) {
            QRgb pixel = region.pixel(x, y);
            uniqueColors.insert(pixel);
            totalPixels++;
            
            // Count pixels that look like text (dark)
            QColor color(pixel);
            if (color.lightness() < 100) {
                textPixels++;
            }
        }
    }
    
    // Spectator bar should have:
    // 1. HIGH color diversity (profile pictures, weapon icons)
    // 2. Significant text content (player names)
    // 3. Large filled area (not mostly empty like CT/T symbol area)
    
    double colorDiversity = (double)uniqueColors.size() / totalPixels;
    double textDensity = (double)textPixels / totalPixels;
    bool hasHighColorCount = uniqueColors.size() > 50;  // Increased threshold
    bool hasSignificantText = textDensity > 0.15;       // At least 15% text
    bool isFilledArea = textPixels > 100;               // Minimum text pixels
    
    bool isSpectatorBar = hasHighColorCount && hasSignificantText && isFilledArea;
    
    if (m_debugMode) {
        logDebugInfo(QString("Spectator complexity: Colors=%1, ColorDiv=%2, TextDensity=%3, TextPixels=%4, IsSpectator=%5")
                    .arg(uniqueColors.size())
                    .arg(colorDiversity, 0, 'f', 3)
                    .arg(textDensity, 0, 'f', 3)
                    .arg(textPixels)
                    .arg(isSpectatorBar ? "YES" : "NO"));
    }
    
    return isSpectatorBar;
}

bool OCRProcessor::detectSpectatorFromChat(const QPixmap &frame)
{
    QString chatText = extractTextFromRegion(frame, m_chatRegion, "Chat");
    return hasSpectatorChatIndicators(chatText);
}

bool OCRProcessor::hasSpectatorChatIndicators(const QString &chatText)
{
    if (chatText.isEmpty()) {
        return false;
    }
    
    QString lowerText = chatText.toLower();
    
    // Look for death/spectator indicators in chat
    QStringList spectatorPatterns = {
        "☠",                    // Skull emoji (death)
        "💀",                   // Death emoji
        "eliminated",           // Death message
        "killed",              // Kill feed
        "died",                // Death notification
        "spectating",          // Direct spectator text
        "observing",           // Observer mode
        "watching"             // Watching player
    };
    
    for (const QString &pattern : spectatorPatterns) {
        if (lowerText.contains(pattern)) {
            if (m_debugMode) {
                logDebugInfo(QString("Found spectator chat indicator: '%1'").arg(pattern));
            }
            return true;
        }
    }
    
    return false;
}

bool OCRProcessor::detectPlayerAlive(const QPixmap &frame)
{
    // If we detect spectator mode, player is definitely not alive
    return !detectSpectatorMode(frame);
}

bool OCRProcessor::detectBuyPhase(const QPixmap &frame)
{
    QString buyText = extractTextFromRegion(frame, m_buyTimerRegion, "BuyTimer");
    
    // Look for "Buy Time Remaining" text
    bool isBuyPhase = buyText.contains("Buy Time Remaining", Qt::CaseInsensitive) ||
                     buyText.contains("00:", Qt::CaseInsensitive);
    
    if (m_debugMode && isBuyPhase) {
        logDebugInfo(QString("BUY PHASE detected: '%1'").arg(buyText));
    }
    
    return isBuyPhase;
}

bool OCRProcessor::detectRoundEnd(const QPixmap &frame)
{
    // Could implement round end detection here if needed
    return false;
}

QString OCRProcessor::detectSide(const QPixmap &frame)
{
    QImage symbolRegion = frame.copy(m_sideSymbolRegion).toImage();
    
    // Use SHAPE recognition, not color (to work with any UI theme)
    bool isCT = detectCTSymbol(symbolRegion);
    bool isT = detectTSymbol(symbolRegion);
    
    if (isCT && !isT) {
        if (m_debugMode) {
            logDebugInfo("Side detected: CT (wing pattern found)");
        }
        return "CT";
    } else if (isT && !isCT) {
        if (m_debugMode) {
            logDebugInfo("Side detected: T (star pattern found)");
        }
        return "T";
    }
    
    if (m_debugMode) {
        logDebugInfo(QString("Side detection unclear - CT:%1 T:%2").arg(isCT ? "YES" : "NO").arg(isT ? "YES" : "NO"));
    }
    
    return "Unknown";
}

bool OCRProcessor::detectCTSymbol(const QImage &symbolRegion)
{
    // Look for CT symbol characteristics: wings + crossed tools
    return hasWingPattern(symbolRegion) && hasCrossedTools(symbolRegion);
}

bool OCRProcessor::detectTSymbol(const QImage &symbolRegion)
{
    // Look for T symbol characteristics: star + curved blades
    return hasStarPattern(symbolRegion) && hasCurvedBlades(symbolRegion);
}

bool OCRProcessor::hasWingPattern(const QImage &region)
{
    // Look for curved wing-like patterns typical of CT symbol
    // This is a simplified implementation - could be made more sophisticated
    if (region.isNull() || region.width() < 20 || region.height() < 20) {
        return false;
    }
    
    // Look for curved patterns in the outer areas (wings spread out)
    int leftCurves = 0, rightCurves = 0;
    
    for (int y = region.height() / 4; y < 3 * region.height() / 4; ++y) {
        // Check left side for wing curve
        for (int x = 0; x < region.width() / 3; ++x) {
            QColor pixel = region.pixelColor(x, y);
            if (pixel.lightness() < 128) {  // Dark pixel (symbol part)
                leftCurves++;
            }
        }
        
        // Check right side for wing curve
        for (int x = 2 * region.width() / 3; x < region.width(); ++x) {
            QColor pixel = region.pixelColor(x, y);
            if (pixel.lightness() < 128) {  // Dark pixel (symbol part)
                rightCurves++;
            }
        }
    }
    
    // Wings should have significant presence on both sides
    bool hasWings = leftCurves > 10 && rightCurves > 10;
    
    if (m_debugMode) {
        logDebugInfo(QString("Wing pattern check - Left curves: %1, Right curves: %2, Has wings: %3")
                    .arg(leftCurves).arg(rightCurves).arg(hasWings ? "YES" : "NO"));
    }
    
    return hasWings;
}

bool OCRProcessor::hasCrossedTools(const QImage &region)
{
    // Look for crossed line patterns in center (tools/compass)
    if (region.isNull()) return false;
    
    int centerX = region.width() / 2;
    int centerY = region.height() / 2;
    int crossPixels = 0;
    
    // Check for crossing lines around center
    for (int i = -5; i <= 5; ++i) {
        // Check horizontal line
        if (centerY + i >= 0 && centerY + i < region.height()) {
            for (int x = centerX - 10; x <= centerX + 10; ++x) {
                if (x >= 0 && x < region.width()) {
                    QColor pixel = region.pixelColor(x, centerY + i);
                    if (pixel.lightness() < 128) {
                        crossPixels++;
                    }
                }
            }
        }
        
        // Check vertical line
        if (centerX + i >= 0 && centerX + i < region.width()) {
            for (int y = centerY - 10; y <= centerY + 10; ++y) {
                if (y >= 0 && y < region.height()) {
                    QColor pixel = region.pixelColor(centerX + i, y);
                    if (pixel.lightness() < 128) {
                        crossPixels++;
                    }
                }
            }
        }
    }
    
    bool hasCross = crossPixels > 20;
    
    if (m_debugMode) {
        logDebugInfo(QString("Crossed tools check - Cross pixels: %1, Has cross: %2")
                    .arg(crossPixels).arg(hasCross ? "YES" : "NO"));
    }
    
    return hasCross;
}

bool OCRProcessor::hasStarPattern(const QImage &region)
{
    // Look for 5-pointed star pattern typical of T symbol
    if (region.isNull() || region.width() < 20 || region.height() < 20) {
        return false;
    }
    
    int centerX = region.width() / 2;
    int centerY = region.height() / 2;
    int starPixels = 0;
    
    // Check for star points in 5 directions
    QList<QPair<int, int>> starDirections = {
        {0, -1},      // Top
        {1, -1},      // Top-right
        {1, 1},       // Bottom-right
        {-1, 1},      // Bottom-left
        {-1, -1}      // Top-left
    };
    
    for (const auto &direction : starDirections) {
        for (int dist = 3; dist < 15; ++dist) {
            int x = centerX + direction.first * dist;
            int y = centerY + direction.second * dist;
            
            if (x >= 0 && x < region.width() && y >= 0 && y < region.height()) {
                QColor pixel = region.pixelColor(x, y);
                if (pixel.lightness() < 128) {
                    starPixels++;
                }
            }
        }
    }
    
    bool hasStar = starPixels > 15;
    
    if (m_debugMode) {
        logDebugInfo(QString("Star pattern check - Star pixels: %1, Has star: %2")
                    .arg(starPixels).arg(hasStar ? "YES" : "NO"));
    }
    
    return hasStar;
}

bool OCRProcessor::hasCurvedBlades(const QImage &region)
{
    // Look for curved knife/sword patterns below star
    if (region.isNull()) return false;
    
    int centerX = region.width() / 2;
    int lowerY = 3 * region.height() / 4;
    int curvePixels = 0;
    
    // Check for curved patterns in lower area
    for (int y = region.height() / 2; y < region.height(); ++y) {
        for (int x = centerX - 15; x <= centerX + 15; ++x) {
            if (x >= 0 && x < region.width()) {
                QColor pixel = region.pixelColor(x, y);
                if (pixel.lightness() < 128) {
                    curvePixels++;
                }
            }
        }
    }
    
    bool hasCurves = curvePixels > 20;
    
    if (m_debugMode) {
        logDebugInfo(QString("Curved blades check - Curve pixels: %1, Has curves: %2")
                    .arg(curvePixels).arg(hasCurves ? "YES" : "NO"));
    }
    
    return hasCurves;
}

void OCRProcessor::processPlayerMoney(const QPixmap &frame, GameData &data)
{
    QString moneyText = extractTextFromRegion(frame, m_moneyRegion, "Money");
    data.money = extractMoney(moneyText);
    data.debugInfo += QString("MoneyText:'%1' ParsedMoney:%2 ").arg(moneyText).arg(data.money);
}

void OCRProcessor::processBuyPhaseMoney(const QPixmap &frame, GameData &data)
{
    QString moneyText = extractTextFromRegion(frame, m_buyMoneyRegion, "BuyMoney");
    data.money = extractMoney(moneyText);
    data.debugInfo += QString("BuyMoneyText:'%1' ParsedMoney:%2 ").arg(moneyText).arg(data.money);
}

void OCRProcessor::skipSpectatorMoney(GameData &data)
{
    data.money = 0;  // Don't use teammate's money
    data.debugInfo += "Money SKIPPED (spectating teammate) ";
}

QString OCRProcessor::extractTextFromRegion(const QPixmap &frame, const QRect &region, const QString &debugName)
{
#ifdef TESSERACT_AVAILABLE
    if (!m_usingRealOCR) return "";
    
    tesseract::TessBaseAPI* api = static_cast<tesseract::TessBaseAPI*>(m_tesseract);
    if (!api) return "";
    
    QPixmap regionPixmap = frame.copy(region);
    QImage regionImage = regionPixmap.toImage();
    QImage processedImage = preprocessImage(regionImage);
    
    if (m_debugMode && !debugName.isEmpty()) {
        saveDebugImage(processedImage, QString("debug_%1_%2.png")
                      .arg(debugName)
                      .arg(QDateTime::currentDateTime().toString("hhmmss")));
    }
    
    QImage rgbImage = processedImage.convertToFormat(QImage::Format_RGB888);
    api->SetImage(rgbImage.bits(), rgbImage.width(), rgbImage.height(), 3, rgbImage.bytesPerLine());
    
    char* text = api->GetUTF8Text();
    QString result = QString::fromUtf8(text ? text : "");
    delete[] text;
    
    if (m_debugMode && !debugName.isEmpty()) {
        logDebugInfo(QString("%1 OCR result: '%2' from region (%3,%4,%5,%6)")
                    .arg(debugName)
                    .arg(result.trimmed())
                    .arg(region.x()).arg(region.y()).arg(region.width()).arg(region.height()));
    }
    
    return result.trimmed();
#else
    Q_UNUSED(frame)
    Q_UNUSED(region)
    Q_UNUSED(debugName)
    return QString::number(QRandomGenerator::global()->bounded(800, 16000));
#endif
}

QImage OCRProcessor::preprocessImage(const QImage &input)
{
    QImage processed = input;
    processed = processed.convertToFormat(QImage::Format_Grayscale8);
    processed = processed.scaled(processed.size() * 3, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    return processed;
}

int OCRProcessor::extractMoney(const QString &text)
{
    QString cleanText = text;
    cleanText.remove(QRegularExpression("[^0-9]"));
    
    bool ok;
    int money = cleanText.toInt(&ok);
    
    if (!ok || !isValidMoney(money)) {
        return 0;
    }
    
    return money;
}

QPair<int, int> OCRProcessor::extractScore(const QString &text)
{
    QRegularExpression scorePattern(R"((\d+)\s*[-–]\s*(\d+))");
    QRegularExpressionMatch match = scorePattern.match(text);
    
    if (match.hasMatch()) {
        bool ok1, ok2;
        int score1 = match.captured(1).toInt(&ok1);
        int score2 = match.captured(2).toInt(&ok2);
        
        if (ok1 && ok2 && isValidScore(score1, score2)) {
            return QPair<int, int>(score1, score2);
        }
    }
    
    return QPair<int, int>(0, 0);
}

bool OCRProcessor::isValidMoney(int money)
{
    return money >= 0 && money <= 16000;
}

bool OCRProcessor::isValidScore(int ctScore, int tScore)
{
    return ctScore >= 0 && ctScore <= 16 && tScore >= 0 && tScore <= 16;
}

bool OCRProcessor::isValidGameData(const GameData &data)
{
    if (data.isSpectating) {
        // When spectating, money validation is not relevant
        return isValidScore(data.ctScore, data.tScore);
    }
    
    // When playing, validate money and scores
    return isValidMoney(data.money) && isValidScore(data.ctScore, data.tScore);
}

void OCRProcessor::saveDebugFrame(const QPixmap &frame)
{
    QString debugDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/CSCAN_Debug";
    QDir().mkpath(debugDir);
    
    QString timestamp = QDateTime::currentDateTime().toString("hhmmss");
    QString filename = QString("%1/fullframe_%2.png").arg(debugDir).arg(timestamp);
    frame.save(filename);
    
    // Also save important regions
    frame.copy(m_moneyRegion).save(QString("%1/money_%2.png").arg(debugDir).arg(timestamp));
    frame.copy(m_scoreboardRegion).save(QString("%1/scoreboard_%2.png").arg(debugDir).arg(timestamp));
    frame.copy(m_spectatorBarRegion).save(QString("%1/spectator_%2.png").arg(debugDir).arg(timestamp));
    frame.copy(m_sideSymbolRegion).save(QString("%1/sidesymbol_%2.png").arg(debugDir).arg(timestamp));
    
    logDebugInfo(QString("Debug frame saved to: %1").arg(filename));
}

void OCRProcessor::saveDebugImage(const QImage &image, const QString &filename)
{
    QString debugDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/CSCAN_Debug";
    QDir().mkpath(debugDir);
    
    QString fullPath = debugDir + "/" + filename;
    image.save(fullPath);
}

void OCRProcessor::logDebugInfo(const QString &info)
{
    if (m_debugMode) {
        qDebug() << "[OCR DEBUG]" << info;
        emit debugMessage(info);
    }
}

void OCRProcessor::setDebugMode(bool enabled)
{
    m_debugMode = enabled;
    logDebugInfo(QString("Debug mode %1").arg(enabled ? "enabled" : "disabled"));
}

void OCRProcessor::startCalibration()
{
    m_calibrationMode = true;
    emit calibrationNeeded("Calibration mode activated - use spinboxes to adjust regions");
}

void OCRProcessor::setMoneyRegion(const QRect &region)
{
    m_moneyRegion = region;
    logDebugInfo(QString("Money region updated to: %1,%2,%3,%4").arg(region.x()).arg(region.y()).arg(region.width()).arg(region.height()));
}

void OCRProcessor::setScoreboardRegion(const QRect &region)
{
    m_scoreboardRegion = region;
    logDebugInfo(QString("Scoreboard region updated to: %1,%2,%3,%4").arg(region.x()).arg(region.y()).arg(region.width()).arg(region.height()));
}

void OCRProcessor::saveCalibration()
{
    m_settings->setValue("moneyRegion", m_moneyRegion);
    m_settings->setValue("buyMoneyRegion", m_buyMoneyRegion);
    m_settings->setValue("scoreboardRegion", m_scoreboardRegion);
    m_settings->setValue("sideSymbolRegion", m_sideSymbolRegion);
    m_settings->setValue("spectatorBarRegion", m_spectatorBarRegion);
    m_settings->setValue("chatRegion", m_chatRegion);
    m_settings->setValue("buyTimerRegion", m_buyTimerRegion);
    
    m_calibrationMode = false;
    logDebugInfo("Calibration settings saved successfully");
}

void OCRProcessor::loadCalibration()
{
    if (m_settings->contains("moneyRegion")) {
        m_moneyRegion = m_settings->value("moneyRegion").toRect();
        m_buyMoneyRegion = m_settings->value("buyMoneyRegion").toRect();
        m_scoreboardRegion = m_settings->value("scoreboardRegion").toRect();
        m_sideSymbolRegion = m_settings->value("sideSymbolRegion").toRect();
        m_spectatorBarRegion = m_settings->value("spectatorBarRegion").toRect();
        m_chatRegion = m_settings->value("chatRegion").toRect();
        m_buyTimerRegion = m_settings->value("buyTimerRegion").toRect();
        
        logDebugInfo("Loaded saved calibration settings");
    } else {
        logDebugInfo("No saved calibration found - using default calibrated regions");
    }
}

bool OCRProcessor::isRegionEmpty(const QPixmap &frame, const QRect &region)
{
    QImage regionImage = frame.copy(region).toImage();
    
    // Check if region is mostly empty/uniform
    QColor firstPixel = regionImage.pixelColor(0, 0);
    int similarPixels = 0;
    int totalPixels = regionImage.width() * regionImage.height();
    
    for (int y = 0; y < regionImage.height(); ++y) {
        for (int x = 0; x < regionImage.width(); ++x) {
            QColor pixel = regionImage.pixelColor(x, y);
            int colorDiff = qAbs(pixel.red() - firstPixel.red()) + 
                           qAbs(pixel.green() - firstPixel.green()) + 
                           qAbs(pixel.blue() - firstPixel.blue());
            
            if (colorDiff < 30) {  // Similar color
                similarPixels++;
            }
        }
    }
    
    // If more than 90% of pixels are similar, consider it empty
    return (double)similarPixels / totalPixels > 0.9;
}

QColor OCRProcessor::getDominantColor(const QPixmap &frame, const QRect &region)
{
    QImage regionImage = frame.copy(region).toImage();
    
    if (regionImage.isNull()) {
        return QColor();
    }
    
    // Simple dominant color calculation
    long long totalRed = 0, totalGreen = 0, totalBlue = 0;
    int totalPixels = regionImage.width() * regionImage.height();
    
    for (int y = 0; y < regionImage.height(); ++y) {
        for (int x = 0; x < regionImage.width(); ++x) {
            QColor pixel = regionImage.pixelColor(x, y);
            totalRed += pixel.red();
            totalGreen += pixel.green();
            totalBlue += pixel.blue();
        }
    }
    
    if (totalPixels > 0) {
        return QColor(totalRed / totalPixels, totalGreen / totalPixels, totalBlue / totalPixels);
    }
    
    return QColor();
}

bool OCRProcessor::hasText(const QPixmap &frame, const QRect &region)
{
    QString text = extractTextFromRegion(frame, region, "TextCheck");
    return !text.isEmpty() && text.length() > 1;
}
