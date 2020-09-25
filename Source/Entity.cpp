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

Projectile laser = {};
std::unordered_map<std::string, Level> levels;
//Level* currentLevel = nullptr;
std::unordered_map<std::string, AnimationList> actorAnimations;

/*********************
 *
 * Player
 *
 ********/

void Player::OnInit()
{

	damage = 100;
	inUse = true;
	AttachAnimation(this);
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

void Enemy::OnInit()
{

	position = { 28, 1 };
	velocity.x = 4;
	damage = 25;
	enemyType = EnemyType::head;
	AttachAnimation(this);
    PlayAnimation(this, ActorState::walk);
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

void Projectile::OnInit(const ProjectileInfo& info)
{

	assert(info.player);

	paintType = info.blockToBeType;
	AttachAnimation(this);
	PlayAnimation(this, ActorState::idle);
	terminalVelocity = { 1000, 1000 };
	Vector adjustedPlayerPosition = { info.player->position.x/* + 0.5f*/, 
									  info.player->position.y + 1 };
	Vector playerPosAdjusted = { adjustedPlayerPosition.x + (info.player->GameWidth() / 2), 
							     adjustedPlayerPosition.y };

	float playerBulletRadius = 0.5f; //half a block
	destination = info.mouseLoc;
	rotation = RadToDeg(atan2f(destination.y - adjustedPlayerPosition.y, destination.x - adjustedPlayerPosition.x));

	float speed = 50.0f;
	Vector ToDest = destination - playerPosAdjusted;
	ToDest = Normalize(ToDest);
	velocity = ToDest * speed;
	Vector gameSize = { GameWidth(), GameHeight() };
	position = adjustedPlayerPosition + (ToDest * playerBulletRadius)/* + gameSize*/;
}

void Projectile::Update(float deltaTime)
{
	UpdateLocation(this, 0, deltaTime);
	float rad = DegToRad(rotation);
	Vector realPosition = { position.x + cosf(rad)*GameWidth(), position.y - sinf(rad) * GameWidth() };
	if (DotProduct(velocity, destination - realPosition) < 0)
	{
		//paint block and remove bullet
		Block* blockPointer = &level->blocks.GetBlock(destination);
		blockPointer->tileType = paintType;
		UpdateAllNeighbors(blockPointer, level);
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

void Dummy::OnInit(const DummyInfo& info)
{

	assert(info.mimicType != ActorType::none);
	health = 0;
	inUse = true;
	AttachAnimation(this, info.mimicType);
	PlayAnimation(this, ActorState::dead);
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

void Portal::OnInit(const PortalInfo& info)
{

	assert(info.levelPortalID);
	assert(info.PortalID);
	assert(info.levelName.size());
	levelPointer = info.levelName;
    portalPointerID = info.levelPortalID;
    portalID = info.PortalID;
	damage = 0;
	AttachAnimation(this);
	PlayAnimation(this, ActorState::idle);
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

void Spring::OnInit()
{

	damage = 0;
	AttachAnimation(this);
	PlayAnimation(this, ActorState::idle);
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

void MovingPlatform::OnInit()
{

	nextLocationIndex = 1;
	incrimentPositivePathIndex = true;
	acceleration = { 0, 0 };
	damage = 0;
	inUse = true;
	AttachAnimation(this);
	PlayAnimation(this, ActorState::idle);
	level->movingPlatforms.push_back(id);
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
 * Grapple
 *
 ********/

void Grapple::OnInit()
{

}
void Grapple::Update(float deltaTime)
{
	UpdateLocation(this, 0, deltaTime);
	float rad = DegToRad(rotation);
	Vector realPosition = { position.x + cosf(rad)*GameWidth(), position.y - sinf(rad) * GameWidth() };
	if (DotProduct(velocity, destination - realPosition) < 0)
	{
		//paint block and remove bullet
		Block* blockPointer = &level->blocks.GetBlock(destination);
		Player* player = level->FindActor<Player>(attachedActor);
		if (player != nullptr)
		{
			player->grappleDeployed = false;
			player->grappleReady = true;
		}
		inUse = false;

	}
	UpdateAnimationIndex(this, deltaTime);
}

void Grapple::Render()
{
	Sprite* sprite = GetSpriteFromAnimation(this);
	Rectangle rectangle = { {position}, {position.x + Pythags(destination - position), position.y + PixelToBlock(sprite->height)} };
	static SDL_Point rotationPoint = { 0, (int32)rectangle.Height() / 2 };
	
	//AddTextureToRender({}, rect, RenderPrio::Sprites, sprite, {}, rotation, &rotationPoint, SDL_FLIP_NONE, CoordinateSpace::World);

	//RenderActor(this, rotation);
}

/*********************
 *
 * Items
 *
 ********/

void Item::OnInit()
{

}
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
	Block& result = blocks[HashingFunction(loc)];
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
const std::unordered_map<uint64, Block>* TileMap::BlockList()
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

//std::unordered_map<std::string, Level> levels;
Player* FindPlayerGlobal()
{
	for (auto& level : levels)
	{
		Player* ptr = level.second.FindPlayer();
		if (ptr)
			return ptr;
	}
	return nullptr;
}

template <typename T>
T* FindPlayerGlobal(ActorID id)
{
	for (auto& level : levels)
	{
		T* a = level.second.FindActor<T>(id);
		if (a)
			return a;
	}
	return nullptr;
}

TileType CheckColor(SDL_Color color)
{
	if (color.r == Brown.r && color.g == Brown.g && color.b == Brown.b && color.a == Brown.a)
		return TileType::filled;
	else
		return TileType::invalid;
}

SDL_Color FloatToSDL_Color(Color c)
{
	return { uint8(c.r * 255), uint8(c.g * 255), uint8(c.b * 255), uint8(c.a * 255) };
}

Color GetTileMapColor(const Block& block)
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
	level->blocks.CleanBlocks();

	//Finding the edges of the "picture"/level
	int32 left = int32(player.position.x);
	int32 right = int32(player.position.x);
	int32 top = int32(player.position.y);
	int32 bot = int32(player.position.y);

	for (auto& block : *level->blocks.BlockList())
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
	for (auto& block : *level->blocks.BlockList())
	{
		if (block.second.tileType == TileType::invalid)
			continue;

		memBuff[size_t((block.second.location.x - left) + ((block.second.location.y - bot) * size_t(width)))] = FloatToSDL_Color(GetTileMapColor(block.second));
	}

	//TODO(choman):  When new entity system is done change this to include other Actors
	int32 playerBlockPositionX = int32(player.position.x);
	int32 playerBlockPositionY = int32(player.position.y);
	memBuff[size_t((playerBlockPositionX - left) + ((playerBlockPositionY - bot) * size_t(width)))] = FloatToSDL_Color(Blue);

	//Saving written memory to file
	int32 stride_in_bytes = 4 * width;
	int32 colorChannels = 4;
	stbi_write_png(level->filename, width, height, colorChannels, memBuff.data(), stride_in_bytes);
}


void LoadLevel(Level* level, Player& player)
{
	level->blocks.ClearBlocks();

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
				level->blocks.AddBlock({ float(x), float(y) }, tileColor);
		}
	}
	level->blocks.UpdateAllBlocks();
	stbi_image_free(image);
}


void UpdateAllNeighbors(Block* block, Level* level)
{
	for (int32 y = -1; y <= 1; y++)
	{
		for (int32 x = -1; x <= 1; x++)
		{
			assert(level);
			if (level->blocks.TryGetBlock({ float(block->location.x + x), float(block->location.y + y) }) != nullptr)
				level->blocks.UpdateBlock(&level->blocks.GetBlock({ float(block->location.x + x), float(block->location.y + y) }));
		}
	}
}


void SurroundBlockUpdate(Block* block, bool updateTop, Level* level)
{
	//update block below
	if (level->blocks.CheckForBlock({ block->location.x, block->location.y - 1 }))
		level->blocks.UpdateBlock(&level->blocks.GetBlock({ block->location.x, block->location.y - 1 }));
	//update block to the left
	if (level->blocks.CheckForBlock({ block->location.x - 1, block->location.y }))
		level->blocks.UpdateBlock(&level->blocks.GetBlock({ block->location.x - 1, block->location.y }));
	//update block to the right
	if (level->blocks.CheckForBlock({ block->location.x + 1, block->location.y }))
		level->blocks.UpdateBlock(&level->blocks.GetBlock({ block->location.x + 1, block->location.y }));
	if (updateTop && level->blocks.CheckForBlock({ block->location.x, block->location.y - 1 }))
		level->blocks.UpdateBlock(&level->blocks.GetBlock({ block->location.x, block->location.y + 1 }));
}


void ClickUpdate(Block* block, bool updateTop, Level* level)
{
	level->blocks.UpdateBlock(block);
	SurroundBlockUpdate(block, updateTop, level);
}


Rectangle CollisionXOffsetToRectangle(Actor* actor)
{
	Rectangle result = {};
	result.botLeft	= { actor->position.x, actor->position.y + PixelToBlock((int)actor->ScaledHeight()) * actor->animationList->colOffset.x};
	result.topRight		= { actor->position.x + PixelToBlock((int)actor->animationList->scaledWidth), actor->position.y + PixelToBlock((int)actor->ScaledHeight()) * (1 - actor->animationList->colOffset.x) };
	return result;
}

Rectangle CollisionYOffsetToRectangle(Actor* actor)
{
	Rectangle result = {};
	result.botLeft = { actor->position.x + (PixelToBlock((int)actor->animationList->scaledWidth) * actor->animationList->colOffset.y), actor->position.y };
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
		AddRectToRender(xRect, transGreen, RenderPrio::Debug, CoordinateSpace::World);
		AddRectToRender(yRect, transOrange, RenderPrio::Debug, CoordinateSpace::World);
	}


	if (rect.topRight.y > xRect.botLeft.y && rect.botLeft.y < xRect.topRight.y)
	{
		//checking the right side of player against the left side of a block
		if (rect.botLeft.x > xRect.botLeft.x && rect.botLeft.x < xRect.topRight.x)
			result |= CollisionRight;

		//checking the left side of player against the right side of the block
		if (rect.topRight.x > xRect.botLeft.x && rect.topRight.x < xRect.topRight.x)
			result |= CollisionLeft;
	}

	if (rect.topRight.x > yRect.botLeft.x && rect.botLeft.x < yRect.topRight.x)
	{
		//checking the top of player against the bottom of the block
		if (rect.botLeft.y < yRect.topRight.y && rect.topRight.y > yRect.topRight.y)
			result |= CollisionTop;

		//checking the bottom of player against the top of the block
		if (rect.topRight.y > yRect.botLeft.y && rect.topRight.y < yRect.topRight.y)
			result |= CollisionBot;
	}
	return result;
}

bool CollisionWithBlocksSubFunction(bool& grounded, Rectangle rect, Actor* actor, bool isEnemy)
{

	uint32 collisionFlags = CollisionWithRect(actor, rect);
	if (collisionFlags > 0)
	{
		ActorType actorType = actor->GetActorType();
		if (collisionFlags & CollisionRight)
		{
			actor->position.x = rect.botLeft.x - actor->GameWidth();
			if (isEnemy)
				actor->velocity.x = -1 * actor->velocity.x;
			else
				actor->velocity.x = 0;
		}

		if (collisionFlags & CollisionLeft)
		{
			actor->position.x = (rect.botLeft.x + 1);
			if (isEnemy)
				actor->velocity.x = -1 * actor->velocity.x;
			else
				actor->velocity.x = 0;
		}

		if (collisionFlags & CollisionTop)
		{
			if (actor->velocity.y > 0)
				actor->velocity.y = 0;
		}

		if (collisionFlags & CollisionBot)
		{
			actor->position.y = rect.botLeft.y + 1;
			if (actor->velocity.y < 0)
				actor->velocity.y = 0;
			actor->jumpCount = 2;
			grounded = true;
		}
		return true;
	}
	return false;
}


void CollisionWithBlocks(Actor* actor, bool isEnemy) 
{
	bool grounded = false;
	float checkOffset = 1;
	float yMax = floorf(actor->position.y + actor->GameHeight() + checkOffset + 0.5f);
	float yMin = actor->position.y - checkOffset + 0.5f;
	float xMax = actor->position.x + actor->GameWidth() + checkOffset + 0.5f;
	float xMin = actor->position.x - checkOffset + 0.5f;

	//Checking Blocks
	for (float y = yMin; y <= yMax; y++)
	{
		for (float x = xMin; x < xMax; x++)
		{
			Block* block = actor->level->blocks.TryGetBlock({ x, y });
			if (block == nullptr || block->tileType == TileType::invalid)
				continue;

			CollisionWithBlocksSubFunction(grounded, { block->location, { block->location + Vector({ 1, 1 }) } }, actor, isEnemy);
		}
	}


	//Checking Moving Platforms
	ActorID collisionID = 0;
	MovingPlatform* collidedPlatform = nullptr;

	for (int32 i = 0; i < actor->level->movingPlatforms.size(); i++)
	{
		if (Actor* actorTwo = actor->level->FindActor<MovingPlatform>(actor->level->movingPlatforms[i]))
		{
			MovingPlatform* MP = (MovingPlatform*)actorTwo;

			Rectangle MPRect = { MP->position, { MP->position.x + MP->GameWidth(), MP->position.y + MP->GameHeight() } };
			if (CollisionWithBlocksSubFunction(grounded, MPRect, actor, isEnemy))
			{
				collisionID = actor->level->movingPlatforms[i];
				collidedPlatform = MP;
			}
		}
	}

	//new parent is different from old parent
	//need to remove child on old parent and put onto new parent
	if (collidedPlatform)
	{

		collidedPlatform->childList.push_back(actor->id);
		actor->parent = collisionID;
	}
	else
	{

		if (MovingPlatform* MP = actor->level->FindActor<MovingPlatform>(actor->parent))
		{
			actor->velocity += MP->velocity;
			actor->parent = 0;
		}
	}
	

	//Grounded Logic
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
		AddRectToRender(xRect, transGreen, RenderPrio::Debug, CoordinateSpace::World);
		AddRectToRender(yRect, transOrange, RenderPrio::Debug, CoordinateSpace::World);
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
	//DebugPrint("Failed to get Portal from %s at portalID %d", basePortal->levelPointer.c_str(), basePortal->portalPointerID);
	DebugPrint("Failed to get Portal from %s at portalID %d", basePortal->levelPointer, basePortal->portalPointerID);
	return nullptr;
}

void UpdateLaser(Actor* player, Vector mouseLoc, TileType paintType, float deltaTime)
{
	laser.paintType = paintType;
	laser.inUse = true;
	AttachAnimation(&laser);
	PlayAnimation(&laser, ActorState::idle);
	Vector adjustedPlayerPosition = { player->position.x + 0.5f, player->position.y + 1 };

	float playerBulletRadius = 0.5f; //half a block
	laser.destination = mouseLoc;
	laser.rotation = RadToDeg(atan2f(laser.destination.y - adjustedPlayerPosition.y, laser.destination.x - adjustedPlayerPosition.x));

	Vector ToDest = Normalize(laser.destination - adjustedPlayerPosition);
	laser.position = adjustedPlayerPosition + (ToDest * playerBulletRadius);
	PlayAnimation(&laser, ActorState::idle);
}


void UpdateAnimationIndex(Actor* actor, float deltaTime)
{
	bool ignoringStates = actor->actorState == ActorState::dead || actor->actorState == ActorState::jump;

	if (actor->animationCountdown < 0)
	{
		actor->index++;
		Animation* anime = actor->animationList->GetAnimation(actor->actorState);
		if (anime == nullptr)
		{
			anime = actor->animationList->GetAnyValidAnimation();
			DebugPrint("Could not get valid animation from actor state in UpdateAnimationIndex()");
		}

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
		actor->currentSprite = actor->animationList->GetAnyValidAnimation()->anime[0];
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
			DummyInfo info;
			info.mimicType = actor->GetActorType();
			Dummy* dummy = level.CreateActor<Dummy>(info);
			dummy->position			= actor->position;
			dummy->lastInputWasLeft	= actor->lastInputWasLeft;
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
	actor->deltaPosition = actor->velocity * deltaTime;
	actor->position += actor->deltaPosition;

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


void UpdateEnemiesPosition(std::vector<Actor*>* actors, float gravity, float deltaTime)
{
	for (int32 i = 0; i < actors->size(); i++)
	{
		Actor* actor = (*actors)[i];
		if (actor->GetActorType() == ActorType::enemy)
		{
			UpdateLocation(actor, gravity, deltaTime);
			CollisionWithBlocks(actor, true);
		}
	}
}

void RenderActors(std::vector<Actor*>* actors)
{
	for (int32 i = 0; i < actors->size(); i++)
	{
		(*actors)[i]->Render();
	}
}

void RenderActorHealthBars(Actor& actor)
{
	float healthHeight = 0.25f;
	Rectangle full = {};
	Rectangle actual = {};
	full.botLeft.x = actor.position.x;
	full.botLeft.y = actor.position.y + PixelToBlock(int(actor.ScaledHeight())) + healthHeight;
	full.topRight.x = full.botLeft.x + PixelToBlock(int(actor.animationList->scaledWidth));
	full.topRight.y = full.botLeft.y + healthHeight;

	actual.botLeft.x = full.topRight.x - (actor.health / 100) * full.Width();
	actual.botLeft.y = full.botLeft.y;
	actual.topRight = full.topRight;

    AddRectToRender(RenderType::DebugFill, full, HealthBarBackground, RenderPrio::Debug, CoordinateSpace::World);
    AddRectToRender(RenderType::DebugFill, actual, Green, RenderPrio::Debug, CoordinateSpace::World);
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
		entityName = "Knight"; //"Dino";
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
	case ActorType::grapple:
	{
		entityName = "Grapple";
		break;
	}
	default:
		assert(false);
		DebugPrint("AttachAnimation could not find animation list for this actor\n");
		break;
	}
	actor->animationList = &actorAnimations[entityName];
}
