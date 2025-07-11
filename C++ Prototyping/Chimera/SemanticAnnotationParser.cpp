#include "SemanticAnnotationParser.hpp" 
#include <iostream> // For errors 
#include <regex>    // For parsing (can be complex) 
#include <charconv> // For parsing numbers 
namespace chimera::frontend::dsl { 
// Basic parsing - needs significant improvement for robustness, handling quotes, escapes etc. 
std::optional<ParsedAnnotation> SemanticAnnotationParser::parseAnnotationString(const std::string& annotation_str) { 
    // Basic regex: @Name or @Name(arg1, arg2=val, ...) 
    static const std::regex annotation_regex(R"(@(\w+)(?:\s*\((.*)\))?)"); 
    std::smatch match; 
    if (std::regex_match(annotation_str, match, annotation_regex)) { 
        ParsedAnnotation parsed; 
        parsed.name = match[1].str(); 
        if (match[2].matched) { // Arguments exist 
            std::string args_str = match[2].str(); 
            // Very basic argument splitting by comma - fails with nested structures, strings with commas etc. 
             static const std::regex arg_split_regex(R"(\s*,\s*)"); 
             std::sregex_token_iterator iter(args_str.begin(), args_str.end(), arg_split_regex, -1); 
             std::sregex_token_iterator end; 
             for (; iter != end; ++iter) { 
                 std::string arg_part = iter->str(); 
                 if (!arg_part.empty()) { 
                    // TODO: Parse key=value pairs? For now, just parse as values 
                    auto value_opt = parseValue(arg_part); 
                    if(value_opt) { 
                        parsed.arguments.push_back(value_opt.value()); 
                    } else { 
                         std::cerr << "Annotation Parser Error: Failed to parse argument value '" << arg_part << "' in annotation @" << parsed
                         // Decide: return nullopt or push monostate? Push monostate for now. 
                         parsed.arguments.push_back(std::monostate{}); 
                         // return std::nullopt; // Strict parsing 
                    } 
                 } 
             } 
        } 
        // If no args matched, arguments vector remains empty (flag annotation) 
        return parsed; 
    } else { 
         std::cerr << "Annotation Parser Error: Invalid annotation syntax: " << annotation_str << std::endl; 
         return std::nullopt; 
    }
 } 
std::vector<ParsedAnnotation> SemanticAnnotationParser::parseAnnotationBlock(const std::string& block_str) { 
     std::vector<ParsedAnnotation> results;
     // Simple split by lines or known delimiters - depends on how blocks are passed 
     // Assuming one annotation per line for simplicity: 
     std::string line; 
     std::istringstream stream(block_str); 
     while(std::getline(stream, line)) { 
         // Trim whitespace? 
         if (line.empty() || line.find_first_not_of(" \t") == std::string::npos) continue; 
         if (line.rfind("@", 0) == 0) { // Starts with @ 
             auto parsed_opt = parseAnnotationString(line); 
             if (parsed_opt) { 
                 results.push_back(parsed_opt.value()); 
             } 
         }
     } 
     return results;
 } 
// Very basic value parsing 
std::optional<AnnotationValue> SemanticAnnotationParser::parseValue(const std::string& value_str) { 
    std::string trimmed_str = value_str; // TODO: Trim whitespace properly 
    // Try parsing as different types 
    // Bool 
    if (trimmed_str == "true") return AnnotationValue{true};
    if (trimmed_str == "false") return AnnotationValue{false}; 
    // Integer (int64_t) 
    int64_t i_val; 
    auto [ptr_i, ec_i] = std::from_chars(trimmed_str.data(), trimmed_str.data() + trimmed_str.size(), i_val); 
    if (ec_i == std::errc() && ptr_i == trimmed_str.data() + trimmed_str.size()) { 
        return AnnotationValue{i_val}; 
    }
    // Double 
    double d_val; 
    auto [ptr_d, ec_d] = std::from_chars(trimmed_str.data(), trimmed_str.data() + trimmed_str.size(), d_val); 
     if (ec_d == std::errc() && ptr_d == trimmed_str.data() + trimmed_str.size()) { 
        return AnnotationValue{d_val}; 
    }
    // BDIType (simple string match for now) 
    // TODO: Robust matching based on BDITypes.hpp enum names 
    if (trimmed_str == "INT32") return AnnotationValue{BDIType::INT32}; 
    if (trimmed_str == "FLOAT64") return AnnotationValue{BDIType::FLOAT64}; 
    // Add other types... 
    // String (assuming if nothing else matches, could check for quotes) 
    // This basic version treats unquoted identifiers as strings too. 
    // Proper parsing needs quote detection. 
    return AnnotationValue{trimmed_str}; 
    // return std::nullopt; // Failed to parse as any known type 
} 
} // namespace chimera::frontend::dsl 
