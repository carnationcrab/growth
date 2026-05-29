# Example mod pack

Minimal override pack loaded after `core`. See `docs/moddability.md`.

To enable, add to `data/manifest.xml` load order:

```xml
<pack pack_id="example_mod" path="mods/example/" required="false"/>
```

After loading, `harvest_plant` interaction uses the modded display name and ui hint.
