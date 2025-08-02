//
// Created by JanHe on 27.07.2025.
//

#ifndef GUARDIAN_H
#define GUARDIAN_H


class Guardian
{
public:
    enum ErrorType
    {
        WARNING, CRITICAL
    };


    static void println(const char* str);
    static void updateFault();
    static void setError(int i, const char* str, ErrorType level);
    static void setError(int i, const char* str);
    static void setup();
    static bool hasError();
    static const char* getErrorTitle();

private:
    static void error_code(int i);
    static void error_title(const char* str);
    static void error_level(ErrorType mode);
    static const char* errorTitle;
    static int errorCode;
    static ErrorType errorLevel;
};


#endif //GUARDIAN_H
