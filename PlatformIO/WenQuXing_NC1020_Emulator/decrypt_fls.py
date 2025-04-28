#!/usr/bin/env python3
import sys
import os

def process_binary(input_file, output_file):
    """
    解密NC1020的fls固件文件
    :param input_file: 输入的fls文件路径
    :param output_file: 输出的解密后文件路径
    """
    try:
        # 读取输入文件
        with open(input_file, 'rb') as f:
            data = f.read()
        
        # 获取文件大小
        size = len(data)
        
        # 创建输出缓冲区
        decrypted = bytearray(size)
        
        # 处理每个0x8000字节的块
        offset = 0
        while offset < size:
            # 计算当前块的大小(最后一个块可能不足0x8000)
            block_size = min(0x8000, size - offset)
            half_size = min(0x4000, block_size // 2)
            
            # 交换前后0x4000字节
            decrypted[offset:offset + half_size] = data[offset + half_size:offset + half_size * 2]
            decrypted[offset + half_size:offset + half_size * 2] = data[offset:offset + half_size]
            
            # 如果有剩余字节(不足0x4000),直接复制
            if half_size * 2 < block_size:
                decrypted[offset + half_size * 2:offset + block_size] = data[offset + half_size * 2:offset + block_size]
            
            offset += 0x8000
        
        # 写入输出文件
        with open(output_file, 'wb') as f:
            f.write(decrypted)
            
        print(f"解密完成! 输出文件: {output_file}")
        
    except Exception as e:
        print(f"处理文件时出错: {str(e)}")
        sys.exit(1)

def main():
    if len(sys.argv) != 2:
        print("用法: python decrypt_fls.py <input.fls>")
        sys.exit(1)
        
    input_file = sys.argv[1]
    if not os.path.exists(input_file):
        print(f"错误: 文件 {input_file} 不存在")
        sys.exit(1)
        
    output_file = "decrypted.fls"
    process_binary(input_file, output_file)

if __name__ == "__main__":
    main() 