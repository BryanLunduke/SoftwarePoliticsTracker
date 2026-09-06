package com.mojang.android.licensing;

///see LicenseResult.h in C++ project 
public class LicenseCodes {

	// Something's not ready, call again later
	public static final int WAIT_PLATFORM_NOT_READY = -2; // Note used from java
    public static final int WAIT_SERVER_NOT_READY = -1;
	
    // License is ok
	public static final int LICENSE_OK = 0;
	public static final int LICENSE_TRIAL_OK = 1;

    // License is not working in one way or another
	public static final int LICENSE_VALIDATION_FAILED = 50;
	public static final int ITEM_NOT_FOUND = 51;
	public static final int LICENSE_NOT_FOUND = 52;
	public static final int ERROR_CONTENT_HANDLER = 100;
	public static final int ERROR_ILLEGAL_ARGUMENT = 101;
	public static final int ERROR_SECURITY = 102;
	public static final int ERROR_INPUT_OUTPUT = 103;
	public static final int ERROR_ILLEGAL_STATE = 104;
	public static final int ERROR_NULL_POINTER = 105;
	public static final int ERROR_GENERAL = 106;
	public static final int ERROR_UNABLE_TO_CONNECT_TO_CDS = 107;

	// The call went wrong so we didn't get a license value at all
	public static final int LICENSE_CHECK_EXCEPTION = 200;
}
