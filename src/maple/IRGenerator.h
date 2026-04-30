#pragma once

#include "ast/AST.h"
#include "ir/IRModule.h"
#include "semantic/SymbolTable.h"

#include <iosfwd>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace maple {

class Module {
public:
    void clear();
    void addLine(std::string line);

    const std::vector<std::string>& lines() const;
    std::string toString() const;
    void print(std::ostream& os) const;

private:
    std::vector<std::string> lines_;
};

struct BackendConfig {
    std::string commandTemplate;
};

class IRGenerator {
public:
    const Module& generate(const Program& program);
    const Module& generate(const IRModule& irModule, const std::string& functionName = "main");

    const Module& module() const;
    void writeToFile(const std::string& path) const;

private:
    struct ExprResult {
        std::string text;
        ValueType type = ValueType::Unknown;
    };

    struct FunctionInfo {
        std::string name;
        ValueType returnType = ValueType::Void;
        std::vector<std::pair<std::string, ValueType>> parameters;
    };

    struct LocalBinding {
        std::string mapleName;
        ValueType type = ValueType::Unknown;
    };

    struct ScopeFrame {
        std::unordered_map<std::string, LocalBinding> bindings;
    };

    struct FunctionContext {
        std::vector<std::string> declarations;
        std::vector<std::string> body;
    };

    Module module_;
    FunctionContext functionContext_;
    std::vector<ScopeFrame> scopes_;
    std::unordered_map<std::string, FunctionInfo> functions_;
    std::vector<std::string> functionOrder_;
    std::unordered_map<std::string, int> localNameCounters_;
    int labelCounter_ = 0;
    int internalTempCounter_ = 0;
    ValueType currentReturnType_ = ValueType::Void;

    void reset();

    static std::string mapleType(ValueType type);
    static std::string functionSymbol(const std::string& name);
    static std::string localSymbol(const std::string& name);
    static std::string normalizeFloatLiteral(const std::string& token);
    static int decodeCharLiteral(const std::string& token);
    static std::string oneLiteral(ValueType type);

    std::string newLabel(const std::string& prefix);

    void emitDeclaration(std::string line);
    void emitBody(std::string line);
    void beginScope();
    void endScope();
    LocalBinding declareBinding(const std::string& sourceName, ValueType type, bool emitDeclaration = true);
    LocalBinding declareInternalTemp(ValueType type, const std::string& hint);
    const LocalBinding& lookupBinding(const std::string& sourceName) const;

    FunctionInfo buildFunctionInfo(const FunctionDecl& functionDecl) const;
    void registerFunctions(const Program& program);
    void emitProgramHeader(const std::string& entryFunction);
    void emitASTFunction(const FunctionDecl& functionDecl);
    void emitSyntheticTopLevelFunction(const std::vector<const Stmt*>& statements);

    void emitBlock(const BlockStmt& blockStmt);
    void emitStmt(const Stmt& stmt);
    void emitVarDecl(const VarDecl& varDecl);
    void emitReturn(const ReturnStmt& returnStmt);
    void emitIf(const IfStmt& ifStmt);
    void emitWhile(const WhileStmt& whileStmt);
    void emitFor(const ForStmt& forStmt);
    void emitFunctionCallStmt(const FunctionCall& functionCall);

    ExprResult emitExpr(const Expr& expr);
    ExprResult emitLiteral(const Literal& literal);
    ExprResult emitIdentifier(const Identifier& identifier);
    ExprResult emitAssignment(const AssignmentExpr& assignmentExpr);
    ExprResult emitUnary(const UnaryExpr& unaryExpr);
    ExprResult emitIncDec(const IncDecExpr& incDecExpr);
    ExprResult emitBinary(const BinaryExpr& binaryExpr);
    ExprResult emitFunctionCall(const FunctionCall& functionCall);
    ExprResult emitFunctionCallInto(const FunctionCall& functionCall, const LocalBinding& targetBinding);

    void emitIRFunction(const IRModule& irModule, const std::string& functionName);
    void ensureIRBindingDeclared(const std::string& token);
    ExprResult parseIRAtom(const std::string& token);
    ExprResult parseIRRhs(const std::string& rhs);
    ValueType inferIRReturnType(const IRModule& irModule) const;
};

std::string generate_abc(const Program& program, const std::string& outputPath, const BackendConfig& config = {});
std::string generate_abc(const IRModule& irModule, const std::string& outputPath, const BackendConfig& config = {});

} // namespace maple
