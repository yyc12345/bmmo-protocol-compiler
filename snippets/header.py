import io
import struct
import os
import enum

_opcode_packer = struct.Struct("<I")
_listlen_packer = struct.Struct("<I")
_strlen_packer = struct.Struct("<I")

class _BpStruct(object):
	def Serialize(self, _ss: io.BytesIO):
		raise Exception("Abstract function call")
	def Deserialize(self, _ss: io.BytesIO):
		raise Exception("Abstract function call")
	def _read_bp_string(self, _ss: io.BytesIO) -> str:
		(_strlen, ) = _strlen_packer.unpack(_ss.read(_strlen_packer.size))
		return ss.read(_strlen).decode(encoding='utf-8', errors='ignore')
	def _write_bp_string(self, _ss: io.BytesIO, strl: str):
		_binstr = strl.encode(encoding='utf-8', errors='ignore')
		_ss.write(_strlen_packer.pack(len(_binstr)))
		_ss.write(_binstr)

class _BpMessage(_BpStruct):
	def GetIsReliable(self) -> bool:
		raise Exception("Abstract function call")
	def GetOpcode(self) -> int:
		raise Exception("Abstract function call")
