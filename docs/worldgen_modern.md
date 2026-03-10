Modern Procedural Planet Generation Pipeline 🌍
1. Icosahedron Sphere Mesh
2. Tectonic Plate Simulation
3. Continental Mask Generation
4. Base Elevation Field
5. Mountain / Ridge Generation
6. Erosion Simulation
7. Climate Simulation
8. River Generation
9. Biome Assignment
10. Final Terrain Mesh

Each stage adds a layer of realism.

1. Icosahedron Sphere Mesh

Most modern engines do NOT use Voronoi cells directly.

Instead they start with an icosphere.

icosahedron
  ↓
subdivide triangles
  ↓
normalize vertices to sphere

Advantages:

uniform triangles

easier LOD

GPU-friendly

easier terrain tessellation

Structure:

struct Vertex
{
    vec3 position;
    float elevation;
    float temperature;
    float moisture;
};

Triangle:

struct Triangle
{
    int v[3];
};

Typical sizes:

LOD	Triangles
Low	20k
Medium	200k
High	2M
2. Tectonic Plate Simulation

Instead of flood-fill plates, modern systems often use spherical Voronoi plates.

Steps:

random plate seeds on sphere
    ↓
assign each vertex to closest seed

This produces plates.

Plate structure:

struct Plate
{
    vec3 center;
    vec3 angularVelocity;
    bool oceanic;
};

Motion uses angular velocity:

velocity = cross(angularVelocity, position)

This causes curved plate motion.

Much more realistic than simple translation.

3. Continental Mask Generation

We determine which plates are continental vs oceanic.

continental plates → thick crust
oceanic plates → thin crust

Example:

60% ocean plates
40% continental plates

Base elevation:

continental plate = +0.3
ocean plate       = -0.5

This creates large continents.

4. Base Elevation Field

Compute distance from plate boundaries.

Why?

Real continents form like this:

coast → plains → mountains → plateau

Elevation function:

elevation =
continentalHeight
+ boundaryEffect
+ noise

Noise uses fractal Brownian motion.

Example:

elevation += fbm(position * 4.0) * 0.2;
5. Mountain / Ridge Generation

Mountains form where plates collide.

For each boundary:

relativeVelocity = vA - vB
compression = dot(relativeVelocity, normal)

If compression > 0:

mountainHeight = compression * strength

Add ridge noise:

ridge = abs(noise(position * 12))
elevation += ridge * mountainHeight

This produces:

Himalaya-style mountain chains

like those formed during the Himalayas collision.

6. Erosion Simulation

Without erosion, terrain looks fake.

Two common approaches:

Hydraulic erosion

Simulates rainfall.

rain
  ↓
water flow
  ↓
sediment transport
  ↓
deposition

Simplified algorithm:

water += rain

sediment = capacity * slope

if sediment > capacity
    deposit
else
    erode

Repeat ~50 iterations.

Thermal erosion

Simulates rock collapse.

if slope > threshold
    move material downhill

This smooths mountains.

7. Climate Simulation

Climate determines temperature and moisture.

Temperature depends on:

latitude
elevation
planet tilt

Example:

temperature =
cos(latitude)
- elevation * 0.6

Moisture depends on:

distance from ocean
prevailing wind
mountain rain shadow

Wind simulation:

moisture moves along wind vector

Mountains cause rain shadows.

8. River Generation

Similar to Red Blob's approach.

Steps:

Step 1 — compute flow direction

Each vertex flows to lowest neighbor.

downhill neighbor
Step 2 — accumulate flow
flow[v] += rainfall

flow[downstream] += flow[v]
Step 3 — mark rivers
if flow > threshold
    river

Rivers carve valleys.

9. Biome Assignment

Based on:

temperature
moisture
elevation

Example biome table:

Temp	Moisture	Biome
Hot	Wet	Rainforest
Hot	Dry	Desert
Cold	Wet	Taiga
Cold	Dry	Tundra

This creates worlds similar to Earth.

Example biomes include:

Tropical Rainforest

Tundra

Desert

10. Final Terrain Mesh

Final vertex position:

vertexPosition =
normalize(vertexPosition)
* (planetRadius + elevation * heightScale)

Add:

normal maps

detail noise

tessellation

Now the planet renders.

Final Planet Generation Flow
sphere mesh
      ↓
plate generation
      ↓
plate collision detection
      ↓
base elevation
      ↓
mountain ranges
      ↓
erosion simulation
      ↓
climate simulation
      ↓
river generation
      ↓
biome assignment
      ↓
terrain mesh

Why This Pipeline Works

It combines three independent systems:

geology (plates)
climate (temperature + moisture)
erosion (water + gravity)

Together they produce very realistic planets.

If you're implementing this in C++

I recommend splitting it like this:

PlanetGenerator
 ├── SphereMesh
 ├── PlateSimulator
 ├── ElevationGenerator
 ├── ErosionSimulator
 ├── ClimateSimulator
 ├── RiverSimulator
 └── BiomeSystem

Each system runs sequentially.