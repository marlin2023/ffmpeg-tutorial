## 源码版本&补丁

源码下载：**ffmpeg-4.0.5版本**

ndk版本：**ndkr17c**版本

直接编译有问题，需要更改源码，对应patch文件生成命令：

**生成补丁**：

diff -Naur ffmpeg-4.0.5  ffmpeg-4.0.5-vis  > ffmpeg-4.0.5-vis.patch



**打补丁**：

cd ffmpeg-4.0.5

fmpeg-4.0.5 ⮀ patch -p1 < ./../ffmpeg-4.0.5-vis.patch

patching file libavcodec/aaccoder.c

patching file libavcodec/hevc_mvs.c

patching file libavcodec/opus_pvq.c



这里把patch中的内容打到了avcodec目录下对应的三个文件中。

## 编译

需要编译armv7 & arm64版本，对应编译脚本内容在**build_android.sh**

执行命令：build_android.sh armv7  || build_android.sh armv8



