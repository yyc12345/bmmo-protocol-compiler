import io
import struct
import os

"""
Message Read Example:
# Create buffer, this buffer can be recyclable use. 
# So you can define it in some global scopes, such as class member.
ss = io.BytesIO()
# Write binary array read from other sources, such as file or network stream
ss.write(blabla)
# Then, we need reset its read pointer for following parsing
ss.seek(io.SEEK_SET, 0)
# Call uniformed deserializer to generate data struct
# If this function return None, it mean that parser can not understand this binary data
# This method also can throw exceptions, usually caused by broken binary data.
your_data = uniform_deserialize(ss)
if your_data is None:
	throw Exception("Invalid OpCode.")
# Clear buffer for future using. This operation is essential, 
# especially you are recyclable using this buffer.
ss.truncate(0)
"""

"""
Message Write Example:
# Create buffer, this buffer can be recyclable use. 
# So you can define it in some global scopes, such as class member.
ss = io.BytesIO()
# Prepare your data struct, fill each fields, make sure all of theme are not None.
# The array and list located in your struct has been initialized. You do not need to init them again. Just fill the data.
your_data = ExampleMessage()
your_data.essential = 114514
your_data.essential_list[0] = "test"
# Call your data's serialize() function to get binary data.
# This method also can throw some exceptions, usually caused by that you have not fill all essential fileds, 
# or, you fill a wrong data type for some fields because Python is a type insensitive language, 
# your editor and interpreter can not check it.
your_data.serialize(ss)
# Get bianry data from buffer and send it via some stream which you want.
your_sender(ss.getvalue())
# Clear buffer for future using. This operation is essential, 
# especially you are recyclable using this buffer.
ss.truncate(0)

"""

def _peek_opcode(ss: io.BytesIO) -> int:
	res = struct.unpack('I', ss.read(4))[0]
	ss.seek(-4, os.SEEK_CUR)
	return res

def _read_bmmo_string(ss: io.BytesIO) -> str:
	_strlen = struct.unpack('I', ss.read(4))[0]
	return ss.read(_strlen).decode(encoding='gb2312', errors='ignore')

def _write_bmmo_string(ss: io.BytesIO, strl: str):
	_binstr = strl.encode(encoding='gb2312', errors='ignore')
	ss.write(struct.pack('I', len(_binstr)))
	ss.write(_binstr)
