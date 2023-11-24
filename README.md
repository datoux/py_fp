# py_fp
Wrapper around Front Panel library to communicate with Opal Kelly Front Panel devices.

## list of py_fp functions:
- py_fp.list_devices() - returns list of connected devices

## list of FPDevice functions:
- `list_devices()`
- `open(serial, firmware_file)`
- `close()`
- `set_wire_in(address, value, send_now)`
- `get_wire_out(address, refresh_wire)`
- `write_register(address, value)`
- `read_register(address)`
- `write_pipe(address, [data_bytes], block_size=1024)`
- `read_pipe(address, [data_bytes], block_size=1024)`
- `set_timeout(timeout)`
- `set_device_id(device_id)`
- `get_device_id()`
- `log(log_level, text, no_time)`


## Example Usage
```python
import py_fp

devices = py_fp.list_devices()
# []

device = py_fp.FPDevice()

# open(serial, firmware)
rc = device.open("12345", "firmware.rbf")
# rc = 0


rc = device.close()
# rc = 0
```

## Installation
1. Clone the repository localy

```bash
 python setup.py build
```

2. copy the build python package to your project dir
3. copy the corresponding okFrontPanel lib to the same dir
