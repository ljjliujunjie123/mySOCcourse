#ifndef __APP_WAV_RECORDER_H_
#define __APP_WAV_RECORDER_H_

//�������趨
#define RECORDER_SAMPLE_RATE 8000

void app_StartWavRecorder();

void app_RecordAudio(const char* filename);

#endif
