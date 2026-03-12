#!/bin/sh
set -e

REPO="6missedcalls/calendly-cli"
INSTALL_DIR="${CALENDLY_INSTALL_DIR:-/usr/local/bin}"

# Detect OS and architecture
OS=$(uname -s | tr '[:upper:]' '[:lower:]')
ARCH=$(uname -m)

case "$OS" in
  darwin) OS="darwin" ;;
  linux)  OS="linux" ;;
  *)      echo "Unsupported OS: $OS" >&2; exit 1 ;;
esac

case "$ARCH" in
  x86_64)  ARCH="x86_64" ;;
  arm64)   ARCH="arm64" ;;
  aarch64) ARCH="arm64" ;;
  *)       echo "Unsupported architecture: $ARCH" >&2; exit 1 ;;
esac

# Get latest release tag
LATEST=$(curl -sI "https://github.com/$REPO/releases/latest" \
  | grep -i ^location: \
  | sed 's|.*/tag/||' \
  | tr -d '\r\n')

if [ -z "$LATEST" ]; then
  echo "Failed to determine latest release" >&2
  exit 1
fi

TARBALL="calendly-${OS}-${ARCH}.tar.gz"
URL="https://github.com/$REPO/releases/download/$LATEST/$TARBALL"

echo "Downloading calendly $LATEST for $OS/$ARCH..."

TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

curl -fsSL "$URL" -o "$TMPDIR/$TARBALL"
tar xzf "$TMPDIR/$TARBALL" -C "$TMPDIR"

if [ ! -w "$INSTALL_DIR" ]; then
  echo "Installing to $INSTALL_DIR (requires sudo)..."
  sudo install -m 755 "$TMPDIR/calendly/calendly" "$INSTALL_DIR/calendly"
else
  install -m 755 "$TMPDIR/calendly/calendly" "$INSTALL_DIR/calendly"
fi

echo "calendly $LATEST installed to $INSTALL_DIR/calendly"
