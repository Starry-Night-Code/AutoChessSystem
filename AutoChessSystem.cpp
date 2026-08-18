#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdlib>
#include <ctime>   // 使用 time(0) 获取当前时间，并将其作为商店随机系统的种子。
#include <cctype>  // 使用字符处理函数 tolower() 把队伍 1 的棋子符号转换为小写。
#include <limits>  // 使用 numeric_limits<streamsize>::max() 清空输入缓冲区。
#ifndef NOMINMAX   // 在引用 <windows.h> 之前定义 NOMINMAX，
#define NOMINMAX   // 阻止 Windows 头文件定义 min 和 max 宏，
#endif             // 从而避免 numeric_limits<streamsize>::max() 与 <windows.h> 定义的 max 宏冲突。
#include <windows.h>  // 使用 Sleep() 为棋盘打印增加观看延迟。

using namespace std;

/*
 * 自走棋对战系统
 * 目标编译标准：C++03
 * 程序有意避免使用 STL 容器、std::string 和 STL 算法。
 * 为便于最终提交，所有逻辑模块都集中在这一个源文件中。
 */

/* 使用固定容量的数组代替动态 STL 容器。 */
const int BOARD_SIZE = 6;                // 棋盘的行数和列数。
const int MAX_OWNED_PIECES = 10;         // 每名玩家最多拥有的棋子数量。
const int MAX_DEPLOYED_PIECES = 6;       // 每名玩家最多上阵的棋子数量。
const int MAX_BATTLE_PIECES = 12;        // 一场战斗中双方棋子的总数上限。
const int SHOP_SLOT_COUNT = 5;           // 商店展示的商品槽位数量。
const int PIECE_TYPE_COUNT = 4;          // 可用棋子职业的数量。
const int MAX_NAME_LENGTH = 20;          // 玩家和棋子名称的字符数组长度。
const int MAX_FILE_NAME_LENGTH = 80;     // 测试文件名的字符数组长度。
const int MAX_ERROR_LENGTH = 128;        // 文件错误信息的字符数组长度。
const int MAX_BATTLE_ROUNDS = 100;       // 一场战斗最多执行的轮数。
const int INITIAL_BOARD_DELAY_MS = 2000; // 首次战斗行动前的等待时间（毫秒）。
const int BATTLE_BOARD_DELAY_MS = 1000;  // 每次输出战斗轮次棋盘后的等待时间（毫秒）。

enum PieceType // 棋子的职业类型。
{
    TYPE_WARRIOR = 1, // 战士。
    TYPE_ARCHER = 2,  // 射手。
    TYPE_MAGE = 3,    // 法师。
    TYPE_PALADIN = 4  // 圣骑士。
};

enum BattleResult // 一场战斗的胜负结果。
{
    RESULT_HUMAN_WIN = 0, // 玩家获胜。
    RESULT_AI_WIN = 1,    // AI 获胜。
    RESULT_DRAW = 2       // 双方平局。
};

class ChessPiece;   // 棋子抽象基类。
class WarriorPiece; // 战士棋子类。
class ArcherPiece;  // 射手棋子类。
class MagePiece;    // 法师棋子类。
class HealingTrait; // 治疗特性类。
class PaladinPiece; // 圣骑士棋子类。
class Player;       // 玩家类。
class AIPlayer;     // AI 玩家类。
class Shop;         // 商店类。
class Battlefield;  // 战场类。
class FileManager;  // 文件测试管理类。
class GameSystem;   // 游戏系统控制类。

/* 多态棋子继承体系的抽象根类。 */
class ChessPiece
{
protected:
    char name[MAX_NAME_LENGTH];
    int id;             // 棋子实例编号；速度相同时编号较小者先行动。
    int type;
    int star;           // 当前星级，取值范围为 1～3。
    int cost;
    int maxHealth;
    int health;
    int attackPower;
    int defense;
    int attackRange;    // 可攻击的最大曼哈顿距离。
    int speed;          // 决定每轮行动顺序，数值越大越先行动。
    int row;            // 当前行坐标；-1 表示棋子位于板凳。
    int col;            // 当前列坐标；-1 表示棋子位于板凳。
    int team;           // 所属队伍：0 为玩家，1 为 AI。
    int alive;          // 战斗存活标志，与 health 一同维护。

public:
    ChessPiece(int newId, int newType, const char* newName, int newCost,
               int newHealth, int newAttack, int newDefense,
               int newRange, int newSpeed);
    virtual ~ChessPiece();

    virtual int selectTarget(ChessPiece* enemies[], int count) = 0; // 返回目标在敌方数组中的下标。
    virtual void useSkill(ChessPiece* target, ChessPiece* enemies[],
                          int enemyCount, ChessPiece* allies[], int allyCount) = 0; // 执行本次攻击或职业技能。
    virtual char getSymbol() const = 0;
    virtual int calculatePriority(ChessPiece* target) const = 0; // 数值越大，目标越优先。
    virtual void resetSpecialState() = 0; // 重置怒气、法力等职业专属战斗状态。
    virtual void takeDamage(int damage);

    void normalAttack(ChessPiece* target);
    void receiveHealing(int amount);
    void moveTo(int newRow, int newCol);
    void setTeam(int newTeam);
    void resetForBattle();      // 开战前恢复生命值并初始化职业状态。
    void restoreAfterBattle();  // 战斗后清除临时伤势和职业状态。
    bool canAttack(const ChessPiece* target) const;
    bool isAlive() const;
    bool isDeployed() const;
    int distanceTo(const ChessPiece* target) const; // 计算与目标之间的曼哈顿距离。

    int getId() const;
    int getType() const;
    int getStar() const;
    int getCost() const;
    int getMaxHealth() const;
    int getHealth() const;
    int getAttackPower() const;
    int getDefense() const;
    int getSpeed() const;
    int getRow() const;
    int getCol() const;
    int getTeam() const;
    void setPosition(int newRow, int newCol);

    bool operator==(const ChessPiece& other) const; // 判断两枚棋子能否参与三合一。
    ChessPiece& operator++();                       // 将棋子永久提升一星。
    friend ostream& operator<<(ostream& out, const ChessPiece& piece); // 输出棋子详细信息。
};

/* 战士通过普通攻击和承受伤害积累怒气，怒气满后发动范围重击。 */
class WarriorPiece : public ChessPiece
{
protected:
    int rage;               // 当前怒气值。
    int maxRage;            // 发动怒气重击所需的怒气上限。
    int ragePerHit;         // 普通攻击后获得的怒气。
    int skillBonusDamage;   // 怒气重击附加的伤害。
    int temporaryDefense;   // 怒气重击后抵减下一次伤害的临时防御值。

private:
    void gainRage(int amount);   // 增加怒气并限制在上限以内。
    bool isRageReady() const;    // 判断是否可以发动怒气重击。

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
};

/* 射手保持远距离输出，并按固定攻击次数触发暴击。 */
class ArcherPiece : public ChessPiece
{
private:
    int shotCounter;       // 当前战斗中已经进行的射击次数。
    int criticalInterval;  // 每隔多少次射击触发一次暴击。
    int criticalPercent;   // 暴击伤害相对于攻击力的百分比。
    int preferredDistance; // 目标距离等于该值时获得选敌加分。
    int focusBonus;        // 处于偏好距离时增加的选敌优先级。
    bool isCriticalShot() const; // 判断本次射击是否为暴击。
    int calculateCriticalDamage(const ChessPiece* target) const; // 计算扣除防御后的暴击伤害。

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
};

/* 法师通过普通攻击积累法力，法力满后释放范围火球术。 */
class MagePiece : public ChessPiece
{
private:
    int mana;           // 当前法力值。
    int maxMana;        // 释放火球术所需的法力上限。
    int manaPerAttack;  // 普通攻击后获得的法力。
    int spellPower;     // 火球术附加的伤害。
    int splashDistance; // 火球术生效的曼哈顿距离。
    void gainMana(int amount); // 增加法力并限制在上限以内。
    bool isManaReady() const;  // 判断是否可以释放火球术。

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
};

/* 封装治疗能力，并通过多重继承提供给圣骑士。 */
class HealingTrait
{
protected:
    int healPower;         // 每次治疗恢复的生命值。
    int healRange;         // 可治疗的最大曼哈顿距离。
    int healCooldown;      // 每次治疗完成后设置的冷却值。
    int remainingCooldown; // 当前剩余冷却行动次数。
    bool canHeal() const;  // 判断治疗技能是否已经冷却完成。
    void tickHealingCooldown(); // 每次行动时减少剩余冷却。
    int findLowestHealthAlly(ChessPiece* allies[], int count,
                            const ChessPiece* healer) const; // 查找范围内生命值比例最低的友军。
    void healAlly(ChessPiece* ally); // 治疗目标并重新设置冷却。
    void resetHealingState();        // 清除跨战斗的冷却状态。

public:
    HealingTrait();
    virtual ~HealingTrait();
};

/* 圣骑士组合战士与治疗特性，并额外拥有护盾和神圣光环。 */
class PaladinPiece : public WarriorPiece, public HealingTrait
{
private:
    int shieldPoints;        // 当前护盾值，受伤时优先消耗。
    int maximumShield;       // 护盾值上限。
    int healThresholdPercent;// 友军生命值比例低于该值时才进行治疗。
    int holyCounter;         // 行动计数，用于周期性触发神圣光环。
    int auraDefense;         // 神圣光环换算护盾时使用的基础数值。
    void addShield(int amount); // 增加护盾并限制在上限以内。

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
};

/* 管理玩家资源、棋子所有权、阵容位置和三合一操作。 */
class Player
{
private:
    void showFormationBoard() const;
    void removeAt(int index, bool destroyPiece); // 删除指定下标并将后续元素向前移动。
    void clearPieces();                          // 释放玩家拥有的全部棋子对象。
    bool checkAndMerge();                        // 重复执行三合一，直到无法继续合成。
    bool isCellOccupied(int checkRow, int checkCol, int ignoredIndex) const; // 检查己方阵容格是否已被占用。

protected:
    char playerName[MAX_NAME_LENGTH];
    int playerId;                        // 玩家编号，同时作为棋子的默认队伍编号。
    int playerHealth;                    // 玩家生命值，降为 0 时游戏结束。
    int gold;
    ChessPiece* pieces[MAX_OWNED_PIECES]; // 玩家唯一拥有的棋子指针数组。
    int pieceCount;                       // pieces[] 中从下标 0 开始连续有效的元素数量。
    int deployedCount;                    // 当前坐标有效、已经上阵的棋子数量。
    int totalWins;                        // 当前游戏中累计获胜的回合数。

    void clearAllPositions(); // 将所有棋子放回板凳，供 AI 重新布阵。

public:
    Player(const char* newName = "Human", int newId = 0);
    virtual ~Player();
    void resetForNewGame(const char* newName, int newId);
    bool buyPiece(ChessPiece* piece); // 接管传入棋子；购买失败时负责释放非空对象。
    bool addPieceForTest(ChessPiece* piece, int newRow, int newCol); // 加入测试阵容并接管棋子所有权。
    bool sellPiece(int index);
    bool placePiece(int index, int newRow, int newCol,
                    int minimumRow, int maximumRow); // 在指定合法行范围内放置或移动棋子。
    bool returnPieceToBench(int index); // 将棋子坐标重置为 (-1,-1)。
    void showRoster(bool readOnly) const;
    void showStatus(bool readOnly) const;
    void addGold(int amount);
    bool spendGold(int amount);
    void takePlayerDamage(int damage);
    void addWin();

    int getPlayerHealth() const;
    int getGold() const;
    int getPieceCount() const;
    int getDeployedCount() const;
    int getTotalWins() const;
    ChessPiece* getPiece(int index) const;
    bool hasDeployedPiece() const;
};

/* 在 Player 的资源与阵容基础上实现确定性的自动购物和布阵策略。 */
class AIPlayer : public Player
{
private:
    int maximumRefreshes; // 每回合允许的最大商店刷新次数。
    int refreshesUsed;    // 当前回合已经使用的刷新次数。

    bool isFrontlineType(int pieceType) const;
    int countPiecesAtStar(int pieceType, int starLevel) const;
    int countRolePieces(bool frontline) const;
    int calculatePieceValue(const ChessPiece* piece) const; // 计算棋子的阵容价值，用于保留或出售决策。
    int calculatePurchaseChange(int pieceType) const;       // 估算购买某职业后阵容数量的净变化。
    int calculateResultingStar(int pieceType) const;        // 预测购买后该职业能够达到的星级。
    void selectCorePieces(int selected[], int& selectedCount) const; // 选出最多六枚用于上阵的核心棋子。
    bool isSelectedIndex(int pieceIndex, const int selected[], int selectedCount) const;
    bool isCoreType(int pieceType) const;
    bool hasUpgradableCorePiece(int pieceType) const;
    int findWeakestCoreValue(bool frontline) const; // 查找指定前后排角色中最弱的核心棋子价值。
    int classifyOffer(const Shop& shop, int slot) const; // 按阵容阶段和升级收益划分商品优先级。
    int chooseBestOffer(const Shop& shop) const;         // 在可购买商品中选择优先级最高的槽位。
    int findSellCandidate(int offerType) const;          // 为目标商品寻找可以出售的非核心棋子。
    bool hasRoomForFutureOffer() const;                  // 判断是否有空位或可出售棋子来接纳后续商品。
    bool shouldRefresh(const Shop& shop) const;          // 判断当前是否值得支付金币刷新商店。

public:
    AIPlayer(const char* newName = "Computer", int newId = 1);
    virtual ~AIPlayer();
    void performShopping(Shop& shop); // 执行购买、必要出售以及至多一次刷新。
    void arrangeFormation();          // 前排放第 1 行、后排放第 0 行并完成阵容布置。
};

/* 使用并行定长数组保存棋子目录，并负责生成商品和棋子对象。 */
class Shop
{
private:
    char pieceNames[PIECE_TYPE_COUNT][MAX_NAME_LENGTH]; // 以下目录数组使用同一下标描述同一职业。
    int pieceTypes[PIECE_TYPE_COUNT];
    int pieceCosts[PIECE_TYPE_COUNT];
    int pieceHealth[PIECE_TYPE_COUNT];
    int pieceAttack[PIECE_TYPE_COUNT];
    int pieceDefense[PIECE_TYPE_COUNT];
    int pieceRange[PIECE_TYPE_COUNT];
    int pieceSpeed[PIECE_TYPE_COUNT];
    int offers[SHOP_SLOT_COUNT]; // 保存目录下标；-1 表示该槽位已经售出或不可用。
    unsigned long randomState;   // 确定性伪随机数生成器的当前状态。
    int nextPieceId;             // 下一枚新建棋子使用的实例编号。

    int nextRandom(int maximum); // 生成 [0, maximum) 范围内的伪随机整数。
    void initializeCatalog();    // 初始化四种职业的名称与基础属性。

public:
    Shop();
    ~Shop();
    void setRandomSeed(unsigned long seed); // 设置商店随机序列的种子，0 会转换为 1。
    void refresh();                         // 为全部商品槽重新生成职业目录下标。
    void showOffers(const Player& viewer, bool readOnly) const;
    bool isOfferAvailable(int slot) const;
    int getOfferType(int slot) const;
    int getOfferCost(int slot) const;
    const char* getOfferName(int slot) const;
    ChessPiece* createOfferedPiece(int slot);      // 创建槽位对应的新棋子，所有权交给调用者。
    ChessPiece* createPieceByType(int pieceType);  // 按职业创建派生类对象并分配新编号。
    void removeOffer(int slot);                    // 将已购买槽位标记为不可用。
};

/* 管理战斗期间的临时棋盘、行动顺序、移动、胜负和阵型恢复。 */
class Battlefield
{
private:
    ChessPiece* cells[BOARD_SIZE][BOARD_SIZE];       // 战斗棋盘中的非拥有型棋子指针。
    ChessPiece* battlePieces[MAX_BATTLE_PIECES];    // 按行动顺序保存双方参战棋子，不负责释放。
    int battlePieceCount;                           // battlePieces[] 中的有效元素数量。
    ChessPiece* originalPieces[MAX_BATTLE_PIECES];  // 需要在战斗后恢复位置的棋子。
    int originalRows[MAX_BATTLE_PIECES];            // 各参战棋子的准备阶段行坐标。
    int originalCols[MAX_BATTLE_PIECES];            // 各参战棋子的准备阶段列坐标。
    int originalCount;                              // 已保存原始位置的棋子数量。
    int currentRound;                               // 当前战斗行动轮次。
    int maximumRounds;                              // 最多执行的轮数；达到上限仍未分胜负则判平局。
    int lastWinner;                                 // 最近一场战斗的 BattleResult 值。
    int verboseMode;                                // 是否打印棋盘并执行观看延迟。
    int lastSurvivorStarsTeam0;                     // 玩家存活棋子的星级总和。
    int lastSurvivorStarsTeam1;                     // AI 存活棋子的星级总和。

    void clearBoard();
    void saveOriginalPosition(ChessPiece* piece); // 保存准备阶段坐标，供战斗结束后恢复。
    void restoreOriginalPositions();              // 恢复坐标、生命值和职业临时状态。
    void collectBattlePieces(Player& first, Player& second); // 收集双方已上阵棋子并设置队伍。
    void sortBattlePiecesBySpeed(); // 按速度降序、编号升序确定固定行动顺序。
    void buildTeamArray(int team, ChessPiece* output[], int& count) const; // 收集指定队伍的存活棋子。
    bool hasLivingTeam(int team) const;
    void removeDeadPiecesFromBoard(); // 清除棋盘格中的死亡棋子指针。
    void moveTowardTarget(ChessPiece* piece, ChessPiece* target); // 向目标贪心移动一个正交格。
    bool deployPlayers(Player& first, Player& second); // 收集并校验双方阵容，然后填充战斗棋盘。
    void printBoard() const;
    bool isInsideBoard(int checkRow, int checkCol) const;
    bool isCellEmpty(int checkRow, int checkCol) const;

public:
    Battlefield();
    ~Battlefield();
    int runBattle(Player& first, Player& second, bool verbose, int& roundsUsed); // 自动执行战斗并返回 BattleResult。
    int calculateSurvivorStars(int team) const; // 返回最近一场战斗中指定队伍的存活星级总和。
};

/* 负责测试文件的打开、校验、逐例读取和结果写入。 */
class FileManager
{
private:
    char inputFileName[MAX_FILE_NAME_LENGTH];   // 测试输入文件路径。
    char outputFileName[MAX_FILE_NAME_LENGTH];  // 测试结果文件路径。
    char errorMessage[MAX_ERROR_LENGTH];        // 最近一次文件操作的错误说明。
    ifstream inputFile;
    ofstream outputFile;
    int currentCase; // 已成功读取的测试用例数量。
    int totalCases;  // 输入文件声明的测试用例总数。
    int filesOpened; // 输入和输出文件均可用时为 1，否则为 0。

public:
    FileManager(const char* inputName = "test_cases.txt",
                const char* outputName = "test_results.txt");
    ~FileManager();
    bool openFiles(); // 打开文件、读取用例总数，并覆盖原结果文件。
    void closeFiles();
    bool readNextCase(Player& human, AIPlayer& computer, Shop& shop,
                      int& expectedWinner); // 重置双方并按下一组记录创建测试阵容。
    void writeHeader();
    void writeResult(int caseNumber, int expectedWinner,
                     int actualWinner, int roundsUsed);
    bool hasMoreCases() const;
    int getCurrentCase() const;
    int getTotalCases() const;
    const char* getErrorMessage() const;
};

/* 组合各业务对象并驱动主菜单、准备阶段、战斗结算和文件测试。 */
class GameSystem
{
private:
    Player human;
    AIPlayer computer;
    Shop shop;
    Battlefield battlefield;
    FileManager fileManager;
    int currentRound;             // 当前游戏回合编号，不是战斗内部行动轮次。
    int gameActive;               // 当前玩家对战是否仍在进行。
    int programRunning;           // 主菜单循环是否继续运行。
    int lastBattleResult;         // 最近一回合的 BattleResult 值。
    unsigned long gameSeed;       // 正常游戏商店随机序列使用的时间种子。

    int readInteger(int minimum, int maximum); // 循环读取并校验指定闭区间内的整数。
    void showMainMenu() const;
    void showPreparationMenu() const;
    void prepareHuman(); // 循环处理玩家准备操作，直到完成准备或放弃游戏。
    void handlePurchase();
    void handleSell();
    void handlePlacement();
    void handleReturnToBench();
    void settleBattle(int result, int survivorStars); // 根据胜负和存活星级结算玩家伤害。
    void startNewGame(); // 重置双方、商店种子和游戏回合状态。
    void playRound();    // 依次执行经济增长、双方准备、战斗和结算。
    void showInstructions() const;
    void runFileTests(); // 使用独立玩家、商店和战场执行全部文件测试。
    bool isGameOver() const;
    void printGameResult() const;

public:
    GameSystem();
    ~GameSystem();
    void run();
};

ChessPiece::ChessPiece(int newId, int newType, const char* newName, int newCost,
                       int newHealth, int newAttack, int newDefense,
                       int newRange, int newSpeed)
    : id(newId), type(newType), star(1), cost(newCost),
      maxHealth(newHealth), health(newHealth), attackPower(newAttack),
      defense(newDefense), attackRange(newRange), speed(newSpeed),
      row(-1), col(-1), team(0), alive(1)
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
    resetSpecialState();
}

void ChessPiece::restoreAfterBattle()
{
    health = maxHealth;
    alive = 1;
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
int ChessPiece::getSpeed() const { return speed; }
int ChessPiece::getRow() const { return row; }
int ChessPiece::getCol() const { return col; }
int ChessPiece::getTeam() const { return team; }
void ChessPiece::setPosition(int newRow, int newCol) { row = newRow; col = newCol; }

bool ChessPiece::operator==(const ChessPiece& other) const
{
    /* 相等关系用于判断棋子是否符合三合一规则。 */
    return type == other.type && star == other.star && strcmp(name, other.name) == 0;
}

ChessPiece& ChessPiece::operator++()
{
    /* 前置 ++ 运算符用于永久提升棋子的星级。 */
    if (star < 3)
    {
        /* 每次升星按当前属性的 1.5 倍成长，整数除法会自动舍去小数部分。 */
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
    /* 遍历所有存活敌人，保留职业优先级最高者；-1 表示没有合法目标。 */
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
        /* 怒气重击会伤害目标及其相邻的敌方棋子。 */
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
        /* 技能释放后获得一次性减伤，效果在下一次受伤时消耗。 */
        temporaryDefense = 2;
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
    /* 距离权重高于生命值，因此战士首先接近最近的敌人，同距离时攻击残血目标。 */
    return 10000 - distanceTo(target) * 100 - target->getHealth();
}

void WarriorPiece::resetSpecialState()
{
    rage = 0;
    temporaryDefense = 0;
}

void WarriorPiece::takeDamage(int damage)
{
    /* 临时防御只抵挡一次伤害；即使减到 0，基类仍会保证至少造成 1 点伤害。 */
    if (temporaryDefense > 0)
    {
        damage -= temporaryDefense;
        temporaryDefense = 0;
    }
    ChessPiece::takeDamage(damage);
    /* 只有承伤后仍存活的战士才能获得怒气，避免死亡棋子继续积累状态。 */
    if (isAlive())
        gainRage(20);
}

void WarriorPiece::gainRage(int amount)
{
    rage += amount;
    if (rage > maxRage) rage = maxRage;
}

bool WarriorPiece::isRageReady() const { return rage >= maxRage; }

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
    /* 射手使用职业评分选择目标，评分相同时保留数组中先出现的敌人。 */
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
    /* 每第三次射击必定触发暴击。 */
    if (isCriticalShot())
    {
        target->takeDamage(calculateCriticalDamage(target));
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
    /* 低生命值目标优先；恰好位于偏好射程时再获得额外加分。 */
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
        /* 火球术会影响目标格及其相邻格。 */
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
    /* 防御越低、距离越近，评分越高，以提高火球术实际造成的伤害。 */
    return 11000 - target->getDefense() * 80 - distanceTo(target) * 50;
}

void MagePiece::resetSpecialState() { mana = 0; }

void MagePiece::gainMana(int amount)
{
    mana += amount;
    if (mana > maxMana) mana = maxMana;
}

bool MagePiece::isManaReady() const { return mana >= maxMana; }

HealingTrait::HealingTrait()
    : healPower(24), healRange(4), healCooldown(3), remainingCooldown(0)
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
    /* 按生命值百分比而非绝对生命值比较，使高血量职业也能被公平地选中。 */
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
    remainingCooldown = healCooldown;
}

void HealingTrait::resetHealingState()
{
    remainingCooldown = 0;
}

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
    /* 每次轮到圣骑士行动，治疗冷却和光环计数都推进一次。 */
    tickHealingCooldown();
    holyCounter++;
    if (canHeal())
    {
        allyIndex = findLowestHealthAlly(allies, allyCount, this);
        if (allyIndex >= 0 &&
            allies[allyIndex]->getHealth() * 100 /
            allies[allyIndex]->getMaxHealth() < healThresholdPercent)
        {
            /* 治疗优先于攻击；成功治疗后本次行动立即结束，并为自身补充护盾。 */
            healAlly(allies[allyIndex]);
            addShield(maximumShield / 2);
            return;
        }
    }
    WarriorPiece::useSkill(target, enemies, enemyCount, allies, allyCount);
    /* 没有治疗时才沿用战士的攻击逻辑，每三次行动额外获得一次光环护盾。 */
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
    /* 护盾先吸收伤害，只有溢出的部分才交给战士的受伤逻辑处理。 */
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

Player::Player(const char* newName, int newId)
    : playerId(newId), playerHealth(30), gold(10), pieceCount(0),
      deployedCount(0), totalWins(0)
{
    int i;
    strncpy(playerName, newName, MAX_NAME_LENGTH - 1);
    playerName[MAX_NAME_LENGTH - 1] = '\0';
    for (i = 0; i < MAX_OWNED_PIECES; i++) pieces[i] = 0;
}

Player::~Player()
{
    /* Player 是 pieces[] 中所有棋子指针所指对象的唯一所有者。 */
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
    /* 本函数接管传入指针的所有权，因此购买失败时也必须在这里释放对象。 */
    if (piece == 0) return false;
    if (pieceCount >= MAX_OWNED_PIECES || gold < piece->getCost())
    {
        delete piece;
        return false;
    }
    gold -= piece->getCost();
    pieces[pieceCount++] = piece;
    piece->setTeam(playerId);
    /* 新棋子入队后立即检查三合一，合成还可能继续触发更高星级的合成。 */
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
    /* 数组始终保持前 pieceCount 个位置连续有效，便于其余逻辑直接遍历。 */
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
    /* 移动已上阵棋子不增加 deployedCount，只有从板凳首次上阵时才检查人数上限。 */
    if (!wasDeployed && deployedCount >= MAX_DEPLOYED_PIECES) return false;
    pieces[index]->setPosition(newRow, newCol);
    if (!wasDeployed) deployedCount++;
    return true;
}

bool Player::returnPieceToBench(int index)
{
    if (index < 0 || index >= pieceCount || !pieces[index]->isDeployed()) return false;
    pieces[index]->setPosition(-1, -1);
    deployedCount--;
    return true;
}

bool Player::checkAndMerge()
{
    /* 通过重复扫描支持连续合成，例如九枚一星棋子可连续合成为一枚三星棋子。 */
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
                        /* 优先保留已上阵的对象，避免合成后阵型位置无故丢失。 */
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
                        /* 从后向前删除可避免数组前移后使尚未处理的下标失效。 */
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

void Player::showFormationBoard() const
{
    char board[BOARD_SIZE][BOARD_SIZE];
    int rowIndex;
    int colIndex;
    int pieceIndex;
    for (rowIndex = 0; rowIndex < BOARD_SIZE; rowIndex++)
        for (colIndex = 0; colIndex < BOARD_SIZE; colIndex++)
            board[rowIndex][colIndex] = '.';

    for (pieceIndex = 0; pieceIndex < pieceCount; pieceIndex++)
    {
        ChessPiece* piece = pieces[pieceIndex];
        if (piece != 0 && piece->isDeployed())
            board[piece->getRow()][piece->getCol()] = piece->getSymbol();
    }

    cout << "\n   0 1 2 3 4 5\n";
    for (rowIndex = 0; rowIndex < BOARD_SIZE; rowIndex++)
    {
        cout << rowIndex << "  ";
        for (colIndex = 0; colIndex < BOARD_SIZE; colIndex++)
            cout << board[rowIndex][colIndex] << ' ';
        cout << "\n";
    }
}

void Player::showRoster(bool readOnly) const
{
    int i;
    if (readOnly)
        cout << "\n*** " << playerName << " Roster ***\n";
    else
        cout << "\n--- " << playerName << " Roster ---\n";
    if (pieceCount == 0) cout << "No pieces owned.\n";
    for (i = 0; i < pieceCount; i++) cout << (i + 1) << ". " << *pieces[i] << "\n";
    showFormationBoard();
}

void Player::showStatus(bool readOnly) const
{
    if (readOnly)
        cout << "\n*** " << playerName << " Status ***\n";
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
    : Player(newName, newId), maximumRefreshes(1), refreshesUsed(0)
{
}

AIPlayer::~AIPlayer()
{
}

bool AIPlayer::isFrontlineType(int pieceType) const
{
    return pieceType == TYPE_WARRIOR || pieceType == TYPE_PALADIN;
}

int AIPlayer::countPiecesAtStar(int pieceType, int starLevel) const
{
    int count = 0;
    int i;
    for (i = 0; i < pieceCount; i++)
        if (pieces[i]->getType() == pieceType && pieces[i]->getStar() == starLevel) count++;
    return count;
}

int AIPlayer::countRolePieces(bool frontline) const
{
    int count = 0;
    int i;
    for (i = 0; i < pieceCount; i++)
        if (isFrontlineType(pieces[i]->getType()) == frontline) count++;
    return count;
}

int AIPlayer::calculatePieceValue(const ChessPiece* piece) const
{
    if (piece == 0) return -1;
    return piece->getStar() * 1000 + piece->getCost() * 100;
}

int AIPlayer::calculatePurchaseChange(int pieceType) const
{
    /* 先假设买入使棋子数 +1；若触发三合一，每次合成会再净减少 2 枚。 */
    int change = 1;
    int starLevel = 1;
    while (starLevel < 3 && countPiecesAtStar(pieceType, starLevel) >= 2)
    {
        change -= 2;
        starLevel++;
    }
    return change;
}

int AIPlayer::calculateResultingStar(int pieceType) const
{
    /* 模拟购买一枚一星棋子后可能发生的连续合成，只预测星级而不修改阵容。 */
    int starLevel = 1;
    while (starLevel < 3 && countPiecesAtStar(pieceType, starLevel) >= 2)
        starLevel++;
    return starLevel;
}

void AIPlayer::selectCorePieces(int selected[], int& selectedCount) const
{
    /*
     * 核心阵容分两阶段选择：先尽量各取两名前排和后排，保证基本阵型；
     * 剩余名额再按“星级优先、费用次优先”的价值评分从高到低补齐。
     */
    bool used[MAX_OWNED_PIECES];
    int desiredCount = pieceCount;
    int rolePass;
    int i;
    if (desiredCount > MAX_DEPLOYED_PIECES) desiredCount = MAX_DEPLOYED_PIECES;
    selectedCount = 0;
    for (i = 0; i < MAX_OWNED_PIECES; i++) used[i] = false;

    for (rolePass = 0; rolePass < 2; rolePass++)
    {
        /* 第一次选择前排，第二次选择后排，每类最多先占两个保底名额。 */
        bool frontline = rolePass == 0;
        int roleSelections;
        for (roleSelections = 0; roleSelections < 2 && selectedCount < desiredCount;
             roleSelections++)
        {
            int bestIndex = -1;
            int bestValue = -1;
            for (i = 0; i < pieceCount; i++)
            {
                int currentValue;
                if (used[i] || isFrontlineType(pieces[i]->getType()) != frontline) continue;
                currentValue = calculatePieceValue(pieces[i]);
                if (bestIndex < 0 || currentValue > bestValue)
                {
                    bestIndex = i;
                    bestValue = currentValue;
                }
            }
            if (bestIndex < 0) break;
            used[bestIndex] = true;
            selected[selectedCount++] = bestIndex;
        }
    }

    while (selectedCount < desiredCount)
    {
        /* 不再限制职业位置，从所有未选择棋子中补入价值最高者。 */
        int bestIndex = -1;
        int bestValue = -1;
        for (i = 0; i < pieceCount; i++)
        {
            int currentValue;
            if (used[i]) continue;
            currentValue = calculatePieceValue(pieces[i]);
            if (bestIndex < 0 || currentValue > bestValue)
            {
                bestIndex = i;
                bestValue = currentValue;
            }
        }
        if (bestIndex < 0) break;
        used[bestIndex] = true;
        selected[selectedCount++] = bestIndex;
    }
}

bool AIPlayer::isSelectedIndex(int pieceIndex, const int selected[],
                               int selectedCount) const
{
    int i;
    for (i = 0; i < selectedCount; i++)
        if (selected[i] == pieceIndex) return true;
    return false;
}

bool AIPlayer::isCoreType(int pieceType) const
{
    int selected[MAX_OWNED_PIECES];
    int selectedCount;
    int i;
    selectCorePieces(selected, selectedCount);
    for (i = 0; i < selectedCount; i++)
        if (pieces[selected[i]]->getType() == pieceType) return true;
    return false;
}

bool AIPlayer::hasUpgradableCorePiece(int pieceType) const
{
    int selected[MAX_OWNED_PIECES];
    int selectedCount;
    int i;
    selectCorePieces(selected, selectedCount);
    for (i = 0; i < selectedCount; i++)
    {
        ChessPiece* piece = pieces[selected[i]];
        if (piece->getType() == pieceType && piece->getStar() < 3) return true;
    }
    return false;
}

int AIPlayer::findWeakestCoreValue(bool frontline) const
{
    int selected[MAX_OWNED_PIECES];
    int selectedCount;
    int weakestValue = -1;
    int i;
    selectCorePieces(selected, selectedCount);
    for (i = 0; i < selectedCount; i++)
    {
        ChessPiece* piece = pieces[selected[i]];
        int currentValue;
        if (isFrontlineType(piece->getType()) != frontline) continue;
        currentValue = calculatePieceValue(piece);
        if (weakestValue < 0 || currentValue < weakestValue) weakestValue = currentValue;
    }
    return weakestValue;
}

int AIPlayer::findSellCandidate(int offerType) const
{
    /*
     * 只出售非核心、孤立的一星棋子：既不拆掉即将合成的对子，
     * 也不让对应前排或后排角色的总数跌破两个。
     */
    int selected[MAX_OWNED_PIECES];
    int selectedCount;
    int candidate = -1;
    int candidateValue = -1;
    int i;
    selectCorePieces(selected, selectedCount);
    for (i = 0; i < pieceCount; i++)
    {
        ChessPiece* piece = pieces[i];
        bool frontline;
        int currentValue;
        if (isSelectedIndex(i, selected, selectedCount) || piece->getStar() != 1 ||
            piece->getType() == offerType ||
            countPiecesAtStar(piece->getType(), 1) != 1)
            continue;
        frontline = isFrontlineType(piece->getType());
        if (countRolePieces(frontline) <= 2) continue;
        currentValue = calculatePieceValue(piece);
        if (candidate < 0 || currentValue < candidateValue ||
            (currentValue == candidateValue && i > candidate))
        {
            candidate = i;
            candidateValue = currentValue;
        }
    }
    return candidate;
}

bool AIPlayer::hasRoomForFutureOffer() const
{
    int pieceType;
    if (pieceCount < MAX_OWNED_PIECES) return true;
    for (pieceType = TYPE_WARRIOR; pieceType <= TYPE_PALADIN; pieceType++)
        if (findSellCandidate(pieceType) >= 0) return true;
    return false;
}

int AIPlayer::classifyOffer(const Shop& shop, int slot) const
{
    /* 返回值是购买优先级类别，数值越大越值得购买，0 表示放弃。 */
    int pieceType;
    int pieceCost;
    int purchaseChange;
    bool frontline;
    bool formationStage;
    if (!shop.isOfferAvailable(slot)) return 0;
    pieceType = shop.getOfferType(slot);
    pieceCost = shop.getOfferCost(slot);
    if (pieceCost > gold) return 0;
    if (pieceCount >= MAX_OWNED_PIECES && findSellCandidate(pieceType) < 0) return 0;

    frontline = isFrontlineType(pieceType);
    purchaseChange = calculatePurchaseChange(pieceType);
    formationStage = pieceCount < MAX_DEPLOYED_PIECES ||
                     countRolePieces(true) < 2 || countRolePieces(false) < 2;
    if (formationStage)
    {
        /* 成型前优先补齐缺少的位置，其次扩充人数，最后才考虑会立即合成的商品。 */
        if (purchaseChange > 0 && countRolePieces(frontline) < 2) return 40;
        if (purchaseChange > 0) return 30;
        return 20;
    }

    /* 成型后以升星为最高目标，其次考虑对子、替换弱核心和长期升级潜力。 */
    if (countPiecesAtStar(pieceType, 1) >= 2) return 50;
    if (countPiecesAtStar(pieceType, 1) == 1 && isCoreType(pieceType)) return 40;
    {
        int weakestValue = findWeakestCoreValue(frontline);
        int offerValue = 1000 + pieceCost * 100;
        if (weakestValue >= 0 && offerValue > weakestValue) return 30;
    }
    if (isCoreType(pieceType) && hasUpgradableCorePiece(pieceType)) return 20;
    return 0;
}

int AIPlayer::chooseBestOffer(const Shop& shop) const
{
    /*
     * 先比较 classifyOffer() 给出的策略类别；类别相同时，成型前选便宜的，
     * 成型后选合成星级更高的，再以价格更高（基础价值更高）作为最终决胜条件。
     */
    bool formationStage = pieceCount < MAX_DEPLOYED_PIECES ||
                          countRolePieces(true) < 2 || countRolePieces(false) < 2;
    int bestSlot = -1;
    int bestClass = 0;
    int bestResultingStar = -1;
    int bestCost = -1;
    int i;
    for (i = 0; i < SHOP_SLOT_COUNT; i++)
    {
        int currentClass = classifyOffer(shop, i);
        int currentResultingStar;
        int currentCost;
        bool better = false;
        if (currentClass <= 0) continue;
        currentResultingStar = calculateResultingStar(shop.getOfferType(i));
        currentCost = shop.getOfferCost(i);
        if (bestSlot < 0 || currentClass > bestClass)
            better = true;
        else if (currentClass == bestClass)
        {
            if (formationStage)
            {
                if (currentCost < bestCost) better = true;
            }
            else if (currentResultingStar > bestResultingStar)
                better = true;
            else if (currentResultingStar == bestResultingStar && currentCost > bestCost)
                better = true;
        }
        if (better)
        {
            bestSlot = i;
            bestClass = currentClass;
            bestResultingStar = currentResultingStar;
            bestCost = currentCost;
        }
    }
    return bestSlot;
}

bool AIPlayer::shouldRefresh(const Shop& shop) const
{
    /* 至少保留 2 金币购买刷新后的商品，因此余额必须达到 4。 */
    return refreshesUsed < maximumRefreshes && gold >= 4 &&
           hasRoomForFutureOffer() && chooseBestOffer(shop) < 0;
}

void AIPlayer::performShopping(Shop& shop)
{
    /* 一轮内反复执行“选商品—必要时腾位置—购买”，无合适商品时最多刷新一次。 */
    int purchases = 0;
    refreshesUsed = 0;
    while (purchases < SHOP_SLOT_COUNT)
    {
        int slot = chooseBestOffer(shop);
        if (slot < 0)
        {
            if (shouldRefresh(shop))
            {
                /* 刷新后重新从循环顶部评估全部新商品。 */
                spendGold(2);
                shop.refresh();
                refreshesUsed++;
                continue;
            }
            break;
        }
        if (pieceCount >= MAX_OWNED_PIECES)
        {
            /* 阵容已满时，只有能找到符合保护规则的出售对象才继续购买。 */
            int sellIndex = findSellCandidate(shop.getOfferType(slot));
            if (sellIndex < 0 || !sellPiece(sellIndex)) break;
        }
        if (shop.getOfferCost(slot) > gold) break;
        if (buyPiece(shop.createOfferedPiece(slot)))
        {
            shop.removeOffer(slot);
            purchases++;
        }
        else
            break;
    }
}

void AIPlayer::arrangeFormation()
{
    int selected[MAX_OWNED_PIECES];
    int selectedCount;
    int frontCol = 0;
    int backCol = 0;
    int i;
    clearAllPositions();
    selectCorePieces(selected, selectedCount);
    /* AI 占用第 0～1 行：近战放第 1 行迎敌，远程放第 0 行保持距离。 */
    for (i = 0; i < selectedCount; i++)
    {
        ChessPiece* piece = pieces[selected[i]];
        if (isFrontlineType(piece->getType()))
            placePiece(selected[i], 1, frontCol++, 0, 1);
        else
            placePiece(selected[i], 0, backCol++, 0, 1);
    }
}

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
    /* 这些并行数组的同一纵向下标共同描述一种职业，复制后由商店长期保存。 */
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
    /* 使用确定性伪随机数生成器，使相同种子产生相同的商店商品序列。 */
    if (maximum <= 0) return 0;
    /* 线性同余法先推进内部状态，再将结果压缩到 [0, maximum) 范围。 */
    randomState = randomState * 1103515245UL + 12345UL;
    return (int)((randomState / 65536UL) % (unsigned long)maximum);
}

void Shop::refresh()
{
    int i;
    for (i = 0; i < SHOP_SLOT_COUNT; i++) offers[i] = nextRandom(PIECE_TYPE_COUNT);
}

void Shop::showOffers(const Player& viewer, bool readOnly) const
{
    int i;
    if (readOnly)
        cout << "\n*** Shop ***\n";
    else
        cout << "\n--- Shop ---\n";
    viewer.showStatus(false);
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
    /* 工厂函数依据职业创建对应派生类，调用者负责接管返回对象的所有权。 */
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
    /* 战斗中的移动是临时的，准备阶段的阵型位置会被保留。 */
    int i;
    for (i = 0; i < originalCount; i++)
    {
        originalPieces[i]->setPosition(originalRows[i], originalCols[i]);
        originalPieces[i]->restoreAfterBattle();
    }
}

void Battlefield::collectBattlePieces(Player& first, Player& second)
{
    /* 收集的只是玩家所拥有对象的非拥有型指针，战场不会 delete 这些棋子。 */
    int i;
    battlePieceCount = 0;
    originalCount = 0;
    for (i = 0; i < first.getPieceCount(); i++)
    {
        ChessPiece* piece = first.getPiece(i);
        if (piece != 0 && piece->isDeployed() && battlePieceCount < MAX_BATTLE_PIECES)
        {
            /* 在覆盖队伍和战斗状态前保存准备阶段坐标，以便战后完整恢复。 */
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
    /* 部署采用“全部校验成功才生效”的方式，失败时恢复所有棋子的原始状态。 */
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
            /* 越界或双方棋子重叠都会使本次部署整体失败。 */
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
    /* 由于不使用 STL 算法，此处手动实现选择排序。 */
    /* 速度高者先行动；速度相同则实例编号较小者先行动，保证结果可复现。 */
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
    /* 方向数组的固定顺序也承担平局规则：距离同样近时依次偏好上、左、右、下。 */
    const int directions[4][2] = {{-1,0},{0,-1},{0,1},{1,0}};
    int bestRow = piece->getRow();
    int bestCol = piece->getCol();
    int bestDistance = piece->distanceTo(target);
    int i;
    for (i = 0; i < 4; i++)
    {
        /* 选择一个能够缩短曼哈顿距离的空闲上下左右相邻格。 */
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
    /*
     * 战斗主循环：部署并固定行动顺序；每名存活棋子重新构造敌我数组，
     * 能攻击则释放职业技能，否则向选定目标移动一步，直到一方全灭或达到轮数上限。
     */
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
            /* 每次行动前重建数组，确保本轮更早行动造成的死亡会立即反映出来。 */
            buildTeamArray(1 - acting->getTeam(), enemies, enemyCount);
            buildTeamArray(acting->getTeam(), allies, allyCount);
            if (enemyCount == 0) break;
            targetIndex = acting->selectTarget(enemies, enemyCount);
            if (targetIndex < 0 || targetIndex >= enemyCount) continue;
            if (acting->canAttack(enemies[targetIndex]))
                acting->useSkill(enemies[targetIndex], enemies, enemyCount, allies, allyCount);
            else
                moveTowardTarget(acting, enemies[targetIndex]);
            /* 死亡对象仍由 Player 拥有，这里仅清除棋盘格引用以允许后续棋子通过。 */
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
        /* 在恢复满血之前保存存活星级，供游戏回合结算伤害时使用。 */
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
    /* 清空战场引用，但不释放任何棋子；棋子的生命周期继续由各自 Player 管理。 */
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
    /* 先关闭上一次可能残留的流，使同一 FileManager 可以重复运行整套测试。 */
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
    /* 在开始逐例读取前先校验用例数量，防止异常文件导致无限或超量循环。 */
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
                               Shop& testShop, int& expectedWinner)
{
    /*
     * 第一行记录测试用例总数，之后每组用例的格式如下：
     * 预期胜者 玩家棋子数 AI 棋子数
     * 随后先记录玩家棋子，再记录 AI 棋子：职业 星级 行 列
     */
    int humanCount, aiCount;
    int i;
    if (!filesOpened || currentCase >= totalCases) return false;
    inputFile >> expectedWinner >> humanCount >> aiCount;
    if (!inputFile || expectedWinner < 0 || expectedWinner > 2 ||
        humanCount < 1 || humanCount > MAX_DEPLOYED_PIECES ||
        aiCount < 1 || aiCount > MAX_DEPLOYED_PIECES)
    {
        strcpy(errorMessage, "Invalid test case header.");
        return false;
    }
    humanPlayer.resetForNewGame("TestHuman", 0);
    aiPlayer.resetForNewGame("TestAI", 1);
    /* 两支队伍的记录连续存放，用下标是否小于 humanCount 判断对象归属。 */
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
        /* 测试文件只保存目标星级，通过重复调用升星运算符还原完整属性。 */
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
      lastBattleResult(RESULT_DRAW), gameSeed((unsigned long)time(0))
{
    shop.setRandomSeed(gameSeed);
}

GameSystem::~GameSystem()
{
}

int GameSystem::readInteger(int minimum, int maximum)
{
    /* 同时处理非数字输入和越界数字；失败后必须清除错误位及整行残留字符。 */
    int value;
    while (true)
    {
        cin >> value;
        if (cin && value >= minimum && value <= maximum) return value;
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
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
    cout << "7. Return piece to bench\n";
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
    /* 重置双方所有动态棋子和局内资源，并为本局建立新的商店随机序列。 */
    human.resetForNewGame("Human", 0);
    computer.resetForNewGame("Computer", 1);
    currentRound = 0;
    gameActive = 1;
    gameSeed = (unsigned long)time(0);
    shop.setRandomSeed(gameSeed);
    while (gameActive && !isGameOver()) playRound();
    if (isGameOver()) printGameResult();
}

void GameSystem::prepareHuman()
{
    /* 准备阶段持续响应菜单操作，只有已上阵棋子后才能正式结束准备。 */
    int preparing = 1;
    shop.refresh();
    while (preparing && gameActive)
    {
        int choice;
        showPreparationMenu();
        choice = readInteger(0, 8);
        if (choice == 1)
        {
            human.showStatus(true);
            human.showRoster(true);
        }
        else if (choice == 2) shop.showOffers(human, true);
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
        else if (choice == 7) handleReturnToBench();
        else if (choice == 8)
        {
            if (!human.hasDeployedPiece()) cout << "Deploy at least one piece first.\n";
            else preparing = 0;
        }
        else
        {
            /* “放弃游戏”需要二次确认，避免误按 0 直接结束当前对局。 */
            cout << "Abandon this game? 1=Yes 0=No: ";
            if (readInteger(0, 1) == 1)
            {
                gameActive = 0;
            }
        }
    }
}

void GameSystem::handlePurchase()
{
    int slot;
    shop.showOffers(human, false);
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
    human.showRoster(false);
    if (human.getPieceCount() == 0) return;
    cout << "Choose piece to sell (1-" << human.getPieceCount() << ", 0 cancel): ";
    index = readInteger(0, human.getPieceCount());
    if (index > 0 && human.sellPiece(index - 1)) cout << "Piece sold.\n";
}

void GameSystem::handlePlacement()
{
    int index, row, col;
    human.showRoster(false);
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

void GameSystem::handleReturnToBench()
{
    int index;
    human.showRoster(false);
    if (human.getDeployedCount() == 0)
    {
        cout << "No deployed pieces.\n";
        return;
    }
    cout << "Choose deployed piece (1-" << human.getPieceCount() << ", 0 cancel): ";
    index = readInteger(0, human.getPieceCount());
    if (index == 0) return;
    if (human.returnPieceToBench(index - 1)) cout << "Piece returned to bench.\n";
    else cout << "That piece is already on the bench.\n";
}

void GameSystem::playRound()
{
    /* 一个游戏回合依次包含经济增长、玩家准备、AI 准备、自动战斗和伤害结算。 */
    int roundsUsed;
    int survivorStars;
    currentRound++;
    human.addGold(5);
    computer.addGold(5);
    cout << "\n========== GAME ROUND " << currentRound << " ==========\n";
    human.showStatus(false);
    prepareHuman();
    if (!gameActive) return;
    shop.refresh();
    computer.performShopping(shop);
    computer.arrangeFormation();
    cout << "\nAI preparation completed.\n";
    computer.showStatus(false);
    lastBattleResult = battlefield.runBattle(human, computer, true, roundsUsed);
    /* 只有获胜方的存活星级参与伤害计算，平局不读取任何一方的存活值。 */
    if (lastBattleResult == RESULT_HUMAN_WIN)
        survivorStars = battlefield.calculateSurvivorStars(0);
    else if (lastBattleResult == RESULT_AI_WIN)
        survivorStars = battlefield.calculateSurvivorStars(1);
    else survivorStars = 0;
    settleBattle(lastBattleResult, survivorStars);
}

void GameSystem::settleBattle(int result, int survivorStars)
{
    /* 胜者造成“2 + 存活星级总和”的伤害；平局则双方固定受到 2 点伤害。 */
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
    human.showStatus(false);
    computer.showStatus(false);
}

void GameSystem::showInstructions() const
{
    cout << "\n*** Instructions ***\n";
    cout << "Buy pieces, combine three identical stars, and deploy up to six pieces.\n";
    cout << "Human pieces must be placed in rows 4-5. AI uses rows 0-1.\n";
    cout << "Battle is automatic. W=Warrior, A=Archer, M=Mage, P=Paladin.\n";
    cout << "Lowercase board symbols belong to the AI. Maximum star level is 3.\n";
}

void GameSystem::runFileTests()
{
    /* 使用独立对象运行测试，避免覆盖菜单对局中的玩家、商店和战场状态。 */
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
        int roundsUsed;
        int actualWinner;
        if (!fileManager.readNextCase(testHuman, testAI, testShop, expectedWinner))
        {
            cout << "Test error: " << fileManager.getErrorMessage() << "\n";
            break;
        }
        /* 关闭 verbose 可跳过棋盘输出和 Sleep 延时，使批量测试快速完成。 */
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
    /* main() 只负责创建控制器，所有游戏流程都由 GameSystem 管理。 */
    GameSystem game;
    game.run();
    return 0;
}
