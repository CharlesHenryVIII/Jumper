# Jumper


### TODO:
#### Current
- [ ] Load fonts the same as the character sprites
	* need to determine a way to load all files in a directory
- [x] bullet fixes
- [ ] player spawning
	* need to figure out where to spawn the player
- [ ] Asperite integration?
- [ ] change how tilesets are managed?
	* possibly get arid of the auto updating of tiles


#### Future
* scaled total time based on incrimenting the delta time
* settings
    * keybinds
* Player spawning
    * creating a new player on player death
* store player ID on levels
* use more IDs
- [x] fix bullets ending at the end of where you clicked instead of ending immediatly passing where you clicked.  Reference is the tail of the sprite not the head


#### Ideas
* camera zoom
* sub pixel rendering/SuperSampling
* multithread saving/png compression
* add paralaxed background(s)
* windows settings local grid highlight
* add gun to change blocks some ideas: 
    * spring/wind
    * tacky(can hop off walls with it)
    * timed explosion to get more height
* moving platforms
* enemy ai
* power ups
* explosives!?
* loading bar at the begining of the game
* multithread loading


### Chris Questions:
* If you had to go to a 4 year college before you did your community college and had to choose a degree before trying things out what would you have chosen instead of programming?
* quaturnions
* openGL GLSL?
    * shaders?


### Gameplay
#### Core Gameplay
* get to the /end/ of the level
* use the tools you have to get there with the least amount of blocks added/changed
* tools include block place and block modifier?


#### Game Design:
* Enemy Types:
	1. Jump_on_head
	2. Melee
	3. ranged
	4. magic
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



### CS20
* singly linked list
* doubly linked list
* stack
* queue
* sort funct
* priority queue
* hashmap
* Write an alocator/malloc
