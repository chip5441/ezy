#ifndef EZY_FEATURES_PRINTABLE_H_INCLUDED
#define EZY_FEATURES_PRINTABLE_H_INCLUDED

#include "common.h"

#include <ostream>

namespace ezy
{
namespace features
{
  template <typename T>
  using printable = typename left_shiftable_with<std::ostream&>::template impl<T>;
}
}

#endif
