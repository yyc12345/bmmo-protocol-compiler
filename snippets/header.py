import io
import struct
import os

def _peek_opcode(ss: io.BytesIO):
	res = struct.unpack('I', ss.read(4))[0]
	ss.seek(-4, os.SEEK_CUR)
	return res
