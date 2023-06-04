import io
import struct
import os
import enum

_opcode_packer: struct.Struct = struct.Struct("<I")
_listlen_packer: struct.Struct = struct.Struct("<I")
_strlen_packer: struct.Struct = struct.Struct("<I")

def _PeekOpCode(self, _ss: io.BytesIO) -> int:
	res = _opcode_packer.unpack(_ss.read(_opcode_packer.size))[0]
	_ss.seek(-4, os.SEEK_CUR)
	return res

class _BpStruct(object):
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

class _BpMessage(_BpStruct):
	def _ReadOpCode(self, _ss: io.BytesIO):
		expect: int = self.GetOpCode()
		gotten: int = _opcode_packer.unpack(_ss.read(_opcode_packer.size))[0]
		if gotten != expect:
			raise Exception(f'Invalid opcode! Expect {expect}, but got {gotten}')
	def _WriteOpCode(self, _ss: io.BytesIO):
		_ss.write(_opcode_packer.pack(self.GetOpCode()))

	def GetIsReliable(self) -> bool:
		raise Exception("Abstract function call")
	def GetOpCode(self) -> int:
		raise Exception("Abstract function call")
