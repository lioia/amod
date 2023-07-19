# AMOD Project

Performance comparison for $1|r_j|\sum{C_j}$ problem with 4 different models:

- precedence variables
- positional variables
- time-indexed variables
- custom heuristics

## Supported OS

- Linux is officially supported and tested. 
- macOS should work out-of-the box.
- Windows support is present but untested

## Building

```bash
cmake -B build
cmake --build build
```

## Clean

```bash
cmake -B build
cmake --build build --target clean
```

## Gurobi Installation

### Download

```bash
wget https://packages.gurobi.com/10.0/gurobi10.0.2_linux64.tar.gz
mv gurobi10.0.2_linux64.tar.gz $HOME/bin
cd $HOME/bin
tar -xzvf gurobi10.0.2_linux64.tar.gz
rm gurobi10.0.2_linux64.tar.gz
```

### License Installation

- Download license from [here](https://license.gurobi.com/manager/licenses)
- Copy the `gurobi.lic` to `$HOME/bin/gurobi1002/`

### Environment Variables Setup

Copy the following lines in `.bashrc`

```
export GUROBI_HOME="$HOME/bin/gurobi1002/linux64"
export GRB_LICENSE_FILE="$HOME/bin/gurobi1002/gurobi.lic"
export PATH="${PATH}:${GUROBI_HOME}/bin"
export LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:${GUROBI_HOME}/lib"
```

## Neovim (`clangd`) Configuration

```bash
cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=1
cp build/compile_commands.json .
```

