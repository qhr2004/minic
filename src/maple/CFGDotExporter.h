#pragma once

#include "maple/IRGenerator.h"

#include <iosfwd>

namespace maple {

void writeCFGDot(const Module& module, std::ostream& os);

} // namespace maple
