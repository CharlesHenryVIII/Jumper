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

struct Actor
{
    Sprite* sprite;
    Vector position;
    Vector velocity = {};
    Vector acceleration;
    Vector colOffset;
    int jumpCount = 2;
    float health = 100;
    float damage;
    bool inUse = true;
    float invincible = false;
};

struct Enemy : public Actor
{
    EnemyType enemyType;
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
void SaveLevel(Level* level);
void LoadLevel(Level* level);
void UpdateAllNeighbors(Block* block);
void SurroundBlockUpdate(Block* block, bool updateTop);
void ClickUpdate(Block* block, bool updateTop);
Rectangle CollisionXOffsetToRectangle(Actor* actor);
Rectangle CollisionYOffsetToRectangle(Actor* actor);
uint32 CollisionWithRect(Actor* actor, Rectangle rect);
void CollisionWithBlocks(Actor* actor, bool isEnemy);
bool CollisionWithEnemy(Actor* actor, float currentTime);
void UpdateActorHealth(Actor* actor, float currentTime);
void UpdateEnemyHealth(float totalTime);
void CreateBullet(Actor* player, Sprite* bulletSprite, Vector mouseLoc, TileType blockToBeType);
void CreateLaser(Actor* player, Sprite* sprite, Vector mouseLoc, TileType paintType);
void UpdateLocation(Actor* actor, float gravity, float deltaTime);
void UpdateBullets(float deltaTime);
void UpdateEnemiesPosition(float gravity, float deltaTime);
void RenderBullets();
void RenderEnemies();


extern Actor player;
extern TileMap tileMap;
extern Projectile laser;
extern std::vector<Projectile> bulletList;
extern Level currentLevel;