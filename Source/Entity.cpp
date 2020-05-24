#include "Entity.h"
#include "Rendering.h"
#include "stb/stb_image.h"
#include "stb/stb_image_write.h"

TileMap tileMap;
Actor player;
Projectile laser = {};
std::vector<Projectile> bulletList = {};
Level currentLevel;
static const Vector terminalVelocity = { 10 , 300 };


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


void SaveLevel(Level* level)
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


void LoadLevel(Level* level)
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
	result.bottomLeft = { actor->position.x + actor->colOffset.x * PixelToBlock(actor->sprite->width), actor->position.y + actor->colOffset.x * PixelToBlock(actor->sprite->height) };
	result.topRight = { actor->position.x + (1 - actor->colOffset.x) * PixelToBlock(actor->sprite->width), actor->position.y + (1 - actor->colOffset.x) * PixelToBlock(actor->sprite->height) };
	return result;
}
Rectangle CollisionYOffsetToRectangle(Actor* actor)
{
	Rectangle result = {};
	result.bottomLeft = { actor->position.x + actor->colOffset.y * PixelToBlock(actor->sprite->width), actor->position.y };
	result.topRight = { actor->position.x + (1 - actor->colOffset.y) * PixelToBlock(actor->sprite->width), actor->position.y + PixelToBlock(actor->sprite->height) };
	return result;
}



uint32 CollisionWithRect(Actor* actor, Rectangle rect)
{
	uint32 result = CollisionNone;

	Rectangle xRect = CollisionXOffsetToRectangle(actor);
	Rectangle yRect = CollisionYOffsetToRectangle(actor);

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

		if (collisionFlags & CollisionRight)
		{
			actor->position.x = block.second.location.x - (1 - actor->colOffset.x) * PixelToBlock(actor->sprite->width);
			if (isEnemy)
				actor->velocity.x = -1 * actor->velocity.x;
			else
				actor->velocity.x = 0;
		}
		if (collisionFlags & CollisionLeft)
		{
			actor->position.x = (block.second.location.x + 1) - actor->colOffset.x * PixelToBlock(actor->sprite->width);
			if (isEnemy)
				actor->velocity.x = -1 * actor->velocity.x;
			else
				actor->velocity.x = 0;
		}
		if (collisionFlags & CollisionTop)
		{
			actor->position.y = block.second.location.y - PixelToBlock(actor->sprite->height);
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


bool CollisionWithEnemy(Actor* actor, float currentTime)
{
	bool result = false;
	for (int32 i = 0; i < currentLevel.enemyList.size(); i++)
	{
		Enemy* enemy = &currentLevel.enemyList[i];
		if (!enemy->inUse)
			continue;

		Rectangle xRect = CollisionXOffsetToRectangle(enemy);
		Rectangle yRect = CollisionYOffsetToRectangle(enemy);

		uint32 xCollisionFlags = CollisionWithRect(&player, xRect);
		uint32 yCollisionFlags = CollisionWithRect(&player, yRect);
		float knockBackAmount = 15;

		result = bool(xCollisionFlags | yCollisionFlags);


		if (yCollisionFlags & CollisionBot)
		{//hit enemy, apply damage to enemy
			enemy->health -= actor->damage;
			actor->invincible = currentTime;
			actor->velocity.y = knockBackAmount;
		}
		else
		{
			if (xCollisionFlags & CollisionRight)
			{//hit by enemy, knockback, take damage, screen flash
				actor->velocity.x = -50 * knockBackAmount;
				actor->velocity.y = knockBackAmount;
			}
			if (xCollisionFlags & CollisionLeft)
			{//hit by enemy, knockback, take damage, screen flash
				actor->velocity.x = 50 * knockBackAmount;
				actor->velocity.y = knockBackAmount;
			}
		}

		if ((xCollisionFlags || yCollisionFlags) && !actor->invincible)
		{
			actor->health -= enemy->damage;
		}

		if (result && !actor->invincible)
			actor->invincible = currentTime;
	}
	return result;
}


void UpdateActorHealth(Actor* actor, float currentTime)
{
	if (actor->inUse)
	{
		if (actor->invincible + 1 <= currentTime)
			if (actor->health <= 0)
				actor->health = 0;

		//if (!actor->health)
		//{
		//    actor->inUse = false;
		//}
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


void CreateBullet(Actor* player, Sprite* bulletSprite, Vector mouseLoc, TileType blockToBeType)
{
	Projectile bullet = {};
	bullet.paintType = blockToBeType;
	bullet.inUse = true;
	bullet.sprite = bulletSprite;
	Vector adjustedPlayerPosition = { player->position.x/* + 0.5f*/, player->position.y + 1 };

	float playerBulletRadius = 0.5f; //half a block
	bullet.destination = mouseLoc;
	bullet.rotation = Atan2fToDegreeDiff(atan2f(bullet.destination.y - adjustedPlayerPosition.y, bullet.destination.x - adjustedPlayerPosition.x));

	float speed = 50.0f;

	Vector ToDest = bullet.destination - adjustedPlayerPosition;

	ToDest = Normalize(ToDest);
	bullet.velocity = ToDest * speed;

	bullet.position = adjustedPlayerPosition + (ToDest * playerBulletRadius);


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
}


void CreateLaser(Actor* player, Sprite* sprite, Vector mouseLoc, TileType paintType)
{
	laser.paintType = paintType;
	laser.inUse = true;
	laser.sprite = sprite;
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
	actor->velocity.y = Clamp(actor->velocity.y, -terminalVelocity.y, terminalVelocity.y);

	actor->velocity.x += actor->acceleration.x * deltaTime;
	actor->velocity.x -= ((0.20f) * (actor->velocity.x / 2.0f));


	actor->velocity.x = Clamp(actor->velocity.x, -terminalVelocity.x, terminalVelocity.x);

	actor->position += actor->velocity * deltaTime;
}




void UpdateBullets(float deltaTime)
{
	for (int32 i = 0; i < bulletList.size(); i++)
	{
		Projectile* bullet = &bulletList[i];

		if (bullet->inUse)
		{
			UpdateLocation(bullet, 0, deltaTime);
			if (DotProduct(bullet->velocity, bullet->destination - bullet->position) < 0)
			{
				//paint block and remove bullet
				Block* blockPointer = &tileMap.GetBlock(bullet->destination);
				blockPointer->tileType = bullet->paintType;
				UpdateAllNeighbors(blockPointer);
				bullet->inUse = false;
			}
		}
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

void RenderBullets()
{
	for (int32 i = 0; i < bulletList.size(); i++)
	{
		Projectile* bullet = &bulletList[i];
		if (bullet->inUse)
		{
			SDL_Color PC = {};
			if (bullet->paintType == TileType::filled)
				PC = Green;
			else
				PC = Red;
			SDL_SetTextureColorMod(bullet->sprite->texture, PC.r, PC.g, PC.b);
			RenderActor(bullet, bullet->rotation);
		}
	}
}

void RenderEnemies()
{
	for (int32 i = 0; i < currentLevel.enemyList.size(); i++)
	{
		Enemy* enemyPointer = &currentLevel.enemyList[i];
		RenderActor(enemyPointer, 0);
	}
}