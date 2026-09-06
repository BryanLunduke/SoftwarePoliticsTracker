#ifndef LICENSECODES_H__
#define LICENSECODES_H__

class LicenseCodes
{
public:
    // Something's not ready, call again later
    static const int WAIT_PLATFORM_NOT_READY = -2;
    static const int WAIT_SERVER_NOT_READY = -1;

    // License is ok
    static const int LICENSE_OK = 0;
    static const int LICENSE_TRIAL_OK = 1;

    // License is not working in one way or another
    static const int LICENSE_VALIDATION_FAILED = 50;
    static const int ITEM_NOT_FOUND = 51;
    static const int LICENSE_NOT_FOUND = 52;
    static const int ERROR_CONTENT_HANDLER = 100;
    static const int ERROR_ILLEGAL_ARGUMENT = 101;
    static const int ERROR_SECURITY = 102;
    static const int ERROR_INPUT_OUTPUT = 103;
    static const int ERROR_ILLEGAL_STATE = 104;
    static const int ERROR_NULL_POINTER = 105;
    static const int ERROR_GENERAL = 106;
    static const int ERROR_UNABLE_TO_CONNECT_TO_CDS = 107;
    
    // The call went wrong so we didn't get a license value at all
    static const int ERROR_EXCEPTION = 200;
    
    static bool isOk(int i) {
        return (i == 0) || (i == 1);
    }
    static bool isReady(int i) {
        return (i >= 0);
    }
};

#endif /*LICENSECODES_H__ */

