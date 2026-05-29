#pragma once

#include "base/gateway/Cstddef.hpp"
#include "base/gateway/Cstdint.hpp"

namespace growth {

using Size = Cstddef::Size;

using U8  = Cstdint::U8;
using U16 = Cstdint::U16;
using U32 = Cstdint::U32;
using U64 = Cstdint::U64;
using I8  = Cstdint::I8;
using I16 = Cstdint::I16;
using I32 = Cstdint::I32;
using I64 = Cstdint::I64;

using Seed    = U64;
using LayerId = U8;

} // namespace growth
