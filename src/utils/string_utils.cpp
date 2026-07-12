#include <game_req_analyzer/utils/string_utils.hpp>
#include <algorithm>
#include <cctype>
#include <sstream>
#include <iomanip>

namespace game_req {

String StringUtils::to_lower(StringView str) {
    String result(str);
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

String StringUtils::to_upper(StringView str) {
    String result(str);
    std::transform(result.begin(), result.end(), result.begin(), ::toupper);
    return result;
}

String StringUtils::trim(StringView str) {
    size_t start = 0;
    while (start < str.size() && std::isspace(static_cast<unsigned char>(str[start]))) start++;
    size_t end = str.size();
    while (end > start && std::isspace(static_cast<unsigned char>(str[end - 1]))) end--;
    return String(str.substr(start, end - start));
}

String StringUtils::trim_left(StringView str) {
    size_t start = 0;
    while (start < str.size() && std::isspace(static_cast<unsigned char>(str[start]))) start++;
    return String(str.substr(start));
}

String StringUtils::trim_right(StringView str) {
    size_t end = str.size();
    while (end > 0 && std::isspace(static_cast<unsigned char>(str[end - 1]))) end--;
    return String(str.substr(0, end));
}

std::vector<String> StringUtils::split(StringView str, StringView delimiter) {
    std::vector<String> result;
    size_t start = 0;
    size_t pos = str.find(delimiter);
    while (pos != String::npos) {
        result.emplace_back(str.substr(start, pos - start));
        start = pos + delimiter.size();
        pos = str.find(delimiter, start);
    }
    result.emplace_back(str.substr(start));
    return result;
}

std::vector<String> StringUtils::split_lines(StringView str) {
    return split(str, "\n");
}

std::vector<String> StringUtils::split_words(StringView str) {
    std::vector<String> result;
    String word;
    for (char c : str) {
        if (std::isspace(static_cast<unsigned char>(c))) {
            if (!word.empty()) {
                result.push_back(word);
                word.clear();
            }
        } else {
            word += c;
        }
    }
    if (!word.empty()) result.push_back(word);
    return result;
}

String StringUtils::join(const std::vector<String>& parts, StringView delimiter) {
    if (parts.empty()) return "";
    String result = parts[0];
    for (size_t i = 1; i < parts.size(); ++i) {
        result += delimiter;
        result += parts[i];
    }
    return result;
}

String StringUtils::join(std::span<const StringView> parts, StringView delimiter) {
    if (parts.empty()) return "";
    String result(parts[0]);
    for (size_t i = 1; i < parts.size(); ++i) {
        result += delimiter;
        result += parts[i];
    }
    return result;
}

bool StringUtils::starts_with(StringView str, StringView prefix) {
    return str.size() >= prefix.size() && str.compare(0, prefix.size(), prefix) == 0;
}

bool StringUtils::ends_with(StringView str, StringView suffix) {
    return str.size() >= suffix.size() && str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

bool StringUtils::contains(StringView str, StringView substr) {
    return str.find(substr) != String::npos;
}

bool StringUtils::equals_ignore_case(StringView a, StringView b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) != std::tolower(static_cast<unsigned char>(b[i]))) {
            return false;
        }
    }
    return true;
}

bool StringUtils::starts_with_ignore_case(StringView str, StringView prefix) {
    if (str.size() < prefix.size()) return false;
    for (size_t i = 0; i < prefix.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(str[i])) != std::tolower(static_cast<unsigned char>(prefix[i]))) {
            return false;
        }
    }
    return true;
}

bool StringUtils::ends_with_ignore_case(StringView str, StringView suffix) {
    if (str.size() < suffix.size()) return false;
    size_t offset = str.size() - suffix.size();
    for (size_t i = 0; i < suffix.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(str[offset + i])) != std::tolower(static_cast<unsigned char>(suffix[i]))) {
            return false;
        }
    }
    return true;
}

String StringUtils::replace(StringView str, StringView from, StringView to) {
    String result(str);
    size_t pos = result.find(from);
    if (pos != String::npos) {
        result.replace(pos, from.size(), to);
    }
    return result;
}

String StringUtils::replace_all(StringView str, StringView from, StringView to) {
    String result(str);
    size_t pos = 0;
    while ((pos = result.find(from, pos)) != String::npos) {
        result.replace(pos, from.size(), to);
        pos += to.size();
    }
    return result;
}

String StringUtils::remove(StringView str, StringView substr) {
    String result(str);
    size_t pos = 0;
    while ((pos = result.find(substr, pos)) != String::npos) {
        result.erase(pos, substr.size());
    }
    return result;
}

String StringUtils::pad_left(StringView str, u32 width, char pad) {
    if (str.size() >= width) return String(str);
    return String(width - str.size(), pad) + str;
}

String StringUtils::pad_right(StringView str, u32 width, char pad) {
    if (str.size() >= width) return String(str);
    return String(str) + String(width - str.size(), pad);
}

String StringUtils::center(StringView str, u32 width, char pad) {
    if (str.size() >= width) return String(str);
    u32 pad_total = width - str.size();
    u32 pad_left = pad_total / 2;
    u32 pad_right = pad_total - pad_left;
    return String(pad_left, pad) + str + String(pad_right, pad);
}

String StringUtils::repeat(StringView str, u32 count) {
    String result;
    result.reserve(str.size() * count);
    for (u32 i = 0; i < count; ++i) result += str;
    return result;
}

Result<i64> StringUtils::to_int(StringView str, i64 default_value) {
    try {
        return std::stoll(String(str));
    } catch (...) {
        return default_value;
    }
}

Result<u64> StringUtils::to_uint(StringView str, u64 default_value) {
    try {
        return std::stoull(String(str));
    } catch (...) {
        return default_value;
    }
}

Result<f64> StringUtils::to_float(StringView str, f64 default_value) {
    try {
        return std::stod(String(str));
    } catch (...) {
        return default_value;
    }
}

Result<bool> StringUtils::to_bool(StringView str, bool default_value) {
    String lower = to_lower(str);
    if (lower == "true" || lower == "1" || lower == "yes" || lower == "on") return true;
    if (lower == "false" || lower == "0" || lower == "no" || lower == "off") return false;
    return default_value;
}

String StringUtils::from_int(i64 value) { return std::to_string(value); }
String StringUtils::from_uint(u64 value) { return std::to_string(value); }
String StringUtils::from_float(f64 value, u32 precision) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(precision) << value;
    return oss.str();
}
String StringUtils::from_bool(bool value) { return value ? "true" : "false"; }

String StringUtils::format_bytes(u64 bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB", "PB"};
    int i = 0;
    double val = static_cast<double>(bytes);
    while (val >= 1024 && i < 5) {
        val /= 1024;
        i++;
    }
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << val << " " << units[i];
    return oss.str();
}

String StringUtils::format_number(u64 num) {
    String str = std::to_string(num);
    String result;
    for (size_t i = 0; i < str.size(); ++i) {
        if (i > 0 && (str.size() - i) % 3 == 0) result += ',';
        result += str[i];
    }
    return result;
}

String StringUtils::format_duration(Duration d) {
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(d).count();
    if (ms < 1000) return StringUtils::from_int(ms) + "ms";
    auto s = ms / 1000;
    if (s < 60) return StringUtils::from_int(s) + "s";
    auto m = s / 60;
    if (m < 60) return StringUtils::from_int(m) + "m" + StringUtils::from_int(s % 60) + "s";
    auto h = m / 60;
    return StringUtils::from_int(h) + "h" + StringUtils::from_int(m % 60) + "m";
}

String StringUtils::format_percentage(f64 value, u32 decimals) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(decimals) << (value * 100) << "%";
    return oss.str();
}

u32 StringUtils::levenshtein_distance(StringView a, StringView b) {
    if (a.empty()) return b.size();
    if (b.empty()) return a.size();
    
    std::vector<u32> prev(b.size() + 1), curr(b.size() + 1);
    for (u32 j = 0; j <= b.size(); ++j) prev[j] = j;
    
    for (size_t i = 1; i <= a.size(); ++i) {
        curr[0] = i;
        for (size_t j = 1; j <= b.size(); ++j) {
            u32 cost = (a[i - 1] == b[j - 1]) ? 0 : 1;
            curr[j] = std::min({curr[j - 1] + 1, prev[j] + 1, prev[j - 1] + cost});
        }
        prev.swap(curr);
    }
    return prev[b.size()];
}

f32 StringUtils::similarity(StringView a, StringView b) {
    u32 dist = levenshtein_distance(a, b);
    u32 max_len = std::max(a.size(), b.size());
    return max_len == 0 ? 1.0f : 1.0f - static_cast<f32>(dist) / max_len;
}

String StringUtils::escape_json(StringView str) {
    String result;
    result.reserve(str.size() * 2);
    for (char c : str) {
        switch (c) {
            case '"': result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\b': result += "\\b"; break;
            case '\f': result += "\\f"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    std::ostringstream oss;
                    oss << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(c);
                    result += oss.str();
                } else {
                    result += c;
                }
        }
    }
    return result;
}

String StringUtils::unescape_json(StringView str) {
    String result;
    for (size_t i = 0; i < str.size(); ++i) {
        if (str[i] == '\\' && i + 1 < str.size()) {
            char next = str[++i];
            switch (next) {
                case '"': result += '"'; break;
                case '\\': result += '\\'; break;
                case '/': result += '/'; break;
                case 'b': result += '\b'; break;
                case 'f': result += '\f'; break;
                case 'n': result += '\n'; break;
                case 'r': result += '\r'; break;
                case 't': result += '\t'; break;
                case 'u': {
                    if (i + 4 < str.size()) {
                        std::string hex = str.substr(i + 1, 4).data();
                        try {
                            int code = std::stoi(hex, nullptr, 16);
                            result += static_cast<char>(code);
                            i += 4;
                        } catch (...) { result += str[i]; }
                    } else { result += '\\'; }
                    break;
                }
                default: result += next; break;
            }
        } else {
            result += str[i];
        }
    }
    return result;
}

String StringUtils::escape_html(StringView str) {
    String result;
    result.reserve(str.size() * 2);
    for (char c : str) {
        switch (c) {
            case '&': result += "&"; break;
            case '<': result += "<"; break;
            case '>': result += ">"; break;
            case '"': result += """; break;
            case '\'': result += "'"; break;
            default: result += c; break;
        }
    }
    return result;
}

String StringUtils::unescape_html(StringView str) {
    String result;
    for (size_t i = 0; i < str.size(); ++i) {
        if (str[i] == '&' && i + 1 < str.size()) {
            size_t end = str.find(';', i);
            if (end != String::npos) {
                String entity = str.substr(i + 1, end - i - 1);
                if (entity == "amp") result += '&';
                else if (entity == "lt") result += '<';
                else if (entity == "gt") result += '>';
                else if (entity == "quot") result += '"';
                else if (entity == "apos" || entity == "#39") result += '\'';
                else result += str[i];
                i = end;
            } else {
                result += str[i];
            }
        } else {
            result += str[i];
        }
    }
    return result;
}

String StringUtils::escape_regex(StringView str) {
    static const String special = ".^$*+?{}[]\\|()";
    String result;
    result.reserve(str.size() * 2);
    for (char c : str) {
        if (special.find(c) != String::npos) result += '\\';
        result += c;
    }
    return result;
}

std::vector<String> StringUtils::tokenize(StringView str, StringView delimiters) {
    std::vector<String> tokens;
    String token;
    for (char c : str) {
        if (delimiters.find(c) != String::npos) {
            if (!token.empty()) {
                tokens.push_back(token);
                token.clear();
            }
        } else {
            token += c;
        }
    }
    if (!token.empty()) tokens.push_back(token);
    return tokens;
}

String StringUtils::detokenize(const std::vector<String>& tokens, StringView delimiter) {
    return join(tokens, delimiter);
}

String StringUtils::indent(StringView str, u32 spaces) {
    String indent_str(spaces, ' ');
    String result;
    std::istringstream iss(String(str));
    String line;
    bool first = true;
    while (std::getline(iss, line)) {
        if (!first) result += '\n';
        result += indent_str + line;
        first = false;
    }
    return result;
}

String StringUtils::dedent(StringView str) {
    std::istringstream iss(String(str));
    String line;
    std::vector<String> lines;
    while (std::getline(iss, line)) lines.push_back(line);
    
    u32 min_indent = UINT32_MAX;
    for (const auto& l : lines) {
        if (l.empty()) continue;
        u32 indent = 0;
        for (char c : l) {
            if (c == ' ') indent++;
            else if (c == '\t') indent += 4;
            else break;
        }
        min_indent = std::min(min_indent, indent);
    }
    
    String result;
    for (size_t i = 0; i < lines.size(); ++i) {
        if (i > 0) result += '\n';
        if (lines[i].size() >= min_indent) result += lines[i].substr(min_indent);
        else result += lines[i];
    }
    return result;
}

String StringUtils::wrap(StringView str, u32 width, StringView indent) {
    std::istringstream iss(String(str));
    String word;
    String result;
    u32 current_width = indent.size();
    bool first_word = true;
    
    while (iss >> word) {
        if (!first_word) {
            if (current_width + 1 + word.size() > width) {
                result += '\n' + indent;
                current_width = indent.size();
            } else {
                result += ' ';
                current_width++;
            }
        } else {
            result += indent;
            current_width = indent.size();
            first_word = false;
        }
        result += word;
        current_width += word.size();
    }
    return result;
}

StringUtils::Tokenizer::Tokenizer(StringView d, StringView delim) : data(d), delimiters(delim) {}

bool StringUtils::Tokenizer::has_next() const {
    return pos < data.size();
}

StringView StringUtils::Tokenizer::next() {
    while (pos < data.size() && delimiters.find(data[pos]) != String::npos) pos++;
    size_t start = pos;
    while (pos < data.size() && delimiters.find(data[pos]) == String::npos) pos++;
    return data.substr(start, pos - start);
}

StringView StringUtils::Tokenizer::peek() {
    size_t saved_pos = pos;
    StringView token = next();
    pos = saved_pos;
    return token;
}

void StringUtils::Tokenizer::skip_delimiters() {
    while (pos < data.size() && delimiters.find(data[pos]) != String::npos) pos++;
}

bool StringUtils::WildcardMatcher::match(StringView pattern, StringView str, bool case_sensitive) {
    size_t p = 0, s = 0;
    size_t star_idx = String::npos, match_idx = 0;
    
    while (s < str.size()) {
        if (p < pattern.size() && (pattern[p] == str[s] || (pattern[p] == '?' && str[s] != '\0'))) {
            ++p; ++s;
        } else if (p < pattern.size() && pattern[p] == '*') {
            star_idx = p++;
            match_idx = s;
        } else if (star_idx != String::npos) {
            p = star_idx + 1;
            match_idx++;
            s = match_idx;
        } else {
            return false;
        }
    }
    
    while (p < pattern.size() && pattern[p] == '*') ++p;
    return p == pattern.size();
}

bool StringUtils::WildcardMatcher::match_any(StringView pattern, std::span<const StringView> strings, bool case_sensitive) {
    for (StringView s : strings) {
        if (match(pattern, s, case_sensitive)) return true;
    }
    return false;
}

std::vector<String> StringUtils::WildcardMatcher::filter(const std::vector<String>& strings, StringView pattern, bool case_sensitive) {
    std::vector<String> result;
    for (const auto& s : strings) {
        if (match(pattern, s, case_sensitive)) result.push_back(s);
    }
    return result;
}

} // namespace game_req

Result<String> StringUtils::read_text_file(const Path& path) {
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file) {
        return make_unexpected(MAKE_ERROR(ErrorCode::IoError, std::format("Cannot open file: {}", path.string())));
    }
    
    std::string content;
    file.seekg(0, std::ios::end);
    content.reserve(file.tellg());
    file.seekg(0, std::ios::beg);
    content.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    
    if (!file) {
        return make_unexpected(MAKE_ERROR(ErrorCode::IoError, std::format("Failed to read file: {}", path.string())));
    }
    
    return content;
}

} // namespace game_req
