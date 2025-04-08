#include <REGX52.H>
#include "uart.h"
#include "FPM10A.h"
#include "LCD1602.h"

const int KEY_BS = 11;
const int KEY_CLEAR = 12;
const int KEY_SWITCH = 13; // �л�������ʽ
const int KEY_CONFIRM = 14;
const int KEY_ADMIN = 15; // ����
const int KEY_EXIT = 16;

unsigned int mode = 0;
extern unsigned int finger_id;

void delay(unsigned int x);
unsigned char getKeyT();
unsigned char getKey();
void readKey();
void modeMain();
void modeSearch();
void funcSearch();
void modeUnlockSuccess();
void modeAdd();
void funcAdd();
void modeDelete();
void funcDelete();

void main()
{
	LCD_Init();
	delay(20);
	Uart_Init();
	delay(200);
	Device_Check();
	delay(800);
	modeMain();
	while (1)
	{
		if (getKey())
			readKey();
		delay(10);
	}
}

void delay(unsigned int x)
{
	unsigned char i, j;
	while (x--)
	{
		i = 2;
		j = 239;
		do
		{
			while (--j);
		} while (--i);
	}
}

// ��ȡ����
unsigned char getKeyT()
{
	unsigned char keyValue = 0;

	P1 = 0x0F;
	delay(3);

	if (P1 != 0x0F)
	{
		// �����
		P1 = 0x0F;
		switch(P1)
		{
			case(0x0E): keyValue += 3; break;
			case(0x0D): keyValue += 2; break;
			case(0x0B): keyValue += 1; break;
			case(0x07): keyValue += 0; break;
		}
		delay(15);
		
		// �����
		P1 = 0xF0;
		switch(P1)
		{
			case(0xE0): keyValue += 13; break;
			case(0xD0): keyValue += 9; break;
			case(0xB0): keyValue += 5; break;
			case(0x70): keyValue += 1; break;
		}
	}
		
	return keyValue;
}

// ����ӳ��
unsigned char getKey()
{
	unsigned char keyValue = getKeyT();
	unsigned char keyType;
	
	switch(keyValue)
	{
		case(1): keyType = 1; break;
		case(2): keyType = 2; break;
		case(3): keyType = 3; break;
		case(4): keyType = KEY_BS; break; // �˸�
		case(5): keyType = 4; break;
		case(6): keyType = 5; break;
		case(7): keyType = 6; break;
		case(8): keyType = KEY_SWITCH; break; // �л�
		case(9): keyType = 7; break;
		case(10): keyType = 8; break;
		case(11): keyType = 9; break;
		case(12): keyType = KEY_ADMIN; break; // ����
		case(13): keyType = KEY_CLEAR; break; // ȡ��
		case(14): keyType = 10; break;
		case(15): keyType = KEY_CONFIRM; break; // ȷ��
		case(16): keyType = KEY_EXIT; break; // �˳�
		default: keyType = 0;
	}
	
	return keyType;
}

// ����������
void readKey()
{
	unsigned char key;
	key = getKey();
	if (key)
	{
		while (getKey()); // �ȴ������ɿ�
		if (key == KEY_CLEAR)
			modeMain();
		else
		{
			if (mode == 0)
			{
				if (key == 1)
					modeSearch();
				else if (key == 2)
					modeAdd();
				else if (key == 3)
					modeDelete();
			}
			else if (mode == 1)
			{
				if (key == KEY_CONFIRM)
					funcSearch();
			}
			else if (mode == 3)
			{
				if (key == KEY_SWITCH)
				{
					if (finger_id == 1000)
						finger_id = 0;
					else
						finger_id = finger_id + 1;
					LCD_ShowNum(2, 5, finger_id, 3);
				}
				else if (key == KEY_CONFIRM)
					funcAdd();
			}
			else if (mode == 4)
			{
				if (key == KEY_CONFIRM)
					funcDelete();
			}
		}
	}
}

// �����棨ģʽ0��
void modeMain()
{
	mode = 0;
	LCD_Clear();
	LCD_ShowString(1, 1, "1:Search");
	LCD_ShowString(2, 1, "2:Add  3:Delete");
}

// ������ģʽ1��
void modeSearch()
{
	mode = 1;
	LCD_Clear();
	LCD_ShowString(1, 1, "Finger &");
	LCD_ShowString(2, 1, "press confirm");
}

// ����
void funcSearch()
{
	unsigned int find_fingerid = 0;
	unsigned char id_show[]={0,0,0};
	FPM10A_Cmd_Get_Img(); //���ָ��ͼ��
	FPM10A_Receive_Data(12);		
	//�жϽ��յ���ȷ����,����0ָ�ƻ�ȡ�ɹ�
	if(FPM10A_RECEICE_BUFFER[9]==0)
	{			
		delayA(100);
		FINGERPRINT_Cmd_Img_To_Buffer1();
		FPM10A_Receive_Data(12);		
		FPM10A_Cmd_Search_Finger();
		FPM10A_Receive_Data(16);			
		if(FPM10A_RECEICE_BUFFER[9] == 0) //������  
		{
			LCD_Clear();
			LCD_ShowString(1, 1, "Search success");
			LCD_ShowString(2, 1, "ID:");
			//ƴ��ָ��ID��
			find_fingerid = FPM10A_RECEICE_BUFFER[10]*256 + FPM10A_RECEICE_BUFFER[11];					
			//ָ��iDֵ��ʾ���� 
			LCD_ShowNum(2, 5, finger_id, 3);
			// SRD = 0;
			delayA(1500);
			// SRD = 1;
			modeMain();
		}
		else //û���ҵ�
		{
			LCD_Clear();
			LCD_ShowString(1, 1, "Search failure");
			delayA(1500);
			modeMain();
		}
	}
	else
	{
		LCD_Clear();
		LCD_ShowString(1, 1, "Search failure");
		delayA(1500);
		modeMain();
	}
}

// �����ɹ���ģʽ2��
void modeUnlockSuccess()
{
	mode = 2;
	LCD_Clear();
	LCD_ShowString(1, 1, "Door open! (R)");
}

// ���ӣ�ģʽ3��
void modeAdd()
{
	mode = 3;
	LCD_Clear();
	LCD_ShowString(1, 1, "Add finger");
	LCD_ShowString(2, 1, "ID:");
	finger_id = 0;
	LCD_ShowNum(2, 5, finger_id, 3);
}

// ����
void funcAdd()
{
	unsigned char id_show[]={0,0,0};
	FPM10A_Cmd_Get_Img();
	FPM10A_Receive_Data(12);
	if(FPM10A_RECEICE_BUFFER[9]==0)
	{
		delayA(100);
		FINGERPRINT_Cmd_Img_To_Buffer1();
		FPM10A_Receive_Data(12);
		delayA(100);
		FPM10A_Cmd_Get_Img(); //���ָ��ͼ��
		FPM10A_Receive_Data(12);
		//�жϽ��յ���ȷ����,����0ָ�ƻ�ȡ�ɹ�
		if(FPM10A_RECEICE_BUFFER[9]==0)
		{
			delayA(100);
			LCD_Clear();
			LCD_ShowString(1, 1, "Add success");
			LCD_ShowString(2, 1, "ID:");
			//ָ��iDֵ��ʾ���� 
			LCD_ShowNum(2, 5, finger_id, 3);
			FINGERPRINT_Cmd_Img_To_Buffer2();
			FPM10A_Receive_Data(12);
			FPM10A_Cmd_Reg_Model();//ת����������
			FPM10A_Receive_Data(12); 
			FPM10A_Cmd_Save_Finger(finger_id);                		         
			FPM10A_Receive_Data(12);
			delayA(1500);
			// finger_id=finger_id+1;
			modeMain();
		}
		else
		{
			LCD_Clear();
			LCD_ShowString(1, 1, "Add failure");
			delayA(1500);
			modeMain();
		}
	}
	else
	{
		LCD_Clear();
		LCD_ShowString(1, 1, "Add failure");
		delayA(1500);
		modeMain();
	}
}

// ɾ����ģʽ4��
void modeDelete()
{
	mode = 4;
	LCD_Clear();
	LCD_ShowString(1, 1, "Delete all?");
}

// ɾ��
void funcDelete()
{
	unsigned char i=0;
	FINGERPRINT_Cmd_Delete_All_Model();
	FPM10A_Receive_Data(12);
	LCD_Clear();
	LCD_ShowString(1, 1, "All deleted.");
	delay(1500);
	modeMain();
}