# Camelot
![Build and Test status](https://github.com/Hanz98/Camelot/actions/workflows/build.yaml/badge.svg)

## Features

- **MCAP Data Reader**: Comprehensive support for reading autonomous vehicle data from MCAP files
  - Objects, Markers, Lanes, and PointCloud message types
  - Flexible filtering by topic and timestamp
  - Clean C++20 API with extensive error handling
  - See [docs/MCAP_Reader.md](docs/MCAP_Reader.md) for detailed documentation

## Setup project 

### Linux 

On Linux, run `setup.sh` file to install dependencies with Conan, and build the CMake project. 

### Windows 

On Windows, run `setup.ps1` file to generate a Visual Studio solution file. You should then be able to open the project folder in Visual Studio.

Also, make sure to install the CMake and Conan extensions to Visual Studio.

## Examples

The `examples/` directory contains demonstration applications:
- `mcap_reader_example` - Full API demonstration
- `mcap_summary` - CLI tool for inspecting MCAP files

Build the examples with the main project and run:
```bash
./build/bin/mcap_summary data.mcap --all
```

