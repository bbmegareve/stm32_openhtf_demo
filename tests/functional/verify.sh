#!/bin/bash
# Verification script for functional test framework

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

echo "##########################################################"
echo "##########################################################"
echo "     Functional Test Framework self test verification"
echo "##########################################################"
echo "##########################################################"
echo ""

######################## Step 1: ################################### 
# Check virtual environment
echo ""
echo " 1) Verifying virtual environment setup."
if [ ! -d "venv" ]; then
    echo "  [ ]: Virtual environment not found"
    echo "        -> Run ./setup.sh first"
    exit 1
fi
echo "  [x]: Virtual environment exists --> OK"

# Activate virtual environment
source venv/bin/activate

######################## Step 2: ################################### 
# Verify Python packages
echo ""
echo " 2) Verifying Python packages."
python3 -c "import openhtf" && echo "  [x]: openhtf --> OK" || echo "  [ ]: openhtf --> FAILED"
python3 -c "import serial" && echo "  [x]: pyserial --> OK" || echo "  [ ]: pyserial --> FAILED"
python3 -c "import yaml" && echo "  [x]: pyyaml --> OK" || echo "  [ ]: pyyaml --> FAILED"
python3 -c "import intelhex" && echo "  [x]: intelhex --> OK" || echo "  [ ]: intelhex --> FAILED"

######################## Step 3: ################################### 
# Verify framework modules
echo ""
echo " 3) Verifying framework modules..."
python3 -c "from plugs import STLinkPlug, SerialConsolePlug" && echo "  [x]: Hardware plugs --> OK" || echo "  [ ]: Hardware plugs --> FAILED"
python3 -c "from framework import flash_layout" && echo "  [x]: Framework utilities --> OK" || echo "  [ ]: Framework utilities --> FAILED"
python3 -c "from framework.bin2ota_wrapper import calculate_crc32" && echo "  [x]: bin2ota integration --> OK" || echo "  [ ]: bin2ota integration --> FAILED"

######################## Step 4: ################################### 
# Test runner
echo ""
echo " 4) Testing test runner."
if python3 run_tests.py --list > /dev/null 2>&1; then
    echo "  [x]: Test runner executes --> OK"
else
    echo "  [ ]: Test runner failed --> FAILED"
    exit 1
fi

######################## Step 5: ################################### 
# Run test list command
echo ""
echo " 5) Running test list command."
echo "=============================================================="
python3 run_tests.py --list

echo ""
echo "_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_"
echo "/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/___\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_"
echo "                Framework verification complete!"
echo "/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_______\_\_\_\_\_\_\_\_\_\_\_\_\_\_"
echo "_/_/_/_/_/_/_/_/_/_/_/_/_/_/_________\_\_\_\_\_\_\_\_\_\_\_\_\_\_"
echo ""
echo ""
echo "Next steps:"
echo "  1. Implement test scripts in tests/functional/tests/ or just test demo script"
echo "  2. Connect hardware (STM32 board, ST-Link, serial console)"
echo "  3. Configure config/hardware_config.yaml for your setup"
echo "  4. Run tests: python3 run_tests.py"
echo ""
