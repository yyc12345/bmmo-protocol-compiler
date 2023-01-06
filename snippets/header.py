import io
import struct
import os
import enum

_opcode_packer = struct.Struct("<I")
_listlen_packer = struct.Struct("<I")
_strlen_packer = struct.Struct("<I")

def _PeekOpCode(_ss: io.BytesIO):
	res = _opcode_packer.unpack(_ss.read(_opcode_packer.size))[0]
	_ss.seek(-4, os.SEEK_CUR)
	return res

class _BpStruct(object):
	def Serialize(self, _ss: io.BytesIO):
		raise Exception("Abstract function call")
	def Deserialize(self, _ss: io.BytesIO):
		raise Exception("Abstract function call")
	def _ReadBpString(self, _ss: io.BytesIO) -> str:
		(_strlen, ) = _strlen_packer.unpack(_ss.read(_strlen_packer.size))
		return _ss.read(_strlen).decode(encoding='utf-8', errors='ignore')
	def _WriteBpString(self, _ss: io.BytesIO, strl: str):
		_binstr = strl.encode(encoding='utf-8', errors='ignore')
		_ss.write(_strlen_packer.pack(len(_binstr)))
		_ss.write(_binstr)

class _BpMessage(_BpStruct):
	def GetIsReliable(self) -> bool:
		raise Exception("Abstract function call")
	def GetOpCode(self) -> int:
		raise Exception("Abstract function call")
