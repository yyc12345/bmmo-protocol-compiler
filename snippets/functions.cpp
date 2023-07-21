
// ==================== RW Macro ====================

// ss: std::istream&
#define _SS_PRE_RD(ist) if (!(ist).good()) \
return false;

// ptr: void*
// len: size_t
#define _SS_RD_STRUCT(ist, ptr, len) (ist).read(reinterpret_cast<char*>(ptr), (len)); \
if (!(ist).good() || (ist).gcount() != (len)) \
return false;

// strl: std::string&
#define _SS_RD_STRING(ist, strl) if (!BPHelper::_ReadString(ist, strl)) \
return false;

#define _SS_RD_BLANK(ist, len) if (!BPHelper::_ReadBlank(ist, len)) \
return false;

// obj: BpStruct&
#define _SS_RD_FUNCTION(ist, obj) if (!((obj).Deserialize(ist))) \
return false;

#define _SS_END_RD(ist) return (ist).good();

// ss: std::ostream&
#define _SS_PRE_WR(ost) ;

// ptr: void*
// len: size_t
#define _SS_WR_STRUCT(ost, ptr, len) (ost).write(reinterpret_cast<const char*>(ptr), (len)); \
if (!(ost).good()) \
return false;

// obj: BpStruct&
#define _SS_WR_STRING(ost, strl) if (!BPHelper::_WriteString(ost, strl)) \
return false;

#define _SS_WR_BLANK(ost, len) if (!BPHelper::_WriteBlank(ost, len)) \
return false;

// obj: BpStruct&
#define _SS_WR_FUNCTION(ost, obj) if (!((obj).Serialize(ost))) \
return false;

#define _SS_END_WR(ost) return (ost).good();

namespace BPHelper {
	// ==================== Global Variabls & Testbench Function ====================
#if _ENABLE_BP_TESTBENCH
	namespace ByteSwap {
		bool g_ForceBigEndian = false;
		bool _InterIsLittleEndian() {
			if (g_ForceBigEndian) return !_g_IsLittleEndian;
			else return _g_IsLittleEndian;
		}
	}
#endif

	// ==================== RW Assist Functions ====================

	bool UniformSerialize(std::ostream& ss, BpMessage* instance) {
		OpCode code = instance->GetOpCode();
		if _BP_IS_BIG_ENDIAN { BPHelper::ByteSwap::_SwapSingle<uint32_t>(&code); }
		_SS_WR_STRUCT(ss, &code, sizeof(uint32_t));

		return instance->Serialize(ss);
	}

	BpMessage* UniformDeserialize(std::istream& ss) {
		OpCode code;
		if (![&ss, &code]() { _SS_RD_STRUCT(ss, &code, sizeof(uint32_t)); return true; }()) return nullptr;
		if _BP_IS_BIG_ENDIAN { BPHelper::ByteSwap::_SwapSingle<uint32_t>(&code); }

		BpMessage* instance = MessageFactory(code);
		if (instance != nullptr) {
			if (!instance->Deserialize(ss)) {
				delete instance;
				instance = nullptr;
			}
		}
		return instance;
	}

	bool _ReadString(std::istream& ss, std::string& strl) {
		uint32_t length = 0;
		_SS_RD_STRUCT(ss, &length, sizeof(uint32_t));
		if _BP_IS_BIG_ENDIAN { BPHelper::ByteSwap::_SwapSingle<uint32_t>(&length); }
		// if (length > ss.str().length()) return false;

		strl.resize(length);
		_SS_RD_STRUCT(ss, strl.data(), length);

		return true;
	}

	bool _WriteString(std::ostream& ss, std::string& strl) {
		uint32_t length = strl.size();
		if _BP_IS_BIG_ENDIAN { BPHelper::ByteSwap::_SwapSingle<uint32_t>(&length); }
		_SS_WR_STRUCT(ss, &length, sizeof(uint32_t));
		_SS_WR_STRUCT(ss, strl.c_str(), strl.size());

		return true;
	}

	bool _ReadBlank(std::istream& ss, uint32_t offset) {
		ss.seekg(offset, std::ios_base::cur);
		return ss.good();
	}

	bool _WriteBlank(std::ostream& ss, uint32_t offset) {
		constexpr const uint32_t len_ph = 512;
		const char placeholder[len_ph]{ 0 };

		while (true) {
			if (offset > len_ph) {
				_SS_WR_STRUCT(ss, placeholder, len_ph);
				offset -= len_ph;
			} else {
				_SS_WR_STRUCT(ss, placeholder, offset);
				break;
			}
		}

		return true;
	}

}
