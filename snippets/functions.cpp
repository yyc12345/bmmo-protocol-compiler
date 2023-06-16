
// ==================== RW Macro ====================

// ss: std::stringstream&
#define _SS_PRE_RD(ss) if (!(ss).good()) \
return false;

// ptr: void*
// len: size_t
#define _SS_RD_STRUCT(ss, ptr, len) (ss).read(reinterpret_cast<char*>(ptr), (len)); \
if (!(ss).good() || (ss).gcount() != (len)) \
return false;

// strl: std::string&
#define _SS_RD_STRING(ss, strl) if (!BPHelper::ReadString(ss, strl)) \
return false;

// obj: BpStruct&
#define _SS_RD_FUNCTION(ss, obj) if (!((obj).Deserialize(ss))) \
return false;

#define _SS_END_RD(ss) return (ss).good();

// ss: std::stringstream&
#define _SS_PRE_WR(ss) ;

// ptr: void*
// len: size_t
#define _SS_WR_STRUCT(ss, ptr, len) (ss).write(reinterpret_cast<const char*>(ptr), (len));

// func: bool
#define _SS_WR_STRING(ss, strl) if (!BPHelper::WriteString(ss, strl)) \
return false;

// obj: BpStruct&
#define _SS_WR_FUNCTION(ss, obj) if (!((obj).Serialize(ss))) \
return false;

#define _SS_END_WR(ss) return (ss).good();

namespace BPHelper {

	// ==================== RW Assist Functions ====================

	bool UniformSerialize(std::stringstream& ss, BpMessage* instance) {
		OpCode code = instance->GetOpCode();
		BPHelper::ByteSwap::SwapSingle<uint32_t>(&code);
		_SS_WR_STRUCT(ss, &code, sizeof(uint32_t));

		return instance->Serialize(ss);
	}

	BpMessage* UniformDeserialize(std::stringstream& ss) {
		OpCode code;
		if (![&ss, &code]() { _SS_RD_STRUCT(ss, &code, sizeof(uint32_t)); return true; }()) return nullptr;
		BPHelper::ByteSwap::SwapSingle<uint32_t>(&code);

		BpMessage* instance = MessageFactory(code);
		if (instance != nullptr) {
			if (!instance->Deserialize(ss)) {
				delete instance;
				instance = nullptr;
			}
		}
		return instance;
	}

	bool ReadString(std::stringstream& ss, std::string& strl) {
		uint32_t length = 0;
		_SS_RD_STRUCT(ss, &length, sizeof(uint32_t));
		BPHelper::ByteSwap::SwapSingle<uint32_t>(&length);
		if (length > ss.str().length()) return false;

		strl.resize(length);
		_SS_RD_STRUCT(ss, strl.data(), length);

		return true;
	}

	bool WriteString(std::stringstream& ss, std::string& strl) {
		uint32_t length = strl.size();
		BPHelper::ByteSwap::SwapSingle<uint32_t>(&length);
		_SS_WR_STRUCT(ss, &length, sizeof(uint32_t));
		_SS_WR_STRUCT(ss, strl.c_str(), strl.size());

		return true;
	}

	void ReadBlank(std::stringstream& ss, uint32_t offset) {
		ss.seekg(offset, std::ios_base::cur);
	}

	void WriteBlank(std::stringstream& ss, uint32_t offset) {
		if (offset == 0) return;
		ss.seekp(offset - 1, std::ios_base::cur);
		
		constexpr const uint8_t c_ZeroFill = UINT8_C(0);
		_SS_WR_STRUCT(ss, &c_ZeroFill, sizeof(uint8_t));
	}

}
