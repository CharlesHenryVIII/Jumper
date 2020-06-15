#pragma once
#include "Math.h"

#include <unordered_map>
#include <vector>

struct Sprite;


enum class EnemyType
{
    head,
    melee,
    ranged,
    magic
};

enum class ActorType
{
    none,
    player,
    enemy,
    projectile,
};

enum class ActorState
{
    none,
    idle,
    walk,
    run,
    jump,
    dead
};

struct Animation
{
    std::vector<Sprite*> anime;
    int32 index = 0;
    double lastAnimationTime = 0;
    double fps = 10;
};

inline int BlockToPixel(float loc)
{
    return int(loc * blockSize);
}

inline VectorInt BlockToPixel(Vector loc)
{
    return { BlockToPixel(loc.x), BlockToPixel(loc.y) };
}


inline float PixelToBlock(int loc)
{
    return { float(loc) / blockSize };
}

inline Vector PixelToBlock(VectorInt loc)
{
    return { PixelToBlock(loc.x), PixelToBlock(loc.y) };
}

typedef uint32 ActorID;

struct Actor
{
private:
    static ActorID lastID;

public:

    Actor() : id(++lastID)
    {
    }

    const ActorID id;
    //Sprite* sprite;
    Vector position;
    Vector velocity = {};
    Vector terminalVelocity = { 10 , 300 };
    Vector acceleration;
    Rectangle_Int colRect;
    Vector colOffset;
    int32 jumpCount = 2;
    float health = 100;
    float damage;
    double lastAnimationTime;
    double fps = 10;
    float scaledWidth = 32;
    bool lastInputWasLeft = false;
    bool inUse = true;
    double invinciblityTime = false;

    ActorState actorState = ActorState::idle;

    Animation death = {};
    Animation idle = {};
    Animation run = {};
    Animation walk = {};
    Animation jump = {};
    //std::vector<Sprite*> deathAnime;
    //int32 deathIndex = 0;
    //std::vector<Sprite*> idleAnime;
    //int32 idleIndex = 0;
    //std::vector<Sprite*> runAnime;
    //int32 runIndex = 0;
    //std::vector<Sprite*> walkAnime;
    //int32 walkIndex = 0;
    //std::vector<Sprite*> jumpAnime;
    //int32 jumpIndex = 0;

    virtual void Update(float deltaTime) = 0;
    virtual void Render(double totalTime) = 0;
    virtual void UpdateHealth(double totalTime) = 0;
    virtual ActorType GetActorType() = 0;
    float SpriteRatio()
    {
        return scaledWidth / colRect.Width();
    }
    float ScaledWidth()
    {
        return scaledWidth;// colRect.Width()* GoldenRatio();
    }
    float ScaledHeight()
    {
        return colRect.Height() * SpriteRatio();
    }
    float GameWidth()
    {
        return PixelToBlock((int)ScaledWidth());// colRect.Width()* GoldenRatio();
    }
    float GameHeight()
    {
        return PixelToBlock((int)ScaledHeight());
    }
};

struct Enemy : public Actor
{
    EnemyType enemyType;
    void Update(float deltaTime) override;
    void Render(double totalTime) override;
    void UpdateHealth(double totalTime) override;
    ActorType GetActorType() override;
};

struct Player : public Actor
{
    void Update(float deltaTime) override;
    void Render(double totalTime) override;
    void UpdateHealth(double totalTime) override;
    ActorType GetActorType() override;
};

enum class TileType {
    invalid,
    filled
};


struct Projectile : public Actor
{
    Vector destination;
    TileType paintType;
    float rotation = 0;

    void Update(float deltaTime) override;
    void Render(double totalTime) override;
    void UpdateHealth(double totalTime) override
    {

    }
    ActorType GetActorType() override;
};


struct Block {
    Vector location = {};
    TileType tileType = TileType::invalid;
    uint32 flags = {};

    Vector PixelPosition() const
    {
        return { location.x * blockSize, location.y * blockSize };
    }
};

struct Level
{
    std::vector<Enemy> enemyList;

    const char* filename;  //DefaultLevel.PNG;
    Block entrance;
    Block exit;
};


class TileMap
{
    std::unordered_map<uint64, Block> blocks;

public:
    uint64 HashingFunction(Vector loc);

    //Block Coordinate System
    Block& GetBlock(Vector loc);
    Block* TryGetBlock(Vector loc);
    void AddBlock(Vector loc, TileType tileType);
    bool CheckForBlock(Vector loc);
    void UpdateBlock(Block* block);
    void UpdateAllBlocks();
    
    //returns &blocks
    const std::unordered_map<uint64, Block>* blockList();
    void ClearBlocks();
    void CleanBlocks();
};


enum class CollisionDirection {
    right,
    left,
    top,
    bottom
};

TileType CheckColor(SDL_Color color);
SDL_Color GetTileMapColor(const Block& block);
void SaveLevel(Level* level, Player& player);
void LoadLevel(Level* level, Player& player);
void UpdateAllNeighbors(Block* block);
void SurroundBlockUpdate(Block* block, bool updateTop);
void ClickUpdate(Block* block, bool updateTop);
Rectangle CollisionXOffsetToRectangle(Actor* actor);
Rectangle CollisionYOffsetToRectangle(Actor* actor);
uint32 CollisionWithRect(Actor* actor, Rectangle rect);
void CollisionWithBlocks(Actor* actor, bool isEnemy);
bool CollisionWithEnemy(Player& player, Actor& enemy, float currentTime);
void UpdateActorHealth(Actor* actor, double currentTime);
void UpdateEnemyHealth(float totalTime);
Actor* CreateBullet(Actor* player, Sprite* bulletSprite, Vector mouseLoc, TileType blockToBeType);
void CreateLaser(Actor* player, Sprite* sprite, Vector mouseLoc, TileType paintType);
void UpdateLocation(Actor* actor, float gravity, float deltaTime);
void UpdateActors(float deltaTime);
void UpdateEnemiesPosition(float gravity, float deltaTime);
void RenderActors(double totalTime);
void RenderEnemies();
void RenderActorHealthBars(Actor& actor);



extern TileMap tileMap;
extern Projectile laser;
extern std::vector<Actor*> actorList;
extern Level currentLevel;