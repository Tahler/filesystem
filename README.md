## Project layout

- `mkfs`: initializes the filesystem (executable)
- `rtfs`: interfaces with the filesystem (shared library)

## Bugs

- There is a flaw in the interconnection of Makefiles here: they depend on
  hardcoded filenames

__TODO: grep -rH TODO__
