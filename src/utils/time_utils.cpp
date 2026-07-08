#include <game_req_analyzer/utils/time_utils.hpp>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <ctime>

namespace game_req {

TimePoint TimeUtils::now() {
    return std::chrono::system_clock::now();
}

TimePoint TimeUtils::now_utc() {
    return std::chrono::system_clock::now();
}

u64 TimeUtils::timestamp_ms() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

u64 TimeUtils::timestamp_us() {
    return std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

u64 TimeUtils::timestamp_ns() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

u64 TimeUtils::timestamp_s() {
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

String TimeUtils::format(TimePoint tp, StringView format) {
    auto time_t = std::chrono::system_clock::to_time_t(tp);
    std::tm tm = *std::localtime(&time_t);
    std::ostringstream oss;
    oss << std::put_time(&tm, format.data());
    return oss.str();
}

String TimeUtils::format_utc(TimePoint tp, StringView format) {
    auto time_t = std::chrono::system_clock::to_time_t(tp);
    std::tm tm = *std::gmtime(&time_t);
    std::ostringstream oss;
    oss << std::put_time(&tm, format.data());
    return oss.str();
}

String TimeUtils::format_iso8601(TimePoint tp) {
    return format(tp, "%Y-%m-%dT%H:%M:%SZ");
}

String TimeUtils::format_rfc3339(TimePoint tp) {
    return format(tp, "%Y-%m-%dT%H:%M:%S%z");
}

Result<TimePoint> TimeUtils::parse(StringView str, StringView format) {
    std::tm tm = {};
    std::istringstream ss(String(str));
    ss >> std::get_time(&tm, format.data());
    if (ss.fail()) return make_unexpected(MAKE_ERROR(ErrorCode::ParseError, "Failed to parse time"));
    return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

Result<TimePoint> TimeUtils::parse_iso8601(StringView str) {
    return parse(str, "%Y-%m-%dT%H:%M:%SZ");
}

Result<TimePoint> TimeUtils::parse_rfc3339(StringView str) {
    return parse(str, "%Y-%m-%dT%H:%M:%S%z");
}

Duration TimeUtils::since(TimePoint tp) {
    return now() - tp;
}

Duration TimeUtils::until(TimePoint tp) {
    return tp - now();
}

String TimeUtils::format_duration(Duration d) {
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(d).count();
    if (ms < 1000) return StringUtils::from_int(ms) + "ms";
    auto s = ms / 1000;
    if (s < 60) return StringUtils::from_int(s) + "s";
    auto m = s / 60;
    if (m < 60) return StringUtils::from_int(m) + "m" + StringUtils::from_int(s % 60) + "s";
    auto h = m / 60;
    return StringUtils::from_int(h) + "h" + StringUtils::from_int(m % 60) + "m";
}

String TimeUtils::format_duration_short(Duration d) {
    auto s = std::chrono::duration_cast<std::chrono::seconds>(d).count();
    if (s < 60) return StringUtils::from_int(s) + "s";
    auto m = s / 60;
    if (m < 60) return StringUtils::from_int(m) + "m";
    auto h = m / 60;
    if (h < 24) return StringUtils::from_int(h) + "h";
    auto d = h / 24;
    return StringUtils::from_int(d) + "d";
}

String TimeUtils::format_duration_verbose(Duration d) {
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(d).count();
    auto s = ms / 1000;
    auto m = s / 60;
    auto h = m / 60;
    auto d = h / 24;
    
    std::ostringstream oss;
    if (d > 0) oss << d << "d ";
    if (h > 0) oss << (h % 24) << "h ";
    if (m > 0) oss << (m % 60) << "m ";
    if (s > 0 || ms < 1000) oss << (s % 60) << "s ";
    if (ms < 1000) oss << ms << "ms";
    return oss.str();
}

Result<Duration> TimeUtils::parse_duration(StringView str) {
    // Simplified: parse "1h30m" format
    Duration total = Duration(0);
    std::string s(str);
    size_t pos = 0;
    while (pos < s.size()) {
        size_t next = pos;
        while (next < s.size() && std::isdigit(s[next])) next++;
        if (next == pos) return make_unexpected(MAKE_ERROR(ErrorCode::ParseError, "Invalid duration"));
        int value = std::stoi(s.substr(pos, next - pos));
        if (next >= s.size()) return make_unexpected(MAKE_ERROR(ErrorCode::ParseError, "Missing unit"));
        char unit = s[next];
        switch (unit) {
            case 'd': total += std::chrono::hours(value * 24); break;
            case 'h': total += std::chrono::hours(value); break;
            case 'm': total += std::chrono::minutes(value); break;
            case 's': total += std::chrono::seconds(value); break;
            default: return make_unexpected(MAKE_ERROR(ErrorCode::ParseError, "Invalid unit"));
        }
        pos = next + 1;
    }
    return total;
}

String TimeUtils::time_ago(TimePoint tp) {
    auto now_tp = now();
    if (tp > now_tp) return "in the future";
    auto dur = now_tp - tp;
    return format_duration_short(dur) + " ago";
}

String TimeUtils::time_until(TimePoint tp) {
    auto now_tp = now();
    if (tp < now_tp) return "in the past";
    auto dur = tp - now_tp;
    return "in " + format_duration_short(dur);
}

TimePoint TimeUtils::start_of_day(TimePoint tp) {
    auto time_t = std::chrono::system_clock::to_time_t(tp);
    std::tm tm = *std::localtime(&time_t);
    tm.tm_hour = 0; tm.tm_min = 0; tm.tm_sec = 0;
    return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

TimePoint TimeUtils::end_of_day(TimePoint tp) {
    auto time_t = std::chrono::system_clock::to_time_t(tp);
    std::tm tm = *std::localtime(&time_t);
    tm.tm_hour = 23; tm.tm_min = 59; tm.tm_sec = 59;
    return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

TimePoint TimeUtils::start_of_week(TimePoint tp) {
    auto time_t = std::chrono::system_clock::to_time_t(tp);
    std::tm tm = *std::localtime(&time_t);
    int wday = tm.tm_wday; // 0 = Sunday
    tm.tm_mday -= wday;
    tm.tm_hour = 0; tm.tm_min = 0; tm.tm_sec = 0;
    return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

TimePoint TimeUtils::end_of_week(TimePoint tp) {
    auto start = start_of_week(tp);
    return start + std::chrono::hours(24 * 7) - std::chrono::seconds(1);
}

TimePoint TimeUtils::start_of_month(TimePoint tp) {
    auto time_t = std::chrono::system_clock::to_time_t(tp);
    std::tm tm = *std::localtime(&time_t);
    tm.tm_mday = 1; tm.tm_hour = 0; tm.tm_min = 0; tm.tm_sec = 0;
    return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

TimePoint TimeUtils::end_of_month(TimePoint tp) {
    auto start = start_of_month(tp);
    auto time_t = std::chrono::system_clock::to_time_t(start);
    std::tm tm = *std::localtime(&time_t);
    tm.tm_mon++;
    if (tm.tm_mon > 11) { tm.tm_mon = 0; tm.tm_year++; }
    tm.tm_mday = 1; tm.tm_hour = 0; tm.tm_min = 0; tm.tm_sec = 0;
    return std::chrono::system_clock::from_time_t(std::mktime(&tm)) - std::chrono::seconds(1);
}

TimePoint TimeUtils::start_of_year(TimePoint tp) {
    auto time_t = std::chrono::system_clock::to_time_t(tp);
    std::tm tm = *std::localtime(&time_t);
    tm.tm_mon = 0; tm.tm_mday = 1; tm.tm_hour = 0; tm.tm_min = 0; tm.tm_sec = 0;
    return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

TimePoint TimeUtils::end_of_year(TimePoint tp) {
    auto start = start_of_year(tp);
    auto time_t = std::chrono::system_clock::to_time_t(start);
    std::tm tm = *std::localtime(&time_t);
    tm.tm_year++; tm.tm_mon = 0; tm.tm_mday = 1; tm.tm_hour = 0; tm.tm_min = 0; tm.tm_sec = 0;
    return std::chrono::system_clock::from_time_t(std::mktime(&tm)) - std::chrono::seconds(1);
}

u32 TimeUtils::day_of_week(TimePoint tp) {
    auto time_t = std::chrono::system_clock::to_time_t(tp);
    std::tm tm = *std::localtime(&time_t);
    return tm.tm_wday; // 0 = Sunday
}

u32 TimeUtils::day_of_year(TimePoint tp) {
    auto time_t = std::chrono::system_clock::to_time_t(tp);
    std::tm tm = *std::localtime(&time_t);
    return tm.tm_yday + 1;
}

u32 TimeUtils::week_of_year(TimePoint tp) {
    auto time_t = std::chrono::system_clock::to_time_t(tp);
    std::tm tm = *std::localtime(&time_t);
    // ISO week date
    return (tm.tm_yday + 7 - tm.tm_wday) / 7 + 1;
}

u32 TimeUtils::days_in_month(u32 year, u32 month) {
    static const u32 days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month >= 1 && month <= 12) {
        u32 d = days[month - 1];
        if (month == 2 && is_leap_year(year)) d = 29;
        return d;
    }
    return 0;
}

bool TimeUtils::is_leap_year(u32 year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

} // namespace game_req
