#!/usr/bin/env bash
#
# StudioPatchbay — macOS release: archive -> export (Developer ID) ->
# DMG -> notarize -> staple. Produces a signed, notarized, stapled
# dist/StudioPatchbay-<version>.dmg that passes Gatekeeper on any Mac.
#
#     packaging/release.sh
#
# The signing identity + team are auto-detected from your keychain's
# "Developer ID Application" certificate — override with the DEV_ID_APP / TEAM
# env vars if you have more than one. Notarisation uses a keychain profile,
# set once (no password lives in this script):
#   xcrun notarytool store-credentials "hardwarerecall-notary" \
#       --apple-id "YOUR_APPLE_ID_EMAIL" \
#       --team-id  "YOUR_TEAM_ID" \
#       --password "the-app-specific-password"
# Override the profile name via NOTARY_PROFILE=... (e.g. to reuse an existing one).
#
set -euo pipefail

SCHEME="StudioPatchbay"

# Auto-detected from the keychain — no personal data committed to the repo.
DEV_ID_APP="${DEV_ID_APP:-$(security find-identity -v -p codesigning 2>/dev/null \
    | grep -m1 'Developer ID Application:' | sed -E 's/^[^"]*"([^"]*)".*/\1/')}"
TEAM="${TEAM:-$(printf '%s' "${DEV_ID_APP}" | sed -E 's/.*\(([A-Z0-9]+)\)$/\1/')}"
NOTARY_PROFILE="${NOTARY_PROFILE:-hardwarerecall-notary}"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$REPO"

BUILD="$REPO/build"
DIST="$REPO/dist"
ARCHIVE="$BUILD/StudioPatchbay.xcarchive"
EXPORT="$BUILD/export"
APP="$EXPORT/StudioPatchbay.app"

say() { printf '\n\033[1;36m==> %s\033[0m\n' "$*"; }
die() { printf '\n\033[1;31mERROR: %s\033[0m\n' "$*" >&2; exit 1; }

# ---- Preflight -------------------------------------------------------------
[ -n "$DEV_ID_APP" ] || die "No 'Developer ID Application' identity in the keychain. Set DEV_ID_APP=\"Developer ID Application: NAME (TEAMID)\"."
[ -n "$TEAM" ]       || die "Could not determine the Team ID. Set TEAM=..."
xcrun --find notarytool >/dev/null 2>&1 || die "notarytool not found (install Xcode)."
say "Signing as: $DEV_ID_APP  (team $TEAM)"

rm -rf "$BUILD"; mkdir -p "$BUILD" "$DIST"

# ---- 1. Archive (Release, hardened runtime) --------------------------------
say "Archiving (Release)"
xcodebuild -project StudioPatchbay.xcodeproj -scheme "$SCHEME" -configuration Release \
    -destination 'generic/platform=macOS' \
    -archivePath "$ARCHIVE" archive \
    -allowProvisioningUpdates \
    DEVELOPMENT_TEAM="$TEAM" \
    ENABLE_HARDENED_RUNTIME=YES \
    | tail -2

# ---- 2. Export with the Developer ID certificate ---------------------------
say "Exporting (Developer ID)"
cat > "$BUILD/ExportOptions.plist" <<PLIST
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>method</key><string>developer-id</string>
    <key>teamID</key><string>$TEAM</string>
    <key>signingStyle</key><string>automatic</string>
</dict>
</plist>
PLIST
xcodebuild -exportArchive -archivePath "$ARCHIVE" -exportPath "$EXPORT" \
    -exportOptionsPlist "$BUILD/ExportOptions.plist" | tail -2
[ -d "$APP" ] || die "Export failed — no StudioPatchbay.app at $APP"

VERSION="$(/usr/libexec/PlistBuddy -c 'Print CFBundleShortVersionString' "$APP/Contents/Info.plist")"
say "Version $VERSION — $(lipo -archs "$APP/Contents/MacOS/StudioPatchbay" 2>/dev/null || echo 'arch?')"
codesign --verify --strict --verbose=2 "$APP"

# ---- 3. Build the DMG (app + Applications shortcut) ------------------------
say "Building DMG"
STAGE="$(mktemp -d)"; trap 'rm -rf "$STAGE"' EXIT
cp -R "$APP" "$STAGE/"
ln -s /Applications "$STAGE/Applications"
DMG="$DIST/StudioPatchbay-$VERSION.dmg"
rm -f "$DMG"
hdiutil create -volname "StudioPatchbay" -srcfolder "$STAGE" -ov -format UDZO "$DMG" >/dev/null
codesign --force --timestamp --sign "$DEV_ID_APP" "$DMG"

# ---- 4. Notarize + staple --------------------------------------------------
say "Notarizing (waits for Apple; usually 1-5 min)"
xcrun notarytool submit "$DMG" --keychain-profile "$NOTARY_PROFILE" --wait

say "Stapling ticket"
xcrun stapler staple "$DMG"
xcrun stapler validate "$DMG"

say "DONE → $DMG"
echo "Upload this DMG to the GitHub Release. It passes Gatekeeper on any Mac"
echo "(macOS 15 Sequoia or later)."
