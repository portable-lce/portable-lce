# Portable LCE

<div align="center">

![](.github-assets/transrights.png) ![](.github-assets/progress.png) ![](.github-assets/freepalestine.gif) ![](.github-assets/internetarchive.gif) ![](.github-assets//ieget-an.gif) ![](.github-assets/minecraft.gif) ![](.github-assets/powered-llvm.gif)
![](.github-assets/opengl.gif) ![](.github-assets/sgi.gif) ![](.github-assets/not-binary.png) ![](.github-assets/adobe_getflash2.gif) ![](.github-assets/flash_get_20010813.gif) ![](.github-assets/SiliconValley_7479_English_imagens_get_flashplayer.gif) ![](.github-assets/problematic-media.gif) ![](.github-assets/seal.gif) ![](.github-assets/notepad-logo3.webp) ![](.github-assets/hrt-e2.gif) ![](.github-assets/4j.png)

</div>

---

This project is a heavily modified version of the Minecraft Console Legacy Edition codebase, aimed at porting old Minecraft (TU19/1.6.1) to different platforms and refactoring the codebase to improve organization and use modern C++ features.

## Status

|  | **Linux** <br/> [![Linux](https://github.com/portable-lce/portable-lce/actions/workflows/build-linux.yml/badge.svg)](https://github.com/portable-lce/portable-lce/actions/workflows/build-linux.yml) | **Windows** <br/> [![Windows](https://github.com/portable-lce/portable-lce/actions/workflows/build-windows.yml/badge.svg)](https://github.com/portable-lce/portable-lce/actions/workflows/build-windows.yml) | **macOS** <br/> [![macOS](https://github.com/portable-lce/portable-lce/actions/workflows/build-macos.yml/badge.svg)](https://github.com/portable-lce/portable-lce/actions/workflows/build-macos.yml) |
| - | - | - | - |
| **app** | `desktop` | `desktop` | `desktop` |
| **ui** | `java`, `shiggy`[^2] | `java`, `shiggy`[^2] | `java` |
| **fs** | `std` | `std` | `std` |
| **renderer** | `gl` | `gl` | `gl` |
| **sound** | `miniaudio` | `miniaudio` | `miniaudio` |
| **input** | `sdl2` | `sdl2` | `sdl2` |
| **thread** | `std` | `std` | `std` |
| **game** | `stub` | `stub` | `stub` |
| **network** | `stub` | `stub` | `stub` |
| **storage** | `stub` | `stub` | `stub` |
| **profile** | `stub` | `stub` | `stub` |
| **leaderboard** | `stub` | `stub` | `stub` |

[^2]: `-Dui_backend=shiggy` supports x86-64 architectures only.

> [!TIP]
>
> This table describes the current backend used for each game component on each platform. If a backend is `stub`, that means that the game uses a [stubbed implementation](https://en.wikipedia.org/wiki/Method_stub) and the feature is unsupported at the moment. In some cases (e.g. leaderboards and profile) it makes sense to use a stubbed implementation, since we don't have access to console services like Xbox live on desktop operating systems. In other cases, it is used temporarily while work is done to properly implement the feature (such as storage for world/DLC saving and loading).

These platforms are currently work-in-progress:
- **Android**: Game runs, but the port predates many refactors and therefore can't be easily upstreamed at the moment. `ui-backend=java` only.
- **Emscripten**: Works except for audio. Predates a major refactor, and requires a rebase. `ui-backend=java` only.

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
./build/targets/app/Minecraft.Client
```

<!-- ### View the online documentation [here](https://portable-lce.github.io/portable-lce). -->

## Generative AI Policy

Submitting code to this repository authored by generative AI tools (LLMs, agentic coding tools, etc...) is strictly forbidden (see [CONTRIBUTING.md](./CONTRIBUTING.md)). Pull requests that are clearly vibe-coded or written by an LLM will be closed. Contributors are expected to both fully understand the code that they write **and** have the necessary skills to *maintain it*.
