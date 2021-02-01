#include "ParticleSystem.h"
#include "Rendering.h"

#include <vector>
#include <unordered_map>

void ParticleSystem::Update(float dt)
{
	timeLeftToSpawn -= dt;
	while (playing && timeLeftToSpawn <= 0)
	{

		Vector worldPosition = location;
		float coneDir = DegToRad(pp.coneDeg.RandomInRange());
		Vector vel = { cosf(coneDir), sinf(coneDir) };

		Particle p = {
			.location = worldPosition,
			.velocity = vel * pp.particleSpeed,
			.acceleration = vel,

			.color = CreateRandomColorShade(pp.colorRangeLo.r, pp.colorRangeHi.r),// colorRangeLo, colorRangeHi),
			.timeLeft = pp.lifeTime, //seconds
			//.fadeInTime = fadeInTime,
			//.fadeOutTime = fadeOutTime, //seconds

			.size = pp.particleSize,
		};
		particles.push_back(p);

		timeLeftToSpawn += (1.0f / pp.particlesPerSecond);
	}

	for (uint32 i = 0; i < particles.size(); i++)
	{
		Particle& p = particles[i];
		p.velocity.x += p.acceleration.x * dt;
		p.velocity.y += pp.gravity * dt;

		if (p.terminalVelocity.x > 0.5)
		{
			p.velocity.x -= ((p.velocity.x / 2.0f) * (1.5f) * dt);
			p.velocity.x = Clamp(p.velocity.x, -p.terminalVelocity.x, p.terminalVelocity.x);
		}

		if (p.terminalVelocity.y > 0.5)
		{
			p.velocity.y -= ((p.velocity.y / 2.0f) * (1.5f) * dt);
			p.velocity.y = Clamp(p.velocity.y, -p.terminalVelocity.y, p.terminalVelocity.y);
		}

		p.location += p.velocity * dt;

        if (p.timeLeft <= 0.0f)
            p.inUse = false;
        p.timeLeft -= dt;
	}

    std::erase_if(particles, [](Particle& p) { return p.inUse == false; } );

}

void ParticleSystem::Render()
{
	for (uint32 i = 0; i < particles.size(); i++)
	{
        const Particle& p = particles[i];
        float s = p.size / 2;
        Rectangle r = {
            .botLeft = { p.location.x - s, p.location.y - s},
            .topRight = { p.location.x + s, p.location.y + s},
        };
        AddRectToRender(r, p.color, RenderPrio::Sprites, CoordinateSpace::World);
	}
}
