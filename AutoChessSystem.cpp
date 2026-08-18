#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <cctype>
#include <windows.h>

using namespace std;

/*
 * Auto Chess Battle System
 * Target compiler: Visual Studio 2008 / C++03
 * The program intentionally avoids STL containers, std::string and algorithms.
 * All logical modules are kept in this single source file for final submission.
 */

/* Fixed capacities replace dynamic STL containers. */
const int BOARD_SIZE = 6;
const int MAX_OWNED_PIECES = 10;
const int MAX_DEPLOYED_PIECES = 6;
const int MAX_BATTLE_PIECES = 12;
const int SHOP_SLOT_COUNT = 5;
const int PIECE_TYPE_COUNT = 4;
const int MAX_NAME_LENGTH = 20;
const int MAX_FILE_NAME_LENGTH = 80;
const int MAX_ERROR_LENGTH = 128;
const int MAX_BATTLE_ROUNDS = 100;
const int INITIAL_BOARD_DELAY_MS = 2000;
const int BATTLE_BOARD_DELAY_MS = 1000;

enum PieceType
{
    TYPE_WARRIOR = 1,
    TYPE_ARCHER = 2,
    TYPE_MAGE = 3,
    TYPE_PALADIN = 4
};

enum BattleResult
{
    RESULT_HUMAN_WIN = 0,
    RESULT_AI_WIN = 1,
    RESULT_DRAW = 2
};

class ChessPiece;
class WarriorPiece;
class ArcherPiece;
class MagePiece;
class HealingTrait;
class PaladinPiece;
class Player;
class AIPlayer;
class Shop;
class Battlefield;
class FileManager;
class GameSystem;

/* Abstract root of the polymorphic piece hierarchy. */
class ChessPiece
{
protected:
    char name[MAX_NAME_LENGTH];
    int id;
    int type;
    int star;
    int cost;
    int maxHealth;
    int health;
    int attackPower;
    int defense;
    int attackRange;
    int speed;
    int row;
    int col;
    int team;
    int alive;
    int actionCounter;

public:
    ChessPiece(int newId, int newType, const char* newName, int newCost,
               int newHealth, int newAttack, int newDefense,
               int newRange, int newSpeed);
    virtual ~ChessPiece();

    virtual int selectTarget(ChessPiece* enemies[], int count) = 0;
    virtual void useSkill(ChessPiece* target, ChessPiece* enemies[],
                          int enemyCount, ChessPiece* allies[], int allyCount) = 0;
    virtual char getSymbol() const = 0;
    virtual int calculatePriority(ChessPiece* target) const = 0;
    virtual void resetSpecialState() = 0;
    virtual void takeDamage(int damage);

    void normalAttack(ChessPiece* target);
    void receiveHealing(int amount);
    void moveTo(int newRow, int newCol);
    void setTeam(int newTeam);
    void resetForBattle();
    void restoreAfterBattle();
    bool canAttack(const ChessPiece* target) const;
    bool isAlive() const;
    bool isDeployed() const;
    int distanceTo(const ChessPiece* target) const;

    int getId() const;
    int getType() const;
    int getStar() const;
    int getCost() const;
    int getMaxHealth() const;
    int getHealth() const;
    int getAttackPower() const;
    int getDefense() const;
    int getAttackRange() const;
    int getSpeed() const;
    int getRow() const;
    int getCol() const;
    int getTeam() const;
    const char* getName() const;
    void setPosition(int newRow, int newCol);

    bool operator==(const ChessPiece& other) const;
    ChessPiece& operator++();
    friend ostream& operator<<(ostream& out, const ChessPiece& piece);
};

class WarriorPiece : public ChessPiece
{
protected:
    int rage;
    int maxRage;
    int ragePerHit;
    int skillBonusDamage;
    int temporaryDefense;

public:
    WarriorPiece(int newId, const char* newName, int newCost,
                 int newHealth, int newAttack, int newDefense,
                 int newRange, int newSpeed);
    virtual ~WarriorPiece();

    virtual int selectTarget(ChessPiece* enemies[], int count);
    virtual void useSkill(ChessPiece* target, ChessPiece* enemies[],
                          int enemyCount, ChessPiece* allies[], int allyCount);
    virtual char getSymbol() const;
    virtual int calculatePriority(ChessPiece* target) const;
    virtual void resetSpecialState();
    virtual void takeDamage(int damage);
    void gainRage(int amount);
    bool isRageReady() const;
    int getRage() const;
};

class ArcherPiece : public ChessPiece
{
private:
    int shotCounter;
    int criticalInterval;
    int criticalPercent;
    int preferredDistance;
    int focusBonus;

public:
    ArcherPiece(int newId, const char* newName, int newCost,
                int newHealth, int newAttack, int newDefense,
                int newRange, int newSpeed);
    virtual ~ArcherPiece();

    virtual int selectTarget(ChessPiece* enemies[], int count);
    virtual void useSkill(ChessPiece* target, ChessPiece* enemies[],
                          int enemyCount, ChessPiece* allies[], int allyCount);
    virtual char getSymbol() const;
    virtual int calculatePriority(ChessPiece* target) const;
    virtual void resetSpecialState();
    bool isCriticalShot() const;
    int calculateCriticalDamage(const ChessPiece* target) const;
    int getShotCounter() const;
};

class MagePiece : public ChessPiece
{
private:
    int mana;
    int maxMana;
    int manaPerAttack;
    int spellPower;
    int splashDistance;

public:
    MagePiece(int newId, const char* newName, int newCost,
              int newHealth, int newAttack, int newDefense,
              int newRange, int newSpeed);
    virtual ~MagePiece();

    virtual int selectTarget(ChessPiece* enemies[], int count);
    virtual void useSkill(ChessPiece* target, ChessPiece* enemies[],
                          int enemyCount, ChessPiece* allies[], int allyCount);
    virtual char getSymbol() const;
    virtual int calculatePriority(ChessPiece* target) const;
    virtual void resetSpecialState();
    void gainMana(int amount);
    bool isManaReady() const;
    int getMana() const;
};

class HealingTrait
{
protected:
    int healPower;
    int healRange;
    int healCooldown;
    int remainingCooldown;
    int totalHealing;

public:
    HealingTrait();
    virtual ~HealingTrait();
    bool canHeal() const;
    void tickHealingCooldown();
    int findLowestHealthAlly(ChessPiece* allies[], int count,
                            const ChessPiece* healer) const;
    void healAlly(ChessPiece* ally);
    void resetHealingState();
    int getHealPower() const;
    int getTotalHealing() const;
};

class PaladinPiece : public WarriorPiece, public HealingTrait
{
private:
    int shieldPoints;
    int maximumShield;
    int healThresholdPercent;
    int holyCounter;
    int auraDefense;

public:
    PaladinPiece(int newId, const char* newName, int newCost,
                 int newHealth, int newAttack, int newDefense,
                 int newRange, int newSpeed);
    virtual ~PaladinPiece();

    virtual int selectTarget(ChessPiece* enemies[], int count);
    virtual void useSkill(ChessPiece* target, ChessPiece* enemies[],
                          int enemyCount, ChessPiece* allies[], int allyCount);
    virtual char getSymbol() const;
    virtual int calculatePriority(ChessPiece* target) const;
    virtual void resetSpecialState();
    virtual void takeDamage(int damage);
    void addShield(int amount);
    bool shouldHeal(ChessPiece* allies[], int count) const;
    int getShieldPoints() const;
};

class Player
{
protected:
    char playerName[MAX_NAME_LENGTH];
    int playerId;
    int playerHealth;
    int gold;
    ChessPiece* pieces[MAX_OWNED_PIECES];
    int pieceCount;
    int deployedCount;
    int ready;
    int totalWins;

    void removeAt(int index, bool destroyPiece);

public:
    Player(const char* newName = "Human", int newId = 0);
    virtual ~Player();
    void resetForNewGame(const char* newName, int newId);
    void clearPieces();
    bool buyPiece(ChessPiece* piece);
    bool addPieceForTest(ChessPiece* piece, int newRow, int newCol);
    bool sellPiece(int index);
    bool placePiece(int index, int newRow, int newCol,
                    int minimumRow, int maximumRow);
    bool checkAndMerge();
    bool isCellOccupied(int checkRow, int checkCol, int ignoredIndex) const;
    void clearAllPositions();
    void showRoster() const;
    void showStatus() const;
    void addGold(int amount);
    bool spendGold(int amount);
    void takePlayerDamage(int damage);
    void addWin();

    const char* getPlayerName() const;
    int getPlayerId() const;
    int getPlayerHealth() const;
    int getGold() const;
    int getPieceCount() const;
    int getDeployedCount() const;
    int getTotalWins() const;
    ChessPiece* getPiece(int index) const;
    bool hasDeployedPiece() const;
};

class AIPlayer : public Player
{
private:
    int reserveGold;
    int maximumRefreshes;
    int duplicateBonus;
    int valueWeight;
    int refreshesUsed;

public:
    AIPlayer(const char* newName = "Computer", int newId = 1);
    virtual ~AIPlayer();
    void resetAI();
    int evaluateOffer(const Shop& shop, int slot) const;
    int countSameType(int pieceType) const;
    void performShopping(Shop& shop);
    void arrangeFormation();
    int chooseBestOffer(const Shop& shop) const;
    bool shouldRefresh(const Shop& shop) const;
    int getReserveGold() const;
    int getRefreshesUsed() const;
};

class Shop
{
private:
    char pieceNames[PIECE_TYPE_COUNT][MAX_NAME_LENGTH];
    int pieceTypes[PIECE_TYPE_COUNT];
    int pieceCosts[PIECE_TYPE_COUNT];
    int pieceHealth[PIECE_TYPE_COUNT];
    int pieceAttack[PIECE_TYPE_COUNT];
    int pieceDefense[PIECE_TYPE_COUNT];
    int pieceRange[PIECE_TYPE_COUNT];
    int pieceSpeed[PIECE_TYPE_COUNT];
    int offers[SHOP_SLOT_COUNT];
    unsigned long randomState;
    int nextPieceId;

    int nextRandom(int maximum);

public:
    Shop();
    ~Shop();
    void initializeCatalog();
    void setRandomSeed(unsigned long seed);
    void refresh();
    void showOffers() const;
    bool isOfferAvailable(int slot) const;
    int getOfferType(int slot) const;
    int getOfferCost(int slot) const;
    const char* getOfferName(int slot) const;
    ChessPiece* createOfferedPiece(int slot);
    ChessPiece* createPieceByType(int pieceType);
    void removeOffer(int slot);
    bool hasAffordableOffer(int availableGold) const;
};

class Battlefield
{
private:
    ChessPiece* cells[BOARD_SIZE][BOARD_SIZE];
    ChessPiece* battlePieces[MAX_BATTLE_PIECES];
    int battlePieceCount;
    ChessPiece* originalPieces[MAX_BATTLE_PIECES];
    int originalRows[MAX_BATTLE_PIECES];
    int originalCols[MAX_BATTLE_PIECES];
    int originalCount;
    int currentRound;
    int maximumRounds;
    int lastWinner;
    int verboseMode;
    int lastSurvivorStarsTeam0;
    int lastSurvivorStarsTeam1;

    void clearBoard();
    void saveOriginalPosition(ChessPiece* piece);
    void restoreOriginalPositions();
    void collectBattlePieces(Player& first, Player& second);
    void sortBattlePiecesBySpeed();
    void buildTeamArray(int team, ChessPiece* output[], int& count) const;
    bool hasLivingTeam(int team) const;
    void removeDeadPiecesFromBoard();
    void moveTowardTarget(ChessPiece* piece, ChessPiece* target);

public:
    Battlefield();
    ~Battlefield();
    bool deployPlayers(Player& first, Player& second);
    int runBattle(Player& first, Player& second, bool verbose, int& roundsUsed);
    void printBoard() const;
    int calculateSurvivorStars(int team) const;
    int getCurrentRound() const;
    int getLastWinner() const;
    bool isInsideBoard(int checkRow, int checkCol) const;
    bool isCellEmpty(int checkRow, int checkCol) const;
};

class FileManager
{
private:
    char inputFileName[MAX_FILE_NAME_LENGTH];
    char outputFileName[MAX_FILE_NAME_LENGTH];
    char errorMessage[MAX_ERROR_LENGTH];
    ifstream inputFile;
    ofstream outputFile;
    int currentCase;
    int totalCases;
    int filesOpened;

public:
    FileManager(const char* inputName = "test_cases.txt",
                const char* outputName = "test_results.txt");
    ~FileManager();
    bool openFiles();
    void closeFiles();
    bool readNextCase(Player& human, AIPlayer& computer, Shop& shop,
                      int& expectedWinner, unsigned long& seed);
    void writeHeader();
    void writeResult(int caseNumber, int expectedWinner,
                     int actualWinner, int roundsUsed);
    bool hasMoreCases() const;
    int getCurrentCase() const;
    int getTotalCases() const;
    const char* getErrorMessage() const;
};

class GameSystem
{
private:
    Player human;
    AIPlayer computer;
    Shop shop;
    Battlefield battlefield;
    FileManager fileManager;
    int currentRound;
    int gameActive;
    int programRunning;
    int lastBattleResult;
    unsigned long gameSeed;
    int abandonedGames;

    int readInteger(int minimum, int maximum);
    void showMainMenu() const;
    void showPreparationMenu() const;
    void prepareHuman();
    void handlePurchase();
    void handleSell();
    void handlePlacement();
    void settleBattle(int result, int survivorStars);

public:
    GameSystem();
    ~GameSystem();
    void run();
    void startNewGame();
    void playRound();
    void showInstructions() const;
    void runFileTests();
    bool isGameOver() const;
    void printGameResult() const;
};

ChessPiece::ChessPiece(int newId, int newType, const char* newName, int newCost,
                       int newHealth, int newAttack, int newDefense,
                       int newRange, int newSpeed)
    : id(newId), type(newType), star(1), cost(newCost),
      maxHealth(newHealth), health(newHealth), attackPower(newAttack),
      defense(newDefense), attackRange(newRange), speed(newSpeed),
      row(-1), col(-1), team(0), alive(1), actionCounter(0)
{
    strncpy(name, newName, MAX_NAME_LENGTH - 1);
    name[MAX_NAME_LENGTH - 1] = '\0';
}

ChessPiece::~ChessPiece()
{
}

void ChessPiece::takeDamage(int damage)
{
    if (damage < 1)
        damage = 1;
    health -= damage;
    if (health <= 0)
    {
        health = 0;
        alive = 0;
    }
}

void ChessPiece::normalAttack(ChessPiece* target)
{
    int damage;
    if (target == 0 || !target->isAlive())
        return;
    damage = attackPower - target->getDefense();
    if (damage < 1)
        damage = 1;
    target->takeDamage(damage);
    actionCounter++;
}

void ChessPiece::receiveHealing(int amount)
{
    if (amount <= 0 || !isAlive()) return;
    health += amount;
    if (health > maxHealth) health = maxHealth;
}

void ChessPiece::moveTo(int newRow, int newCol)
{
    row = newRow;
    col = newCol;
}

void ChessPiece::setTeam(int newTeam)
{
    team = newTeam;
}

void ChessPiece::resetForBattle()
{
    health = maxHealth;
    alive = 1;
    actionCounter = 0;
    resetSpecialState();
}

void ChessPiece::restoreAfterBattle()
{
    health = maxHealth;
    alive = 1;
    actionCounter = 0;
    resetSpecialState();
}

bool ChessPiece::canAttack(const ChessPiece* target) const
{
    return target != 0 && target->isAlive() && distanceTo(target) <= attackRange;
}

bool ChessPiece::isAlive() const { return alive != 0; }
bool ChessPiece::isDeployed() const { return row >= 0 && col >= 0; }

int ChessPiece::distanceTo(const ChessPiece* target) const
{
    int rowDistance;
    int colDistance;
    if (target == 0)
        return 9999;
    rowDistance = row - target->row;
    colDistance = col - target->col;
    if (rowDistance < 0) rowDistance = -rowDistance;
    if (colDistance < 0) colDistance = -colDistance;
    return rowDistance + colDistance;
}

int ChessPiece::getId() const { return id; }
int ChessPiece::getType() const { return type; }
int ChessPiece::getStar() const { return star; }
int ChessPiece::getCost() const { return cost; }
int ChessPiece::getMaxHealth() const { return maxHealth; }
int ChessPiece::getHealth() const { return health; }
int ChessPiece::getAttackPower() const { return attackPower; }
int ChessPiece::getDefense() const { return defense; }
int ChessPiece::getAttackRange() const { return attackRange; }
int ChessPiece::getSpeed() const { return speed; }
int ChessPiece::getRow() const { return row; }
int ChessPiece::getCol() const { return col; }
int ChessPiece::getTeam() const { return team; }
const char* ChessPiece::getName() const { return name; }
void ChessPiece::setPosition(int newRow, int newCol) { row = newRow; col = newCol; }

bool ChessPiece::operator==(const ChessPiece& other) const
{
    /* Equality is defined for the three-copy merge rule. */
    return type == other.type && star == other.star && strcmp(name, other.name) == 0;
}

ChessPiece& ChessPiece::operator++()
{
    /* Prefix ++ performs a permanent star upgrade. */
    if (star < 3)
    {
        star++;
        maxHealth = maxHealth * 3 / 2;
        attackPower = attackPower * 3 / 2;
        defense += 2;
        health = maxHealth;
    }
    return *this;
}

ostream& operator<<(ostream& out, const ChessPiece& piece)
{
    out << "#" << piece.id << " " << piece.name
        << " [" << piece.getSymbol() << "]"
        << " Star:" << piece.star
        << " Cost:" << piece.cost
        << " HP:" << piece.health << "/" << piece.maxHealth
        << " ATK:" << piece.attackPower
        << " DEF:" << piece.defense
        << " RNG:" << piece.attackRange
        << " SPD:" << piece.speed;
    if (piece.isDeployed())
        out << " Pos:(" << piece.row << "," << piece.col << ")";
    else
        out << " Pos:Bench";
    return out;
}

WarriorPiece::WarriorPiece(int newId, const char* newName, int newCost,
                           int newHealth, int newAttack, int newDefense,
                           int newRange, int newSpeed)
    : ChessPiece(newId, TYPE_WARRIOR, newName, newCost, newHealth,
                 newAttack, newDefense, newRange, newSpeed),
      rage(0), maxRage(100), ragePerHit(35), skillBonusDamage(12),
      temporaryDefense(0)
{
}

WarriorPiece::~WarriorPiece()
{
}

int WarriorPiece::selectTarget(ChessPiece* enemies[], int count)
{
    int bestIndex = -1;
    int bestPriority = -999999;
    int i;
    for (i = 0; i < count; i++)
    {
        if (enemies[i] != 0 && enemies[i]->isAlive())
        {
            int priority = calculatePriority(enemies[i]);
            if (bestIndex < 0 || priority > bestPriority)
            {
                bestIndex = i;
                bestPriority = priority;
            }
        }
    }
    return bestIndex;
}

void WarriorPiece::useSkill(ChessPiece* target, ChessPiece* enemies[],
                            int enemyCount, ChessPiece* allies[], int allyCount)
{
    int i;
    int damage;
    (void)allies;
    (void)allyCount;
    if (target == 0)
        return;
    if (isRageReady())
    {
        /* A rage strike damages the target and adjacent enemy pieces. */
        damage = attackPower + skillBonusDamage - target->getDefense();
        if (damage < 1) damage = 1;
        target->takeDamage(damage);
        for (i = 0; i < enemyCount; i++)
        {
            if (enemies[i] != 0 && enemies[i] != target && enemies[i]->isAlive() &&
                enemies[i]->distanceTo(target) <= 1)
            {
                int splashDamage = attackPower / 2 - enemies[i]->getDefense();
                if (splashDamage < 1) splashDamage = 1;
                enemies[i]->takeDamage(splashDamage);
            }
        }
        rage = 0;
        temporaryDefense = 2;
        actionCounter++;
    }
    else
    {
        normalAttack(target);
        gainRage(ragePerHit);
    }
}

char WarriorPiece::getSymbol() const { return 'W'; }

int WarriorPiece::calculatePriority(ChessPiece* target) const
{
    if (target == 0) return -999999;
    return 10000 - distanceTo(target) * 100 - target->getHealth();
}

void WarriorPiece::resetSpecialState()
{
    rage = 0;
    temporaryDefense = 0;
}

void WarriorPiece::takeDamage(int damage)
{
    if (temporaryDefense > 0)
    {
        damage -= temporaryDefense;
        temporaryDefense = 0;
    }
    ChessPiece::takeDamage(damage);
    if (isAlive())
        gainRage(20);
}

void WarriorPiece::gainRage(int amount)
{
    rage += amount;
    if (rage > maxRage) rage = maxRage;
}

bool WarriorPiece::isRageReady() const { return rage >= maxRage; }
int WarriorPiece::getRage() const { return rage; }

ArcherPiece::ArcherPiece(int newId, const char* newName, int newCost,
                         int newHealth, int newAttack, int newDefense,
                         int newRange, int newSpeed)
    : ChessPiece(newId, TYPE_ARCHER, newName, newCost, newHealth,
                 newAttack, newDefense, newRange, newSpeed),
      shotCounter(0), criticalInterval(3), criticalPercent(200),
      preferredDistance(3), focusBonus(50)
{
}

ArcherPiece::~ArcherPiece()
{
}

int ArcherPiece::selectTarget(ChessPiece* enemies[], int count)
{
    int bestIndex = -1;
    int bestPriority = -999999;
    int i;
    for (i = 0; i < count; i++)
    {
        if (enemies[i] != 0 && enemies[i]->isAlive())
        {
            int priority = calculatePriority(enemies[i]);
            if (bestIndex < 0 || priority > bestPriority)
            {
                bestIndex = i;
                bestPriority = priority;
            }
        }
    }
    return bestIndex;
}

void ArcherPiece::useSkill(ChessPiece* target, ChessPiece* enemies[],
                           int enemyCount, ChessPiece* allies[], int allyCount)
{
    (void)enemies;
    (void)enemyCount;
    (void)allies;
    (void)allyCount;
    if (target == 0) return;
    shotCounter++;
    /* Every third shot is a deterministic critical hit. */
    if (isCriticalShot())
    {
        target->takeDamage(calculateCriticalDamage(target));
        actionCounter++;
    }
    else
    {
        normalAttack(target);
    }
}

char ArcherPiece::getSymbol() const { return 'A'; }

int ArcherPiece::calculatePriority(ChessPiece* target) const
{
    int distance;
    if (target == 0) return -999999;
    distance = distanceTo(target);
    return 12000 - target->getHealth() * 2 - distance * 40 +
           (distance == preferredDistance ? focusBonus : 0);
}

void ArcherPiece::resetSpecialState() { shotCounter = 0; }
bool ArcherPiece::isCriticalShot() const { return shotCounter % criticalInterval == 0; }

int ArcherPiece::calculateCriticalDamage(const ChessPiece* target) const
{
    int damage;
    if (target == 0) return 0;
    damage = attackPower * criticalPercent / 100 - target->getDefense();
    if (damage < 1) damage = 1;
    return damage;
}

int ArcherPiece::getShotCounter() const { return shotCounter; }

MagePiece::MagePiece(int newId, const char* newName, int newCost,
                     int newHealth, int newAttack, int newDefense,
                     int newRange, int newSpeed)
    : ChessPiece(newId, TYPE_MAGE, newName, newCost, newHealth,
                 newAttack, newDefense, newRange, newSpeed),
      mana(0), maxMana(100), manaPerAttack(40), spellPower(28),
      splashDistance(1)
{
}

MagePiece::~MagePiece()
{
}

int MagePiece::selectTarget(ChessPiece* enemies[], int count)
{
    int bestIndex = -1;
    int bestPriority = -999999;
    int i;
    for (i = 0; i < count; i++)
    {
        if (enemies[i] != 0 && enemies[i]->isAlive())
        {
            int priority = calculatePriority(enemies[i]);
            if (bestIndex < 0 || priority > bestPriority)
            {
                bestIndex = i;
                bestPriority = priority;
            }
        }
    }
    return bestIndex;
}

void MagePiece::useSkill(ChessPiece* target, ChessPiece* enemies[],
                         int enemyCount, ChessPiece* allies[], int allyCount)
{
    int i;
    (void)allies;
    (void)allyCount;
    if (target == 0) return;
    if (isManaReady())
    {
        /* Fireball affects the target cell and its adjacent cells. */
        for (i = 0; i < enemyCount; i++)
        {
            if (enemies[i] != 0 && enemies[i]->isAlive() &&
                enemies[i]->distanceTo(target) <= splashDistance)
            {
                int damage = attackPower + spellPower - enemies[i]->getDefense();
                if (damage < 1) damage = 1;
                enemies[i]->takeDamage(damage);
            }
        }
        mana = 0;
        actionCounter++;
    }
    else
    {
        normalAttack(target);
        gainMana(manaPerAttack);
    }
}

char MagePiece::getSymbol() const { return 'M'; }

int MagePiece::calculatePriority(ChessPiece* target) const
{
    if (target == 0) return -999999;
    return 11000 - target->getDefense() * 80 - distanceTo(target) * 50;
}

void MagePiece::resetSpecialState() { mana = 0; }

void MagePiece::gainMana(int amount)
{
    mana += amount;
    if (mana > maxMana) mana = maxMana;
}

bool MagePiece::isManaReady() const { return mana >= maxMana; }
int MagePiece::getMana() const { return mana; }

HealingTrait::HealingTrait()
    : healPower(24), healRange(4), healCooldown(3), remainingCooldown(0),
      totalHealing(0)
{
}

HealingTrait::~HealingTrait()
{
}

bool HealingTrait::canHeal() const { return remainingCooldown <= 0; }

void HealingTrait::tickHealingCooldown()
{
    if (remainingCooldown > 0) remainingCooldown--;
}

int HealingTrait::findLowestHealthAlly(ChessPiece* allies[], int count,
                                       const ChessPiece* healer) const
{
    int bestIndex = -1;
    int bestPercent = 101;
    int i;
    for (i = 0; i < count; i++)
    {
        if (allies[i] != 0 && allies[i]->isAlive() &&
            healer->distanceTo(allies[i]) <= healRange)
        {
            int percent = allies[i]->getHealth() * 100 / allies[i]->getMaxHealth();
            if (percent < bestPercent)
            {
                bestPercent = percent;
                bestIndex = i;
            }
        }
    }
    return bestIndex;
}

void HealingTrait::healAlly(ChessPiece* ally)
{
    if (ally == 0 || !ally->isAlive() || !canHeal()) return;
    ally->receiveHealing(healPower);
    totalHealing += healPower;
    remainingCooldown = healCooldown;
}

void HealingTrait::resetHealingState()
{
    remainingCooldown = 0;
    totalHealing = 0;
}

int HealingTrait::getHealPower() const { return healPower; }
int HealingTrait::getTotalHealing() const { return totalHealing; }

PaladinPiece::PaladinPiece(int newId, const char* newName, int newCost,
                           int newHealth, int newAttack, int newDefense,
                           int newRange, int newSpeed)
    : WarriorPiece(newId, newName, newCost, newHealth, newAttack,
                   newDefense, newRange, newSpeed),
      shieldPoints(0), maximumShield(30), healThresholdPercent(60),
      holyCounter(0), auraDefense(2)
{
    type = TYPE_PALADIN;
}

PaladinPiece::~PaladinPiece()
{
}

int PaladinPiece::selectTarget(ChessPiece* enemies[], int count)
{
    return WarriorPiece::selectTarget(enemies, count);
}

void PaladinPiece::useSkill(ChessPiece* target, ChessPiece* enemies[],
                            int enemyCount, ChessPiece* allies[], int allyCount)
{
    int allyIndex;
    tickHealingCooldown();
    holyCounter++;
    if (shouldHeal(allies, allyCount) && canHeal())
    {
        allyIndex = findLowestHealthAlly(allies, allyCount, this);
        if (allyIndex >= 0)
        {
            healAlly(allies[allyIndex]);
            addShield(maximumShield / 2);
            actionCounter++;
            return;
        }
    }
    WarriorPiece::useSkill(target, enemies, enemyCount, allies, allyCount);
    if (holyCounter % 3 == 0)
        addShield(auraDefense * 3);
}

char PaladinPiece::getSymbol() const { return 'P'; }

int PaladinPiece::calculatePriority(ChessPiece* target) const
{
    if (target == 0) return -999999;
    return 10500 - distanceTo(target) * 100 - target->getAttackPower();
}

void PaladinPiece::resetSpecialState()
{
    WarriorPiece::resetSpecialState();
    resetHealingState();
    shieldPoints = 0;
    holyCounter = 0;
}

void PaladinPiece::takeDamage(int damage)
{
    if (shieldPoints > 0)
    {
        if (shieldPoints >= damage)
        {
            shieldPoints -= damage;
            damage = 0;
        }
        else
        {
            damage -= shieldPoints;
            shieldPoints = 0;
        }
    }
    if (damage > 0)
        WarriorPiece::takeDamage(damage);
}

void PaladinPiece::addShield(int amount)
{
    shieldPoints += amount;
    if (shieldPoints > maximumShield) shieldPoints = maximumShield;
}

bool PaladinPiece::shouldHeal(ChessPiece* allies[], int count) const
{
    int i;
    for (i = 0; i < count; i++)
    {
        if (allies[i] != 0 && allies[i]->isAlive() &&
            allies[i]->getHealth() * 100 / allies[i]->getMaxHealth() < healThresholdPercent)
            return true;
    }
    return false;
}

int PaladinPiece::getShieldPoints() const { return shieldPoints; }

Player::Player(const char* newName, int newId)
    : playerId(newId), playerHealth(30), gold(10), pieceCount(0),
      deployedCount(0), ready(0), totalWins(0)
{
    int i;
    strncpy(playerName, newName, MAX_NAME_LENGTH - 1);
    playerName[MAX_NAME_LENGTH - 1] = '\0';
    for (i = 0; i < MAX_OWNED_PIECES; i++) pieces[i] = 0;
}

Player::~Player()
{
    /* Player is the sole owner of every pointer in pieces[]. */
    clearPieces();
}

void Player::resetForNewGame(const char* newName, int newId)
{
    clearPieces();
    strncpy(playerName, newName, MAX_NAME_LENGTH - 1);
    playerName[MAX_NAME_LENGTH - 1] = '\0';
    playerId = newId;
    playerHealth = 30;
    gold = 10;
    ready = 0;
    totalWins = 0;
}

void Player::clearPieces()
{
    int i;
    for (i = 0; i < pieceCount; i++)
    {
        delete pieces[i];
        pieces[i] = 0;
    }
    pieceCount = 0;
    deployedCount = 0;
}

bool Player::buyPiece(ChessPiece* piece)
{
    if (piece == 0) return false;
    if (pieceCount >= MAX_OWNED_PIECES || gold < piece->getCost())
    {
        delete piece;
        return false;
    }
    gold -= piece->getCost();
    pieces[pieceCount++] = piece;
    piece->setTeam(playerId);
    checkAndMerge();
    return true;
}

bool Player::addPieceForTest(ChessPiece* piece, int newRow, int newCol)
{
    if (piece == 0 || pieceCount >= MAX_OWNED_PIECES)
    {
        delete piece;
        return false;
    }
    if (isCellOccupied(newRow, newCol, -1))
    {
        delete piece;
        return false;
    }
    pieces[pieceCount++] = piece;
    piece->setTeam(playerId);
    piece->setPosition(newRow, newCol);
    deployedCount++;
    return true;
}

void Player::removeAt(int index, bool destroyPiece)
{
    int i;
    if (index < 0 || index >= pieceCount) return;
    if (pieces[index] != 0 && pieces[index]->isDeployed()) deployedCount--;
    if (destroyPiece) delete pieces[index];
    for (i = index; i < pieceCount - 1; i++) pieces[i] = pieces[i + 1];
    pieceCount--;
    pieces[pieceCount] = 0;
}

bool Player::sellPiece(int index)
{
    int refund;
    if (index < 0 || index >= pieceCount) return false;
    refund = pieces[index]->getCost() / 2;
    if (refund < 1) refund = 1;
    gold += refund;
    removeAt(index, true);
    return true;
}

bool Player::placePiece(int index, int newRow, int newCol,
                        int minimumRow, int maximumRow)
{
    bool wasDeployed;
    if (index < 0 || index >= pieceCount) return false;
    if (newRow < minimumRow || newRow > maximumRow || newCol < 0 || newCol >= BOARD_SIZE)
        return false;
    if (isCellOccupied(newRow, newCol, index)) return false;
    wasDeployed = pieces[index]->isDeployed();
    if (!wasDeployed && deployedCount >= MAX_DEPLOYED_PIECES) return false;
    pieces[index]->setPosition(newRow, newCol);
    if (!wasDeployed) deployedCount++;
    return true;
}

bool Player::checkAndMerge()
{
    /* Repeated scanning supports chain merges such as three two-star pieces. */
    bool mergedAny = false;
    bool found = true;
    while (found)
    {
        int i, j, k;
        found = false;
        for (i = 0; i < pieceCount && !found; i++)
        {
            if (pieces[i]->getStar() >= 3) continue;
            for (j = i + 1; j < pieceCount && !found; j++)
            {
                if (!(*pieces[i] == *pieces[j])) continue;
                for (k = j + 1; k < pieceCount; k++)
                {
                    if (*pieces[i] == *pieces[k])
                    {
                        ChessPiece* survivor = pieces[i];
                        ChessPiece* removeOne = pieces[j];
                        ChessPiece* removeTwo = pieces[k];
                        int index;
                        if (!survivor->isDeployed() && pieces[j]->isDeployed())
                        {
                            survivor = pieces[j];
                            removeOne = pieces[i];
                        }
                        if (!survivor->isDeployed() && pieces[k]->isDeployed())
                        {
                            survivor = pieces[k];
                            removeTwo = pieces[i];
                        }
                        ++(*survivor);
                        for (index = pieceCount - 1; index >= 0; index--)
                        {
                            if (pieces[index] == removeOne || pieces[index] == removeTwo)
                                removeAt(index, true);
                        }
                        found = true;
                        mergedAny = true;
                        break;
                    }
                }
            }
        }
    }
    return mergedAny;
}

bool Player::isCellOccupied(int checkRow, int checkCol, int ignoredIndex) const
{
    int i;
    for (i = 0; i < pieceCount; i++)
    {
        if (i != ignoredIndex && pieces[i] != 0 && pieces[i]->isDeployed() &&
            pieces[i]->getRow() == checkRow && pieces[i]->getCol() == checkCol)
            return true;
    }
    return false;
}

void Player::clearAllPositions()
{
    int i;
    for (i = 0; i < pieceCount; i++) pieces[i]->setPosition(-1, -1);
    deployedCount = 0;
}

void Player::showRoster() const
{
    int i;
    cout << "\n--- " << playerName << " Roster ---\n";
    if (pieceCount == 0) cout << "No pieces owned.\n";
    for (i = 0; i < pieceCount; i++) cout << (i + 1) << ". " << *pieces[i] << "\n";
}

void Player::showStatus() const
{
    cout << playerName << " HP:" << playerHealth << " Gold:" << gold
         << " Pieces:" << pieceCount << " Deployed:" << deployedCount << "\n";
}

void Player::addGold(int amount) { if (amount > 0) gold += amount; }

bool Player::spendGold(int amount)
{
    if (amount < 0 || gold < amount) return false;
    gold -= amount;
    return true;
}

void Player::takePlayerDamage(int damage)
{
    if (damage < 0) damage = 0;
    playerHealth -= damage;
    if (playerHealth < 0) playerHealth = 0;
}

void Player::addWin() { totalWins++; }
const char* Player::getPlayerName() const { return playerName; }
int Player::getPlayerId() const { return playerId; }
int Player::getPlayerHealth() const { return playerHealth; }
int Player::getGold() const { return gold; }
int Player::getPieceCount() const { return pieceCount; }
int Player::getDeployedCount() const { return deployedCount; }
int Player::getTotalWins() const { return totalWins; }

ChessPiece* Player::getPiece(int index) const
{
    if (index < 0 || index >= pieceCount) return 0;
    return pieces[index];
}

bool Player::hasDeployedPiece() const { return deployedCount > 0; }

AIPlayer::AIPlayer(const char* newName, int newId)
    : Player(newName, newId), reserveGold(2), maximumRefreshes(1),
      duplicateBonus(300), valueWeight(10),
      refreshesUsed(0)
{
}

AIPlayer::~AIPlayer()
{
}

void AIPlayer::resetAI()
{
    refreshesUsed = 0;
    ready = 0;
}

int AIPlayer::evaluateOffer(const Shop& shop, int slot) const
{
    int typeValue;
    int costValue;
    int score;
    if (!shop.isOfferAvailable(slot)) return -999999;
    typeValue = shop.getOfferType(slot);
    costValue = shop.getOfferCost(slot);
    if (costValue > gold) return -999999;
    score = valueWeight * (6 - costValue) + countSameType(typeValue) * duplicateBonus;
    if (typeValue == TYPE_WARRIOR || typeValue == TYPE_PALADIN) score += 25;
    if (pieceCount < 3) score += 100;
    return score;
}

int AIPlayer::countSameType(int pieceType) const
{
    int count = 0;
    int i;
    for (i = 0; i < pieceCount; i++)
        if (pieces[i]->getType() == pieceType) count++;
    return count;
}

int AIPlayer::chooseBestOffer(const Shop& shop) const
{
    int bestSlot = -1;
    int bestScore = -999999;
    int i;
    for (i = 0; i < SHOP_SLOT_COUNT; i++)
    {
        int score = evaluateOffer(shop, i);
        if (score > bestScore)
        {
            bestScore = score;
            bestSlot = i;
        }
    }
    return bestSlot;
}

bool AIPlayer::shouldRefresh(const Shop& shop) const
{
    return refreshesUsed < maximumRefreshes && gold >= reserveGold + 2 &&
           !shop.hasAffordableOffer(gold - reserveGold);
}

void AIPlayer::performShopping(Shop& shop)
{
    /* The AI keeps a small gold reserve and refreshes at most once per round. */
    int purchases = 0;
    refreshesUsed = 0;
    while (pieceCount < MAX_OWNED_PIECES && purchases < SHOP_SLOT_COUNT)
    {
        int slot = chooseBestOffer(shop);
        if (slot < 0 || !shop.isOfferAvailable(slot) ||
            shop.getOfferCost(slot) > gold - reserveGold)
        {
            if (shouldRefresh(shop))
            {
                spendGold(2);
                shop.refresh();
                refreshesUsed++;
                continue;
            }
            break;
        }
        if (buyPiece(shop.createOfferedPiece(slot)))
        {
            shop.removeOffer(slot);
            purchases++;
        }
        else
        {
            break;
        }
    }
}

void AIPlayer::arrangeFormation()
{
    int selected[MAX_OWNED_PIECES];
    int selectedCount = 0;
    int i, j;
    clearAllPositions();
    for (i = 0; i < pieceCount; i++) selected[i] = i;
    for (i = 0; i < pieceCount - 1; i++)
    {
        /* Melee units use row 1; ranged units use row 0. */
        int best = i;
        for (j = i + 1; j < pieceCount; j++)
        {
            int scoreBest = pieces[selected[best]]->getStar() * 1000 +
                            pieces[selected[best]]->getCost() * 100;
            int scoreCurrent = pieces[selected[j]]->getStar() * 1000 +
                               pieces[selected[j]]->getCost() * 100;
            if (scoreCurrent > scoreBest) best = j;
        }
        if (best != i)
        {
            int temporary = selected[i];
            selected[i] = selected[best];
            selected[best] = temporary;
        }
    }
    selectedCount = pieceCount;
    if (selectedCount > MAX_DEPLOYED_PIECES) selectedCount = MAX_DEPLOYED_PIECES;
    {
        int frontCol = 0;
        int backCol = 0;
        for (i = 0; i < selectedCount; i++)
        {
            ChessPiece* piece = pieces[selected[i]];
            if (piece->getType() == TYPE_WARRIOR || piece->getType() == TYPE_PALADIN)
                placePiece(selected[i], 1, frontCol++, 0, 1);
            else
                placePiece(selected[i], 0, backCol++, 0, 1);
        }
    }
    ready = 1;
}

int AIPlayer::getReserveGold() const { return reserveGold; }
int AIPlayer::getRefreshesUsed() const { return refreshesUsed; }

Shop::Shop() : randomState(1), nextPieceId(1)
{
    int i;
    initializeCatalog();
    for (i = 0; i < SHOP_SLOT_COUNT; i++) offers[i] = -1;
}

Shop::~Shop()
{
}

void Shop::initializeCatalog()
{
    const char* names[PIECE_TYPE_COUNT] = {"Warrior", "Archer", "Mage", "Paladin"};
    int types[PIECE_TYPE_COUNT] = {TYPE_WARRIOR, TYPE_ARCHER, TYPE_MAGE, TYPE_PALADIN};
    int costs[PIECE_TYPE_COUNT] = {2, 3, 4, 4};
    int healthValues[PIECE_TYPE_COUNT] = {120, 80, 75, 110};
    int attackValues[PIECE_TYPE_COUNT] = {24, 30, 20, 20};
    int defenseValues[PIECE_TYPE_COUNT] = {8, 3, 2, 9};
    int rangeValues[PIECE_TYPE_COUNT] = {1, 3, 2, 1};
    int speedValues[PIECE_TYPE_COUNT] = {3, 5, 4, 2};
    int i;
    for (i = 0; i < PIECE_TYPE_COUNT; i++)
    {
        strncpy(pieceNames[i], names[i], MAX_NAME_LENGTH - 1);
        pieceNames[i][MAX_NAME_LENGTH - 1] = '\0';
        pieceTypes[i] = types[i];
        pieceCosts[i] = costs[i];
        pieceHealth[i] = healthValues[i];
        pieceAttack[i] = attackValues[i];
        pieceDefense[i] = defenseValues[i];
        pieceRange[i] = rangeValues[i];
        pieceSpeed[i] = speedValues[i];
    }
}

void Shop::setRandomSeed(unsigned long seed)
{
    randomState = seed == 0 ? 1 : seed;
}

int Shop::nextRandom(int maximum)
{
    /* A small deterministic generator makes file tests reproducible. */
    if (maximum <= 0) return 0;
    randomState = randomState * 1103515245UL + 12345UL;
    return (int)((randomState / 65536UL) % (unsigned long)maximum);
}

void Shop::refresh()
{
    int i;
    for (i = 0; i < SHOP_SLOT_COUNT; i++) offers[i] = nextRandom(PIECE_TYPE_COUNT);
}

void Shop::showOffers() const
{
    int i;
    cout << "\n--- Shop ---\n";
    for (i = 0; i < SHOP_SLOT_COUNT; i++)
    {
        cout << (i + 1) << ". ";
        if (offers[i] < 0) cout << "Sold\n";
        else cout << pieceNames[offers[i]] << " Cost:" << pieceCosts[offers[i]] << "\n";
    }
}

bool Shop::isOfferAvailable(int slot) const
{
    return slot >= 0 && slot < SHOP_SLOT_COUNT && offers[slot] >= 0;
}

int Shop::getOfferType(int slot) const
{
    if (!isOfferAvailable(slot)) return -1;
    return pieceTypes[offers[slot]];
}

int Shop::getOfferCost(int slot) const
{
    if (!isOfferAvailable(slot)) return 999999;
    return pieceCosts[offers[slot]];
}

const char* Shop::getOfferName(int slot) const
{
    if (!isOfferAvailable(slot)) return "Unavailable";
    return pieceNames[offers[slot]];
}

ChessPiece* Shop::createOfferedPiece(int slot)
{
    if (!isOfferAvailable(slot)) return 0;
    return createPieceByType(pieceTypes[offers[slot]]);
}

ChessPiece* Shop::createPieceByType(int pieceType)
{
    int i;
    for (i = 0; i < PIECE_TYPE_COUNT; i++)
    {
        if (pieceTypes[i] == pieceType)
        {
            int newId = nextPieceId++;
            if (pieceType == TYPE_WARRIOR)
                return new WarriorPiece(newId, pieceNames[i], pieceCosts[i], pieceHealth[i],
                                        pieceAttack[i], pieceDefense[i], pieceRange[i], pieceSpeed[i]);
            if (pieceType == TYPE_ARCHER)
                return new ArcherPiece(newId, pieceNames[i], pieceCosts[i], pieceHealth[i],
                                       pieceAttack[i], pieceDefense[i], pieceRange[i], pieceSpeed[i]);
            if (pieceType == TYPE_MAGE)
                return new MagePiece(newId, pieceNames[i], pieceCosts[i], pieceHealth[i],
                                     pieceAttack[i], pieceDefense[i], pieceRange[i], pieceSpeed[i]);
            if (pieceType == TYPE_PALADIN)
                return new PaladinPiece(newId, pieceNames[i], pieceCosts[i], pieceHealth[i],
                                        pieceAttack[i], pieceDefense[i], pieceRange[i], pieceSpeed[i]);
        }
    }
    return 0;
}

void Shop::removeOffer(int slot)
{
    if (slot >= 0 && slot < SHOP_SLOT_COUNT) offers[slot] = -1;
}

bool Shop::hasAffordableOffer(int availableGold) const
{
    int i;
    for (i = 0; i < SHOP_SLOT_COUNT; i++)
        if (isOfferAvailable(i) && getOfferCost(i) <= availableGold) return true;
    return false;
}

Battlefield::Battlefield()
    : battlePieceCount(0), originalCount(0), currentRound(0),
      maximumRounds(MAX_BATTLE_ROUNDS), lastWinner(RESULT_DRAW), verboseMode(0),
      lastSurvivorStarsTeam0(0), lastSurvivorStarsTeam1(0)
{
    clearBoard();
}

Battlefield::~Battlefield()
{
}

void Battlefield::clearBoard()
{
    int r, c;
    for (r = 0; r < BOARD_SIZE; r++)
        for (c = 0; c < BOARD_SIZE; c++) cells[r][c] = 0;
}

bool Battlefield::isInsideBoard(int checkRow, int checkCol) const
{
    return checkRow >= 0 && checkRow < BOARD_SIZE && checkCol >= 0 && checkCol < BOARD_SIZE;
}

bool Battlefield::isCellEmpty(int checkRow, int checkCol) const
{
    return isInsideBoard(checkRow, checkCol) &&
           (cells[checkRow][checkCol] == 0 || !cells[checkRow][checkCol]->isAlive());
}

void Battlefield::saveOriginalPosition(ChessPiece* piece)
{
    if (piece == 0 || originalCount >= MAX_BATTLE_PIECES) return;
    originalPieces[originalCount] = piece;
    originalRows[originalCount] = piece->getRow();
    originalCols[originalCount] = piece->getCol();
    originalCount++;
}

void Battlefield::restoreOriginalPositions()
{
    /* Battle movement is temporary; preparation formations are permanent. */
    int i;
    for (i = 0; i < originalCount; i++)
    {
        originalPieces[i]->setPosition(originalRows[i], originalCols[i]);
        originalPieces[i]->restoreAfterBattle();
    }
}

void Battlefield::collectBattlePieces(Player& first, Player& second)
{
    int i;
    battlePieceCount = 0;
    originalCount = 0;
    for (i = 0; i < first.getPieceCount(); i++)
    {
        ChessPiece* piece = first.getPiece(i);
        if (piece != 0 && piece->isDeployed() && battlePieceCount < MAX_BATTLE_PIECES)
        {
            saveOriginalPosition(piece);
            piece->setTeam(0);
            piece->resetForBattle();
            battlePieces[battlePieceCount++] = piece;
        }
    }
    for (i = 0; i < second.getPieceCount(); i++)
    {
        ChessPiece* piece = second.getPiece(i);
        if (piece != 0 && piece->isDeployed() && battlePieceCount < MAX_BATTLE_PIECES)
        {
            saveOriginalPosition(piece);
            piece->setTeam(1);
            piece->resetForBattle();
            battlePieces[battlePieceCount++] = piece;
        }
    }
}

bool Battlefield::deployPlayers(Player& first, Player& second)
{
    int i;
    clearBoard();
    collectBattlePieces(first, second);
    if (!first.hasDeployedPiece() || !second.hasDeployedPiece())
    {
        restoreOriginalPositions();
        return false;
    }
    for (i = 0; i < battlePieceCount; i++)
    {
        int r = battlePieces[i]->getRow();
        int c = battlePieces[i]->getCol();
        if (!isInsideBoard(r, c) || cells[r][c] != 0)
        {
            restoreOriginalPositions();
            clearBoard();
            return false;
        }
        cells[r][c] = battlePieces[i];
    }
    return true;
}

void Battlefield::sortBattlePiecesBySpeed()
{
    /* Manual selection sort is used because STL algorithms are forbidden. */
    int i, j;
    for (i = 0; i < battlePieceCount - 1; i++)
    {
        int best = i;
        for (j = i + 1; j < battlePieceCount; j++)
        {
            if (battlePieces[j]->getSpeed() > battlePieces[best]->getSpeed() ||
                (battlePieces[j]->getSpeed() == battlePieces[best]->getSpeed() &&
                 battlePieces[j]->getId() < battlePieces[best]->getId()))
                best = j;
        }
        if (best != i)
        {
            ChessPiece* temporary = battlePieces[i];
            battlePieces[i] = battlePieces[best];
            battlePieces[best] = temporary;
        }
    }
}

void Battlefield::buildTeamArray(int teamNumber, ChessPiece* output[], int& count) const
{
    int i;
    count = 0;
    for (i = 0; i < battlePieceCount; i++)
    {
        if (battlePieces[i]->getTeam() == teamNumber && battlePieces[i]->isAlive())
            output[count++] = battlePieces[i];
    }
}

bool Battlefield::hasLivingTeam(int teamNumber) const
{
    int i;
    for (i = 0; i < battlePieceCount; i++)
        if (battlePieces[i]->getTeam() == teamNumber && battlePieces[i]->isAlive()) return true;
    return false;
}

void Battlefield::removeDeadPiecesFromBoard()
{
    int i;
    for (i = 0; i < battlePieceCount; i++)
    {
        ChessPiece* piece = battlePieces[i];
        int pieceRow = piece->getRow();
        int pieceCol = piece->getCol();
        if (!piece->isAlive() && isInsideBoard(pieceRow, pieceCol) &&
            cells[pieceRow][pieceCol] == piece)
        {
            cells[pieceRow][pieceCol] = 0;
        }
    }
}

void Battlefield::moveTowardTarget(ChessPiece* piece, ChessPiece* target)
{
    const int directions[4][2] = {{-1,0},{0,-1},{0,1},{1,0}};
    int bestRow = piece->getRow();
    int bestCol = piece->getCol();
    int bestDistance = piece->distanceTo(target);
    int i;
    for (i = 0; i < 4; i++)
    {
        /* Choose an empty orthogonal neighbor that reduces Manhattan distance. */
        int newRow = piece->getRow() + directions[i][0];
        int newCol = piece->getCol() + directions[i][1];
        if (isCellEmpty(newRow, newCol))
        {
            int rowDistance = newRow - target->getRow();
            int colDistance = newCol - target->getCol();
            int distance;
            if (rowDistance < 0) rowDistance = -rowDistance;
            if (colDistance < 0) colDistance = -colDistance;
            distance = rowDistance + colDistance;
            if (distance < bestDistance)
            {
                bestDistance = distance;
                bestRow = newRow;
                bestCol = newCol;
            }
        }
    }
    if (bestRow != piece->getRow() || bestCol != piece->getCol())
    {
        cells[piece->getRow()][piece->getCol()] = 0;
        piece->moveTo(bestRow, bestCol);
        cells[bestRow][bestCol] = piece;
    }
}

int Battlefield::runBattle(Player& first, Player& second, bool verbose, int& roundsUsed)
{
    int roundNumber;
    verboseMode = verbose ? 1 : 0;
    lastWinner = RESULT_DRAW;
    lastSurvivorStarsTeam0 = 0;
    lastSurvivorStarsTeam1 = 0;
    roundsUsed = 0;
    if (!deployPlayers(first, second)) return RESULT_DRAW;
    sortBattlePiecesBySpeed();
    if (verboseMode)
    {
        printBoard();
        Sleep(INITIAL_BOARD_DELAY_MS);
    }
    for (roundNumber = 1; roundNumber <= maximumRounds; roundNumber++)
    {
        int i;
        currentRound = roundNumber;
        for (i = 0; i < battlePieceCount; i++)
        {
            ChessPiece* acting = battlePieces[i];
            ChessPiece* enemies[MAX_DEPLOYED_PIECES];
            ChessPiece* allies[MAX_DEPLOYED_PIECES];
            int enemyCount, allyCount, targetIndex;
            if (!acting->isAlive()) continue;
            buildTeamArray(1 - acting->getTeam(), enemies, enemyCount);
            buildTeamArray(acting->getTeam(), allies, allyCount);
            if (enemyCount == 0) break;
            targetIndex = acting->selectTarget(enemies, enemyCount);
            if (targetIndex < 0 || targetIndex >= enemyCount) continue;
            if (acting->canAttack(enemies[targetIndex]))
                acting->useSkill(enemies[targetIndex], enemies, enemyCount, allies, allyCount);
            else
                moveTowardTarget(acting, enemies[targetIndex]);
            removeDeadPiecesFromBoard();
        }
        if (verboseMode)
        {
            cout << "\nBattle round " << roundNumber << "\n";
            printBoard();
            Sleep(BATTLE_BOARD_DELAY_MS);
        }
        if (!hasLivingTeam(0) || !hasLivingTeam(1)) break;
    }
    roundsUsed = currentRound;
    if (hasLivingTeam(0) && !hasLivingTeam(1)) lastWinner = RESULT_HUMAN_WIN;
    else if (!hasLivingTeam(0) && hasLivingTeam(1)) lastWinner = RESULT_AI_WIN;
    else lastWinner = RESULT_DRAW;
    {
        int i;
        for (i = 0; i < battlePieceCount; i++)
        {
            if (battlePieces[i]->isAlive())
            {
                if (battlePieces[i]->getTeam() == 0)
                    lastSurvivorStarsTeam0 += battlePieces[i]->getStar();
                else
                    lastSurvivorStarsTeam1 += battlePieces[i]->getStar();
            }
        }
    }
    restoreOriginalPositions();
    clearBoard();
    return lastWinner;
}

void Battlefield::printBoard() const
{
    int r, c;
    cout << "   0 1 2 3 4 5\n";
    for (r = 0; r < BOARD_SIZE; r++)
    {
        cout << r << "  ";
        for (c = 0; c < BOARD_SIZE; c++)
        {
            char symbol = '.';
            if (cells[r][c] != 0 && cells[r][c]->isAlive())
            {
                symbol = cells[r][c]->getSymbol();
                if (cells[r][c]->getTeam() == 1)
                    symbol = (char)tolower(symbol);
            }
            cout << symbol << ' ';
        }
        cout << "\n";
    }
}

int Battlefield::calculateSurvivorStars(int teamNumber) const
{
    if (teamNumber == 0) return lastSurvivorStarsTeam0;
    if (teamNumber == 1) return lastSurvivorStarsTeam1;
    return 0;
}

int Battlefield::getCurrentRound() const { return currentRound; }
int Battlefield::getLastWinner() const { return lastWinner; }

FileManager::FileManager(const char* inputName, const char* outputName)
    : currentCase(0), totalCases(0), filesOpened(0)
{
    strncpy(inputFileName, inputName, MAX_FILE_NAME_LENGTH - 1);
    inputFileName[MAX_FILE_NAME_LENGTH - 1] = '\0';
    strncpy(outputFileName, outputName, MAX_FILE_NAME_LENGTH - 1);
    outputFileName[MAX_FILE_NAME_LENGTH - 1] = '\0';
    errorMessage[0] = '\0';
}

FileManager::~FileManager()
{
    closeFiles();
}

bool FileManager::openFiles()
{
    closeFiles();
    inputFile.open(inputFileName);
    if (!inputFile)
    {
        strcpy(errorMessage, "Cannot open test input file.");
        return false;
    }
    outputFile.open(outputFileName);
    if (!outputFile)
    {
        strcpy(errorMessage, "Cannot open test output file.");
        inputFile.close();
        return false;
    }
    inputFile >> totalCases;
    if (!inputFile || totalCases < 1 || totalCases > 100)
    {
        strcpy(errorMessage, "Invalid test case count.");
        closeFiles();
        return false;
    }
    currentCase = 0;
    filesOpened = 1;
    errorMessage[0] = '\0';
    return true;
}

void FileManager::closeFiles()
{
    if (inputFile.is_open()) inputFile.close();
    if (outputFile.is_open()) outputFile.close();
    filesOpened = 0;
}

bool FileManager::readNextCase(Player& humanPlayer, AIPlayer& aiPlayer,
                               Shop& testShop, int& expectedWinner,
                               unsigned long& seed)
{
    /*
     * File format after the first line (case count):
     * expectedWinner humanCount aiCount seed
     * followed by human and AI records: type star row column
     */
    int humanCount, aiCount;
    int i;
    if (!filesOpened || currentCase >= totalCases) return false;
    inputFile >> expectedWinner >> humanCount >> aiCount >> seed;
    if (!inputFile || expectedWinner < 0 || expectedWinner > 2 ||
        humanCount < 1 || humanCount > MAX_DEPLOYED_PIECES ||
        aiCount < 1 || aiCount > MAX_DEPLOYED_PIECES)
    {
        strcpy(errorMessage, "Invalid test case header.");
        return false;
    }
    humanPlayer.resetForNewGame("TestHuman", 0);
    aiPlayer.resetForNewGame("TestAI", 1);
    for (i = 0; i < humanCount + aiCount; i++)
    {
        int pieceType, pieceStar, pieceRow, pieceCol;
        ChessPiece* piece;
        int upgradeCount;
        inputFile >> pieceType >> pieceStar >> pieceRow >> pieceCol;
        if (!inputFile || pieceType < TYPE_WARRIOR || pieceType > TYPE_PALADIN ||
            pieceStar < 1 || pieceStar > 3 || pieceRow < 0 || pieceRow >= BOARD_SIZE ||
            pieceCol < 0 || pieceCol >= BOARD_SIZE)
        {
            strcpy(errorMessage, "Invalid piece record in test case.");
            return false;
        }
        piece = testShop.createPieceByType(pieceType);
        for (upgradeCount = 1; upgradeCount < pieceStar; upgradeCount++) ++(*piece);
        if (i < humanCount)
        {
            if (!humanPlayer.addPieceForTest(piece, pieceRow, pieceCol))
            {
                strcpy(errorMessage, "Invalid human test formation.");
                return false;
            }
        }
        else
        {
            if (!aiPlayer.addPieceForTest(piece, pieceRow, pieceCol))
            {
                strcpy(errorMessage, "Invalid AI test formation.");
                return false;
            }
        }
    }
    currentCase++;
    return true;
}

void FileManager::writeHeader()
{
    if (!filesOpened) return;
    outputFile << "Auto Chess Battle Test Results\n";
    outputFile << "Case Expected Actual Rounds Status\n";
}

void FileManager::writeResult(int caseNumber, int expectedWinner,
                              int actualWinner, int roundsUsed)
{
    if (!filesOpened) return;
    outputFile << caseNumber << ' ' << expectedWinner << ' ' << actualWinner << ' '
               << roundsUsed << ' ' << (expectedWinner == actualWinner ? "PASS" : "FAIL") << "\n";
}

bool FileManager::hasMoreCases() const { return filesOpened && currentCase < totalCases; }
int FileManager::getCurrentCase() const { return currentCase; }
int FileManager::getTotalCases() const { return totalCases; }
const char* FileManager::getErrorMessage() const { return errorMessage; }

GameSystem::GameSystem()
    : human("Human", 0), computer("Computer", 1),
      fileManager("test_cases.txt", "test_results.txt"),
      currentRound(0), gameActive(0), programRunning(1),
      lastBattleResult(RESULT_DRAW), gameSeed((unsigned long)time(0)),
      abandonedGames(0)
{
    shop.setRandomSeed(gameSeed);
}

GameSystem::~GameSystem()
{
}

int GameSystem::readInteger(int minimum, int maximum)
{
    int value;
    while (true)
    {
        cin >> value;
        if (cin && value >= minimum && value <= maximum) return value;
        cin.clear();
        cin.ignore(10000, '\n');
        cout << "Invalid input. Enter " << minimum << "-" << maximum << ": ";
    }
}

void GameSystem::showMainMenu() const
{
    cout << "\n================================\n";
    cout << "      AUTO CHESS BATTLE\n";
    cout << "================================\n";
    cout << "1. Start Human vs AI\n";
    cout << "2. Instructions\n";
    cout << "3. Run file tests\n";
    cout << "0. Exit\n";
    cout << "Select: ";
}

void GameSystem::showPreparationMenu() const
{
    cout << "\n--- Preparation ---\n";
    cout << "1. Show status and roster\n";
    cout << "2. Show shop\n";
    cout << "3. Buy piece\n";
    cout << "4. Refresh shop (2 gold)\n";
    cout << "5. Sell piece\n";
    cout << "6. Place or move piece\n";
    cout << "7. Show piece details\n";
    cout << "8. Finish preparation\n";
    cout << "0. Abandon game\n";
    cout << "Select: ";
}

void GameSystem::run()
{
    while (programRunning)
    {
        int choice;
        showMainMenu();
        choice = readInteger(0, 3);
        if (choice == 1) startNewGame();
        else if (choice == 2) showInstructions();
        else if (choice == 3) runFileTests();
        else programRunning = 0;
    }
    cout << "Goodbye.\n";
}

void GameSystem::startNewGame()
{
    human.resetForNewGame("Human", 0);
    computer.resetForNewGame("Computer", 1);
    computer.resetAI();
    currentRound = 0;
    gameActive = 1;
    gameSeed = (unsigned long)time(0);
    shop.setRandomSeed(gameSeed);
    while (gameActive && !isGameOver()) playRound();
    if (isGameOver()) printGameResult();
}

void GameSystem::prepareHuman()
{
    int preparing = 1;
    shop.refresh();
    while (preparing && gameActive)
    {
        int choice;
        showPreparationMenu();
        choice = readInteger(0, 8);
        if (choice == 1)
        {
            human.showStatus();
            human.showRoster();
        }
        else if (choice == 2) shop.showOffers();
        else if (choice == 3) handlePurchase();
        else if (choice == 4)
        {
            if (human.spendGold(2))
            {
                shop.refresh();
                cout << "Shop refreshed.\n";
            }
            else cout << "Not enough gold.\n";
        }
        else if (choice == 5) handleSell();
        else if (choice == 6) handlePlacement();
        else if (choice == 7) human.showRoster();
        else if (choice == 8)
        {
            if (!human.hasDeployedPiece()) cout << "Deploy at least one piece first.\n";
            else preparing = 0;
        }
        else
        {
            cout << "Abandon this game? 1=Yes 0=No: ";
            if (readInteger(0, 1) == 1)
            {
                gameActive = 0;
                abandonedGames++;
            }
        }
    }
}

void GameSystem::handlePurchase()
{
    int slot;
    shop.showOffers();
    cout << "Choose slot (1-5, 0 cancel): ";
    slot = readInteger(0, SHOP_SLOT_COUNT);
    if (slot == 0) return;
    slot--;
    if (!shop.isOfferAvailable(slot))
    {
        cout << "That slot is unavailable.\n";
        return;
    }
    if (human.getPieceCount() >= MAX_OWNED_PIECES)
    {
        cout << "Roster is full.\n";
        return;
    }
    if (human.getGold() < shop.getOfferCost(slot))
    {
        cout << "Not enough gold.\n";
        return;
    }
    if (human.buyPiece(shop.createOfferedPiece(slot)))
    {
        cout << "Purchased " << shop.getOfferName(slot) << ".\n";
        shop.removeOffer(slot);
    }
}

void GameSystem::handleSell()
{
    int index;
    human.showRoster();
    if (human.getPieceCount() == 0) return;
    cout << "Choose piece to sell (1-" << human.getPieceCount() << ", 0 cancel): ";
    index = readInteger(0, human.getPieceCount());
    if (index > 0 && human.sellPiece(index - 1)) cout << "Piece sold.\n";
}

void GameSystem::handlePlacement()
{
    int index, row, col;
    human.showRoster();
    if (human.getPieceCount() == 0) return;
    cout << "Choose piece (1-" << human.getPieceCount() << ", 0 cancel): ";
    index = readInteger(0, human.getPieceCount());
    if (index == 0) return;
    cout << "Row (4-5): ";
    row = readInteger(4, 5);
    cout << "Column (0-5): ";
    col = readInteger(0, 5);
    if (human.placePiece(index - 1, row, col, 4, 5)) cout << "Piece placed.\n";
    else cout << "Cannot place the piece there.\n";
}

void GameSystem::playRound()
{
    int roundsUsed;
    int survivorStars;
    currentRound++;
    human.addGold(5);
    computer.addGold(5);
    cout << "\n========== GAME ROUND " << currentRound << " ==========\n";
    human.showStatus();
    prepareHuman();
    if (!gameActive) return;
    shop.refresh();
    computer.performShopping(shop);
    computer.arrangeFormation();
    cout << "\nAI preparation completed.\n";
    computer.showStatus();
    lastBattleResult = battlefield.runBattle(human, computer, true, roundsUsed);
    if (lastBattleResult == RESULT_HUMAN_WIN)
        survivorStars = battlefield.calculateSurvivorStars(0);
    else if (lastBattleResult == RESULT_AI_WIN)
        survivorStars = battlefield.calculateSurvivorStars(1);
    else survivorStars = 0;
    settleBattle(lastBattleResult, survivorStars);
}

void GameSystem::settleBattle(int result, int survivorStars)
{
    int damage;
    if (result == RESULT_HUMAN_WIN)
    {
        damage = 2 + survivorStars;
        computer.takePlayerDamage(damage);
        human.addWin();
        cout << "Human wins the round. AI takes " << damage << " damage.\n";
    }
    else if (result == RESULT_AI_WIN)
    {
        damage = 2 + survivorStars;
        human.takePlayerDamage(damage);
        computer.addWin();
        cout << "AI wins the round. Human takes " << damage << " damage.\n";
    }
    else
    {
        human.takePlayerDamage(2);
        computer.takePlayerDamage(2);
        cout << "Draw. Both players take 2 damage.\n";
    }
    human.showStatus();
    computer.showStatus();
}

void GameSystem::showInstructions() const
{
    cout << "\n--- Instructions ---\n";
    cout << "Buy pieces, combine three identical stars, and deploy up to six pieces.\n";
    cout << "Human pieces must be placed in rows 4-5. AI uses rows 0-1.\n";
    cout << "Battle is automatic. W=Warrior, A=Archer, M=Mage, P=Paladin.\n";
    cout << "Lowercase board symbols belong to the AI. Maximum star level is 3.\n";
}

void GameSystem::runFileTests()
{
    Player testHuman("TestHuman", 0);
    AIPlayer testAI("TestAI", 1);
    Shop testShop;
    Battlefield testField;
    int passed = 0;
    if (!fileManager.openFiles())
    {
        cout << "Test error: " << fileManager.getErrorMessage() << "\n";
        return;
    }
    fileManager.writeHeader();
    while (fileManager.hasMoreCases())
    {
        int expectedWinner;
        unsigned long seed;
        int roundsUsed;
        int actualWinner;
        if (!fileManager.readNextCase(testHuman, testAI, testShop, expectedWinner, seed))
        {
            cout << "Test error: " << fileManager.getErrorMessage() << "\n";
            break;
        }
        testShop.setRandomSeed(seed);
        actualWinner = testField.runBattle(testHuman, testAI, false, roundsUsed);
        fileManager.writeResult(fileManager.getCurrentCase(), expectedWinner,
                                actualWinner, roundsUsed);
        if (actualWinner == expectedWinner) passed++;
        cout << "Case " << fileManager.getCurrentCase() << ": expected "
             << expectedWinner << ", actual " << actualWinner << "\n";
    }
    cout << "Passed " << passed << "/" << fileManager.getTotalCases()
         << ". Results written to test_results.txt.\n";
    fileManager.closeFiles();
}

bool GameSystem::isGameOver() const
{
    return human.getPlayerHealth() <= 0 || computer.getPlayerHealth() <= 0;
}

void GameSystem::printGameResult() const
{
    cout << "\n========== GAME OVER ==========\n";
    if (human.getPlayerHealth() > computer.getPlayerHealth()) cout << "Human wins!\n";
    else if (computer.getPlayerHealth() > human.getPlayerHealth()) cout << "AI wins!\n";
    else cout << "The game is a draw.\n";
    cout << "Round wins - Human:" << human.getTotalWins()
         << " AI:" << computer.getTotalWins() << "\n";
}

int main()
{
    /* main() only creates the controller; all game flow belongs to GameSystem. */
    GameSystem game;
    game.run();
    return 0;
}
