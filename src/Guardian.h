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
        WARNING, CRITICAL, NORMAL
    };


    static void println(const char* str);
    static void updateFault();
    static void setError(int i, const char* str, ErrorType level);
    static void setError(int i, const char* str);
    static void testScan();
    static void setup();
    static bool hasError();
    static const char* getErrorTitle();
    static void setProgress(int i, unsigned int progress);
    static void setTitle(const char* str);
    static void setValue(int line, const char* key, const char* value);
    static void setValue(int line, const char* key, const char* value, const char* suffix);
    static void update();
    static void boot(int percentage, const char* str);
    static void clear();
    static void print(const char* str);
    static void clearError();
    static ErrorType getErrorType();
    static bool isCritical();
    static int getErrorCode();

private:
    static void error_code(int i);
    static void error_title(const char* str);
    static void error_level(ErrorType mode);
    static const char* errorTitle;
    static int errorCode;
    static ErrorType errorLevel;
};




#endif //GUARDIAN_H
