"""
OpenHTF Test Runner
Main entry point for executing hardware-in-the-loop tests.
"""

import sys
import argparse
import logging
from pathlib import Path

import openhtf as htf
from openhtf import conf
from openhtf.util import configuration
from openhtf.output.callbacks import json_factory
from openhtf.output.callbacks import console_summary


# setup project paths
PROJECT_ROOT = Path(__file__).parent
sys.path.insert(0, str(PROJECT_ROOT))


def load_test_configuration(config_file: str):
    """Load OpenHTF Test Execution Configuration YAML file."""
    # Declare all configuration keys here that tests will use
    # These must be declared before loading
    configuration.CONF.declare('main_firmware_hex')
    configuration.CONF.declare('nightly_firmware_hex')
    
    config_path = PROJECT_ROOT / config_file
    if config_path.exists():
        with open(config_path, 'r') as f:
            configuration.CONF.load_from_file(f)
        logging.getLogger(__name__).debug(f"Loaded configuration from {config_file}")
    else:
        logging.getLogger(__name__).warning(f"Configuration file not found: {config_file}")


def setup_logging(verbose: bool = False):
    """Configure logging for test execution."""
    level = logging.DEBUG if verbose else logging.INFO
    
    # Configure standard Python logging
    logging.basicConfig(
        level=level,
        format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
        handlers=[
            logging.StreamHandler(sys.stdout),
            logging.FileHandler(PROJECT_ROOT / 'logs' / 'test_runner.log')
        ]
    )
    
    # Add file handler for OpenHTF logs (always write to file)
    file_handler = logging.FileHandler(PROJECT_ROOT / 'logs' / 'test_runner.log')
    file_handler.setLevel(logging.DEBUG)
    file_handler.setFormatter(
        logging.Formatter('%(levelname).1s %(asctime)s.%(msecs)03d %(name)s - %(message)s',
                        datefmt='%H:%M:%S')
    )
    logging.getLogger('openhtf.test_record').addHandler(file_handler)
    logging.getLogger('openhtf.test_record').setLevel(logging.DEBUG)
    
    # For verbose mode, also show OpenHTF phase/plug logs on console
    if verbose:
        console_handler = logging.StreamHandler(sys.stdout)
        console_handler.setLevel(logging.DEBUG)
        console_handler.setFormatter(
            logging.Formatter('%(levelname).4s %(asctime)s.%(msecs)03d - %(message)s',
                            datefmt='%H:%M:%S')
        )
        logging.getLogger('openhtf.test_record').addHandler(console_handler)


def discover_tests(test_dir: Path):
    """
    Discover test modules in tests/ directory.
    
    Args:
        test_dir: Path to tests directory
        
    Returns:
        List of test module names
    """
    test_modules = []
    if test_dir.exists():
        for test_file in sorted(test_dir.glob('test_*.py')):
            module_name = test_file.stem
            test_modules.append(module_name)
    return test_modules


def list_tests():
    """List all available tests."""
    test_dir = PROJECT_ROOT / 'tests'
    tests = discover_tests(test_dir)
    
    print("**** Available Functional Tests Scripts: ****")
    print("-" * 80)
    if tests:
        for i, test in enumerate(tests, 1):
            print(f"  {i}. {test}")
    else:
        print("  No tests found in tests/ directory")
    print("-" * 80)
    print(f"Total: {len(tests)} tests scripts found")


def run_test(test_name: str, config: dict):
    """
    Run a specific test script.
    
    Args:
        test_name: Name of test module (without .py)
        config: Test configuration dictionary
    """
    logger = logging.getLogger(__name__)
    logger.info(f"Running test script: {test_name}")
    
    # Import test module
    test_dir = PROJECT_ROOT / 'tests'
    sys.path.insert(0, str(test_dir))
    
    try:
        test_module = __import__(test_name)
        
        # Get the test function 
        # *note: assumes test function has same name as module
        test_func = getattr(test_module, test_name, None)
        
        if test_func is None:
            logger.error(f"Test function '{test_name}' not found")
            return False
        
        # Execute the test and capture result
        logger.info(f"Executing {test_name}...")
        test = test_func()
        test_result = test.execute(test_start=lambda: logger.info("Test started"))

        if test_result:
            logger.info(f" [OK] Test PASSED: {test_name}")
            return True
        else:
            logger.error(f" [!!] Test FAILED: {test_name}")
            return False
            
    except Exception as e:
        logger.error(f" [!!] Test execution FAILED: {e}", exc_info=True)
        return False


def run_all_tests(config: dict):
    """
    Run all discovered tests.
    
    Args:
        config: Test configuration dictionary
    """
    test_dir = PROJECT_ROOT / 'tests'
    tests = discover_tests(test_dir)
    
    logger = logging.getLogger(__name__)
    logger.info(f"Running {len(tests)} tests")
    
    passed = 0
    failed = 0
    
    for test_name in tests:
        if run_test(test_name, config):
            passed += 1
        else:
            failed += 1
    
    results = {
        'passed': passed,
        'failed': failed,
        'skipped': 0
    }
    
    # Print summary
    print("\n" + "=" * 80)
    print("Test Summary:")
    print(f"  PASSED:  {results['passed']}")
    print(f"  FAILED:  {results['failed']}")
    print(f"  SKIPPED: {results['skipped']}")
    print("=" * 80)
    
    return results['failed'] == 0


def main():
    """Main entry point for runner."""
    parser = argparse.ArgumentParser(
        description='OpenHTF Functional Test Runner for Artemis Edge',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # List all available Functional Tests
  python3 run_tests.py --list
  
  # Run all Functional Tests
  python3 run_tests.py
  
  # Run specific Functional Test
  python3 run_tests.py --test test_001_helloWorld_test
  
  # Run with verbose logging
  python3 run_tests.py --verbose
  
  # Use custom configuration
  python3 run_tests.py --config config/custom_config.yaml
        """
    )
    
    parser.add_argument(
        '--list',
        action='store_true',
        help='List all available Functional Tests'
    )
    
    parser.add_argument(
        '--test',
        type=str,
        help='Run specific Functional Test by name (e.g., test_001_helloWorld_test)'
    )
    
    parser.add_argument(
        '--config',
        type=str,
        default='config/hw_cfg.yaml',
        help='Path to configuration file (default: config/hw_cfg.yaml)'
    )
    
    parser.add_argument(
        '--verbose', '-v',
        action='store_true',
        help='Enable verbose logging'
    )
    
    parser.add_argument(
        '--report-dir',
        type=str,
        default='reports',
        help='Directory for test reports (default: reports/)'
    )

    parser.add_argument(
        '--firmware-target',
        choices=['nightly', 'main'],
        default='nightly',
        help='Which firmware target to copy before running tests (nightly/main)'
    )
    
    args = parser.parse_args()
    
    # Set up logging
    setup_logging(verbose=args.verbose)
    logger = logging.getLogger(__name__)
    
    if args.list:
        list_tests()
        return 0
    
    load_test_configuration('config/test_cfg.yaml')
    
    config = {
        'config_file': args.config,
        'report_dir': args.report_dir,
        'verbose': args.verbose
    }
    
    logger.info("/_" * 40)
    logger.info("      OpenHTF: Functional Test Runner")
    logger.info("       Hardware-in-the-Loop Testing")
    logger.info("_/" * 40)
    
# Ensure the latest built firmware is copied into the test folder (as configured)
    try:
        # Import lazily to avoid adding test dependencies for non-test users
        from framework import src_draft_get
        src_draft_get.main(['--target', args.firmware_target])
    except Exception as e:
        logger.warning("Could not copy firmware draft: %s", e)

    # Run tests
    try:
        if args.test:
            # Run a specific Functional Test
            run_test(args.test, config)
            success = True
        else:
            # Run all Functional Tests
            success = run_all_tests(config)
        
        return 0 if success else 1
    
    except KeyboardInterrupt:
        logger.warning("\n [!!] Execution Interrupted by USER !!")
        return 130
    except Exception as e:
        logger.error(f" [!!] Test runner error: {e}", exc_info=True)
        return 1

# Entry point
if __name__ == '__main__':
    sys.exit(main())
