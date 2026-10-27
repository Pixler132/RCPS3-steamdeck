#!/bin/sh
# Point ES-DE's RPCS3 find rule at this fork's installed launcher.
# Run on the host after an EmuDeck reset/refresh of ES-DE, which replaces es_find_rules.xml
# and sends PS3 games back to the stock AppImage. Keeps a timestamped backup of the old file.
set -e
RULES="$HOME/ES-DE/custom_systems/es_find_rules.xml"
LAUNCHER="$HOME/.local/opt/rpcs3-steamdeck/bin/rpcs3-steamdeck.sh"

[ -x "$LAUNCHER" ] || { echo "missing launcher: $LAUNCHER" >&2; exit 1; }
cp -p "$RULES" "$RULES.$(date +%Y%m%d-%H%M%S).bak"

# Replace the first <entry> inside <emulator name="RPCS3">, whatever it currently points at.
awk -v launcher="$LAUNCHER" '
	/<emulator name="RPCS3">/ { in_rpcs3 = 1 }
	in_rpcs3 && !done && /<entry>/ { sub(/<entry>.*<\/entry>/, "<entry>" launcher "</entry>"); done = 1 }
	/<\/emulator>/ { in_rpcs3 = 0 }
	{ print }
' "$RULES" > "$RULES.tmp"
mv "$RULES.tmp" "$RULES"
grep -n -A4 '<emulator name="RPCS3">' "$RULES"
