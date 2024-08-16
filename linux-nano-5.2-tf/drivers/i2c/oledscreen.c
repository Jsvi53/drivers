#include <linux/module.h>          // 包含模块编写所需的基本头文件
#include <linux/kernel.h>          // 包含内核函数的头文件
#include <linux/fs.h>              // 包含文件系统操作相关的头文件
#include <linux/init.h>            // 包含初始化函数的宏定义
#include <asm/io.h>                // 包含I/O操作相关的函数，如ioremap和iounmap
#include <asm/uaccess.h>           // 包含用户空间和内核空间之间数据拷贝的函数
#include <linux/device.h>          // 包含设备类相关的函数
#include <linux/cdev.h>            // 包含字符设备操作相关的函数
#include <linux/platform_device.h> // 包含平台设备相关的函数
#include <linux/of.h>              // 包含设备树操作相关的函数
#include <linux/i2c.h>             // 包含I2C通信相关的函数
#include <linux/kobject.h>         // 包含内核对象操作相关的函数
#include <linux/sysfs.h>           // 包含系统文件系统(sysfs)操作相关的函数
#include <linux/uaccess.h>         // 包含用户空间和内核空间之间数据拷贝的函数
#include "oledscreen.h"            // 包含oledscreen.h头文件

#define SH1107_CMD 0 // 写命令
#define SH1107_DATA 1 // 写数据
#define USE_HORIZONTAL 90

// 定义一个用于接收数据的字符数组
static char recv[255] = {0};
u8 OLED_GRAM[96][16];

// 定义指向I2C设备的指针
struct i2c_client *sh1107_dev;

// 定义一个函数，用于向I2C设备写入一个字节的数据
static void sh1107_write_byte(u8 chData, u8 chCmd)
{
    u8 cmd = 0x00;
    if (chCmd) {
        cmd = 0x40;
    } else {
        cmd = 0x00;
    }
    i2c_smbus_write_byte_data(sh1107_dev, cmd, chData);
}

// 开启OLED显示
void sh1107_display_on(void)
{
    sh1107_write_byte(0x8D, SH1107_CMD);    // 开启电荷泵
    sh1107_write_byte(0x14, SH1107_CMD);    // 设置电荷泵，开启电荷泵
    sh1107_write_byte(0xAF, SH1107_CMD);    // 开启OLED面板
}

// 关闭OLED显示
void sh1107_display_off(void)
{
    sh1107_write_byte(0x8D, SH1107_CMD);    // 开启电荷泵
    sh1107_write_byte(0x10, SH1107_CMD);    // 设置电荷泵，关闭电荷泵
    sh1107_write_byte(0xAE, SH1107_CMD);    // 关闭OLED面板
}

// 更新显存到OLED
void sh1107_refresh_gram(void)
{
    u8 i, j;

    for (i = 0; i < 16; i ++) {
        sh1107_write_byte(0xB0 + i, SH1107_CMD);    // 设置页地址
        sh1107_write_byte(0x00, SH1107_CMD);        // 设置列地址的低地址
        sh1107_write_byte(0x10, SH1107_CMD);        // 设置列地址的高地址
        for (j = 0; j < 80; j ++) {
            sh1107_write_byte(OLED_GRAM[j][i], SH1107_DATA);
        }
    }
}

// 初始化OLED
void sh1107_clear_screen(u8 chFill)    // 清屏函数，chFill为填充参数，0x00或0xff
{
    memset(OLED_GRAM,chFill, sizeof(OLED_GRAM));    // 将显示缓冲区清零，memset函数原型：void *memset(void *s, int ch, size_t n);
    sh1107_refresh_gram();
}

// 设置坐标，范围0~127, chPoint 0:关闭点，1:开启点
void sh1107_draw_point(u8 chXpos, u8 chYpos, u8 chPoint)
{
    u8 chPos, chBx, chTemp = 0;

    if (chXpos > 127 || chYpos > 63) {
        return;
    }
    chPos = 7 - chYpos / 8; //
    chBx = chYpos % 8;
    chTemp = 1 << (7 - chBx);

    if (chPoint) {
        OLED_GRAM[chXpos][chPos] |= chTemp;

    } else {
        OLED_GRAM[chXpos][chPos] &= ~chTemp;
    }
}

/**
  * @brief  填充矩形
  *
  * @param  chXpos1: 矩形的左上角的X坐标
  * @param  chYpos1: 矩形的左上角的Y坐标
  * @param  chXpos2: 矩形的右下角的X坐标
  * @param  chYpos3: 矩形的右下角的Y坐标
  * @retval None
**/
void sh1107_fill_screen(uint8_t chXpos1, uint8_t chYpos1, uint8_t chXpos2, uint8_t chYpos2, uint8_t chDot)
{
    uint8_t chXpos, chYpos;

    for (chXpos = chXpos1; chXpos <= chXpos2; chXpos ++) {
        for (chYpos = chYpos1; chYpos <= chYpos2; chYpos ++) {
            sh1107_draw_point(chXpos, chYpos, chDot);
        }
    }

    sh1107_refresh_gram();
}

/**
  * @brief 在指定位置显示一个字符
  *
  * @param  chXpos: 显示字符的横坐标
  * @param  chYpos: 显示字符的纵坐标
  * @param  chSize: 字体大小
  * @param  chMode：模式
  * @retval None
**/
void sh1107_display_char(uint8_t chXpos, uint8_t chYpos, uint8_t chChr, uint8_t chSize, uint8_t chMode)
{
    uint8_t i, j;
    uint8_t chTemp, chYpos0 = chYpos;

    chChr = chChr - ' ';
    for (i = 0; i < chSize; i ++) {
        if (chMode) {
            chTemp = c_chFont1608[chChr][i];
        } else {
            chTemp = ~c_chFont1608[chChr][i];
        }

        for (j = 0; j < 8; j ++) {
            if (chTemp & 0x80) {
                sh1107_draw_point(chXpos, chYpos, 1);
            } else {
                sh1107_draw_point(chXpos, chYpos, 0);
            }
            chTemp <<= 1;
            chYpos ++;

            if ((chYpos - chYpos0) == chSize) {
                chYpos = chYpos0;
                chXpos ++;
                break;
            }
        }
    }
}

// 更新显存到OLED
void OLED_Refresh(void)
{
	u8 i, n;
	for (i = 0; i < 16; i++)
	{
        sh1107_write_byte(0xB0 + i, SH1107_CMD);    // 设置页地址
        sh1107_write_byte(0x00, SH1107_CMD);        // 设置列地址的低地址
        sh1107_write_byte(0x10, SH1107_CMD);        // 设置列地址的高地址
		for (n = 0; n < 80; n++){
            sh1107_write_byte(OLED_GRAM[n][i], SH1107_DATA);
        }
	}
}

// 开启OLED显示
void OLED_DisPlay_On(void)
{
	sh1107_write_byte(0x8D, SH1107_CMD); // 电荷泵使能
	sh1107_write_byte(0x14, SH1107_CMD); // 开启电荷泵
	sh1107_write_byte(0xAF, SH1107_CMD); // 点亮屏幕
}

// 关闭OLED显示
void OLED_DisPlay_Off(void)
{
	sh1107_write_byte(0x8D, SH1107_CMD); // 电荷泵使能
	sh1107_write_byte(0x10, SH1107_CMD); // 关闭电荷泵
	sh1107_write_byte(0xAE, SH1107_CMD); // 关闭屏幕
}

// 反显函数
void OLED_ColorTurn(u8 i)
{
	if (i == 0)
	{
		sh1107_write_byte(0xA6, SH1107_CMD); // 正常显示
	}
	if (i == 1)
	{
		sh1107_write_byte(0xA7, SH1107_CMD); // 反色显示
	}
}

// 屏幕旋转180度
void OLED_DisplayTurn(u8 i)
{
	if (i == 0)
	{
		sh1107_write_byte(0xD3, SH1107_CMD); /*set display offset*/
		sh1107_write_byte(0x68, SH1107_CMD); /*18*/
		sh1107_write_byte(0xC0, SH1107_CMD); // 正常显示
		sh1107_write_byte(0xA0, SH1107_CMD);
	}
	if (i == 1)
	{
		sh1107_write_byte(0xD3, SH1107_CMD); /*set display offset*/
		sh1107_write_byte(0x18, SH1107_CMD); /*18*/
		sh1107_write_byte(0xC8, SH1107_CMD); // 反转显示
		sh1107_write_byte(0xA1, SH1107_CMD);
	}
}

// 清屏函数
void OLED_Clear(void)
{
	u8 i, n;
	for (i = 0; i < 16; i++)
	{
		for (n = 0; n < 80; n++)
		{
			OLED_GRAM[n][i] = 0; // 清除所有数据
		}
	}
	OLED_Refresh(); // 更新显示
}

void OLED_DrawPoint(uint8_t x, uint8_t y, uint8_t t)
{
    uint8_t i, m, n;
    uint8_t x0 = x, y0 = y;

    // 根据屏幕方向调整坐标
    if (USE_HORIZONTAL == 90)
    {
        x = 79 - y0;
        y = x0;
    }
    else if (USE_HORIZONTAL == 270)
    {
        x = y0;
        y = 127 - x0;
    }

    // 计算在GRAM中的行和列
    i = y / 8;
    m = y % 8;
    n = 1 << m;

    // 根据像素状态进行绘制
    if (t)
    {
        OLED_GRAM[x][i] |= n;
    }
    else
    {
        OLED_GRAM[x][i] = ~OLED_GRAM[x][i];
        OLED_GRAM[x][i] |= n;
        OLED_GRAM[x][i] = ~OLED_GRAM[x][i];
    }
}


// 在指定位置显示一个字符,包括部分字符
// x:0~127
// y:0~63
// size1:选择字体 6x8/6x12/8x16/12x24
// mode:0,反色显示;1,正常显示
void OLED_ShowChar(u8 x, u8 y, u8 chr, u8 size1, u8 mode)
{
    u8 i, m, temp, size2, chr1;  // temp:字模数据,chr1:字符在字库中的偏移位置,size2:字库中的字节数,即一个字符占多少字节
    u8 x0 = x, y0 = y;
    if (size1 == 8)
        size2 = 6;
    else
        size2 = (size1 / 8 + ((size1 % 8) ? 1 : 0)) * (size1 / 2); // 得到字体一个字符对应点阵集所占的字节数
    chr1 = chr - ' ';                                              // 计算偏移后的值
    for (i = 0; i < size2; i++)
    {
        if (size1 == 8)
        {
            temp = asc2_0806[chr1][i];
        } // 调用0806字体
        else if (size1 == 12)
        {
            temp = asc2_1206[chr1][i];
        } // 调用1206字体
        else if (size1 == 16)
        {
            temp = asc2_1608[chr1][i];
        } // 调用1608字体
        else if (size1 == 24)
        {
            temp = asc2_2412[chr1][i];
        } // 调用2412字体
        else
            return;
        for (m = 0; m < 8; m++)
        {
            if (temp & 0x01)
                OLED_DrawPoint(x, y, mode);
            else
                OLED_DrawPoint(x, y, !mode);
            temp >>= 1;
            y++;
        }
        x++;
        if ((size1 != 8) && ((x - x0) == size1 / 2))
        {
            x = x0;
            y0 = y0 + 8;
        }
        y = y0;
    }
}

// 显示字符串
// x,y:起点坐标, u8是无符号整型,即0~255
// size1:字体大小
//*chr:字符串起始地址
// mode:0,反色显示;1,正常显示
void OLED_ShowString(u8 x, u8 y, u8 *chr, u8 size1, u8 mode)
{
    while ((*chr >= ' ') && (*chr <= '~')) // 判断是不是非法字符!
    {
        OLED_ShowChar(x, y, *chr, size1, mode);
        if (size1 == 8)
            x += 6;
        else
            x += size1 / 2;
        chr++;
    }
}

// 初始化OLED
void sh1107_init(void)
{
    sh1107_write_byte(0xAE, SH1107_CMD);    // 关闭显示
    sh1107_write_byte(0x00, SH1107_CMD);    // 设置低列地址
    sh1107_write_byte(0x10, SH1107_CMD);    // 设置高列地址
    sh1107_write_byte(0x20, SH1107_CMD);    // 设置内存地址模式
    sh1107_write_byte(0x81, SH1107_CMD);    // 设置对比度
    sh1107_write_byte(0x6f, SH1107_CMD);    // 设置对比度
    sh1107_write_byte(0xA0, SH1107_CMD);    // 设置段重映射
    sh1107_write_byte(0xC0, SH1107_CMD);    // 设置扫描方向
    sh1107_write_byte(0xA4, SH1107_CMD);    // 全局显示开启
    sh1107_write_byte(0xA6, SH1107_CMD);    // 设置显示方式
    sh1107_write_byte(0xD5, SH1107_CMD);    // 设置时钟分频因子
    sh1107_write_byte(0x91, SH1107_CMD);
    sh1107_write_byte(0xD9, SH1107_CMD);    // 设置预充电周期
    sh1107_write_byte(0x22, SH1107_CMD);
    sh1107_write_byte(0xDB, SH1107_CMD);    // 设置VCOMH
    sh1107_write_byte(0x3f, SH1107_CMD);
    sh1107_write_byte(0xA8, SH1107_CMD);    // 设置驱动路数
    sh1107_write_byte(0x4F, SH1107_CMD);
    sh1107_write_byte(0xD3, SH1107_CMD);    // 设置显示偏移
    sh1107_write_byte(0x68, SH1107_CMD);    // 设置显示偏移
    sh1107_write_byte(0xDC, SH1107_CMD);    // 设置显示开始行
    sh1107_write_byte(0x00, SH1107_CMD);
    sh1107_write_byte(0xAD, SH1107_CMD);    // 设置电荷泵
    sh1107_write_byte(0x8A, SH1107_CMD);    // 设置电荷泵
    sh1107_write_byte(0xAF, SH1107_CMD);    // 开启显示
    sh1107_display_on();
    OLED_Clear();
}

// 定义一个用于读取EEPROM数据的函数
static ssize_t sh1107_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    int ret;
    // 这里写入要读
    ret = i2c_smbus_write_byte(sh1107_dev, 0x02);
    if (ret)
    {
        // 如果写地址失败，打印错误信息
        printk(KERN_ERR "write addr failed!\n");
    }
    // 从EEPROM读取一个字节的数据
    recv[0] = i2c_smbus_read_byte(sh1107_dev);
    // 将读取到的数据格式化为十六进制字符串，并返回写入到缓冲区的长度
    return sprintf(buf, "read data = 0x%x\n", recv[0]);
}

// 定义一个用于向EEPROM写入数据的函数
static ssize_t sh1107_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    int ret;
    // 定义要写入的数据，这里为0x2f
    // 一共可以显示168字
    unsigned char data[168];
    copy_from_user(data, buf, 168);   // 从用户空间拷贝数据到内核空间, buf是用户空间的数据缓冲区
    // 将数据写入EEPROM
    OLED_ShowString(0, 32, data, 16, 1);

    if(ret < 0)
    {
        // 如果写入数据失败，打印错误信息
        printk(KERN_ERR "write data failed!\n");
    }
    // 返回写入的字节数
    return count;
}

// 定义一个sysfs文件属性，它关联了sh1107_show和sh1107_store函数
static DEVICE_ATTR(sh1107, 0660, sh1107_show, sh1107_store);

// 定义当I2C设备被探测到时执行的函数
static int sh1107_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int ret;
    // 创建一个新的属性文件，用于与EEPROM交互
    ret = device_create_file(&client->dev, &dev_attr_sh1107);
    if (ret != 0)
    {
        // 如果创建文件失败，打印信息并返回错误
        printk(KERN_INFO "create sh1107_dev file failed!\n");
        return -1;
    }
    // 将探测到的I2C设备指针赋值给全局变量
    sh1107_dev = client;

    // 初始化OLED
    sh1107_init();
    OLED_Clear();
    OLED_ShowString(0,6,"Planc-Pi", 24,1);
    OLED_ShowString(0,48,"2023/10/31",16,1);
    OLED_Refresh();
    OLED_DisPlay_On();

    // 打印探测到的设备信息
    printk(KERN_INFO "sh1107_dev detected!\n");
    return 0;
}

// 定义当I2C设备被移除时执行的函数
static int sh1107_remove(struct i2c_client *client)
{
    // 删除属性文件
    device_remove_file(&client->dev, &dev_attr_sh1107);
    // 打印移除设备的信息
    printk(KERN_INFO "exit sysfs sh1107!\n");
    return 0;
}

//sh1107_id是设备ID表，用于识别设备，0是设备ID，和sh1107_of_match的区别是这个是用于非设备树的
static const struct i2c_device_id sh1107_id[] = {
    {"solomon,sh1107", 0},
    {}};

//sh1107_of_match是设备树ID表，用于识别设备，和sh1107_id的区别是这个是用于设备树的
static const struct of_device_id sh1107_of_match[] = {
    {.compatible = "solomon,sh1107"},
    {},
};

// 将设备树ID表与模块关联
MODULE_DEVICE_TABLE(of, sh1107_of_match);

// 定义I2C驱动的结构体
static struct i2c_driver sh1107_driver = {
    .driver = {
        .name = "solomon,sh1107",            // 设备名称, test是厂商名，sh1107是设备名
        .owner = THIS_MODULE,              // 模块所有者
        .of_match_table = sh1107_of_match, // 设备树匹配表
    },
    .probe = sh1107_probe,   // 探测函数
    .remove = sh1107_remove, // 移除函数
    .id_table = sh1107_id,   // 设备ID表
};

// 注册I2C驱动
module_i2c_driver(sh1107_driver);

// 模块的初始化和退出函数
// static int sh1107_driver_init(void)
// {
//     platform_driver_register(&sh1107_driver);
//     printk(KERN_INFO "sh1107 driver init\n");
//     return 0;
// }
// static void sh1107_driver_exit(void)
// {
//     platform_driver_unregister(&sh1107_driver);
//     printk(KERN_INFO "sh1107 driver exit\n");
// }

// // 模块的初始化和退出函数
// module_init(sh1107_driver_init);
// module_exit(sh1107_driver_exit);

// 模块元数据
MODULE_LICENSE("GPL");               // 许可证
MODULE_AUTHOR("33213037@qq.com");         // 作者信息
MODULE_VERSION("0.1");               // 版本号
MODULE_DESCRIPTION("sh1107_driver"); // 模块描述