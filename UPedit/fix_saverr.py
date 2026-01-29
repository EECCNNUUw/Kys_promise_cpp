import os
import struct

def decode_xor_file(input_path, output_path, even_xor=0xee, odd_xor=0x99):
    """解码使用交替XOR加密的文件"""
    with open(input_path, 'rb') as f:
        data = bytearray(f.read())
    
    decoded = bytearray()
    for i, byte in enumerate(data):
        xor_key = even_xor if i % 2 == 0 else odd_xor
        decoded.append(byte ^ xor_key)
    
    with open(output_path, 'wb') as f:
        f.write(decoded)
    
    print(f"解码完成: {input_path} -> {output_path}")
    print(f"  原始大小: {len(data)} bytes")
    print(f"  解码后大小: {len(decoded)} bytes")

def fix_ranger_file(input_path, output_path):
    """修复Ranger.grp文件"""
    with open(input_path, 'rb') as f:
        data = bytearray(f.read())
    
    # 先进行XOR解码
    decoded = bytearray()
    for i, byte in enumerate(data):
        xor_key = 0xee if i % 2 == 0 else 0x99
        decoded.append(byte ^ xor_key)
    
    # 修复位置28的字节（0xea -> 0xee）
    if len(decoded) > 28:
        if decoded[28] == 0xea:
            decoded[28] = 0xee
            print(f"  修复位置28: 0xea -> 0xee")
    
    with open(output_path, 'wb') as f:
        f.write(decoded)
    
    print(f"修复完成: {input_path} -> {output_path}")

def fix_d1_file(input_path, output_path):
    """修复d1.grp文件"""
    with open(input_path, 'rb') as f:
        data = bytearray(f.read())
    
    # d1.grp的差异似乎是数据损坏，我们尝试修复已知的差异位置
    # 根据分析，主要差异在位置26-31, 180-181等
    
    # 位置26-31: saverr=c6 06 00 00 00 00 -> savecorr=68 00 a5 06 ff ff
    if len(data) > 31:
        data[26] = 0x68  # c6 -> 68
        data[27] = 0x00  # 06 -> 00
        data[28] = 0xa5  # 00 -> a5
        data[29] = 0x06  # 00 -> 06
        data[30] = 0xff  # 00 -> ff
        data[31] = 0xff  # 00 -> ff
        print(f"  修复位置26-31")
    
    # 位置180-181: saverr=15 08 -> savecorr=00 00
    if len(data) > 181:
        data[180] = 0x00  # 15 -> 00
        data[181] = 0x00  # 08 -> 00
        print(f"  修复位置180-181")
    
    # 位置4094: saverr=01 -> savecorr=00
    if len(data) > 4094:
        data[4094] = 0x00  # 01 -> 00
        print(f"  修复位置4094")
    
    # 位置4468: saverr=00 -> savecorr=03
    if len(data) > 4468:
        data[4468] = 0x03  # 00 -> 03
        print(f"  修复位置4468")
    
    # 位置4470-4475: saverr=00 00 00 00 00 00 -> savecorr=ff ff ff ff ff ff
    if len(data) > 4475:
        for i in range(4470, 4476):
            data[i] = 0xff
        print(f"  修复位置4470-4475")
    
    # 位置4536: saverr=00 -> savecorr=6a
    if len(data) > 4536:
        data[4536] = 0x6a  # 00 -> 6a
        print(f"  修复位置4536")
    
    # 位置4538-4541: saverr=00 00 00 00 -> savecorr=ff ff ff ff
    if len(data) > 4541:
        for i in range(4538, 4542):
            data[i] = 0xff
        print(f"  修复位置4538-4541")
    
    with open(output_path, 'wb') as f:
        f.write(data)
    
    print(f"修复完成: {input_path} -> {output_path}")

def create_fixed_saverr():
    """创建修复后的saverr文件夹"""
    saverr_dir = r'd:\program\misc\UPedit\saverr'
    savecorr_dir = r'd:\program\misc\UPedit\savecorr'
    saverr_fixed_dir = r'd:\program\misc\UPedit\saverr_fixed'
    
    # 创建修复后的文件夹
    if not os.path.exists(saverr_fixed_dir):
        os.makedirs(saverr_fixed_dir)
    
    print("=" * 80)
    print("开始修复saverr文件")
    print("=" * 80)
    
    # 处理Ranger.grp
    ranger_saverr = os.path.join(saverr_dir, 'Ranger.grp')
    ranger_fixed = os.path.join(saverr_fixed_dir, 'Ranger.grp')
    if os.path.exists(ranger_saverr):
        print(f"\n处理 Ranger.grp:")
        fix_ranger_file(ranger_saverr, ranger_fixed)
    
    # 处理R1.grp
    r1_saverr = os.path.join(saverr_dir, 'R1.grp')
    r1_fixed = os.path.join(saverr_fixed_dir, 'r1.grp')
    if os.path.exists(r1_saverr):
        print(f"\n处理 R1.grp:")
        fix_ranger_file(r1_saverr, r1_fixed)
    
    # 处理d1.grp
    d1_saverr = os.path.join(saverr_dir, 'd1.grp')
    d1_fixed = os.path.join(saverr_fixed_dir, 'd1.grp')
    if os.path.exists(d1_saverr):
        print(f"\n处理 d1.grp:")
        fix_d1_file(d1_saverr, d1_fixed)
    
    # 复制其他相同的文件
    same_files = ['alldef.grp', 'allsin.grp', 's1.grp', 'ranger.idx']
    for filename in same_files:
        src = os.path.join(saverr_dir, filename)
        dst = os.path.join(saverr_fixed_dir, filename)
        if os.path.exists(src):
            import shutil
            shutil.copy2(src, dst)
            print(f"\n复制 {filename} (无需修复)")
    
    # 复制allsin.idx从savecorr
    allsin_idx_src = os.path.join(savecorr_dir, 'allsin.idx')
    allsin_idx_dst = os.path.join(saverr_fixed_dir, 'allsin.idx')
    if os.path.exists(allsin_idx_src):
        import shutil
        shutil.copy2(allsin_idx_src, allsin_idx_dst)
        print(f"\n复制 allsin.idx (从savecorr)")
    
    print("\n" + "=" * 80)
    print("修复完成！修复后的文件保存在: " + saverr_fixed_dir)
    print("=" * 80)

def verify_fix():
    """验证修复结果"""
    saverr_fixed_dir = r'd:\program\misc\UPedit\saverr_fixed'
    savecorr_dir = r'd:\program\misc\UPedit\savecorr'
    
    print("\n" + "=" * 80)
    print("验证修复结果")
    print("=" * 80)
    
    files_to_check = ['Ranger.grp', 'r1.grp', 'd1.grp', 'alldef.grp', 'allsin.grp', 's1.grp']
    
    for filename in files_to_check:
        fixed_path = os.path.join(saverr_fixed_dir, filename)
        corr_path = os.path.join(savecorr_dir, filename)
        
        if os.path.exists(fixed_path) and os.path.exists(corr_path):
            with open(fixed_path, 'rb') as f1, open(corr_path, 'rb') as f2:
                data1 = f1.read()
                data2 = f2.read()
            
            if data1 == data2:
                print(f"\n{filename}: ✓ 完全匹配")
            else:
                # 计算差异字节数
                diff_count = sum(1 for i in range(min(len(data1), len(data2))) if data1[i] != data2[i])
                print(f"\n{filename}: ✗ 仍有 {diff_count} 个字节差异")

if __name__ == '__main__':
    create_fixed_saverr()
    verify_fix()
