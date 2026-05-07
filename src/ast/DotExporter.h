#pragma once

#include "ast/AST.h"

#include <iosfwd>

void writeASTDot(const ASTNode& node, std::ostream& os);
