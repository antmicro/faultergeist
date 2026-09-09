# export_placement

OpenROAD extension that writes die area and cell placement to a CSV file.
Such a file can be consumed by Faultergeist's fault generator.

## Export from ODB

With OpenROAD in `PATH`, run:

```
./export_placement design.odb -o cells.csv
```

You can also override the OpenROAD binary with `$OPENROAD` or `--openroad <path>`.
Requires a version of OpenROAD built with Python support.

## Use in OpenROAD scripts

This extension can also be used from OpenROAD scripts as a module:

```python
import sys
sys.path.insert(0, "/path/to/scripts/export_placement")
import export_placement
export_placement.export_placement(design.getBlock(), "cell_placement.csv")
```
