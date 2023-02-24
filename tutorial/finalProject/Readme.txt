Final Project:
1.Game Loop:
The player needs to colelct the black pickups in every round while avoiding
gray obstacles, and collecting health pickups.
After every round the player clicks on the next wave button to initiate another - harder wave.
The game ends when the player finishes wave 5 or when the snake dies from hitting obstacles.


2.Mechanics
- Movement:
The snake moves in 3d space via a translation applied to it by an animation visitor
towards a constant direction.
The player rotates the snake by pressing the arrow keys.
All other links of the snakes are children of the moved object and are moved with it.
*Difficulties:
~ How to move the snake in the corrct direction using the correct sstem and axis systems.
~ Correct Snake Rotation and angles

- Collision Detection
The player interacts with both moving and static objects by colliding with them.
Collisions are detected by interscting bounding boxes as in assignment2.
Collisions are handled primarily in the GameObject Class
 - Object types:
 There are three types of interactables:
  - Pickups: Raise score
  - Health Pickups: Restore health
  - Obstacles: Reduce health
*Difficulties:
~ How to Have collision detection while not being too heavy on performance
~ How to mask interactables only to interact with specific Objects
~ When to decimate meshes to avoid performance impacts


- Object Movement
 - Bezier:
 Objects can move in a random bezier curve plotted by random points chosen in a uniform distribution.
 The plot lines can be displayed.
 - Random:
 Objects can move in Random constant directions while changing the movement vector now and then
*Difficulties:
~ How to generate the Bezier curves to behave uniformly, how to plot and display them.
~ How to make movement look fluid and still be playable 
~ When does regenerating movement direction come in and how to make it looks more alive.

- SpawnManager:
Spawning is controlled by a SpawnManager which tracks and manages:
 - Spawn zones: which areas are object allowed to spawn in
 - Spawn numbers: spawns correct number of each type of obejct 
   in context of current wave number to allow increasing difficulty.
 - Active objects: to know when to advance to the next wave 
*Difficulties:
~ How to keep track of the correct objects
~ How to spawn the objects at correct times to avoid messing with visited data structures

- Sound: Audio feedback occurs when the snake interacts with other objects, and on game start and end.
Achieved using Built in windows sound libraries.

- Cameras
 - First Person Camera: child of snake, located on its front and has a large fov
 - Top down - child of the snake as well to allow looking a snake from above

- Gui: UI containing buttons , current score, health and wave.
  Interactive and reactive to game state - when level is finished, when snake is dead , etc'

 
3.Optimizations and visuals
- Decimation: models are predecimated so that Collision detection is less constly.
- GameManager: Ensures spawning at corect time frames, keeps track of active interactables and game state.
- Tick awareness: costly operations such as collision detection are being called every X'th iteration 
  to create a balance between performance and good reactivity
- Skinning: a skinning attempt using weights and DQS has been made, results were not acheived unfortunately.
- Masks: collisions only occur between objects that should interact with one another to conserve resources.
*Difficulties:
~ When to Decimate and how to use predecimated meshes
~ How often (ticks) to use collision detection to keep good performace
~ How to properly apply skinning without mesh moving or objects flying around
~ How to keep track of correct objects (spawnables) and how to balance difficulty
