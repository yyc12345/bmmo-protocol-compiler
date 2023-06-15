
class BpStruct {
public:
	BpStruct() {};
	virtual ~BpStruct() {};

	virtual bool Serialize(std::stringstream& data) = 0;
	virtual bool Deserialize(std::stringstream& data) = 0;
};

class BpMessage : public BpStruct {
public:
	BpMessage() {};
	virtual ~BpMessage() {};

	virtual OpCode GetOpCode() = 0;
	virtual bool IsReliable() = 0;
};

template<class _Ty, size_t _Nsize>
struct CStyleArray {
	_Ty _Elems[_Nsize];
	CStyleArray() : _Elems() {}
	CStyleArray(const CStyleArray& rhs) : _Elems() {
		if constexpr (std::is_arithmetic_v<_Ty>) {
			std::memmove(_Elems, rhs._Elems, sizeof(_Ty) * _Nsize);
		} else {
			for (int _i = 0; _i < _Nsize; ++_i) this->_Elems[_i] = rhs._Elems[_i];
		}
	}
	CStyleArray(CStyleArray&& rhs) : _Elems() {
		if constexpr (std::is_arithmetic_v<_Ty>) {
			std::memmove(_Elems, rhs._Elems, sizeof(_Ty) * _Nsize);
		} else {
			for (int _i = 0; _i < _Nsize; ++_i) this->_Elems[_i] = std::move(rhs._Elems[_i]);
		}
	}
	CStyleArray& operator=(const CStyleArray& rhs) {
		if constexpr (std::is_arithmetic_v<_Ty>) {
			std::memmove(_Elems, rhs._Elems, sizeof(_Ty) * _Nsize);
		} else {
			for (int _i = 0; _i < _Nsize; ++_i) this->_Elems[_i] = rhs._Elems[_i];
		}
	}
	CStyleArray& operator=(CStyleArray&& rhs) {
		if constexpr (std::is_arithmetic_v<_Ty>) {
			std::memmove(_Elems, rhs._Elems, sizeof(_Ty) * _Nsize);
		} else {
			for (int _i = 0; _i < _Nsize; ++_i) this->_Elems[_i] = std::move(rhs._Elems[_i]);
		}
	}
	[[nodiscard]] constexpr _Ty& operator[](size_t idx) noexcept {
		return _Elems[idx];
	}
	[[nodiscard]] constexpr const _Ty& operator[](size_t idx) const noexcept {
		return _Elems[idx];
	}
	[[nodiscard]] constexpr _Ty* data() noexcept {
		return _Elems;
	}
	[[nodiscard]] constexpr const _Ty* data() const noexcept {
		return _Elems;
	}

	~CStyleArray() {}
};

namespace BPHelper {
	bool UniformSerialize(std::stringstream& ss, BpMessage* instance);
	BpMessage* UniformDeserialize(std::stringstream& ss);

	bool ReadString(std::stringstream& ss, std::string& strl);
	bool WriteString(std::stringstream& ss, std::string& strl);

	namespace ByteSwap {
#if __cpp_lib_endian
		constexpr const bool g_IsLittleEndian = (std::endian::native == std::endian::little);
#define _ENDIAN_CHECKER constexpr (g_IsLittleEndian)
#else
		const uint16_t g_EndianProbe = UINT16_C(0xFEFF);
		const bool g_IsLittleEndian = reinterpret_cast<const uint8_t*>(&g_EndianProbe)[0] == UINT8_C(0xFF);
#define _ENDIAN_CHECKER (g_IsLittleEndian)
#endif

		template <class>
		constexpr bool g_AlwaysFalse = false;
		template <class _Ty, std::enable_if_t<std::is_integral_v<_Ty>, int> = 0>
		void SwapSingle(void* v) {
			if _ENDIAN_CHECKER return;

#if __cpp_lib_byteswap
			* reinterpret_cast<_Ty*>(v) = std::byteswap(*reinterpret_cast<_Ty*>(v));
#else
			if constexpr (sizeof(_Ty) == 1) {
				return;     // 8bit data do not need swap
			} else if constexpr (sizeof(_Ty) == 2) {

				uint16_t val = *reinterpret_cast<uint16_t*>(v);
				*reinterpret_cast<uint16_t*>(v) = ((uint16_t)(
					(uint16_t)((uint16_t)(val) >> 8) |
					(uint16_t)((uint16_t)(val) << 8)
					));

			} else if constexpr (sizeof(_Ty) == 4) {

				uint32_t val = *reinterpret_cast<uint32_t*>(v);
				*reinterpret_cast<uint32_t*>(v) = ((uint32_t)(
					(((uint32_t)(val) & (uint32_t)0x000000ffU) << 24) |
					(((uint32_t)(val) & (uint32_t)0x0000ff00U) << 8) |
					(((uint32_t)(val) & (uint32_t)0x00ff0000U) >> 8) |
					(((uint32_t)(val) & (uint32_t)0xff000000U) >> 24)
					));

			} else if constexpr (sizeof(_Ty) == 8) {

				uint64_t val = *reinterpret_cast<uint64_t*>(v);
				*reinterpret_cast<uint64_t*>(v) = ((uint64_t)(\
					(((uint64_t)(val) &
					(uint64_t)UINT64_C(0x00000000000000ff)) << 56) |
					(((uint64_t)(val) &
					(uint64_t)UINT64_C(0x000000000000ff00)) << 40) |
					(((uint64_t)(val) &
					(uint64_t)UINT64_C(0x0000000000ff0000)) << 24) |
					(((uint64_t)(val) &
					(uint64_t)UINT64_C(0x00000000ff000000)) << 8) |
					(((uint64_t)(val) &
					(uint64_t)UINT64_C(0x000000ff00000000)) >> 8) |
					(((uint64_t)(val) &
					(uint64_t)UINT64_C(0x0000ff0000000000)) >> 24) |
					(((uint64_t)(val) &
					(uint64_t)UINT64_C(0x00ff000000000000)) >> 40) |
					(((uint64_t)(val) &
					(uint64_t)UINT64_C(0xff00000000000000)) >> 56)
					));

			} else {
				static_assert(g_AlwaysFalse<_Ty>, "Unexpected integer size");
			}
#endif
		}

		template <class _Ty, std::enable_if_t<std::is_integral_v<_Ty>, int> = 0>
		void SwapArray(void* v, uint32_t len) {
			if _ENDIAN_CHECKER return;

			uint32_t c = UINT32_C(0);
			for (; c < len; ++c) SwapSingle<_Ty>(reinterpret_cast<_Ty*>(v) + c);
		}

#undef _ENDIAN_CHECKER

	}

}
