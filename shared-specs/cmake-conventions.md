# CMake Conventions

**Classification:** Human-authored shared spec
**Applies to:** All `CMakeLists.txt` files generated for examples in this repository

---

## Project-Level CMakeLists.txt

The top-level `CMakeLists.txt` for every example must follow this exact structure:

```cmake
cmake_minimum_required(VERSION 3.24)

# Optional: include managed components
# include($ENV{IDF_PATH}/tools/cmake/project.cmake)  # already pulled in below

project(<example_name>)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
```

- `cmake_minimum_required(VERSION 3.24)` is required for full ESP-IDF v5.x CMake feature support.
- The `project()` call must come **before** `include(project.cmake)`.
- Do not add `add_subdirectory()` at the top level — ESP-IDF's `project.cmake` discovers components automatically.
- Extra components (not in `components/`) can be added via `set(EXTRA_COMPONENT_DIRS ...)` before the `include`.

---

## Component CMakeLists.txt

Every component in `components/<name>/` must have its own `CMakeLists.txt`:

```cmake
idf_component_register(
    SRCS
        "src/<name>.c"
        # additional source files
    INCLUDE_DIRS
        "include"
    REQUIRES
        # direct ESP-IDF dependencies only
        driver
        esp_timer
        nvs_flash
)
```

Rules:

- `SRCS` lists only the `.c` files for this component. Do not glob.
- `INCLUDE_DIRS` exposes the `include/` directory to consumers. Private headers go in `src/` and are not listed here.
- `REQUIRES` lists only **direct** dependencies. Transitive dependencies are handled automatically by CMake. Over-listing in `REQUIRES` is harmless but noisy; under-listing causes linker failures.
- `PRIV_REQUIRES` is used for dependencies needed only by the implementation (`.c` files), not by the public header.
- Do not use `target_link_libraries()` directly — always go through `REQUIRES`/`PRIV_REQUIRES`.

---

## main Component

`main/CMakeLists.txt` follows the same pattern:

```cmake
idf_component_register(
    SRCS
        "main.c"
    INCLUDE_DIRS
        "."
    REQUIRES
        driver
        # application-level components
        <component_name>
)
```

- `main/` is a special component — it is always built and provides `app_main`.
- The `INCLUDE_DIRS "."` is included for local header access; omit if `main/` has no local headers.
- `Kconfig.projbuild` for project-wide Kconfig symbols **must** live in `main/` (ESP-IDF requirement — see `AIGenLessonsLearned.md`).

---

## Kconfig Standards

### File Placement

| File | Location | Purpose |
|---|---|---|
| `Kconfig.projbuild` | `main/Kconfig.projbuild` | Project-wide Kconfig menu; symbols under `CONFIG_` prefix |
| `Kconfig` | `components/<name>/Kconfig` | Component-specific Kconfig; symbols under `CONFIG_` prefix |

### Kconfig Style

```kconfig
menu "Example Configuration"

    config EXAMPLE_SENSOR_POLL_RATE_HZ
        int "Sensor poll rate (Hz)"
        default 10
        range 1 100
        help
            Rate at which the sensor is polled in Hz.
            Higher values increase power consumption.

    config EXAMPLE_WIFI_SSID
        string "Wi-Fi SSID"
        default "myssid"
        help
            SSID of the Wi-Fi network to connect to.

endmenu
```

Rules:

- Every `config` symbol must have a `help` text.
- Use `default` values that produce a working build on first try.
- Use `range` for `int` and `hex` types wherever bounds are meaningful.
- Group related symbols in a `menu`/`endmenu` block named after the component or example.
- Use `depends on` to hide symbols that are irrelevant when a parent feature is disabled.
- Symbol names are SCREAMING_SNAKE_CASE. The `CONFIG_` prefix is added automatically.

---

## Managed Components (idf_component.yml)

When an example depends on a component from the ESP-IDF Component Registry, declare it in `main/idf_component.yml`:

```yaml
## IDF Component Manager Manifest File
dependencies:
  espressif/esp_modem: "^1.1.0"
  idf: ">=5.3.0"
```

- Pin minor versions (`^1.1.0`) rather than using `*`. This allows patch updates but prevents breaking minor changes.
- Always specify the `idf` version constraint.
- Run `idf.py reconfigure` after editing `idf_component.yml` to regenerate the lock file.
- Commit `dependencies.lock` to ensure reproducible builds.

---

## Component Directory Layout

```
components/
  <component_name>/
    CMakeLists.txt
    Kconfig                 # optional; component-level Kconfig
    include/
      <component_name>.h    # single public header
    src/
      <component_name>.c    # primary implementation
      <internal_helper>.c   # additional implementation files if needed
      <internal_header>.h   # private headers (not exposed to consumers)
```

- Each component exposes exactly one public header. This enforces clear API boundaries.
- `src/` is not in `INCLUDE_DIRS` — internal headers are not visible to consumers.
- Test files live in `components/<name>/test/` if component-level tests are written.

---

## Forbidden CMake Patterns

| Pattern | Why forbidden |
|---|---|
| `file(GLOB ...)` to collect sources | Breaks incremental builds; CMake won't know when new files are added |
| `add_definitions(-DFOO)` | Use Kconfig instead; definitions must be traceable to config |
| `target_include_directories()` directly | Use `INCLUDE_DIRS` in `idf_component_register` |
| Hardcoded absolute paths | All paths must be relative or use `${CMAKE_CURRENT_SOURCE_DIR}` |
| `cmake_policy(SET CMP0000 OLD)` | Do not suppress policy warnings; fix the root cause |
