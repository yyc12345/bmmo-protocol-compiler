
_opcode_packer: _pStructStruct = _pStructStruct("<I")
_listlen_packer: _pStructStruct = _pStructStruct("<I")
_strlen_packer: _pStructStruct = _pStructStruct("<I")

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

def ReadOpCode(_ss: io.BytesIO) -> OpCode:
	(_opcode, ) = _opcode_packer.unpack(_ss.read(_opcode_packer.size))
	return _opcode
def PeekOpCode(_ss: io.BytesIO) -> OpCode:
	(_opcode, ) = _opcode_packer.unpack(_ss.read(_opcode_packer.size))
	_ss.seek(_opcode_packer.size, os.SEEK_CUR)
	return _opcode
def WriteOpCode(_opcode: OpCode, _ss: io.BytesIO):
	_ss.write(_opcode_packer.pack(_opcode))

def UniformSerialize(_instance: BpMessage, _ss: io.BytesIO):
	WriteOpCode(_instance.GetOpCode(), _ss)
	_instance.Serialize(_ss)
def UniformDeserialize(_ss: io.BytesIO) -> BpMessage:
	_opcode: OpCode = ReadOpCode(_ss)
	_instance = MessageFactory(_opcode)
	if _instance is not None:
		_instance.Deserialize(_ss)
	return _instance
