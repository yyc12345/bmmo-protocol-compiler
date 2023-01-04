
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

class _EndianHelper {
private:
    static uint16_t mEndianProbe = UINT16_C(0xFEFF);
    
public:
    static inline bool IsLittleEndian(){
        return ((_EndianHelper::mEndianProbe >> 8) == UINT16_C(0xFF))
    }
    
    #define XOR_SWAP(val, ia, ib)  (val)[(ia)] ^= (val)[(ib)];  (val)[(ib)] ^= (val)[(ia)]; (val)[(ia)] ^= (val)[(ib)];
    static inline void SwapEndian8(void* val) {
        return; // 8bit data do not need swap
    }
    static inline void SwapEndian16(void* val) {
        if (_EndianHelper::IsLittleEndian()) return;
        
        XOR_SWAP((uint8_t*)val, 0, 1);
    }
    static inline void SwapEndian32(void* val) {
        if (_EndianHelper::IsLittleEndian()) return;

        XOR_SWAP((uint8_t*)val, 0, 3);
        XOR_SWAP((uint8_t*)val, 1, 2);
    }
    static inline void SwapEndian64(void* val) {
        if (_EndianHelper::IsLittleEndian()) return;

        XOR_SWAP((uint8_t*)val, 0, 7);
        XOR_SWAP((uint8_t*)val, 1, 6);
        XOR_SWAP((uint8_t*)val, 2, 5);
        XOR_SWAP((uint8_t*)val, 3, 4);
    }
    #undef XOR_SWAP

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
        const std::vector<T*>* vec, uint32_t new_size, 
        void (*pfunc_init)(T* _p), 
        void (*pfunc_free)(T* _p)
    );
}
