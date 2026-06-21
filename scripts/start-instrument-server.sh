#!/usr/bin/env bash
set -euo pipefail

INSTRUMENT_SERVER_BIN="${INSTRUMENT_SERVER_BIN:-/instrument-server/build/instrument_server}"
export DATABASE_URL="${DATABASE_URL:-postgresql://instruments:instruments@127.0.0.1:5432/instruments}"
export GRPC_LISTEN_ADDRESS="${GRPC_LISTEN_ADDRESS:-0.0.0.0:50051}"

exec "$INSTRUMENT_SERVER_BIN"
