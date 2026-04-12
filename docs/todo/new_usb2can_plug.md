# USB2CAN plug:

## overrall
Create a new OpenHTF plug in plugs/can_plug.py.
Wrap python-can to open a CAN interface using channel and bustype from hw_cfg.yaml.
Implement: setUp, tearDown, send_frame(arbitration_id, data),
wait_for_frame(arbitration_id, timeout).



## Use cases:
 - send a UDS CAN frame to request sensor data
 - wait for a CAN frame response with the sensor data
 - responses can be UDS multiframe, so need to handle that (python-can does this for us)
 - if multiple frames are received, need to filter by arbitration_id and timestamp to find the right one


## Equipment
Device to support:  USB2CAN
https://www.8devices.com/products/korlan

```
Specifications:
CPU	ARM 32bit Cortex-M0 (STM32F072)
Electrical isolation	Galvanic isolation 2.5kV
Power source	USB
Interface speed	USB 2.0 Full speed (12 Mbps)
CAN specifications compliance	2.0A (11-bit ID) and 2.0B (29-bit ID)
CAN bus baud rate	20-2000 Kbit/s user definable speed
Firmware upgrade	Via bootloader, USB DFU protocol
Simultaneous use quantity	Up to 4 USB2CAN converters can be connected for simultaneous use on a single PC
CAN bus interface connector	SUB-DB9/ OBD2/ (option to add different connector)
LED status indications	Power, Error, Info
Available drivers	Drivers for Windows XP, Windows Vista, Windows 7, Windows 10, Linux (SocketCAN)
Supported library	Open source CANAL API DLL for Windows
3rd party protocol support	Open source CANAL API DLL for Windows
Testing modes	Silent and loopback
Available certifications	CE (RED), FCC, IC (Comming soon)

```



## python-can:
https://python-can.readthedocs.io/en/stable/api.html

```
# import the library
import can

# create a bus instance using 'with' statement,
# this will cause bus.shutdown() to be called on the block exit;
# many other interfaces are supported as well (see documentation)
with can.Bus(interface='socketcan',
              channel='vcan0',
              receive_own_messages=True) as bus:

   # send a message
   message = can.Message(arbitration_id=123, is_extended_id=True,
                         data=[0x11, 0x22, 0x33])
   bus.send(message, timeout=0.2)

   # iterate over received messages
   for msg in bus:
       print(f"{msg.arbitration_id:X}: {msg.data}")

   # or use an asynchronous notifier
   notifier = can.Notifier(bus, [can.Logger("recorded.log"), can.Printer()])
```