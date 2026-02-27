# World generation pipeline

When the user submits the world generation form, the following runs in order.

## 1. Form submission

The world gen menu (e.g. **WorldGenMenu**) builds a `form_dict` from the UI and calls:

- **SimAPI.apply_world_gen_form(form_dict)**

Optionally, the game can also call **SimAPI.boot(defdb_path, form_dict)** at startup with the same dict; the C++ side then runs seed and planet generation during boot.

### Form keys (examples)

| Key             | Type   | Use |
|-----------------|--------|-----|
| `seed`          | string | User seed; empty → random seed. |
| `height_scale`  | int/float | WorldSeed.height_scale. |
| `octaves`       | int    | WorldSeed.octaves. |
| `frequency`     | int/float | WorldSeed.frequency. |
| `planet_preset` | string | `"Earthlike"`, `"OceanWorld"`, or `"Dry"`. |
| `world_size`    | string | Optional; for future world map scale. |
| `temperature`   | float  | Optional; for future climate. |
| `precipitation` | float  | Optional; for future climate. |

## 2. Seed generator

- **SeedGenerator.from_form(form)** (or **from_seed** if form is empty) produces a **WorldSeed** (value, height_scale, octaves, frequency).
- That **WorldSeed** is stored in the sim bridge and used for terrain and any other deterministic proc gen (world map, chunks).

## 3. Planet generator

- **planet_preset** is read from the form (default `"Earthlike"`).
- A **PlanetGenBlueprint** is created and **set_blueprint_to_preset(blueprint, preset_name)** is called.
- **blueprint.seed** is set to **WorldSeed.value**.
- **PlanetGenerator.generate_from_blueprint(blueprint)** produces a **PlanetGenome** (stored params: L_star, a, e, S_eff, M_p, R_p, P_rot, obliquity, p0, albedo, greenhouse, water_fraction, precipitation, material_tags, schema_version).
- The **PlanetGenome** is stored in the sim bridge. Derived values (g, P_orb, T_eq, scale_height, etc.) are computed on demand via **Planet(genome)**.

## 4. Stored state (sim bridge)

After **apply_world_gen_form** (or **boot** with form):

- **WorldSeed** – used for world map, chunk generation, streaming.
- **PlanetGenome** – used for physics, climate, and any planet-scale proc gen (e.g. day/year length, gravity, insolation).

Downstream systems (world map generator, chunk manager, environment sampler) read these from the bridge; they do not run the generators themselves.

## 5. Presets and blueprints

- Presets are defined in **data/core/defs/planet_presets.xml** (schema: **schemas/planet_presets.xsd**).
- C++ uses built-in preset logic for **Earthlike**, **OceanWorld**, and **Dry**; the XML documents the same structure for tooling or future loaders.
- **set_blueprint_to_random(blueprint, rng)** randomises preset and material weights for variety.

## 6. References

- **Planet genome and blueprint:** `gde/README.md` (Planet genome section).
- **World seed and form:** `gde/README.md` (World seed section).
- **Planet presets XML:** `data/core/defs/planet_presets.xml`.
