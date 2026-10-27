#!/bin/sh
# Host entry point for this fork's RPCS3 (used by ES-DE and the start menu in place of the AppImage).
# The binary links against the rpcs3-build container's libraries (glibc 2.44, Qt, LLVM),
# so it runs inside that distrobox. It runs the installed snapshot, not build/bin, so a
# rebuild does not change what ES-DE launches. Refresh the snapshot and this launcher with:
#   cmake --install build --prefix "$HOME/.local/opt/rpcs3-steamdeck"
#   install -m755 tools/rpcs3-host.sh "$HOME/.local/opt/rpcs3-steamdeck/bin/rpcs3-steamdeck.sh"
#
# Config, cache/logs and dev_hdd0 (saves, trophies, game updates) live under DATA, apart from
# the stock AppImage's ~/.config/rpcs3 and ~/.cache/rpcs3, so neither build overwrites the other.
# The dev_hdd0 location is set in DATA/config/rpcs3/vfs.yml.
# Set RPCS3_BIN to run another binary (e.g. build/bin/rpcs3) against the same data.
DATA="$HOME/.local/share/rpcs3-steamdeck"
BIN="${RPCS3_BIN:-$HOME/.local/opt/rpcs3-steamdeck/bin/rpcs3}"
exec /usr/bin/distrobox enter rpcs3-build -- \
	env XDG_CONFIG_HOME="$DATA/config" XDG_CACHE_HOME="$DATA/cache" "$BIN" "$@"
