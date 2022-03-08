: "-DFST_DEBUG_OUTPUT=""D:\\code\\c\\fst\\src\\log.txt"""
g++ -c StaticCompressorPlugin.cpp ../fst/src/FstAudioEffect.cpp -I"../fst/src" -w

dllwrap  --output-def libPlugin.def  --driver-name c++ ^
StaticCompressorPlugin.o FstAudioEffect.o ^
-L. --add-stdcall-alias -lole32 -lkernel32 -lgdi32 -lgdiplus -luuid -luser32 -lshell32 -mwindows --no-export-all-symbols --def Plugin.def ^
-o StaticCompressor.dll -Ofast

: Clean up
del *.o