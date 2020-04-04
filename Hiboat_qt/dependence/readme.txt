ffmpeg官网: https://ffmpeg.org/download.html#build-windows
FFmpeg Builds: https://ffmpeg.zeranoe.com/builds/

ffmpeg版本: 4.2.2

Qt编译MinGW32bit应用程序:(MinGW64bit类似, 已完成)
	1. 解压win32文件夹下的ffmpeg-4.2.2-win32-dev.zip, 将include和lib替换到QT项目主目录下的ffmpeg;
	2. 解压win32文件夹下的ffmpeg-4.2.2-win32-shared.zip, 将bin下面的所有dll文件放置到qt项目生成exe的目录下.
	