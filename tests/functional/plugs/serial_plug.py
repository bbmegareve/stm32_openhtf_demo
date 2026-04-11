"""
SerialConsolePlug - Serial UART Monitoring for STM32 Firmware

Provides hardware abstraction for monitoring serial console output,
capturing logs, and searching for patterns in firmware output.
"""

import serial
import threading
import logging
import time
import re
import yaml
from pathlib import Path
from typing import Optional, List
from collections import deque

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


class SerialConsolePlug(BasePlug):
    """Plug for monitoring STM32 serial console output."""
    
    def __init__(self, port: Optional[str] = None, baudrate: Optional[int] = None, 
                 timeout: Optional[float] = None, max_log_lines: int = 10000):
        """
        Initialize Serial Console plug.
        
        Args:
            port: Serial port device path (None = load from config)
            baudrate: Serial baudrate (None = load from config)
            timeout: Read timeout in seconds (None = load from config)
            max_log_lines: Maximum lines to keep in log buffer
        """
        super(SerialConsolePlug, self).__init__()
        
        # Load hardware config if parameters not provided
        if port is None or baudrate is None or timeout is None:
            config = _load_hardware_config()
            self.port = port if port is not None else config['serial']['port']
            self.baudrate = baudrate if baudrate is not None else config['serial']['baudrate']
            self.timeout = timeout if timeout is not None else config['serial']['timeout']
        else:
            self.port = port
            self.baudrate = baudrate
            self.timeout = timeout
            
        self.max_log_lines = max_log_lines
        
        self.serial_conn: Optional[serial.Serial] = None
        self.log_buffer: deque = deque(maxlen=max_log_lines)
        self.reader_thread: Optional[threading.Thread] = None
        self.running = False
        
    def setUp(self):
        """Set up serial connection and start reader thread."""
        try:
            self.serial_conn = serial.Serial(
                port=self.port,
                baudrate=self.baudrate,
                timeout=self.timeout,
                bytesize=serial.EIGHTBITS,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE
            )
            
            # Clear any existing data
            self.serial_conn.reset_input_buffer()
            self.serial_conn.reset_output_buffer()
            
            # Start reader thread
            self.running = True
            self.reader_thread = threading.Thread(target=self._reader_loop, daemon=True)
            self.reader_thread.start()
            
            self.logger.info(f"UART CLI connected: {self.port} @ {self.baudrate} : OK")
            
        except serial.SerialException as e:
            self.logger.error(f"UART CLI failed to open port {self.port}: {e}")
            raise
    
    def tearDown(self):
        """Stop reader thread and close serial connection."""
        self.running = False
        
        if self.reader_thread:
            self.reader_thread.join(timeout=2.0)
        
        if self.serial_conn and self.serial_conn.is_open:
            self.serial_conn.close()
            self.logger.info("UART CLI disconnected : OK")
    
    def _reader_loop(self):
        """Background thread for reading serial data."""
        while self.running and self.serial_conn and self.serial_conn.is_open:
            try:
                if self.serial_conn.in_waiting > 0:
                    line = self.serial_conn.readline()
                    try:
                        decoded = line.decode('utf-8', errors='replace').rstrip()
                        if decoded:
                            self.log_buffer.append(decoded)
                            self.logger.debug(f"UART CLI RX: {decoded}")
                    except Exception as e:
                        self.logger.warning(f"UART CLI decode error: {e}")
                else:
                    time.sleep(0.01)  # Small delay if no data
            except serial.SerialException as e:
                self.logger.error(f"UART CLI read error: {e}")
                break
            except Exception as e:
                self.logger.error(f"Unexpected error in UART CLI reader loop: {e}")
                break
    
    def wait_for_pattern(self, pattern: str, timeout: float = 10.0, 
                        is_regex: bool = True) -> bool:
        """
        Wait for specific pattern in serial output.
        
        Args:
            pattern: Pattern to search for (string or regex)
            timeout: Maximum time to wait in seconds
            is_regex: If True, treat pattern as regex, else literal string
            
        Returns:
            True if pattern found, False if timeout
        """
        start_time = time.time()
        compiled_pattern = re.compile(pattern) if is_regex else None
        
        # Track which lines we've already checked
        last_checked_index = len(self.log_buffer)
        
        while (time.time() - start_time) < timeout:
            # Check new lines that arrived since last check
            current_size = len(self.log_buffer)
            
            for i in range(last_checked_index, current_size):
                line = self.log_buffer[i]
                
                if is_regex:
                    if compiled_pattern.search(line):
                        self.logger.info(f"Pattern '{pattern}' found: {line}")
                        return True
                else:
                    if pattern in line:
                        self.logger.info(f"String '{pattern}' found: {line}")
                        return True
            
            last_checked_index = current_size
            time.sleep(0.05)  # Small delay between checks
        
        self.logger.warning(f"Pattern '{pattern}' not found within {timeout}s")
        return False
    
    def wait_for_patterns(self, patterns: List[str], timeout: float = 10.0,
                         match_all: bool = True) -> bool:
        """
        Wait for multiple patterns in UART CLI output.
        
        Args:
            patterns: List of patterns to search for
            timeout: Maximum time to wait in seconds
            match_all: If True, all patterns must match; if False, any pattern
            
        Returns:
            True if condition met (all/any patterns found), False if timeout
        """
        start_time = time.time()
        found_patterns = set()
        
        while (time.time() - start_time) < timeout:
            for pattern in patterns:
                if pattern not in found_patterns:
                    # Check if pattern exists in recent logs
                    for line in list(self.log_buffer)[-100:]:  # Check last 100 lines
                        if re.search(pattern, line):
                            found_patterns.add(pattern)
                            self.logger.info(f" UART CLI Pattern '{pattern}' found")
                            break
            
            if match_all:
                if len(found_patterns) == len(patterns):
                    return True
            else:
                if len(found_patterns) > 0:
                    return True
            
            time.sleep(0.05)
        
        missing = set(patterns) - found_patterns
        self.logger.warning(f"UART CLI Timeout waiting for patterns. Missing: {missing}")
        return False
    
    def get_logs(self, last_n: Optional[int] = None) -> List[str]:
        """
        Get captured UART CLI logs.
        
        Args:
            last_n: Number of recent lines to return (None = all)
            
        Returns:
            List of log lines
        """
        if last_n is None:
            return list(self.log_buffer)
        else:
            return list(self.log_buffer)[-last_n:]
    
    def clear_logs(self):
        """Clear UART CLI log buffer."""
        self.log_buffer.clear()
        self.logger.info("UART CLI log buffer cleared")
    
    def send_command(self, command: str):
        """
        Send command to UART CLI.
        
        Args:
            command: Command string to send (will append newline)
        """
        if self.serial_conn and self.serial_conn.is_open:
            cmd_bytes = (command + '\n').encode('utf-8')
            self.serial_conn.write(cmd_bytes)
            self.serial_conn.flush()
            self.logger.info(f"UART CLI TX: {command}")
        else:
            self.logger.error("UART CLI port not open, cannot send command")
    
    def search_logs(self, pattern: str, is_regex: bool = True) -> List[str]:
        """
        Search existing logs for pattern.
        
        Args:
            pattern: Pattern to search for
            is_regex: If True, treat as regex, else literal string
            
        Returns:
            List of matching log lines
        """
        matches = []
        compiled_pattern = re.compile(pattern) if is_regex else None
        
        for line in self.log_buffer:
            if is_regex:
                if compiled_pattern.search(line):
                    matches.append(line)
            else:
                if pattern in line:
                    matches.append(line)
        
        return matches
    
    def get_log_count(self) -> int:
        """
        Get number of lines in log buffer.
        
        Returns:
            Number of log lines
        """
        return len(self.log_buffer)
