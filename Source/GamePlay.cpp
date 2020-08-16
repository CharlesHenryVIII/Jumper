#include "GamePlay.h"
#include "Rendering.h"
#include "Entity.h"
#include "TiledInterop.h"
#include "WinUtilities.h"
#include "Misc.h"
#include "Menu.h"

Portal* levelChangePortal;
Level cacheLevel;
float playerAccelerationAmount;
Actor* actorPlayer;
bool paused;
Vector mouseLocBlocks;


void SwitchToGame()
{
    levelChangePortal = nullptr;
    cacheLevel = {};
    cacheLevel.filename = "Level.PNG";
    playerAccelerationAmount = 50;
    currentLevel = GetLevel("Default");
    currentLevel->blocks.UpdateAllBlocks();
    gameState = GameState::game;
    actorPlayer = nullptr;
    actorPlayer = FindPlayer(*currentLevel);
    paused = false;
    mouseLocBlocks = {};
}


void DoPlayGame(float deltaTime, std::unordered_map<int32, Key>& keyStates, VectorInt mouseLocation)
{

    if (paused)
    {
        if (DrawButton(fonts["1"], "QUIT TO MAIN MENU", { windowInfo.width / 2, windowInfo.height / 4 }, 
                        UIX::mid, UIY::mid, Green, White, mouseLocation, keyStates[SDL_BUTTON_LEFT].downThisFrame)
            || keyStates[SDLK_RETURN].downThisFrame)
            SwitchToMenu();
        deltaTime = 0.0f;
        AddRectToRender({ {0, (float)windowInfo.height}, { (float)windowInfo.width, 0 } }, lightBlack, RenderPrio::foreground, CoordinateSpace::UI);
    }

    //TODO: fix creating a new player makes a new player ID number;
    if (FindPlayer(*currentLevel) == nullptr)
    {
        DebugPrint("player not found");
        //playerID = CreateActor(ActorType::player, ActorType::none, totalTime);
        //LoadLevel(currentLevel, *(Player*)FindActor(playerID));
    }

    Player* player = FindPlayer(*currentLevel);


	/*********************
	*
	* Mouse Control
	*
	********/

    {

		//1
		Vector mouseLocationFlipped = { (float)mouseLocation.x, float(windowInfo.height - mouseLocation.y) };
		//2
		float blocksPerPixels = camera.size.x / windowInfo.width;
		Vector mouseLocationInBlocks = mouseLocationFlipped * blocksPerPixels;
		//3
		Vector windowCenterInBlocks = { windowInfo.width / 2 * blocksPerPixels, windowInfo.height / 2 * blocksPerPixels };
		mouseLocBlocks = (mouseLocationInBlocks - windowCenterInBlocks) + camera.position;
    }

    /*********************
     *
     * Keyboard Control
     *
     ********/

    if (player != nullptr)
    { 
        player->acceleration.x = 0;
        if (keyStates[SDLK_w].downThisFrame || keyStates[SDLK_SPACE].downThisFrame || keyStates[SDLK_UP].downThisFrame)
        {
            if (player->jumpCount > 0)
            {
                player->velocity.y = 20.0f;
                player->jumpCount -= 1;
                PlayAnimation(player, ActorState::jump);
            }
        }

        bool lDown = keyStates[SDLK_a].down || keyStates[SDLK_LEFT].down;
        //bool lDownThisFrame = keyStates[SDLK_a].downThisFrame || keyStates[SDLK_LEFT].downThisFrame;
        bool lUpThisFrame = keyStates[SDLK_a].upThisFrame || keyStates[SDLK_LEFT].upThisFrame;
        bool rDown = keyStates[SDLK_d].down || keyStates[SDLK_RIGHT].down;
        //bool rDownThisFrame = keyStates[SDLK_d].downThisFrame || keyStates[SDLK_RIGHT].downThisFrame;
        bool rUpThisFrame = keyStates[SDLK_d].upThisFrame || keyStates[SDLK_RIGHT].upThisFrame;

        Actor* playerParent = FindActor(player->parent, *currentLevel); //needs nullptr check when using
        float xVelOffset = 0;

        if (lDown != rDown)
        {
            if (lDown)
                player->lastInputWasLeft = true;
            else
                player->lastInputWasLeft = false;
        }

        //TODO: Clunky clean up code
        //Set player velocity to the block they are on so they can be moved on a moving platform...
        //if the player is grounded and velocity is greater than terminal velocity then decelerate to the terminal velocity.
        //else continue to accelerate the player;
        if (playerParent)
        {
            xVelOffset = playerParent->velocity.x;
        }
        if ((lDown && rUpThisFrame) || (rDown && lUpThisFrame))
        {
            player->velocity.x = xVelOffset + 0;
            player->acceleration.x = 0;

        }
        else if (lDown == rDown && player->velocity.x < xVelOffset + 15)
        {

            if ((lUpThisFrame && !rDown) || (rUpThisFrame && !rDown) || fabsf(player->velocity.x) <= xVelOffset + 1)
            {
                player->velocity.x = xVelOffset + 0;
                player->acceleration.x = 0;
            }

            else if (player->velocity.x > xVelOffset + 0)
                player->acceleration.x -= playerAccelerationAmount;
            else if (player->velocity.x < -xVelOffset + 0)
                player->acceleration.x += playerAccelerationAmount;
        }
        else
        {

            if (player->grounded)
            {
                if (player->velocity.x > player->terminalVelocity.x + xVelOffset)
                    player->acceleration.x -= playerAccelerationAmount * 4;
                else if (player->velocity.x < -(player->terminalVelocity.x))
                    player->acceleration.x += playerAccelerationAmount * 4;

                else if (lDown && player->velocity.x >= -(player->terminalVelocity.x + xVelOffset))
                    player->acceleration.x -= playerAccelerationAmount;
                else if (rDown && player->velocity.x <= xVelOffset + player->terminalVelocity.x)
                    player->acceleration.x += playerAccelerationAmount;
            }
            else
            {

                float airAccelerationAmount = 5;
                if (lDown)
                {
                    if (lDown && player->velocity.x >= -player->terminalVelocity.x)
                        player->acceleration.x -= playerAccelerationAmount;
                    else
                        player->acceleration.x -= airAccelerationAmount;
                }

                if (rDown)
                {

                    if (player->velocity.x <= player->terminalVelocity.x)
                        player->acceleration.x += playerAccelerationAmount;
                    else
                        player->acceleration.x += airAccelerationAmount;
                }
            }
        }
    

        if (player->grappleDeployed)
        {
            //update grapple and player
        }
        else
        {
            if (keyStates[SDL_BUTTON_LEFT].downThisFrame && player->grappleEnabled && player->grappleReady)
            {
                //spawn grapple
                
                Grapple* grapple = new Grapple();
                grapple->inUse = true;

                AttachAnimation(grapple);
                PlayAnimation(grapple, ActorState::idle);
                Sprite* sprite = GetSpriteFromAnimation(grapple);
                grapple->animationList->colRect = { {0,0}, {sprite->width,sprite->height} };
                grapple->animationList->scaledWidth = (float)sprite->width;
                grapple->terminalVelocity = { 1000, 1000 };

                Vector adjustedPlayerPosition = { player->position.x, player->position.y + 1 };
                Vector playerPosAdjusted = { adjustedPlayerPosition.x + (player->GameWidth() / 2), adjustedPlayerPosition.y };
                //grapple->destination = PixelToBlock(CameraToPixelCoord(mouseLocation));
                grapple->rotation = Atan2fToDegreeDiff(atan2f(grapple->destination.y - adjustedPlayerPosition.y, grapple->destination.x - adjustedPlayerPosition.x));

                float speed = 10.0f;
                Vector ToDest = grapple->destination - playerPosAdjusted;
                ToDest = Normalize(ToDest);
                grapple->velocity = ToDest * speed;
                Vector gameSize = { grapple->GameWidth(), grapple->GameHeight() };
                float playerBulletRadius = 0.5f; //half a block
                grapple->position = adjustedPlayerPosition + (ToDest * playerBulletRadius);
                currentLevel->actors.push_back(grapple);

                player->grappleReady = false;
                player->grappleDeployed = true;
            }
        }

        if (keyStates[SDLK_q].downThisFrame)
            player->grappleEnabled = !player->grappleEnabled;
        if (keyStates[SDLK_0].downThisFrame)
            SaveLevel(&cacheLevel, *player);
        if (keyStates[SDLK_9].downThisFrame)
            LoadLevel(&cacheLevel, *player);
    }


    if (keyStates[SDLK_1].downThisFrame)
        debugList[DebugOptions::playerCollision] = !debugList[DebugOptions::playerCollision];
    if (keyStates[SDLK_2].downThisFrame)
        debugList[DebugOptions::blockCollision] = !debugList[DebugOptions::blockCollision];
    if (keyStates[SDLK_3].downThisFrame)
        debugList[DebugOptions::clickLocation] = !debugList[DebugOptions::clickLocation];
    if (keyStates[SDLK_4].downThisFrame)
        debugList[DebugOptions::paintMethod] = !debugList[DebugOptions::paintMethod];
    if (keyStates[SDLK_5].downThisFrame)
        debugList[DebugOptions::editBlocks] = !debugList[DebugOptions::editBlocks];

    if (levelChangePortal != nullptr && keyStates[SDLK_w].downThisFrame)
    {
        //remove player from new level, load player from old level, delete player from old level.
        Level* oldLevel = currentLevel;
        currentLevel = GetLevel(levelChangePortal->levelPointer);

        std::erase_if(currentLevel->actors, [](Actor* actor)
        {
            if (actor->GetActorType() == ActorType::player)
            {
                delete actor;
                return true;
            }
            else
                return false;
        });

        currentLevel->actors.push_back(player);
        std::erase_if(oldLevel->actors, [](Actor* actor)
        {
            return actor->GetActorType() == ActorType::player;
        });
        player->position = GetPortalsPointer(levelChangePortal)->position;
    }
    levelChangePortal = nullptr;



    /*********************
     *
     * Mouse Control
     *
     ********/

    laser.inUse = false;
    if (debugList[DebugOptions::editBlocks] && (keyStates[SDL_BUTTON_LEFT].down || keyStates[SDL_BUTTON_RIGHT].down) && player != nullptr)
    {

        
        //PixelToBlock(mouseLocationFlipped);
        //Vector mouseLocationTranslated = /*{ mousePixelCoord.x * blocksPerPixel, mousePixelCoord.y * blocksPerPixel};*/PixelToBlock(CameraToPixelCoord(mouseLocation));

        Rectangle clickRect = {};
        clickRect.botLeft = { mouseLocBlocks.x - 0.5f, mouseLocBlocks.y - 0.5f };
        clickRect.topRight = { mouseLocBlocks.x + 0.5f, mouseLocBlocks.y + 0.5f };
        Block* blockPointer = &currentLevel->blocks.GetBlock(mouseLocBlocks);
        blockPointer->location = mouseLocBlocks;
        blockPointer->location.x = floorf(blockPointer->location.x);
        blockPointer->location.y = floorf(blockPointer->location.y);

        TileType paintType;
        if (keyStates[SDL_BUTTON_LEFT].down)
            paintType = TileType::filled;
        else if (keyStates[SDL_BUTTON_RIGHT].down)
            paintType = TileType::invalid;

        if (debugList[DebugOptions::paintMethod])
        {
            UpdateLaser(player, mouseLocBlocks, paintType, deltaTime);
            blockPointer->tileType = paintType;
            UpdateAllNeighbors(blockPointer);
        }
        else if (keyStates[SDL_BUTTON_LEFT].downThisFrame || keyStates[SDL_BUTTON_RIGHT].downThisFrame)
        {
            // TODO: remove the cast
            currentLevel->actors.push_back(CreateBullet(player, mouseLocBlocks, paintType));
        }
    }

    /*********************
     *
     * Updates
     *
     ********/



    for (int32 i = 0; i < currentLevel->actors.size(); i++)
        currentLevel->actors[i]->Update(deltaTime);

    if (player != nullptr)
    {
        for (int32 i = 0; i < currentLevel->actors.size(); i++)
        {
            Actor* actor =  currentLevel->actors[i];
            switch (actor->GetActorType())
            {
            case ActorType::enemy:
            {

                //flash screen 
                if (CollisionWithActor(*player, *actor, *currentLevel))
                    AddRectToRender({ {0, (float)windowInfo.height}, { (float)windowInfo.width, 0 } }, lightWhite, RenderPrio::foreground, CoordinateSpace::UI);
                break;
            }
            case ActorType::portal:
            {

                if (CollisionWithActor(*player, *actor, *currentLevel))
                    levelChangePortal = (Portal*)actor;
                break;
            }
            case ActorType::spring:
            {

                uint32 result = CollisionWithActor(*player, *actor, *currentLevel);
                if (result > 0)
                {
                    Spring* spring = (Spring*)actor;
                    if (CollisionLeft & result)
                        player->position.x = spring->position.x + spring->GameWidth();
                    else if (CollisionRight & result)
                        player->position.x = spring->position.x - player->GameWidth();
                    else if (CollisionBot & result)
                        player->velocity.y = spring->springVelocity.y;
                }
                break;
            }
            //case ActorType::movingPlatform:
            //{

            //	//TODO: determine how to update the actors x position with the moving platform
            //	uint32 result = CollisionWithActor(*player, *actor, *currentLevel);
            //	if (result > 0)
            //	{

            //		player->parent = actor->id;
            //		actor->children.push_back(player->id);
            //		MovingPlatform* MP = (MovingPlatform*)actor;
            //		if (CollisionBot & result)
            //		{

            //			player->grounded = true;
            //			player->jumpCount = 2;
            //			player->position.y = MP->position.y + MP->GameHeight();
            //			player->velocity.y = MP->velocity.y;

            //			//if (player->velocity.x != 0)
            //			//	PlayAnimation(player, ActorState::run);
            //			//else
            //			//	PlayAnimation(player, ActorState::idle);
            //		}

            //		if (CollisionLeft & result)
            //		{
            //			player->position.x = MP->position.x + MP->GameWidth();
            //			player->velocity.x = MP->velocity.x;
            //		}
            //		if (CollisionRight & result)
            //		{
            //			player->position.x = MP->position.x - player->GameWidth();
            //			player->velocity.x = MP->velocity.x;
            //		}
            //		if (CollisionTop & result)
            //		{
            //			player->position.y = MP->position.y - player->GameHeight();
            //			if (actor->velocity.y > 0)
            //				actor->velocity.y = 0;
            //		}
            //	}
            //	else
            //	{
            //		if (player->parent == actor->id)
            //		{

            //			player->parent = 0;
            //			for (int32 i = 0; i < actor->children.size(); i++)
            //			{
            //				if (actor->children[i] == player->id)
            //					actor->children[i] = 0;
            //			}
            //			player->grounded = false;
            //		}
            //	}
            //	break;
            //}
            }
        }
        
        for (int32 i = 0; i < currentLevel->movingPlatforms.size(); i++)
        {
            currentLevel->movingPlatforms[i]->Update(deltaTime);
        }
    }



    /*********************
     *
     * Renderer
     *
     ********/


    BackgroundRender(sprites["background"], &camera);
    if (player != nullptr)
        camera.position = player->position;
    
    RenderBlocks();
    RenderActors();
    RenderLaser();
    RenderMovingPlatforms();

    if (player != nullptr && player->grappleEnabled)
    {

        if (player->grappleReady)
            DrawText(fonts["1"], Green, "Grapple Ready", 1.0f, { windowInfo.width / 2, windowInfo.height }, UIX::mid, UIY::bot);
        else
            DrawText(fonts["1"], Red, "Grapple Ready", 1.0f, { windowInfo.width / 2, windowInfo.height }, UIX::mid, UIY::bot);
    }

    DrawText(fonts["1"], Green, std::to_string(1 / deltaTime) + "fps", 1.0f, { 0, 0 }, UIX::left, UIY::top);
    if (player != nullptr)
    {
        DrawText(fonts["1"], Green, "{ " + std::to_string(player->position.x) + ", " + std::to_string(player->position.y) + " }", 0.75f, { 0, windowInfo.height - 40 }, UIX::left, UIY::bot);
        DrawText(fonts["1"], Green, "{ " + std::to_string(player->velocity.x) + ", " + std::to_string(player->velocity.y) + " }", 0.75f, { 0, windowInfo.height - 20 }, UIX::left, UIY::bot);
        DrawText(fonts["1"], Green, "{ " + std::to_string(player->acceleration.x) + ", " + std::to_string(player->acceleration.y) + " }", 0.75f, { 0, windowInfo.height }, UIX::left, UIY::bot);

    }

    if (DrawButton(fonts["1"], "ESC", { windowInfo.width, windowInfo.height }, 
                    UIX::right, UIY::bot, Red, White, mouseLocation, keyStates[SDL_BUTTON_LEFT].downThisFrame)
                    || keyStates[SDLK_ESCAPE].downThisFrame)
        paused = !paused;

    //Present Screen
    std::erase_if(currentLevel->actors, [](Actor* p) {
        if (!p->inUse)
        {
            delete p;
            return true;
        }
        return false;
    });
}
