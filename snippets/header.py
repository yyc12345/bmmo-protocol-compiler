import io, struct, os, enum, typing
import __main__

_g_EnableBpTestbench = (getattr(__main__, '_ENABLE_BP_TESTBENCH', None) is not None) and isinstance(__main__._ENABLE_BP_TESTBENCH, bool) and __main__._ENABLE_BP_TESTBENCH
if _g_EnableBpTestbench:
	# define testbench used data
	g_ForceBigEndian: bool = False

	def _ChangeFormatEndian(old_format: str) -> str:
		return '>' + old_format[1:]

	class _FakeStructStruct(object):
		__mLEStruct: struct.Struct
		__mBEStruct: struct.Struct
		def __init__(self, format):
			self.__mLEStruct = struct.Struct(format)
			self.__mBEStruct = struct.Struct(_ChangeFormatEndian(format))
		def __PickStruct(self) -> struct.Struct:
			if g_ForceBigEndian: return self.__mBEStruct
			else: return self.__mLEStruct
		def pack(self, *args, **kwargs):
			return self.__PickStruct().pack(*args, **kwargs)
		def unpack(self, *args, **kwargs):
			return self.__PickStruct().unpack(*args, **kwargs)
		@property
		def size(self):
			return self.__PickStruct().size
		
	def _FakeStructPack(format, *args, **kwargs):
		return struct.pack((_ChangeFormatEndian(format) if g_ForceBigEndian else format), *args, **kwargs)
	
	def _FakeStructUnpack(format, *args, **kwargs):
		return struct.unpack((_ChangeFormatEndian(format) if g_ForceBigEndian else format), *args, **kwargs)
	
	_pStructStruct = _FakeStructStruct
	_pStructPack = _FakeStructPack
	_pStructUnpack = _FakeStructUnpack
else:
	# production environment
	_pStructStruct = struct.Struct
	_pStructPack = struct.pack
	_pStructUnpack = struct.unpack
