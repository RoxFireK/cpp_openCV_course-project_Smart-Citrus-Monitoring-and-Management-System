# 🍊 智能柑橘果园管理系统 (Smart Orchard Management System)

这是一个基于 OpenCV 的计算机视觉项目，用于自动检测柑橘（橘子）图像中的病斑，判断其品质（好/坏），并统计坏果数量与病害严重程度。

## 📋 项目简介

本项目通过图像处理技术，实现了以下功能：

-   **颜色过滤**：利用 HSV 色彩空间精准提取橘子轮廓。
-   **形状匹配与轮廓检测**：识别橘子外部形状并定位内部病斑区域。
-   **品质评估**：根据病斑数量自动判断橘子品质（好/坏）及病害程度。
-   **数据管理**：提供控制台交互界面，支持批量图片检测、坏果统计和详情查询。

## 🛠️ 环境配置与依赖

### 核心依赖
- **C++ 编译器**：支持 C++11 及以上标准
  - Windows: Visual Studio 2019/2022 或 MinGW-w64
  - Linux: GCC 7+ 
  - macOS: Clang 10+
- **OpenCV 4.10+：图像处理核心库（需要手动下载安装）
- **CMake 3.10+：跨平台构建工具

### 手动安装 OpenCV (不使用包管理器)

#### Windows 平台

1. **下载 OpenCV**
   - 访问 [OpenCV 官方 Releases](https://github.com/opencv/opencv/releases)
   - 下载 Windows 版本（如 `opencv-4.x.x-windows.exe`）
   - 双击运行解压到指定目录，例如 `C:\opencv`

2. **配置环境变量**
   - 将 `C:\opencv\build\x64\vc15\bin` 添加到系统 PATH 环境变量
   - 重启命令行或 IDE 使配置生效

3. **Visual Studio 配置（推荐）**
   - 创建新项目后，打开项目属性
   - **VC++ 目录** → **包含目录**：添加 `C:\opencv\build\include`
   - **VC++ 目录** → **库目录**：添加 `C:\opencv\build\x64\vc15\lib`
   - **链接器** → **输入** → **附加依赖项**：添加 `opencv_world4xx.lib`（根据版本号调整）

4. **MinGW-w64 配置**
   - 使用 CMake 构建时，通过 `-DOpenCV_DIR` 指定 OpenCV 路径

