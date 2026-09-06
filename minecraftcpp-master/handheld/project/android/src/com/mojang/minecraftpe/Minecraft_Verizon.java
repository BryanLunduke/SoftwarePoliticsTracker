package com.mojang.minecraftpe;

import android.os.Bundle;

import com.mojang.android.licensing.LicenseCodes;
import com.verizon.vcast.apps.LicenseAuthenticator;


public class Minecraft_Verizon extends MainActivity {

	@Override
    public void onCreate(Bundle savedInstanceState)
    {
    	super.onCreate(savedInstanceState);

    	_licenseLib = new LicenseAuthenticator(this);
        _verizonThread = new VerizonLicenseThread(_licenseLib, VCastMarketKeyword, false);
        _verizonThread.start();
    }
	
	@Override
    public int checkLicense() {
    	if (_verizonThread == null)
    		return _licenseCode;
    		
    	if (!_verizonThread.hasCode)
    		return -1;
    	
    	_licenseCode = _verizonThread.returnCode;
    	_verizonThread = null;
    	return _licenseCode;
    }
	@Override
	public boolean hasBuyButtonWhenInvalidLicense() { return false; }

    private LicenseAuthenticator _licenseLib;
    private VerizonLicenseThread _verizonThread;
    private int _licenseCode;
    static private final String VCastMarketKeyword = "Minecraft";
}

//
// Requests license code from the Verizon VCAST application on the phone
//
class VerizonLicenseThread extends Thread
{
	public VerizonLicenseThread(LicenseAuthenticator licenseLib, String keyword, boolean isTest) {
		_keyword = keyword;
		_isTest = isTest;
		_licenseLib = licenseLib;
	}

	public void run() {
		if (_isTest)
			validateTestLicense();
		else
			validateLicense();
	}

	void validateTestLicense() {
		try {
			//final int status = LicenseAuthenticator.LICENSE_NOT_FOUND;
			final int status = LicenseAuthenticator.LICENSE_OK;
			returnCode = _licenseLib.checkTestLicense( _keyword, status );
		}
		catch (Throwable e) {
			returnCode = LicenseCodes.LICENSE_CHECK_EXCEPTION;
		}
		hasCode = true;
	}

	void validateLicense() {
		try {
			returnCode = _licenseLib.checkLicense( _keyword );
		}
		catch (Throwable e) {
			returnCode = LicenseCodes.LICENSE_CHECK_EXCEPTION; 
			//e.printStackTrace();
		}
		hasCode = true;
	}
	
	public volatile boolean hasCode = false;
	public volatile int     returnCode = -1; 

	private String _keyword;
	private boolean _isTest;
	private LicenseAuthenticator _licenseLib;
}
