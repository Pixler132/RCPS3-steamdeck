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
- Game update 01.01 installed 2026-10-26 from `~/Downloads/UP0005-BLUS30490_00-M3DDGHPATCH00001-A0101-V0100-PE.pkg`
  into `/home/deck/Emulation/storage/rpcs3/dev_hdd0/game/BLUS30490/` (the stock AppImage's dev_hdd0); copied with the rest of
  dev_hdd0 into our build's own data dir on 2026-10-27 (see below).
  Runs before this date are disc version 01.00 (PPU hash `9696f609...`); later runs are 01.01. The installer warned
  "Missing 11233 bytes from PKG total files size" and then reported success. Install log: `runs/2026-10-26-update-0101-install/`.
- Our build and the stock AppImage are isolated since 2026-10-27. Our build runs only through
  `~/.local/opt/rpcs3-steamdeck/bin/rpcs3-steamdeck.sh` (installed copy of `tools/rpcs3-host.sh`), which sets
  `XDG_CONFIG_HOME`/`XDG_CACHE_HOME` to `~/.local/share/rpcs3-steamdeck/{config,cache}`:
  - config: `~/.local/share/rpcs3-steamdeck/config/rpcs3/` (per-game: `custom_configs/config_BLUS30490.yml`; Player 1 pad: Evdev "Microsoft X-Box 360 pad 0",
    Steam Input's virtual gamepad 28de:11ff, mapping copied from EmuDeck's template 2026-10-27; previous DS4 setup in `Default.yml.bak-ds4-evdev-2026-10-27`)
  - dev_hdd0 (saves, trophies, game updates): `~/.local/share/rpcs3-steamdeck/dev_hdd0/`, set in that config's `vfs.yml`
  - logs: `~/.local/share/rpcs3-steamdeck/cache/rpcs3/RPCS3.log` (current run), `RPCS3.log.gz` (previous run)
  - `RPCS3_BIN=build/bin/rpcs3` runs a fresh build against the same data.
  - ES-DE's RPCS3 find rule and `~/.local/share/applications/rpcs3-steamdeck.desktop` point at that launcher.
    EmuDeck resets/refreshes of ES-DE overwrite `es_find_rules.xml`; `tools/esde-restore-steamdeck.sh` repoints it.
- Stock AppImage: `~/Applications/rpcs3.AppImage` (self-updates; v0.0.43-20412-744126ca as of 2026-10-27), launched by EmuDeck's
  `~/Emulation/tools/launchers/rpcs3.sh` and the "RPCS3 AppImage" menu entry. Uses `~/.config/rpcs3/`, `~/.cache/rpcs3/`
  and `~/Emulation/storage/rpcs3/dev_hdd0/`. EmuDeck updates of RPCS3 overwrite files in `~/.config/rpcs3/`.

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
