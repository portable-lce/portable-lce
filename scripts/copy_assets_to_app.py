#!/usr/bin/env python3

import os
import shutil
import sys
from pathlib import Path

project_source_root = Path(sys.argv[1])
build_root_dir = Path(sys.argv[2])
client_build_dir = Path(sys.argv[3])
media_archive = Path(sys.argv[4])
output_stamp = Path(sys.argv[5])

# At runtime, the client expects a `Common/` folder from the resources source and a compiled
# media archive (LinuxMedia.arc) inside of said folder at its current working directory to launch.
#
# `meson install` also handles this, but installs it to system folders, which can be annoying for
# testing. Since we want a way to run it straight from `/build` when debugging, we do this instead.
#
# this script doesn't handle copying the same way `meson install` does but it should be good enough
dest_common = Path(client_build_dir / "Common")
src_assets = Path(project_source_root / "targets" / "resources")

# clear out any old assets
if dest_common.exists():
    shutil.rmtree(dest_common, ignore_errors=True)
    shutil.rmtree(client_build_dir / "music", ignore_errors=True)
    shutil.rmtree(client_build_dir / "Sound", ignore_errors=True)
    # XXX: Check "copy DLC" bellow for info
    # shutil.rmtree(client_build_dir / "DurangoMedia", ignore_errors=True)

# copy `resources/Common` into the build directory for the client.
shutil.copytree(
    src_assets / "Common",
    dest_common,
)

# copy the media archive to `Common/Media` inside the folder we just copied.
shutil.copy(media_archive, client_build_dir / "Common" / "Media")

# copy music and Sound
shutil.copytree(
    src_assets / "Common" / "music",
    client_build_dir / "music"
)
shutil.copytree(
    src_assets / "DurangoMedia" / "Sound", 
    client_build_dir / "Sound"
)

# copy DLC
# XXX: The DLC path is handled inside of 4JLibs, the Windows64 build expects `DurangoMedia/DLC` to load DLC data from
# shutil.copytree(
#     src_assets / "DurangoMedia" / "DLC", 
#     client_build_dir / "DurangoMedia" / "DLC"
# )

# modify the stamp so this only happens when client or media_archive targets are changed
output_stamp.touch()