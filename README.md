⚡ESCAPE THE MAZE
✨ Project Report
Course Code: CT-159
Course Instructors: Miss Samia Masood Awan and Miss Sahar

Group Members:

Syed Wahaaj Ali (Group Lead) — CT-24035
Mashood Khan — CT-24021
Ibtisam Khan — CT-24023
✨ Project Description
This project is an Escape The Maze game where the player navigates through procedurally generated mazes while avoiding enemies and collecting keys to unlock the exit door. The game features:

Procedurally generated maze levels using recursive backtracking algorithm.
Five progressive difficulty levels with a final boss battle.
Dynamic enemy AI with A* pathfinding and vision-based chase mechanics.
Bush-based stealth system allowing players to hide from enemies.
Key collection system with progressive difficulty (2-10 keys per level).
Mystery chests with randomized positive and negative effects.
Lives system with three attempts to complete the game.
Boss level featuring a giant meteor enemy, spawning minions, and laser attacks.
Time-based challenge with 60-90 second time limits per level.
Dynamic music system with chase music, boss battle music, and ambient background music.
Visual effects including red vignette during chase sequences and blackout effects.
The game combines strategic planning, real-time decision making, pathfinding, and procedural generation to create an engaging maze escape experience.

✨ Data Structures Used
a) Vectors (Dynamic Arrays)
Vectors serve as the primary dynamic collection throughout the game, storing game entities and providing efficient access and modification capabilities.

Enemy Vector
The enemy vector dynamically stores all active enemies including standard patrol enemies, boss entities, and spawning minions. It allows efficient iteration during collision detection, AI updates, and rendering. Enemies can be added during gameplay (minion spawning) and the vector automatically manages memory allocation.

Key Vector
The key vector maintains all collectible keys in the current level. It supports dynamic querying for collection status and enables efficient iteration during rendering and collision detection with the player.

Bush Vector
The bush vector stores all obstacle bushes generated from the maze algorithm. It is extensively used for collision detection, pathfinding obstacle marking, and rendering. The vector supports efficient spatial queries needed for real-time collision checks.

Patrol Points Vector
Each standard enemy contains a vector of patrol points defining their movement pattern. This provides flexible AI behavior where enemies can have variable numbers of patrol destinations.

b) 2D Vectors (Matrices)
Two-dimensional vectors represent grid-based spatial data structures essential for maze generation and pathfinding.

Maze Grid
The maze grid is a 2D vector of integers representing the maze layout where 1 indicates walls (bushes) and 0 indicates open paths. This grid-based representation enables efficient maze generation algorithms and spatial queries.

Visited Array
During maze connectivity validation, a 2D vector of characters tracks visited cells in the breadth-first search traversal. This ensures the generated maze has a valid path from player spawn to the exit door.

Occupancy Grid
The occupancy grid marks cells occupied by bushes for pathfinding calculations. It enables O(1) lookup during A* pathfinding to determine if a cell is traversable, significantly improving pathfinding performance.

Parent Array
A 2D vector of coordinate pairs stores parent information for each cell during A* pathfinding. This enables path reconstruction from target to source, allowing enemies to follow optimal paths around obstacles.

c) Queue (FIFO)
The queue data structure implements breadth-first search algorithms essential for maze generation and pathfinding.

BFS Queue
During maze generation, a queue processes cells in breadth-first order to validate maze connectivity. It ensures the player starting position can reach the exit door through open paths. The queue also powers the A* pathfinding algorithm where cells are explored level by level to find optimal enemy movement paths.

d) Pairs
Pairs store coordinate information and facilitate spatial data management throughout the game.

Grid Coordinates
Pairs of integers represent grid cell coordinates (row, column) in various algorithms. They are used in the BFS queue, parent tracking array, and free cell collections.

Free Cells Vector
A vector of coordinate pairs identifies all non-occupied cells in the grid. This enables efficient random selection of spawn points for enemies and collectibles while avoiding obstacles.

e) Structs (Custom Data Structures)
Structs encapsulate related data into cohesive units, improving code organization and type safety.

Bush Struct
The Bush struct contains a Rectangle area defining the collision bounds of each bush obstacle. It provides clean encapsulation of obstacle data.

Key Struct
The Key struct stores position and collection status for each collectible key. It maintains game state for key collection progression.

Chest Struct
The Chest struct tracks position, opened status, and effect type for mystery chests. It enables randomized gameplay events through effect variations.

Enemy Struct
The Enemy struct is the most complex, containing position, speed, radius, vision range, enemy type (standard/boss/minion), movement direction, patrol points, release status, and pathfinding target information. This comprehensive structure supports varied enemy behaviors and AI logic.

GridState Struct
The GridState struct encapsulates grid dimensions and the 2D maze grid array. It provides a clean interface for maze generation algorithms.

f) Enumerations
EnemyType Enum
The EnemyType enumeration defines three enemy categories: TYPE_STANDARD for patrol enemies, TYPE_BOSS for the final boss, and TYPE_MINION for spawning minions. This enables polymorphic behavior and clear type distinction in enemy logic.

3. Algorithms Implemented
a) Depth-First Search with Recursive Backtracking
The maze generation algorithm uses recursive depth-first search with backtracking to create complex, navigable mazes. Starting from a random cell, the algorithm recursively carves paths by randomly selecting unvisited neighboring cells. When no unvisited neighbors exist, it backtracks to find alternative paths. The openness parameter controls maze complexity by occasionally skipping paths, creating denser or more open layouts. This produces organic, interconnected maze structures with guaranteed connectivity.

b) Breadth-First Search
BFS validates maze connectivity by exploring the grid level-by-level from the player starting position. Using a queue, it visits all reachable cells and marks them in a visited array. After traversal, the algorithm checks if the exit door position is reachable. This ensures every generated maze has at least one valid solution path, preventing impossible level configurations.

c) A* Pathfinding Algorithm
The enemy AI uses simplified A* pathfinding to navigate around obstacles toward the player or patrol points. The algorithm maintains an occupancy grid marking bush positions, then uses BFS with parent tracking to find optimal paths. From the enemy position, it explores neighboring cells, tracks parent relationships, and reconstructs the shortest path to the target. This enables intelligent enemy movement that avoids walls while pursuing the player efficiently.

d) Flood Fill Algorithm
The maze generation process employs flood fill to remove isolated wall sections and create open areas. After initial maze carving, the algorithm floods connected regions with a removal percentage, creating clearings and preventing overly dense maze sections. This improves gameplay by balancing maze complexity with navigable space.

e) Collision Detection Algorithms
Circle-Rectangle Collision
This algorithm detects collisions between circular entities (player, enemies, keys) and rectangular obstacles (bushes). It checks if any point on the circle intersects the rectangle bounds, enabling accurate collision response for movement blocking and hiding mechanics.

Circle-Circle Collision
For entity interactions, the algorithm calculates distance between circle centers and compares it to the sum of radii. This efficiently detects player-enemy, player-key, and player-chest collisions.

Point-to-Line Distance
The boss laser attack uses point-to-line segment distance calculation. It projects the player position onto the laser line segment, finds the closest point, and measures distance. This enables precise laser beam collision detection using vector dot product and parametric line equations.

f) Random Maze Generation Enhancement Algorithms
Corridor Creation
Horizontal and vertical corridor generation algorithms create additional pathways through the maze by randomly carving straight lines. This prevents purely recursive maze patterns and adds strategic elements to level design.

Open Area Generation
The algorithm places rectangular open regions in the maze by clearing grid sections. Using uniform random distribution, it generates clearings of variable size, creating battle arenas and safe zones within the maze structure.

Border Opening Creation
To ensure multiple entry/exit points, the algorithm creates openings along maze borders at random positions. This prevents corner-trap situations and improves maze traversability.

g) Fisher-Yates Shuffle
The maze carving algorithm randomizes direction exploration order using the Fisher-Yates shuffle. This ensures unbiased random maze generation where each possible maze configuration has equal probability, creating varied gameplay experiences across levels.

h) Spatial Partitioning
Grid-Based Spatial Hashing
The pathfinding system uses grid-based spatial partitioning to convert world coordinates to grid cells. This enables O(1) lookup for obstacle checking and efficient spatial queries during collision detection and pathfinding operations.

Free Cell Identification
The algorithm scans the occupancy grid to identify all traversable cells, storing them in a vector for random selection. This enables efficient spawn point generation without collision conflicts.

i) Dynamic Difficulty Scaling
Maze Density Algorithm
As level increases, the maze generation openness parameter decreases, creating denser mazes. The algorithm calculates openness as 50 minus level-scaled factors, clamped between 10-50, progressively increasing difficulty.

Enemy Scaling
Enemy count scales linearly with level number, and vision range grows dynamically during gameplay. The boss level implements special scaling with increased key requirements, longer timers, and unique mechanics.

j) Path Reconstruction
After A* pathfinding finds the target, the path reconstruction algorithm traces parent pointers backward from goal to start. It follows the parent chain until reaching the starting cell, then returns the immediate next cell for enemy movement. This provides smooth, optimal pathfinding behavior.

✨ Summary
The Escape The Maze game demonstrates comprehensive application of data structures and algorithms for game development. Vectors efficiently manage dynamic collections of enemies, keys, and obstacles with automatic memory management. Two-dimensional vectors represent spatial grid data for maze generation and pathfinding. Queues enable breadth-first search algorithms for connectivity validation and optimal pathfinding. Pairs organize coordinate data, while structs encapsulate complex entity information with clear interfaces. Enumerations provide type safety for enemy classification.

The algorithm suite combines procedural generation through recursive DFS, connectivity validation via BFS, intelligent AI using A* pathfinding, and efficient collision detection through geometric algorithms. Random algorithms ensure varied gameplay, while spatial partitioning optimizes performance. Dynamic difficulty scaling and path reconstruction provide engaging, balanced gameplay progression.

The project integrates computational geometry, graph theory, recursive algorithms, and object-oriented design to create a complete game system. The modular architecture supports extensibility, the efficient algorithms ensure smooth real-time performance, and the procedural generation provides unlimited replayability.

Overall, this demonstrates practical application of data structures and algorithms in creating responsive, scalable, and engaging interactive entertainment.
