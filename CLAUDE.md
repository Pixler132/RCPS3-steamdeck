# CLAUDE.md

## Motivation

The goal of working in this tree is to get **3D Dot Game Heroes (USA, BLUS30490)** playable on a Steam Deck.
The game boots and runs, but **stalls/goes black after a certain point**. We want to:

1. Reproduce the stall reliably on a locally built RPCS3 (not the AppImage).
2. Profile and instrument it to understand *why* it stalls (what each PPU/SPU/RSX thread is waiting on).
3. Reason about the root cause in the emulator code, step by step, before changing anything.
4. Land a minimal, targeted fix and verify it doesn't regress other games (Ratchet & Clank Collection, Sly Collection are also installed).

Go slowly: gather evidence first and form hypotheses before editing code.

This file holds **verified facts only**: environment, paths, build steps, external references.
Hypotheses, conclusions and session state do **not** go here. They travel through the owner's
pasted handoffs, so the owner reviews them and each session takes a fresh look, not
inheriting a previous agent's theory.

## Environment

- Device: Steam Deck (SteamOS, AMD Van Gogh APU, 4C/8T, ~14.5 GiB RAM). Root filesystem is read-only by default (`sudo steamos-readonly disable` to unlock); the owner permits host package installs for this project. SteamOS updates wipe host installs.
- VS Code runs as a Flatpak, so Claude's shell is inside the Flatpak sandbox. Run host commands with `flatpak-spawn --host ...`.
- All building happens in a distrobox container named `rpcs3-build` (Arch Linux, passwordless sudo). From the sandbox:
  `flatpak-spawn --host distrobox enter rpcs3-build -- <command>`
- Git: `origin` is the owner's repo Pixler132/RCPS3-steamdeck; `upstream` is RPCS3/rpcs3 (shallow clone). `main` sits on top of upstream `master`. `CLAUDE.md` and `.claude/skills/` are committed; `Game/`, game/firmware/key files and RPCS3 user data are ignored via `.gitignore`.
- Game image: `~/Emulation/roms/ps3/3D Dot Game Heroes (USA).iso` (identical copy in `Game/`, with `.dkey` and `PS3UPDAT.PUP` firmware)
- RPCS3 config: `~/.config/rpcs3/` (shared between the AppImage and our build). Per-game config would go in `~/.config/rpcs3/custom_configs/config_BLUS30490.yml` (none exists yet; `custom_configs/` is empty as of 2026-10-25)
- Logs: `~/.cache/rpcs3/RPCS3.log` (current run), `RPCS3.log.gz` (previous run)
- Baseline AppImage: `~/.config/rpcs3/rpcs3.AppImage`, v0.0.43-20290-eda7621f

## Build

From the repo root, inside the container:

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DUSE_LTO=OFF \
  -DCMAKE_EXE_LINKER_FLAGS=-fuse-ld=lld -DCMAKE_MODULE_LINKER_FLAGS=-fuse-ld=lld -DCMAKE_SHARED_LINKER_FLAGS=-fuse-ld=lld
cmake --build build -j6
build/bin/rpcs3
```

Use `RelWithDebInfo` so `perf`/`gdb` give symbolized stacks. LTO is off to keep incremental rebuilds fast and link-time RAM manageable on 16 GB; `-j6` leaves headroom so the Deck doesn't swap.
Build/configure logs go to `build/*.log`.

lld is required (matches upstream `.ci/build-linux.sh`): with the default GNU ld, the final link fails with
`undefined reference to ff_init_half2float_tables`. `3rdparty/CMakeLists.txt` lists the prebuilt ffmpeg static libs
with avutil before swscale, and GNU ld resolves static archives in order. lld does not.

## References

- Upstream issue: RPCS3 #4433 "3D Dot Game Heroes random freezing" (Steam Deck duplicate: #11714).
- PR #19824 "lv2: Add PPU event wake-up delay" (merged 2026-10-14) is linked to #4433. It adds `Core: PPU Event Wake-Up Delay` (0–200 µs).
  The PR's stated reasoning and user reports on the issue are claims from those pages, not verified in this tree for this stall.
- Community patches for BLUS30490 in `~/.config/rpcs3/patches/patch.yml` (FPS unlock, DoF) target game version 01.01 only (PPU hash 27a382ce...).
