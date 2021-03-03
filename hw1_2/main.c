#include <msp430.h> 
#define KEY_INPUT (P1IN & BIT3)//KEY_INPUTδ������1��������0
/*
 * main.c
 */
void convertLEDstate(int);//��ת״̬
int delayCheckPress(long int);//�ӳټӼ�ⰴ��

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;	// �رտ��Ź���ʱ��

    P1DIR |= BIT6;
    P1DIR |= BIT0;

    P1OUT &= ~BIT6;
    P1OUT &= ~BIT0;

    P1DIR &= ~BIT3;
    P1REN |= BIT3;
    P1OUT |= BIT3;

    int blinkState = 0;

    while(1)
    {
    	int pressUPcount = 0;
    	if(blinkState){
    		while(1){
    			int i = 1;
    			for(;i<3;i++){
    				convertLEDstate(i);
    				pressUPcount = delayCheckPress(20000);
    				convertLEDstate(i);
    				if(pressUPcount > 0){break;}
    		}
    			if(pressUPcount > 0){blinkState = 0;break;}
    		}
    		/*
    		pressUPcount += delayCheckPress(8000);
    		convertLEDstate(2);
    		pressUPcount += delayCheckPress(8000);
    		convertLEDstate(2);
    		*/
    	}
    	pressUPcount = delayCheckPress(80000);
    	blinkState = pressUPcount ;
    	//__delay_cycles(100);//��һ������
    }
	return 0;
}

void convertLEDstate(int LED){
	if(LED==1){
		P1OUT ^= BIT0;
	}
	if(LED==2){
		P1OUT ^= BIT6;
	}

}

int delayCheckPress(long int tmp){
	int isPress = 0;//
	int pressDOWNflag = 0;
	//int pressUPflag = 0;
	int cycleNum = 100;
	for(;cycleNum>0;cycleNum--){
		for(;tmp>0;tmp--){
		if(!KEY_INPUT){pressDOWNflag = 1;}
		//__delay_cycles(1000);//������
		if(pressDOWNflag && KEY_INPUT){
			isPress = 1;
			return isPress;
			}
		}
	}
	return isPress;
}
/*
int delay(int tmp){
	int isPress = 0;
	for(;tmp>0;tmp--){
		if(!KEY_INPUT){
			if(PressUP(3000)){
				isPress = 1;
				break;
			}
		}
	}
	for(tmp=1000;tmp>0;tmp--);
	return isPress;
}*/
/*
void showBlink(void){
	while(KEY_INPUT){
	P1OUT ^= BIT0;//��תLED1
	delay(3000000);
	//P1OUT ^= BIT0;
	P1OUT ^= BIT6;//��תLED2
	delay(3000000);
	//P1OUT ^= BIT6;
	}
}

int PressUP(int tmp){
	int pressUP = 0;
	for(;tmp > 0;tmp--){
		if(KEY_INPUT){
			pressUP = 1;
		}
	};
	return pressUP;
}*/
