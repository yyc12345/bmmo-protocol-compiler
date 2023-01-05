
// ==================== RW Macro ====================

#define SSTREAM_PRE_RD(ss) if (!(ss)->good()) \
return false;

#define SSTREAM_RD_STRUCT(ss, len, ptr) (ss)->read((char*)(ptr), (len)); \
if (!(ss)->good() || (ss)->gcount() != (len)) \
return false;

#define SSTREAM_RD_FUNCTION(ss, func) if (!(func)) \
return false;

#define SSTREAM_END_RD(ss) return (ss)->good();

#define SSTREAM_PRE_WR(ss) ;

#define SSTREAM_WR_STRUCT(ss, len, ptr) (ss)->write((char*)(ptr), (len));

#define SSTREAM_WR_FUNCTION(ss, func) if (!(func)) \
return false;

#define SSTREAM_END_WR(ss) return (ss)->good();

namespace _Helper {

    // ==================== RW Assist Functions ====================

    bool PeekOpCode(std::stringstream* ss, _OpCode* code) {
        SSTREAM_RD_STRUCT(ss, sizeof(uint32_t), code);
        _EndianHelper::SwapEndian32(code);
        
        ss->seekg(-(int32_t)(sizeof(uint32_t)), std::ios_base::cur);
        return true;
    }

    bool ReadOpCode(std::stringstream* ss, _OpCode* code) {
        SSTREAM_RD_STRUCT(ss, sizeof(uint32_t), code);
        _EndianHelper::SwapEndian32(code);
        return true;
    }

    bool WriteOpCode(std::stringstream* ss, _OpCode code) {
        _EndianHelper::SwapEndian32(&code);
        SSTREAM_WR_STRUCT(ss, sizeof(uint32_t), &code);
        return true;
    }

    bool ReadString(std::stringstream* ss, std::string* strl) {
        uint32_t length = 0;
        SSTREAM_RD_STRUCT(ss, sizeof(uint32_t), &length);
        _EndianHelper::SwapEndian32(&length);
        if (length > ss->str().length()) return false;

        strl->resize(length);
        SSTREAM_RD_STRUCT(ss, length, strl->data());

        return true;
    }

    bool WriteString(std::stringstream* ss, std::string* strl) {
        uint32_t length = strl->size();
        _EndianHelper::SwapEndian32(&length);
        SSTREAM_WR_STRUCT(ss, sizeof(uint32_t), &length);
        SSTREAM_WR_STRUCT(ss, strl->size(), strl->c_str());

        return true;
    }

    // ==================== Vector Assist Functions ====================
    template<typename T> void ResizePtrVector(
        std::vector<T*>* vec, uint32_t new_size, 
        void (*pfunc_init)(T* _p), 
        void (*pfunc_free)(T* _p)
    ) {
        size_t old_size = vec->size();
        size_t c;
        if (new_size > old_size) {
            vec->resize(new_size);
            for(c = old_size; c < new_size; ++c) {
                (*vec)[c] = new T();
                if (pfunc_init != NULL) pfunc_init((*vec)[c]);
            }
        } else if (new_size < old_size) {
            for(c = new_size; c < old_size; ++c) {
                if (pfunc_free != NULL) pfunc_free((*vec)[c]);
                delete ((*vec)[c]);
            }
            vec->resize(new_size);
        }
    }
    
}