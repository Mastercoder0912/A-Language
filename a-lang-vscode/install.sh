#!/bin/bash

# Installation script for A Language VS Code extension

EXT_DIR="$HOME/.vscode/extensions/a-language-0.2.0"

echo "Installing A Language VS Code extension..."
echo "Target directory: $EXT_DIR"

# Create extensions directory if it doesn't exist
mkdir -p "$HOME/.vscode/extensions"

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Copy a fresh extension folder so old files do not linger between installs.
rm -rf "$EXT_DIR"
cp -R "$SCRIPT_DIR" "$EXT_DIR"

if [ $? -eq 0 ]; then
    echo "✓ Extension installed successfully!"
    echo ""
    echo "Next steps:"
    echo "1. Restart VS Code"
    echo "2. Create or open a file with .a extension"
    echo "3. You should see syntax highlighting"
    echo ""
    echo "To verify:"
    echo "  ls -la $EXT_DIR"
else
    echo "✗ Installation failed. Check permissions and try again."
    exit 1
fi
