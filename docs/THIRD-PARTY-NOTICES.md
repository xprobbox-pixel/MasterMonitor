# Dependency and asset notices

- JUCE 8.0.9, pinned commit f72bad64d29715216226685810c5196bd0d79d77. See licenses/JUCE.md and JUCE's module-level notices in the fetched source. JUCE licensing must suit the intended distribution: obtain an appropriate commercial licence or satisfy the applicable open-source licence terms. An unsigned development artifact is not a commercial distribution authorisation.
- libebur128 1.2.6, pinned commit 67b33abe1558160ed76ada1322329b0e9e058b02, MIT. Copyright Jan Kokemüller. Full licence in licenses/libebur128.txt; embedded filter and queue notices are in licenses/ebur-dependencies and the source headers.
- Steinberg VST3 SDK code is supplied through the pinned JUCE modules; see modules/juce_audio_processors/format_types/VST3_SDK/LICENSE.txt in that checkout. Apple platform SDKs are supplied by the native build environment.
- Supplied MASTER MONITOR logo and screenshot are user-provided assets. No transfer of trademark or asset rights is implied. The original logo is embedded without generative alteration. Screenshot is retained as a reference asset.
- pluginval 1.0.3 is an external development validator, GPL-licensed; it is not linked into the plugin or shipped in installers. FFmpeg is an external validation tool, not bundled or linked.

Development identity values are in cmake/Identity.cmake. Replace them before a public release. No blanket licence is assigned to the user's artwork or project; resolve the desired source licence and JUCE obligations before distribution.
