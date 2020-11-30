# Jumper


### TODO:
#### Current

- [ ] Squash bugs
	- [ ] Player is not touching the ground and size seems to big compared to what it used to be
		- [x] Might be in the RenderActor() function
		- [x] This is because the swap in the texture loading was not flipping properly
		- [ ] Change how texture scaling is being handled, time to refactor old code
			- [ ] replace ScaledWidth with a scale value.
				-[ ] Get art assets that are consistant in size
	- [x] Look into player linear velocity feeling slow on detach
- [x] Sound
	- [x] Cache songs
	- [x] Load Wav through SDL
	- [x] Mix audio
	- [x] Audio fade out
		- [x] Audio fadeout on deletion instead of time based
	- [x] Audio stopping based on time elapsed
	- [ ] Audio fade in
	- [x] Audio thread syncing with main thread
		- [ ] Verify working
	- [ ] REWRITE THE ENTIRE CALLBACK FUNCTION
	- [ ] Action area or events based on region 
	- [ ] TRIGGER VOLUME
- [ ] load operating system files in a folder
- [ ] Debug writing to disk for trouble shooting issues in release mode / other computers
	- [ ] it works on my machine

- [ ] GET SOME DAMN GAMEPLAY DOWN
	- [x] Grappling
		- [ ] Fix collision against MovingPlatforms
		- [ ] Add grappling to MovingPlatforms
		- [ ] isentropic add entropy
		- [ ] Dont allow to rotate above a curtain height based on velocity?
	- [ ] Power Ups (health, movement, jumping height, extra jump, etc) 
	- [ ] Wall jump 
	- [ ] Explosions 
		- [ ] Changing player movement
		- [ ] Changing tiles 
	- [ ] Wind
	- [ ] Slippery surfaces/walls
	- [x] Moving Platforms 
	- [x] Spring Board

- [ ] Strong types for physics/math
- [ ] Load fonts the same as the character sprites
	* need to determine a way to load all files in a directory
	* also cant really do this because I need the information about the font (NAME, offset length, width, etc)
	* also cant garentee that it will have loaded correctly.  Need to verify that I am loading assets like the character sprites.
- [ ] player spawning
	- [ ] Determine how the program will know where to spawn the player
- [ ] Asperite integration?
- [ ] change how tilesets are managed?
	- [ ] possibly get arid of the auto updating of tiles

#### Rendering
- [ ] change between linear and nearest depending on the sprite/texture
- [ ] Fix tile sprites rendering outside their UV's

#### Homework
- [ ] Write a heap alloc
- [ ] Write an unordered map implimentation (faster for debug)

#### Future
* scaled total time based on incrimenting the delta time
* settings
    * keybinds
* Player spawning
    * creating a new player on player death


#### Ideas
* multithread saving/png compression
* windows settings local grid highlight
* add gun to change blocks some ideas: 
    * spring/wind
    * tacky(can hop off walls with it)
    * timed explosion to get more height
* enemy ai
* power ups
* explosives!?
* loading bar at the begining of the game
* multithread loading


### Chris Questions:
* quaturnions


### Gameplay
#### Gameplay and Game Designs:
* V2:
	* Each level consists of building a device/model/vehicle/building and parts are spread across the level. The player(s) will need to find all the parts and put them together back at the start of the level
	* Each level might have a few smaller levels where you will need to get the pieces.
	* Add obstacles along the way in the form of puzzles, platforming, combat and more.
	
* V1:
	* get to the /end/ of the level
	* use the tools you have to get there with the least amount of blocks added/changed
	* tools include block place and block modifier?
	* Levels:
		1.  Just jumping to get use to the mechanics;
		2.  jump on top of enemies to kill them;
		3.  boss fight, jump on head 3 times and boss flees;
		4.  learn how to melee and use it on enemies, continue platforming;
		5.  boss fight with same boss but looks different since its the next phase, try to kill him with melees and timing, he flees;
		6.  learn ranged and continue platforming;
		7.  boss fight with same boss but looks different since its the next phase, try to kill him with ranged and timing, he flees;
		8.  learn magic and continue platforming;
		9.  with same boss but looks different since its the next phase, try to kill him with magic but he flees;
		10. final fight with the boss where you have to do all the phases at once but different arena and what not. fin;

* Common Game Design Inclusions accross revisions:
	* Enemy Types:
		1. Jump_on_head
		2. Melee
		3. ranged
		4. magic



### CS20
- [ ] singly linked list
- [ ] doubly linked list
- [ ] stack
- [ ] queue
- [x] sort funct
- [ ] priority queue
- [ ] hashmap
- [ ] Write an alocator/malloc
