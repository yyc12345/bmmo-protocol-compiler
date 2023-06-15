# C\#生成

## 代码布局

### 基本布局

```cs
// snippets/header.cs粘贴在此

namespace /* 命名空间生成在此 */ {

    // OpCode生成在此
    public enum OpCode : uint {
        msg_a = 0,
        msg_b = 1
        // 更多OpCode将在这里继续生成
    }

    // snippets/functions.cs粘贴在此

    // alias，enum，struct，msg定义在这里

    // 通用反序列化函数生成在此
	public static partial class BPHelper {
		public static BpMessage UniformDeserialize(BinaryReader _br) {
			BpMessage _data = null;
			switch ((OpCode)_br.ReadUInt32()) {
				case OpCode.msg.a:
					_data = new msg_a();
					break;
				case OpCode.msg_b:
					_data = new msg_b();
					break;
				// 更多msg将在这里继续生成
			}
			if (!(_data is null)) _data.Deserialize(_br);
			return _data;
		}
	}

}

```

`snippets/header.cs`只定义了需要引用的命名空间。  
`snippets/functions.cs`模板定义了`BpMessage`和`BpStruct`这两个基类。以及数十种数据序列化与反序列化的函数。还有一个通用序列化函数`UniformSerialize`。

### Alias布局

C\#不支持alias，所有alias将在编译时转换为其对应的数据类型。

### Enum布局

```cs
public enum enum_a : uint {
    Entry = 0
    // 有更多项则继续生成
}
```

此处enum的底层类型`uint`仅为示例，编译时需要按用户提供的底层类型替换。  
每一项其数值在编译时确定并以等号指定。

### Struct布局

```cs
public class struct_a : BpStruct {

    // C# / Decl写在此处。

    public struct_a() {
        // C# / Ctor写在此处。
    }
    public override void Deserialize(BinaryReader _br) {
        // C# / Deserialize写在此处。无内容可反序列化则跳过1个字节。
    }
    public override void Serialize(BinaryWriter _bw) {
        // C# / Serialize写在此处。无内容可序列化则写入1个空字节。
    }
}
```

所有struct均继承`BpStruct`。  
每个struct拥有3个函数，构造函数，序列化函数，反序列化函数。

### Msg布局

```cs
public class msg_a : BpStruct {

    // C# / Decl写在此处。

    public msg_a() {
        // C# / Ctor写在此处。
    }
    public override bool IsReliable() => true;  // 根据类定义改变
    public override OpCode GetOpCode() => OpCode.msg_a; //根据类名生成到对应的OpCode
    public override void Deserialize(BinaryReader _br) {
        // C# / Deserialize写在此处。无内容可反序列化则跳过1个字节。
    }
    public override void Serialize(BinaryWriter _bw) {
        // C# / Serialize写在此处。无内容可序列化则写入1个空字节。
    }
}
```

所有msg均继承`BpMessage`。  
每个struct拥有5个函数，构造函数，序列化函数，反序列化函数，获取该msg是否是可靠消息的函数，获取该msg对应OpCode的函数。

## 名称要求

由于生成的代码需要使用一些名称来定义一些内部数据结构，因此请避免使用任何C\#关键词或以下词汇。

* OpCode
* BPHelper
* BpStruct
* BpMessage
* IsReliable
* GetOpCode
* Deserialize
* Serialize
