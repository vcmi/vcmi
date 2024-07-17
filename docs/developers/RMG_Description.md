# RMG Description

## Fundamentals

Random maps are represented by undirected graph of zones linked with connections.

On maps with water, a single extra water zone is created.

### Modifiers

Zone filling process is split into multiple phases, each of them represented as a modifier. A modifier can require other modifiers to finish their job before launching. A modifier might be preceded by other modifier from every zone, or many modifiers from all zones. For instance, placing underground rock requires all underground zones to finish treasure placement first.

### Thread pool

A queue of Modifiers jobs is created in roughly topological order, so that Modificators with no dependencies are placed first. The queue is iterated in a circular manner and if Modificator with no remaining preceders is found, it is picked for execution in a separate thread. After job completion, Modifier is erased from dependencies of Modifiers which depend on it.

## Placing Zones

### Generating distance graph

Based on zone connections, a simple distance graph is created using Dijkstra algorithm.

### Initial zone placement

Based on distance graph, zones are placed one by one on N x N grid of size just enough to fit all the zones (for instance, 5 zones are placed on a 3 x 3 grid and 24 zones on 5 x 5 grid). Adjacent zones are placed close while distant zones are placed as far away from each other as possible.

### Iterative optimization

Finally, zones are moved from their initial positions using Fruchterman-Reingold algorithm. It assumes all the zones are soft spheres, which attract connected zones as springs but push back not overlapping zones crossing their borders. These forces are summend and determine the vector shift of the zone position. The algorithm uses classic "simulated annealing" approach - zones start with high "temperature" (are very soft and squishy) and then gradually become colder (harder) and push away overlapping zones with stronger force.

To prevent getting stuck in local minima, sometimes most misplaced zones swap placed manually.

### Penrose Tiling

Using iterative subdivision, a set of Penrose tiling vertices ic created at random orientation and centered over middle of the map. All the tiles on a map are assigned to the closes vertex. Then, every vertex is assigned to the closest zone, creating irregular shapes.

## Zone connections

Directly adjacent zones are connected with a guard and a road, and overlapping zones on different levels are connected with Subterranean Gate. Zones which are not directly adjacent might be covered through water zone. If a zone shouldn't be connected with nearby zones adjacent to body of water, it's coast is sealed with obstacles.

### Water routes

**TODO**

### Fractalization

Every zone starts with at least one free tile in the center. Now, from every tiles at a distant greater than some number, a random tile is chosen. From it, algorithm routes a free path connected to already existing free paths. Process is repetaed until no tiles distant from free paths are left. Tiles used for connections are marked `free` and nothing else can be placed on them.

Zones with type `junction` are not fractalized.

The remaining tiles that are not obstacles (such as zone edges) are marked as `possible"` so they can be either filled with treasures or left `free.`

## Treasures

Every object or treasure pile in the zone is placed as far away from existing objects as possible. This includes towns and zone guards placed first. Zone keeps a priority queue of tiles sorted by their distance to closest object. Whenever an object is placed, these distances are updated.

### Treasure generation

Treasures are separated in value ranges. The highest range is picked first. A large number of treasure piles is generated, then RMG tries to fit each of treasure piles into a zone. Then, lower treasure ranges are generated

### Treasure placement

New treasure pile can't be closer to any previous object than some distance determined by total treasure density and value. Lower value piles and placed with lower minimal distance.

Any new treasure is placed so that it can't join two previously separated blocked islands, to prevent sealing a gap and ensuring entire zone is passable.

## Obstacles

After all the treasures are placed, tiles marked as `possible` are iteratively stripped and cleared from lose appendages, leaving `free` space. Then remaining ones are marked as `blocked`, and covered with obstacles.

### Biomes

For every zone, a few random obstacle sets are selected. [Details](https://github.com/vcmi/vcmi/blob/develop/docs/modders/Entities_Format/Biome_Format.md)

### Filling space

Tiles which need to be `blocked` but are not `used` are filled with obstacles. Largest obstacles which cover the most tiles are picked first, other than that they are chosen randomly.