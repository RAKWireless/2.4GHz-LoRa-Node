#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
RAK3183 固件文件搬运工具
功能：将编译生成的hex文件复制到firmware目录并重命名为RAK3183_版本号.hex
"""

import os              # 操作系统接口，用于文件和目录操作
import shutil          # 高级文件操作工具，用于复制文件
import re              # 正则表达式库，用于版本号模式匹配
import sys             # 系统相关的参数和函数，用于程序退出
from pathlib import Path      # 面向对象的文件系统路径操作
from datetime import datetime # 日期时间处理，用于生成默认版本号

def get_version_from_changelog():
    """从CHANGELOG.md文件中提取版本号"""
    project_root = Path(__file__).parent.parent
    changelog_path = project_root / "CHANGELOG.md"
    
    if not changelog_path.exists():
        print(f"未找到CHANGELOG.md文件: {changelog_path}")
        return None
    
    try:
        with open(changelog_path, 'r', encoding='utf-8') as f:
            content = f.read()
        
        # 匹配版本号的正则表达式（优先级从高到低）
        version_patterns = [
            r'##\s*\[?v?(\d+\.\d+\.\d+)\]?',  # ## [v1.1.0] 或 ## v1.1.0
            r'##\s*Version\s+(\d+\.\d+\.\d+)', # ## Version 1.1.0
            r'#\s*v?(\d+\.\d+\.\d+)',         # # v1.1.0
            r'[Vv]ersion:?\s*(\d+\.\d+\.\d+)', # Version: 1.1.0
            r'[Vv]?(\d+\.\d+\.\d+)',          # 简单的版本号格式
        ]
        
        for pattern in version_patterns:
            matches = re.findall(pattern, content)
            if matches:
                version = matches[0]  # 取第一个匹配的版本号
                print(f"从CHANGELOG.md中提取到版本号: {version}")
                return version
                
    except Exception as e:
        print(f"读取CHANGELOG.md文件出错: {e}")
    
    return None

def copy_firmware_file():
    """复制并重命名固件文件"""
    # 定义路径
    project_root = Path(__file__).parent.parent
    source_file = project_root / "boards/RAK3183/examples/LoRaWAN_ISM2400/gcc/bin/rak3183.hex"
    firmware_dir = project_root / "firmware"
    
    # 检查源文件是否存在
    if not source_file.exists():
        print(f"错误: 源文件不存在: {source_file}")
        print("请先编译项目生成hex文件")
        return False
    
    # 获取版本号
    version = get_version_from_changelog()
    if not version:
        # 如果无法从changelog获取版本号，使用当前日期作为版本号
        version = datetime.now().strftime("%Y%m%d")
        print(f"使用日期作为版本号: {version}")
        
        # 或者提示用户手动输入
        user_version = input(f"无法自动获取版本号，当前使用 {version}，或手动输入版本号 (直接回车使用默认): ").strip()
        if user_version:
            version = user_version
    
    # 创建firmware目录（如果不存在）
    firmware_dir.mkdir(exist_ok=True)
    
    # 构造目标文件名
    target_filename = f"RAK3183_{version}.hex"
    target_path = firmware_dir / target_filename
    
    try:
        # 复制文件
        shutil.copy2(source_file, target_path)
        
        # 获取文件大小
        file_size = target_path.stat().st_size
        
        print("=== 操作成功 ===")
        print(f"源文件: {source_file}")
        print(f"目标文件: {target_path}")
        print(f"文件大小: {file_size:,} 字节")
        print(f"版本号: {version}")
        
        return True
        
    except Exception as e:
        print(f"复制文件时出错: {e}")
        return False

def main():
    """主函数"""
    print("=== RAK3183 固件搬运工具 ===")
    print("将编译生成的hex文件复制到firmware目录并重命名")
    print()
    
    success = copy_firmware_file()
    
    if success:
        print("\n✅ 固件文件搬运完成！")
    else:
        print("\n❌ 固件文件搬运失败！")
        sys.exit(1)

if __name__ == "__main__":
    main()
