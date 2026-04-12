# <img src=".github-assets/logo.jpg" alt="Logo" width="50" height="50" style="vertical-align: middle;"> Portable LCE

![](.github-assets/transrights.png) ![](.github-assets/progress.png) ![](.github-assets/freepalestine.gif) ![](.github-assets/internetarchive.gif) ![](.github-assets//ieget-an.gif) ![](.github-assets/minecraft.gif) ![](.github-assets/powered-llvm.gif)
![](.github-assets/opengl.gif) ![](.github-assets/adobe_getflash2.gif) ![](.github-assets/flash_get_20010813.gif) ![](.github-assets/SiliconValley_7479_English_imagens_get_flashplayer.gif) ![](.github-assets/problematic-media.gif)
---

Portable LCE is a modified version of the Minecraft Console Legacy Edition, aimed at porting old Minecraft to different platforms (such as Linux, Android, Emscripten, etc.) and refactoring the codebase to improve organization and use modern C++ features.

## Scope & Platform Support

At the moment, we're aiming to support the following platforms:

Please note that these percentages are **estimates** and do not necessarily reflect the final playability of the game on each platform.

- Linux (~90%)
- Emscripten (~10%) [[Check the Emscripten Branch](https://github.com/portable-lce/portable-lce/tree/feat/emscripten)]
- macOS (not started) [No official support but people have been able to run the game on MacOS]
- iOS (not started)
- Android (~35%)

> [!WARNING]
> There is NO Windows support, for that, go to [MCLCE/MinecraftConsoles](https://github.com/MCLCE/MinecraftConsoles). 

> All efforts are focused towards a native Linux port, OpenGL rendering pipeline, and modernizing the existing LCE codebase/tooling to make future platform ports easier.
> 
> `Windows64` and other platforms originally supported by LCE are currently unsupported, since the original Visual Studio tooling has been stripped from this repository and replaced with our own.

---

## Join our community:
* **Discord:** https://discord.gg/SC6WCZezry

## Building (Linux)

### Prerequisites

#### System Libraries

Debian/Ubuntu:
```bash
sudo apt-get install -y build-essential libsdl2-dev libgl-dev libglu1-mesa-dev libpthread-stubs0-dev
```

Arch/Manjaro:
```bash
sudo pacman -S base-devel pkgconf sdl2-compat mesa glu
```

Fedora/Red Hat/Nobara:
```bash
sudo dnf install gcc gcc-c++ make SDL2-devel mesa-libGL-devel mesa-libGLU-devel openssl-devel
```

#### Toolchain

This project requires a C++23 compiler with full standard library support.

**If your distro ships GCC 15+**, you're good - just use the system compiler:

```bash
meson setup build
```

**If your distro ships an older GCC:** install LLVM with libc++ and use the provided toolchain file:

```bash
# Debian/Ubuntu
wget https://apt.llvm.org/llvm.sh
chmod +x llvm.sh
sudo ./llvm.sh 20
sudo apt install libc++-20-dev libc++abi-20-dev
```

```bash
# Fedora/RHEL (if needed)
sudo dnf install clang lld libcxx-devel libcxxabi-devel
```

Then configure with the LLVM native file (see Configure & Build below).

#### Meson + Ninja

Install [Meson](https://mesonbuild.com/) and [Ninja](https://ninja-build.org/):

```bash
pip install meson ninja
```

Or follow the [Meson quickstart guide](https://mesonbuild.com/Quick-guide.html).

#### Docker (alternative)

If you don't want to install dependencies, use the included devcontainer. Open the project in VS Code with the [Dev Containers](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-containers) extension, or build manually:

```bash
docker build -t portable-lce-dev .devcontainer/
docker run -it --rm -v $(pwd):/workspaces/portable-lce -w /workspaces/portable-lce portable-lce-dev bash
```

### Configure & Build

```bash
# If using system GCC 15+
meson setup build

# If using LLVM/libc++
meson setup --native-file ./scripts/llvm_native.txt build

# Compile
meson compile -C build
```

The binary is output to:

```
./build/targets/app/Minecraft.Client
```

#### Clean

To perform a clean compilation:

```bash
meson compile --clean -C build
```

...or to reconfigure an existing build directory:

```bash
meson setup --native-file ./scripts/llvm_native.txt build --reconfigure
```

...or to hard reset the build directory:

```bash
rm -r ./build
meson setup --native-file ./scripts/llvm_native.txt build
```

---

## Running

Game assets are automatically copied to the build output directory during compilation. Run from that directory:

```sh
cd build/targets/app
./Minecraft.Client
```

---

### View the online documentation [here](https://portable-lce.github.io/portable-lce).

---

## Generative AI Policy

Submitting code to this repository authored by generative AI tools (LLMs, agentic coding tools, etc...) is strictly forbidden (see [CONTRIBUTING.md](./CONTRIBUTING.md)). Pull requests that are clearly vibe-coded or written by an LLM will be closed. Contributors are expected to both fully understand the code that they write **and** have the necessary skills to *maintain it*.
