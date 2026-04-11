"""Hardware abstraction plugs for OpenHTF functional testing framework."""

from .stlink_plug import STLinkPlug
from .serial_plug import SerialConsolePlug

__all__ = ['STLinkPlug', 'SerialConsolePlug']
