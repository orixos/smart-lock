#include <REGX52.H>
#include "LCD1602.h"
#include "AT24C02.h"
#include "IR.h"
#include "RC522.h"

sbit LED_RED = P2^3;
sbit LED_GREEN = P2^2;
sbit BUZZER = P3^7;
sbit pinIR = P2^4;

const int KEY_BS = 11;
const int KEY_CLEAR = 12;
const int KEY_SWITCH = 13; // �л�������ʽ
const int KEY_CONFIRM = 14;
const int KEY_ADMIN = 15; // ����
const int KEY_EXIT = 16;
const int EMPTY_PWD = 66; // ռ�ÿ�����λ
const int MAX_ERROR_COUNT = 3; // ���ɴ������
const int MAX_CARD_NUM = 6;

unsigned int key; // ��ǰ���µİ���
unsigned int count; // �����������λ��
unsigned int errorCount; // ����������
unsigned int pwd[6]; // �����������
unsigned int correctPwd[6]; // �Ž�����
const unsigned int devPwd[6] = {1, 4, 2, 8, 5, 7}; // ����������
unsigned int mode; // ģʽѡ��
unsigned int i; // ����forѭ��
unsigned int IRFlag; // �����־��0ʱ���ܽ��պ����ź�
unsigned char Command; // �����ź�
extern unsigned char UID[4];
unsigned char forgetFlag; // ���������־��1ʱ������ʾ����
// ��������ʱ������2024�����˸��ٰ�ȡ��������ʾ���룬����ʱ��1.5s

void delay(unsigned int x);
void Buzzer_Time(unsigned int ms);
unsigned char getKeyT();
unsigned char getKey();
void readKey();
void getIR();
void modeUnlock();
void modeUnlockSuccess();
void modeUnlockFailed();
void modeChangePwd();
void modeChangePwdSuccess();
void modeDeveloper();
void modeAlertUnlocked();
void modeExit();
void modeForget();
void modeMenu();
void modeIC();
void modeAdmin();
void modeAddIC();
void modeDelIC();
void modeIR();
void funcClearPwd();
void funcAppendNum(unsigned int input);
void funcDeleteNum();
void funcConfirm(int arr[]);
void funcGetPwd();
void funcWritePwd();
unsigned int funcReadLockFlag();
void funcWriteLockFlag(unsigned int flag);
void funcShowID(unsigned char *p);
void initFirstPwd();
void funcReadCard();
void funcAddCard(int cNum);
void funcDelCard(int cNum);

void main()
{
	// initFirstPwd(); // ��һ��ʹ��ʱ������ע��ȡ�����Գ�ʼ��AT24C02
	LCD_Init();
	RFID_Init();
	// IR_Init();
	// IRFlag = 0;
	LCD_ShowString(1, 4, "Welcome to");
	LCD_ShowString(2, 1, "Entrance System!");
	delay(1500);
	Buzzer_Time(100);
	
	// �����������״̬��ֱ��Ҫ�����뿪�����������
	if (funcReadLockFlag() == 0)
	{
		errorCount = MAX_ERROR_COUNT;
		modeDeveloper();
	}
	else
	{
		modeUnlock();
	}
	
	while (1)
	{	
		if (getKey())
		{
			readKey();
		}
		delay(3);
	}
}

// ��ʱ��12.000MHz��
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

// ������
void Buzzer_Time(unsigned int ms)
{
	BUZZER = 0;
	delay(ms);
	BUZZER = 1;
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
	key = getKey();
	if (key)
	{
		while (getKey()); // �ȴ������ɿ�

		if (key == KEY_EXIT)
		{
			modeExit();
		}
		
		if (mode == 0)
		{
			if (forgetFlag == 1 && key == KEY_CLEAR)
			{
				modeForget();
				return;
			}
			
			if (pwd[0] == 2 && pwd[1] == 0 && pwd[2] == 2 && pwd[3] == 4 && key == KEY_BS)
				forgetFlag = 1;
			
			if (key >= 1 && key <= 10)
				funcAppendNum(key);
			else if (key == KEY_BS)
				funcDeleteNum();
			else if (key == KEY_CLEAR)
				modeUnlock();
			else if (key == KEY_CONFIRM)
				funcConfirm(correctPwd);
			else if (key == KEY_SWITCH)
				modeMenu();
			
		}
		else if (mode == 1)
		{
			if (key > 0 && key <= 10)
				modeUnlock();
			else if (key == KEY_BS || key == KEY_CLEAR || key == KEY_CONFIRM)
				modeUnlock();
			else if (key == KEY_ADMIN)
				modeAdmin();
		}
		else if (mode == 3)
		{
			if (key >= 1 && key <= 10)
				funcAppendNum(key);
			else if (key == KEY_BS)
				funcDeleteNum();
			else if (key == KEY_CLEAR)
				modeChangePwd();
			else if (key == KEY_CONFIRM)
				funcWritePwd();
		}
		else if (mode == 5)
		{
			if (key >= 1 && key <= 10)
				funcAppendNum(key);
			else if (key == KEY_BS)
				funcDeleteNum();
			else if (key == KEY_CLEAR)
				modeDeveloper();
			else if (key == KEY_CONFIRM)
				funcConfirm(devPwd);
		}
		else if (mode == 9)
		{
			if (key == 2)
				modeIC();
			else if (key == 1)
				modeUnlock();
			else if (key == KEY_CLEAR)
				modeUnlock();
			else if (key == 3)
				modeIR();
		}
		else if (mode == 10)
		{
			if (key == KEY_SWITCH)
				modeMenu();
			else if (key == KEY_CONFIRM)
				funcReadCard();
			else if (key == KEY_CLEAR)
				modeUnlock();
		}
		else if (mode == 11)
		{
			if (key == 1)
				modeChangePwd();
			else if (key == KEY_CLEAR)
				modeUnlockSuccess();
			else if (key == 2)
				modeAddIC();
			else if (key == 3)
				modeDelIC();
		}
		else if (mode == 12)
		{
			if (key >= 1 && key <= 6)
				funcAddCard(key);
			else if (key == KEY_CLEAR)
				modeUnlockSuccess();
		}
		else if (mode == 13)
		{
			if (key >= 1 && key <= 6)
				funcDelCard(key);
			else if (key == KEY_CLEAR)
				modeUnlockSuccess();
			else if (key == 10) // �������0����ɾ��ȫ��IC����Ϣ
				funcDelCard(key);
		}
	}
}

// �������棨ģʽ0��
void modeUnlock()
{
	mode = 0;
	// IRFlag = 0;
	LED_RED = 1;
	LED_GREEN = 1;
	funcClearPwd();
	LCD_WriteCommand(0x01);
	LCD_ShowString(1, 1, "Input PIN:");
}

// �ѽ������棨ģʽ1��
void modeUnlockSuccess()
{
	mode = 1;
	errorCount = 0;
	// IRFlag = 1;
	LCD_WriteCommand(0x01);
	LCD_ShowString(1, 1, "Door open! (R)");
	LED_GREEN = 0;
	Buzzer_Time(100);
}

// δ�ɹ��������棨ģʽ2��
void modeUnlockFailed()
{
	mode = 2;
	errorCount++;
	LCD_WriteCommand(0x01);
	LCD_ShowString(1, 1, "Failed! (E)");
	LED_GREEN = 1;
	for (i = 0; i < 3; i++)
	{
		LED_RED = 0;
		Buzzer_Time(60);
		LED_RED = 1;
		delay(60);
	}
	delay(800);
	if (errorCount >= MAX_ERROR_COUNT)
		modeDeveloper();
	else
		modeUnlock();
}

// �޸����루ģʽ3��
void modeChangePwd()
{
	mode = 3;
	funcClearPwd();
	LCD_WriteCommand(0x01);
	LCD_ShowString(1, 1, "Set a new PIN:");
	LED_GREEN = 1;
}

// ��ʾ�����޸ĳɹ���ģʽ4��
void modeChangePwdSuccess()
{
	mode = 4;
	LCD_WriteCommand(0x01);
	LCD_ShowString(1, 1, "PIN changed");
	Buzzer_Time(100);
	for (i = 0; i < 5; i++)
	{
		LED_GREEN = 0;
		delay(110);
		LED_GREEN = 1;
		delay(110);
	}
	delay(800);
	modeUnlock();
}

// ���뿪�������루ģʽ5��
void modeDeveloper()
{
	mode = 5;
	LED_RED = 0;
	funcClearPwd();
	funcWriteLockFlag(0);
	LCD_WriteCommand(0x01);
	LCD_ShowString(1, 1, "Input dev pwd:");
}

// ��д�ѽ����������ڽ��������ģʽ6��
void modeAlertUnlocked()
{
	mode = 6;
	errorCount = 0;
	// IRFlag = 1;
	funcWriteLockFlag(1);
	LCD_WriteCommand(0x01);
	LCD_ShowString(1, 1, "Alert unlocked.");
	for (i = 0; i < 3; i++)
	{
		LED_GREEN = 0;
		Buzzer_Time(200);
		LED_GREEN = 1;
		delay(200);
	}	
	delay(800);
	modeUnlock();
}

// �˳����棨ģʽ7��
void modeExit()
{
	mode = 7;
	LCD_WriteCommand(0x01);
	LCD_ShowString(1, 1, "Have a good time");
	LCD_ShowString(2, 3, "By  Infinity");
	while (1);
}

// �������루ģʽ8��
void modeForget()
{
	mode = 8;
	forgetFlag = 0;
	funcGetPwd();
	LCD_WriteCommand(0x01);
	LCD_ShowString(1, 1, "* PIN:");
	for (i = 0; i < 6; i++)
	{
		LCD_ShowNum(1, i + 8, correctPwd[i], 1);
	}
	delay(1500);
	modeUnlock();
}

// ��������˵���ģʽ9��
void modeMenu()
{
	mode = 9;
	LCD_WriteCommand(0x01);
	LCD_ShowString(1, 1, "1:PIN");
	LCD_ShowString(2, 1, "2:IC  3:IR");
}

// IC��������ģʽ10��
void modeIC()
{
	mode = 10;
	LCD_WriteCommand(0x01);
	LCD_ShowString(1, 1, "Press confirm to");
	LCD_ShowString(2, 1, "start  detecting");
}

// �������˵���ģʽ11��
void modeAdmin()
{
	mode = 11;
	LCD_WriteCommand(0x01);
	LCD_ShowString(1, 1, "1:Change PIN");
	LCD_ShowString(2, 1, "2:AddIC  3:DelIC");
}

// IC��д�루ģʽ12��
void modeAddIC()
{
	mode = 12;
	LCD_WriteCommand(0x01);
	LCD_ShowString(1, 5, "Add Card");
	LCD_ShowString(2, 3, "Press 1 to 6");
}

// ɾ��IC����ģʽ13��
void modeDelIC()
{
	mode = 13;
	LCD_WriteCommand(0x01);
	LCD_ShowString(1, 1, "Del Card (0:All)");
	LCD_ShowString(2, 3, "Press 1 to 6");
}

// ������գ�ģʽ14��
void modeIR()
{
	mode = 14;
	LCD_WriteCommand(0x01);
	LCD_ShowString(1, 1, "Press KEY_POWER");
	LCD_ShowString(2, 1, "to unlock.");
	pinIR = 1;
	while (1)
	{
		if (pinIR == 0)
		{
			delay(10);
			if (pinIR == 0)
			{
				while (pinIR == 0);
				delay(10);
				break;
			}
		}
	}
	modeUnlockSuccess();
}

// ��ʼ���������������
void funcClearPwd()
{
	count = 0;
	for (i = 0; i < 6; i++)
	{
		pwd[i] = EMPTY_PWD;
	}
}

// ����һλ����
void funcAppendNum(unsigned int input)
{
	if (count < 6)
	{
		pwd[count] = input % 10;
		count += 1;
		LCD_ShowNum(2, count, input % 10, 1);
	}
}

// ɾ��һλ����
void funcDeleteNum()
{
	LCD_ShowString(2, count, " ");
	if (count > 0)
		count -= 1;
	pwd[count] = EMPTY_PWD;
}

// ��������Ƿ���ȷ
void funcConfirm(int arr[])
{
	if (count == 6)
	{
		funcGetPwd();
		if ((pwd[0] == arr[0]) &&
			(pwd[1] == arr[1]) &&
			(pwd[2] == arr[2]) &&
			(pwd[3] == arr[3]) &&
			(pwd[4] == arr[4]) &&
			(pwd[5] == arr[5]))
		{
			if (mode == 5)
				modeAlertUnlocked();
			else
				modeUnlockSuccess();
		}
		else
		{
			modeUnlockFailed();
		}
	}
}

// ��AT24C02��ȡ���벢д��correctPwd
void funcGetPwd()
{
	for (i = 0; i < 6; i++)
	{
		correctPwd[i] = AT24C02_ReadByte(i);
		delay(5);
	}
}

// ��AT24C02д������
void funcWritePwd()
{
	if (count == 6)
	{
		for (i = 0; i < 6; i++)
		{
			AT24C02_WriteByte(i, pwd[i]);
			delay(5);
		}
		modeChangePwdSuccess();
	}
}

// ��AT24C02��ȡ������־
unsigned int funcReadLockFlag()
{
	return AT24C02_ReadByte(6);
}

// ��AT24C02д��������־
void funcWriteLockFlag(unsigned int flag)
{
	AT24C02_WriteByte(6, flag);
}

// ��ʾ����
void funcShowID(unsigned char *p)
{
	unsigned char i;
	LCD_ShowString(1, 1, "CardID:");
	for (i = 0; i < 4; i++)
		LCD_ShowHexNum(1, 9 + i * 2, p[i], 2);
}

// ����
void funcReadCard()
{
	int flag0;
	flag0 = 0;
	while (1)
	{
		char cardNum;
		cardNum = Rc522Test();
		delay(10);
		LCD_WriteCommand(0x01);
		funcShowID(UID);
		if (cardNum >= 1 && cardNum <= MAX_CARD_NUM)
		{
			LCD_ShowString(2, 1, "Card");
			LCD_ShowNum(2, 6, cardNum, 1);
			LCD_ShowString(2, 13, "OPEN");
			flag0 = 1;
			delay(1500);
			break;
		}
		else if (cardNum == 0)
		{
			LCD_ShowString(2, 1, "No Record!");
			delay(1500);
			break;
		}
		delay(20);
	}
	if (flag0 == 1)
		modeUnlockSuccess();
	else if (flag0 == 0)
		modeIC();
}

// д��
void funcAddCard(int cNum)
{
	int flag0;
	flag0 = 0;
	while (1)
	{
		char cardNum;
		cardNum = Rc522Test();
		delay(10);
		LCD_WriteCommand(0x01);
		funcShowID(UID);
		if (cardNum >= 1 && cardNum <= MAX_CARD_NUM)
		{
			LCD_ShowString(2, 1, "Card");
			LCD_ShowNum(2, 6, cardNum, 1);
			LCD_ShowString(2, 10, "existed");
			flag0 = 1;
			for (i = 0; i < 3; i++)
			{
				LED_RED = 0;
				Buzzer_Time(60);
				LED_RED = 1;
				delay(60);
			}
			delay(700);
			break;
		}
		else if (cardNum == 0)
		{
			// AT24C02��8-11λ�濨1��Ϣ��12-15λ�濨2��Ϣ���Դ�����
			for (i = 0; i < 4; i++)
			{
				AT24C02_WriteByte(cNum * 4 + 4 + i, UID[i]);
				delay(5);
			}
			/*
			AT24C02_WriteByte(cNum * 4 + 4, UID[0]);
			delay(5);
			AT24C02_WriteByte(cNum * 4 + 5, UID[1]);
			delay(5);
			AT24C02_WriteByte(cNum * 4 + 6, UID[2]);
			delay(5);
			AT24C02_WriteByte(cNum * 4 + 7, UID[3]);
			delay(5);
			*/
			LCD_ShowString(2, 1, "Card");
			LCD_ShowNum(2, 6, cNum, 1);
			LCD_ShowString(2, 10, "WRITTEN");
			Buzzer_Time(100);
			for (i = 0; i < 5; i++)
			{
				LED_GREEN = 0;
				delay(110);
				LED_GREEN = 1;
				delay(110);
			}
			delay(800);
			break;
		}
		delay(20);
	}
	modeUnlockSuccess();
}

// ɾ����
void funcDelCard(int cNum)
{
	if (cNum != 10)
	{
		for (i = 0; i < 4; i++)
		{
			AT24C02_WriteByte(cNum * 4 + 4 + i, 0);
			delay(5);
		}
		delay(20);
		LCD_WriteCommand(0x01);
		LCD_ShowString(1, 6, "Card");
		LCD_ShowNum(1, 11, cNum, 1);
		LCD_ShowString(2, 1, "deleted  success");
		delay(1500);
		modeUnlockSuccess();
	}
	else
	{
		for (i = 8; i < 32; i++)
		{
			AT24C02_WriteByte(i, 0);
			delay(5);
		}
		delay(20);
		LCD_WriteCommand(0x01);
		LCD_ShowString(1, 4, "All  cards");
		LCD_ShowString(2, 1, "deleted  success");
		delay(1500);
		modeUnlockSuccess();
	}
}

// *****************************************************
// ��ʼ������ʼ���հ�AT24C02ʱʹ�ã�д����������ǰ��
void initFirstPwd()
{
	unsigned int initialPwd[6] = {1, 2, 3, 4, 5, 6};
	for (i = 0; i < 6; i++)
	{
		AT24C02_WriteByte(i, initialPwd[i]);
		delay(5);
	}
	AT24C02_WriteByte(6, 1);
	delay(5);
	for (i = 8; i < 32; i++)
	{
		AT24C02_WriteByte(i, 0);
		delay(5);
	}
	LCD_Init();
	LCD_ShowString(1, 1, "Init Finished");
	while (1);
}
// *****************************************************