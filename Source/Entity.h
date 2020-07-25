#pragma once
#include "Math.h"
//#include "Rendering.h"
#include <unordered_map>
#include <vector>
#include <cassert>

struct Sprite;
struct Level;

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
    dummy,
    portal,
    spring,
    item,
    count
};

enum class ActorState
{
    none,
    idle,
    walk,
    run,
    jump,
    dead,
    count
};

struct Animation
{
    std::vector<Sprite*> anime;
    float fps = 10;
    ActorState type = ActorState::none;
    Animation* fallBack = nullptr;
};

struct AnimationList
{
    std::vector<Animation*> animations;
    float scaledWidth = 32;
    Rectangle_Int colRect = {};
    Vector colOffset = {};

    Animation* GetAnimation(ActorState state)
    {
        for (int32 i = 0; i < animations.size(); i++)
        {
            if (animations[i]->type == state)
                return animations[i];
		}
        return nullptr;
    }
    Animation* GetAnyValidAnimation()
    {
        for (int32 i = 0; i < animations.size(); i++)
        {
            if (animations[i]->anime.size() > 0)
                return animations[i];
		}
        return nullptr;
    }
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
    Vector position;
    Vector velocity = {};
    Vector terminalVelocity = { 10 , 300 };
    Vector acceleration;
    int32 jumpCount = 2;
    SDL_Color colorMod = White;
    float health = 100;
    float damage;
    bool lastInputWasLeft = false;
    bool inUse = true;
    bool grounded = true;
    float invinciblityTime = false;
    //ActorType actorType = ActorType::none;
    ActorState actorState = ActorState::none;

    Sprite* currentSprite = nullptr;
    AnimationList* animationList = {};
    int32 index = 0;
    float animationCountdown = 0;

    virtual void Update(float deltaTime) = 0;
    virtual void Render() = 0;
    virtual void UpdateHealth(Level& level, float deltaHealth) = 0;
    virtual ActorType GetActorType() = 0;
    float SpriteRatio()
    {
        assert(animationList->colRect.Width());
        return animationList->scaledWidth / animationList->colRect.Width();
    }
    float ScaledHeight()
    {
        return animationList->colRect.Height() * SpriteRatio();
    }
    float GameWidth()
    {
        return PixelToBlock((int)animationList->scaledWidth);// colRect.Width()* GoldenRatio();
    }
    float GameHeight()
    {
        return PixelToBlock((int)ScaledHeight());
    }
};

struct Enemy : public Actor
{
    Enemy(EnemyType type, ActorID* enemyID = nullptr);
    EnemyType enemyType;
    void Update(float deltaTime) override;
    void Render() override;
    void UpdateHealth(Level& level, float deltaHealth) override;
    ActorType GetActorType() override { return ActorType::enemy; }
};

struct Player : public Actor
{
    Player(ActorID* playerID = nullptr);
    void Update(float deltaTime) override;
    void Render() override;
    void UpdateHealth(Level& level, float deltaHealth) override;
    ActorType GetActorType() override { return ActorType::player; }
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
    void Render() override;
    void UpdateHealth(Level& level, float deltaHealth) override
    {

    };
    ActorType GetActorType() override { return ActorType::projectile; }
};

struct Dummy : public Actor
{
    Dummy(ActorID* dummyID = nullptr);
    void Update(float deltaTime) override;
    void Render() override;
    void UpdateHealth(Level& level, float deltaHealth) override
    {

    };
    ActorType GetActorType() override { return ActorType::dummy; }
};

struct Portal : public Actor
{
    Portal(int32 PID, const std::string& LP, int32 LPID);
    std::string levelPointer;
    int32 portalPointerID = 0;
    int32 portalID = 0;

    void Update(float deltaTime) override;
    void Render() override;
    void UpdateHealth(Level& level, float deltaHealth) override
    {

    };
    ActorType GetActorType() override { return ActorType::portal; }
};

struct Spring : public Actor
{
    Spring();
    Vector springVelocity = { 0.0f, 30.0f };


    void Update(float deltaTime) override;
    void Render() override;
    void UpdateHealth(Level& level, float deltaHealth) override
    {

    };
    ActorType GetActorType() override { return ActorType::spring; }
};

struct Item : public Actor
{

    Item(float healthChange);
    void Update(float deltaTime) override;
    void Render() override;
    void UpdateHealth(Level& level, float deltaHealth) override
    {

    };
    ActorType GetActorType() override { return ActorType::item; }
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

struct Level
{
    std::vector<Actor*> actors;
    TileMap blocks;
    std::string name;
    const char* filename;  //DefaultLevel.PNG;
    ActorID playerID;
};


enum class CollisionDirection {
    right,
    left,
    top,
    bottom
};


extern Projectile laser;
extern Level* currentLevel;
extern std::unordered_map<std::string, Level> levels;
extern std::unordered_map<std::string, AnimationList> actorAnimations;


TileType CheckColor(SDL_Color color);
SDL_Color GetTileMapColor(const Block& block);
void SaveLevel(Level* level, Player& player);
void LoadLevel(Level* level, Player& player);
void UpdateAllNeighbors(Block* block);
void SurroundBlockUpdate(Block* block, bool updateTop);
void ClickUpdate(Block* block, bool updateTop);
uint32 CollisionWithRect(Actor* actor, Rectangle rect);
void CollisionWithBlocks(Actor* actor, bool isEnemy);
uint32 CollisionWithActor(Player& player, Actor& enemy, Level& level);
//ActorID CreateActor(ActorType actorType, ActorType dummyType, Level& level);
ActorID CreatePlayer(Level& level);
ActorID CreateEnemy(Level& level);
ActorID CreateDummy(ActorType mimicType, Level& level);
Actor* FindActor(ActorID actorID, Level& level);
//returns first find of that type
Player* FindPlayer(Level& level);
void UpdateAnimationIndex(Actor* actor, float deltaTime);
void PlayAnimation(Actor* actor, ActorState state);
void UpdateActorHealth(Level& level, Actor* actor, float deltaHealth);
Portal* CreatePortal(int32 PortalID, const std::string& levelPointer, int32 levelPortalID, Level& level);
Portal* GetPortalsPointer(Portal* basePortal);
Actor* CreateBullet(Actor* player, Vector mouseLoc, TileType blockToBeType);
Spring* CreateSpring(Level& level);
void UpdateLaser(Actor* player, Vector mouseLoc, TileType paintType, float deltaTime);
void UpdateLocation(Actor* actor, float gravity, float deltaTime);
void UpdateEnemiesPosition(float gravity, float deltaTime);
void RenderActors();
void RenderActorHealthBars(Actor& actor);
//void InstantiateEachFrame(const std::string& fileName, const std::string& folderName);
void LoadAllAnimationStates(const std::string& entity);
void AttachAnimation(Actor* actor, ActorType overrideType = ActorType::none);
