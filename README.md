# xiaozhi-esp32-s3-ruler

我是一个超薄小智 AI 迷你卡片电脑。

## 如何刷固件

```
esptool.py write_flash -z 0x0 merged-binary_0x0.bin
```

## 小智源码修改和适配

修改了屏幕和 I2S 的 GPIO 配置，已经改好了编译即可。
参考: https://github.com/eggfly/xiaozhi-esp32

## 硬件配置

* ESP32-S3-PICO-1-N8R8 SIP, 8MB Flash (Quad SPI), 8 MB PSRAM (Octal SPI), 3.3V
* 1.47" ST7789 172*320 圆角屏 IPS
* MAX98357 功放 & 音腔喇叭
* INMP441 I2S 麦克风
* 一个红外发射 LED、一个红色充电 LED、一个白色 LED
* TP4054 线性充电 & 302040 锂电池
* SHT30-DIS-B 温湿度传感器
* BMP280 温度气压传感器
* BMI270 六轴运动传感器
* TCA8418 键盘扫描 IC & 35键 5*7 矩阵键盘
* TF 卡槽 & 兼容 SDMMC/SPI 接线
* 三排 2.54mm 扩展接口，可扩展串口、I2C、I2S 音频输出等外设

## GPIO 接线表

| GPIO  |  Device Pin   | Description |
| ------- | ---------- | --- |
| CHIP_PU | RESET | 键盘右下角按钮 |
| IO0 | BOOT | 键盘左下角按钮 |
| IO1  | I2C_SDA  | I2C 数据(外部上拉) |
| IO2  | I2C_SDA |  I2C 时钟(外部上拉) |
| IO3  | IR LED | 红外 LED |
| IO4  | MIC_WS | 麦克风 Word Select |
| IO5  | MIC_SCK | 麦克风时钟 |
| IO6  | MIC_DIN | 麦克风数据 |
| IO7  | SPK_DOUT | 喇叭数据 |
| IO8  | KB_INT | TCA8418 INT |
| IO9  | SD_D3 | TF卡 |
| IO10 | SD_D2 | TF卡 |
| IO11  | SD_D1 | TF卡 |
| IO12  | SD_D0 | TF卡 |
| IO13  | SD_CLK | TF卡 |
| IO14  | SD_CMD | TF卡 |
| IO15 | SPK_BCLK | 喇叭比特时钟 |
| IO16  | SPK_LRCK | 喇叭左右声道 |
| IO17  | DAC_SD_MODE | 默认左右声道1/2均分 |
| IO18  | HALF_VSYS | 分压，电池电压 ADC |
| IO19  | USB_DN | USB- |
| IO20  | USB_DP | USB+ |
| IO21  | LCD_SCLK | 屏幕 SPI 时钟 |
| IO39  | 扩展 IO | 2.54mm 扩展 |
| IO40  | LCD_DC | 屏幕 RS |
| IO41  | LCD_CS | 屏幕 SPI 片选 |
| IO42  | LCD_BACKLIGHT | PMOS 低电平背光亮 |
| IO43  | TX | 串口 |
| IO44  | RX | 串口 |
| IO45  | LCD_RST | 屏幕 EN |
| IO47  | LCD_MOSI | 屏幕 SPI 数据 |
| IO48  | BUILTIN_LED | 小智白色 LED |


## I2C
```
I2C 地址: SDA=1, SCL=2
// Wire.begin(1, 2);
-> Scanning for I2C devices ...
-> I2C device found at address 0x34 --> TCA8418 | Keyboard scan IC;
-> I2C device found at address 0x44
-> I2C device found at address 0x69
-> I2C device found at address 0x77
```

| Device  |  Address   | Description |
| ------- | ---------- | --- |
| TCA8418 | 52 (0x34)  | Keyboard scan IC |
| SHT30   | 68 (0x44)  | 温湿度传感器 |
| BMI270  | 105 (0x69) | 运动传感器 (BMI2_I2C_2ND_ADDR) |
| BMP280  | 119 (0x77) | 气压传感器 (BMP280_ADDR_1) |

