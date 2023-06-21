# C++生成

C++代码由于其特性，分为HPP部分和CPP部分，将于下述部分分开介绍。

## HPP代码布局

HPP文件主要定义各类所需的函数，以及用户指定的各种结构。

### 基本布局

```cpp
// snippets/header.hpp粘贴在此

namespace /* 命名空间生成在此 */ {

    // OpCode生成在此
    enum class OpCode : uint32_t {
        msg_a = 0,
        msg_b = 1
        // 更多OpCode将在这里继续生成
    }

    // snippets/functions.hpp粘贴在此
    
    // alias，enum，struct，msg定义在这里

    // 测试用代码生成在这里
#if _ENABLE_BP_TESTBENCH
    const std::map<std::string, _BPTestbench_StructProperty> _BPTestbench_StructLayouts {
        // 此处记录所有生成的struct，msg的类结构
    };
    const std::vector<std::string> _BPTestbench_MessageList {
        // 此处记录所有生成的msg的名称。以OpCode中的顺序排序。
    };
#endif
}

```

`snippets/header.hpp`定义了需要引用的头文件。  
`snippets/functions.hpp`定义了以下内容：

* 测试用的enum，struct等。
* 基类BpStruct和BpMessage
* 不具有任何额外成员的，模拟C-style数组的struct，CStyleArray。并为其添加必要的拷贝，移动构造函数，以及拷贝，移动赋值函数。
* 定义命名空间BPHelper，其中包括各类辅助函数。
* 定义命名空间BPHelper::_ByteSwap，其中包含端序转换的相关代码。

_ENABLE_BP_TESTBENCH是启用测试的宏。若在引用此HPP前定义此宏，则将启用测试用代码的编译。本宏不应该在生产环境中定义。在本工程中，本宏只在testbench.cpp中引用，为测试服务。  
由于C++没有动态反射机制，所以在测试中需要创建_BPTestbench_StructLayouts和_BPTestbench_MessageList变量来记录生成的msg的个数，以及struct和msg中各类成员的属性和相对偏移以便测试。相当于实现了一个反射。

### Alias布局

```cpp
using alias_t = source_t;
```

alias_t为新别名。source_t为原名。

### Enum布局

```cpp
enum class enum_a : uint32_t {
    Entry = 0
    // 有更多项则继续生成
};
```

此处enum的底层类型`uint32_t`仅为示例，编译时需要按用户提供的底层类型替换。  
每一项其数值在编译时确定并以等号指定。

### Struct布局

```cpp
class struct_a : public BpStruct {
public:
    #pragma pack(1)
    struct Payload_t {
        // C++ / Decl写在此处。
        Payload_t();
        ~Payload_t();
        Payload_t(const Payload_t& _rhs);
        Payload_t(Payload_t&& _rhs) noexcept;
        Payload_t& operator=(const Payload_t& _rhs);
        Payload_t& operator=(Payload_t&& _rhs) noexcept;
        bool Serialize(std::stringstream& _ss);
        bool Deserialize(std::stringstream& _ss);
        void ByteSwap();
    };
    #pragma pack()
    Payload_t Payload;
    struct_empty_narrow() : Payload() {}
    virtual ~struct_empty_narrow() {}
    struct_empty_narrow(const struct_empty_narrow& rhs) : Payload(rhs.Payload) {}
    struct_empty_narrow(struct_empty_narrow&& rhs) noexcept : Payload(std::move(rhs.Payload)) {}
    struct_empty_narrow& operator=(const struct_empty_narrow& rhs) { this->Payload = rhs.Payload; return *this; }
    struct_empty_narrow& operator=(struct_empty_narrow&& rhs) noexcept { this->Payload = std::move(rhs.Payload); return *this; }
    virtual bool Serialize(std::stringstream& _ss) override {
        Payload_t* pPayload = nullptr;
        if _BP_IS_BIG_ENDIAN { pPayload = new Payload_t(Payload); pPayload->ByteSwap(); }
        else { pPayload = &Payload; }
        bool hr = pPayload->Serialize(_ss);
        if _BP_IS_BIG_ENDIAN { delete pPayload; }
        return hr;
    }
    virtual bool Deserialize(std::stringstream& _ss) override {
        bool hr = Payload.Deserialize(_ss);
        if _BP_IS_BIG_ENDIAN { Payload.ByteSwap(); }
        return hr;
    }
};
```

所有struct均继承`BpStruct`。  
每个struct定义了一个公开内部类型Payload_t。由于外部的类具有虚函数，因此其数据地址起始位置被虚表占据，不能保证由连续此类构成的数据串的连续性。因此创建一个不具有任何虚函数的内部类型Payload_t，作为真正存储数据的载体。外部类公开实现一个成员Payload来存储数据。  
Payload_t实现构造函数，析构函数，复制构造函数，移动构造函数，复制赋值函数，移动赋值函数。这些函数是C++需要的。另外还实现序列化函数，反序列化函数，ByteSwap函数。这些函数是Bp所需要的。  
外部类所实现的这些函数实际上是对内部类函数的调用。因此是固定且唯一的。  
`#pragma pack`指令确保Payload_t以1进行对齐。为我们手动指定对齐提供便利。

### Msg布局

```cpp
class msg_a : public BpMessage {
public:
    #pragma pack(1)
    struct Payload_t {
        // C++ / Decl写在此处。
        Payload_t();
        ~Payload_t();
        Payload_t(const Payload_t& _rhs);
        Payload_t(Payload_t&& _rhs) noexcept;
        Payload_t& operator=(const Payload_t& _rhs);
        Payload_t& operator=(Payload_t&& _rhs) noexcept;
        bool Serialize(std::stringstream& _ss);
        bool Deserialize(std::stringstream& _ss);
        void ByteSwap();
    };
    #pragma pack()
    Payload_t Payload;
    struct_empty_narrow() : Payload() {}
    virtual ~struct_empty_narrow() {}
    struct_empty_narrow(const struct_empty_narrow& rhs) : Payload(rhs.Payload) {}
    struct_empty_narrow(struct_empty_narrow&& rhs) noexcept : Payload(std::move(rhs.Payload)) {}
    struct_empty_narrow& operator=(const struct_empty_narrow& rhs) { this->Payload = rhs.Payload; return *this; }
    struct_empty_narrow& operator=(struct_empty_narrow&& rhs) noexcept { this->Payload = std::move(rhs.Payload); return *this; }
    virtual bool Serialize(std::stringstream& _ss) override {
        Payload_t* pPayload = nullptr;
        if _BP_IS_BIG_ENDIAN { pPayload = new Payload_t(Payload); pPayload->ByteSwap(); }
        else { pPayload = &Payload; }
        bool hr = pPayload->Serialize(_ss);
        if _BP_IS_BIG_ENDIAN { delete pPayload; }
        return hr;
    }
    virtual bool Deserialize(std::stringstream& _ss) override {
        bool hr = Payload.Deserialize(_ss);
        if _BP_IS_BIG_ENDIAN { Payload.ByteSwap(); }
        return hr;
    }
    virtual OpCode GetOpCode() override { return OpCode::msg_a; }   //根据类名生成到对应的OpCode
    virtual bool IsReliable() override { return true; } // 根据类定义改变
};
```

所有msg均继承`BpMessage`。  
几乎所有函数与struct具有相似含义。只额外多实现了BpMessage中需要实现的GetOpCode和IsReliable函数。


## CPP代码布局

CPP代码主要为HPP中的各种定义给予实现。

### 基本布局

```cpp
// include语句生成在此
#include "CodeGenTest.hpp"

namespace /* 命名空间生成在此 */ {

    // snippets/functions.cpp粘贴在此
    
    // alias，enum，struct，msg定义在这里

    // snippets/tail.cpp粘贴在此
}

```

include语句由编译器计算HPP文件对CPP文件的相对位置，并放置于include语句中，保证include语句总能include到生成的HPP文件。  
`snippets/functions.cpp`定义了HPP文件中声明的函数的实现，以及一系列用于读取写入数据的宏。  
`snippets/tail.cpp`将`snippets/functions.cpp`中定义的宏取消定义，防止宏污染。

### Alias布局

CPP代码中不涉及alias

### Enum布局

CPP代码中不涉及enum

### Struct布局

```cpp
struct_a::Payload_t::Payload_t() : 
// C++ / Ctor写在此处。
{}
struct_a::Payload_t::~Payload_t() {}
struct_a::Payload_t::Payload_t(const struct_a::Payload_t& _rhs) : 
// C++ / Copy Ctor写在此处。
{}
struct_a::Payload_t::Payload_t(struct_a::Payload_t&& _rhs) noexcept : 
// C++ / Move Ctor写在此处。
{}
struct_a::Payload_t& struct_a::Payload_t::operator=(const struct_a::Payload_t& _rhs) {
    // C++ / Copy operator=写在此处。
    return *this;
}
struct_a::Payload_t& struct_a::Payload_t::operator=(struct_a::Payload_t&& _rhs) noexcept {
    // C++ / Move operator=写在此处。
    return *this;
}
void struct_a::Payload_t::ByteSwap() {
    if _BP_IS_LITTLE_ENDIAN return;
    // C++ / ByteSwap写在此处。
}
bool struct_a::Payload_t::Deserialize(std::stringstream& _ss) {
    _SS_PRE_RD(_ss);
    // C++ / Serialize写在此处。
    _SS_END_RD(_ss);
}
bool struct_a::Payload_t::Serialize(std::stringstream& _ss) {
    _SS_PRE_WR(_ss);
    // C++ / Deserialize写在此处。
    _SS_END_WR(_ss);
}
```

此处实现了HPP文件中对应类型的内置类型Payload_t的相关函数。
Ctor, Copy Ctor, Move Ctor的语句是写在初始化列表中的。如果没有任何成员，则初始化列表为空，初始化列表的冒号（:）也将省略。不同成员的初始化列表语句之间用逗号分隔。

### Msg布局

与struct布局一致。

## 名称要求

由于生成的代码需要使用一些名称来定义一些内部数据结构，因此请避免使用任何C++关键词或以下词汇。

* OpCode
* BPHelper
* BpStruct
* BpMessage
* IsReliable
* GetOpCode
* Deserialize
* Serialize

## C++优化

### 合并成员

C++可以使用类型不安全的方式来快速读写数据，生成的代码亦采取这一点来加速数据的读取写入。  
类似于Python，生成的代码将合并**连续的**Primitive, Primitive Tuple, Narual, Natural Tuple，将这四者看为一个整体并计算其大小N，然后获取这个整体的起始成员的首地址（也就是这个整体的首地址）。直接读取或写入N个字节的数据。

对于Primitive List，Natural List，则需要先写入其长度L，获取其单个成员的大小N。然后直接取列表首地址，直接读取或写入N * L个字节的数据。

其它结构则使用平凡的读取和写入函数。

C++之所以可以这样读写数据，是因为在设计时，我们将每个数据的Payload_t设计为没有任何多余字段的设计。通过保障C++结构与我们的输入输出的数据结构完全一致，来获得直接从内存读写的能力。这也是我们为什么要在class内再定义一个struct Payload_t的原因，因为class本身有虚函数，会占用类开头的一个指针类型大小的数据位置。无法做到结构与输入输出数据结构一致，因而也不能一次性读取N个结构。
