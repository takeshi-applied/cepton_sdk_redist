"""
Sample script for getting points from packet log.
"""

import json

import cepton_sdk
from cepton_sdk.common import *

def on_frame(serial_number, points):
    print("Received {} points from sensor {}".format(
        len(points), serial_number))

def read_json(file_name):
    with open(file_name) as f:
        s = f.read()
        return json.loads(s)

# Initialize
cepton_sdk.initialize(control_flags=cepton_sdk.core.ControlFlag.DISABLE_NETWORK)

callback_id = cepton_sdk.listen_frames(on_frame)

# cepton_sdk.mock_network_receive(1, timestamp, data, len(d))

cepton_sdk.wait(1)

cepton_sdk.unlisten_frames(callback_id)

cepton_sdk.deinitialize()

print("finish")

