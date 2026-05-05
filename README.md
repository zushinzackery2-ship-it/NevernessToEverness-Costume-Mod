<div align="center">

# NevernessToEverness-Costume-Mod

**Runtime costume unlocker & anti-blur patch for Neverness to Everness**

*Unlock all costumes without conditions · Anti-blur rendering · Numpad hotkey switching*

![C++](https://img.shields.io/badge/C%2B%2B-20-blue?style=flat-square)
![Platform](https://img.shields.io/badge/Platform-Windows%20x64-lightgrey?style=flat-square)
![Toolchain](https://img.shields.io/badge/Toolchain-Visual%20Studio%202022-green?style=flat-square)
![Unreal](https://img.shields.io/badge/Unreal-5.x-purple?style=flat-square)

</div>

---

> [!CAUTION]
> **免责声明**
> 本项目仅用于授权环境下的逆向工程学习、Modding 研究与调试验证。
> 不得用于任何违反法律法规、游戏服务条款或未授权目标的行为。
> 使用本项目产生的一切后果由使用者自行承担。

---

## 功能概览

| 功能 | 说明 |
|:-----|:-----|
| **动态切换未解锁时装** | 无需氪金/满足解锁条件，自由切换所有角色时装 |
| **反虚化** | 解除远景模糊限制，全距离清晰渲染 |

---

## 编译

**环境要求：** Windows x64 · MSVC v143 · Visual Studio 2022 · Rei-Universal-Dumper SDK

```bat
build.bat
```

产物：`bin\Rei-CurrentFashionSetter.dll`

---

## 使用

1. 使用 Dumper 对目标游戏生成 SDK
2. 编译本项目得到 DLL
3. 将 DLL 注入目标游戏进程
4. 使用热键切换时装（可切换未解锁时装）：

| 按键 | 功能 |
|:-----|:-----|
| `Numpad 8` | 切换到上一套时装 |
| `Numpad 2` | 切换到下一套时装 |

---

## 目录结构

<details>
<summary><strong>展开</strong></summary>

```text
CurrentFashionSetter/
├── build.bat
├── README.md
│
├── CurrentFashionSetter.cpp    # DLL 入口 + 键盘钩子
├── FashionSetter.*             # 时装切换核心逻辑
├── AppearanceData.*            # 外观数据表查询
├── PostRenderRunner.*          # PostRender 回调注入
├── SdkGlue.cpp                 # UE SDK 胶水代码
├── SdkRuntimeHelpers.*         # SDK 运行时辅助函数
├── Log.*                       # 日志系统
├── Pch.*                       # 预编译头
│
├── AntiFadeMod/                # 相机补丁模块
│   ├── CameraPatch.hpp
│   ├── CameraRuntimeCalls.cpp
│   ├── CameraSourcePatch.cpp
│   └── SdkHelpers.*
│
├── tools/                      # 调试/探测工具
│   ├── layout_compile_probe.cpp
│   ├── ipc_fashion_probe.py
│   └── build_layout_compile_probe.bat
│
├── bin/                        # 编译产物
└── obj/                        # 中间文件
```

</details>

---

## 日志

| 日志 | 位置 |
|:-----|:-----|
| 主日志 | `C:\Rei-Dumper\CurrentFashionSetter.log` |

---

## 依赖

- Dumper — UE SDK 生成工具（对目标游戏生成 C++ SDK）
- Windows x64 · Visual Studio 2022 · C++20

---

<div align="center">

**Platform:** Windows x64 · **Toolchain:** Visual Studio 2022 · **C++ Standard:** C++20

</div>
