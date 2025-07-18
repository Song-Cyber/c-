#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include "tools.hpp"
#include <time.h>
#include <easyx.h>
#include <conio.h>
#include <string>
#include <vector>
#include <algorithm>
#include <Windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

using namespace std;

// 游戏常量定义
#define BULLET_NUM 30
#define ENEMY_NUM 20
#define ENEMY_BULLET_NUM 50
#define MAX_LIVES 1500         // 最大生命上限1500条
#define BASE_MAX_HEALTH 3      // 基础最大生命值
#define WIN_SCORE 9999
#define CLEAR_ITEM_COOLDOWN 60
#define REBORN_TIME 30         // 重生时间为0.5秒(30帧)
#define INPUT_COOLDOWN 100     // 输入冷却时间0.1秒(100毫秒)

// 游戏状态枚举
enum GameState {
    START_SCREEN,
    PLAYING,
    GAME_OVER,
    WIN_SCREEN,
    PAUSED
};

// 道具类型枚举
enum ItemType {
    CLEAR_SCREEN_ITEM,
    FIRE_RATE_ITEM
};

// 道具类
class Item {
public:
    int x;
    int y;
    int width;
    int height;
    bool active;
    ItemType type;
    int timer;

    Item() : active(false), timer(0) {
        width = 30;
        height = 30;
    }

    void init(int x, int y, ItemType type) {
        this->x = x;
        this->y = y;
        this->type = type;
        active = true;
        timer = 0;
    }

    void update() {
        if (!active) return;

        y += 2;

        if (y > getheight()) {
            active = false;
        }
    }

    bool checkCollision(int x1, int y1, int w1, int h1) {
        return x < x1 + w1 &&
            x + width > x1 &&
            y < y1 + h1 &&
            y + height > y1;
    }
};

// 资源管理器类
class ResourceManager {
public:
    IMAGE img_bk;
    IMAGE img_gamer[2];
    IMAGE img_bullet[2];
    IMAGE img_enemy[3];
    IMAGE img_enemy_bullet;
    IMAGE img_life;
    IMAGE img_title;
    IMAGE img_gameover;
    IMAGE img_win;
    IMAGE img_item[2];
    IMAGE img_health;
    IMAGE img_pause_bg;     // 暂停背景
    IMAGE img_continue;     // 继续按钮
    IMAGE img_restart;      // 重新开始按钮
    IMAGE img_quit;         // 退出按钮

    ResourceManager() {
        loadResource();
    }

private:
    void loadResource() {
        loadimage(&img_bk, "Resource/images/background.png");
        loadimage(img_gamer + 0, "Resource/images/hero1.png");
        loadimage(img_gamer + 1, "Resource/images/hero2.png");
        loadimage(img_bullet + 0, "Resource/images/bullet1.png");
        loadimage(img_bullet + 1, "Resource/images/bullet2.png");
        loadimage(&img_enemy[0], "Resource/images/enemy1.png");
        loadimage(&img_enemy[1], "Resource/images/enemy2.png");
        loadimage(&img_enemy[2], "Resource/images/explosion.png");
        loadimage(&img_enemy_bullet, "Resource/images/bullet.png");
        loadimage(&img_life, "Resource/images/life.png");
        loadimage(&img_title, "Resource/images/title.png");
        loadimage(&img_gameover, "Resource/images/gameover.png");
        loadimage(&img_win, "Resource/images/win.png");
        loadimage(&img_item[0], "Resource/images/weapon1.png");
        loadimage(&img_item[1], "Resource/images/weapon2.png");
        // 加载暂停界面资源
        loadimage(&img_pause_bg, "Resource/images/background.png");
        loadimage(&img_continue, "Resource/images/continue.png");
        loadimage(&img_restart, "Resource/images/restart.png");
        loadimage(&img_quit, "Resource/images/quit.png");
    }
};

// 定时器类
class Timer {
private:
    static int start[10];

public:
    static bool check(int ms, int id) {
        int end = clock();
        if (end - start[id] >= ms) {
            start[id] = end;
            return true;
        }
        return false;
    }
};

int Timer::start[10] = { 0 };

// 爆炸效果类
class Explosion {
public:
    int x;
    int y;
    int frame;
    bool active;
    int timer;

    Explosion() : active(false), frame(0), timer(0) {}

    void init(int x, int y) {
        this->x = x;
        this->y = y;
        frame = 0;
        active = true;
        timer = 0;
    }

    void update() {
        if (!active) return;

        timer++;
        if (timer > 10) {
            frame++;
            timer = 0;
        }

        if (frame > 5) {
            active = false;
        }
    }

    void draw(ResourceManager& res) {
        if (active) {
            putimage(x, y, &res.img_enemy[2]);
        }
    }
};

// 子弹基类
class Bullet {
public:
    int x;
    int y;
    int width;
    int height;
    bool isDie;
    int type;

    Bullet() : isDie(true), type(0) {}

    virtual void move() = 0;
    virtual void draw(ResourceManager& res) = 0;

    bool checkCollision(int x1, int y1, int w1, int h1) {
        return x < x1 + w1 &&
            x + width > x1 &&
            y < y1 + h1 &&
            y + height > y1;
    }
};

// 玩家子弹类
class PlayerBullet : public Bullet {
public:
    PlayerBullet() {
        width = 10;
        height = 20;
        type = 0;
    }

    void move() override {
        y -= 8;
        if (y < 0) {
            isDie = true;
        }
    }

    void draw(ResourceManager& res) override {
        if (!isDie) {
            drawImg(x, y, res.img_bullet + type);
        }
    }
};

// 敌机子弹类
class EnemyBullet : public Bullet {
public:
    EnemyBullet() {
        width = 8;
        height = 16;
        type = 1;
    }

    void move() override {
        y += 5;
        if (y > getheight()) {
            isDie = true;
        }
    }

    void draw(ResourceManager& res) override {
        if (!isDie) {
            drawImg(x, y, &res.img_enemy_bullet);
        }
    }
};

// 飞机基类
class Plane {
public:
    int x;
    int y;
    int width;
    int height;
    bool isDie;
    int frame;
    int type;

    Plane() : isDie(true), frame(0), type(0) {}

    virtual void move() = 0;
    virtual void draw(ResourceManager& res) = 0;

    bool checkCollision(Plane* other) {
        return x < other->x + other->width &&
            x + width > other->x &&
            y < other->y + other->height &&
            y + height > other->y;
    }

    bool checkCollision(Bullet* bullet) {
        return bullet->checkCollision(x, y, width, height);
    }
};

// 玩家类
class Player : public Plane {
private:
    int lives;
    int health;
    int maxHealth;          // 最大生命值
    int invincibleTime;
    bool isInvincible;
    int fireRateBoostTime;
    int baseFireRate;
    int rebornTime;

public:
    Player() : lives(3), health(BASE_MAX_HEALTH), maxHealth(BASE_MAX_HEALTH),
        invincibleTime(0), isInvincible(false), fireRateBoostTime(0),
        baseFireRate(150), rebornTime(0) {
        width = 60;
        height = 80;
        type = 0;
    }

    // 更新最大生命值（根据游戏难度）
    void updateMaxHealth(int level) {
        // 记录旧的最大生命值
        int oldMaxHealth = maxHealth;

        // 计算难度加成（每10级增加10点生命，上限50级）
        int effectiveLevel = min(level, 50);
        int healthBonus = (effectiveLevel / 10) * 10;

        maxHealth = BASE_MAX_HEALTH + healthBonus;

        //  提升血量上限时自动补满血量
        if (maxHealth > oldMaxHealth) {
            health = maxHealth;
        }
        // 确保当前生命值不超过新的最大生命值
        else if (health > maxHealth) {
            health = maxHealth;
        }
    }

    void init(int screenWidth, int screenHeight) {
        x = (screenWidth - width) / 2;
        y = screenHeight - height - 50;
        isDie = false;
        frame = 0;
        lives = 3; // 初始3条命
        health = maxHealth;
        isInvincible = false;
        invincibleTime = 0;
        fireRateBoostTime = 0;
        rebornTime = 0;
    }

    void addLives(int count) {
        lives += count;
        if (lives > MAX_LIVES) {
            lives = MAX_LIVES;
        }
    }

    void move() override {
        if (isDie) return;

        int speed = 5;
        if (GetAsyncKeyState(VK_UP) && y > 0) {
            y -= speed;
        }
        if (GetAsyncKeyState(VK_DOWN) && y + height < getheight()) {
            y += speed;
        }
        if (GetAsyncKeyState(VK_LEFT) && x > 0) {
            x -= speed;
        }
        if (GetAsyncKeyState(VK_RIGHT) && x + width < getwidth()) {
            x += speed;
        }
    }

    void draw(ResourceManager& res) override {
        if (!isDie && (!isInvincible || (invincibleTime / 10) % 2 == 0)) {
            drawImg(x, y, res.img_gamer + frame);
        }
    }

    void updateFrame() {
        if (Timer::check(10, 1)) {
            frame = (frame + 1) % 2;
        }
    }

    void update() {
        if (rebornTime > 0) {
            rebornTime--;
        }

        if (isInvincible) {
            invincibleTime--;
            if (invincibleTime <= 0) {
                isInvincible = false;
                isDie = false;
            }
        }

        if (fireRateBoostTime > 0) {
            fireRateBoostTime--;
        }
    }

    void hit() {
        if (isInvincible) return;

        health--;
        if (health <= 0) {
            isDie = true;
            lives--;
            if (lives > 0) {
                health = maxHealth;  // 使用当前的最大生命值
                isInvincible = true;
                invincibleTime = REBORN_TIME;
                rebornTime = REBORN_TIME;
                x = (getwidth() - width) / 2;
                y = getheight() - height - 50;
            }
        }
    }

    void useItem(ItemType itemType) {
        switch (itemType) {
        case FIRE_RATE_ITEM:
            fireRateBoostTime = 180;
            break;
        }
    }

    int getLives() const { return lives; }
    int getHealth() const { return health; }
    int getMaxHealth() const { return maxHealth; }  // 新增获取最大生命值
    int getRebornTime() const { return rebornTime; }

    bool canShoot() const {
        if (isDie) return false;

        int fireRate = baseFireRate;
        if (fireRateBoostTime > 0) {
            fireRate /= 2;
        }

        return Timer::check(fireRate, 0);
    }
};

// 敌机类
class Enemy : public Plane {
private:
    int shootCooldown;
    int health;

public:
    Enemy() : shootCooldown(0), health(1) {
        width = 50;
        height = 50;
    }

    void init(int type, int screenWidth) {
        this->type = type;
        x = rand() % (screenWidth - width);
        y = -height;
        isDie = false;
        frame = 0;
        health = (type == 0) ? 1 : 2;
        shootCooldown = 0;
    }

    void move() override {
        if (isDie) return;

        int speed = (type == 0) ? 3 : 2;
        y += speed;

        if (y > getheight()) {
            isDie = true;
        }
    }

    void draw(ResourceManager& res) override {
        if (!isDie) {
            drawImg(x, y, &res.img_enemy[type]);
        }
    }

    bool canShoot() {
        if (isDie) return false;

        if (shootCooldown <= 0 && rand() % 100 < 2) {
            shootCooldown = 60;
            return true;
        }
        else if (shootCooldown > 0) {
            shootCooldown--;
        }
        return false;
    }

    bool hit() {
        health--;
        if (health <= 0) {
            isDie = true;
            return true;
        }
        return false;
    }

    int getScore() const {
        return (type == 0) ? 1 : 3;
    }
};

// 游戏主类
class Game {
private:
    ResourceManager res;
    Player player;
    vector<Enemy> enemies;
    vector<PlayerBullet> playerBullets;
    vector<EnemyBullet> enemyBullets;
    vector<Explosion> explosions;
    vector<Item> items;
    GameState gameState;
    int score;
    int level;
    int screenWidth;
    int screenHeight;
    int itemSpawnTimer;
    bool clearItemActive;
    bool fireRateItemActive;
    int clearItemCooldown;
    bool bgmPlaying;
    int lastLifeScore;  // 记录上次加命时的分数
    int itemSpawnProbability;  // 道具生成概率
    bool pauseKeyPressed; // 防止空格键连续触发
    int pauseSelection;   // 暂停界面当前选中的选项索引
    clock_t lastInputTime; // 上次输入处理时间
    int soundCounter;     // 音效计数器，用于生成唯一别名

public:
    Game(int width, int height) :
        screenWidth(width), screenHeight(height),
        score(0), level(1), gameState(START_SCREEN),
        itemSpawnTimer(0), clearItemActive(false),
        fireRateItemActive(false), clearItemCooldown(0),
        bgmPlaying(false), lastLifeScore(0), itemSpawnProbability(0),
        pauseKeyPressed(false), pauseSelection(0), lastInputTime(0),
        soundCounter(0) {

        enemies.resize(ENEMY_NUM);
        playerBullets.resize(BULLET_NUM);
        enemyBullets.resize(ENEMY_BULLET_NUM);
        explosions.resize(10);
        items.resize(2);
    }

    void init() {
        player.updateMaxHealth(level);  // 初始化时设置最大生命值
        player.init(screenWidth, screenHeight);
        score = 0;
        level = 1;
        lastLifeScore = 0;  // 重置加命分数记录
        itemSpawnProbability = 40;  // 初始道具概率
        pauseSelection = 0; // 重置暂停界面选择
        lastInputTime = clock(); // 重置输入计时器
        soundCounter = 0;    // 重置音效计数器

        for (auto& enemy : enemies) {
            enemy = Enemy();
        }

        for (auto& bullet : playerBullets) {
            bullet = PlayerBullet();
        }
        for (auto& bullet : enemyBullets) {
            bullet = EnemyBullet();
        }

        for (auto& explosion : explosions) {
            explosion = Explosion();
        }

        for (auto& item : items) {
            item.active = false;
        }

        clearItemActive = false;
        fireRateItemActive = false;
        itemSpawnTimer = 0;
        clearItemCooldown = 0;
        pauseKeyPressed = false;

        gameState = PLAYING;

        // 播放游戏背景音乐
        playSound("Resource/sound/game_bgm.wav", true);
    }

    void playSound(const char* path, bool loop = false) {
        char cmd[256];
        char alias[50];

        // 检查文件是否存在
        FILE* file = fopen(path, "r");
        if (!file) {
            printf("Error: Sound file not found: %s\n", path);
            return;
        }
        fclose(file);

        if (loop) {
            // 背景音乐处理
            if (bgmPlaying) {
                // 如果已经有背景音乐在播放，先关闭
                mciSendString("close bgm", NULL, 0, NULL);
                bgmPlaying = false;
            }

            // 打开并播放背景音乐
            sprintf(cmd, "open \"%s\" type mpegvideo alias bgm", path);
            MCIERROR err = mciSendString(cmd, NULL, 0, NULL);
            if (err != 0) {
                char errorMsg[256];
                mciGetErrorString(err, errorMsg, sizeof(errorMsg));
                printf("MCI Error: %s\n", errorMsg);
                return;
            }

            // 设置重复播放
            sprintf(cmd, "play bgm repeat");
            err = mciSendString(cmd, NULL, 0, NULL);
            if (err != 0) {
                char errorMsg[256];
                mciGetErrorString(err, errorMsg, sizeof(errorMsg));
                printf("MCI Error: %s\n", errorMsg);
                return;
            }

            bgmPlaying = true;
        }
        else {
            // 音效处理 - 使用唯一别名
            soundCounter++;
            sprintf(alias, "sfx%d", soundCounter);

            // 先关闭可能存在的同名别名
            sprintf(cmd, "close %s", alias);
            mciSendString(cmd, NULL, 0, NULL);

            // 使用更简单的打开方式，避免驱动程序错误
            sprintf(cmd, "open \"%s\" alias %s", path, alias);
            MCIERROR err = mciSendString(cmd, NULL, 0, NULL);
            if (err != 0) {
                // 如果打开失败，尝试使用更通用的方式
                sprintf(cmd, "open \"%s\" type waveaudio alias %s", path, alias);
                err = mciSendString(cmd, NULL, 0, NULL);
                if (err != 0) {
                    char errorMsg[256];
                    mciGetErrorString(err, errorMsg, sizeof(errorMsg));
                    printf("MCI Error: %s\n", errorMsg);
                    return;
                }
            }

            // 播放音效，不等待播放完成
            sprintf(cmd, "play %s", alias);
            err = mciSendString(cmd, NULL, 0, NULL);
            if (err != 0) {
                char errorMsg[256];
                mciGetErrorString(err, errorMsg, sizeof(errorMsg));
                printf("MCI Error: %s\n", errorMsg);
                return;
            }
        }
    }

    void createEnemy() {
        for (auto& enemy : enemies) {
            if (enemy.isDie) {
                int type = rand() % 2;
                enemy.init(type, screenWidth);
                break;
            }
        }
    }

    void createPlayerBullet() {
        for (auto& bullet : playerBullets) {
            if (bullet.isDie) {
                bullet.x = player.x + player.width / 2;
                bullet.y = player.y;
                bullet.isDie = false;
                break;
            }
        }
    }

    void createEnemyBullet(int x, int y) {
        for (auto& bullet : enemyBullets) {
            if (bullet.isDie) {
                bullet.x = x;
                bullet.y = y;
                bullet.isDie = false;
                break;
            }
        }
    }

    void createExplosion(int x, int y) {
        for (auto& explosion : explosions) {
            if (!explosion.active) {
                explosion.init(x, y);
                break;
            }
        }
    }

    void createItem(int x, int y, ItemType type) {
        bool alreadyExists = false;
        for (auto& item : items) {
            if (item.active && item.type == type) {
                alreadyExists = true;
                break;
            }
        }

        if (alreadyExists) return;

        for (auto& item : items) {
            if (!item.active) {
                item.init(x, y, type);

                if (type == CLEAR_SCREEN_ITEM) {
                    clearItemActive = true;
                }
                else {
                    fireRateItemActive = true;
                }

                break;
            }
        }
    }

    // 道具生成概率随难度提升
    void spawnRandomItem() {
        bool skipClearItem = (clearItemCooldown > 0);

        // 基础概率40%，每10级增加8%
        itemSpawnProbability = 40 + min(level, 50) / 10 * 8;
        if (itemSpawnProbability > 100) itemSpawnProbability = 100;

        if (rand() % 100 < itemSpawnProbability) {
            int x = rand() % (screenWidth - 30);
            int y = -30;

            ItemType type;
            if (skipClearItem) {
                type = FIRE_RATE_ITEM;
            }
            else {
                type = (rand() % 2 == 0) ? CLEAR_SCREEN_ITEM : FIRE_RATE_ITEM;
            }

            bool typeExists = false;
            for (auto& item : items) {
                if (item.active && item.type == type) {
                    typeExists = true;
                    break;
                }
            }

            if (!typeExists) {
                createItem(x, y, type);
            }
        }
    }

    void clearScreen() {
        int enemyCount = 0;

        for (auto& enemy : enemies) {
            if (!enemy.isDie) {
                enemy.isDie = true;
                createExplosion(enemy.x, enemy.y);
                score += enemy.getScore();
                enemyCount++;
                playSound("Resource/sound/clear.wav", false);
            }
        }

        for (auto& bullet : enemyBullets) {
            if (!bullet.isDie) {
                bullet.isDie = true;
            }
        }

        if (enemyCount > 0) {
            score += enemyCount * 2;
        }

        playSound("Resource/sound/clear.wav", false);
    }

    void update() {
        // 更新玩家最大生命值（根据当前难度）
        player.updateMaxHealth(level);

        //  每100分增加1条命
        if (score - lastLifeScore >= 100) {
            int livesToAdd = (score - lastLifeScore) / 100;
            player.addLives(livesToAdd);
            lastLifeScore += livesToAdd * 100;
        }

        if (clearItemCooldown > 0) {
            clearItemCooldown--;
        }

        for (auto& explosion : explosions) {
            explosion.update();
        }

        player.updateFrame();
        player.update();

        for (auto& enemy : enemies) {
            if (!enemy.isDie) {
                enemy.move();
                if (enemy.canShoot()) {
                    createEnemyBullet(enemy.x + enemy.width / 2, enemy.y + enemy.height);
                }
            }
        }

        for (auto& bullet : playerBullets) {
            if (!bullet.isDie) {
                bullet.move();
            }
        }

        for (auto& bullet : enemyBullets) {
            if (!bullet.isDie) {
                bullet.move();
            }
        }

        for (auto& item : items) {
            if (item.active) {
                item.update();

                if (item.checkCollision(player.x, player.y, player.width, player.height)) {
                    if (item.type == CLEAR_SCREEN_ITEM) {
                        clearScreen();
                        clearItemCooldown = CLEAR_ITEM_COOLDOWN;
                    }
                    else {
                        player.useItem(item.type);
                        playSound("Resource/sound/button.wav", false);
                    }

                    item.active = false;

                    if (item.type == CLEAR_SCREEN_ITEM) {
                        clearItemActive = false;
                    }
                    else {
                        fireRateItemActive = false;
                    }
                }

                if (!item.active && item.y > getheight()) {
                    if (item.type == CLEAR_SCREEN_ITEM) {
                        clearItemActive = false;
                    }
                    else {
                        fireRateItemActive = false;
                    }
                }
            }
        }

        for (auto& bullet : playerBullets) {
            if (bullet.isDie) continue;

            for (auto& enemy : enemies) {
                if (enemy.isDie) continue;

                if (enemy.checkCollision(&bullet)) {
                    if (enemy.hit()) {
                        score += enemy.getScore();
                        createExplosion(enemy.x, enemy.y);
                        playSound("Resource/sound/r.wav", false);

                        if (rand() % 100 < 20) {
                            spawnRandomItem();
                        }
                    }
                    bullet.isDie = true;
                    break;
                }
            }
        }

        for (auto& enemy : enemies) {
            if (enemy.isDie) continue;

            if (player.checkCollision(&enemy)) {
                player.hit();
                createExplosion(player.x, player.y);
                enemy.isDie = true;

                if (player.getLives() > 0 && player.getHealth() == player.getMaxHealth()) {
                    clearScreen();
                    clearItemCooldown = CLEAR_ITEM_COOLDOWN;
                }
                break;
            }
        }

        for (auto& bullet : enemyBullets) {
            if (bullet.isDie) continue;

            if (player.checkCollision(&bullet)) {
                player.hit();
                createExplosion(player.x, player.y);
                bullet.isDie = true;

                if (player.getLives() > 0 && player.getHealth() == player.getMaxHealth()) {
                    clearScreen();
                    clearItemCooldown = CLEAR_ITEM_COOLDOWN;
                }
                break;
            }
        }

        if (player.getLives() <= 0) {
            gameState = GAME_OVER;
            mciSendString("close bgm", NULL, 0, NULL);
            bgmPlaying = false;
        }
        else if (score >= WIN_SCORE) {
            gameState = WIN_SCREEN;
            playSound("Resource/sound/win_sound.wav", false);
            mciSendString("close bgm", NULL, 0, NULL);
            bgmPlaying = false;
        }

        level = score / 10 + 1;

        itemSpawnTimer++;
        if (itemSpawnTimer >= 180) {
            spawnRandomItem();
            itemSpawnTimer = 0;
        }
    }

    void handleInput() {
        clock_t currentTime = clock();
        bool inputReady = (currentTime - lastInputTime) >= INPUT_COOLDOWN;

        if (gameState == START_SCREEN) {
            if (inputReady && (GetAsyncKeyState(VK_RETURN) & 0x8000)) {
                // 开始游戏时播放游戏背景音乐
                playSound("Resource/sound/game_bgm.wav", true);
                init();
                lastInputTime = currentTime;
            }
        }
        else if (gameState == PLAYING) {
            // 空格键暂停游戏
            if (GetAsyncKeyState(VK_SPACE) & 0x8000) {
                if (!pauseKeyPressed) {
                    pauseKeyPressed = true;
                    gameState = PAUSED;
                    // 暂停背景音乐
                    mciSendString("pause bgm", NULL, 0, NULL);
                    lastInputTime = currentTime; // 重置输入计时器
                }
            }
            else {
                pauseKeyPressed = false;
            }

            player.move();
            if (player.canShoot()) {
                createPlayerBullet();
            }
        }
        else if (gameState == PAUSED) {
            // 空格键继续游戏
            if (GetAsyncKeyState(VK_SPACE) & 0x8000) {
                if (!pauseKeyPressed) {
                    pauseKeyPressed = true;
                    gameState = PLAYING;
                    // 继续播放背景音乐
                    mciSendString("resume bgm", NULL, 0, NULL);
                    lastInputTime = currentTime; // 重置输入计时器
                }
            }
            else {
                pauseKeyPressed = false;
            }

            // 使用键盘方向键选择选项（有冷却时间）
            if (inputReady) {
                if (GetAsyncKeyState(VK_UP) & 0x8000) {
                    pauseSelection = (pauseSelection + 2) % 3; // 向上移动，相当于减1
                    lastInputTime = currentTime;
                }
                else if (GetAsyncKeyState(VK_DOWN) & 0x8000) {
                    pauseSelection = (pauseSelection + 1) % 3;
                    lastInputTime = currentTime;
                }
                else if (GetAsyncKeyState(VK_RETURN) & 0x8000) {
                    // 处理选中的选项
                    switch (pauseSelection) {
                    case 0: // 继续游戏
                        gameState = PLAYING;
                        mciSendString("resume bgm", NULL, 0, NULL);
                        break;
                    case 1: // 重新开始
                        gameState = START_SCREEN;
                        mciSendString("close bgm", NULL, 0, NULL);
                        bgmPlaying = false;
                        playSound("Resource/sound/start_bgm.wav", true);
                        break;
                    case 2: // 退出游戏
                        gameState = GAME_OVER;
                        mciSendString("close bgm", NULL, 0, NULL);
                        bgmPlaying = false;
                        break;
                    }
                    lastInputTime = currentTime;
                }
            }
        }
        else if (gameState == GAME_OVER || gameState == WIN_SCREEN) {
            if (inputReady && (GetAsyncKeyState('R') & 0x8000)) {
                playSound("Resource/sound/game_start.wav", false);
                playSound("Resource/sound/game_bgm.wav", true);
                init();
                lastInputTime = currentTime;
            }
            if (inputReady && (GetAsyncKeyState(VK_ESCAPE) & 0x8000)) {
                exit(0);
            }
        }
    }

    void draw() {
        putimage(0, 0, &res.img_bk);

        for (auto& enemy : enemies) {
            enemy.draw(res);
        }

        for (auto& bullet : playerBullets) {
            bullet.draw(res);
        }
        for (auto& bullet : enemyBullets) {
            bullet.draw(res);
        }

        for (auto& explosion : explosions) {
            explosion.draw(res);
        }

        for (auto& item : items) {
            if (item.active) {
                drawImg(item.x, item.y, &res.img_item[item.type]);
            }
        }

        player.draw(res);

        drawUI();
    }

    void drawStartScreen() {
        putimage(0, 0, &res.img_bk);
        drawImg((screenWidth - res.img_title.getwidth()) / 2, 100, &res.img_title);

        settextcolor(LIGHTGREEN);
        settextstyle(36, 0, "楷体");
        outtextxy((screenWidth - textwidth("按 ENTER 开始游戏")) / 2, 400, "按 ENTER 开始游戏");

        settextcolor(LIGHTCYAN);
        settextstyle(24, 0, "楷体");
        outtextxy((screenWidth - textwidth("使用方向键移动，自动射击")) / 2, 500, "使用方向键移动，自动射击");

        settextcolor(YELLOW);
        settextstyle(20, 0, "楷体");
        outtextxy((screenWidth - textwidth("击败小飞机得1分，大飞机得3分，达到9999分获胜")) / 2, 550,
            "击败小飞机得1分，大飞机得3分，达到9999分获胜");
    }

    void drawPauseScreen() {
        // 绘制半透明背景
        putimage(0, 0, &res.img_pause_bg);

        // 绘制半透明黑色遮罩
        setfillcolor(BLACK);
        setfillstyle(BS_SOLID);
        solidrectangle(0, 0, screenWidth, screenHeight);
        settextcolor(WHITE);
        settextstyle(48, 0, "楷体");
        outtextxy((screenWidth - textwidth("游戏暂停")) / 2, 100, "游戏暂停");

        // 绘制按钮
        int centerX = screenWidth / 2;
        int centerY = screenHeight / 2;

        // 继续按钮
        drawImg(centerX - 100, centerY - 100, &res.img_continue);

        // 重新开始按钮
        drawImg(centerX - 100, centerY - 30, &res.img_restart);

        // 退出按钮
        drawImg(centerX - 100, centerY + 40, &res.img_quit);

        // 绘制高亮框
        setlinecolor(YELLOW);
        setlinestyle(PS_SOLID, 3);
        switch (pauseSelection) {
        case 0: // 继续
            rectangle(centerX - 105, centerY - 105, centerX + 105, centerY - 55);
            break;
        case 1: // 重新开始
            rectangle(centerX - 105, centerY - 35, centerX + 105, centerY + 15);
            break;
        case 2: // 退出
            rectangle(centerX - 105, centerY + 35, centerX + 105, centerY + 85);
            break;
        }

        // 提示文字
        settextcolor(LIGHTGRAY);
        settextstyle(20, 0, "楷体");
        outtextxy((screenWidth - textwidth("使用方向键选择，回车键确认")) / 2, screenHeight - 100, "使用方向键选择，回车键确认");
    }

    void drawGameOver() {
        putimage(0, 0, &res.img_bk);
        drawImg((screenWidth - res.img_gameover.getwidth()) / 2, 100, &res.img_gameover);

        char scoreStr[50];
        sprintf(scoreStr, "最终得分: %d", score);
        settextcolor(YELLOW);
        settextstyle(36, 0, "楷体");
        outtextxy((screenWidth - textwidth(scoreStr)) / 2, 400, scoreStr);

        settextcolor(LIGHTGREEN);
        settextstyle(24, 0, "楷体");
        outtextxy((screenWidth - textwidth("按 R 重新开始，按 ESC 退出")) / 2, 500, "按 R 重新开始，按 ESC 退出");
    }

    void drawWinScreen() {
        putimage(0, 0, &res.img_bk);
        drawImg((screenWidth - res.img_win.getwidth()) / 2, 100, &res.img_win);

        char scoreStr[50];
        sprintf(scoreStr, "最终得分: %d", score);
        settextcolor(YELLOW);
        settextstyle(36, 0, "楷体");
        outtextxy((screenWidth - textwidth(scoreStr)) / 2, 400, scoreStr);

        settextcolor(LIGHTGREEN);
        settextstyle(24, 0, "楷体");
        outtextxy((screenWidth - textwidth("恭喜通关！按 R 重新开始，按 ESC 退出")) / 2, 500, "恭喜通关！按 R 重新开始，按 ESC 退出");
    }

    void drawUI() {
        char scoreStr[20];
        sprintf(scoreStr, "SCORE: %d", score);
        settextcolor(YELLOW);
        settextstyle(24, 0, "Arial");
        outtextxy(20, 20, scoreStr);

        // 显示生命图标
        for (int i = 0; i < player.getLives(); i++) {
            drawImg(20 + i * 40, 60, &res.img_life);
        }

        // 显示血量
        char healthStr[30];
        sprintf(healthStr, "HP: %d/%d", player.getHealth(), player.getMaxHealth());
        outtextxy(20, 100, healthStr);

        // 显示剩余命数
        char livesStr[30];
        sprintf(livesStr, "剩余命数: %d", player.getLives());
        settextcolor(LIGHTGREEN);
        outtextxy(20, 140, livesStr);

        // 显示道具生成概率
        char probStr[40];
        sprintf(probStr, "道具概率: %d%%", itemSpawnProbability);
        settextcolor(LIGHTCYAN);
        outtextxy(20, 160, probStr);

        char levelStr[20];
        sprintf(levelStr, "LEVEL: %d", level);
        settextcolor(YELLOW);
        outtextxy(screenWidth - 150, 20, levelStr);

        char targetStr[30];
        sprintf(targetStr, "目标: %d/%d", score, WIN_SCORE);
        outtextxy(screenWidth - 150, 50, targetStr);

        if (player.getRebornTime() > 0) {
            char rebornStr[30];
            sprintf(rebornStr, "重生: %.1fs", player.getRebornTime() / 60.0f);
            settextcolor(LIGHTRED);
            settextstyle(24, 0, "Arial");
            outtextxy((screenWidth - textwidth(rebornStr)) / 2, screenHeight / 2, rebornStr);
        }

        if (player.getHealth() > 0) {
            settextcolor(WHITE);
            settextstyle(16, 0, "Arial");

            if (clearItemActive) {
                outtextxy(screenWidth - 150, 80, "清除道具在场");
            }

            if (fireRateItemActive) {
                outtextxy(screenWidth - 150, 100, "加速道具在场");
            }

            if (clearItemCooldown > 0) {
                char cooldownStr[40];
                sprintf(cooldownStr, "清除冷却: %.1fs", clearItemCooldown / 60.0f);
                outtextxy(screenWidth - 150, 120, cooldownStr);
            }
        }
    }

    void run() {
        int startTime = clock();

        handleInput();

        if (gameState == PLAYING) {
            if (Timer::check(1000 - min(800, level * 80), 2)) {
                createEnemy();
            }

            update();
        }

        BeginBatchDraw();
        {
            switch (gameState) {
            case START_SCREEN:
                drawStartScreen();
                break;
            case PLAYING:
                draw();
                break;
            case PAUSED:
                drawPauseScreen();
                break;
            case GAME_OVER:
                drawGameOver();
                break;
            case WIN_SCREEN:
                drawWinScreen();
                break;
            }
        }
        EndBatchDraw();

        int frameTime = clock() - startTime;
        if (1000 / 60 - frameTime > 0) {
            Sleep(1000 / 60 - frameTime);
        }
    }

    GameState getState() const { return gameState; }
};

int main() {
    const int screenWidth = 480;
    const int screenHeight = 850;

    initgraph(screenWidth, screenHeight);
    Game game(screenWidth, screenHeight);
    srand(static_cast<unsigned int>(time(NULL)));
    BeginBatchDraw();

    // 播放开始界面背景音乐
    game.playSound("Resource/sound/start_bgm.wav", true);

    while (true) {
        game.run();
    }
    EndBatchDraw();
    closegraph();
    return 0;
}