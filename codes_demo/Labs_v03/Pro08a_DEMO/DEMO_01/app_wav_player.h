#ifndef __APP_WAV_PLAYER_H_
#define __APP_WAV_PLAYER_H_

//����wav������������������SD����Ŀ¼������wav�ļ�
void app_StartWavPlayer();

//���ŵ���wav�ļ��������Ƿ���ȷ�˳�
//��ʽ֧�֡��Ҳ�����ɻ��û���ֹʱ����1
//��ʽ��֧��ʱ����0
int app_PlayWavFile(const char* filename);

#endif
