import io
import struct
import os

def _create_buffer() -> io.BytesIO:
	return io.BytesIO()

def _get_buffer(ss: io.BytesIO) -> bytes:
	return ss.getvalue()

def _reset_buffer(ss: io.BytesIO):
	ss.seek(io.SEEK_SET, 0)

def _clear_buffer(ss: io.BytesIO):
	ss.truncate(0)

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
