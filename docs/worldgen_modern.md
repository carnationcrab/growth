# Modern procedural planet generation

Reference for the **algorithm stages** used in contemporary globe world-gen (sim-side; not Godot scenes). For how Growth wires stages into data and C++, see [moddability.md](moddability.md) and `PlanetGlobePipeline`.

## Pipeline overview

1. Icosahedron sphere mesh
2. Tectonic plate simulation
3. Continental mask generation
4. Base elevation field
5. Mountain / ridge generation
6. Erosion simulation
7. Climate simulation
8. River generation
9. Biome assignment
10. Final terrain mesh

```text
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
```

---

## 1. Icosahedron sphere mesh

Most modern engines do not use Voronoi cells as the primary surface mesh. They start with an **icosphere**:

```text
icosahedron
  ↓
subdivide triangles
  ↓
normalise vertices to sphere
```

**Advantages:** uniform triangles, simpler LOD, GPU-friendly tessellation.

**Vertex (typical fields):**

```cpp
struct Vertex {
    vec3 position;
    float elevation;
    float temperature;
    float moisture;
};
```

**Triangle:**

```cpp
struct Triangle {
    int v[3];
};
```

| LOD    | Triangles (approx.) |
|--------|---------------------|
| Low    | 20k                 |
| Medium | 200k                |
| High   | 2M                  |

---

## 2. Tectonic plate simulation

Plates are often defined as a **spherical Voronoi** partition rather than flood-fill on an arbitrary graph:

```text
random plate seeds on sphere
  ↓
assign each vertex to closest seed
```

**Plate:**

```cpp
struct Plate {
    vec3 center;
    vec3 angularVelocity;
    bool oceanic;
};
```

Motion uses angular velocity:

```text
velocity = cross(angularVelocity, position)
```

Curved motion follows from rotation about the planet centre, not uniform translation.

---

## 3. Continental mask generation

Continental vs oceanic plates set crust thickness and baseline height:

| Plate type   | Typical base elevation |
|--------------|------------------------|
| Continental  | +0.3                   |
| Oceanic      | −0.5                   |

A common split is roughly 40% continental / 60% oceanic plates; exact ratios are content-driven.

---

## 4. Base elevation field

Elevation combines plate membership, distance to plate boundaries, and low-frequency noise. Coastlines often grade: coast → plains → mountains → plateau as distance from boundaries changes.

**Growth implementation** follows [1843-planet-generation](https://github.com/redblobgames/1843-planet-generation): convergent land–land boundaries seed the **mountain distance field at plate seed regions** (not every boundary Voronoi cell), then three BFS distance fields (mountain / ocean / coastline) blend elevation. See `ElevationAssigner.cpp`.

```text
elevation = continentalHeight + boundaryEffect + noise
```

Fractal Brownian motion (FBM) is wired in `ElevationAssigner.cpp` via `elevation_fbm_detail()` (`0.1 × fbm` at `frequency × 4` on unit-sphere sites, `WorldSeed` octaves/frequency).

---

## 5. Mountain / ridge generation

Colliding boundaries with **compression** along the boundary normal raise terrain:

```text
relativeVelocity = vA - vB
compression = dot(relativeVelocity, normal)

if compression > 0:
    mountainHeight = compression * strength
```

Ridge detail is often added with high-frequency noise:

```text
ridge = abs(noise(position * 12))
elevation += ridge * mountainHeight
```

Produces elongated chains along convergent boundaries (e.g. Himalayan-style arcs).

---

## 6. Erosion simulation (implemented)

Growth follows [Red Blob Games 1843 planet generation](https://github.com/redblobgames/1843-planet-generation) / mapgen4 river code, **not** a separate iterative sediment loop.

Pipeline stage `erosion` runs after `river_flow`:

1. `priority_flood` — Barnes-style priority-flood on **region** elevation from ocean seeds (`RegionPriorityFlood.cpp`): raise interior depressions, carve near-saddle rims (`elev[c] + ε`) so land drains to the ocean before triangle rivers. See epic OW-E11 in [epic_overworld_generation.md](epic_overworld_generation.md).
2. `river_downflow` — ocean seeds + elevation-priority visit order (`order_t`).
3. `river_flow` — land triangles seed flow from `0.5 × moisture²`; accumulate downstream into `s_flow`.
4. `erosion` — for each triangle in reverse visit order, if the trunk is higher than the tributary, set trunk elevation to tributary elevation (hydraulic valley cutting).

Implementation: `HydraulicErosion.cpp`, `RiverFlow.cpp`. Region heights are synced from triangles after erosion for the terrain mesh.

Thermal erosion and multi-iteration sediment transport are **not** implemented yet.

---

## 7. Climate simulation

**Temperature** depends on latitude, elevation, and optionally axial tilt:

```text
temperature = cos(latitude) - elevation * 0.6
```

**Moisture** depends on distance from ocean, prevailing wind, and rain shadow behind mountains. Moisture advection follows a wind vector; orographic lift depletes moisture on the lee side.

---

## 8. River generation

Classic **flow accumulation** on the mesh (cf. Red Blob Games):

**Step 1 — flow direction:** each vertex drains to its lowest neighbour.

**Step 2 — accumulate flow:**

```text
flow[v] += rainfall
flow[downstream] += flow[v]
```

**Step 3 — rivers:** edges with large `s_flow` are drawn as rivers; carving is the `erosion` stage above (trunk elevation lowered to tributary).

---

## 9. Biome assignment

Biomes map from temperature, moisture, and elevation. Example lookup:

| Temperature | Moisture | Biome        |
|-------------|----------|--------------|
| Hot         | Wet      | Rainforest   |
| Hot         | Dry      | Desert       |
| Cold        | Wet      | Taiga        |
| Cold        | Dry      | Tundra       |

---

## 10. Final terrain mesh

### Height field on a fixed icosphere

Growth keeps **connectivity fixed** for one generation (`topology.sites` + `topology.triangles` from `IcosphereEngine`). Optional **jitter** nudges sites tangentially once at the start, then renormalises; it does not model plate advection.

All geology, climate, flood, rivers, and erosion mutate **scalar fields** (`region_elevation`, `triangle_elevation`, …). Plate tectonics change the elevation profile, not vertex positions in 3D.

### Radial derivation (not a per-stage “rebuild”)

The preview/game shell is **derived once** at `terrain_mesh` via `generate_planet_terrain_mesh_quad`:

```text
dir   = normalize(topology.sites[r])
pos   = dir * (1 + k_planet_elevation_scale * region_elevation[r])
```

`k_planet_elevation_scale` is `0.08` in [`PlanetTerrainMesh.hpp`](../gde/sim_core/include/world/PlanetTerrainMesh.hpp).

This is a **design choice** (RBG / 1843-style spherical height field), not the only possible planet model. It guarantees:

- one height per region along a fixed ray from the planet centre;
- no horizontal vertex drift from erosion or rivers;
- cheap O(regions) export; same rule as preview river segments in `WorldGenPreviewExport`.

Erosion does **not** move mesh vertices between stages. `apply_hydraulic_erosion` edits `triangle_elevation`; `sync_region_elevation_from_triangles` averages back onto regions; then `stage_terrain_mesh` fills vertex buffers **once**.

Equivalent options: update cached `dir[r]` in place after scalar changes, or displace in a vertex shader — same maths, no topology regen.

### Dual mesh vs quad shell

| Structure | Role |
|-----------|------|
| **Half-edge / dual triangles** | River downflow, flow accumulation, hydraulic erosion (scalar graph) |
| **Quad icosphere mesh** | Watertight render shell: one triangle per topology face, region vertices only |

Rivers solve on **triangle** elevations; the quad shell uses **region** elevation (after triangle values are synced and smoothed). Fine valley detail can be softened at export — that is separate from radial vs non-radial displacement.

### Streaming (no overworld mesh required)

Gameplay streaming ([`epic_game_world_streaming.md`](epic_game_world_streaming.md)) should sample a **PlanetSurfaceAtlas** (`unit_dir` → plate, elevation, moisture, biome). Chunks add local tangent-space detail; they do not need to regenerate or slide the icosphere.

`PlanetTerrainMesh` remains optional debug/export geometry for Godot preview.

Rendering may add normal maps, detail noise, and tessellation on top of the displaced mesh.

---

## Why this pipeline works

Three largely independent systems interact:

| System   | Role                          |
|----------|-------------------------------|
| Geology  | Plates, boundaries, uplift    |
| Climate  | Temperature, moisture, wind   |
| Erosion  | Hydraulic and thermal shaping |

Geology sets large-scale form; climate and erosion refine surface detail and hydrology.

---

## Mapping to Growth

Stages are registered in C++ by stable `stage_id` strings; order comes from `world_gen_pipelines.xml`. `PlanetGlobePipeline` runs the list sequentially.

Default pipeline `default_globe` (see [moddability.md](moddability.md)): topology → half_edge_mesh → region_neighbours → tectonic_plates → plate_properties → elevation → moisture → triangle_values → priority_flood → river_downflow → river_flow → erosion → river_carve → terrain_mesh (optional).

Implementation lives under `gde/sim_core/include/world/` (`IcosphereEngine`, `TectonicPlateAssigner`, `ElevationAssigner`, `RiverFlow`, `PlanetTerrainMesh`, etc.), orchestrated by `PlanetGlobePipeline`.

### Session atlas vs preview marshal

After world gen, **`PlanetSurfaceAtlasBuilder`** copies compact gameplay fields from `PlanetGlobe` into **`SimBridge`** (fixed topology + per-region plate, elevation, moisture). Temperature is derived at query time (`OverworldSurfaceSampler`), not stored. The Godot marshal dict remains for **`SpherePreview`** debug layers only; pressing **Start** commits the session and enters chunk streaming without re-uploading arrays. See [game_scene_order.md](game_scene_order.md) step 5b.
