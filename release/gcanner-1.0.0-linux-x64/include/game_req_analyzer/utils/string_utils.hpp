#pragma once

#include <game_req_analyzer/core/pch.hpp>

namespace game_req {

class StringUtils {
public:
    static String to_lower(StringView str);
    static String to_upper(StringView str);
    static String trim(StringView str);
    static String trim_left(StringView str);
    static String trim_right(StringView str);
    
    static std::vector<String> split(StringView str, StringView delimiter);
    static std::vector<String> split_lines(StringView str);
    static std::vector<String> split_words(StringView str);
    
    static String join(const std::vector<String>& parts, StringView delimiter);
    static String join(std::span<const StringView> parts, StringView delimiter);
    
    static bool starts_with(StringView str, StringView prefix);
    static bool ends_with(StringView str, StringView suffix);
    static bool contains(StringView str, StringView substr);
    static bool equals_ignore_case(StringView a, StringView b);
    static bool starts_with_ignore_case(StringView str, StringView prefix);
    static bool ends_with_ignore_case(StringView str, StringView suffix);
    
    static String replace(StringView str, StringView from, StringView to);
    static String replace_all(StringView str, StringView from, StringView to);
    static String remove(StringView str, StringView substr);
    
    static String pad_left(StringView str, u32 width, char pad = ' ');
    static String pad_right(StringView str, u32 width, char pad = ' ');
    static String center(StringView str, u32 width, char pad = ' ');
    
    static String repeat(StringView str, u32 count);
    
    static Result<i64> to_int(StringView str, i64 default_value = 0);
    static Result<u64> to_uint(StringView str, u64 default_value = 0);
    static Result<f64> to_float(StringView str, f64 default_value = 0.0);
    static Result<bool> to_bool(StringView str, bool default_value = false);
    
    static String from_int(i64 value);
    static String from_uint(u64 value);
    static String from_float(f64 value, u32 precision = 6);
    static String from_bool(bool value);
    
    static String format_bytes(u64 bytes);
    static String format_number(u64 num);
    static String format_duration(Duration d);
    static String format_percentage(f64 value, u32 decimals = 1);
    
    static u32 levenshtein_distance(StringView a, StringView b);
    static f32 similarity(StringView a, StringView b);
    
    static String escape_json(StringView str);
    static String unescape_json(StringView str);
    static String escape_html(StringView str);
    static String unescape_html(StringView str);
static String escape_regex(StringView str);
    
    static Result<String> read_text_file(const Path& path);

    static std::vector<String> tokenize(StringView str, StringView delimiters = " \t\n\r");
    static String detokenize(const std::vector<String>& tokens, StringView delimiter = " ");
    
    static String indent(StringView str, u32 spaces = 4);
    static String dedent(StringView str);
    
    static String wrap(StringView str, u32 width, StringView indent = "");

    struct Tokenizer {
        StringView data;
        StringView delimiters;
        u32 pos = 0;
        
        Tokenizer(StringView d, StringView delim = " \t\n\r") : data(d), delimiters(delim) {}
        
        bool has_next() const { return pos < data.size(); }
        StringView next();
        StringView peek();
        void skip_delimiters();
    };
};

class WildcardMatcher {
public:
    static bool match(StringView pattern, StringView str, bool case_sensitive = true);
    static bool match_any(StringView pattern, std::span<const StringView> strings, bool case_sensitive = true);
    
    static std::vector<String> filter(const std::vector<String>& strings, StringView pattern, bool case_sensitive = true);
};

} // namespace game_req