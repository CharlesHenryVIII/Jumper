#include "Entity.h"
#include "Rendering.h"
#include "Math.h"
#include "GamePlay.h"
#include "stb/stb_image.h"
#include "stb/stb_image_write.h"
#include "JSONInterop.h"
#include "WinUtilities.h"
#include "Misc.h"
#include "Console.h"

#include <cassert>


ActorID Actor::lastID = 0;

Projectile laser = {};
//Level* currentLevel = nullptr;
std::unordered_map<std::string, AnimationList> s_actorAnimations;


/*********************
 *
 * Actor 
 *
 ********/

void Actor::UpdateLocation(float gravity, float deltaTime)
{
	if (angularUpdate)
	{
		//Assume no parents other than grapple, velocity will always be world space
		//TODO: Refactor with new actor parenting methods
		Player* player = dynamic_cast<Player*>(this);
		assert(player);
		Grapple* grapple = player->level->FindActor<Grapple>(player->grapple);
		assert(grapple);

		Vector acceleration = { player->acceleration.x, player->acceleration.y + gravity };
		grapple->angularVelocity += LinearToAngularVelocity(grapple->position, player->Center(), acceleration * deltaTime);
		Vector tension = (Center() - grapple->position);
		float rads = atan2f(tension.y, tension.x);
		float angularPosition = rads + (grapple->angularVelocity * tau * deltaTime);

		player->position = { grapple->grappleDistance * cosf(angularPosition) - player->GameWidth() / 2.0f, grapple->grappleDistance * sinf(angularPosition) - player->GameHeight() / 2.0f };
		player->position += grapple->position;
		int test = 0;

	}
	else
	{
		if (switchToLinearUpdate)
		{

			//Assume no parents other than grapple, velocity will always be world space
			//TODO: Refactor with new actor parenting methods
			assert(GetActorType() == ActorType::Player);
			Player* player = dynamic_cast<Player*>(this);
			Grapple* grapple = player->level->FindActor<Grapple>(player->grapple);

			Vector tensionPrime = Normalize(grapple->position - grapple->shotOrigin);
			Vector tension = { tensionPrime.y, -tensionPrime.x };
			player->velocity = tension * (grapple->grappleDistance * grapple->angularVelocity * tau);

			player->switchToLinearUpdate = false;
		}
		else
		{

			velocity.y += gravity * deltaTime;
			velocity.y = Clamp(velocity.y, -terminalVelocity.y, terminalVelocity.y);

			velocity.x += acceleration.x * deltaTime;
		}

		//Velocity and position will be local for Audio and Particle actors
		deltaPosition = velocity * deltaTime;
		position += deltaPosition;

		ActorType actorType = GetActorType();
		if (velocity.x == 0 && velocity.y == 0 &&
			(actorType == ActorType::Player || actorType == ActorType::Enemy))
			PlayAnimation(ActorState::Idle);

		if (actorType != ActorType::Player)
		{
			if (velocity.x < 0)
				lastInputWasLeft = true;
			else if (velocity.x > 0)
				lastInputWasLeft = false;
		}
	}
}

gbMat4 Actor::GetWorldMatrix()
{
	gbMat4 f_rotation;
	if (rotation)
		gb_mat4_rotate(&f_rotation, { 0, 0, 1.0f }, rotation);
	else
		gb_mat4_identity(&f_rotation);

	gbMat4 f_translation;
	gb_mat4_translate(&f_translation, gbVec3({ position.x, position.y, 0 }));

	gbMat4 result;
	result = f_translation * f_rotation;
	if (parent)
	{
		if (Actor* a = level->FindActorGeneric(parent))
			result = a->GetWorldMatrix() * result;
	}

	return result;
}

Vector Actor::GetWorldPosition()
{
	gbVec4 vec = this->GetWorldMatrix().col[3];
	return { vec.x, vec.y };
}

void Actor::PlayAnimation(ActorState state)
{
	ActorType actorType = GetActorType();
	if (animationList->GetAnimation(state) == nullptr)
	{
		ActorState holdingState = ActorState::None;
		for (int32 i = 1; i < (int32)ActorState::Count; i++)
		{
			if (animationList->GetAnimation((ActorState)i) != nullptr)
			{
				holdingState = (ActorState)i;
				break;
			}
		}
		if (holdingState == ActorState::None)
		{
			//no valid animation to use
			assert(false);
			ConsoleLog("No valid animation for %s \n", std::to_string((int)GetActorType()).c_str());
			return;
		}
		state = holdingState;
	}
	if (actorState != state || state == ActorState::Jump)
	{
		actorState = state;
		index = 0;
		animationCountdown =	(1.0f / animationList->GetAnimation(state)->fps);
		UpdateAnimationIndex(this, 0);
	}
}

void Actor::AttachAnimation(ActorType overrideType)
{
	std::string entityName;
	ActorType actorReferenceType;

	if (overrideType != ActorType::None)
		actorReferenceType = overrideType;
	else
		actorReferenceType = GetActorType();

	switch (actorReferenceType)
	{
	case ActorType::Player:
	{
		entityName = "Knight"; //"Dino";
		break;
	}
	case ActorType::Enemy:
	{
		entityName = "Striker";//"HeadMinion";
		break;
	}
	case ActorType::Projectile:
	{
		entityName = "Bullet";
		break;
	}
	case ActorType::Portal:
	{
		entityName = "Portal";
		break;
	}
	case ActorType::Spring:
	{
		entityName = "Spring";
		break;
	}
	case ActorType::MovingPlatform:
	{
		entityName = "MovingPlatform";
		break;
	}
	case ActorType::Grapple:
	{
		entityName = "Grapple";
		break;
	}
	default:
		assert(false);
		ConsoleLog("AttachAnimation could not find animation list for this actor\n");
		break;
	}
	animationList = &s_actorAnimations[entityName];
}

Rectangle Actor::CollisionXOffsetToRectangle()
{
	Rectangle result = {};
	result.botLeft	= { position.x, position.y + GameHeight() * animationList->colOffset.x};
	result.topRight		= { position.x + PixelToGame((int)animationList->scaledWidth), position.y + GameHeight() * (1 - animationList->colOffset.x) };
	return result;
}

Rectangle Actor::CollisionYOffsetToRectangle()
{
	Rectangle result = {};
	result.botLeft = { position.x + (PixelToGame((int)animationList->scaledWidth) * animationList->colOffset.y), position.y };
	result.topRight   = { position.x + (PixelToGame((int)animationList->scaledWidth) * (1 - animationList->colOffset.y)), position.y + GameHeight() };
	return result;
}

uint32 Actor::CollisionWithRect(Rectangle rect)
{
	uint32 result = CollisionNone;
	Rectangle xRect = CollisionXOffsetToRectangle();
	Rectangle yRect = CollisionYOffsetToRectangle();

	if (GetActorType() == ActorType::Player && g_debugList[DebugOptions::PlayerCollision])
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


/*********************
 *
 * Player
 *
 ********/


void Player::OnInit()
{
	damage = 100;
	inUse = true;
	AttachAnimation();

	Vector coneDir;
	float offsetDeg = 60.0f;
	if (velocity.x > 0.0f)
		coneDir = RotateVector({ -velocity.x, velocity.y }, -offsetDeg);
	else
		coneDir = RotateVector({ -velocity.x, velocity.y },  offsetDeg);

    pp = {
        .spawnLocation = position,

        .colorRangeLo = { 0.50f, 0.50f, 0.50f, 0.50f },
        .colorRangeHi = { 0.75f, 0.75f, 0.75f, 0.70f },

		.coneDir = coneDir,
		.coneDeg = 10.0f,

        //.particleSize = 2,
		.particleSpeed = 5.0f,
        .lifeTime = 0.25f,
        .particlesPerSecond = 10.0f,
        //.fadeInTime = 0;
        //.fadeOutTime = 0;

        //.terminalVelocity = {};
    };

	ParticleGen* p = level->CreateActor<ParticleGen>(pp);
	p->parent = id;
	dustGenerator = p->id;
}

void Player::Update(float deltaTime)
{
	UpdateLocation(-60.0f, deltaTime);
	CollisionWithBlocks(this, false);

	invinciblityTime = Max(invinciblityTime - deltaTime, 0.0f);

	ParticleGen* g = level->FindActor<ParticleGen>(dustGenerator);
	if (g == nullptr)
	{
		g = level->CreateActor<ParticleGen>(pp);
		g->parent = id;
		dustGenerator = g->id;
	}
	assert(g);

	if (grounded)
	{
		if (velocity.x != 0)
		{
			PlayAnimation(ActorState::Run);

			if (velocity.x > 0.0f)
			{
				g->coneDir = RotateVector({ -velocity.x, velocity.y }, -60.0f);
				g->position.x = 0;
			}
			else
			{
				g->coneDir = RotateVector({ -velocity.x, velocity.y }, 60.0f);
				g->position.x = GameWidth();
			}

			g->playing = true;
		}
		else
		{
			PlayAnimation(ActorState::Idle);
			g->playing = false;
		}
	}
	else
		g->playing = false;
		
	UpdateAnimationIndex(this, deltaTime);

	timeToMakeSound += deltaTime;
	float t = 2.0f / (fabs(velocity.x));//0.5f;
	if (timeToMakeSound >= t && g_gameState == GameState::Game && grounded)
	{
		AudioParams params = {
	.nameOfSound = "Grass",
		};
		AudioPlayer* ap = level->CreateActor<AudioPlayer>(params);
		ap ->parent = id;

		timeToMakeSound = 0.0f;//timeToMakeSound - t;
	}
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
	AttachAnimation();
    PlayAnimation(ActorState::Walk);

	Vector coneDir;
	float offsetDeg = 60.0f;
	if (velocity.x > 0.0f)
		coneDir = RotateVector({ -velocity.x, velocity.y }, -offsetDeg);
	else
		coneDir = RotateVector({ -velocity.x, velocity.y },  offsetDeg);

    pp = {
        .spawnLocation = position,

        .colorRangeLo = { 0.50f, 0.50f, 0.50f, 0.50f },
        .colorRangeHi = { 0.75f, 0.75f, 0.75f, 0.70f },

		.coneDir = coneDir,
		.coneDeg = 10.0f,

        //.particleSize = 2,
		.particleSpeed = 5.0f,
        .lifeTime = 0.25f,
        .particlesPerSecond = 10.0f,
        //.fadeInTime = 0;
        //.fadeOutTime = 0;

        //.terminalVelocity = {};
    };

	ParticleGen* p = level->CreateActor<ParticleGen>(pp);
	p->parent = id;
	dustGenerator = p->id;

	//AudioParams a = {
	//	.nameOfSound = "Grass",
	//	.loopCount = AUDIO_MAXLOOPS,
	//};
}

void Enemy::Update(float deltaTime)
{
	UpdateLocation(-60.0f, deltaTime);

	CollisionWithBlocks(this, true);
	invinciblityTime = Max(invinciblityTime - deltaTime, 0.0f);
	if (grounded == true)
	{
		if (velocity.x != 0)
			PlayAnimation(ActorState::Run);
		else
			PlayAnimation(ActorState::Idle);
	}

	ParticleGen* g = level->FindActor<ParticleGen>(dustGenerator);
	if (g == nullptr)
	{
		g = level->CreateActor<ParticleGen>(pp);
		g->parent = id;
		dustGenerator = g->id;
	}
	assert(g);

	if (grounded)
	{
		if (velocity.x != 0)
		{
			PlayAnimation(ActorState::Run);

			if (velocity.x > 0.0f)
			{
				g->coneDir = RotateVector({ -velocity.x, velocity.y }, -60.0f);
				g->position.x = 0;
			}
			else
			{
				g->coneDir = RotateVector({ -velocity.x, velocity.y }, 60.0f);
				g->position.x = GameWidth();
			}

			g->playing = true;
		}
		else
		{
			PlayAnimation(ActorState::Idle);
			g->playing = false;
		}
	}
	else
		g->playing = false;

	UpdateAnimationIndex(this, deltaTime);

	//TODO: Improve accuracy of audio playback
	timeToMakeSound += deltaTime;
	float t = 1.0f / (fabs(velocity.x));//0.5f;
	if (timeToMakeSound >= t && g_gameState == GameState::Game)
	{
		AudioParams params = {
	.nameOfSound = "Grass",
		};
		AudioPlayer* ap = level->CreateActor<AudioPlayer>(params);
		ap ->parent = id;

		timeToMakeSound = 0.0f;//timeToMakeSound - t;
	}
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
	allowRenderFlip = false;
	AttachAnimation();
	PlayAnimation(ActorState::Idle);
	terminalVelocity = { 1000, 1000 };
	Vector adjustedPlayerPosition = { info.player->position.x/* + 0.5f*/,
									  info.player->position.y + 1 };
	Vector playerPosAdjusted = { adjustedPlayerPosition.x + (info.player->GameWidth() / 2),
							     adjustedPlayerPosition.y };

	destination = info.mouseLoc;
	rotation = RadToDeg(atan2f(destination.y - adjustedPlayerPosition.y, destination.x - adjustedPlayerPosition.x));

	float speed = 50.0f;
	Vector ToDest = destination - playerPosAdjusted;
	ToDest = Normalize(ToDest);
	velocity = ToDest * speed;
	Vector gameSize = { GameWidth(), GameHeight() };
	position = adjustedPlayerPosition + (ToDest * info.player->spawnRadius);
}

void Projectile::Update(float deltaTime)
{
	UpdateLocation(0, deltaTime);
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

	assert(info.mimicType != ActorType::None);
	health = 0;
	inUse = true;
	AttachAnimation(info.mimicType);
	PlayAnimation(ActorState::Dead);
}

void Dummy::Update(float deltaTime)
{
	UpdateLocation(-60.0f, deltaTime);
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
	AttachAnimation();
	PlayAnimation(ActorState::Idle);
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
	AttachAnimation();
	PlayAnimation(ActorState::Idle);
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
	allowRenderFlip = false;
	AttachAnimation();
	PlayAnimation(ActorState::Idle);
	level->movingPlatforms.push_back(id);
}
void MovingPlatform::Update(float deltaTime)
{
	UpdateLocation(0, deltaTime);

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

		position = dest;
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

void Grapple::OnInit(const GrappleInfo& info)
{
	assert(info.actorID);

    //Grapple needs to travel to the location where the player clicked
	//TODO:  make sure there isnt a valid grapple already attached to the player

	AttachAnimation();
	PlayAnimation(ActorState::Idle);
	attachedActor = info.actorID;
	Player* player = level->FindActor<Player>(attachedActor);
	assert(player);
	terminalVelocity = { 1000, 1000 };
	player->grapple = id;
	player->grappleReady = false;
	grappleState = GrappleState::Sending;
	grappleDistance = 0;

	Vector ToDest = info.mouseLoc - player->Center();
	ToDest = Normalize(ToDest);
	velocity = ToDest * grappleSpeed + player->velocity;
	position = player->Center() + (ToDest * player->spawnRadius);
	rotation = RadToDeg(atan2f(velocity.x, velocity.y));
}


void Grapple::Update(float deltaTime)
{
	UpdateLocation(0, deltaTime);
	Player* player = level->FindActor<Player>(attachedActor);
	assert(player);

	//TODO: fix magic number
	shotOrigin = player->Center();
	rotation = 270.0f - RadToDeg(atan2f(position.x - shotOrigin.x, position.y - shotOrigin.y));
	switch (grappleState)
	{
		case GrappleState::None:
		{
			assert(false);
			break;
		}
		case GrappleState::Sending:
		{

			//TODO: change to work with moving platforms
			//CollisionWithBlocks(this, false)
			Block* blockPointer = level->blocks.TryGetBlock({ position.x, position.y });
			if (blockPointer)
			{

				grappleState = GrappleState::Attached;
				grappleDistance = Distance(player->Center(), position);
				velocity = {};
				angularVelocity = LinearToAngularVelocity(position, player->Center(), player->velocity);
				player->angularUpdate = true;

				goto DoAttached;
			}
			else if (Distance(position, shotOrigin) > player->grappleRange)
			{
				grappleState = GrappleState::Retracting;
				goto DoRetracting;
			}
			else
				break;
		}
		case GrappleState::Attached:
		{

			DoAttached:
			if (player->jumpCount == 0)
				player->jumpCount++;

			break;
		}
		case GrappleState::Retracting:
		{

			DoRetracting:
			acceleration = { 8.0f, 8.0f };
			velocity = Normalize(shotOrigin - position) * grappleSpeed + player->velocity;
			if (Pythags(shotOrigin - position) < 1.0f)
			{
				player->grapple = 0;
				player->grappleReady = true;
				inUse = false;
			}
			break;
		}
	}

	UpdateAnimationIndex(this, deltaTime);
}

void Grapple::Render()
{
	Sprite* sprite = GetSpriteFromAnimation(this);

	Rectangle rect = {};
	rect.botLeft = { position.x, position.y - GameHeight() / 2.0f };
	float length = Pythags(position - shotOrigin);
	rect.topRight = { rect.botLeft.x + length, position.y + GameHeight() / 2.0f };

	Rectangle sRect = { {}, { ((rect.topRight.x - rect.botLeft.x) / GameWidth()) * sprite->width, (float)sprite->height } };
	Vector rotationPoint = { 0, GameHeight() / 2.0f};
	AddTextureToRender(sRect, rect, RenderPrio::Sprites, sprite, White, rotation, rotationPoint, SDL_FLIP_NONE, CoordinateSpace::World);
}


/*********************
 *
 * AudioPlayer
 *
 ********/

float FadeInGameUnits(float min, float max, float distance)
{
	//TODO: Change to constant?
	float minFadeDistance = (g_camera.size.x / 20.0f);
	float maxFadeDistance = (g_camera.size.x / 2.0f) * 1.1f;
    float offset = distance - min;

	float lerpResult = Lerp(1.0f, 0.0f, offset / (max - min));
    return Clamp(lerpResult, 0.0f, 1.0f);
}

void AudioPlayer::OnInit(AudioParams info)
{
	audioParams = info;
<<<<<<< HEAD
	audioID = PlayAudio(audioParams);
=======
	//AttachAnimation(this);
    //PlayAnimation(ActorState::Idle);
>>>>>>> 3f6a660 (Modified Actor and ParticleGen)
}

void AudioPlayer::Update(float deltaTime)
{
	Vector worldPosition = GetWorldPosition();
	Vector diff = g_camera.position - worldPosition;
	Vector v = {};

	v.x = FadeInGameUnits((g_camera.size.x / 20.0f), (g_camera.size.x * 1.2f / 2.0f), fabs(diff.x));
	if (diff.x < 0.0f)
		v.x *= -1.0f;
	SetAudioPan(audioID, v.x);

	v.y = FadeInGameUnits(5, 20, fabs(diff.y));
	SetAudioVolume(audioID, v.y);

	inUse = AudioIDValid(audioID);
}

void AudioPlayer::Render()
{
	if (g_debugList[DebugOptions::ShowAudio])
	{
		Sprite* sp = g_sprites["Speaker"];
		Vector s = { (PixelToGame(sp->width) / 2.0f), (PixelToGame(sp->height) / 2.0f) };
		Vector wp = GetWorldPosition();
		Rectangle DR = {
			.botLeft = wp - s,
			.topRight = wp + s,
		};
		AddTextureToRender({}, DR, RenderPrio::Debug, sp, White, 0.0f, position, false, CoordinateSpace::World);
	}
}


/*********************
 *
 * ParticleGen
 *
 ********/

void ParticleGen::OnInit(const ParticleParams info)
{
	colorRangeLo = info.colorRangeLo;
	colorRangeHi = info.colorRangeHi;

	coneDir = info.coneDir;
	coneDeg = info.coneDeg;
	particleSpeed = info.particleSpeed;

	lifeTime = info.lifeTime;
	particlesPerSecond = info.particlesPerSecond;
	particleSize = info.particleSize;
	terminalVelocity = info.terminalVelocity;
	timeLeftToSpawn = 0.0f;

	//fadeInTime = info.fadeInTime;
	//fadeOutTime = info.fadeOutTime;
}

void ParticleGen::Update(float deltaTime)
{

	//Create Particles
	timeLeftToSpawn -= deltaTime;
	while (playing && timeLeftToSpawn <= 0)
	{

		Vector worldPosition = GetWorldPosition();
		float coneDeg2 = DegToRad(Random<float>(-coneDeg, coneDeg));
		Vector vel = RotateVector(Normalize(coneDir), coneDeg2);

		Particle p = {
			.location = worldPosition,
			.velocity = vel * particleSpeed,
			.acceleration = vel,

			.color = CreateRandomColorShade(colorRangeLo.r, colorRangeHi.r),// colorRangeLo, colorRangeHi),
			.timeLeft = lifeTime, //seconds
			//.fadeInTime = fadeInTime,
			//.fadeOutTime = fadeOutTime, //seconds

			.size = particleSize,
		};
		AddParticle(p);

		timeLeftToSpawn += (1.0f / particlesPerSecond);
	}
}

void ParticleGen::Render()
{

}

/*********************
 *
 * Items
 *
 ********/

void Item::OnInit()
{

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
	Gamestate* gs = GetGamestate();
	for (auto& level : gs->levels)
	{
		Player* ptr = level.second.FindPlayer();
		if (ptr)
			return ptr;
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
#if 0
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
	assert(false);
	stbi_write_png(level->filename, width, height, colorChannels, memBuff.data(), stride_in_bytes);
}


void LoadLevel(Level* level, Player& player)
{
	level->blocks.ClearBlocks();

	int32 textureHeight, textureWidth, colorChannels;
	assert(false);
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
#endif

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

bool TriggerVolumePoint(const Rectangle& rect, const Vector& v)
{
	return (v.x >= rect.botLeft.x && v.x <= rect.botLeft.x) && (v.y >= rect.topRight.y && v.y <= rect.topRight.y);
}

bool TriggerVolumeRectangle()
{
	return true;
}

void ClickUpdate(Block* block, bool updateTop, Level* level)
{
	level->blocks.UpdateBlock(block);
	SurroundBlockUpdate(block, updateTop, level);
}


uint32 CollisionWithBlocksSubFunction(bool& grounded, Rectangle rect, Actor* actor, bool isEnemy)
{

	uint32 collisionFlags = actor->CollisionWithRect(rect);
	if (collisionFlags)
	{
		ActorType actorType = actor->GetActorType();
		if (collisionFlags & CollisionRight)
		{
			actor->position.x = rect.botLeft.x - actor->GameWidth();
			if (isEnemy)
				actor->velocity.x = -1 * actor->velocity.x;
			else
			{
				actor->velocity.x = 0;
			}
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
			actor->ResetJumpCount();
			grounded = true;
		}
		return collisionFlags;
	}
	return collisionFlags;
}

//TODO: move to be a member function in the Block struct
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

			if(CollisionWithBlocksSubFunction(grounded, { block->location, { block->location + Vector({ 1, 1 }) } }, actor, isEnemy))
			{
				if (Player* player = dynamic_cast<Player*>(actor))
				{
					if (Grapple* grapple = player->level->FindActor<Grapple>(player->grapple))
						grapple->angularVelocity = 0;
				}
			}
		}
	}

#if 0

	//Checking Moving Platforms
	MovingPlatform* collidedPlatform = nullptr;
	uint32 collisionFlags = 0;
	for (ActorID ID : actor->level->movingPlatforms)
	{
		if (MovingPlatform* MP = actor->level->FindActor<MovingPlatform>(ID))
		{

			Rectangle MPRect = { MP->position, { MP->position.x + MP->GameWidth(), MP->position.y + MP->GameHeight() } };
			collisionFlags = CollisionWithBlocksSubFunction(grounded, MPRect, actor, isEnemy);
			if (collisionFlags)
			{

                collidedPlatform = MP;
				if (Player* player = dynamic_cast<Player*>(actor))
				{
					if (Grapple* grapple = player->level->FindActor<Grapple>(player->grapple))
					{
						grapple->angularVelocity = 0;
                        collidedPlatform = nullptr;
					}
				}
			}
		}
	}

    #else
    //Checking Moving Platforms
	ActorID collisionID = 0;
	MovingPlatform* collidedPlatform = nullptr;
	uint32 collisionFlags = 0;
	for (int32 i = 0; i < actor->level->movingPlatforms.size(); i++)
	{
		if (Actor* actorTwo = actor->level->FindActor<MovingPlatform>(actor->level->movingPlatforms[i]))
		{
			MovingPlatform* MP = (MovingPlatform*)actorTwo;

			Rectangle MPRect = { MP->position, { MP->position.x + MP->GameWidth(), MP->position.y + MP->GameHeight() } };
			collisionFlags = CollisionWithBlocksSubFunction(grounded, MPRect, actor, isEnemy);
			if (collisionFlags)
			{

				bool attachable = true;
				if (Player* player = dynamic_cast<Player*>(actor))
				{
					if (Grapple* grapple = player->level->FindActor<Grapple>(player->grapple))
					{
						grapple->angularVelocity = 0;
						attachable = false;
					}

				}
				if (attachable)
				{
					collisionID = actor->level->movingPlatforms[i];
					collidedPlatform = MP;
				}
			}
		}
	}
    #endif

	//new parent is different from old parent
	//need to remove child on old parent and put onto new parent
	if (collidedPlatform)
	{
		collidedPlatform->childList.push_back(actor->id);
		actor->parent = collidedPlatform->id;
	}
	else if (MovingPlatform* MP = actor->level->FindActor<MovingPlatform>(actor->parent))
	{

		if (collisionFlags & CollisionLeft || collisionFlags & CollisionRight)
			actor->velocity.x += MP->velocity.x;
		else
			actor->velocity += MP->velocity;
		actor->parent = 0;
	}

	//Grounded Logic
	if (actor->grounded != grounded)
	{
		if (!grounded && (actor->GetActorType() == ActorType::Player || actor->GetActorType() == ActorType::Enemy))
			actor->PlayAnimation(ActorState::Jump);
	}
	actor->grounded = grounded;
}

uint32 CollisionWithActor(Player& player, Actor& enemy, Level& level)
{
	uint32 result = 0;

	float xPercentOffset = 0.2f;
	float yPercentOffset = 0.3f;
	Rectangle xRect = enemy.CollisionXOffsetToRectangle();
	Rectangle yRect = enemy.CollisionYOffsetToRectangle();

	if (g_debugList[DebugOptions::PlayerCollision])
	{
		AddRectToRender(xRect, transGreen, RenderPrio::Debug, CoordinateSpace::World);
		AddRectToRender(yRect, transOrange, RenderPrio::Debug, CoordinateSpace::World);
	}

	uint32 xCollisionFlags = player.CollisionWithRect(xRect) & (CollisionLeft | CollisionRight);
	uint32 yCollisionFlags = player.CollisionWithRect(yRect) & (CollisionBot | CollisionTop);
	float knockBackAmount = 20;
	result = xCollisionFlags | yCollisionFlags;
	if (enemy.GetActorType() == ActorType::Enemy)
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

	Level* level = GetGamestate()->GetLevel(basePortal->levelPointer);
	for (int32 i = 0; i < level->actors.size(); i++)
	{
		Actor* actor = level->actors[i];
		if (actor->GetActorType() == ActorType::Portal)
		{
			Portal* result = (Portal*)actor;
			if (result->portalID == basePortal->portalPointerID)
			{
				return result;
			}
		}
	}

	assert(false);
	ConsoleLog("Failed to get Portal from %s at portalID %d", basePortal->levelPointer, basePortal->portalPointerID);
	return nullptr;
}


void UpdateLaser(Actor* player, Vector mouseLoc, TileType paintType, float deltaTime)
{
	laser.paintType = paintType;
	laser.inUse = true;
	laser.AttachAnimation();
	Vector adjustedPlayerPosition = { player->position.x + 0.5f, player->position.y + 1 };

	float playerBulletRadius = 0.5f; //half a block
	laser.destination = mouseLoc;
	laser.rotation = RadToDeg(atan2f(laser.destination.y - adjustedPlayerPosition.y, laser.destination.x - adjustedPlayerPosition.x));

	Vector ToDest = Normalize(laser.destination - adjustedPlayerPosition);
	laser.position = adjustedPlayerPosition + (ToDest * playerBulletRadius);
	laser.PlayAnimation(ActorState::Idle);
}

void UpdateAnimationIndex(Actor* actor, float deltaTime)
{
	bool ignoringStates = actor->actorState == ActorState::Dead || actor->actorState == ActorState::Jump;

	if (actor->animationCountdown < 0)
	{
		actor->index++;
		Animation* anime = actor->animationList->GetAnimation(actor->actorState);
		if (anime == nullptr)
		{
			anime = actor->animationList->GetAnyValidAnimation();
			ConsoleLog("Could not get valid animation from actor state in UpdateAnimationIndex()");
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
			dummy->PlayAnimation(ActorState::Dead);
			actor->inUse = false;
		}
	}
}



void UpdateEnemiesPosition(std::vector<Actor*>* actors, float gravity, float deltaTime)
{
	for (int32 i = 0; i < actors->size(); i++)
	{
		Actor* actor = (*actors)[i];
		if (actor->GetActorType() == ActorType::Enemy)
		{
			actor->UpdateLocation(gravity, deltaTime);
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
	full.botLeft.y = actor.position.y + actor.GameHeight() + healthHeight;
	full.topRight.x = full.botLeft.x + PixelToGame(int(actor.animationList->scaledWidth));
	full.topRight.y = full.botLeft.y + healthHeight;

	actual.botLeft.x = full.topRight.x - (actor.health / 100) * full.Width();
	actual.botLeft.y = full.botLeft.y;
	actual.topRight = full.topRight;

    AddRectToRender(RenderType::DebugFill, full, HealthBarBackground, RenderPrio::UI, CoordinateSpace::World);
    AddRectToRender(RenderType::DebugFill, actual, Green, RenderPrio::UI, CoordinateSpace::World);
}

void LoadAnimationStates(/*std::vector<AnimationData>* animationData*/)
{
	std::vector<std::string> dirNames = GetDirNames("Assets/Actor_Art", false);

	std::string stateStrings[] = { "Error", "Idle", "Walk", "Run", "Jump", "Dead" };
	static_assert(sizeof(stateStrings) / sizeof(stateStrings[0]) == (size_t)ActorState::Count, "Change in count");

	AnimationData data;
	for (const std::string& string : dirNames)
	{
		data = {};
		data = GetAnimationData(string, stateStrings);

		if (s_actorAnimations.find(data.name) != s_actorAnimations.end())
		{
			ConsoleLog(LogLevel::LogLevel_Warning, "Already found animation: %s", data.name);
			continue;
		}


		AnimationList* animationList = &s_actorAnimations[data.name];
		animationList->animations.reserve((int32)ActorState::Count);

		for (int32 i = 1; i < (int32)ActorState::Count; i++)
		{
			Animation* animeP = new Animation();
			animeP->type = (ActorState)i;
			animeP->anime.reserve(20);
			if (data.animationFPS[i])
				animeP->fps = data.animationFPS[i];
			for (int32 j = 1; true; j++)
			{
				std::string combinedName = "Assets/Actor_Art/" + std::string(data.name) + "/" + stateStrings[i] + " (" + std::to_string(j) + ").png";
				Sprite* sprite = CreateSprite(combinedName.c_str());
				if (sprite == nullptr)
					break;
				else
					animeP->anime.push_back(sprite);
			}
			if (animeP->anime.size() > 0)
				animationList->animations.push_back(animeP);
			else
				delete animeP;
		}

		//std::swap(data.collisionRectangle.botLeft.y, data.collisionRectangle.topRight.y);
		Sprite* sprite = animationList->GetAnyValidAnimation()->anime[0];
		assert(sprite);
		if (data.collisionRectangle.botLeft.y)
			data.collisionRectangle.botLeft.y = sprite->height - data.collisionRectangle.botLeft.y;
		if (data.collisionRectangle.topRight.y)
			data.collisionRectangle.topRight.y = sprite->height - data.collisionRectangle.topRight.y;

		if (data.collisionRectangle.botLeft.x == 0 &&
			data.collisionRectangle.botLeft.y == 0 &&
			data.collisionRectangle.topRight.x == 0 &&
			data.collisionRectangle.topRight.y == 0)
		{
			data.collisionRectangle = { { 0, 0 }, { (float)sprite->width, (float)sprite->height } };

		}
		else
		{
			if (data.collisionRectangle.botLeft.x == inf)
				data.collisionRectangle.botLeft.x = (float)sprite->width;
			if (data.collisionRectangle.topRight.x == inf)
				data.collisionRectangle.topRight.x = (float)sprite->width;
			if (data.collisionRectangle.botLeft.y == inf)
				data.collisionRectangle.botLeft.y = (float)sprite->height;
			if (data.collisionRectangle.topRight.y == inf)
				data.collisionRectangle.topRight.y = (float)sprite->height;
		}

		animationList->colOffset = data.collisionOffset;
		animationList->colRect.bottomLeft.x = (int32)data.collisionRectangle.botLeft.x;
		animationList->colRect.bottomLeft.y = (int32)data.collisionRectangle.botLeft.y;
		animationList->colRect.topRight.x  = (int32)data.collisionRectangle.topRight.x;
		animationList->colRect.topRight.y  = (int32)data.collisionRectangle.topRight.y;

		if (data.scaledWidth == inf)
			animationList->scaledWidth = (float)sprite->width;
		else
			s_actorAnimations[data.name].scaledWidth = data.scaledWidth;

	}
}

