import os
import struct
import wave

from ImgToCArray import emitDataArray

def conv(fn):
    wav = wave.Wave_read(fn)

    nframes = wav.getnframes()

    header = struct.pack("iiii",
        wav.getnchannels(),
        wav.getsampwidth(),
        wav.getframerate(),
        nframes)

    frames = wav.readframes(nframes)

    rawname = os.path.splitext(fn)[0]
    cdata = emitDataArray("PCM_%s"%rawname, header + frames)

    out = file("%s.pcm"%rawname, 'wb')
    out.write(cdata)

if __name__ == "__main__":
    import sys, glob

    try: arg = sys.argv[1]
    except:
        print "usage: wavToRaw.py <glob-pattern|filename>"
        sys.exit(1)


    fns = glob.glob(arg)
    if not fns: fns = [arg]

    for fn in fns:
        conv(fn)
