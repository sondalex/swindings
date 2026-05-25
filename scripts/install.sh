#!/usr/bin/env bash
set -e
set -o pipefail

REPO="swindings"
OWNER="sondalex"
DEFAULT_BIN_DIR="$HOME/.local/bin"
BIN_DIR="$DEFAULT_BIN_DIR"
DEFAULT_SHARE_DIR="$HOME/.local/share"
SHARE_DIR="$DEFAULT_SHARE_DIR"
ARCH="linux-x86_64"
APP="swindings"

# Color / progress functions
green() { echo -e "\033[0;32m$1\033[0m"; }
red()   { echo -e "\033[0;31m$1\033[0m"; }
dim()   { echo -e "\033[2m$1\033[0m"; }

TOTAL_STEPS=6
CURRENT_STEP=0
step() {
  CURRENT_STEP=$((CURRENT_STEP + 1))
  echo -e "\033[1m[$CURRENT_STEP/$TOTAL_STEPS]\033[0m $1"
}

# Use --progress-bar only when stdout is a terminal
curl_download() {
  if [ -t 1 ]; then
    curl -fL --progress-bar "$@"
  else
    curl -fsSL "$@"
  fi
}

usage() {
  cat <<EOF
Usage: $0 [--version <vX.Y.Z>] [--bin-dir <dir>] [--share-dir <dir>]
Install $APP from GitHub releases.
  --version <tag>:   Specify release version (default: latest)
  --bin-dir <dir>:   Install binary directory (default: $DEFAULT_BIN_DIR)
  --share-dir <dir>: Install share directory (default: $DEFAULT_SHARE_DIR)
EOF
  exit 1
}

# Parse options
VERSION=""
while [[ $# -gt 0 ]]; do
  case "$1" in
    --version)
      VERSION="$2"; shift; shift;;
    --bin-dir)
      BIN_DIR="$2"; shift; shift;;
    --share-dir)
      SHARE_DIR="$2"; shift; shift;;
    -h|--help)
      usage;;
    *)
      usage;;
  esac
done

# Find GH release URL
API_URL="https://api.github.com/repos/$OWNER/swindings/releases"
step "Resolving version..."
if [ -z "$VERSION" ]; then
  VERSION=$(curl -fsSL $API_URL/latest | grep '"tag_name":' | cut -d '"' -f4)
else
  VERSION="${VERSION#v}"
  VERSION=$(curl -fsSL $API_URL | grep -F '"tag_name": "v$VERSION"' | head -1 | awk -F'"' '{print $4}')
  [ -z "$VERSION" ] && red "Version not found!" && exit 1
fi
dim "  -> $VERSION"

TAR="swindings-${VERSION#v}-$ARCH.tar.gz"
URL="https://github.com/$OWNER/swindings/releases/download/v${VERSION#v}/$TAR"
URL_SHA="${URL}.sha256"

TMPDIR=$(mktemp -d)
cd "$TMPDIR"
step "Downloading archive..."
curl_download -O "$URL"
step "Downloading checksum..."
curl_download -O "$URL_SHA"
step "Verifying checksum..."
# Verify checksum (BSD format)
CHECKSUM_BSD=$(cat $TAR.sha256 | awk '{print $4}')
CALC=$(sha256sum $TAR | awk '{print $1}')
if [[ "$CALC" != "$CHECKSUM_BSD" ]]; then
  red "Checksum verification failed! Aborting."
  exit 2
fi
green "  Checksum OK."

step "Extracting archive..."
tar -xzf $TAR
# Location inside archive: bin/swindings
[ ! -f bin/swindings ] && red "Build archive corrupt!" && exit 3
step "Installing files..."
mkdir -p "$BIN_DIR"
cp bin/swindings "$BIN_DIR/"
chmod +x "$BIN_DIR/swindings"

[ ! -d share ] && red "Build archive corrupt (missing share/)!" && exit 3
mkdir -p "$SHARE_DIR"
cp -r share/. "$SHARE_DIR/"

green "$APP installed to $BIN_DIR/swindings."
green "Shared data installed to $SHARE_DIR."

