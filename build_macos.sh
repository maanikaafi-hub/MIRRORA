#!/bin/bash
set -e
if [ -z "$JUCE_DIR" ]; then echo "Set JUCE_DIR to your JUCE source folder first."; exit 1; fi
cmake -S . -B build -G Xcode
cmake --build build --config Release --target MIRRORA_VST3 MIRRORA_AU
