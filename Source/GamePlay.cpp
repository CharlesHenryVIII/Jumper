#include "GamePlay.h"
#include "Rendering.h"
#include "Entity.h"
#include "JSONInterop.h"
#include "WinUtilities.h"
#include "Misc.h"
#include "Menu.h"
#include "Console.h"
#include "Audio.h"

Portal* s_levelChangePortal;
bool s_paused;
Gamestate g_gamestate;


Level* Gamestate::GetLevel(const std::string& name)
{
	if (levels.find(name) != levels.end())
	{
		return &levels[name];
	}

	LoadLevel(&g_gamestate.levels[name], name);
    return &g_gamestate.levels[name];
}

void SwitchToGame()
{
    ConsoleLog("Switched To Game");
    g_gamestate = {};
    s_levelChangePortal = nullptr;
    g_applicationState = ApplicationState::Game;
    s_paused = false;

    g_gamestate.currentLevel = g_gamestate.GetLevel("Default");
    Spawner* sp = g_gamestate.currentLevel->FindActor<Spawner>();
    g_gamestate.currentLevel->CreateActor<Player>()->position = sp->position;
}




void DoPlayGame(float deltaTime, std::unordered_map<int32, Key>& keyStates, VectorInt mouseLocation)
{

    if (s_paused)
    {
        if (DrawButton(g_fonts["Main"], "QUIT TO MAIN MENU", { g_windowInfo.width / 2, g_windowInfo.height / 4 }, 
                        UIX::mid, UIY::mid, Green, White, mouseLocation, keyStates[SDL_BUTTON_LEFT].downThisFrame)
            || keyStates[SDLK_RETURN].downThisFrame)
            SwitchToMenu();
        deltaTime = 0.0f;
        AddRectToRender({ {0, (float)g_windowInfo.height}, { (float)g_windowInfo.width, 0 } }, lightBlack, RenderPrio::foreground, CoordinateSpace::UI);
    }

    //TODO: fix creating a new player makes a new player ID number;
    //if (FindPlayerGlobal() == nullptr)
    //{
    //    ConsoleLog("player not found");
    //}




	/*********************
	*
	* Mouse Control
	*
	********/

    Vector mouseLocBlocks = {};
    {

		//1
		Vector mouseLocationFlipped = { (float)mouseLocation.x, float(g_windowInfo.height - mouseLocation.y) };
		//2
		float blocksPerPixels = g_camera.size.x / g_windowInfo.width;
		Vector mouseLocationInBlocks = mouseLocationFlipped * blocksPerPixels;
		//3
		Vector windowCenterInBlocks = { g_windowInfo.width / 2 * blocksPerPixels, g_windowInfo.height / 2 * blocksPerPixels };
		mouseLocBlocks = (mouseLocationInBlocks - windowCenterInBlocks) + g_camera.position;
    }

	if (keyStates[SDLK_1].downThisFrame)
		g_debugList[DebugOptions::PlayerCollision] = !g_debugList[DebugOptions::PlayerCollision];
	if (keyStates[SDLK_2].downThisFrame)
		g_debugList[DebugOptions::BlockCollision] = !g_debugList[DebugOptions::BlockCollision];
	if (keyStates[SDLK_3].downThisFrame)
		g_debugList[DebugOptions::ClickLocation] = !g_debugList[DebugOptions::ClickLocation];
	if (keyStates[SDLK_4].downThisFrame)
		g_debugList[DebugOptions::PaintMethod] = !g_debugList[DebugOptions::PaintMethod];
	if (keyStates[SDLK_5].downThisFrame)
		g_debugList[DebugOptions::EditBlocks] = !g_debugList[DebugOptions::EditBlocks];



    /*********************
     *
     * Keyboard Control
     *
     ********/

    Player* player = g_gamestate.currentLevel->FindPlayer();
    if (player)
    {
        player->acceleration.x = 0;
        if (keyStates[SDLK_w].downThisFrame || keyStates[SDLK_SPACE].downThisFrame || keyStates[SDLK_UP].downThisFrame)
        {
            if (player->jumpCount > 0)
            {
                player->velocity.y = 20.0f;
                player->jumpCount -= 1;
                player->PlayAnimation(ActorState::Jump);
                PlayAudio("Jump");
            }
        }

        bool lDown = keyStates[SDLK_a].down || keyStates[SDLK_LEFT].down;
        //bool lDownThisFrame = keyStates[SDLK_a].downThisFrame || keyStates[SDLK_LEFT].downThisFrame;
        bool lUpThisFrame = keyStates[SDLK_a].upThisFrame || keyStates[SDLK_LEFT].upThisFrame;
        bool rDown = keyStates[SDLK_d].down || keyStates[SDLK_RIGHT].down;
        //bool rDownThisFrame = keyStates[SDLK_d].downThisFrame || keyStates[SDLK_RIGHT].downThisFrame;
        bool rUpThisFrame = keyStates[SDLK_d].upThisFrame || keyStates[SDLK_RIGHT].upThisFrame;

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
        float playerAccelerationAmount = 50;
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


		if (player->grapple == ActorID::Invalid && (keyStates[SDL_BUTTON_LEFT].downThisFrame) && player->grappleEnabled && player->grappleReady)
		{
			//spawn grapple

			GrappleInfo info = {};
			info.actorID = player->id;
			info.mouseLoc = mouseLocBlocks;
			player->grapple = player->level->CreateActor<Grapple>(info)->id;
		}
        else if (player->grapple != ActorID::Invalid)
        {

            Grapple* grapple = player->level->FindActor<Grapple>(player->grapple);
            if (keyStates[SDL_BUTTON_LEFT].upThisFrame && grapple)
            {

				//TODO: Handle grapple not being deleted when going through a portal
				if (grapple->grappleState == GrappleState::Attached)
					player->switchToLinearUpdate = true;
				grapple->grappleState = GrappleState::Retracting;
				player->angularUpdate = false;
			}
		}
		if (keyStates[SDLK_q].downThisFrame)
			player->grappleEnabled = !player->grappleEnabled;
#if 0
		if (keyStates[SDLK_0].downThisFrame)
			SaveLevel(&cacheLevel, *player);
		if (keyStates[SDLK_9].downThisFrame)
			LoadLevel(&cacheLevel, *player);
#endif
		if (s_levelChangePortal != nullptr && keyStates[SDLK_w].downThisFrame)
		{
			//remove player from new level, load player from old level, delete player from old level.
			Level* oldLevel = s_levelChangePortal->level;
			Level* newLevel = g_gamestate.GetLevel(s_levelChangePortal->levelPointer);
			if (oldLevel && newLevel)
			{


				newLevel->AddActor(player);
				std::erase_if(oldLevel->actors, [player](Actor* actor)
				{
					return actor == player;
				});
				player->position = GetPortalsPointer(s_levelChangePortal)->position;
			}
		}
		s_levelChangePortal = nullptr;

		laser.inUse = false;
		if (g_debugList[DebugOptions::EditBlocks] && (keyStates[SDL_BUTTON_LEFT].down || keyStates[SDL_BUTTON_RIGHT].down) && player != nullptr)
		{
			Rectangle clickRect = {};
			clickRect.botLeft = { mouseLocBlocks.x - 0.5f, mouseLocBlocks.y - 0.5f };
			clickRect.topRight = { mouseLocBlocks.x + 0.5f, mouseLocBlocks.y + 0.5f };
			Block* blockPointer = &player->level->blocks.GetBlock(mouseLocBlocks);
			blockPointer->location = mouseLocBlocks;
			blockPointer->location.x = floorf(blockPointer->location.x);
			blockPointer->location.y = floorf(blockPointer->location.y);

			TileType paintType;
			if (keyStates[SDL_BUTTON_LEFT].down)
				paintType = TileType::filled;
			else if (keyStates[SDL_BUTTON_RIGHT].down)
				paintType = TileType::invalid;

			if (g_debugList[DebugOptions::PaintMethod])
			{
				UpdateLaser(player, mouseLocBlocks, paintType, deltaTime);
				blockPointer->tileType = paintType;
				UpdateAllNeighbors(blockPointer, player->level);
			}
			else if (keyStates[SDL_BUTTON_LEFT].downThisFrame || keyStates[SDL_BUTTON_RIGHT].downThisFrame)
			{
				ProjectileInfo info;
				info.player = player;
				info.mouseLoc = mouseLocBlocks;
				info.blockToBeType = paintType;
				player->level->CreateActor<Projectile>(info);
			}
		}

	}

    /*********************
     *
     * Updates
     *
     ********/



#if 0
	for (Actor* actor : g_gamestate.currentLevel->actors)
		actor->Update(deltaTime);
#elif 1
    
	Level* level = g_gamestate.currentLevel;
    for (Actor* actor : level->actors)
	    actor->Update(deltaTime);

	while (level->actorsToAdd.size())
	{
		std::vector<Actor*> newActors = level->actorsToAdd;

        for (Actor* actor : newActors)
        {
            level->actors.push_back(actor);
            if (actor->GetActorType() == ActorType::Dummy)
                int32 i = 0;
        }

		level->actorsToAdd.clear();
        for (Actor* actor : newActors)
			actor->Update(deltaTime);
	}



	//for (int32 j = 0; j < level->actorsToAdd.size(); j++)
	//	level->actors.push_back(level->actorsToAdd[j]);
//      if (level->actorsToAdd.size())
//      {
//          level->actorsToAdd.clear();
//          update = true;
//      }



#else
	for (int32 i = 0; i < g_gamestate.currentLevel->actors.size(); i++)
		g_gamestate.currentLevel->actors[i]->Update(deltaTime);
#endif


    if (g_gamestate.currentLevel && player)
    {
        for (Actor* actor : g_gamestate.currentLevel->actors)
        {
            switch (actor->GetActorType())
            {
            case ActorType::Enemy:
            {

                //flash screen 
                if (CollisionWithActor(*player, *actor, *player->level))
                    AddRectToRender({ {0, (float)g_windowInfo.height}, { (float)g_windowInfo.width, 0 } }, lightWhite, RenderPrio::foreground, CoordinateSpace::UI);
                break;
            }
            case ActorType::Portal:
            {

                if (CollisionWithActor(*player, *actor, *player->level))
                    s_levelChangePortal = (Portal*)actor;
                break;
            }
            case ActorType::Spring:
            {

                uint32 result = CollisionWithActor(*player, *actor, *player->level);
                if (result > 0)
                {
                    Spring* spring = (Spring*)actor;
                    if (CollisionLeft & result)
                        player->position.x = spring->position.x + spring->GameWidth();
                    else if (CollisionRight & result)
                        player->position.x = spring->position.x - player->GameWidth();
                    else if (CollisionBot & result)
                    {
                        player->velocity.y = spring->springVelocity.y;
                        player->ResetJumpCount();
                    }
                }
                break;
            }
            }
        }
    }
    
    for (ActorID ID : g_gamestate.currentLevel->movingPlatforms)
    {
        if (MovingPlatform* MP = g_gamestate.currentLevel->FindActor<MovingPlatform>(ID))
        {
			for (ActorID item : MP->childList)
			{

                Actor* child = g_gamestate.currentLevel->FindActorGeneric(item);
				assert(child);
				if (child)
					child->position += MP->deltaPosition;
			}
			MP->childList.clear();
		}
	}

    /*********************
     *
     * Renderer
     *
     ********/


    BackgroundRender(g_sprites["background"], &g_camera);
    if (player != nullptr)
    {

        g_camera.position.x = player->position.x + (player->GameWidth() / 2.0f);
        g_camera.position.y = player->position.y + (player->GameHeight() / 2.0f);
        g_camera.level = player->level;
		RenderLaser();
    }
    
    RenderBlocks(&g_gamestate.currentLevel->blocks);
    RenderActors(&g_gamestate.currentLevel->actors);
    RenderMovingPlatforms(g_gamestate.currentLevel);

    if (player != nullptr && player->grappleEnabled)
    {

        if (player->grappleReady)
            DrawText(g_fonts["Main"], Green, 1.0f, { g_windowInfo.width / 2, g_windowInfo.height }, UIX::mid, UIY::bot, RenderPrio::UI, "Grapple Ready");
        else
            DrawText(g_fonts["Main"], Red,   1.0f, { g_windowInfo.width / 2, g_windowInfo.height }, UIX::mid, UIY::bot, RenderPrio::UI, "Grapple Ready");
    }

    if (player != nullptr)
    {
        //std::string string = ToString("%f", player->position.x);
        DrawText(g_fonts["Main"], Green, 0.75f, { 0, g_windowInfo.height - 60 }, UIX::left, UIY::bot, RenderPrio::UI, "{ %07.2f, %07.2f }", player->position.x,     player->position.y);
        DrawText(g_fonts["Main"], Green, 0.75f, { 0, g_windowInfo.height - 40 }, UIX::left, UIY::bot, RenderPrio::UI, "{ %07.2f, %07.2f }", player->velocity.x,     player->velocity.y);
        DrawText(g_fonts["Main"], Green, 0.75f, { 0, g_windowInfo.height - 20 }, UIX::left, UIY::bot, RenderPrio::UI, "{ %07.2f, %07.2f }", player->acceleration.x, player->acceleration.y);
        if (player->grapple != ActorID::Invalid)
        {
			DrawText(g_fonts["Main"], Green, 0.75f, { 0, g_windowInfo.height }, UIX::left, UIY::bot, RenderPrio::UI, 
                "Angular Velocity: %07.2f", player->level->FindActor<Grapple>(player->grapple)->angularVelocity);
        }
    }

    if (DrawButton(g_fonts["Main"], "ESC", { g_windowInfo.width, g_windowInfo.height }, 
                    UIX::right, UIY::bot, Red, White, mouseLocation, keyStates[SDL_BUTTON_LEFT].downThisFrame)
                    || keyStates[SDLK_ESCAPE].downThisFrame)
        s_paused = !s_paused;

	if (Level* level = g_gamestate.currentLevel)
	{
		std::erase_if(level->actors, [](Actor* p) {
			if (!p->inUse)
			{
				delete p;
				return true;
			}
			return false;
		});
	}
}
