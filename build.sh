#!/bin/bash

# S2S Geospatial Adapter - Build Script
# Usage: ./build.sh [clean|release|debug|test]

set -e

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${PROJECT_DIR}/build"
BUILD_TYPE="Release"

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "S2S Geospatial Adapter - Build System"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

# Parse arguments
case "${1:-release}" in
    clean)
        echo "🧹 Cleaning build directory..."
        rm -rf "${BUILD_DIR}"
        echo "✅ Clean complete"
        exit 0
        ;;
    debug)
        BUILD_TYPE="Debug"
        echo "🔧 Debug build selected"
        ;;
    release)
        BUILD_TYPE="Release"
        echo "🚀 Release build selected"
        ;;
    test)
        BUILD_TYPE="Release"
        echo "🧪 Test build selected"
        ;;
    *)
        echo "❌ Unknown argument: $1"
        echo "Usage: ./build.sh [clean|release|debug|test]"
        exit 1
        ;;
esac

# Create build directory
if [ ! -d "${BUILD_DIR}" ]; then
    echo "📁 Creating build directory..."
    mkdir -p "${BUILD_DIR}"
fi

cd "${BUILD_DIR}"

# Install dependencies with Conan
echo ""
echo "📦 Installing dependencies with Conan..."
if command -v conan &> /dev/null; then
    conan install .. --build=missing -s build_type="${BUILD_TYPE}"
    echo "✅ Conan dependencies installed"
else
    echo "⚠️  Conan not found. Skipping dependency installation."
    echo "   Install with: brew install conan (macOS) or apt install conan (Linux)"
fi

# Configure with CMake
echo ""
echo "⚙️  Configuring with CMake..."
cmake -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" ..
echo "✅ CMake configuration complete"

# Build
echo ""
echo "🔨 Building..."
NUM_JOBS=$(nproc 2>/dev/null || echo 4)
cmake --build . -j"${NUM_JOBS}"
echo "✅ Build complete"

# Show build artifacts
echo ""
echo "📂 Build artifacts:"
ls -lh bin/ 2>/dev/null || echo "   (executable targets not found)"

# Run tests if requested
if [ "$1" = "test" ]; then
    echo ""
    echo "🧪 Running tests..."
    ctest --output-on-failure
    echo "✅ Tests complete"
fi

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "✨ Build successful!"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "📖 Next steps:"
echo "   1. Run daemon:  ./build/bin/s2sgeo_daemon"
echo "   2. Run adapter: ./build/bin/s2sgeo_adapter  (in another terminal)"
echo "   3. See docs:    cat docs/QUICKSTART.md"
echo ""
