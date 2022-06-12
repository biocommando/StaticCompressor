: "-DFST_DEBUG_OUTPUT=""D:\\code\\c\\fst\\src\\log.txt"""
g++ -static -c StaticCompressorPlugin.cpp ../fst/src/FstAudioEffect.cpp -I"../fst/src" -w

g++ -shared ^
StaticCompressorPlugin.o FstAudioEffect.o ^
-L. -lole32 -lkernel32 -lgdi32 -lgdiplus -luuid -luser32 -lshell32 --def Plugin.def ^
-o StaticCompressor.dll -Ofast

: Clean up
del *.o