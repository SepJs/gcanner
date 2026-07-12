#pragma once
#include <game_req_analyzer/core/error.hpp>
#include <game_req_analyzer/core/error.hpp>

#include <game_req_analyzer/core/pch.hpp>
#include <game_req_analyzer/core/error.hpp>

namespace game_req {

class JsonUtils {
public:
    static Result<Json> parse(StringView str);
    static Result<Json> parse_file(const Path& path);
    static Result<void> write_file(const Path& path, const Json& j, int indent = 4);
    static Result<String> to_string(const Json& j, int indent = -1);
    
    template<typename T>
    static Result<T> from_json(const Json& j) {
        try {
            return j.get<T>();
        } catch (const std::exception& e) {
            return make_unexpected(MAKE_ERROR(ErrorCode::ParseError, 
                std::format("JSON conversion failed: {}", e.what())));
        }
    }
    
    template<typename T>
    static Json to_json(const T& value) {
        return Json(value);
    }
    
    static Result<Json> merge(const Json& a, const Json& b);
    static Result<Json> patch(const Json& target, const Json& patch);
    static Result<Json> diff(const Json& a, const Json& b);
    
    static bool has_key(const Json& j, StringView key);
    static Result<const Json&> get(const Json& j, StringView key);
    static Result<Json&> get_mut(Json& j, StringView key);
    static void set(Json& j, StringView key, const Json& value);
    static void remove(Json& j, StringView key);
    
    static Result<std::vector<String>> keys(const Json& j);
    static Result<std::vector<Json>> values(const Json& j);
    
    static Result<Json> parse_lines(StringView str);
    static Result<String> to_lines(const Json& j);
    
    struct SchemaValidator {
        Json schema;
        
        SchemaValidator(const Json& s) : schema(s) {}
        Result<void> validate(const Json& j) const;
        Result<void> validate_file(const Path& path) const;
    };
    
    static SchemaValidator load_schema(const Path& path);
};

} // namespace game_req