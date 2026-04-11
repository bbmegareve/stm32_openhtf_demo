"""
STLinkPlug - STM32CubeProgrammer CLI Wrapper

Provides hardware abstraction for flashing STM32 MCU memory,
reading flash contents, and device reset operations.
"""

import subprocess
import logging
import os
import yaml
from pathlib import Path
from typing import Optional, Dict, Any

import openhtf as htf
from openhtf.plugs import BasePlug


def _load_hardware_config():
    """ 
    Helper function to load hardware configuration
    from YAML file.
    """
    config_path = Path(__file__).parent.parent / 'config' / 'hw_cfg.yaml'
    with open(config_path, 'r') as f:
        return yaml.safe_load(f)


class STLinkPlug(BasePlug):
    """Plug for interfacing with ST-Link programmer."""
    
    def __init__(self, device_serial: Optional[str] = None, programmer_cli: Optional[str] = None):
        """
        Initialize STLink plug.
        
        Args:
            device_serial: Optional ST-Link serial number for multi-device setups (None = auto-detect)
            programmer_cli: Path or command for STM32CubeProgrammer CLI (None = load from config)
        """
        super(STLinkPlug, self).__init__()
        
        if programmer_cli is None or device_serial is None:
            config = _load_hardware_config()
            self.device_serial = device_serial if device_serial is not None else config['stlink']['serial_number']
            self.programmer_cli = programmer_cli if programmer_cli is not None else config['stlink']['programmer_cli']
        else:
            self.device_serial = device_serial
            self.programmer_cli = programmer_cli
            
    def tearDown(self):
        """Cleanup when plug is torn down."""
        self.logger.info("STM32 MCU tearDown")
        
    def _run_command(self, args: list) -> tuple[int, str, str]:
        """
        Run STM32CubeProgrammer CLI cmd.
        
        Args:
            args: Command arguments list
            
        Returns:
            Tuple of (return_code, stdout, stderr)
        """
        cmd = [self.programmer_cli] + args
        self.logger.debug(f"Running command: {' '.join(cmd)}")
        
        try:
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=60
            )
            
            # Log output
            if result.stdout:
                self.logger.info(f"STM32CubeProgrammer output:\n{result.stdout}")
            if result.stderr and result.stderr.strip():
                self.logger.warning(f"STM32CubeProgrammer stderr:\n{result.stderr}")
                
            return result.returncode, result.stdout, result.stderr
        except subprocess.TimeoutExpired:
            self.logger.error("Command timeout")
            return -1, "", "Command timeout"
        except Exception as e:
            self.logger.error(f"STM32 MCU command failed: {e}")
            return -1, "", str(e)
    
    def connect(self) -> bool:
        """
        Connect to ST-Link programmer:
        STM32_Programmer_CLI -c port=SWD sn=<serial> mode=UR
        
        Returns:
            True if connection successful
        """
        args = ["-c", "port=SWD"]
        if self.device_serial:
            args.append(f"sn={self.device_serial}")
        args.append("mode=UR")  # Under reset
        
        ret, stdout, stderr = self._run_command(args)
        if ret == 0:
            self.logger.info("STM32 MCU connected to ST-Link : OK")
            return True
        else:
            self.logger.error(f"STM32 MCU failed to connect: {stderr}")
            return False
    
    def disconnect(self) -> bool:
        """
        Disconnect from ST-Link programmer:
        STM32_Programmer_CLI -c port=SWD -rst
        
        Returns:
            True if disconnection successful
        """
        ret, stdout, stderr = self._run_command(["-c", "port=SWD", "-rst"])
        if ret == 0:
            self.logger.info("STM32 MCU disconnected from ST-Link : OK")
            return True
        else:
            self.logger.warning(f"STM32 MCU disconnect: {stderr}")
            return False
    
    def program_firmware(self, binary_path: str, address: int = 0x08000000) -> bool:
        """
        Program firmware binary to STM32 flash.
        STM32_Programmer_CLI -c port=SWD mode=UR -w <binary> <address> -v -rst
        
        Args:
            binary_path: Path to binary file
            address: Flash address (default: 0x08000000)
            
        Returns:
            True if programming successful
        """
        if not os.path.exists(binary_path):
            self.logger.error(f"STM32 MCU binary file not found: {binary_path}")
            return False
        
        args = [
            "-c", "port=SWD", "mode=UR",
            "-w", binary_path, f"{hex(address)}",
            "-v",  # Verify after programming
            "-rst"  # Reset after programming
        ]
        
        if self.device_serial:
            args.insert(2, f"sn={self.device_serial}")
        
        ret, stdout, stderr = self._run_command(args)
        if ret == 0 and "Download verified successfully" in stdout:
            self.logger.info(f"STM32 MCU programmed {binary_path} to {hex(address)} : OK")
            return True
        else:
            self.logger.error(f"STM32 MCU programming failed: {stderr}")
            return False
    
    def read_flash(self, address: int, size: int) -> Optional[bytes]:
        """
        Read flash memory region.
        STM32_Programmer_CLI -c port=SWD -r <address> <size> <file>
        
        Args:
            address: Flash address to read from
            size: Number of bytes to read
            
        Returns:
            Bytes read from flash, or None on error
        """
        # Create temporary file for reading
        temp_file = f"/tmp/stlink_read_{address:08x}.bin"
        
        args = [
            "-c", "port=SWD",
            "-r", hex(address), hex(size), temp_file
        ]
        
        if self.device_serial:
            args.insert(2, f"sn={self.device_serial}")
        
        ret, stdout, stderr = self._run_command(args)
        if ret == 0 and os.path.exists(temp_file):
            with open(temp_file, 'rb') as f:
                data = f.read()
            os.remove(temp_file)
            self.logger.info(f"STM32 MCU read {len(data)} bytes from {hex(address)} : OK")
            return data
        else:
            self.logger.error(f"STM32 MCU read failed: {stderr}")
            return None
    
    def write_flash(self, address: int, data: bytes) -> bool:
        """
        Write data to flash memory:
        STM32_Programmer_CLI -c port=SWD mode=UR -w <binary> <address> -v -rst
        
        Args:
            address: Flash address to write to
            data: Bytes to write
            
        Returns:
            True if write successful
        """
        # Create temporary binary file
        temp_file = f"/tmp/stlink_write_{address:08x}.bin"
        
        try:
            with open(temp_file, 'wb') as f:
                f.write(data)
            
            success = self.program_firmware(temp_file, address)
            os.remove(temp_file)
            return success
        except Exception as e:
            self.logger.error(f"STM32 MCU write flash failed: {e}")
            return False
    
    def reset_device(self, mode: str = "hw") -> bool:
        """
        Reset STM32 device:
            STM32_Programmer_CLI -c port=SWD -rst
        
        Args:
            mode: Reset mode - "hw" (hardware) or "sw" (software)
            
        Returns:
            True if reset successful
        """
        args = ["-c", "port=SWD"]
        if self.device_serial:
            args.append(f"sn={self.device_serial}")
        args.append("-rst")
        
        ret, stdout, stderr = self._run_command(args)
        if ret == 0:
            self.logger.info(f"STM32 MCU reset ({mode}) : OK")
            return True
        else:
            self.logger.error(f"STM32 MCU reset failed: {stderr}")
            return False
    
    def erase_flash(self, start_address: Optional[int] = None, end_address: Optional[int] = None) -> bool:
        """
        Erase flash memory region.
            STM32_Programmer_CLI -c port=SWD mode=UR -e all
            STM32_Programmer_CLI -c port=SWD mode=UR -e <start> <end>
        
        Args:
            start_address: Start address (None = full chip erase)
            end_address: End address (only with start_address)
            
        Returns:
            True if erase successful
        """
        args = ["-c", "port=SWD", "mode=UR"]
        if self.device_serial:
            args.append(f"sn={self.device_serial}")
        
        if start_address is None:
            # Full chip erase
            args.append("-e")
            args.append("all")
        else:
            # Sector erase
            args.append("-e")
            args.append(f"{hex(start_address)} {hex(end_address)}")
        
        ret, stdout, stderr = self._run_command(args)
        if ret == 0:
            self.logger.info("STM32 MCU flash erase: OK")
            return True
        else:
            self.logger.error(f"STM32 MCU erase failed: {stderr}")
            return False
    
    def get_device_info(self) -> Optional[Dict[str, Any]]:
        """
        Get device information:
        STM32_Programmer_CLI -c port=SWD sn=<serial> mode=UR
        
        Returns:
            Dictionary with device info, or None on error
        """
        args = ["-c", "port=SWD"]
        if self.device_serial:
            args.append(f"sn={self.device_serial}")
        
        ret, stdout, stderr = self._run_command(args)
        if ret == 0:
            # Parse device info from stdout
            info = {
                'connected': True,
                'raw_output': stdout
            }
            # TODO: Parse specific fields:
            # [] device ID 
            # [] flash size.
            return info
        else:
            self.logger.error(f"Failed to get STM32 uC info: {stderr}")
            return None
