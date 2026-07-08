#include <game_req_analyzer/utils/json_utils.hpp>
#include <fstream>

namespace game_req {

Result<Json> JsonUtils::parse(StringView str) {
    try {
        return Json::parse(str);
    } catch (const std::exception& e) {
        return make_unexpected(MAKE_ERROR(ErrorCode::ParseError, e.what()));
    }
}

Result<Json> JsonUtils::parse_file(const Path& path) {
    std::ifstream file(path);
    if (!file) return make_unexpected(MAKE_ERROR(ErrorCode::FileNotFound, path.string()));
    try {
        return Json::parse(file);
    } catch (const std::exception& e) {
        return make_unexpected(MAKE_ERROR(ErrorCode::ParseError, e.what()));
    }
}

Result<void> JsonUtils::write_file(const Path& path, const Json& j, int indent) {
    std::ofstream file(path);
    if (!file) return make_unexpected(MAKE_ERROR(ErrorCode::IoError, "Failed to open file for writing"));
    file << j.dump(indent);
    return success();
}

Result<String> JsonUtils::to_string(const Json& j, int indent) {
    return j.dump(indent);
}

Result<Json> JsonUtils::merge(const Json& a, const Json& b) {
    Json result = a;
    result.merge_patch(b);
    return result;
}

Result<Json> JsonUtils::patch(const Json& target, const Json& patch) {
    Json result = target;
    result.merge_patch(patch);
    return result;
}

Result<Json> JsonUtils::diff(const Json& a, const Json& b) {
    return b.diff(a);
}

bool JsonUtils::has_key(const Json& j, StringView key) {
    return j.contains(key);
}

Result<const Json&> JsonUtils::get(const Json& j, StringView key) {
    auto it = j.find(key);
    if (it == j.end()) return make_unexpected(MAKE_ERROR(ErrorCode::InvalidArgument, "Key not found"));
    return *it;
}

Result<Json&> JsonUtils::get_mut(Json& j, StringView key) {
    auto it = j.find(key);
    if (it == j.end()) return make_unexpected(MAKE_ERROR(ErrorCode::InvalidArgument, "Key not found"));
    return *it;
}

void JsonUtils::set(Json& j, StringView key, const Json& value) {
    j[key] = value;
}

void JsonUtils::remove(Json& j, StringView key) {
    j.erase(key);
}

Result<std::vector<String>> JsonUtils::keys(const Json& j) {
    std::vector<String> result;
    for (auto& [key, val] : j.items()) result.push_back(key);
    return result;
}

Result<std::vector<Json>> JsonUtils::values(const Json& j) {
    std::vector<Json> result;
    for (auto& [key, val] : j.items()) result.push_back(val);
    return result;
}

Result<Json> JsonUtils::parse_lines(StringView str) {
    Json result = Json::array();
    String line;
    for (char c : str) {
        if (c == '\n') {
            if (!line.empty()) {
                try {
                    result.push_back(Json::parse(line));
                } catch (...) {}
                line.clear();
            }
        } else {
            line += c;
        }
    }
    return result;
}

Result<String> JsonUtils::to_lines(const Json& j) {
    return j.dump(4);
}

JsonUtils::SchemaValidator::SchemaValidator(const Json& s) : schema(s) {}

Result<void> JsonUtils::SchemaValidator::validate(const Json& j) const {
    // Simplified - would use JSON Schema validator
    return success();
}

Result<void> JsonUtils::SchemaValidator::validate_file(const Path& path) const {
    auto j = parse_file(path);
    if (!j) return make_unexpected(j.error());
    return validate(*j);
}

JsonUtils::SchemaValidator JsonUtils::load_schema(const Path& path) {
    auto j = parse_file(path);
    if (!j) return SchemaValidator({});
    return SchemaValidator(*j);
}

} // namespace game_req
