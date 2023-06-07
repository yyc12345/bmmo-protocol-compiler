import io
import struct
import os
import enum
import typing

_opcode_packer: struct.Struct = struct.Struct("<I")
_listlen_packer: struct.Struct = struct.Struct("<I")
_strlen_packer: struct.Struct = struct.Struct("<I")

class BpStruct(object):
	def _ReadBpString(self, _ss: io.BytesIO) -> str:
		(_strlen, ) = _strlen_packer.unpack(_ss.read(_strlen_packer.size))
		return _ss.read(_strlen).decode(encoding='utf-8', errors='ignore')
	def _WriteBpString(self, _ss: io.BytesIO, strl: str):
		_binstr = strl.encode(encoding='utf-8', errors='ignore')
		_ss.write(_strlen_packer.pack(len(_binstr)))
		_ss.write(_binstr)

	def Serialize(self, _ss: io.BytesIO):
		raise Exception("Abstract function call")
	def Deserialize(self, _ss: io.BytesIO):
		raise Exception("Abstract function call")

class BpMessage(BpStruct):
	def IsReliable(self) -> bool:
		raise Exception("Abstract function call")
	def GetOpCode(self) -> int:
		raise Exception("Abstract function call")

def UniformSerialize(instance: BpMessage, ss: io.BytesIO):
	ss.write(_opcode_packer.pack(instance.GetOpCode()))
	instance.Serialize(ss)
