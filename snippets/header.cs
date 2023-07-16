// https://learn.microsoft.com/en-us/dotnet/csharp/language-reference/preprocessor-directives
#if NETCOREAPP2_1 || NETCOREAPP2_2 || NETCOREAPP3_0 || NETCOREAPP3_1 || NET5_0_OR_GREATER
#define _BP_MODERN_CSHARP
#endif

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

#if _BP_MODERN_CSHARP
using System.Buffers.Binary;
#endif
