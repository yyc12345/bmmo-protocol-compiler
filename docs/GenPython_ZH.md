# Python生成

## 代码布局

### 基本布局

```py
# snippets/header.py粘贴在此

# OpCode生成在此
class OpCode(enum.IntEnum):
	msg_a: int = 0
	msg_b: int = 1
	# 更多OpCode将在这里继续生成

# alias，enum，struct，msg定义在这里

# 一个tuple，用于记录所有生成的msg的类型，用于快速反序列化。
_all_msgs: tuple[type] = (
	msg_a,
	msg_b,
	# 更多msg的名字将在这里继续生成
)
# 通用反序列化函数生成在此
def UniformDeserialize(_ss: io.BytesIO) -> BpMessage:
	(_opcode, ) = _opcode_packer.unpack(_ss.read(_opcode_packer.size))
	if _opcode < 0 or _opcode >= len(_all_msgs): return None
	else:
		_data = _all_msgs[_opcode]()
		_data.Deserialize(_ss)
		return _data
```

`snippets/header.py`是Python代码的模板。模板定义了一些常用的格式字符串，这些格式字符串会被反复使用，因此将其独立出来定义可以减少不必要的性能开销。  
模板还定义了`BpMessage`和`BpStruct`这两个基类。这两个基类中有`str`类型和OpCode的序列化和反序列化方法。

### Alias布局

Python不支持alias，所有alias将在编译时转换为其对应的数据类型。

### Enum布局

```py
class enum_a(enum.IntEnum):
	Entry: int = 0
	# 有更多项则继续生成
```

所有enum均继承`enum.IntEnum`。  
每一项均为`int`类型，且其数值在编译时确定并以等号指定。

### Struct布局

```py
class struct_a(BpStruct):
	_struct_packer: typing.ClassVar[tuple[pStructStruct]] = (pStructStruct('<B3xI'), pStructStruct('<I'), )

	# Py / Decl写在此处。

	def __init__(self):
		# Py / Ctor写在此处。无内容可够造则写pass
	def Deserialize(self, _ss: io.BytesIO):
		# Py / Deserialize写在此处。无内容可反序列化则跳过1个字节。
	def Serialize(self, _ss: io.BytesIO):
		# Py / Serialize写在此处。无内容可序列化则写入1个空字节。
```

所有struct均继承`BpStruct`。  
每个struct拥有3个函数，构造函数，序列化函数，反序列化函数。  
`_struct_packer`与Python优化相关，参见相应章节。其项按需进行构造。

### Msg布局

```py
class msg_a(BpMessage):
	_struct_packer: typing.ClassVar[tuple[pStructStruct]] = (pStructStruct('<B3xI'), pStructStruct('<I'), )

	# Py / Decl写在此处。

	def __init__(self):
		# Py / Ctor写在此处。无内容可够造则写pass
	def IsReliable(self) -> bool:
		return True # 根据类定义改变
	def GetOpCode(self) -> int:
		return OpCode.msg_a    # 根据类名生成到对应的OpCode
	def Deserialize(self, _ss: io.BytesIO):
		# Py / Deserialize写在此处。无内容可反序列化则跳过1个字节。
	def Serialize(self, _ss: io.BytesIO):
		# Py / Serialize写在此处。无内容可序列化则写入1个空字节。
```

所有msg均继承`BpMessage`。  
每个struct拥有5个函数，构造函数，序列化函数，反序列化函数，获取该msg是否是可靠消息的函数，获取该msg对应OpCode的函数。

## 名称要求

由于生成的代码需要使用一些名称来定义一些内部数据结构，因此请避免使用任何Python关键词或以下词汇。

* OpCode
* UniformDeserialize
* BpStruct
* BpMessage
* IsReliable
* GetOpCode
* Deserialize
* Serialize

## Python优化

### 合并连续的Primitive成员

为了提升Python序打包解包性能，编译器允许将**多个连续的Primitive类型**合并在一起进行打包解包。  
例如对于`int x, y, z;`的声明。编译器将生成如下打包解包语句：

```py
# 序列化
_ss.write(struct.pack("<III", self.x, self.y, self.z))
# 反序列化
(self.x, self.y, self.z, ) = struct.unpack("<III", _ss.read(DSIZE))
```

其中`III`是针对这3个合并了的Primitive类型成员的格式字符串，DSIZE是合并后的数据大小。这些参数需要根据合并的Primitive类型，**以及是否有Align Padding**，在编译期间计算完成。

### Struct预编译

为了避免Python反复分析格式字符串，编译器将创建一个名为_struct_packer的类变量。_struct_packer是一个tuple，_struct_packer中的每一项为已编译的pStructStruct。  
pStructStruct在生产环境中将被解析为struct.Struct。在测试环境中将被解析为一个测试用的复杂类。下文将pStructStruct理解为struct.Struct即可。  
“被合并的连续Primitive成员”，和“Primitive静态数组”将会使用此特性。他们会将他们需要使用的格式字符串预先编译为pStructStruct并存储在_struct_packer中。在运行时直接引用，而无需在运行时解析格式字符串。  
需要注意的是，“合并的连续Primitive成员”，和“Primitive静态数组”虽然均使用此特性，但“Primitive静态数组”和“连续的Primitive成员”绝对不可以合并在一起！它们绝不可能共享同一个pStructStruct！  
_struct_packer的定义位置如下图。`typing.ClassVar`用于暗示它是一个类变量，而非实例变量。

```
# ...
class msg_a(BpMessage):
	_struct_packer: typing.ClassVar[tuple[pStructStruct]] = (pStructStruct('<III'), )
# ...
```

对于_struct_packer的取用将通过下标进行。例如对于上述的被合并的连续Primitive成员，其打包解包将被改写为如下代码。  
此处的打包解包使用了下标为0的已编译的pStructStruct。其中NAME是当前的类名。

```py
# 序列化
_ss.write(NAME._struct_packer[0].pack(self.x, self.y, self.z))
# 反序列化
(self.x, self.y, self.z, ) = NAME._struct_packer[0].unpack(_ss.read(NAME._struct_packer[0].size))
```

对于Primitive静态数组，其打包解包改写为如下代码。

```py
# 序列化
_ss.write(NAME._struct_packer[0].pack(*self.data))
# 反序列化
self.data = list.NAME._struct_packer[0].unpack(_ss.read(NAME._struct_packer[0].size)))
```
