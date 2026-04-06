package com.mojang.minecraftpe;

import android.content.Intent;
import android.net.Uri;
import com.mojang.android.licensing.LicenseCodes;

public class Minecraft_Market_Demo extends MainActivity {
	@Override
	public void buyGame() {
		final Uri buyLink = Uri.parse("market://details?id=com.mojang.minecraftpe");
        Intent marketIntent = new Intent( Intent.ACTION_VIEW, buyLink );
        startActivity(marketIntent);
	}

	@Override
    protected boolean isDemo() { return true; }
}
