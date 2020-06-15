#include "Entity.h"
#include "Rendering.h"
#include "stb/stb_image.h"
#include "stb/stb_image_write.h"


ActorID Actor::lastID = 0;

TileMap tileMap;
Projectile laser = {};
std::vector<Actor*> actorList= {};
Level currentLevel;
//static const Vector terminalVelocity = { 10 , 300 };

/*********************
 *
 * Player
 *
 ********/


void Player::Update(float deltaTime)
{
	UpdateLocation(this, -60.0f, deltaTime);
	CollisionWithBlocks(this, false);
	//if (health <= 0)
	//{
	//	//death animation
	//	actorState = ActorState::dead;
	//}
	//else if (velocity.x == 0 && velocity.y == 0)
	//{
	//	//idle
	//	actorState = ActorState::idle;
	//}
	//else if (velocity.y != 0)
	//{
	//	//jumping/in-air, 
	//	//TODO:  Fix issue with when this wont be true at the top of a jump when velcoity approaches zero
	//	actorState = ActorState::jump;
	//}
	//else if (velocity.x != 0)
	//{
	//	//walk
	//	actorState = ActorState::walk;
	//}
}

void Player::Render(double totalTime)
{
	RenderActor(this, 0, totalTime);
	RenderActorHealthBars(*this);
}

void Player::UpdateHealth(double totalTime)
{
	UpdateActorHealth(this, totalTime);
}

ActorType Player::GetActorType()
{
	return ActorType::player;
}


/*********************
 *
 * Enemy
 *
 ********/


void Enemy::Update(float deltaTime)
{
	UpdateLocation(this, -60.0f, deltaTime);
	CollisionWithBlocks(this, true);
	actorState = ActorState::idle;
}

void Enemy::Render(double totalTime)
{
	RenderActor(this, 0, totalTime);
	RenderActorHealthBars(*this);
}

void Enemy::UpdateHealth(double totalTime)
{
	UpdateActorHealth(this, totalTime);
}

ActorType Enemy::GetActorType()
{
	return ActorType::enemy;
}


/*********************
 *
 * Projectile
 *
 ********/

void Projectile::Update(float deltaTime)
{
	UpdateLocation(this, 0, deltaTime);
	if (DotProduct(velocity, destination - position) < 0)
	{
		//paint block and remove bullet
		Block* blockPointer = &tileMap.GetBlock(destination);
		blockPointer->tileType = paintType;
		UpdateAllNeighbors(blockPointer);
		inUse = false;
	}
	actorState = ActorState::idle;
}

void Projectile::Render(double totalTime)
{
	SDL_Color PC = {};
	if (paintType == TileType::filled)
		PC = Green;
	else
		PC = Red;
	SDL_SetTextureColorMod(idle.anime[0]->texture, PC.r, PC.g, PC.b);
	RenderActor(this, rotation, totalTime);
}

ActorType Projectile::GetActorType()
{
	return ActorType::projectile;
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
	tileMap.CleanBlocks();

	//Finding the edges of the "picture"/level
	int32 left = int32(player.position.x);
	int32 right = int32(player.position.x);
	int32 top = int32(player.position.y);
	int32 bot = int32(player.position.y);

	for (auto& block : *tileMap.blockList())
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
	for (auto& block : *tileMap.blockList())
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
	tileMap.ClearBlocks();

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
				tileMap.AddBlock({ float(x), float(y) }, tileColor);
		}
	}
	tileMap.UpdateAllBlocks();
	stbi_image_free(image);
}


void UpdateAllNeighbors(Block* block)
{
	for (int32 y = -1; y <= 1; y++)
	{
		for (int32 x = -1; x <= 1; x++)
		{
			if (tileMap.TryGetBlock({ float(block->location.x + x), float(block->location.y + y) }) != nullptr)
				tileMap.UpdateBlock(&tileMap.GetBlock({ float(block->location.x + x), float(block->location.y + y) }));
		}
	}
}


void SurroundBlockUpdate(Block* block, bool updateTop)
{
	//update block below
	if (tileMap.CheckForBlock({ block->location.x, block->location.y - 1 }))
		tileMap.UpdateBlock(&tileMap.GetBlock({ block->location.x, block->location.y - 1 }));
	//update block to the left
	if (tileMap.CheckForBlock({ block->location.x - 1, block->location.y }))
		tileMap.UpdateBlock(&tileMap.GetBlock({ block->location.x - 1, block->location.y }));
	//update block to the right
	if (tileMap.CheckForBlock({ block->location.x + 1, block->location.y }))
		tileMap.UpdateBlock(&tileMap.GetBlock({ block->location.x + 1, block->location.y }));
	if (updateTop && tileMap.CheckForBlock({ block->location.x, block->location.y - 1 }))
		tileMap.UpdateBlock(&tileMap.GetBlock({ block->location.x, block->location.y + 1 }));
}


void ClickUpdate(Block* block, bool updateTop)
{
	tileMap.UpdateBlock(block);
	SurroundBlockUpdate(block, updateTop);
}

Rectangle CollisionXOffsetToRectangle(Actor* actor)
{
	Rectangle result = {};
	result.bottomLeft = { actor->position.x + PixelToBlock(int32(actor->ScaledWidth())) / 2, actor->position.y + actor->colOffset.x * PixelToBlock(int32(actor->ScaledHeight())) };
	result.topRight = { actor->position.x + (1 - actor->colOffset.x) * PixelToBlock(int32(actor->ScaledWidth())), actor->position.y + (1 - actor->colOffset.x) * PixelToBlock(int32(actor->ScaledHeight())) };
	return result;
}
Rectangle CollisionYOffsetToRectangle(Actor* actor)
{
	Rectangle result = {};
	result.bottomLeft = { actor->position.x + PixelToBlock(int32(actor->ScaledWidth())) / 2, actor->position.y };
	result.topRight = { actor->position.x + (1 - actor->colOffset.y) * PixelToBlock(int32(actor->ScaledWidth())), actor->position.y + PixelToBlock(int32(actor->ScaledHeight())) };
	return result;
}



uint32 CollisionWithRect(Actor* actor, Rectangle rect)
{
	uint32 result = CollisionNone;

	Rectangle xRect = CollisionXOffsetToRectangle(actor);
	Rectangle yRect = CollisionYOffsetToRectangle(actor);

	float xPercentOffset = 0.2f;
	float yPercentOffset = 0.2f;

	xRect.bottomLeft	= { actor->position.x, actor->position.y + PixelToBlock((int)actor->ScaledHeight()) * xPercentOffset };
	xRect.topRight		= { actor->position.x + PixelToBlock((int)actor->ScaledWidth()), actor->position.y + PixelToBlock((int)actor->ScaledHeight()) * (1 - xPercentOffset) };

	yRect.bottomLeft	= { actor->position.x + (PixelToBlock((int)actor->ScaledWidth()) * yPercentOffset),		actor->position.y };
	yRect.topRight		= { actor->position.x + (PixelToBlock((int)actor->ScaledWidth()) * (1 - yPercentOffset)), actor->position.y + PixelToBlock((int)actor->ScaledHeight()) };

	//if (debugList[DebugOptions::playerCollision])
	//{
	//	SDL_SetRenderDrawBlendMode(windowInfo.renderer, SDL_BLENDMODE_BLEND);
	//	DebugRectRender(xRect, transOrange);
	//	DebugRectRender(yRect, transGreen);
	//}


	if ((rect.topRight.y) > xRect.bottomLeft.y && rect.bottomLeft.y < xRect.topRight.y)
	{
		//checking the right side of player against the left side of a block
		if (rect.bottomLeft.x > xRect.bottomLeft.x && rect.bottomLeft.x < xRect.topRight.x)
			result |= CollisionRight;

		//checking the left side of player against the right side of the block
		if ((rect.topRight.x) > xRect.bottomLeft.x && (rect.topRight.x) < xRect.topRight.x)
			result |= CollisionLeft;
	}

	if (rect.topRight.x > yRect.bottomLeft.x && rect.bottomLeft.x < yRect.topRight.x)
	{
		//checking the top of player against the bottom of the block
		if (rect.bottomLeft.y > yRect.bottomLeft.y && rect.bottomLeft.y < yRect.topRight.y)
			result |= CollisionTop;

		//checking the bottom of player against the top of the block
		if ((rect.topRight.y) > yRect.bottomLeft.y && (rect.topRight.y) < yRect.topRight.y)
			result |= CollisionBot;
	}
	return result;
}


void CollisionWithBlocks(Actor* actor, bool isEnemy)
{
	for (auto& block : *tileMap.blockList())
	{
		if (block.second.tileType == TileType::invalid)
			continue;

		uint32 collisionFlags = CollisionWithRect(actor, { block.second.location, { block.second.location + Vector({ 1, 1 }) } });

		SDL_SetRenderDrawBlendMode(windowInfo.renderer, SDL_BLENDMODE_BLEND);
		//DebugRectRender({ block.second.location, { block.second.location + Vector({ 1, 1 }) } }, transRed);

		if (collisionFlags & CollisionRight)
		{
			actor->position.x = block.second.location.x - actor->GameWidth();// PixelToBlock(int32(actor->colRect.Width() * actor->GoldenRatio()));
			if (isEnemy)
				actor->velocity.x = -1 * actor->velocity.x;
			else
				actor->velocity.x = 0;
		}
		if (collisionFlags & CollisionLeft)
		{
			actor->position.x = (block.second.location.x + 1);// - actor->colOffset.x * PixelToBlock(int32(actor->colRect.Width() * actor->GoldenRatio()));
			if (isEnemy)
				actor->velocity.x = -1 * actor->velocity.x;
			else
				actor->velocity.x = 0;
		}
		if (collisionFlags & CollisionTop)
		{
			actor->position.y = block.second.location.y - actor->GameHeight();//PixelToBlock(int32(actor->colRect.Height() * actor->GoldenRatio()));
			if (actor->velocity.y > 0)
				actor->velocity.y = 0;
		}
		if (collisionFlags & CollisionBot)
		{
			actor->position.y = block.second.location.y + 1;
			if (actor->velocity.y < 0)
				actor->velocity.y = 0;
			actor->jumpCount = 2;
		}
	}
}

bool CollisionWithEnemy(Player& player, Actor& enemy, float currentTime)
{
	bool result = false;

	Rectangle xRect = CollisionXOffsetToRectangle(&enemy);
	Rectangle yRect = CollisionYOffsetToRectangle(&enemy);

	float xPercentOffset = 0.2f;
	float yPercentOffset = 0.3f;

	xRect.bottomLeft	= { enemy.position.x - PixelToBlock((int)enemy.scaledWidth), enemy.position.y + PixelToBlock((int)enemy.ScaledHeight()) * xPercentOffset };
	xRect.topRight		= { enemy.position.x + PixelToBlock((int)enemy.scaledWidth), enemy.position.y + PixelToBlock((int)enemy.ScaledHeight()) * (1 - xPercentOffset) };

	yRect.bottomLeft	= { enemy.position.x - PixelToBlock((int)enemy.scaledWidth), enemy.position.y + PixelToBlock((int)enemy.ScaledHeight()) * yPercentOffset };
	yRect.topRight		= { enemy.position.x + PixelToBlock((int)enemy.scaledWidth), enemy.position.y + PixelToBlock((int)enemy.ScaledHeight()) * (1 - yPercentOffset) };

	uint32 xCollisionFlags = CollisionWithRect(&player, xRect);
	uint32 yCollisionFlags = CollisionWithRect(&player, yRect);
	float knockBackAmount = 15;

	result = bool(xCollisionFlags | yCollisionFlags);


	if (yCollisionFlags & CollisionBot)
	{//hit enemy, apply damage to enemy
		enemy.health -= player.damage;
		player.velocity.y = knockBackAmount;
		player.invinciblityTime = currentTime + 1.0;
	}
	else
	{
		if (xCollisionFlags & CollisionRight)
		{//hit by enemy, knockback, take damage, screen flash
			player.velocity.x = -50 * knockBackAmount;
			player.velocity.y = knockBackAmount;
		}
		if (xCollisionFlags & CollisionLeft)
		{//hit by enemy, knockback, take damage, screen flash
			player.velocity.x = 50 * knockBackAmount;
			player.velocity.y = knockBackAmount;
		}
	}

	if ((xCollisionFlags || yCollisionFlags) && player.invinciblityTime <= currentTime)
	{
		player.health -= enemy.damage;
	}

	if (result && player.invinciblityTime <= currentTime)
		player.invinciblityTime = currentTime + 1.0;
	return result;
}


void UpdateActorHealth(Actor* actor, double currentTime)
{
	if (actor->inUse)
	{
		if (actor->invinciblityTime <= currentTime)
		{
			if (actor->health <= 0)
				actor->health = 0;
		}
		if (!actor->health)
		{
		    actor->inUse = false;
		}
	}
}
void UpdateEnemyHealth(float totalTime)
{
	for (int32 i = 0; i < currentLevel.enemyList.size(); i++)
	{
		Enemy* enemy = &currentLevel.enemyList[i];
		UpdateActorHealth(enemy, totalTime);
	}
}


Actor* CreateBullet(Actor* player, Sprite* sprite, Vector mouseLoc, TileType blockToBeType)
{
	Projectile* bullet_a = new Projectile();
	Projectile& bullet = *bullet_a;

	bullet.paintType = blockToBeType;
	bullet.inUse = true;
	bullet.idle.anime[0] = sprite;
	bullet.terminalVelocity = { 1000, 1000 };
	Vector adjustedPlayerPosition = { player->position.x/* + 0.5f*/, player->position.y + 1 };

	float playerBulletRadius = 0.5f; //half a block
	bullet.destination = mouseLoc;
	bullet.rotation = Atan2fToDegreeDiff(atan2f(bullet.destination.y - adjustedPlayerPosition.y, bullet.destination.x - adjustedPlayerPosition.x));

	float speed = 50.0f;

	Vector ToDest = bullet.destination - adjustedPlayerPosition;

	ToDest = Normalize(ToDest);
	bullet.velocity = ToDest * speed;

	bullet.position = adjustedPlayerPosition + (ToDest * playerBulletRadius);

	return bullet_a;

#if 0
	bool notCached = true;
	for (int32 i = 0; i < bulletList.size(); i++)
	{
		if (!bulletList[i].inUse)
		{
			notCached = false;
			bulletList[i] = bullet;
			break;
		}
	}
	if (notCached)
		bulletList.push_back(bullet);
#endif
}


void CreateLaser(Actor* player, Sprite* sprite, Vector mouseLoc, TileType paintType)
{
	laser.paintType = paintType;
	laser.inUse = true;
	laser.run.anime.push_back(sprite);
	Vector adjustedPlayerPosition = { player->position.x + 0.5f, player->position.y + 1 };

	float playerBulletRadius = 0.5f; //half a block
	laser.destination = mouseLoc;
	laser.rotation = Atan2fToDegreeDiff(atan2f(laser.destination.y - adjustedPlayerPosition.y, laser.destination.x - adjustedPlayerPosition.x));

	Vector ToDest = Normalize(laser.destination - adjustedPlayerPosition);

	laser.position = adjustedPlayerPosition + (ToDest * playerBulletRadius);
}


void UpdateLocation(Actor* actor, float gravity, float deltaTime)
{
	actor->velocity.y += gravity * deltaTime;
	actor->velocity.y = Clamp(actor->velocity.y, -actor->terminalVelocity.y, actor->terminalVelocity.y);

	actor->velocity.x += actor->acceleration.x * deltaTime;
	//actor->velocity.x -= ((0.20f) * (actor->velocity.x / 2.0f));


	actor->velocity.x = Clamp(actor->velocity.x, -actor->terminalVelocity.x, actor->terminalVelocity.x);

	actor->position += actor->velocity * deltaTime;

	if (actor->velocity.x < 0)
		actor->lastInputWasLeft = true;
	else if (actor->velocity.x > 0)
		actor->lastInputWasLeft = false;
}




void UpdateActors(float deltaTime)
{
	for (int32 i = 0; i < actorList.size(); i++)
	{
		Actor* actor = actorList[i];
		actor->Update(deltaTime);
	}
}

void UpdateEnemiesPosition(float gravity, float deltaTime)
{
	for (int32 i = 0; i < currentLevel.enemyList.size(); i++)
	{
		Enemy* enemyPointer = &currentLevel.enemyList[i];

		UpdateLocation(enemyPointer, gravity, deltaTime);
		CollisionWithBlocks(enemyPointer, true);
	}
}

void RenderActors(double totalTime)
{
	for (int32 i = 0; i < actorList.size(); i++)
	{
		actorList[i]->Render(totalTime);
	}
}

void RenderActorHealthBars(Actor& actor)
{
	int32 healthHeight = 8;
	Rectangle full = {};
	Rectangle actual = {};
	full.bottomLeft.x = actor.position.x;
	full.bottomLeft.y = actor.position.y + PixelToBlock(int(actor.ScaledHeight() + healthHeight));
	full.topRight.x = full.bottomLeft.x + PixelToBlock(int(actor.ScaledWidth()));
	full.topRight.y = full.bottomLeft.y + healthHeight;

	actual.bottomLeft.x = full.topRight.x - (actor.health / 100) * full.Width();
	actual.bottomLeft.y = full.bottomLeft.y;
	actual.topRight = full.topRight;

	GameSpaceRectRender(full, HealthBarBackground);
	GameSpaceRectRender(actual, Green);
}