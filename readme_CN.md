# GPS导航系统 - 基于Arduino和TFT显示屏
基于ESP32-2432S028（Cheap Yellow Display）和SR2631U10 GPS模块的车载GPS仪项目源码
使用Arduino IDE构建的GPS导航系统，在TFT屏幕上实时显示位置跟踪、方向指示、速度监控和卫星信息。

![Finished](https://github.com/user-attachments/assets/103fac04-0cad-4439-b8ed-2e0b84913fa9)

## 项目概述

该项目使用ESP32-2432S028（CYD）和SR2631U10 GPS模块实现了一个GPS导航显示系统。系统接收GPS数据，进行处理，并显示关键导航信息，包括：
- 经纬度坐标
- 海拔高度
- 当前速度
- 指南针方向（低速时具有冻结功能）
- 当前时间和日期（转换为北京时间）
- 卫星数量和信号精度

## 硬件需求

- 带有2.8寸显示屏（320x240分辨率）的ESP32-2432S028开发板，基于ESP32-D0WDQ6控制器（被称为Cheap Yellow Display，CYD）
- GPS模块（支持NMEA协议），我用的是SR2631U10，它被广泛的用在穿越机（FPV）上
- UART串口转四杜邦连接线
- Type-c或Mini-USB电源

## 软件需求

- Arduino IDE
- 所需库：
  - TFT_eSPI.h（用于TFT显示）
  - HardwareSerial.h（用于串行通信）
  - TinyGPSPlus.h（用于GPS数据解析）
  - math.h（用于数学计算）

## 安装说明

1. 克隆或下载此仓库到本地计算机
2. 打开Arduino IDE
3. 通过库管理器安装所需库
4. 打开`GPSnavi_based_on_CYD_and_SR2631U10.ino`文件
5. 如有需要，在User_Setup.h文件中配置TFT显示屏设置
6. 将代码上传到ESP32-2432S028开发板

## GPS和TFT显示屏UART接口引脚配置

| 组件 | 引脚 |
|------|------|
| GPS RX | 22 |
| GPS TX | 27 |
| VCC | 3.3V |
| GND | GND |

## 使用说明

1. 根据引脚配置连接所有硬件组件
2. 启动系统
3. 等待GPS模块获取卫星信号（将显示搜索屏幕）
4. 一旦获取到卫星信号，将显示主导航屏幕
5. 系统将自动更新实时GPS数据

## 功能特性

### 显示面板

#### 左侧面板
- 经纬度坐标
- 海拔高度（以米为单位）
- 当前速度（以公里/小时为单位）

#### 右侧面板
- 指南针方向及角度差
- 当前时间（北京时间）
- 当前日期

#### 头部区域
- 卫星数量
- 位置精度（以米为单位）
- 卫星图标指示器

### 方向系统
- 基于航向角实时计算方向
- 低速（< 4 km/h）时的方向冻结功能
- 显示主方向（北、东、南、西）
- 当角度差> 10°时显示次要方向

### 时间和日期
- 自动从UTC转换为北京时间
- 跨时区边界的正确日期处理
- 高效的时间显示更新（仅在必要时更改）

### GPS功能
- 实时位置跟踪
- 速度监控
- 海拔高度显示
- 卫星数量和精度信息
- 带状态更新的信号搜索屏幕

## 代码结构

### 主要函数

- `setup()`: 初始化硬件和显示屏
- `loop()`: 主程序循环，处理GPS数据解析和显示更新
- `parseGPSData()`: 处理来自模块的原始GPS数据
- `updateLeftPanel()`: 更新位置和速度信息
- `updateRightPanel()`: 更新方向、时间和日期信息
- `getDirection()`: 从航向角计算指南针方向
- `utcToBeijingTime()`: 将UTC时间转换为北京时间
- `utcToBeijingDate()`: 将UTC日期转换为北京日期
- `showSearchingScreen()`: 显示GPS信号搜索屏幕
- `switchToMainUI()`: 从搜索屏幕切换到主导航屏幕

### 配置常量

代码包含各种可修改的配置常量：

- GPS波特率和引脚分配
- 显示组件的更新间隔
- UI元素的颜色定义
- 面板区域尺寸

##关于头文件nissan_logo.h和satellite_icon_bw.h

nissan_logo.h: 我的车是Nissan X-Trail，故创建了一个包含Nissan标志的位图，用于在搜索屏幕上显示
可通过位图生成器来生成你喜欢的图标
satellite_icon_bw.h: 包含黑白卫星图标位图数据，用于在主导航屏幕上显示

## 定制提示

1. **时区设置**：修改`utcToBeijingTime()`和`utcToBeijingDate()`函数，使用您的本地时区代替北京时间
2. **显示颜色**：更新"颜色定义"部分中的颜色定义，以更改UI外观
3. **更新间隔**：调整`UPDATE_INTERVAL`和`SEARCH_UPDATE_INTERVAL`以更改显示更新频率
4. **屏幕旋转**：更改`TFT_ROTATION`以调整屏幕方向
5. **方向冻结速度**：修改`updateRightPanel()`中方向冻结发生的速度阈值

## 故障排除

### GPS信号问题
- 确保GPS模块能清晰看到天空
- 检查GPS模块接线和波特率设置
- 初始时等待几分钟，让GPS模块获取卫星信号

### 显示问题
- 验证TFT显示屏连接
- 检查特定TFT显示屏型号的User_Setup.h配置
- 确保使用`TFT_ROTATION`设置了正确的旋转

### 串行通信问题
- 验证GPS RX/TX引脚分配
- 检查GPS波特率是否与模块配置匹配

## 许可证

该项目是开源的，可在MIT许可证下使用。

## 致谢

- 使用TinyGPSPlus库进行GPS数据解析
- 使用TFT_eSPI库进行TFT显示控制
- 感谢中国发达的制造业，提供了价格低廉、品质优良的组件

