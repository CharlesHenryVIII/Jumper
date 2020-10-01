#pragma once
#include "Math.h"
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
    None,
    Player,
    Enemy,
    Projectile,
    Dummy,
    Portal,
    Spring,
    MovingPlatform,
    Grapple,
    Item,
    Count
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

struct AnimationData {
	const char* name = nullptr;
    float animationFPS[int(ActorState::count)] = {};
	Vector collisionOffset = { 0.125f, 0.25f };
	Rectangle collisionRectangle = {};
	float scaledWidth = 32;
};

struct AnimationList
{
    std::vector<Animation*> animations;
    float scaledWidth = 32;
    Rectangle_Int colRect = {};
    Vector colOffset = {};

    Animation* GetAnimation(ActorState state) const
    {
        for (int32 i = 0; i < animations.size(); i++)
        {
            if (animations[i]->type == state)
                return animations[i];
		}
        return nullptr;
    }
    Animation* GetAnyValidAnimation() const
    {
        for (int32 i = 0; i < animations.size(); i++)
        {
            if (animations[i]->anime.size() > 0)
                return animations[i];
		}
        return nullptr;
    }
};



inline int32 BlockToPixel(float loc)
{
    return int32(loc * blockSize);
}

inline VectorInt BlockToPixel(Vector loc)
{
    return { BlockToPixel(loc.x), BlockToPixel(loc.y) };
}


inline float PixelToBlock(int32 loc)
{
    return { float(loc) / blockSize };
}

inline Vector PixelToBlock(VectorInt loc)
{
    return { PixelToBlock(loc.x), PixelToBlock(loc.y) };
}

struct Player;
enum class TileType {
    invalid,
    filled,
    moving,
};
//asssert on functions
struct DummyInfo {

	ActorType mimicType = ActorType::None;
};

struct ProjectileInfo {

    Player* player = nullptr;
    Vector mouseLoc = {};
    TileType blockToBeType = TileType::invalid;
};

struct PortalInfo {

	int32 PortalID = 0;
	std::string levelName;
	int32 levelPortalID = 0;
};


typedef uint32 ActorID;
struct Actor
{
private:
    static ActorID lastID;

public:

    Actor() : id(++lastID)
    {
    }

    ActorID id;
    Vector position = {};
    Vector velocity = {};
    Vector terminalVelocity = { 10 , 300 };
    Vector acceleration = {};
    Vector deltaPosition = {};
    int32 jumpCount = 2;
    Color colorMod = White;
    float health = 100;
    float damage = 0;
    bool lastInputWasLeft = false;
    bool inUse = true;
    bool grounded = true;
    float invinciblityTime = false;
    ActorState actorState = ActorState::none;

    ActorID parent = 0;

    Sprite* currentSprite = nullptr;
    const AnimationList* animationList = {};
    Level* level = {};
    int32 index = 0;
    float animationCountdown = 0;

    virtual Actor* Copy() = 0;
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
        return PixelToBlock((int)animationList->scaledWidth);
    }
    float GameHeight()
    {
        return PixelToBlock((int)ScaledHeight());
    }
};


#define ACTOR_TYPE(typename__)                                             \
    static const ActorType s_type = ActorType:: typename__;                \
    ActorType GetActorType() override { return ActorType:: typename__; }   \
    Actor* Copy() override{                                                \
        typename__* actor = new typename__();                              \
        ActorID tempID = actor->id;                                        \
        *actor = *this;                                                    \
        actor->id = tempID;                                                \
        return actor;                                                      \
    }



struct Enemy : public Actor
{
    ACTOR_TYPE(Enemy);

    EnemyType enemyType;
    void OnInit();
    void Update(float deltaTime) override;
    void Render() override;
    void UpdateHealth(Level& level, float deltaHealth) override;
};

struct Player : public Actor
{
    ACTOR_TYPE(Player);

    bool grappleEnabled = false;
    bool grappleReady = true;
    bool grappleDeployed = false;
    void OnInit();
    void Update(float deltaTime) override;
    void Render() override;
    void UpdateHealth(Level& level, float deltaHealth) override;
};



struct Projectile : public Actor
{
    ACTOR_TYPE(Projectile);

    Vector destination;
    TileType paintType;
    float rotation = 0;

    void OnInit(const ProjectileInfo& info);
    void Update(float deltaTime) override;
    void Render() override;
    void UpdateHealth(Level& level, float deltaHealth) override {};
};

struct Dummy : public Actor
{
    ACTOR_TYPE(Dummy);

    void OnInit(const DummyInfo& info);
    void Update(float deltaTime) override;
    void Render() override;
    void UpdateHealth(Level& level, float deltaHealth) override {};
};

struct Portal : public Actor
{
    ACTOR_TYPE(Portal);
    std::string levelPointer;
    int32 portalPointerID = 0;
    int32 portalID = 0;

    void OnInit(const PortalInfo& info);
    void Update(float deltaTime) override;
    void Render() override;
    void UpdateHealth(Level& level, float deltaHealth) override {};
};

struct Spring : public Actor
{
    ACTOR_TYPE(Spring);

    Vector springVelocity = { 0.0f, 30.0f };

    void OnInit();
    void Update(float deltaTime) override;
    void Render() override;
    void UpdateHealth(Level& level, float deltaHealth) override {};
};

//NOTE: moving platform must have atleast 2 locations, the first being the original location
struct MovingPlatform : public Actor
{
    ACTOR_TYPE(MovingPlatform);

    std::vector<Vector> locations;
    Vector dest;
    int32 nextLocationIndex;
    float speed = 2;
    bool incrimentPositivePathIndex;
	std::vector<ActorID> childList;

    void OnInit();
    void Update(float deltaTime) override;
    void Render() override;
    void UpdateHealth(Level& level, float deltaHealth) override {};
};

struct Grapple : public Actor
{
    ACTOR_TYPE(Grapple);
    Vector destination;
    float rotation = 0;
    ActorID attachedActor = 0;

    void OnInit();
    void Update(float deltaTime) override;
    void Render() override;
    void UpdateHealth(Level& level, float deltaHealth) override {};
};

struct Item : public Actor
{
    ACTOR_TYPE(Item);

    void OnInit();
    void Update(float deltaTime) override;
    void Render() override;
    void UpdateHealth(Level& level, float deltaHealth) override {};
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
    const std::unordered_map<uint64, Block>* BlockList();
    void ClearBlocks();
    void CleanBlocks();
};


extern Projectile laser;
extern std::unordered_map<std::string, AnimationList> actorAnimations;

struct Level
{
private:

	Actor* FindActorInternal(ActorID actorID)
	{
		for (auto& actor : actors)
		{
			if (actor->id == actorID)
				return actor;
		}
		return nullptr;
	}

public:

    std::string name;
    std::vector<Actor*> actors;
    std::vector<ActorID> movingPlatforms;
    TileMap blocks;
    ActorID playerID;

    struct NullInfo {};

    template <typename Type, typename Info = NullInfo>
    Type* CreateActor(const Info& thing = {})
    {
        Type* t = new Type();
        actors.push_back(t);
        t->level = this;
        if constexpr(std::is_same_v<Info, NullInfo>)
			t->OnInit();
        else
			t->OnInit(thing);

        return t;
    }

	template <typename T>
	T* FindActor(ActorID id)
	{
        T* a = (T*)FindActorInternal(id);
		if (a && a->GetActorType() == T::s_type)
			return static_cast<T*>(a);
		return nullptr;
	}

    Actor* FindActorGeneric(ActorID id)
    {
		if (Actor* a = FindActorInternal(id))
			return a;
		return nullptr;
    }

	//returns first find of that type
	Player* FindPlayer()
	{
		for (Actor* actor : actors)
		{
			if (actor->GetActorType() == ActorType::Player)
				return (Player*)actor;
		}
		return nullptr;
	}

    void AddActor(Actor* actor)
    {
        actors.push_back(actor);
        actor->level = this;
    }
};

enum class CollisionDirection {
    right,
    left,
    top,
    bottom
};




Player* FindPlayerGlobal();
TileType CheckColor(SDL_Color color);
Color GetTileMapColor(const Block& block);
//void SaveLevel(Level* level, Player& player);
//void LoadLevel(Level* level, Player& player);
void UpdateAllNeighbors(Block* block, Level* level);
void SurroundBlockUpdate(Block* block, bool updateTop, Level* level);
void ClickUpdate(Block* block, bool updateTop, Level* level);
uint32 CollisionWithRect(Actor* actor, Rectangle rect);
void CollisionWithBlocks(Actor* actor, bool isEnemy);
uint32 CollisionWithActor(Player& player, Actor& enemy, Level& level);
void UpdateAnimationIndex(Actor* actor, float deltaTime);
void PlayAnimation(Actor* actor, ActorState state);
void UpdateActorHealth(Level& level, Actor* actor, float deltaHealth);
Portal* GetPortalsPointer(Portal* basePortal);
void UpdateLaser(Actor* player, Vector mouseLoc, TileType paintType, float deltaTime);
void UpdateLocation(Actor* actor, float gravity, float deltaTime);
void UpdateEnemiesPosition(std::vector<Actor*>* actors, float gravity, float deltaTime);
void RenderActors(std::vector<Actor*>* actors);
void RenderActorHealthBars(Actor& actor);
void LoadAllAnimationStates();
void LoadAnimationStates(std::vector<AnimationData>* animationData);
void AttachAnimation(Actor* actor, ActorType overrideType = ActorType::None);

