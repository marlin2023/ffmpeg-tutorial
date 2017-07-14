all:
	g++ -o encode test_main.cpp EncodeYuv.cpp  -I/Users/chris/work/StreamingMedia/ffmpeg/lib/ffmpeg-3.0.1/include 	\
	 -L/Users/chris/work/StreamingMedia/ffmpeg/lib/ffmpeg-3.0.1/lib    \
	-lavformat -lavcodec  -lavutil -lswscale  -lz  -lm          \
	-I/Users/chris/work/StreamingMedia/x264/x264_out/include -L/Users/chris/work/StreamingMedia/x264/x264_out/lib  \
	-lx264 -lbz2 -lswresample  -lfdk-aac  -framework CoreFoundation -framework CoreVideo -framework VideoDecodeAcceleration  \
	-framework Security -framework VideoToolbox -framework VideoToolbox -framework CoreMedia -liconv 

