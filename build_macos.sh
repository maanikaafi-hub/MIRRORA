#!/bin/bash
# Builds a universal (Apple Silicon + Intel) macOS VST3 + AU.
# JUCE is used from $JUCE_DIR, ./JUCE, ../JUCE or downloaded automatically.
set -e
cmake -S . -B build -G Xcode -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
cmake --build build --config Release --target MIRRORA_VST3 MIRRORA_AU --parallel

echo
echo "Built:"
echo "  build/MIRRORA_artefacts/Release/VST3/MIRRORA.vst3"
echo "  build/MIRRORA_artefacts/Release/AU/MIRRORA.component"
