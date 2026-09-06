package com.mojang.minecraftpe.sound;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Random;

import android.content.Context;
import android.media.SoundPool;

import com.mojang.minecraftpe.R;

public class SoundPlayer
{
	public SoundPlayer(Context c, int streamType) {
		_context = c;
		_stream = streamType;
		
		_random = new Random();
		_pool = new SoundPool(4, _stream, 0);

		_init();
	}
		
	private void _init() {
		// Sounds that needs to be preloaded (although asynchronous :p)
		SoundId[] preLoaded = {
			new SoundId(R.raw.click, "random.click"),
			new SoundId(R.raw.explode, "random.explode"),
			new SoundId(R.raw.splash, "random.splash"),
			new SoundId(R.raw.hurt, "random.hurt"),
			new SoundId(R.raw.pop, "random.pop"),

			new SoundId(R.raw.door_open, "random.door_open"),
			new SoundId(R.raw.door_close, "random.door_close"),
			
			new SoundId(R.raw.cloth1, "step.cloth"),
			new SoundId(R.raw.cloth2, "step.cloth"),
			new SoundId(R.raw.cloth3, "step.cloth"),
			new SoundId(R.raw.cloth4, "step.cloth"),

			new SoundId(R.raw.glass1, "random.glass"),
			new SoundId(R.raw.glass2, "random.glass"),
			new SoundId(R.raw.glass3, "random.glass"),

			new SoundId(R.raw.grass1, "step.grass"),
			new SoundId(R.raw.grass2, "step.grass"),
			new SoundId(R.raw.grass3, "step.grass"),
			new SoundId(R.raw.grass4, "step.grass"),

			//new SoundId(R.raw.gravel1, "step.gravel"),
			new SoundId(R.raw.gravel2, "step.gravel"),
			new SoundId(R.raw.gravel3, "step.gravel"),
			new SoundId(R.raw.gravel4, "step.gravel"),

			new SoundId(R.raw.sand1, "step.sand"),
			new SoundId(R.raw.sand2, "step.sand"),
			new SoundId(R.raw.sand3, "step.sand"),
			new SoundId(R.raw.sand4, "step.sand"),

			new SoundId(R.raw.stone1, "step.stone"),
			new SoundId(R.raw.stone2, "step.stone"),
			new SoundId(R.raw.stone3, "step.stone"),
			new SoundId(R.raw.stone4, "step.stone"),

			new SoundId(R.raw.wood1, "step.wood"),
			new SoundId(R.raw.wood2, "step.wood"),
			new SoundId(R.raw.wood3, "step.wood"),
			new SoundId(R.raw.wood4, "step.wood"),

			new SoundId(R.raw.sheep1, "mob.sheep"),
			new SoundId(R.raw.sheep2, "mob.sheep"),
			new SoundId(R.raw.sheep3, "mob.sheep"),

			new SoundId(R.raw.chicken2, "mob.chicken"),
			new SoundId(R.raw.chicken3, "mob.chicken"),
			new SoundId(R.raw.chickenhurt1, "mob.chickenhurt"),
			new SoundId(R.raw.chickenhurt2, "mob.chickenhurt"),

			new SoundId(R.raw.cow1, "mob.cow"),
			new SoundId(R.raw.cow2, "mob.cow"),
			new SoundId(R.raw.cow3, "mob.cow"),
			new SoundId(R.raw.cow4, "mob.cow"),
			new SoundId(R.raw.cowhurt1, "mob.cowhurt"),
			new SoundId(R.raw.cowhurt2, "mob.cowhurt"),

			new SoundId(R.raw.pig1, "mob.pig"),
			new SoundId(R.raw.pig2, "mob.pig"),
			new SoundId(R.raw.pig3, "mob.pig"),
			new SoundId(R.raw.pigdeath, "mob.pigdeath"),
			
			new SoundId(R.raw.zombie1, "mob.zombie"),
			new SoundId(R.raw.zombie2, "mob.zombie"),
			new SoundId(R.raw.zombie3, "mob.zombie"),
			new SoundId(R.raw.zombiedeath, "mob.zombiedeath"),
			new SoundId(R.raw.zombiehurt1, "mob.zombiehurt"),
			new SoundId(R.raw.zombiehurt2, "mob.zombiehurt"),

			new SoundId(R.raw.skeleton1, "mob.skeleton"),
			new SoundId(R.raw.skeleton2, "mob.skeleton"),
			new SoundId(R.raw.skeleton3, "mob.skeleton"),
			new SoundId(R.raw.skeletonhurt1, "mob.skeletonhurt"),
			new SoundId(R.raw.skeletonhurt2, "mob.skeletonhurt"),
			new SoundId(R.raw.skeletonhurt3, "mob.skeletonhurt"),
			new SoundId(R.raw.skeletonhurt4, "mob.skeletonhurt"),

			new SoundId(R.raw.spider1, "mob.spider"),
			new SoundId(R.raw.spider2, "mob.spider"),
			new SoundId(R.raw.spider3, "mob.spider"),
			new SoundId(R.raw.spider4, "mob.spider"),
			new SoundId(R.raw.spiderdeath, "mob.spiderdeath"),

			new SoundId(R.raw.fallbig1, "damage.fallbig"),
			new SoundId(R.raw.fallbig2, "damage.fallbig"),
			new SoundId(R.raw.fallsmall, "damage.fallsmall"),

			new SoundId(R.raw.bow, "random.bow"),
			new SoundId(R.raw.bowhit1, "random.bowhit"),
			new SoundId(R.raw.bowhit2, "random.bowhit"),
			new SoundId(R.raw.bowhit3, "random.bowhit"),
			new SoundId(R.raw.bowhit4, "random.bowhit"),
			
			new SoundId(R.raw.creeper1, "mob.creeper"),
			new SoundId(R.raw.creeper2, "mob.creeper"),
			new SoundId(R.raw.creeper3, "mob.creeper"),
			new SoundId(R.raw.creeper4, "mob.creeper"),
			new SoundId(R.raw.creeperdeath, "mob.creeperdeath"),
			new SoundId(R.raw.eat1, "random.eat"),
			new SoundId(R.raw.eat2, "random.eat"),
			new SoundId(R.raw.eat3, "random.eat"),
			new SoundId(R.raw.fuse, "random.fuse"),

			new SoundId(R.raw.zpig1, "mob.zombiepig.zpig"),
			new SoundId(R.raw.zpig2, "mob.zombiepig.zpig"),
			new SoundId(R.raw.zpig3, "mob.zombiepig.zpig"),
			new SoundId(R.raw.zpig4, "mob.zombiepig.zpig"),
			new SoundId(R.raw.zpigangry1, "mob.zombiepig.zpigangry"),
			new SoundId(R.raw.zpigangry2, "mob.zombiepig.zpigangry"),
			new SoundId(R.raw.zpigangry3, "mob.zombiepig.zpigangry"),
			new SoundId(R.raw.zpigangry4, "mob.zombiepig.zpigangry"),
			new SoundId(R.raw.zpigdeath, "mob.zombiepig.zpigdeath"),
			new SoundId(R.raw.zpighurt1, "mob.zombiepig.zpighurt"),
			new SoundId(R.raw.zpighurt2, "mob.zombiepig.zpighurt"),
		}; 
		
		for (SoundId s: preLoaded) {
			load(s.name, s.soundId);
		}
		// Sounds that are loaded in a separate thread
	}

//	public void loadWithAlias(String filename, String alias) {
//		int id = loadRaw(filename);
//		addRaw(new SoundId(id, alias));
//	}
	
	public void play(String s, float volume, float pitch) {
		SoundId sound = get(s);//load(s);
		if (sound != null) {
			volume *= getCurrentStreamVolume();
			//System.out.println("playing sound id: " + sound.soundId);
			if (s.equals("step.sand") || s.equals("step.gravel")) volume *= 0.5f;
			_pool.play(sound.soundId, volume, volume, 0, 0, pitch);
		}
	}
	
	private SoundId get(String s) {
		List<SoundId> sounds = _sounds.get( s );
		if (sounds == null) return null;
		
		return sounds.get(_random.nextInt(sounds.size()));
	}

	public float getCurrentStreamVolume() {
		return 2.5f;
	    //AudioManager mgr = (AudioManager)_context.getSystemService(Context.AUDIO_SERVICE);
	    //return ((float)mgr.getStreamVolume(_stream)) / mgr.getStreamMaxVolume(_stream);
	}
	
	synchronized public SoundId load(String id, int resId) {
		if (id == null)
			return null;

		List<SoundId> sounds = _sounds.get(id);
		SoundId soundId = null;
		
		if (sounds == null) {
			sounds = new ArrayList<SoundId>();
			_sounds.put(id, sounds);
		}
		
		for (SoundId sid : sounds)
			if (sid.soundId == resId)
				return sid;
		
		int snd = _pool.load(_context, resId, DefaultPriority);
		soundId = new SoundId(snd, id);
		sounds.add(soundId);

		return soundId;
	}
	
	private Context _context;
	private Random _random;
	private int _stream;
	private SoundPool _pool;

	private Map<String, List<SoundId>> _sounds = new HashMap<String, List<SoundId>>();

	static private final int DefaultPriority = 1;

	public class SoundId {
		SoundId(int soundId, String name) {
			this.soundId = soundId;
			this.name = name;
		}
		int soundId;
		String name;
	}
}
