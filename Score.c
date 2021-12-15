#include <reg52.h>
#define uchar unsigned char
#define uint unsigned int
#define fosc 11059200ul
#define N 20
uchar code table[] = {0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x6f, 0x77, 0x7c, 0x39, 0x5e, 0x79, 0x71};
int teamscore[2] = {10,20};
int memberscore[8] = {0};
int minute, time, section, num, team;
//		��������
//		��Ա�����ʾ����
int playerShown = 0;
//		�������ʾģ�����
int displayMethod = 1;
uchar addr = 0;
bit cycleDisplay = 0;
bit addScoreEN = 0;
//--����ʹ�õ�IO��--//
sbit I2C_SCL = P2 ^ 1;
sbit I2C_SDA = P2 ^ 0;
sbit dula = P2 ^ 6;
sbit wela = P2 ^ 7;
sbit beep = P2 ^ 3;
//--����ȫ�ֱ���--//
void I2C_Delay10us();
void I2C_Start();                         //��ʼ�źţ���I2C_SCLʱ���ź��ڸߵ�ƽ�ڼ�I2C_SDA�źŲ���һ���½���
void I2C_Stop();                          //��ֹ�źţ���I2C_SCLʱ���źŸߵ�ƽ�ڼ�I2C_SDA�źŲ���һ��������
uchar I2C_SendByte(uchar dat, uchar ack); //ʹ��I2c��ȡһ���ֽ�
uchar I2C_ReadByte();                     //ͨ��I2C����һ���ֽڡ���I2C_SCLʱ���źŸߵ�ƽ�ڼ䣬���ַ����ź�I2C_SDA�����ȶ�
void TimerInit();                         // ��ʼ����ʱ���Ͷ�ʱ�¼�����¼�
void delay(uint z);                       // ��ʱ����
void matrixkeyscan();                     // ����ɨ��
void showPlayerScore(int team);           // ��ʾĳ����Ա�ɼ�
void addPlayerScore(int score);           // Ϊĳ����Ա�ӷ�
void output_time();                       // ����ʱ����ʾ
void showAllScores();                     // �ܵ÷���ʾ
void displayController();                 // ��ʾ·�ɿ�����
void midBreak();                          // �г���Ϣ��ʾ
void showWinner();                        // ʤ����ʾ

/*
    2021-11-30 ��1��
    ����˵����
		ʵ���˼Ʒ����Ļ�������
		1-> �������κ͵���ʱ��ʾ
		2-> ÿ����Ա�ļӼ���
		3-> ��Ա�Լ������ܵ÷ֵ���ʾ����

    2021-11-30 ��2��
    2��˵����
        1-> ��д�˲��ֻᵼ���ڴ濪������Ĳ������㷨
        2-> �޸��˶�ջ���ͷŵ��µĵ�Ƭ���ڴ����������������

    2021-12-12 ��3��
    3��˵����
        1-> �������г���Ϣ����
        2-> �����˶ϵ���书��
        3-> �����㷨�����ٵ�Ƭ���Ĵ��濪��(�������ʾ)
        4-> �޸��˶�Ա���ܵ÷ֳ����ٷֺ���ʾ�쳣������
        5-> �޸�������ܵ���ӰЧ�����ѵ����⣨���465�У�
        6-> ���ܵ÷ֳ���100��2������ĵĵ÷ֽ���ʱ������ʾ��2���ֻ�һ�Σ�
        �ϵ����ʹ�õ��Ǵ���оƬΪ�����ϵ�AT24C02оƬ�������ٶȽ�������20KB�Ĵ���ռ䡣

    ע�� by Milhouse����˳����Master Ruan�����Ĳ�����Leo������֣�
*/

void main()
{
    TimerInit();
    while (1)
    {
        /* ����ɨ����� */
        matrixkeyscan();
        /* �������ʾģ�� */
        displayController();
    }
}

//��������: ��24c02��һ����ַд��һ������
void At24c02Write(unsigned char addr, unsigned char dat)
{
    I2C_Start();
    I2C_SendByte(0xa0, 1); //����д������ַ
    I2C_SendByte(addr, 1); //����Ҫд���ڴ��ַ
    I2C_SendByte(dat, 0);  //��������
    I2C_Stop();
}

// ��ȡ24c02��һ����ַ��һ������
unsigned char At24c02Read(unsigned char addr)
{
    unsigned char num;
    I2C_Start();
    I2C_SendByte(0xa0, 1); //����д������ַ
    I2C_SendByte(addr, 1); //����Ҫ��ȡ�ĵ�ַ
    I2C_Start();
    I2C_SendByte(0xa1, 1); //���Ͷ�������ַ
    num = I2C_ReadByte();  //��ȡ����
    I2C_Stop();
    return num;
}

void I2C_Delay10us()
{ }

// ��ʼ�źţ���I2C_SCLʱ���ź��ڸߵ�ƽ�ڼ�I2C_SDA�źŲ���һ���½���
void I2C_Start()
{
    I2C_SDA = 1;
    I2C_Delay10us();
    I2C_SCL = 1;
    I2C_Delay10us(); //����ʱ����I2C_SDA����ʱ��>4.7us
    I2C_SDA = 0;
    I2C_Delay10us(); //����ʱ����>4us
    I2C_SCL = 0;
    I2C_Delay10us();
}

//��ֹ�źţ���I2C_SCLʱ���źŸߵ�ƽ�ڼ�I2C_SDA�źŲ���һ��������
void I2C_Stop()
{
    I2C_SDA = 0;
    I2C_Delay10us();
    I2C_SCL = 1;
    I2C_Delay10us(); //����ʱ�����4.7us
    I2C_SDA = 1;
    I2C_Delay10us();
}

//ͨ��I2C����һ���ֽڡ���I2C_SCLʱ���źŸߵ�ƽ�ڼ䣬 ���ַ����ź�I2C_SDA�����ȶ�
uchar I2C_SendByte(uchar dat, uchar ack)
{
    uchar a = 0, b = 0;     //���255��һ����������Ϊ1us�������ʱ255us��
    for (a = 0; a < 8; a++) //Ҫ����8λ�������λ��ʼ
    {
        I2C_SDA = dat >> 7; //��ʼ�ź�֮��I2C_SCL=0�����Կ���ֱ�Ӹı�I2C_SDA�ź�
        dat = dat << 1;
        I2C_Delay10us();
        I2C_SCL = 1;
        I2C_Delay10us(); //����ʱ��>4.7us
        I2C_SCL = 0;
        I2C_Delay10us(); //ʱ�����4us
    }
    I2C_SDA = 1;
    I2C_Delay10us();
    I2C_SCL = 1;
    while (I2C_SDA && (ack == 1)) //�ȴ�Ӧ��Ҳ���ǵȴ����豸��I2C_SDA����
    {
        b++;
        if (b > 200) //�������200usû��Ӧ����ʧ�ܣ�����Ϊ��Ӧ�𣬱�ʾ���ս���
        {
            I2C_SCL = 0;
            I2C_Delay10us();
            return 0;
        }
    }
    I2C_SCL = 0;
    I2C_Delay10us();
    return 1;
}

// ʹ��I2c��ȡһ���ֽ�
uchar I2C_ReadByte()
{
    uchar a = 0, dat = 0;
    I2C_SDA = 1; //��ʼ�ͷ���һ���ֽ�֮��I2C_SCL����0
    I2C_Delay10us();
    for (a = 0; a < 8; a++) //����8���ֽ�
    {
        I2C_SCL = 1;
        I2C_Delay10us();
        dat <<= 1;
        dat |= I2C_SDA;
        I2C_Delay10us();
        I2C_SCL = 0;
        I2C_Delay10us();
    }
    return dat;
}

void displayController()
{
		if ( section >= 5)
	{
		showWinner();
		return;
	}
    switch (displayMethod)
    {
    case 0:
        /* ������������ʾģ�� */
        showAllScores();
        break;
    case 1:
        /* ������������ʱ��ʾģ�� */
        output_time();
        break;
    case 2:
        /* ��ʾA����Ա�÷� */
        showPlayerScore(10);
        break;
    case 3:
        /* ��ʾB����Ա�÷� */
        showPlayerScore(11);
        break;
    case 4:
        /* �г���Ϣ */
        midBreak();
        break;
    }
}
//ʤ����ʾ
void showWinner()
{
	if ( teamscore[0] > teamscore[1] )
	{
		P0	= 0XFF;
		wela	= 1;
		P0	= 0xfe;
		wela	= 0;
		P0	= 0X00;
		dula	= 1;
		P0	= table[10];
		dula	= 0;
		delay( 2 );

		P0	= 0XFF;
		wela	= 1;
		P0	= 0xfd;
		wela	= 0;
		dula	= 1;
		P0	= 0x40;
		dula	= 0;
		delay( 2 );

		P0	= 0XFF;
		wela	= 1;
		P0	= 0xfb;
		wela	= 0;
		dula	= 1;
		P0	= table[teamscore[0] / 100];
		dula	= 0;
		delay( 2 );

		P0	= 0XFF;
		wela	= 1;
		P0	= 0xf7;
		wela	= 0;
		dula	= 1;
		P0	= 0x40;
		dula	= 0;
		delay( 2 );

		P0	= 0XFF;
		wela	= 1;
		P0	= 0xef;
		wela	= 0;
		dula	= 1;
		if(teamscore[0] >= 100)
			P0	= table[teamscore[0] / 100 % 10];
		else
			P0	= table[teamscore[0] / 10];
		dula	= 0;
		delay( 2 );

		P0	= 0XFF;
		wela	= 1;
		P0	= 0xdf;;
		wela	= 0;
		dula	= 1;
		P0	= table[teamscore[0] % 10];
		dula	= 0;
		delay( 2 );

		P0	= 0XFF;
		wela	= 1;
		P0	= 0xff;
		wela	= 0;
	} else if ( (teamscore[0] < teamscore[1]) )
	{
		P0	= 0XFF;
		wela	= 1;
		P0	= 0xfe;
		wela	= 0;
		P0	= 0X00;
		dula	= 1;
		P0	= table[11];
		dula	= 0;
		delay( 2 );

		P0	= 0XFF;
		wela	= 1;
		P0	= 0xfd;
		wela	= 0;
		dula	= 1;
		P0	= 0x40;
		dula	= 0;
		delay( 2 );

		P0	= 0XFF;
		wela	= 1;
		P0	= 0xfb;
		wela	= 0;
		dula	= 1;
		P0	= 0x40;
		dula	= 0;
		delay( 2 );

		P0	= 0XFF;
		wela	= 1;
		P0	= 0xf7;
		wela	= 0;
		dula	= 1;
		P0	= table[teamscore[1] / 100];
		dula	= 0;
		delay( 2 );

		P0	= 0XFF;
		wela	= 1;
		P0	= 0xef;
		wela	= 0;
		dula	= 1;
		if(teamscore[1] >= 100)
			P0	= table[teamscore[1] / 100 % 10];
		else
			 P0	= table[teamscore[1] / 10];
		dula	= 0;
		delay( 2 );

		P0	= 0XFF;
		wela	= 1;
		P0	= 0xdf;;
		wela	= 0;
		dula	= 1;
		P0	= table[teamscore[1]%10];
		dula	= 0;
		delay( 2 );
		P0	= 0XFF;
		wela	= 1;
		P0	= 0xff;
		wela	= 0;
	} else {
		P0	= 0XFF;
		wela	= 1;
		P0	= 0xfe;
		wela	= 0;
		P0	= 0X00;
		dula	= 1;
		P0	= table[14];
		dula	= 0;
		delay( 2 );

		P0	= 0XFF;
		wela	= 1;
		P0	= 0xfd;
		wela	= 0;
		dula	= 1;
		P0	= 0x67;
		dula	= 0;
		delay( 2 );

		P0	= 0XFF;
		wela	= 1;
		P0	= 0xfb;
		wela	= 0;
		dula	= 1;
		P0	= 0x3e;
		dula	= 0;
		delay( 2 );

		P0	= 0XFF;
		wela	= 1;
		P0	= 0xf7;
		wela	= 0;
		dula	= 1;
		P0	= table[10];
		dula	= 0;
		delay( 2 );

		P0	= 0XFF;
		wela	= 1;
		P0	= 0xef;
		wela	= 0;
		dula	= 1;
		P0	= 0x38;
		dula	= 0;
		delay( 2 );
	}
}

// �г���Ϣ��ʾ
void midBreak()
{
    int i;
    for (i = 1; i <= 6; i++)
    {
        P0 = 0;
        dula = 1;
        dula = 0;

        switch (i)
        {
        case 1:
            P0 = 0xfe;
            break;
        case 2:
            P0 = 0xfd;
            break;
        case 3:
            P0 = 0xfb;
            break;
        case 4:
            P0 = 0xf7;
            break;
        case 5:
            P0 = 0xef;
            break;
        case 6:
            P0 = 0xdf;
            break;
        }

        wela = 1;
        wela = 0;

        switch (i)
        {
        case 1:
            P0 = table[11];
            break;
        case 2:
            P0 = 0x40;
            break;
        case 3:
            P0 = table[minute / 10];
            break;
        case 4:
            P0 = table[minute % 10];
            break;
        case 5:
            P0 = table[time / 10];
            break;
        case 6:
            P0 = table[time % 10];
            break;
        }

        dula = 1;
        dula = 0;
        delay(1);
    }

    P0 = 0XFF;
    wela = 1;
    P0 = 0xff;
    wela = 0;
}

void showPlayerScore(int team)
{
    int i;
    int index;
    int hundredMode = 0; // �ٷ���ʾģʽ

    if (team == 11)
    {
        // B ����Ա������4��ʼ
        index = playerShown + 4;
    }
    else
    {
        index = playerShown;
    }

    if (memberscore[index] > 99)
        hundredMode = 1; // �����ٷ���ʾ

    for (i = 1; i <= 6; i++)
    {
        P0 = 0;
        dula = 1;
        dula = 0;

        switch (i)
        {
        case 1:
            P0 = 0xfe;
            break;
        case 2:
            P0 = 0xfd;
            break;
        case 3:
            P0 = 0xfb;
            break;
        case 4:
            P0 = 0xf7;
            break;
        case 5:
            P0 = 0xef;
            break;
        case 6:
            P0 = 0xdf;
            break;
        }

        wela = 1;
        wela = 0;

        switch (i)
        {
        case 1:
            P0 = table[team];
            break;
        case 2:
            if (hundredMode)
                // �ٷ�ģʽ�£���λ��ʾ��Ա���
                if (team == 11)
                    P0 = table[index - 3];
                else
                    P0 = table[index + 1];
            else
                // ����ģʽ�£���λ��ʾ "��"
                P0 = 0x40;
            break;
        case 3:
            if (hundredMode)
                // �ٷ�ģʽ�£���λ��ʾ "��"
                P0 = 0x40;
            else
                // ����ģʽ�£���λ��ʾ��Ա���
                if (team == 11)
                    P0 = table[index - 3];
                else
                    P0 = table[index + 1];
            break;
        case 4:
            if (hundredMode)
                // �ٷ�ģʽ���£���λ��ʾ�ɼ��İ�λ
                P0 = table[memberscore[index] / 100];
            else
                // ����ģʽ�£���λ��ʾ "��"
                P0 = 0x40;
            break;
        case 5:
            P0 = table[memberscore[index] % 100 / 10];
            break;
        case 6:
            P0 = table[memberscore[index] % 10];
            break;
        }

        dula = 1;
        dula = 0;
        delay(1);
    }

    P0 = 0XFF;
    wela = 1;
    P0 = 0xff;
    wela = 0;
}

void addPlayerScore(int score)
{
    // �ӷ�ʹ�ܿ���
    if (addScoreEN == 1)
    {
        // A ����Ա�ķ���
        if (displayMethod == 2)
        {
            memberscore[playerShown] = memberscore[playerShown] + score;
            // �ӷֲ�������Ժ󣬽����˵÷�д�봢��
            At24c02Write(playerShown + 5, memberscore[playerShown]);
            delay(2);
            teamscore[0] = teamscore[0] + score;
            // �ӷֲ�������Ժ󣬽��ܷ�д�봢��
            At24c02Write(0, teamscore[0]);
        }
        // B ����Ա�ķ���
        if (displayMethod == 3)
        {
            memberscore[playerShown + 4] = memberscore[playerShown + 4] + score;
            At24c02Write(playerShown + 4 + 5, memberscore[playerShown + 4]);
            delay(2);
            teamscore[1] = teamscore[1] + score;
            At24c02Write(1, teamscore[1]);
        }
    }
}

void matrixkeyscan()
{
    int i;
    uchar temp;
    static int row = 0;
    if (row == 4)
        row = 0;
    switch (row)
    {
    case 0:
        /* 1110 ��P3^0���ͣ�ɨ���һ�� */
        P3 = 0xfe;
        break;
    case 1:
        /* 1101 ��P3^1���ͣ�ɨ��ڶ��� */
        P3 = 0xfd;
        break;
    case 2:
        /* 1011 ��P3^2���ͣ�ɨ������� */
        P3 = 0xfb;
        break;
    case 3:
        /* 0111 ��P3^3���ͣ�ɨ������� */
        P3 = 0xf7;
        /* 4��ɨ����ϣ�row���»�ȥɨ���һ�� */
        break;
    }

    /* ���ʵ�ʵ�P3���� */
    temp = P3;
    /* Ϊ�����жϣ�P3^0 ~ P3^3 ���������붼������Ч�ֻ�� P3^4 ~ P3^7 �Ƿ����� */
    temp = temp & 0xf0;
    /* ֻҪ P3^4 ~ P3^7 ����˿ڱ����ͣ����а����������� */
    if (temp != 0xf0)
    {
        /* ����ʱ10ms���ų����̶������µ�����Ӧ */
        delay(10);
        /* 10ms�����ж�һ�� P3 ��ʵ��ֵ */
        temp = P3;
        temp = temp & 0xf0;
        /* ������Ǳ����ͣ���ô����ı������� */
        while (temp != 0xf0)
        {
            /* ��ʼ�ж��ĸ�/Щ������������ */
            switch (temp)
            {
            /* 1110 �� P3^4 ����Ͻ�ļ������£���Ӧ��һ�� */
            case 0xe0:
                switch (row)
                {
                /* ��һ�� */
                case 0:
                    displayMethod = 0;
                    break;
                case 1:
                    /* A��0����Ա�÷� */
                    displayMethod = 2;
                    playerShown = 0;
                    break;
                case 2:
                    /* B��1����Ա�÷� */
                    displayMethod = 3;
                    playerShown = 0;
                    break;
                case 3:
                    addPlayerScore(1);
                    break;
                }
                break;
            /* 1101 �� P3^5 ����Ͻ�ļ������£���Ӧ�ڶ��� */
            case 0xd0:
                switch (row)
                {
                /* ��һ�� */
                case 0:
                    teamscore[0] = 0;
                    teamscore[1] = 0;
                    time = 0;
                    section = 1;
                    minute = 5;
                    for(i = 0; i < 8; i++) {
                        memberscore[i] = 0;
                        At24c02Write(i + 5, 0);
                        delay(2);
                    }
                    At24c02Write(2, section);
                    delay(2);
                    At24c02Write(0, 0);
                    delay(2);
                    At24c02Write(1, 0);
                    break;
                case 1:
                    /* A��1����Ա�÷� */
                    displayMethod = 2;
                    playerShown = 1;
                    break;
                case 2:
                    /* B��1����Ա�÷� */
                    displayMethod = 3;
                    playerShown = 1;
                    break;
                case 3:
                    addPlayerScore(2);
                    break;
                }
                break;
            /* 1011 �� P3^6 ����Ͻ�ļ������£���Ӧ������s */
            case 0xb0:
                switch (row)
                {
                /* ��һ�� */
                case 0:
                    break;
                case 1:
                    /* A��2����Ա�÷� */
                    displayMethod = 2;
                    playerShown = 2;
                    break;
                case 2:
                    /* B��2����Ա�÷� */
                    displayMethod = 3;
                    playerShown = 2;
                    break;
                case 3:
                    addPlayerScore(3);
                    break;
                }
                break;
            /* 0111 �� P3^7 ����Ͻ�ļ������£���Ӧ������ */
            case 0x70:
                switch (row)
                {
                /* ��һ�� */
                case 0:
                    /* �����ʱ����ʾ�ı������κ�ʣ��ʱ����Ϣ */
                    if (displayMethod == 1)
                    {
                        TR0 = ~TR0;
                        addScoreEN = ~addScoreEN;
                    }
                    displayMethod = 1;
                    break;
                case 1:
                    /* A��3����Ա�÷� */
                    displayMethod = 2;
                    playerShown = 3;
                    break;
                case 2:
                    /* B��3����Ա�÷� */
                    displayMethod = 3;
                    playerShown = 3;
                    break;
                case 3:
                    /* addPlayerScore(2); */
                    break;
                }

                break;
            }
            /* ����û��������£����������м���������ȴ��û��ͷ� */

            while (temp != 0xf0)
            {
                temp = P3;
                temp = temp & 0xf0;
                switch (displayMethod)
                {
                case 0:
                    /* ������������ʾģ�� */
                    showAllScores();
                    break;
                case 1:
                    /* ������������ʱ��ʾģ�� */
                    output_time();
                    break;
                case 2:
                    /* ��ʾA����Ա�÷� */
                    showPlayerScore(10);
                    break;
                case 3:
                    /* ��ʾB����Ա�÷� */
                    showPlayerScore(11);
                    break;
                }
                beep=0;
            }
            beep=1;
        }
    }
    row++;
}

void output_time()
{
    /*
        ��> Leo ��ע

        '''
        ����һ�²���ԭ�򣬵����ǵ���һ������ܺ���Ҫ�Ƚ���ѡ����λѡ��Ϊ��һ������ܵ�ֵ��������ͬʱ�ı䡣
        ���ȸı��ѡʱ����ô��ǰ����ܾͻ��ж��ݵ�ʱ����ʾ��һ������ܵ����֡����ȸı�λѡʱ����һ������ܾͻ��ж��ݵ�ʱ����ʾ��ǰ����ܵ����֡�
        ��ô��������ǣ��Ƚ���ѡ��0�������������ı�λѡ��ֵΪ��һ������ܣ�����ٸı��ѡֵ��ͬ����һ�ַ����ǣ��Ƚ�λѡ��ֵ 0xff ���ر���������ܣ��ı��ѡֵ�����ı�λѡֵ��
        '''

        ���ڹ�������ʦ����Ӱ��ͨ��������ʱ����ʵ�ֵģ��������ְ취ֻ�ܾ����ܽ��Ͳ�Ӱ�����ȶ��Һ����׵��������ˢ��Ƶ�ʹ�������˸��
        ����һƪ���ڵ�Ƭ�����������ʾԭ��ͨ�ⷽ�����濴����һ�ָ������Ƶ��������Ӱ����������ͨ���ڽ�����һ���������λѡ�Ͷ�ѡ����֮ǰ�ȹر��������ʾ���ﵽ��ȫ��Ӱ��Ч����
    */

    int i;
    for (i = 1; i <= 6; i++)
    {
        // ������õ��Ƿ�ֹ��һ�εĶ�ѡ��Ϣ������һ�εĶ�ѡ��������һ�£�������ȥ��Ӱ
        P0 = 0;
        dula = 1;
        dula = 0;

        //
        switch (i)
        {
        case 1:
            P0 = 0xfe;
            break;
        case 2:
            P0 = 0xfd;
            break;
        case 3:
            P0 = 0xfb;
            break;
        case 4:
            P0 = 0xf7;
            break;
        case 5:
            P0 = 0xef;
            break;
        case 6:
            P0 = 0xdf;
            break;
        }

        wela = 1;
        wela = 0;

        switch (i)
        {
        case 1:
            P0 = table[section];
            break;
        case 2:
            P0 = 0x40;
            break;
        case 3:
            P0 = table[minute / 10];
            break;
        case 4:
            P0 = table[minute % 10];
            break;
        case 5:
            P0 = table[time / 10];
            break;
        case 6:
            P0 = table[time % 10];
            break;
        }

        dula = 1;
        dula = 0;
        delay(1);
    }
		
	P0	= 0XFF;
	wela	= 1;
	P0	= 0xf7;
	wela	= 0;
	dula	= 1;
	P0	= 0x80;
	dula	= 0;
	delay( 2 );


    P0 = 0XFF;
    wela = 1;
    P0 = 0xff;
    wela = 0;
}
// ������ʾ AB �ӵĳɼ�
void showHundredScore()
{
    // cycleDisplay - ��ʱ���Ķ������ʾ�л�
    int i;
    for (i = 1; i <= 6; i++)
    {
        P0 = 0;
        dula = 1;
        dula = 0;

        switch (i)
        {
        case 1:
            P0 = 0xfe;
            break;
        case 2:
            P0 = 0xfd;
            break;
        case 3:
            P0 = 0xfb;
            break;
        case 4:
            P0 = 0xf7;
            break;
        case 5:
            P0 = 0xef;
            break;
        case 6:
            P0 = 0xdf;
            break;
        }

        wela = 1;
        wela = 0;

        switch (i)
        {
        // ������ A/b
        case 1:
            P0 = P0 = table[(int)cycleDisplay + 10];
            break;
        // "��"
        case 2:
            P0 = 0x40;
            break;
        // "��"
        case 3:
            P0 = 0x40;
            break;
        // �ɼ���λ
        case 4:
            P0 = table[teamscore[cycleDisplay] / 100];
            break;
        // �ɼ�ʮλ
        case 5:
            P0 = table[teamscore[cycleDisplay] % 100 / 10];
            break;
        // �ɼ���λ
        case 6:
            P0 = table[teamscore[cycleDisplay] % 10];
            break;
        }

        dula = 1;
        dula = 0;
        delay(1);
    }

    P0 = 0XFF;
    wela = 1;
    P0 = 0xff;
    wela = 0;
}

void showAllScores()
{
    // ���ӱȷ�ֻҪ��һ������100�������ٷ���ʾģ��
    if (teamscore[0] > 99 || teamscore[1] > 99)
        showHundredScore();
    else
    {
        int i;
        for (i = 1; i <= 6; i++)
        {
            P0 = 0;
            dula = 1;
            dula = 0;

            switch (i)
            {
            case 1:
                P0 = 0xfe;
                break;
            case 2:
                P0 = 0xfd;
                break;
            case 3:
                P0 = 0xfb;
                break;
            case 4:
                P0 = 0xf7;
                break;
            case 5:
                P0 = 0xef;
                break;
            case 6:
                P0 = 0xdf;
                break;
            }

            wela = 1;
            wela = 0;

            switch (i)
            {
            // �ɼ�ʮλ
            case 1:
                P0 = table[teamscore[0] / 10];
                break;
            // �ɼ���λ
            case 2:
                P0 = table[teamscore[0] % 10];
                break;
            // "��"
            case 3:
                P0 = 0x40;
                break;
            // "��"
            case 4:
                P0 = 0x40;
                break;
            // �ɼ�ʮλ
            case 5:
                P0 = table[teamscore[1] / 10];
                break;
            // �ɼ���λ
            case 6:
                P0 = table[teamscore[1] % 10];
                break;
            }

            dula = 1;
            dula = 0;
            delay(1);
        }

        P0 = 0XFF;
        wela = 1;
        P0 = 0xff;
        wela = 0;
    }
}

void delay(uint z)
{
    uint x, y;
    for (x = z; x > 0; x--)
        for (y = 114; y > 0; y--)
            ;
}

void TimerInit()
{
    int i;
    TMOD |= 0X01;
    TH0 = (65536 - fosc / N / 12) / 256;
    TL0 = (65536 - fosc / N / 12) % 256;
    EA = 1;
    ET0 = 1;
    TR0 = 0;
    // ��ȡ A �ӵ÷�
    teamscore[0] = At24c02Read(0);
    // �ȴ�2ms���ô洢���ܹ���ʱ����ɶ�ȡ
    delay(2);
    // ��ȡ B �ӵ÷�
    teamscore[1] = At24c02Read(1);
    delay(2);
    // ��ȡ������Ϣ
    section = At24c02Read(2);
    delay(2);
    // ��ȡ��󱣴�ļ�ʱ��������
    minute = At24c02Read(4);
    delay(2);
    // ��ȡ��󱣴�ļ�ʱ������
    time = At24c02Read(3);
    for (i=0;i<8;i++) {
        delay(2);
        // ��ȡ��Ա���Եĵ÷���Ϣ
        memberscore[i] = At24c02Read(i+5);
    }
}

void timer0() interrupt 1
{
    // ����ѭ����ʾ��ʱ
    static int cycleNum = 0;
    static int displayMethodbak = 0;
    num++;
    TH0 = (65536 - fosc / N / 12) / 256;
    TL0 = (65536 - fosc / N / 12) % 256;
    // 20���ж�����Ϊ1��
    if (num == 20)
    {
        cycleNum++;
        num = 0;
        // ����2���
        if (cycleNum == 2)
        {
            // ���ü�ʱ
            cycleNum = 0;
            // ��ת������ʾ
            cycleDisplay = ~cycleDisplay;
        }

        // ������ʱ
        if (displayMethod != 4)
        {
            // ��
            time--;
            At24c02Write(3, time);
            if (time == -1)
            {
                // ����
                minute = minute - 1;
                delay(2);
                At24c02Write(4, minute);
                if (minute == -1)
                {
                    // ���ݵ�ǰ�������ʾ��ģʽ
                    displayMethodbak = displayMethod;
                    // �޸��������ʾģʽΪ�г���Ϣģʽ
                    displayMethod = 4;
                    // ����һ��1���ӵ��г���Ϣ��ʱ
                    minute = 0;
                    section = section + 1;
                    delay(2);
                    At24c02Write(2, section);
                }
                time = 59;
            }
        }
        else
        {
            // �г���Ϣ��ʱ
            time--;
            if (time == -1)
            {
                // �ָ������֮ǰ��ʾ��ģʽ
                displayMethod = displayMethodbak;
                // ��ʼ�µ�һ��5���Ӽ�ʱ
                minute = 4;
                time = 59;
                delay(2);
                At24c02Write(4, minute);
            }
        }
    }
}

