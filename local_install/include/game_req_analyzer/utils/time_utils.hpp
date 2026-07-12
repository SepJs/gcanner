#pragma once

#include <game_req_analyzer/core/pch.hpp>

namespace game_req {

class TimeUtils {
public:
    static TimePoint now();
    static TimePoint now_utc();
    static u64 timestamp_ms();
    static u64 timestamp_us();
    static u64 timestamp_ns();
    static u64 timestamp_s();
    
    static String format(TimePoint tp, StringView format = "%Y-%m-%d %H:%M:%S");
    static String format_utc(TimePoint tp, StringView format = "%Y-%m-%d %H:%M:%S");
    static String format_iso8601(TimePoint tp);
    static String format_rfc3339(TimePoint tp);
    
    static Result<TimePoint> parse(StringView str, StringView format = "%Y-%m-%d %H:%M:%S");
    static Result<TimePoint> parse_iso8601(StringView str);
    static Result<TimePoint> parse_rfc3339(StringView str);
    
    static Duration since(TimePoint tp);
    static Duration until(TimePoint tp);
    
    static String format_duration(Duration d);
    static String format_duration_short(Duration d);
    static String format_duration_verbose(Duration d);
    
    static Result<Duration> parse_duration(StringView str);
    
    struct Timer {
        TimePoint start_time;
        bool running = false;
        
        Timer() : start_time(now()), running(true) {}
        explicit Timer(bool auto_start) : start_time(now()), running(auto_start) {
            if (!auto_start) running = false;
        }
        
        void start() { start_time = now(); running = true; }
        void stop() { running = false; }
        void reset() { start_time = now(); running = true; }
        
        [[nodiscard]] Duration elapsed() const {
            return running ? now() - start_time : Duration(0);
        }
        
        [[nodiscard]] u64 elapsed_ms() const {
            return chrono::duration_cast<chrono::milliseconds>(elapsed()).count();
        }
        
        [[nodiscard]] u64 elapsed_us() const {
            return chrono::duration_cast<chrono::microseconds>(elapsed()).count();
        }
        
        [[nodiscard]] u64 elapsed_ns() const {
            return chrono::duration_cast<chrono::nanoseconds>(elapsed()).count();
        }
        
        [[nodiscard]] f64 elapsed_s() const {
            return elapsed_ms() / 1000.0;
        }
    };
    
    struct ScopedTimer {
        StringView name;
        Timer timer;
        std::function<void(StringView, Duration)> callback;
        
        ScopedTimer(StringView n, std::function<void(StringView, Duration)> cb = {})
            : name(n), timer(true), callback(cb) {}
        
        ~ScopedTimer() {
            if (callback) callback(name, timer.elapsed());
        }
        
        [[nodiscard]] Duration elapsed() const { return timer.elapsed(); }
    };
    
    static String time_ago(TimePoint tp);
    static String time_until(TimePoint tp);
    
    static TimePoint start_of_day(TimePoint tp);
    static TimePoint end_of_day(TimePoint tp);
    static TimePoint start_of_week(TimePoint tp);
    static TimePoint end_of_week(TimePoint tp);
    static TimePoint start_of_month(TimePoint tp);
    static TimePoint end_of_month(TimePoint tp);
    static TimePoint start_of_year(TimePoint tp);
    static TimePoint end_of_year(TimePoint tp);
    
    static u32 day_of_week(TimePoint tp);  // 0 = Sunday
    static u32 day_of_year(TimePoint tp);
    static u32 week_of_year(TimePoint tp);
    static u32 days_in_month(u32 year, u32 month);
    static bool is_leap_year(u32 year);
};

} // namespace game_req