#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
build="${1:-build/mac-universal}"
out="${2:-release}"
mkdir -p "$out"
out="$(cd "$out" && pwd)"
version="$(python3 -c 'from scripts.identity import identity; print(identity()["MM_VERSION"])')"
bundle="$(python3 -c 'from scripts.identity import identity; print(identity()["MM_BUNDLE_ID"])')"
stage="$(mktemp -d "${TMPDIR:-/tmp}/mastermonitor.XXXXXX")"
trap 'rm -rf "$stage"' EXIT
mkdir -p "$out/macos" "$out/validation"
for kind in VST3 AU Standalone; do
 case "$kind" in
 VST3) extension=vst3; location=/Library/Audio/Plug-Ins/VST3;;
 AU) extension=component; location=/Library/Audio/Plug-Ins/Components;;
 Standalone) extension=app; location=/Applications;;
 esac
 item="MASTER MONITOR.$extension"
 mkdir -p "$stage/$kind"
 ditto "$build/MasterMonitor_artefacts/Release/$kind/$item" "$stage/$kind/$item"
 mkdir -p "$stage/$kind/$item/Contents/Resources/Licenses"
 ditto docs/licenses "$stage/$kind/$item/Contents/Resources/Licenses"
 cp docs/THIRD-PARTY-NOTICES.md "$stage/$kind/$item/Contents/Resources/Licenses/"
 binary="$stage/$kind/$item/Contents/MacOS/MASTER MONITOR"
 lipo "$binary" -verify_arch arm64 x86_64
 if [[ -n "${MM_APP_SIGN:-}" ]]; then
  codesign --force --options runtime --timestamp --sign "$MM_APP_SIGN" "$stage/$kind/$item"
 else
  codesign --force --sign - "$stage/$kind/$item"
 fi
 codesign --verify --deep --strict --verbose=2 "$stage/$kind/$item"
 ditto "$stage/$kind/$item" "$out/macos/$item"
 pkgbuild --analyze --root "$stage/$kind" "$stage/$kind.plist"
 python3 - "$stage/$kind.plist" <<'PY'
import plistlib,sys
p=sys.argv[1]
a=plistlib.load(open(p,'rb'))
for b in a: b['BundleIsRelocatable']=False
plistlib.dump(a,open(p,'wb'))
PY
 pkgbuild --root "$stage/$kind" --component-plist "$stage/$kind.plist" --identifier "$bundle.$kind" --version "$version" --install-location "$location" "$stage/$kind.pkg"
done
cat > "$stage/distribution.xml" <<EOF_XML
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="2">
<title>MASTER MONITOR $version</title>
<options customize="always" require-scripts="false" hostArchitectures="arm64,x86_64"/>
<domains enable_anywhere="false" enable_currentUserHome="false" enable_localSystem="true"/>
<choices-outline><line choice="VST3"/><line choice="AU"/><line choice="Standalone"/></choices-outline>
<choice id="VST3" title="VST3 Plugin"><pkg-ref id="$bundle.VST3"/></choice>
<choice id="AU" title="Audio Unit"><pkg-ref id="$bundle.AU"/></choice>
<choice id="Standalone" title="Standalone Application"><pkg-ref id="$bundle.Standalone"/></choice>
<pkg-ref id="$bundle.VST3" version="$version">VST3.pkg</pkg-ref>
<pkg-ref id="$bundle.AU" version="$version">AU.pkg</pkg-ref>
<pkg-ref id="$bundle.Standalone" version="$version">Standalone.pkg</pkg-ref>
</installer-gui-script>
EOF_XML
pkg="$out/MASTER-MONITOR-$version-macOS.pkg"
if [[ -n "${MM_INSTALLER_SIGN:-}" ]]; then
 productbuild --distribution "$stage/distribution.xml" --package-path "$stage" --sign "$MM_INSTALLER_SIGN" "$pkg"
else
 productbuild --distribution "$stage/distribution.xml" --package-path "$stage" "$pkg"
fi
pkgutil --check-signature "$pkg" > "$out/validation/package-signature.txt" 2>&1 || true
if [[ -n "${MM_NOTARY_PROFILE:-}" ]]; then
 [[ -n "${MM_APP_SIGN:-}" && -n "${MM_INSTALLER_SIGN:-}" ]] || { echo 'Notarisation requires both Developer ID identities.'; exit 1; }
 xcrun notarytool submit "$pkg" --keychain-profile "$MM_NOTARY_PROFILE" --wait --output-format json > "$out/validation/notarisation.json"
 python3 - "$out/validation/notarisation.json" <<'PY'
import json,sys
assert json.load(open(sys.argv[1]))['status']=='Accepted','Notarisation was not accepted'
PY
 xcrun stapler staple "$pkg"
 xcrun stapler validate "$pkg"
 spctl --assess --type install --verbose=2 "$pkg"
 # The separately delivered standalone app also receives its own stapled ticket.
 ditto -c -k --keepParent "$out/macos/MASTER MONITOR.app" "$stage/standalone.zip"
 xcrun notarytool submit "$stage/standalone.zip" --keychain-profile "$MM_NOTARY_PROFILE" --wait --output-format json > "$out/validation/standalone-notarisation.json"
 python3 - "$out/validation/standalone-notarisation.json" <<'PY_CHECK'
import json,sys
assert json.load(open(sys.argv[1]))['status']=='Accepted','Standalone notarisation was not accepted'
PY_CHECK
 xcrun stapler staple "$out/macos/MASTER MONITOR.app"
 xcrun stapler validate "$out/macos/MASTER MONITOR.app"
 spctl --assess --type execute --verbose=2 "$out/macos/MASTER MONITOR.app"
else
 echo 'Development package: no notarisation performed.'
 spctl --assess --type install --verbose=2 "$pkg" > "$out/validation/package-assessment.txt" 2>&1 || true
fi
python3 scripts/manifest.py "$out"
