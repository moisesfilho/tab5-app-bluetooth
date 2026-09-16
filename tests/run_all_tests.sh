#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OS_ROOT="$(cd "${SCRIPT_DIR}/../../tab5-os" && pwd)"

cd "${OS_ROOT}"
python3 -m pytest -q \
    "${SCRIPT_DIR}/test_bt_passive_rescan_contract.py" \
    "${SCRIPT_DIR}/test_ble_bridge_sync_contracts.py"
