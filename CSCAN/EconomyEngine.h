#ifndef ECONOMYENGINE_H
#define ECONOMYENGINE_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QMap>

// Forward declaration
struct GameData;

enum class BuyDecision {
    FULL_BUY,
    FORCE_BUY,
    ECO_ROUND,
    PISTOL_ARMOR,
    ANTI_ECO,
    SAVE_ROUND
};

enum class WeaponType {
    RIFLE,
    SMG,
    PISTOL,
    AWP,
    SHOTGUN
};

struct WeaponInfo {
    QString name;
    int price;
    WeaponType type;
    QString side; // "CT", "T", or "BOTH"
    int effectiveness; // 1-10 rating
};

struct BuyRecommendation {
    BuyDecision decision;
    QString strategy;
    QStringList weapons;
    QStringList utility;
    QStringList armor;
    int totalCost;
    int remainingMoney;
    QString reasoning;
    int confidence; // 1-100 confidence in recommendation
};

class EconomyEngine : public QObject
{
    Q_OBJECT

public:
    explicit EconomyEngine(QObject* parent = nullptr);

    // Main recommendation function
    BuyRecommendation generateRecommendation(const GameData& gameData);

    // Economy analysis
    bool shouldForceBuy(int money, int roundsLost, const QString& side);
    bool shouldEco(int money, int teamMoney, int roundsLost);
    bool isAntiEcoRound(int money, int enemyMoney);

    // Weapon selection
    QStringList getBestWeapons(int money, const QString& side, BuyDecision decision);
    QStringList getBestUtility(int money, const QString& side, BuyDecision decision);
    QString getBestArmor(int money, BuyDecision decision);

signals:
    void recommendationReady(const BuyRecommendation& recommendation);

private:
    // Weapon database
    void initializeWeaponDatabase();
    QMap<QString, WeaponInfo> m_weapons;
    QMap<QString, int> m_utilityPrices;
    QMap<QString, int> m_armorPrices;

    // Economic calculations
    int calculateTeamEconomy(const GameData& gameData);
    int estimateEnemyEconomy(const GameData& gameData);
    BuyDecision decideBuyStrategy(const GameData& gameData);

    // Round analysis
    bool isImportantRound(int ctScore, int tScore);
    int getRoundImportance(int ctScore, int tScore, const QString& side);

    // Strategy helpers
    QString generateStrategyText(BuyDecision decision, const QString& side);
    QString generateReasoningText(const GameData& gameData, BuyDecision decision);

    // CS2 Economy Constants
    static const int LOSS_BONUS_ROUND_1 = 1400;
    static const int LOSS_BONUS_ROUND_2 = 1900;
    static const int LOSS_BONUS_ROUND_3 = 2400;
    static const int LOSS_BONUS_ROUND_4 = 2900;
    static const int LOSS_BONUS_ROUND_5 = 3400;
    static const int WIN_BONUS = 3250;
    static const int PLANT_BONUS = 800;
    static const int DEFUSE_BONUS = 250;

    // Money thresholds
    static const int FULL_BUY_THRESHOLD = 4000;
    static const int FORCE_BUY_THRESHOLD = 2000;
    static const int ECO_THRESHOLD = 2500;
};

#endif // ECONOMYENGINE_H