# FauxBoy

This is a C++23 Game Boy emulator with m-cycle granularity aiming for just enough cycle-accuracy to load and play common
ROMs

The goal at the moment is to implement the CPU and graphics, sound support is out of scope but will be considered if the
technical challenge is interesting enough

## Dependencies

Below are dependencies and their versions used during development for the different build targets

The current state of the repo is dev-first and centered around my dev environment which uses Nix, will improve the
dependency management and build process when the repo reaches a state to start distributing binaries

### Main

[SDL3](https://github.com/libsdl-org/SDL) (coming soon)

### Test

[Catch2](https://github.com/catchorg/Catch2): `3.8.1` \
[simdjson](https://github.com/simdjson/simdjson): `4.2.2`

## Building

```shell
cmake --preset <preset>
cmake --build ./build/<preset>
```

## Running Tests

### SingleStepTests

```shell
cmake --preset gcc-release-tests
cmake --build build/gcc-release-tests
./build/gcc-release-tests/test/unit_tests --single-step-tests-dir '<json_root_path>' '[single-step-tests]'
```
