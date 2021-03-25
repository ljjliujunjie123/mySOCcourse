#ifndef __APP_WAV_PLAYER_H_
#define __APP_WAV_PLAYER_H_

//启动wav播放器，遍历并播放SD卡根目录下所有wav文件
void app_StartWavPlayer();

//播放单个wav文件，返回是否正确退出
//格式支持、且播放完成或被用户终止时返回1
//格式不支持时返回0
int app_PlayWavFile(const char* filename);

#endif
