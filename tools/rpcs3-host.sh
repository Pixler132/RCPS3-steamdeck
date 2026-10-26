#!/bin/sh
# Host entry point for this fork's RPCS3 (used by ES-DE in place of the AppImage).
# The binary links against the rpcs3-build container's libraries (glibc 2.44, Qt, LLVM),
# so it runs inside that distrobox. It runs the installed snapshot, not build/bin, so a
# rebuild does not change what ES-DE launches. Refresh the snapshot with:
#   cmake --install build --prefix "$HOME/.local/opt/rpcs3-steamdeck"
exec /usr/bin/distrobox enter rpcs3-build -- "$HOME/.local/opt/rpcs3-steamdeck/bin/rpcs3" "$@"
