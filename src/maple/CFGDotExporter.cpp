#include "maple/CFGDotExporter.h"

#include <cstddef>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace maple {

namespace {

struct BasicBlock {
    std::string name;
    std::vector<std::string> labels;
    std::vector<std::string> instructions;
};

struct Edge {
    std::size_t from = 0;
    std::size_t to = 0;
    std::string kind;
};

struct FunctionCFG {
    std::string name;
    std::vector<BasicBlock> blocks;
    std::unordered_map<std::string, std::size_t> labelToBlock;
    std::vector<Edge> edges;
};

std::string trim(const std::string& text) {
    const std::size_t first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return "";
    }

    const std::size_t last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
}

bool startsWith(const std::string& text, const std::string& prefix) {
    return text.rfind(prefix, 0) == 0;
}

std::string escapeDot(const std::string& text) {
    std::string escaped;
    escaped.reserve(text.size());

    for (char ch : text) {
        switch (ch) {
        case '\\':
            escaped += "\\\\";
            break;
        case '"':
            escaped += "\\\"";
            break;
        default:
            escaped.push_back(ch);
            break;
        }
    }

    return escaped;
}

std::string join(const std::vector<std::string>& parts, const std::string& separator) {
    std::ostringstream oss;

    for (std::size_t index = 0; index < parts.size(); ++index) {
        if (index != 0) {
            oss << separator;
        }
        oss << parts[index];
    }

    return oss.str();
}

std::string parseFunctionName(const std::string& line) {
    const std::string prefix = "func ";
    if (!startsWith(line, prefix)) {
        throw std::runtime_error("CFG generation error: invalid function header '" + line + "'");
    }

    const std::size_t nameStart = prefix.size();
    const std::size_t nameEnd = line.find(' ', nameStart);
    if (nameEnd == std::string::npos) {
        throw std::runtime_error("CFG generation error: cannot parse function name from '" + line + "'");
    }

    return line.substr(nameStart, nameEnd - nameStart);
}

bool isEmpty(const BasicBlock& block) {
    return block.name.empty() && block.labels.empty() && block.instructions.empty();
}

class FunctionParser {
public:
    explicit FunctionParser(std::string name) {
        cfg_.name = std::move(name);
    }

    void addLine(const std::string& rawLine) {
        const std::string line = trim(rawLine);
        if (line.empty() || startsWith(line, "var ")) {
            return;
        }

        if (startsWith(line, "# BB ")) {
            startNamedBlock(line.substr(5));
            return;
        }

        if (!line.empty() && line.front() == '@') {
            addLabel(line);
            return;
        }

        current_.instructions.push_back(line);
    }

    FunctionCFG finish() {
        flushCurrent();
        buildEdges();
        return std::move(cfg_);
    }

private:
    void startNamedBlock(const std::string& name) {
        if (current_.instructions.empty() && current_.name.empty()) {
            current_.name = name;
            return;
        }

        flushCurrent();
        current_.name = name;
    }

    void addLabel(const std::string& label) {
        if (!current_.instructions.empty()) {
            flushCurrent();
        }

        current_.labels.push_back(label);
    }

    void flushCurrent() {
        if (isEmpty(current_)) {
            return;
        }

        const std::size_t blockIndex = cfg_.blocks.size();
        for (const std::string& label : current_.labels) {
            cfg_.labelToBlock[label] = blockIndex;
        }

        cfg_.blocks.push_back(std::move(current_));
        current_ = BasicBlock{};
    }

    std::size_t blockIndexForLabel(const std::string& label) const {
        const auto found = cfg_.labelToBlock.find(label);
        if (found == cfg_.labelToBlock.end()) {
            throw std::runtime_error("CFG generation error: unknown label '" + label + "' in function " + cfg_.name);
        }

        return found->second;
    }

    static std::string parseBranchTarget(const std::string& instruction, const std::string& opcode) {
        const std::size_t start = opcode.size();
        const std::size_t end = instruction.find(' ', start);
        if (end == std::string::npos) {
            throw std::runtime_error("CFG generation error: cannot parse branch target from '" + instruction + "'");
        }

        return instruction.substr(start, end - start);
    }

    static std::string gotoTarget(const std::string& instruction) {
        return trim(instruction.substr(5));
    }

    void addEdge(std::size_t from, std::size_t to, std::string kind) {
        cfg_.edges.push_back(Edge{from, to, std::move(kind)});
    }

    void buildEdges() {
        for (std::size_t blockIndex = 0; blockIndex < cfg_.blocks.size(); ++blockIndex) {
            const BasicBlock& block = cfg_.blocks[blockIndex];
            const bool hasNext = blockIndex + 1 < cfg_.blocks.size();

            std::size_t conditionalIndex = block.instructions.size();
            bool isBrfalse = false;
            for (std::size_t instructionIndex = 0; instructionIndex < block.instructions.size(); ++instructionIndex) {
                const std::string& instruction = block.instructions[instructionIndex];
                if (startsWith(instruction, "brfalse ")) {
                    conditionalIndex = instructionIndex;
                    isBrfalse = true;
                    break;
                }
                if (startsWith(instruction, "brtrue ")) {
                    conditionalIndex = instructionIndex;
                    isBrfalse = false;
                    break;
                }
            }

            if (conditionalIndex != block.instructions.size()) {
                const std::string& conditionalInstruction = block.instructions[conditionalIndex];
                const std::string conditionalOpcode = isBrfalse ? "brfalse " : "brtrue ";
                const std::string branchTarget = parseBranchTarget(conditionalInstruction, conditionalOpcode);
                addEdge(
                    blockIndex,
                    blockIndexForLabel(branchTarget),
                    isBrfalse ? "false branch" : "true branch");

                bool explicitComplementBranch = false;
                for (std::size_t instructionIndex = conditionalIndex + 1; instructionIndex < block.instructions.size(); ++instructionIndex) {
                    const std::string& instruction = block.instructions[instructionIndex];
                    if (!startsWith(instruction, "goto ")) {
                        continue;
                    }

                    addEdge(
                        blockIndex,
                        blockIndexForLabel(gotoTarget(instruction)),
                        isBrfalse ? "true branch" : "false branch");
                    explicitComplementBranch = true;
                    break;
                }

                if (!explicitComplementBranch && hasNext) {
                    addEdge(blockIndex, blockIndex + 1, isBrfalse ? "true branch" : "false branch");
                }

                continue;
            }

            bool sawReturn = false;
            for (const std::string& instruction : block.instructions) {
                if (startsWith(instruction, "return")) {
                    sawReturn = true;
                    break;
                }
            }
            if (sawReturn) {
                continue;
            }

            bool handledGoto = false;
            for (const std::string& instruction : block.instructions) {
                if (!startsWith(instruction, "goto ")) {
                    continue;
                }

                const std::size_t targetIndex = blockIndexForLabel(gotoTarget(instruction));
                addEdge(
                    blockIndex,
                    targetIndex,
                    targetIndex <= blockIndex ? "loop back edge" : "goto");
                handledGoto = true;
                break;
            }
            if (handledGoto) {
                continue;
            }

            if (hasNext) {
                addEdge(blockIndex, blockIndex + 1, "fallthrough");
            }
        }
    }

    FunctionCFG cfg_;
    BasicBlock current_;
};

std::vector<FunctionCFG> buildCFGs(const Module& module) {
    std::vector<FunctionCFG> functions;

    bool inFunction = false;
    FunctionParser parser("");

    for (const std::string& rawLine : module.lines()) {
        const std::string line = trim(rawLine);
        if (line.empty()) {
            continue;
        }

        if (!inFunction) {
            if (startsWith(line, "func ")) {
                parser = FunctionParser(parseFunctionName(line));
                inFunction = true;
            }
            continue;
        }

        if (line == "}") {
            functions.push_back(parser.finish());
            inFunction = false;
            continue;
        }

        parser.addLine(line);
    }

    return functions;
}

std::string blockTitle(const BasicBlock& block, std::size_t index) {
    if (!block.name.empty() && !block.labels.empty()) {
        return block.name + " (" + join(block.labels, ", ") + ")";
    }
    if (!block.name.empty()) {
        return block.name;
    }
    if (!block.labels.empty()) {
        return join(block.labels, ", ");
    }

    return "bb" + std::to_string(index);
}

std::string blockLabel(const BasicBlock& block, std::size_t index) {
    std::ostringstream oss;
    oss << escapeDot(blockTitle(block, index)) << "\\l";

    for (const std::string& instruction : block.instructions) {
        oss << escapeDot(instruction) << "\\l";
    }

    return oss.str();
}

std::string edgeColor(const std::string& kind) {
    if (kind == "true branch") {
        return "forestgreen";
    }
    if (kind == "false branch") {
        return "firebrick";
    }
    if (kind == "loop back edge") {
        return "royalblue";
    }
    if (kind == "fallthrough") {
        return "black";
    }

    return "gray40";
}

} // namespace

void writeCFGDot(const Module& module, std::ostream& os) {
    const std::vector<FunctionCFG> functions = buildCFGs(module);

    os << "digraph CFG {\n";
    os << "  rankdir=TB;\n";
    os << "  compound=true;\n";
    os << "  node [shape=box, style=\"rounded\", fontname=\"Courier\"];\n";
    os << "  edge [fontname=\"Courier\"];\n";

    for (std::size_t functionIndex = 0; functionIndex < functions.size(); ++functionIndex) {
        const FunctionCFG& function = functions[functionIndex];
        os << "  subgraph cluster_" << functionIndex << " {\n";
        os << "    label=\"" << escapeDot("Function " + function.name) << "\";\n";
        os << "    color=\"gray70\";\n";

        for (std::size_t blockIndex = 0; blockIndex < function.blocks.size(); ++blockIndex) {
            os << "    f" << functionIndex << "_b" << blockIndex
               << " [label=\"" << blockLabel(function.blocks[blockIndex], blockIndex) << "\"];\n";
        }

        os << "  }\n";
    }

    for (std::size_t functionIndex = 0; functionIndex < functions.size(); ++functionIndex) {
        const FunctionCFG& function = functions[functionIndex];
        for (const Edge& edge : function.edges) {
            os << "  f" << functionIndex << "_b" << edge.from
               << " -> f" << functionIndex << "_b" << edge.to
               << " [label=\"" << escapeDot(edge.kind) << "\", color=\"" << edgeColor(edge.kind) << "\"];\n";
        }
    }

    os << "}\n";
}

} // namespace maple
