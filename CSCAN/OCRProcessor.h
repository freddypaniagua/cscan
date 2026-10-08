#ifndef OCRPROCESSOR_H
#define OCRPROCESSOR_H

#include <QObject>
#include <QPixmap>
#include <QRect>
#include <QString>
#include <QSettings>

enum class GameState {
    IN_ROUND,
    BUY_PHASE,
    ROUND_END,
    SPECTATING,
    DEAD,
    UNKNOWN
};

struct GameData {
    int money = 0;
    int ctScore = 0;
    int tScore = 0;
    QString side = "CT";
    bool roundEnded = false;
    bool buyPhaseActive = false;
    bool dataValid = false;
    bool isSpectating = false;
    bool isAlive = true;
    GameState gameState = GameState::UNKNOWN;
    QString debugInfo = "";  // For debugging what OCR reads
};

class OCRProcessor : public QObject
{
    Q_OBJECT
    
public:
    explicit OCRProcessor(QObject *parent = nullptr);
    ~OCRProcessor();
    
    void processFrame(const QPixmap &frame);
    
    // Calibration mode
    void startCalibration();
    void setMoneyRegion(const QRect &region);
    void setScoreboardRegion(const QRect &region);
    void saveCalibration();
    void loadCalibration();
    
    // Debug mode
    void setDebugMode(bool enabled);
    void saveDebugFrame(const QPixmap &frame);
    
    // Current regions (calibrated for your setup)
    QRect getMoneyRegion() const { return m_moneyRegion; }
    QRect getScoreboardRegion() const { return m_scoreboardRegion; }
    QRect getBuyTimerRegion() const { return m_buyTimerRegion; }
    QRect getSpectatorBarRegion() const { return m_spectatorBarRegion; }
    
signals:
    void gameDataReady(const GameData &data);
    void processingError(const QString &error);
    void calibrationNeeded(const QString &message);
    void debugMessage(const QString &message);
    
private:
    void* m_tesseract;
    bool m_usingRealOCR = false;
    bool m_debugMode = false;
    bool m_calibrationMode = false;
    
    // Calibrated regions based on your screenshots
    QRect m_moneyRegion;              // Bottom-left: $400, $950, $14200, etc.
    QRect m_buyMoneyRegion;           // Top-left during buy phase: $9700, $4550
    QRect m_scoreboardRegion;         // Top-center: "1 0", "7 8", "5 5"
    QRect m_sideSymbolRegion;         // Bottom-center: CT/T symbol area
    QRect m_buyTimerRegion;           // Center-top: "Buy Time Remaining 00:33"
    QRect m_spectatorBarRegion;       // Bottom-center: spectator player info bar
    QRect m_chatRegion;               // Left side: chat/kill feed for backup detection
    QRect m_healthRegion;             // To avoid: "82", "100" health
    QRect m_armorRegion;              // To avoid: "100" armor
    QRect m_ammoRegion;               // To avoid: "4 90" ammo
    
    // Settings for persistence
    QSettings *m_settings;
    
    // Initialization methods
    void initializeRealOCR();
    void initializeMockOCR();
    void initializeCalibratedRegions();  // Your exact coordinates
    
    // Processing methods
    void processFrameReal(const QPixmap &frame);
    void processFrameMock(const QPixmap &frame);
    
    // Game state detection (primary systems)
    GameState detectGameState(const QPixmap &frame);
    bool detectSpectatorMode(const QPixmap &frame);        // Primary: spectator bar
    bool detectSpectatorFromChat(const QPixmap &frame);    // Backup: chat analysis
    bool detectPlayerAlive(const QPixmap &frame);
    bool detectBuyPhase(const QPixmap &frame);             // "Buy Time Remaining"
    bool detectRoundEnd(const QPixmap &frame);
    
    // Spectator detection methods
    bool hasSpectatorOverlay(const QImage &spectatorBar);  // Primary detection
    bool hasSpectatorChatIndicators(const QString &chatText); // Backup detection
    
    // Side detection (shape-based, not color)
    QString detectSide(const QPixmap &frame);
    bool detectCTSymbol(const QImage &symbolRegion);       // Wing pattern detection
    bool detectTSymbol(const QImage &symbolRegion);        // Star pattern detection
    
    // OCR methods with debug output
    QString extractTextFromRegion(const QPixmap &frame, const QRect &region, const QString &debugName = "");
    QImage preprocessImage(const QImage &input);
    int extractMoney(const QString &text);
    QPair<int, int> extractScore(const QString &text);
    
    // Shape recognition helpers
    bool hasWingPattern(const QImage &region);             // CT symbol wings
    bool hasCrossedTools(const QImage &region);            // CT symbol tools
    bool hasStarPattern(const QImage &region);             // T symbol star
    bool hasCurvedBlades(const QImage &region);            // T symbol knives
    
    // Debug helpers
    void saveDebugImage(const QImage &image, const QString &filename);
    void logDebugInfo(const QString &info);
    
    // Screen analysis helpers
    bool isRegionEmpty(const QPixmap &frame, const QRect &region);
    QColor getDominantColor(const QPixmap &frame, const QRect &region);
    bool hasText(const QPixmap &frame, const QRect &region);
    bool hasComplexContent(const QImage &region);          // For spectator bar detection
    
    // Money processing logic
    void processPlayerMoney(const QPixmap &frame, GameData &data);
    void processBuyPhaseMoney(const QPixmap &frame, GameData &data);
    void skipSpectatorMoney(GameData &data);
    
    // Validation helpers
    bool isValidMoney(int money);
    bool isValidScore(int ctScore, int tScore);
    bool isValidGameData(const GameData &data);
};

#endif // OCRPROCESSOR_H
