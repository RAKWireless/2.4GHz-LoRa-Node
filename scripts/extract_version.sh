#!/bin/bash

# Simple version extraction for Makefile usage
# Usage: ./extract_version.sh path_to_changelog

CHANGELOG_FILE="$1"

if [ ! -f "$CHANGELOG_FILE" ]; then
    echo "v1.0.0"
    exit 0
fi

# Extract the latest version from CHANGELOG.md
# Look for pattern like "# [1.1.0] 2025-7-2"
VERSION=$(grep -m1 "^# \[.*\]" "$CHANGELOG_FILE" | sed 's/^# \[\(.*\)\].*/\1/')

if [ -z "$VERSION" ]; then
    echo "v1.0.0"
else
    echo "v$VERSION"
fi

if [ -z "$VERSION" ]; then
    echo "Error: Could not extract version from CHANGELOG.md"
    exit 1
fi

echo "Extracted version: $VERSION"

# Create output directory if it doesn't exist
OUTPUT_DIR=$(dirname "$OUTPUT_FILE")
mkdir -p "$OUTPUT_DIR"

# Generate version.h file
cat > "$OUTPUT_FILE" << EOF
/*
 * Auto-generated version file
 * Do not edit manually - this file is generated from CHANGELOG.md
 * Generated on: $(date)
 */

#ifndef _VERSION_H
#define _VERSION_H

#define VERSION "v$VERSION"
#define VERSION_MAJOR $(echo $VERSION | cut -d. -f1)
#define VERSION_MINOR $(echo $VERSION | cut -d. -f2)
#define VERSION_PATCH $(echo $VERSION | cut -d. -f3)

#endif /* _VERSION_H */
EOF

echo "Generated $OUTPUT_FILE with version $VERSION"
