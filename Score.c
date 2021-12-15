#include <reg52.h>
#define uchar unsigned char
#define uint unsigned int
#define fosc 11059200ul
#define N 20
uchar code table[] = {0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x6f, 0x77, 0x7c, 0x39, 0x5e, 0x79, 0x71};
int teamscore[2] = {10,20};
int memberscore[8] = {0};
int minute, time, section, num, team;
//		程序设置
//		球员编号显示控制
int playerShown = 0;
//		数码管显示模块控制
int displayMethod = 1;
uchar addr = 0;
bit cycleDisplay = 0;
bit addScoreEN = 0;
//--定义使用的IO口--//
sbit I2C_SCL = P2 ^ 1;
sbit I2C_SDA = P2 ^ 0;
sbit dula = P2 ^ 6;
sbit wela = P2 ^ 7;
sbit beep = P2 ^ 3;
//--声明全局变量--//
void I2C_Delay10us();
void I2C_Start();                         //起始信号：在I2C_SCL时钟信号在高电平期间I2C_SDA信号产生一个下降沿
void I2C_Stop();                          //终止信号：在I2C_SCL时钟信号高电平期间I2C_SDA信号产生一个上升沿
uchar I2C_SendByte(uchar dat, uchar ack); //使用I2c读取一个字节
uchar I2C_ReadByte();                     //通过I2C发送一个字节。在I2C_SCL时钟信号高电平期间，保持发送信号I2C_SDA保持稳定
void TimerInit();                         // 初始化定时器和定时事件相关事件
void delay(uint z);                       // 延时函数
void matrixkeyscan();                     // 键盘扫描
void showPlayerScore(int team);           // 显示某个球员成绩
void addPlayerScore(int score);           // 为某个球员加分
void output_time();                       // 比赛时间显示
void showAllScores();                     // 总得分显示
void displayController();                 // 显示路由控制器
void midBreak();                          // 中场休息显示
void showWinner();                        // 胜者显示

/*
    2021-11-30 第1版
    初稿说明：
		实现了计分器的基本功能
		1-> 比赛场次和倒计时显示
		2-> 每个队员的加减分
		3-> 队员以及比赛总得分的显示功能

    2021-11-30 第2版
    2版说明：
        1-> 重写了部分会导致内存开销过大的不合理算法
        2-> 修复了堆栈不释放导致的单片机内存溢出而死机的问题

    2021-12-12 第3版
    3版说明：
        1-> 新增了中场休息功能
        2-> 新增了断电记忆功能
        3-> 精简算法，减少单片机的储存开销(数码管显示)
        4-> 修复了队员和总得分超过百分后显示异常的问题
        5-> 修复了数码管的消影效果不佳的问题（详见465行）
        6-> 在总得分超过100后，2个队伍的的得分将定时轮流显示（2秒轮换一次）
        断电记忆使用的是储存芯片为板子上的AT24C02芯片，储存速度较慢，有20KB的储存空间。

    注释 by Milhouse（张顺），Master Ruan（阮文博），Leo（沈嘉乐）
*/

void main()
{
    TimerInit();
    while (1)
    {
        /* 逐行扫描键盘 */
        matrixkeyscan();
        /* 数码管显示模块 */
        displayController();
    }
}

//函数功能: 往24c02的一个地址写入一个数据
void At24c02Write(unsigned char addr, unsigned char dat)
{
    I2C_Start();
    I2C_SendByte(0xa0, 1); //发送写器件地址
    I2C_SendByte(addr, 1); //发送要写入内存地址
    I2C_SendByte(dat, 0);  //发送数据
    I2C_Stop();
}

// 读取24c02的一个地址的一个数据
unsigned char At24c02Read(unsigned char addr)
{
    unsigned char num;
    I2C_Start();
    I2C_SendByte(0xa0, 1); //发送写器件地址
    I2C_SendByte(addr, 1); //发送要读取的地址
    I2C_Start();
    I2C_SendByte(0xa1, 1); //发送读器件地址
    num = I2C_ReadByte();  //读取数据
    I2C_Stop();
    return num;
}

void I2C_Delay10us()
{ }

// 起始信号：在I2C_SCL时钟信号在高电平期间I2C_SDA信号产生一个下降沿
void I2C_Start()
{
    I2C_SDA = 1;
    I2C_Delay10us();
    I2C_SCL = 1;
    I2C_Delay10us(); //建立时间是I2C_SDA保持时间>4.7us
    I2C_SDA = 0;
    I2C_Delay10us(); //保持时间是>4us
    I2C_SCL = 0;
    I2C_Delay10us();
}

//终止信号：在I2C_SCL时钟信号高电平期间I2C_SDA信号产生一个上升沿
void I2C_Stop()
{
    I2C_SDA = 0;
    I2C_Delay10us();
    I2C_SCL = 1;
    I2C_Delay10us(); //建立时间大于4.7us
    I2C_SDA = 1;
    I2C_Delay10us();
}

//通过I2C发送一个字节。在I2C_SCL时钟信号高电平期间， 保持发送信号I2C_SDA保持稳定
uchar I2C_SendByte(uchar dat, uchar ack)
{
    uchar a = 0, b = 0;     //最大255，一个机器周期为1us，最大延时255us。
    for (a = 0; a < 8; a++) //要发送8位，从最高位开始
    {
        I2C_SDA = dat >> 7; //起始信号之后I2C_SCL=0，所以可以直接改变I2C_SDA信号
        dat = dat << 1;
        I2C_Delay10us();
        I2C_SCL = 1;
        I2C_Delay10us(); //建立时间>4.7us
        I2C_SCL = 0;
        I2C_Delay10us(); //时间大于4us
    }
    I2C_SDA = 1;
    I2C_Delay10us();
    I2C_SCL = 1;
    while (I2C_SDA && (ack == 1)) //等待应答，也就是等待从设备把I2C_SDA拉低
    {
        b++;
        if (b > 200) //如果超过200us没有应答发送失败，或者为非应答，表示接收结束
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

// 使用I2c读取一个字节
uchar I2C_ReadByte()
{
    uchar a = 0, dat = 0;
    I2C_SDA = 1; //起始和发送一个字节之后I2C_SCL都是0
    I2C_Delay10us();
    for (a = 0; a < 8; a++) //接收8个字节
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
        /* 分流至分数显示模块 */
        showAllScores();
        break;
    case 1:
        /* 分流至比赛计时显示模块 */
        output_time();
        break;
    case 2:
        /* 显示A队球员得分 */
        showPlayerScore(10);
        break;
    case 3:
        /* 显示B队球员得分 */
        showPlayerScore(11);
        break;
    case 4:
        /* 中场休息 */
        midBreak();
        break;
    }
}
//胜者显示
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

// 中场休息显示
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
    int hundredMode = 0; // 百分显示模式

    if (team == 11)
    {
        // B 队球员索引从4开始
        index = playerShown + 4;
    }
    else
    {
        index = playerShown;
    }

    if (memberscore[index] > 99)
        hundredMode = 1; // 开启百分显示

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
                // 百分模式下，此位显示球员编号
                if (team == 11)
                    P0 = table[index - 3];
                else
                    P0 = table[index + 1];
            else
                // 正常模式下，此位显示 "—"
                P0 = 0x40;
            break;
        case 3:
            if (hundredMode)
                // 百分模式下，此位显示 "—"
                P0 = 0x40;
            else
                // 正常模式下，此位显示球员编号
                if (team == 11)
                    P0 = table[index - 3];
                else
                    P0 = table[index + 1];
            break;
        case 4:
            if (hundredMode)
                // 百分模式下下，此位显示成绩的百位
                P0 = table[memberscore[index] / 100];
            else
                // 正常模式下，此位显示 "—"
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
    // 加分使能开启
    if (addScoreEN == 1)
    {
        // A 队球员的分数
        if (displayMethod == 2)
        {
            memberscore[playerShown] = memberscore[playerShown] + score;
            // 加分操作完成以后，将个人得分写入储存
            At24c02Write(playerShown + 5, memberscore[playerShown]);
            delay(2);
            teamscore[0] = teamscore[0] + score;
            // 加分操作完成以后，将总分写入储存
            At24c02Write(0, teamscore[0]);
        }
        // B 队球员的分数
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
        /* 1110 将P3^0拉低，扫描第一行 */
        P3 = 0xfe;
        break;
    case 1:
        /* 1101 将P3^1拉低，扫描第二行 */
        P3 = 0xfd;
        break;
    case 2:
        /* 1011 将P3^2拉低，扫描第三行 */
        P3 = 0xfb;
        break;
    case 3:
        /* 0111 将P3^3拉低，扫描第四行 */
        P3 = 0xf7;
        /* 4行扫描完毕，row重新回去扫描第一行 */
        break;
    }

    /* 获得实际的P3输入 */
    temp = P3;
    /* 为方便判断，P3^0 ~ P3^3 所有行输入都视作无效项，只看 P3^4 ~ P3^7 是否被拉低 */
    temp = temp & 0xf0;
    /* 只要 P3^4 ~ P3^7 任意端口被拉低，则有按键被按下了 */
    if (temp != 0xf0)
    {
        /* 先延时10ms，排除键盘抖动导致的误响应 */
        delay(10);
        /* 10ms后再判断一下 P3 的实际值 */
        temp = P3;
        temp = temp & 0xf0;
        /* 如果还是被拉低，那么是真的被按下了 */
        while (temp != 0xf0)
        {
            /* 开始判断哪个/些按键被按下了 */
            switch (temp)
            {
            /* 1110 即 P3^4 所管辖的键被按下，对应第一列 */
            case 0xe0:
                switch (row)
                {
                /* 第一行 */
                case 0:
                    displayMethod = 0;
                    break;
                case 1:
                    /* A队0号球员得分 */
                    displayMethod = 2;
                    playerShown = 0;
                    break;
                case 2:
                    /* B队1号球员得分 */
                    displayMethod = 3;
                    playerShown = 0;
                    break;
                case 3:
                    addPlayerScore(1);
                    break;
                }
                break;
            /* 1101 即 P3^5 所管辖的键被按下，对应第二列 */
            case 0xd0:
                switch (row)
                {
                /* 第一行 */
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
                    /* A队1号球员得分 */
                    displayMethod = 2;
                    playerShown = 1;
                    break;
                case 2:
                    /* B队1号球员得分 */
                    displayMethod = 3;
                    playerShown = 1;
                    break;
                case 3:
                    addPlayerScore(2);
                    break;
                }
                break;
            /* 1011 即 P3^6 所管辖的键被按下，对应第三列s */
            case 0xb0:
                switch (row)
                {
                /* 第一行 */
                case 0:
                    break;
                case 1:
                    /* A队2号球员得分 */
                    displayMethod = 2;
                    playerShown = 2;
                    break;
                case 2:
                    /* B队2号球员得分 */
                    displayMethod = 3;
                    playerShown = 2;
                    break;
                case 3:
                    addPlayerScore(3);
                    break;
                }
                break;
            /* 0111 即 P3^7 所管辖的键被按下，对应第四列 */
            case 0x70:
                switch (row)
                {
                /* 第一行 */
                case 0:
                    /* 输出计时器显示的比赛场次和剩余时间信息 */
                    if (displayMethod == 1)
                    {
                        TR0 = ~TR0;
                        addScoreEN = ~addScoreEN;
                    }
                    displayMethod = 1;
                    break;
                case 1:
                    /* A队3号球员得分 */
                    displayMethod = 2;
                    playerShown = 3;
                    break;
                case 2:
                    /* B队3号球员得分 */
                    displayMethod = 3;
                    playerShown = 3;
                    break;
                case 3:
                    /* addPlayerScore(2); */
                    break;
                }

                break;
            }
            /* 如果用户持续按下，则不跳出本行监听，程序等待用户释放 */

            while (temp != 0xf0)
            {
                temp = P3;
                temp = temp & 0xf0;
                switch (displayMethod)
                {
                case 0:
                    /* 分流至分数显示模块 */
                    showAllScores();
                    break;
                case 1:
                    /* 分流至比赛计时显示模块 */
                    output_time();
                    break;
                case 2:
                    /* 显示A队球员得分 */
                    showPlayerScore(10);
                    break;
                case 3:
                    /* 显示B队球员得分 */
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
        —> Leo 备注

        '''
        分析一下产生原因，当我们点亮一个数码管后，总要先将段选或者位选改为下一个数码管的值，而不能同时改变。
        当先改变段选时，那么当前数码管就会有短暂的时间显示下一个数码管的数字。当先改变位选时，下一个数码管就会有短暂的时间显示当前数码管的数字。
        那么解决方法是，先将段选置0送入锁存器，改变位选的值为下一个数码管，最后再改变段选值。同理另一种方法是，先将位选赋值 0xff 即关闭所有数码管，改变段选值，最后改变位选值。
        '''

        由于郭天祥老师的消影是通过增加延时函数实现的，但是这种办法只能尽可能降低残影的亮度而且很容易导致数码管刷新频率过慢而闪烁。
        我在一篇关于单片机的数码管显示原理通解方法里面看到了一种更加完善的数码管消影方法，可以通过在进行下一个数码管送位选和段选数据之前先关闭数码管显示来达到完全消影的效果。
    */

    int i;
    for (i = 1; i <= 6; i++)
    {
        // 这里采用的是防止上一次的段选信息窜入下一次的段选，先重置一下，彻底消去残影
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
// 轮流显示 AB 队的成绩
void showHundredScore()
{
    // cycleDisplay - 定时器的队伍的显示切换
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
        // 队伍名 A/b
        case 1:
            P0 = P0 = table[(int)cycleDisplay + 10];
            break;
        // "—"
        case 2:
            P0 = 0x40;
            break;
        // "—"
        case 3:
            P0 = 0x40;
            break;
        // 成绩百位
        case 4:
            P0 = table[teamscore[cycleDisplay] / 100];
            break;
        // 成绩十位
        case 5:
            P0 = table[teamscore[cycleDisplay] % 100 / 10];
            break;
        // 成绩个位
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
    // 两队比分只要有一个超过100，则进入百分显示模块
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
            // 成绩十位
            case 1:
                P0 = table[teamscore[0] / 10];
                break;
            // 成绩个位
            case 2:
                P0 = table[teamscore[0] % 10];
                break;
            // "—"
            case 3:
                P0 = 0x40;
                break;
            // "—"
            case 4:
                P0 = 0x40;
                break;
            // 成绩十位
            case 5:
                P0 = table[teamscore[1] / 10];
                break;
            // 成绩个位
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
    // 读取 A 队得分
    teamscore[0] = At24c02Read(0);
    // 等待2ms以让存储器能够有时间完成读取
    delay(2);
    // 读取 B 队得分
    teamscore[1] = At24c02Read(1);
    delay(2);
    // 读取场次信息
    section = At24c02Read(2);
    delay(2);
    // 读取最后保存的计时器分钟数
    minute = At24c02Read(4);
    delay(2);
    // 读取最后保存的计时器秒数
    time = At24c02Read(3);
    for (i=0;i<8;i++) {
        delay(2);
        // 读取球员各自的得分信息
        memberscore[i] = At24c02Read(i+5);
    }
}

void timer0() interrupt 1
{
    // 队伍循环显示计时
    static int cycleNum = 0;
    static int displayMethodbak = 0;
    num++;
    TH0 = (65536 - fosc / N / 12) / 256;
    TL0 = (65536 - fosc / N / 12) % 256;
    // 20次中断正好为1秒
    if (num == 20)
    {
        cycleNum++;
        num = 0;
        // 到达2秒后
        if (cycleNum == 2)
        {
            // 重置计时
            cycleNum = 0;
            // 反转队伍显示
            cycleDisplay = ~cycleDisplay;
        }

        // 正常计时
        if (displayMethod != 4)
        {
            // 秒
            time--;
            At24c02Write(3, time);
            if (time == -1)
            {
                // 分钟
                minute = minute - 1;
                delay(2);
                At24c02Write(4, minute);
                if (minute == -1)
                {
                    // 备份当前数码管显示的模式
                    displayMethodbak = displayMethod;
                    // 修改数码管显示模式为中场休息模式
                    displayMethod = 4;
                    // 开启一个1分钟的中场休息计时
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
            // 中场休息计时
            time--;
            if (time == -1)
            {
                // 恢复数码管之前显示的模式
                displayMethod = displayMethodbak;
                // 开始新的一轮5分钟计时
                minute = 4;
                time = 59;
                delay(2);
                At24c02Write(4, minute);
            }
        }
    }
}

