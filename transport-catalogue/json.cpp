#include "json.h"

using namespace std;

namespace json {

    namespace {

        Node LoadNode(istream& input);

        Node LoadArray(istream& input) {
            Array result;
            char c;
            for (; input >> c && c != ']';) {
                if (c != ',') {
                    input.putback(c);
                }
                result.push_back(LoadNode(input));
            }

            if (c != ']' && result.empty()) {
                throw ParsingError("Array parsing error"s);
            }

            return Node(move(result));
        }

        Node LoadNumber(std::istream& input) {
            string str_num;

            auto read_char = [&str_num, &input] {
                str_num += static_cast<char>(input.get());
                if (!input) {
                    throw ParsingError("Read number error"s);
                }
            };

            auto read_digits = [&input, read_char] {
                if (!isdigit(input.peek())) {
                    throw ParsingError("A digit is expected"s);
                }
                while (isdigit(input.peek())) {
                    read_char();
                }
            };

            if (input.peek() == '-') {
                read_char();
            }

            if (input.peek() == '0') {
                read_char();
            }
            else {
                read_digits();
            }

            bool is_int = true;

            if (input.peek() == '.') {
                read_char();
                read_digits();
                is_int = false;
            }

            if (int ch = input.peek(); ch == 'e' || ch == 'E') {
                read_char();
                if (ch = input.peek(); ch == '+' || ch == '-') {
                    read_char();
                }
                read_digits();
                is_int = false;
            }

            try {
                if (is_int) {
                    try {
                        return Node{ std::stoi(str_num) };
                    }
                    catch (...) {
                    }
                }
                return Node{ std::stod(str_num) };
            }
            catch (...) {
                throw ParsingError("Failed to convert "s + str_num + " to number"s);
            }
        }

        Node LoadString(std::istream& input) {
            auto it = istreambuf_iterator<char>(input);
            auto end = istreambuf_iterator<char>();
            string str;

            while (true) {
                if (it == end) {
                    throw ParsingError("Read string error");
                }

                const char ch = *it;
                if (ch == '"') {
                    ++it;
                    break;
                }
                else if (ch == '\\') {
                    ++it;
                    if (it == end) {
                        throw ParsingError("Read string error");
                    }
                    const char escaped_char = *(it);
                    switch (escaped_char) {
                    case 'n':
                        str.push_back('\n');
                        break;
                    case 't':
                        str.push_back('\t');
                        break;
                    case 'r':
                        str.push_back('\r');
                        break;
                    case '"':
                        str.push_back('"');
                        break;
                    case '\\':
                        str.push_back('\\');
                        break;
                    default:
                        throw ParsingError("Unrecognized escape sequence \\"s + escaped_char);
                    }
                }
                else if (ch == '\n' || ch == '\r') {
                    throw ParsingError("Unexpected end of line"s);
                }
                else {
                    str.push_back(ch);
                }
                ++it;
            }

            return Node{ std::move(str) };
        }

        Node LoadDict(istream& input) {
            Dict result;
            char c;
            for (; input >> c && c != '}';) {
                if (c == ',') {
                    input >> c;
                }

                string key = LoadString(input).AsString();
                input >> c;

                if (c != ':') {
                    throw ParsingError("Dict parsing error: absence of :"s);
                }

                result.insert({ move(key), LoadNode(input) });
            }

            if (c != '}') {
                throw ParsingError("Dict parsing error: absence of }"s);
            }

            return Node(move(result));
        }

        Node LoadNull(istream& input) {
            string str_null;
            for (char c; input >> c;) {
                if (!isalpha(c)) {
                    input.putback(c);
                    break;
                }
                str_null.push_back(c);
            }
            if (str_null != "null"s) {
                throw ParsingError("Null parsing error");
            }
            return Node{ nullptr };
        }

        Node LoadBool(istream& input) {
            string str_bool;
            for (char c; input >> c;) {
                if (!isalpha(c)) {
                    input.putback(c);
                    break;
                }
                str_bool.push_back(c);
            }
            if (str_bool == "true"s) {
                return Node{ true };
            }
            if (str_bool == "false"s) {
                return Node{ false };
            }
            throw ParsingError("Bool parsing error");
        }

        Node LoadNode(istream& input) {
            char c;
            input >> c;

            if (c == 'n') {
                input.putback(c);
                return LoadNull(input);
            }
            else if (c == 't' || c == 'f') {
                input.putback(c);
                return LoadBool(input);
            }
            else if (c == '[') {
                return LoadArray(input);
            }
            else if (c == '{') {
                return LoadDict(input);
            }
            else if (c == '"') {
                return LoadString(input);
            }
            else {
                input.putback(c);
                return LoadNumber(input);
            }
        }

    }  // namespace

    const Node::Value& Node::GetValue() const {
        return value_;
    }

    Node::Value& Node::GetValue() {
        return value_;
    }

    bool Node::IsInt() const {
        return holds_alternative<int>(value_);
    }

    bool Node::IsDouble() const {
        return holds_alternative<double>(value_) || holds_alternative<int>(value_);
    }

    bool Node::IsPureDouble() const {
        return holds_alternative<double>(value_);
    }

    bool Node::IsBool() const {
        return holds_alternative<bool>(value_);
    }

    bool Node::IsString() const {
        return holds_alternative<std::string>(value_);
    }

    bool Node::IsNull() const {
        return holds_alternative<std::nullptr_t>(value_);
    }

    bool Node::IsArray() const {
        return holds_alternative<Array>(value_);
    }

    bool Node::IsMap() const {
        return holds_alternative<Dict>(value_);
    }

    int Node::AsInt() const {
        if (const int* val = get_if<int>(&value_)) {
            int result = *val;
            return result;
        }
        throw std::logic_error("wrong type");
    }

    bool Node::AsBool() const {
        if (const bool* val = get_if<bool>(&value_)) {
            bool result = *val;
            return result;
        }
        throw std::logic_error("wrong type");
    }

    double Node::AsDouble() const {
        if (const int* val = get_if<int>(&value_)) {
            double result = *val;
            return result;
        }
        if (const double* val = get_if<double>(&value_)) {
            double result = *val;
            return result;
        }
        throw std::logic_error("wrong type");
    }

    const string& Node::AsString() const {
        if (const string* val = get_if<string>(&value_)) {
            return *val;
        }
        throw std::logic_error("wrong type");
    }

    const Array& Node::AsArray() const {
        if (const auto* val = get_if<Array>(&value_)) {
            return *val;
        }
        throw std::logic_error("wrong type");
    }

    const Dict& Node::AsMap() const {
        if (const auto* val = get_if<Dict>(&value_)) {
            return *val;
        }
        throw std::logic_error("wrong type");
    }

    Document::Document(Node root)
        : root_(move(root)) {
    }

    const Node& Document::GetRoot() const {
        return root_;
    }

    Document Load(istream& input) {
        return Document{ LoadNode(input) };
    }

    struct PrintContext {
        ostream& out;
        int indent_step = 4;
        int indent = 0;

        void PrintIndent() const {
            for (int i = 0; i < indent; ++i) {
                out.put(' ');
            }
        }

        PrintContext Indented() const {
            return { out, indent_step, indent_step + indent };
        }
    };

    void PrintNode(const Node& value, const PrintContext& ctx);

    template <typename Value>
    void PrintValue(const Value& value, const PrintContext& ctx) {
        ctx.out << value;
    }

    void PrintString(const string& value, ostream& out) {
        out.put('"');
        for (const char c : value) {
            switch (c) {
            case '\r':
                out << "\\r"sv;
                break;
            case '\n':
                out << "\\n"sv;
                break;
            case '\t':
                out << "\\t"sv;
                break;
            case '"':
                [[fallthrough]];
            case '\\':
                out.put('\\');
                [[fallthrough]];
            default:
                out.put(c);
                break;
            }
        }
        out.put('"');
    }

    template <>
    void PrintValue<string>(const string& value, const PrintContext& ctx) {
        PrintString(value, ctx.out);
    }

    template <>
    void PrintValue<nullptr_t>(const nullptr_t&, const PrintContext& ctx) {
        ctx.out << "null"sv;
    }

    template <>
    void PrintValue<bool>(const bool& value, const PrintContext& ctx) {
        ctx.out << (value ? "true"sv : "false"sv);
    }

    template <>
    void PrintValue<Array>(const Array& nodes, const PrintContext& ctx) {
        ostream& out = ctx.out;
        out << "[\n"sv;
        bool first = true;
        auto inner_ctx = ctx.Indented();
        for (const Node& node : nodes) {
            if (first) {
                first = false;
            }
            else {
                out << ",\n"sv;
            }
            inner_ctx.PrintIndent();
            PrintNode(node, inner_ctx);
        }
        out.put('\n');
        ctx.PrintIndent();
        out.put(']');
    }

    template <>
    void PrintValue<Dict>(const Dict& nodes, const PrintContext& ctx) {
        ostream& out = ctx.out;
        out << "{\n"sv;
        bool first = true;
        auto inner_ctx = ctx.Indented();
        for (const auto& [key, node] : nodes) {
            if (first) {
                first = false;
            }
            else {
                out << ",\n"sv;
            }
            inner_ctx.PrintIndent();
            PrintString(key, ctx.out);
            out << ": "sv;
            PrintNode(node, inner_ctx);
        }
        out.put('\n');
        ctx.PrintIndent();
        out.put('}');
    }

    void PrintNode(const Node& node, const PrintContext& ctx) {
        visit([&ctx](const auto& value) {
            PrintValue(value, ctx);
            },
            node.GetValue());
    }

    void Print(const Document& doc, std::ostream& output) {
        PrintNode(doc.GetRoot(), PrintContext{ output });
    }

}  // namespace json