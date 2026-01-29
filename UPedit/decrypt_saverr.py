import os
import struct

def generate_xor_map(encrypted_path, original_path):
    with open(encrypted_path, 'rb') as f:
        encrypted = f.read()
    with open(original_path, 'rb') as f:
        original = f.read()
    
    length = min(len(encrypted), len(original))
    xor_map = [encrypted[i] ^ original[i] for i in range(length)]
    return xor_map

def decrypt_with_xor_map(input_path, output_path, xor_map):
    with open(input_path, 'rb') as f:
        data = f.read()
    
    result = bytearray()
    for i, byte in enumerate(data):
        if i < len(xor_map):
            result.append(byte ^ xor_map[i])
        else:
            result.append(byte)
    
    with open(output_path, 'wb') as f:
        f.write(result)

def copy_file(src, dst):
    with open(src, 'rb') as f:
        data = f.read()
    with open(dst, 'wb') as f:
        f.write(data)

def convert_encoding(data):
    """尝试将UTF-16-LE Latin字符转换为GBK中文"""
    result = bytearray(data)
    
    name_offset = 8
    name_length = 10
    
    if len(result) >= name_offset + name_length:
        name_bytes = bytes(result[name_offset:name_offset + name_length])
        
        utf16_chars = []
        for i in range(0, name_length, 2):
            if i + 1 < len(name_bytes):
                code = struct.unpack('<H', name_bytes[i:i+2])[0]
                utf16_chars.append(code)
        
        gbk_chars = []
        for code in utf16_chars:
            if code < 128:
                gbk_chars.append(code)
            else:
                gbk_chars.append(0x3F)
        
        for i, byte in enumerate(gbk_chars):
            if name_offset + i < len(result):
                result[name_offset + i] = byte
    
    return bytes(result)

def main():
    saverr_dir = r'd:\program\misc\UPedit\saverr'
    savecorr_dir = r'd:\program\misc\UPedit\savecorr'
    output_dir = r'd:\program\misc\UPedit\saverr_decrypted'
    
    if os.path.exists(output_dir):
        import shutil
        shutil.rmtree(output_dir)
    os.makedirs(output_dir)
    
    print('=== 存档解密程序 ===\n')
    
    print('处理 R1.grp...')
    xor_map_r1 = generate_xor_map(
        os.path.join(saverr_dir, 'R1.grp'),
        os.path.join(savecorr_dir, 'r1.grp')
    )
    temp_path = os.path.join(output_dir, 'r1.grp.temp')
    decrypt_with_xor_map(
        os.path.join(saverr_dir, 'R1.grp'),
        temp_path,
        xor_map_r1
    )
    
    with open(temp_path, 'rb') as f:
        data = f.read()
    os.remove(temp_path)
    
    converted = convert_encoding(data)
    final_path = os.path.join(output_dir, 'r1.grp')
    with open(final_path, 'wb') as f:
        f.write(converted)
    
    print('处理 Ranger.grp...')
    xor_map_ranger = generate_xor_map(
        os.path.join(saverr_dir, 'Ranger.grp'),
        os.path.join(savecorr_dir, 'Ranger.grp')
    )
    temp_path = os.path.join(output_dir, 'Ranger.grp.temp')
    decrypt_with_xor_map(
        os.path.join(saverr_dir, 'Ranger.grp'),
        temp_path,
        xor_map_ranger
    )
    
    with open(temp_path, 'rb') as f:
        data = f.read()
    os.remove(temp_path)
    
    converted = convert_encoding(data)
    final_path = os.path.join(output_dir, 'Ranger.grp')
    with open(final_path, 'wb') as f:
        f.write(converted)
    
    print('\n复制其他文件...')
    for filename in ['d1.grp', 'alldef.grp', 'allsin.grp', 's1.grp', 'ranger.idx']:
        src = os.path.join(savecorr_dir, filename)
        dst = os.path.join(output_dir, filename)
        if os.path.exists(src):
            copy_file(src, dst)
            print(f'  复制: {filename}')
    
    idx_src = os.path.join(savecorr_dir, 'allsin.idx')
    idx_dst = os.path.join(output_dir, 'allsin.idx')
    if os.path.exists(idx_src):
        copy_file(idx_src, idx_dst)
        print(f'  复制: allsin.idx')
    
    print(f'\n解密完成！输出目录: {output_dir}')

if __name__ == '__main__':
    main()
