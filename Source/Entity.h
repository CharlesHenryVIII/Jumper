#pragma once
#include "Math.h"
#include "Audio.h"
#include "ParticleSystem.h"

#include <string>
#include <unordered_map>
#include <vector>
#include <cassert>

struct Sprite;
struct Level;
typedef uint32 ActorID;


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
    AudioPlayer,
    ParticleGen,
    Item,
    Count
};

enum class ActorState
{
    None,
    Idle,
    Walk,
    Run,
    Jump,
    Dead,
    Count
};

enum class GrappleState
{
    None,
    Sending,
    Attached,
    Retracting,
};

struct Animation
{
    std::vector<Sprite*> anime;
    float fps = 10;
    ActorState type = ActorState::None;
    Animation* fallBack = nullptr;
};

struct AnimationData {
	const char* name = nullptr;
    float animationFPS[int(ActorState::Count)] = {};
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


inline float PixelToGame(int32 loc)
{
    return { static_cast<float>(loc) / blockSize };
}

inline Vector PixelToGame(VectorInt loc)
{
    return { PixelToGame(loc.x), PixelToGame(loc.y) };
}

enum class TileType {
    invalid,
    filled,
    moving,
};
//asssert on functions
struct DummyInfo {

	ActorType mimicType = ActorType::None;
};

struct Player;
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

struct GrappleInfo {

    ActorID actorID = {};
    Vector mouseLoc = {};
};

struct Actor
{
private:
    static ActorID lastID;
    bool locationUpdated = false;

public:

    Actor() : id(++lastID)
    {
    }

    ActorID id;
    Vector position = {};
    Vector velocity = {};
    Vector acceleration = {};
    Vector terminalVelocity = { 10 , 300 };
    Vector deltaPosition = {};
    float rotation = 0;
    Color colorMod = White;
    float health = 100;
    float damage = 0;
    float invinciblityTime = 0;
    int32 jumpCount = 2;
    bool lastInputWasLeft = false;
    bool inUse = true;
    bool grounded = true;
    bool allowRenderFlip = true;
    bool angularUpdate = false;
    bool switchToLinearUpdate = false;
    ActorState actorState = ActorState::None;

    ActorID parent = 0;

    //std::unordered_map<std::string, ParticleGenID> particleGenerators;

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
    float PixelToGameRatio()
    {
        assert(animationList->colRect.Width());
        return animationList->scaledWidth / animationList->colRect.Width();
        //return 1.0f;
    }
    float GameWidth()
    {
        return PixelToGame((int)animationList->scaledWidth);
    }
    float GameHeight()
    {
        return PixelToGame((int)(animationList->colRect.Height() * PixelToGameRatio()));
    }
	inline void ResetJumpCount()
	{
		jumpCount = 2;
	}
    Vector Center()
    {
        return { position.x + GameWidth() / 2.0f, position.y + GameHeight() / 2.0f };
    }
    gbMat4 GetWorldMatrix();
    Vector GetWorldPosition();

	//Vector GetWorldVelocity()
	//{
	//    Vector result = velocity;

	//    if (parent)
	//        result = level->FindActor<Actor>(parent)->GetWorldVelocity() * result;
	//    return result;
	//}

    void UpdateLocation(float gravity, float deltaTime);
    void PlayAnimation(ActorState state);
    void AttachAnimation(ActorType overrideType = ActorType::None);
    Rectangle CollisionXOffsetToRectangle();
    Rectangle CollisionYOffsetToRectangle();
    uint32 CollisionWithRect(Rectangle rect);
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
    float timeToMakeSound = 0;
    ParticleGenID dustGenerator;
    ParticleParams pp;


    void OnInit();
    void Update(float deltaTime) override;
    void Render() override;
    void UpdateHealth(Level& level, float deltaHealth) override;
};

struct Player : public Actor
{
    ACTOR_TYPE(Player);

	float spawnRadius = 0.5f; //half a block
    float grappleRange = 15.0f;
    bool grappleEnabled = false;
    bool grappleReady = true;
    ActorID grapple = 0;
    ParticleGenID dustGenerator;

    ParticleParams pp;

    float timeToMakeSound = 0;

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
    //Grapple needs to attach to the left clicked position,
    //Allow the player to get closer but not fartherfrom the attached location,
    //then slowely real the actor into the location,
    //stop and delete if the player releases the mouse 1 button.
    ActorID attachedActor = 0;
    GrappleState grappleState = GrappleState::None;
	float grappleSpeed = 40.0f;
    float grappleDistance;
    float angularVelocity = 0;
    Vector shotOrigin = {};
    void OnInit(const GrappleInfo& info);
    void Update(float deltaTime) override;
    void Render() override;
    void UpdateHealth(Level& level, float deltaHealth) override {};
};

struct AudioPlayer : public Actor
{
    ACTOR_TYPE(AudioPlayer);

    AudioID audioID = 0;
    AudioParams audioParams;
    void OnInit(const AudioParams info);
    void Update(float deltaTime) override;
    void Render() override;
    void UpdateHealth(Level& level, float deltaHealth) override {};
};

struct ParticleGen : public Actor
{
    ACTOR_TYPE(ParticleGen);

	Color colorRangeLo;
	Color colorRangeHi;

	Vector coneDir;
	float coneDeg;
	float particleSpeed;

	float lifeTime;
	float particlesPerSecond;
	float particleSize;
	Vector terminalVelocity;
	float timeLeftToSpawn;
    bool playing;

	//float fadeInTime;
	//float fadeOutTime;

    void OnInit(const ParticleParams info);
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
extern std::unordered_map<std::string, AnimationList> s_actorAnimations;

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
void CollisionWithBlocks(Actor* actor, bool isEnemy);
uint32 CollisionWithActor(Player& player, Actor& enemy, Level& level);
void UpdateAnimationIndex(Actor* actor, float deltaTime);
void UpdateActorHealth(Level& level, Actor* actor, float deltaHealth);
Portal* GetPortalsPointer(Portal* basePortal);
void UpdateLaser(Actor* player, Vector mouseLoc, TileType paintType, float deltaTime);
//void UpdateLocation(Actor* actor, float gravity, float deltaTime);
void UpdateEnemiesPosition(std::vector<Actor*>* actors, float gravity, float deltaTime);
void RenderActors(std::vector<Actor*>* actors);
void RenderActorHealthBars(Actor& actor);
//void LoadAllAnimationStates();
void LoadAnimationStates();

