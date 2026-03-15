#!/bin/bash
# Setup script for functional test framework

set -e

echo "##########################################################"
echo "##########################################################"
echo "              OpenHTF Setup"
echo "##########################################################"
echo "##########################################################"
echo ""

# Get script directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

######################## Step 1: ################################### 
# Check Python version
echo " 1) Checking installed Python 3 version."
python3 --version || { echo "ERROR: Python 3 not found"; exit 1; }
echo "   [x] Python 3 found"
echo ""

######################## Step 2: ################################### 
# Check if virtual environment exists
if [ ! -d "venv" ]; then
    echo " 2) Creating python virtual environment."
    python3 -m venv venv
    echo "   [x] Virtual environment created"
else
    echo "   [x] Virtual environment already exists"
fi
echo ""

######################## Step 3: ################################### 
# Activate virtual environment
echo " 3) Activating python virtual environment."
source venv/bin/activate

# Verify activation
if [ "$VIRTUAL_ENV" != "" ]; then
    echo "   [x] Virtual environment activated: $VIRTUAL_ENV"
else
    echo "   ERROR: Failed to activate virtual environment"
    exit 1
fi
echo ""

######################## Step 4: ################################### 
# Upgrade pip
echo " 4) Upgrading pip."
pip install --upgrade pip --quiet
echo "   [x] pip upgraded"
echo ""

######################## Step 5: ################################### 
# Install dependencies
echo " 5) Installing Python dependencies from requirements.txt."
pip install -r requirements.txt
echo "   [x] Dependencies installed"
echo ""

######################## Step 6: ################################### 
# Verify key dependencies
echo " 6) Verifying installed packages."
python3 -c "import openhtf; print('   [x] openhtf:', openhtf.__version__)" || echo "   [ ] openhtf not found"
python3 -c "import serial; print('   [x] pyserial:', serial.__version__)" || echo "   [ ] pyserial not found"
python3 -c "import yaml; print('   [x] pyyaml installed')" || echo "   [ ] pyyaml not found"
echo ""

########################## Step 7: ###################################
# Create required directories
echo " 7) Creating required directories for test logs and reports."
mkdir -p logs reports
echo "   [x] Created logs/ and reports/ directories"
echo ""

########################## Step 8: ###################################
# Check STM32CubeProgrammer
echo " 8) Checking STM32CubeProgrammer CLI."
if command -v STM32_Programmer_CLI &> /dev/null; then
    echo "   [x] STM32CubeProgrammer CLI found"
    STM32_Programmer_CLI --version 2>&1 | head -n 1 || true
else
    echo "  Warning: STM32CubeProgrammer CLI not found in PATH"
    echo "  Download from: https://www.st.com/en/development-tools/stm32cubeprog.html"
    echo "  After installation, ensure 'STM32_Programmer_CLI' is in your PATH"
fi
echo ""


########################## Step 9: ###################################
# Test framework
echo " 9) Testing OpenHTF skeleton infrastructure."
if python3 run_tests.py --list > /dev/null 2>&1; then
    echo "   [x] Test runner executes successfully"
    python3 run_tests.py --list
else
    echo "   [ ] Test runner skeleton ready (no tests implemented yet)"
    python3 run_tests.py --list 2>&1 || true
fi
echo ""

echo "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"
echo "     Setup complete!"
echo "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"
echo ""
echo "Virtual environment is active"
echo ""
echo "  # List available tests"
echo "  python3 run_hil_tests.py --list"
echo ""
echo "  # Run all tests"
echo "  python3 run_hil_tests.py"
echo ""
echo "  # Run specific test"
echo "  python3 run_hil_tests.py --test t001_my_hello_world_test"
echo ""
echo "To activate virtual environment:"
echo "  cd $SCRIPT_DIR"
echo "  source venv/bin/activate"
echo ""
echo "To deactivate virtual environment:"
echo "  deactivate"
echo ""
