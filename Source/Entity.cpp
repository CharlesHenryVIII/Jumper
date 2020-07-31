#include "Entity.h"
#include "Rendering.h"
#include "Math.h"
#include "stb/stb_image.h"
#include "stb/stb_image_write.h"
#include "TiledInterop.h"
#include "WinUtilities.h"
#include "Misc.h"

#include <cassert>


ActorID Actor::lastID = 0;

//TileMap tileMap;
Projectile laser = {};
//std::vector<Actor*> actorList= {};
std::unordered_map<std::string, Level> levels;
Level* currentLevel = nullptr;
std::unordered_map<std::string, AnimationList> actorAnimations;
//static const Vector terminalVelocity = { 10 , 300 };

/*********************
 *
 * Player
 *
 ********/

Player::Player(ActorID* playerID)
{
	damage = 100;
	inUse = true;
	AttachAnimation(this);
	*playerID = id;
}

void Player::Update(float deltaTime)
{
	UpdateLocation(this, -60.0f, deltaTime);
	CollisionWithBlocks(this, false);
	invinciblityTime = Max(invinciblityTime - deltaTime, 0.0f);
	if (grounded == true)
	{
		if (velocity.x != 0)
			PlayAnimation(this, ActorState::run);
		else
			PlayAnimation(this, ActorState::idle);
	}
	UpdateAnimationIndex(this, deltaTime);
}

void Player::Render()
{
	RenderActor(this, 0);
	RenderActorHealthBars(*this);
}

void Player::UpdateHealth(Level& level, float deltaHealth)
{
	UpdateActorHealth(level, this, deltaHealth);
}


/*********************
 *
 * Enemy
 *
 ********/

Enemy::Enemy(EnemyType type, ActorID* enemyID)
{
	position = { 28, 1 };
	velocity.x = 4;
	damage = 25;
	inUse = true;
	enemyType = type;
	AttachAnimation(this);
	*enemyID = id;
}

void Enemy::Update(float deltaTime)
{
	UpdateLocation(this, -60.0f, deltaTime);
	CollisionWithBlocks(this, true);
	invinciblityTime = Max(invinciblityTime - deltaTime, 0.0f);
	if (grounded == true)
	{
		if (velocity.x != 0)
			PlayAnimation(this, ActorState::run);
		else
			PlayAnimation(this, ActorState::idle);
	}
	UpdateAnimationIndex(this, deltaTime);
}

void Enemy::Render()
{
	RenderActor(this, 0);
	RenderActorHealthBars(*this);
}

void Enemy::UpdateHealth(Level& level, float deltaHealth)
{
	UpdateActorHealth(level, this, deltaHealth);
}

/*********************
 *
 * Projectile
 *
 ********/

void Projectile::Update(float deltaTime)
{
	UpdateLocation(this, 0, deltaTime);
	float rad = DegToRad(rotation);
	Vector realPosition = { position.x + cosf(rad)*GameWidth(), position.y - sinf(rad) * GameWidth() };
	if (DotProduct(velocity, destination - realPosition) < 0)
	{
		//paint block and remove bullet
		Block* blockPointer = &currentLevel->blocks.GetBlock(destination);
		blockPointer->tileType = paintType;
		UpdateAllNeighbors(blockPointer);
		inUse = false;
	}
	UpdateAnimationIndex(this, deltaTime);
}

void Projectile::Render()
{
	if (paintType == TileType::filled)
		colorMod = Green;
	else
		colorMod = Red;
	RenderActor(this, rotation);
}


/*********************
 *
 * Dummy
 *
 ********/

Dummy::Dummy(ActorID* dummyID)
{
	*dummyID = id;
	inUse = true;
	health = 0;
}

void Dummy::Update(float deltaTime)
{
	UpdateLocation(this, -60.0f, deltaTime);
	CollisionWithBlocks(this, false);
	UpdateAnimationIndex(this, deltaTime);
}

void Dummy::Render()
{
	RenderActor(this, 0);
}


/*********************
 *
 * Portal
 *
 ********/

Portal::Portal(int32 PID, const std::string& LP, int32 LPID)
{
	inUse = true;
    levelPointer = LP;
    portalPointerID = LPID;
    portalID = PID;
	damage = 0;
}

void Portal::Update(float deltaTime)
{
	UpdateAnimationIndex(this, deltaTime);
}
void Portal::Render()
{
	RenderActor(this, 0);
}


/*********************
 *
 * Spring
 *
 ********/

Spring::Spring()
{
	inUse = true;
	damage = 0;
}

void Spring::Update(float deltaTime)
{
	UpdateAnimationIndex(this, deltaTime);
}
void Spring::Render()
{
	RenderActor(this, 0);
}


/*********************
 *
 * MovingPlatform
 *
 ********/

MovingPlatform::MovingPlatform()
{
	nextLocationIndex = 1;
	incrimentPositivePathIndex = true;
	acceleration = { 0, 0 };
	inUse = true;
	damage = 0;
}

void MovingPlatform::Update(float deltaTime)
{
	UpdateLocation(this, 0, deltaTime);

	if (DotProduct(velocity, dest - position) <= 0)
	{
		int32 incrimentationAmount;
		if (incrimentPositivePathIndex)
			incrimentationAmount = 1;
		else
			incrimentationAmount = -1;

		if (nextLocationIndex + incrimentationAmount < (int32)locations.size() && nextLocationIndex + incrimentationAmount >= 0)
			nextLocationIndex += incrimentationAmount;
		else
		{

			incrimentPositivePathIndex = !incrimentPositivePathIndex;
			nextLocationIndex -= incrimentationAmount;
		}

		dest = locations[nextLocationIndex];
		Vector directionVector = Normalize(dest - position);
		velocity = directionVector * speed;
	}
	UpdateAnimationIndex(this, deltaTime);
}
void MovingPlatform::Render()
{
	RenderActor(this, 0);
}

/*********************
 *
 * Items
 *
 ********/

Item::Item(float healthChange)
{
	inUse = true;
	damage = -healthChange;
}

void Item::Update(float deltaTime)
{
	UpdateAnimationIndex(this, deltaTime);
}
void Item::Render()
{
	RenderActor(this, 0);
}




/*********************
 *
 * TileMap
 *
 ********/


uint64 TileMap::HashingFunction(Vector loc)
{
	int32 x = int32(floorf(loc.x));
	int32 y = int32(floorf(loc.y));

	uint64 hash = uint64(y) << 32;
	hash |= (uint64(x) & 0xffffffff);
	return hash;
}

//Block Coordinate System
Block& TileMap::GetBlock(Vector loc)
{
	loc.x = floorf(loc.x);
	loc.y = floorf(loc.y);
	auto& result = blocks[HashingFunction(loc)];
	result.location = loc;
	return result;
}
Block* TileMap::TryGetBlock(Vector loc)
{
	if (CheckForBlock(loc))
		return &GetBlock(loc);
	else
		return nullptr;
}
void TileMap::AddBlock(Vector loc, TileType tileType)
{
	blocks[HashingFunction(loc)] = { loc, tileType };
}
bool TileMap::CheckForBlock(Vector loc)
{
	auto it = blocks.find(HashingFunction(loc));
	if (it != blocks.end())
		return it->second.tileType != TileType::invalid;
	return false;
}
void TileMap::UpdateBlock(Block* block)
{//32 pixels per block 64 total 8x8 = 256x256
	uint32 blockFlags = 0;

	constexpr uint32 left = 1 << 0;     //1 or odd
	constexpr uint32 botLeft = 1 << 1;  //2
	constexpr uint32 bot = 1 << 2;      //4
	constexpr uint32 botRight = 1 << 3; //8
	constexpr uint32 right = 1 << 4;    //16
	constexpr uint32 top = 1 << 5;      //32


	if (CheckForBlock({ block->location.x - 1, block->location.y }))
		blockFlags += left;
	if (CheckForBlock({ block->location.x - 1, block->location.y - 1 }))
		blockFlags += botLeft;
	if (CheckForBlock({ block->location.x,     block->location.y - 1 }))
		blockFlags += bot;
	if (CheckForBlock({ block->location.x + 1, block->location.y - 1 }))
		blockFlags += botRight;
	if (CheckForBlock({ block->location.x + 1, block->location.y }))
		blockFlags += right;
	if (CheckForBlock({ block->location.x,     block->location.y + 1 }))
		blockFlags += top;

	block->flags = blockFlags;
}
void TileMap::UpdateAllBlocks()
{
	for (auto& block : blocks)
	{
		UpdateBlock(&GetBlock(block.second.location));
	}
}

//returns &blocks
const std::unordered_map<uint64, Block>* TileMap::blockList()
{
	return &blocks;
}
void TileMap::ClearBlocks()
{
	blocks.clear();
}
void TileMap::CleanBlocks()
{
	for (auto it = blocks.begin(); it != blocks.end();)
	{
		if (it->second.tileType == TileType::invalid)
			it = blocks.erase(it);
		else
			it++;
	}
}


TileType CheckColor(SDL_Color color)
{
	if (color.r == Brown.r && color.g == Brown.g && color.b == Brown.b && color.a == Brown.a)
		return TileType::filled;
	else
		return TileType::invalid;
}


SDL_Color GetTileMapColor(const Block& block)
{
	if (block.tileType == TileType::filled)
		return Brown;
	else
		return {};

}


void SaveLevel(Level* level, Player &player)
{
	//Setup
	stbi_flip_vertically_on_write(true);
	currentLevel->blocks.CleanBlocks();

	//Finding the edges of the "picture"/level
	int32 left = int32(player.position.x);
	int32 right = int32(player.position.x);
	int32 top = int32(player.position.y);
	int32 bot = int32(player.position.y);

	for (auto& block : *currentLevel->blocks.blockList())
	{
		if (block.second.location.x < left)
			left = int32(block.second.location.x);
		if (block.second.location.x > right)
			right = int32(block.second.location.x);

		if (block.second.location.y < bot)
			bot = int32(block.second.location.y);
		if (block.second.location.y > top)
			top = int32(block.second.location.y);
	}

	int32 width = right - left + 1;
	int32 height = top - bot + 1;

	//Getting memory to write to
	std::vector<SDL_Color> memBuff;
	memBuff.resize(width * height, {});

	//writing to memory
	SDL_Color tileColor = {};
	for (auto& block : *currentLevel->blocks.blockList())
	{
		if (block.second.tileType == TileType::invalid)
			continue;

		memBuff[size_t((block.second.location.x - left) + ((block.second.location.y - bot) * size_t(width)))] = GetTileMapColor(block.second);
	}

	//TODO(choman):  When new entity system is done change this to include other Actors
	int32 playerBlockPositionX = int32(player.position.x);
	int32 playerBlockPositionY = int32(player.position.y);
	memBuff[size_t((playerBlockPositionX - left) + ((playerBlockPositionY - bot) * size_t(width)))] = Blue;

	//Saving written memory to file
	int stride_in_bytes = 4 * width;
	int colorChannels = 4;
	stbi_write_png(level->filename, width, height, colorChannels, memBuff.data(), stride_in_bytes);
}


void LoadLevel(Level* level, Player& player)
{
	currentLevel->blocks.ClearBlocks();
	stbi_set_flip_vertically_on_load(true);

	int32 textureHeight, textureWidth, colorChannels;
	SDL_Color* image = (SDL_Color*)stbi_load(level->filename, &textureWidth, &textureHeight, &colorChannels, STBI_rgb_alpha);

	for (int32 y = 0; y < textureHeight; y++)
	{
		for (int32 x = 0; x < textureWidth; x++)
		{
			uint32 index = size_t(x) + size_t(y * textureWidth);

			TileType tileColor = CheckColor(image[index]);
			if (image[index].r == Blue.r && image[index].g == Blue.g && image[index].b == Blue.b && image[index].a == Blue.a)
				player.position = { float(x + 0.5f), float(y + 0.5f) };
			else if (tileColor != TileType::invalid)
				currentLevel->blocks.AddBlock({ float(x), float(y) }, tileColor);
		}
	}
	currentLevel->blocks.UpdateAllBlocks();
	stbi_image_free(image);
}


void UpdateAllNeighbors(Block* block)
{
	for (int32 y = -1; y <= 1; y++)
	{
		for (int32 x = -1; x <= 1; x++)
		{
			if (currentLevel->blocks.TryGetBlock({ float(block->location.x + x), float(block->location.y + y) }) != nullptr)
				currentLevel->blocks.UpdateBlock(&currentLevel->blocks.GetBlock({ float(block->location.x + x), float(block->location.y + y) }));
		}
	}
}


void SurroundBlockUpdate(Block* block, bool updateTop)
{
	//update block below
	if (currentLevel->blocks.CheckForBlock({ block->location.x, block->location.y - 1 }))
		currentLevel->blocks.UpdateBlock(&currentLevel->blocks.GetBlock({ block->location.x, block->location.y - 1 }));
	//update block to the left
	if (currentLevel->blocks.CheckForBlock({ block->location.x - 1, block->location.y }))
		currentLevel->blocks.UpdateBlock(&currentLevel->blocks.GetBlock({ block->location.x - 1, block->location.y }));
	//update block to the right
	if (currentLevel->blocks.CheckForBlock({ block->location.x + 1, block->location.y }))
		currentLevel->blocks.UpdateBlock(&currentLevel->blocks.GetBlock({ block->location.x + 1, block->location.y }));
	if (updateTop && currentLevel->blocks.CheckForBlock({ block->location.x, block->location.y - 1 }))
		currentLevel->blocks.UpdateBlock(&currentLevel->blocks.GetBlock({ block->location.x, block->location.y + 1 }));
}


void ClickUpdate(Block* block, bool updateTop)
{
	currentLevel->blocks.UpdateBlock(block);
	SurroundBlockUpdate(block, updateTop);
}


Rectangle CollisionXOffsetToRectangle(Actor* actor)
{
	Rectangle result = {};
	result.bottomLeft	= { actor->position.x, actor->position.y + PixelToBlock((int)actor->ScaledHeight()) * actor->animationList->colOffset.x};
	result.topRight		= { actor->position.x + PixelToBlock((int)actor->animationList->scaledWidth), actor->position.y + PixelToBlock((int)actor->ScaledHeight()) * (1 - actor->animationList->colOffset.x) };
	return result;
}

Rectangle CollisionYOffsetToRectangle(Actor* actor)
{
	Rectangle result = {};
	result.bottomLeft = { actor->position.x + (PixelToBlock((int)actor->animationList->scaledWidth) * actor->animationList->colOffset.y), actor->position.y };
	result.topRight   = { actor->position.x + (PixelToBlock((int)actor->animationList->scaledWidth) * (1 - actor->animationList->colOffset.y)), actor->position.y + PixelToBlock((int)actor->ScaledHeight()) };
	return result;
}

uint32 CollisionWithRect(Actor* actor, Rectangle rect)
{
	uint32 result = CollisionNone;
	Rectangle xRect = CollisionXOffsetToRectangle(actor);
	Rectangle yRect = CollisionYOffsetToRectangle(actor);

	if (actor->GetActorType() == ActorType::player && debugList[DebugOptions::playerCollision])
	{
		AddRectToRender(xRect, transGreen, RenderPrio::Debug);
		AddRectToRender(yRect, transOrange, RenderPrio::Debug);
	}


	if (rect.topRight.y > xRect.bottomLeft.y && rect.bottomLeft.y < xRect.topRight.y)
	{
		//checking the right side of player against the left side of a block
		if (rect.bottomLeft.x > xRect.bottomLeft.x && rect.bottomLeft.x < xRect.topRight.x)
			result |= CollisionRight;

		//checking the left side of player against the right side of the block
		if (rect.topRight.x > xRect.bottomLeft.x && rect.topRight.x < xRect.topRight.x)
			result |= CollisionLeft;
	}

	if (rect.topRight.x > yRect.bottomLeft.x && rect.bottomLeft.x < yRect.topRight.x)
	{
		//checking the top of player against the bottom of the block
		if (rect.bottomLeft.y < yRect.topRight.y && rect.topRight.y > yRect.topRight.y)
			result |= CollisionTop;

		//checking the bottom of player against the top of the block
		if (rect.topRight.y > yRect.bottomLeft.y && rect.topRight.y < yRect.topRight.y)
			result |= CollisionBot;
	}
	return result;
}


void CollisionWithBlocks(Actor* actor, bool isEnemy)
{
	bool grounded = false;
	float checkOffset = 1;
	float yMax = floorf(actor->position.y + actor->GameHeight() + checkOffset + 0.5f);
	float yMin = actor->position.y - checkOffset + 0.5f;
	float xMax = actor->position.x + actor->GameWidth() + checkOffset + 0.5f;
	float xMin = actor->position.x - checkOffset + 0.5f;

	for (float y = yMin; y <= yMax; y++)
	{
		for (float x = xMin; x < xMax; x++)
		{
			Block* block = currentLevel->blocks.TryGetBlock({ x, y });
			if (block == nullptr || block->tileType == TileType::invalid)
				continue;

			uint32 collisionFlags = CollisionWithRect(actor, { block->location, { block->location + Vector({ 1, 1 }) } });
			if (!collisionFlags)
				continue;

			ActorType actorType = actor->GetActorType();
			if (collisionFlags & CollisionRight)
			{
				actor->position.x = block->location.x - actor->GameWidth();// PixelToBlock(int32(actor->colRect.Width() * actor->GoldenRatio()));
				if (isEnemy)
					actor->velocity.x = -1 * actor->velocity.x;
				else
					actor->velocity.x = 0;
			}

			if (collisionFlags & CollisionLeft)
			{
				actor->position.x = (block->location.x + 1);// - actor->colOffset.x * PixelToBlock(int32(actor->colRect.Width() * actor->GoldenRatio()));
				if (isEnemy)
					actor->velocity.x = -1 * actor->velocity.x;
				else
					actor->velocity.x = 0;
			}

			if (collisionFlags & CollisionTop)
			{
				//actor->position.y = block->location.y - actor->GameHeight();//PixelToBlock(int32(actor->colRect.Height() * actor->GoldenRatio()));
				if (actor->velocity.y > 0)
					actor->velocity.y = 0;
			}

			if (collisionFlags & CollisionBot)
			{
				actor->position.y = block->location.y + 1;
				if (actor->velocity.y < 0)
					actor->velocity.y = 0;
				actor->jumpCount = 2;
				grounded = true;
			}
		}
	}

	if (actor->grounded != grounded)
	{
		if (!grounded && (actor->GetActorType() == ActorType::player || actor->GetActorType() == ActorType::enemy))
			PlayAnimation(actor, ActorState::jump);
	}
	actor->grounded = grounded;
}

uint32 CollisionWithActor(Player& player, Actor& enemy, Level& level)
{
	uint32 result = 0;

	float xPercentOffset = 0.2f;
	float yPercentOffset = 0.3f;
	Rectangle xRect = CollisionXOffsetToRectangle(&enemy);
	Rectangle yRect = CollisionYOffsetToRectangle(&enemy);

	if (debugList[DebugOptions::playerCollision])
	{
		AddRectToRender(xRect, transGreen, RenderPrio::Debug);
		AddRectToRender(yRect, transOrange, RenderPrio::Debug);
	}

	uint32 xCollisionFlags = CollisionWithRect(&player, xRect) & (CollisionLeft | CollisionRight);
	uint32 yCollisionFlags = CollisionWithRect(&player, yRect) & (CollisionBot | CollisionTop);
	float knockBackAmount = 20;
	result = xCollisionFlags | yCollisionFlags;
	if (enemy.GetActorType() == ActorType::enemy)
	{
		if (result & CollisionBot)
		{//hit enemy, apply damage to enemy
			enemy.UpdateHealth(level, -player.damage);
			player.velocity.y = knockBackAmount;
		}
		else
		{

			if (result & CollisionRight)
			{//hit by enemy, knockback, take damage, screen flash
				player.velocity.x = -knockBackAmount;
				player.velocity.y = knockBackAmount;
				player.UpdateHealth(level, -enemy.damage);
				enemy.invinciblityTime = 1;
			}
			if (result & CollisionLeft)
			{//hit by enemy, knockback, take damage, screen flash
				player.velocity.x = knockBackAmount;
				player.velocity.y = knockBackAmount;
				player.UpdateHealth(level, -enemy.damage);
				enemy.invinciblityTime = 1;
			}
		}
	}

	return result;
}

Player* CreatePlayer(Level& level)
{

	ActorID playerID = {};
	Player* player = new Player(&playerID);
	level.actors.push_back(player);
	return player;
}

Enemy* CreateEnemy(Level& level)
{

    ActorID enemyID = {};
    Enemy* enemy = new Enemy(EnemyType::head, &enemyID);
    PlayAnimation(enemy, ActorState::walk);
    level.actors.push_back(enemy);
    return enemy;
}


Dummy* CreateDummy(ActorType mimicType, Level& level)
{
	ActorID dummyID = {};
	Dummy* dummy = new Dummy(&dummyID);
	AttachAnimation(dummy, mimicType);
	PlayAnimation(dummy, ActorState::dead);
	dummy->actorState = ActorState::dead;
	level.actors.push_back(dummy);
	return dummy;
}

Portal* CreatePortal(int32 PortalID, const std::string& levelName, int32 levelPortalID, Level& level)
{

	Portal* portal = new Portal(PortalID, levelName, levelPortalID);
	AttachAnimation(portal);
	PlayAnimation(portal, ActorState::idle);
	level.actors.push_back(portal);
	return portal;
}

Spring* CreateSpring(Level& level)
{

	Spring* spring = new Spring();
	AttachAnimation(spring);
	PlayAnimation(spring, ActorState::idle);
	level.actors.push_back(spring);
	return spring;
}


MovingPlatform* CreateMovingPlatform(Level& level)
{

	MovingPlatform* MP = new MovingPlatform();
	AttachAnimation(MP);
	PlayAnimation(MP, ActorState::idle);
	level.actors.push_back(MP);
	return MP;
}

Item* CreateItem(Level& level)
{

	return new Item(0);
}

Portal* GetPortalsPointer(Portal* basePortal)
{

	Level* level = GetLevel(basePortal->levelPointer);
	for (int32 i = 0; i < level->actors.size(); i++)
	{
		Actor* actor = level->actors[i];
		if (actor->GetActorType() == ActorType::portal)
		{
			Portal* result = (Portal*)actor;
			if (result->portalID == basePortal->portalPointerID)
			{
				return result;
			}
		}
	}

	assert(false);
	DebugPrint("Failed to get Portal from %s at portalID %d", basePortal->levelPointer.c_str(), basePortal->portalPointerID);
	return nullptr;
}

Projectile* CreateBullet(Actor* player, Vector mouseLoc, TileType blockToBeType)
{
	Projectile* bullet_a = new Projectile();
	Projectile& bullet = *bullet_a;

	bullet.paintType = blockToBeType;
	bullet.inUse = true;
	AttachAnimation(&bullet);
	PlayAnimation(&bullet, ActorState::idle);
	Sprite* sprite = GetSpriteFromAnimation(&bullet);
	bullet.animationList->colRect = { {0,0}, {sprite->width,sprite->height} };
	bullet.animationList->scaledWidth = (float)sprite->width;
	bullet.terminalVelocity = { 1000, 1000 };
	Vector adjustedPlayerPosition = { player->position.x/* + 0.5f*/, player->position.y + 1 };
	Vector playerPosAdjusted = { adjustedPlayerPosition.x + (player->GameWidth() / 2), adjustedPlayerPosition.y };

	float playerBulletRadius = 0.5f; //half a block
	bullet.destination = mouseLoc;
	bullet.rotation = Atan2fToDegreeDiff(atan2f(bullet.destination.y - adjustedPlayerPosition.y, bullet.destination.x - adjustedPlayerPosition.x));

	float speed = 50.0f;
	Vector ToDest = bullet.destination - playerPosAdjusted;
	ToDest = Normalize(ToDest);
	bullet.velocity = ToDest * speed;
	Vector gameSize = { bullet.GameWidth(), bullet.GameHeight() };
	bullet.position = adjustedPlayerPosition + (ToDest * playerBulletRadius)/* + gameSize*/;
	return bullet_a;
}


void UpdateLaser(Actor* player, Vector mouseLoc, TileType paintType, float deltaTime)
{
	laser.paintType = paintType;
	laser.inUse = true;
	AttachAnimation(&laser);
	PlayAnimation(&laser, ActorState::idle);
	Sprite* sprite = GetSpriteFromAnimation(&laser);
	laser.animationList->colRect = { {0,0}, {sprite->width, sprite->height} };
	laser.animationList->scaledWidth = (float)sprite->width;
	Vector adjustedPlayerPosition = { player->position.x + 0.5f, player->position.y + 1 };

	float playerBulletRadius = 0.5f; //half a block
	laser.destination = mouseLoc;
	laser.rotation = Atan2fToDegreeDiff(atan2f(laser.destination.y - adjustedPlayerPosition.y, laser.destination.x - adjustedPlayerPosition.x));

	Vector ToDest = Normalize(laser.destination - adjustedPlayerPosition);
	laser.position = adjustedPlayerPosition + (ToDest * playerBulletRadius);
	PlayAnimation(&laser, ActorState::idle);
}

Actor* FindActor(ActorID actorID,Level& level)
{
	for (auto& actor : level.actors)
	{
		if (actor->id == actorID)
			return actor;
	}
	return nullptr;
}

//returns first find of that type
Player* FindPlayer(Level& level)
{
	for (auto& actor : level.actors)
	{
		if (actor->GetActorType() == ActorType::player)
			return (Player*)actor;
	}
	return nullptr;
}

void UpdateAnimationIndex(Actor* actor, float deltaTime)
{
	bool ignoringStates = actor->actorState == ActorState::dead || actor->actorState == ActorState::jump;

	if (actor->animationCountdown < 0)
	{
		actor->index++;
		Animation* anime = actor->animationList->GetAnimation(actor->actorState);
		actor->animationCountdown = 1.0f / anime->fps;
		if (actor->index >= anime->anime.size())
		{
			if (ignoringStates)
				actor->index = (int32)anime->anime.size() - 1;
			else
				actor->index = 0;
		}
	}
	//TODO:: Fix animationCountdown going far negative when index == anime.size()
	actor->animationCountdown -= deltaTime;
	if (actor->animationList->GetAnimation(actor->actorState) == nullptr)
		actor->currentSprite = nullptr;
	else
		actor->currentSprite = actor->animationList->GetAnimation(actor->actorState)->anime[actor->index];
}


void PlayAnimation(Actor* actor, ActorState state)
{
	ActorType actorType = actor->GetActorType();
	if (actor->animationList->GetAnimation(state) == nullptr)
	{
		ActorState holdingState = ActorState::none;
		for (int32 i = 1; i < (int32)ActorState::count; i++)
		{
			if (actor->animationList->GetAnimation((ActorState)i) != nullptr)
			{
				holdingState = (ActorState)i;
				break;
			}
		}
		if (holdingState == ActorState::none)
		{
			//no valid animation to use
			assert(false);
			DebugPrint("No valid animation for %s \n", std::to_string((int)actor->GetActorType()).c_str());
			return;
		}
		state = holdingState;
	}
	if (actor->actorState != state || state == ActorState::jump)
	{
		actor->actorState = state;
		actor->index = 0;
		actor->animationCountdown =	(1.0f / actor->animationList->GetAnimation(state)->fps);
		UpdateAnimationIndex(actor, 0);
	}
}

void UpdateActorHealth(Level& level, Actor* actor, float deltaHealth)
{
	ActorType actorType = actor->GetActorType();
	if (actor->inUse)
	{
		if (actor->invinciblityTime <= 0)
		{
			actor->health += deltaHealth;
			if (deltaHealth < 0)
				actor->invinciblityTime = 1;
		}
		else
		{
			if (deltaHealth > 0)
				actor->health += deltaHealth;
		}

		actor->health = Max(actor->health, 0.0f);
		if (actor->health == 0)
		{
			//TODO: put a level pointer onto each actor
			Dummy* dummy = CreateDummy(actor->GetActorType(), level);

			dummy->position						= actor->position;
			dummy->animationList->colOffset		= actor->animationList->colOffset;
			dummy->animationList->colRect		= actor->animationList->colRect;
			dummy->animationList->scaledWidth	= actor->animationList->scaledWidth;
			dummy->lastInputWasLeft				= actor->lastInputWasLeft;
			PlayAnimation(dummy, ActorState::dead);
			actor->inUse = false;
		}
	}
}

void UpdateLocation(Actor* actor, float gravity, float deltaTime)
{
	actor->velocity.y += gravity * deltaTime;
	actor->velocity.y = Clamp(actor->velocity.y, -actor->terminalVelocity.y, actor->terminalVelocity.y);

	actor->velocity.x += actor->acceleration.x * deltaTime;
	//actor->velocity.x = Clamp(actor->velocity.x, -actor->terminalVelocity.x, actor->terminalVelocity.x);
	actor->position += actor->velocity * deltaTime;

	ActorType actorType = actor->GetActorType();
	if (actor->velocity.x == 0 && actor->velocity.y == 0 &&
		(actorType == ActorType::player || actorType == ActorType::enemy))
		PlayAnimation(actor, ActorState::idle);

	if (actorType != ActorType::player)
	{
		if (actor->velocity.x < 0)
			actor->lastInputWasLeft = true;
		else if (actor->velocity.x > 0)
			actor->lastInputWasLeft = false;
	}
}



void UpdateEnemiesPosition(float gravity, float deltaTime)
{
	for (int32 i = 0; i < currentLevel->actors.size(); i++)
	{
		Actor* actor = currentLevel->actors[i];
		if (actor->GetActorType() == ActorType::enemy)
		{
			UpdateLocation(actor, gravity, deltaTime);
			CollisionWithBlocks(actor, true);
		}
	}
}

void RenderActors()
{
	for (int32 i = 0; i < currentLevel->actors.size(); i++)
	{
		currentLevel->actors[i]->Render();
	}
}

//UI or Game space?  Game
void RenderActorHealthBars(Actor& actor)
{
	int32 healthHeight = 8;
	Rectangle full = {};
	Rectangle actual = {};
	full.bottomLeft.x = actor.position.x;
	full.bottomLeft.y = actor.position.y + PixelToBlock(int(actor.ScaledHeight() + healthHeight));
	full.topRight.x = full.bottomLeft.x + PixelToBlock(int(actor.animationList->scaledWidth));
	full.topRight.y = full.bottomLeft.y + healthHeight;

	actual.bottomLeft.x = full.topRight.x - (actor.health / 100) * full.Width();
	actual.bottomLeft.y = full.bottomLeft.y;
	actual.topRight = full.topRight;

	GameSpaceRectRender(full, HealthBarBackground);
	GameSpaceRectRender(actual, Green);
}

void LoadAllAnimationStates(const std::string& entity)
{
	if (actorAnimations.find(entity) != actorAnimations.end())
		return;

	std::string stateStrings[(int32)ActorState::count] = {"error", "Idle", "Walk", "Run", "Jump", "Dead"};
	actorAnimations[entity].animations.reserve((int32)ActorState::count);

	for (int32 i = 1; i < (int32)ActorState::count; i++)
	{
		Animation* animeP = new Animation();
		animeP->type = (ActorState)i;
		animeP->anime.reserve(20);
		for (int32 j = 1; true; j++)
		{
			std::string combinedName = "Assets/" + entity + "/" + stateStrings[i] + " (" + std::to_string(j) + ").png";
			Sprite* sprite = CreateSprite(combinedName.c_str(), SDL_BLENDMODE_BLEND);
			if (sprite == nullptr)
				break;
			else
				animeP->anime.push_back(sprite);
		}
		if (animeP->anime.size() > 0)
			actorAnimations[entity].animations.push_back(animeP);
		else
			delete animeP;
	}
}

void AttachAnimation(Actor* actor, ActorType overrideType)
{
	std::string entityName;
	ActorType actorReferenceType;

	if (overrideType != ActorType::none)
		actorReferenceType = overrideType;
	else
		actorReferenceType = actor->GetActorType();

	switch (actorReferenceType)
	{
	case ActorType::player:
	{
		entityName = "Dino";
		break;
	}
	case ActorType::enemy:
	{
		entityName = "Striker";//"HeadMinion";
		break;
	}
	case ActorType::projectile:
	{
		entityName = "Bullet";
		break;
	}
	case ActorType::portal:
	{
		entityName = "Portal";
		break;
	}
	case ActorType::spring:
	{
		entityName = "Spring";
		break;
	}
	case ActorType::movingPlatform:
	{
		entityName = "MovingPlatform";
		break;
	}
	default:
		assert(false);
		DebugPrint("AttachAnimation could not find animation list for this actor\n");
		break;
	}
	actor->animationList = &actorAnimations[entityName];
}
