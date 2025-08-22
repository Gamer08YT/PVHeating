//
// Created by JanHe on 22.08.2025.
//

#ifndef BACKTRACE_H
#define BACKTRACE_H


class Backtrace
{
private:
    static String format_addrs_to_string(const void* const* addrs, size_t count);
    static String get_current_task_backtrace(size_t max_depth);

public:
    static void report_backtrace_to_ha();
};


#endif //BACKTRACE_H
