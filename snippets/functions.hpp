
class _BpStruct {
public:
    _BpStruct() {};
    virtual ~_BpStruct() {};

    virtual bool Serialize(std::stringstream* data) = 0;
    virtual bool Deserialize(std::stringstream* data) = 0;
};

class _BpMessage : public _BpStruct {
public:
    _BpMessage() {};
    virtual ~_BpMessage() {};

    virtual _OpCode GetOpCode() = 0;
    virtual bool GetIsReliable() = 0;
};

template<class T, size_t N>
struct _CStyleArray {
	T _Elems[N];
	_CStyleArray() : _Elems() {}
	_CStyleArray(const _CStyleArray& rhs) : _Elems() {
		for (int _i = 0; _i < N; ++_i) this->_Elems[_i] = rhs._Elems[_i];
	}
	_CStyleArray(_CStyleArray&& rhs) : _Elems() {
		for (int _i = 0; _i < N; ++_i) this->_Elems[_i] = std::move(rhs._Elems[_i]);
	}
	_CStyleArray& operator=(const _CStyleArray& rhs) {
		for (int _i = 0; _i < N; ++_i) this->_Elems[_i] = rhs._Elems[_i];
	}
	_CStyleArray& operator=(_CStyleArray&& rhs) {
		for (int _i = 0; _i < N; ++_i) this->_Elems[_i] = std::move(rhs._Elems[_i]);
	}
	[[nodiscard]] constexpr T& operator[](size_t idx) noexcept {
		return _Elems[idx];
	}
	[[nodiscard]] constexpr const T& operator[](size_t idx) const noexcept {
		return _Elems[idx];
	}
	[[nodiscard]] constexpr T* data() noexcept {
		return _Elems;
	}
	[[nodiscard]] constexpr const T* data() const noexcept {
		return _Elems;
	}

	~_CStyleArray() {}
};

class _EndianHelper {
private:
    static const uint16_t mEndianProbe = UINT16_C(0xFEFF);
    
public:
    static inline bool IsLittleEndian(){
        return (((uint8_t*)(&_EndianHelper::mEndianProbe))[0] == UINT8_C(0xFF));
    }
    
    static inline void SwapEndian8(void* val) {
        return; // 8bit data do not need swap
    }
    static inline void SwapEndian16(void* v) {
        if (_EndianHelper::IsLittleEndian()) return;
        
        uint16_t val = *v;
        *(uint16_t*)v = ((uint16_t) (
            (uint16_t) ((uint16_t) (val) >> 8) |
            (uint16_t) ((uint16_t) (val) << 8)
        ))
    }
    static inline void SwapEndian32(void* v) {
        if (_EndianHelper::IsLittleEndian()) return;

        uint32_t val = *v;
        *(uint32_t*)v = ((uint32_t) (
            (((uint32_t) (val) & (uint32_t) 0x000000ffU) << 24) |
            (((uint32_t) (val) & (uint32_t) 0x0000ff00U) <<  8) |
            (((uint32_t) (val) & (uint32_t) 0x00ff0000U) >>  8) |
            (((uint32_t) (val) & (uint32_t) 0xff000000U) >> 24)
        ))
    }
    static inline void SwapEndian64(void* v) {
        if (_EndianHelper::IsLittleEndian()) return;

        uint64_t val = *v;
        *(uint64_t*)v = ((uint64_t) ( \
              (((uint64_t) (val) &
            (uint64_t) UINT64_C(0x00000000000000ffU)) << 56) |
              (((uint64_t) (val) &
            (uint64_t) UINT64_C(0x000000000000ff00U)) << 40) |
              (((uint64_t) (val) &
            (uint64_t) UINT64_C(0x0000000000ff0000U)) << 24) |
              (((uint64_t) (val) &
            (uint64_t) UINT64_C(0x00000000ff000000U)) <<  8) |
              (((uint64_t) (val) &
            (uint64_t) UINT64_C(0x000000ff00000000U)) >>  8) |
              (((uint64_t) (val) &
            (uint64_t) UINT64_C(0x0000ff0000000000U)) >> 24) |
              (((uint64_t) (val) &
            (uint64_t) UINT64_C(0x00ff000000000000U)) >> 40) |
              (((uint64_t) (val) &
            (uint64_t) UINT64_C(0xff00000000000000U)) >> 56)
        ))
    }

    static inline void SwapEndianArray8(void* val, uint32_t len) {
        return; // 8bit data do not need swap
    }
    static inline void SwapEndianArray16(void* val, uint32_t len) {
        if (_EndianHelper::IsLittleEndian()) return;
        
        uint32_t c = UINT32_C(0);
        for(; c < len; ++c) _EndianHelper::SwapEndian16(((uint16_t*)val) + c);
    }
    static inline void SwapEndianArray32(void* val, uint32_t len) {
        if (_EndianHelper::IsLittleEndian()) return;

        uint32_t c = UINT32_C(0);
        for(; c < len; ++c) _EndianHelper::SwapEndian32(((uint32_t*)val) + c);
    }
    static inline void SwapEndianArray64(void* val, uint32_t len) {
        if (_EndianHelper::IsLittleEndian()) return;

        uint32_t c = UINT32_C(0);
        for(; c < len; ++c) _EndianHelper::SwapEndian64(((uint64_t*)val) + c);
    }
};

namespace _Helper {
    _BpMessage* UniformDeserializer(std::stringstream* ss);
    
    bool PeekOpCode(std::stringstream* ss, _OpCode* code);
    bool ReadOpCode(std::stringstream* ss, _OpCode* code);
    bool WriteOpCode(std::stringstream* ss, _OpCode code);
    
    bool ReadString(std::stringstream* ss, std::string* strl);
    bool WriteString(std::stringstream* ss, std::string* strl);
    
    template<typename T> void ResizePtrVector(
        std::vector<T*>* vec, uint32_t new_size, 
        void (*pfunc_init)(T* _p), 
        void (*pfunc_free)(T* _p)
    );
}
