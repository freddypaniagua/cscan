#ifndef OCRPROCESSOR_H
#define OCRPROCESSOR_H

#include <QObject>
#include <QPixmap>
#include <QRect>
#include <QString>

struct GameData {
    int money = 0;
    int ctScore = 0;
    int tScore = 0;
    QString side = "CT";
    bool roundEnded = false;
    bool buyPhaseActive = false;
    bool dataValid = false;
};

class OCRProcessor : public QObject
{
    Q_OBJECT

public:
    explicit OCRProcessor(QObject* parent = nullptr);
    ~OCRProcessor();

    void processFrame(const QPixmap& frame);

    // Screen regions for 1920x1080 (we'll make these configurable later)
    static const QRect MONEY_REGION;
    static const QRect SCOREBOARD_REGION;
    static const QRect BUY_TIMER_REGION;

signals:
    void gameDataReady(const GameData& data);
    void processingError(const QString& error);

private:
    void* m_tesseract;
    bool m_usingRealOCR = false;

    // Initialization methods
    void initializeRealOCR();
    void initializeMockOCR();

    // Processing methods
    void processFrameReal(const QPixmap& frame);
    void processFrameMock(const QPixmap& frame);

    // OCR methods
    QString extractTextFromRegion(const QPixmap& frame, const QRect& region);
    QImage preprocessImage(const QImage& input);
    int extractMoney(const QString& text);
    QPair<int, int> extractScore(const QString& text);
    QString detectSide(const QPixmap& frame);
    bool detectRoundEnd(const QPixmap& frame);
    bool detectBuyPhase(const QPixmap& frame);

    // Debug helpers
    void saveDebugImage(const QImage& image, const QString& filename);
};

#endif // OCRPROCESSOR_H