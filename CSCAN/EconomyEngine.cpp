#include "EconomyEngine.h"
#include "OCRProcessor.h" // For GameData struct
#include <QDebug>

EconomyEngine::EconomyEngine(QObject* parent)
    : QObject(parent)
{
    initializeWeaponDatabase();
    qDebug() << "Economy Engine initialized with CS2 weapon database";
}

void EconomyEngine::initializeWeaponDatabase()
{
    // Rifles
    m_weapons["AK-47"] = { "AK-47", 2700, WeaponType::RIFLE, "T", 9 };
    m_weapons["M4A4"] = { "M4A4", 3100, WeaponType::RIFLE, "CT", 8 };
    m_weapons["M4A1-S"] = { "M4A1-S", 2900, WeaponType::RIFLE, "CT", 8 };
    m_weapons["Galil AR"] = { "Galil AR", 1800, WeaponType::RIFLE, "T", 6 };
    m_weapons["FAMAS"] = { "FAMAS", 2050, WeaponType::RIFLE, "CT", 6 };

    // SMGs
    m_weapons["MAC-10"] = { "MAC-10", 1050, WeaponType::SMG, "T", 5 };
    m_weapons["MP9"] = { "MP9", 1250, WeaponType::SMG, "CT", 5 };
    m_weapons["MP5-SD"] = { "MP5-SD", 1500, WeaponType::SMG, "CT", 6 };
    m_weapons["UMP-45"] = { "UMP-45", 1200, WeaponType::SMG, "BOTH", 6 };
    m_weapons["P90"] = { "P90", 2350, WeaponType::SMG, "BOTH", 7 };

    // AWP
    m_weapons["AWP"] = { "AWP", 4750, WeaponType::AWP, "BOTH", 10 };

    // Pistols
    m_weapons["Glock-18"] = { "Glock-18", 0, WeaponType::PISTOL, "T", 3 };
    m_weapons["USP-S"] = { "USP-S", 0, WeaponType::PISTOL, "CT", 3 };
    m_weapons["P250"] = { "P250", 300, WeaponType::PISTOL, "BOTH", 4 };
    m_weapons["Five-SeveN"] = { "Five-SeveN", 500, WeaponType::PISTOL, "CT", 5 };
    m_weapons["Tec-9"] = { "Tec-9", 500, WeaponType::PISTOL, "T", 5 };
    m_weapons["Desert Eagle"] = { "Desert Eagle", 700, WeaponType::PISTOL, "BOTH", 6 };

    // Utility prices
    m_utilityPrices["HE Grenade"] = 300;
    m_utilityPrices["Flashbang"] = 200;
    m_utilityPrices["Smoke Grenade"] = 300;
    m_utilityPrices["Incendiary"] = 600;
    m_utilityPrices["Molotov"] = 400;
    m_utilityPrices["Decoy Grenade"] = 50;

    // Armor prices
    m_armorPrices["Kevlar Vest"] = 650;
    m_armorPrices["Kevlar + Helmet"] = 1000;
}

BuyRecommendation EconomyEngine::generateRecommendation(const GameData& gameData)
{
    BuyRecommendation recommendation;

    // Analyze current situation
    BuyDecision decision = decideBuyStrategy(gameData);
    recommendation.decision = decision;

    // Get weapon recommendations
    recommendation.weapons = getBestWeapons(gameData.money, gameData.side, decision);
    recommendation.utility = getBestUtility(gameData.money, gameData.side, decision);
    recommendation.armor.append(getBestArmor(gameData.money, decision));

    // Calculate costs
    int weaponCost = 0;
    for (const QString& weapon : recommendation.weapons) {
        if (m_weapons.contains(weapon)) {
            weaponCost += m_weapons[weapon].price;
        }
    }

    int utilityCost = 0;
    for (const QString& utility : recommendation.utility) {
        if (m_utilityPrices.contains(utility)) {
            utilityCost += m_utilityPrices[utility];
        }
    }

    int armorCost = 0;
    for (const QString& armor : recommendation.armor) {
        if (m_armorPrices.contains(armor)) {
            armorCost += m_armorPrices[armor];
        }
    }

    recommendation.totalCost = weaponCost + utilityCost + armorCost;
    recommendation.remainingMoney = gameData.money - recommendation.totalCost;

    // Generate strategy and reasoning
    recommendation.strategy = generateStrategyText(decision, gameData.side);
    recommendation.reasoning = generateReasoningText(gameData, decision);

    // Calculate confidence
    recommendation.confidence = 85; // Base confidence, could be improved with more data

    qDebug() << "Generated recommendation:" << recommendation.strategy
        << "Cost:" << recommendation.totalCost;

    return recommendation;
}

BuyDecision EconomyEngine::decideBuyStrategy(const GameData& gameData)
{
    int money = gameData.money;
    bool isImportant = isImportantRound(gameData.ctScore, gameData.tScore);

    // Full buy conditions
    if (money >= FULL_BUY_THRESHOLD) {
        return BuyDecision::FULL_BUY;
    }

    // Force buy in critical situations
    if (money >= FORCE_BUY_THRESHOLD && isImportant) {
        return BuyDecision::FORCE_BUY;
    }

    // Anti-eco when enemy likely has low money
    if (money >= 2500) {
        return BuyDecision::ANTI_ECO;
    }

    // Eco round when money is low
    if (money < ECO_THRESHOLD) {
        return BuyDecision::ECO_ROUND;
    }

    // Default to pistol + armor
    return BuyDecision::PISTOL_ARMOR;
}

QStringList EconomyEngine::getBestWeapons(int money, const QString& side, BuyDecision decision)
{
    QStringList recommendations;

    switch (decision) {
    case BuyDecision::FULL_BUY:
        if (side == "T") {
            if (money >= 4750) recommendations << "AWP";
            else recommendations << "AK-47";
        }
        else {
            if (money >= 4750) recommendations << "AWP";
            else recommendations << "M4A4";
        }
        break;

    case BuyDecision::FORCE_BUY:
        if (side == "T") {
            if (money >= 2700) recommendations << "AK-47";
            else if (money >= 1800) recommendations << "Galil AR";
            else recommendations << "MAC-10";
        }
        else {
            if (money >= 3100) recommendations << "M4A4";
            else if (money >= 2050) recommendations << "FAMAS";
            else recommendations << "MP9";
        }
        break;

    case BuyDecision::ANTI_ECO:
        if (side == "T") {
            recommendations << "MAC-10";
        }
        else {
            recommendations << "MP9";
        }
        break;

    case BuyDecision::ECO_ROUND:
        // Stick with default pistol or upgrade slightly
        if (money >= 700) recommendations << "Desert Eagle";
        else if (money >= 500) {
            if (side == "T") recommendations << "Tec-9";
            else recommendations << "Five-SeveN";
        }
        else if (money >= 300) recommendations << "P250";
        break;

    case BuyDecision::PISTOL_ARMOR:
        if (money >= 500) {
            if (side == "T") recommendations << "Tec-9";
            else recommendations << "Five-SeveN";
        }
        break;

    default:
        break;
    }

    return recommendations;
}

QStringList EconomyEngine::getBestUtility(int money, const QString& side, BuyDecision decision)
{
    QStringList utility;
    int remainingMoney = money;

    // Calculate money after weapon and armor
    // This is simplified - in real implementation we'd calculate exactly

    switch (decision) {
    case BuyDecision::FULL_BUY:
        if (remainingMoney >= 600) {
            utility << "HE Grenade" << "Flashbang";
            if (side == "T") utility << "Smoke Grenade";
        }
        else if (remainingMoney >= 300) {
            utility << "HE Grenade";
        }
        break;

    case BuyDecision::FORCE_BUY:
        if (remainingMoney >= 500) {
            utility << "Flashbang" << "HE Grenade";
        }
        else if (remainingMoney >= 200) {
            utility << "Flashbang";
        }
        break;

    case BuyDecision::ANTI_ECO:
        utility << "HE Grenade"; // Effective against unarmored enemies
        break;

    default:
        // Eco rounds - minimal utility
        if (remainingMoney >= 200) {
            utility << "Flashbang";
        }
        break;
    }

    return utility;
}

QString EconomyEngine::getBestArmor(int money, BuyDecision decision)
{
    switch (decision) {
    case BuyDecision::FULL_BUY:
    case BuyDecision::FORCE_BUY:
        if (money >= 1000) return "Kevlar + Helmet";
        else if (money >= 650) return "Kevlar Vest";
        break;

    case BuyDecision::PISTOL_ARMOR:
        if (money >= 650) return "Kevlar Vest";
        break;

    case BuyDecision::ANTI_ECO:
        if (money >= 650) return "Kevlar Vest"; // Helmet not needed vs pistols
        break;

    default:
        break;
    }

    return ""; // No armor
}

bool EconomyEngine::isImportantRound(int ctScore, int tScore)
{
    // Match point rounds
    if (ctScore >= 15 || tScore >= 15) return true;

    // Close game situations
    int scoreDiff = abs(ctScore - tScore);
    if (scoreDiff <= 2 && (ctScore + tScore) >= 20) return true;

    // Potential comeback rounds
    if (scoreDiff >= 5) return true;

    return false;
}

QString EconomyEngine::generateStrategyText(BuyDecision decision, const QString& side)
{
    switch (decision) {
    case BuyDecision::FULL_BUY:
        return side == "T" ? "Execute site takes with utility" : "Hold angles and trade kills";

    case BuyDecision::FORCE_BUY:
        return side == "T" ? "Fast rush with limited utility" : "Stack sites and play retake";

    case BuyDecision::ECO_ROUND:
        return side == "T" ? "Stack and hunt for picks" : "Save and avoid engagements";

    case BuyDecision::ANTI_ECO:
        return "Aggressive positioning vs eco";

    case BuyDecision::PISTOL_ARMOR:
        return "Close range engagements";

    default:
        return "Standard round play";
    }
}

QString EconomyEngine::generateReasoningText(const GameData& gameData, BuyDecision decision)
{
    QString reasoning = QString("Money: $%1, Score: %2-%3")
        .arg(gameData.money)
        .arg(gameData.ctScore)
        .arg(gameData.tScore);

    switch (decision) {
    case BuyDecision::FULL_BUY:
        reasoning += " - Full economy available";
        break;
    case BuyDecision::FORCE_BUY:
        reasoning += " - Force buy in important round";
        break;
    case BuyDecision::ECO_ROUND:
        reasoning += " - Save for next round";
        break;
    default:
        reasoning += " - Balanced buy strategy";
        break;
    }

    return reasoning;
}