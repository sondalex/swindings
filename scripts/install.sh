#!/usr/bin/env bash
set -e
set -o pipefail

REPO="swindings"
OWNER="sondalex"
DEFAULT_BIN_DIR="$HOME/.local/bin"
BIN_DIR="$DEFAULT_BIN_DIR"
ARCH="linux-x86_64"
APP="swindings"

# Color functions
green() { echo -e "\033[0;32m$1\033[0m"; }
red() { echo -e "\033[0;31m$1\033[0m"; }

usage() {
  cat <<EOF
Usage: $0 [--version <vX.Y.Z>] [--bin-dir <dir>]
Install $APP from GitHub releases.
  --version <tag>: Specify release version (default: latest)
  --bin-dir <dir>: Install directory (default: $DEFAULT_BIN_DIR)
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
    -h|--help)
      usage;;
    *)
      usage;;
  esac
done

# Find GH release URL
API_URL="https://api.github.com/repos/$OWNER/swindings/releases"
if [ -z "$VERSION" ]; then
  VERSION=$(curl -fsSL $API_URL/latest | grep '"tag_name":' | cut -d '"' -f4)
else
  VERSION="${VERSION#v}"
  VERSION=$(curl -fsSL $API_URL | grep -F '"tag_name": "v$VERSION"' | head -1 | awk -F'"' '{print $4}')
  [ -z "$VERSION" ] && red "Version not found!" && exit 1
fi

TAR="swindings-${VERSION#v}-$ARCH.tar.gz"
URL="https://github.com/$OWNER/swindings/releases/download/v${VERSION#v}/$TAR"
URL_SHA="${URL}.sha256"

TMPDIR=$(mktemp -d)
cd "$TMPDIR"
echo "Downloading $TAR..."
curl -fsSLO "$URL"
echo "Downloading checksum..."
curl -fsSLO "$URL_SHA"
# Verify checksum (BSD format)
CHECKSUM_BSD=$(cat $TAR.sha256 | awk '{print $4}')
CALC=$(sha256sum $TAR | awk '{print $1}')
if [[ "$CALC" != "$CHECKSUM_BSD" ]]; then
  red "Checksum verification failed! Aborting."
  exit 2
fi
green "Checksum verified."

tar -xzvf $TAR
# Location inside archive: bin/swindings
[ ! -f bin/swindings ] && red "Build archive corrupt!" && exit 3
mkdir -p "$BIN_DIR"
cp bin/swindings "$BIN_DIR/"
chmod +x "$BIN_DIR/swindings"

green "$APP was installed to $BIN_DIR/swindings."

