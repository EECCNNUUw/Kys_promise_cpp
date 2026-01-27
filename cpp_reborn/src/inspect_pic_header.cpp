#include <iostream>
#include <fstream>
#include <vector>
#include <iomanip>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <file>" << std::endl;
        return 1;
    }

    std::ifstream file(argv[1], std::ios::binary);
    if (!file) {
        std::cerr << "Cannot open " << argv[1] << std::endl;
        return 1;
    }

    // Read Count
    int count = 0;
    file.read((char*)&count, 4);
    std::cout << "Count: " << count << std::endl;

    // Read First Offset (End of Img 0)
    int offset0 = 0;
    file.read((char*)&offset0, 4);
    std::cout << "Offset[0] (End of Img 0): " << offset0 << std::endl;
    
    // Calculate Start of Img 0
    int start0 = (count + 1) * 4;
    std::cout << "Start of Img 0 (Calculated): " << start0 << std::endl;

    // Seek to Start of Img 0
    file.seekg(start0, std::ios::beg);
    
    // Read Subheader
    int x, y, black;
    file.read((char*)&x, 4);
    file.read((char*)&y, 4);
    file.read((char*)&black, 4);
    std::cout << "Img 0 Subheader: x=" << x << " y=" << y << " black=" << black << std::endl;
    
    // Read first 16 bytes of data
    std::cout << "First 16 bytes of Img 0 Data:" << std::endl;
    unsigned char buf[16];
    file.read((char*)buf, 16);
    
    for(int i=0; i<16; ++i) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)buf[i] << " ";
    }
    std::cout << std::endl;

    // Check Img 6 (MenuEsc)
    // Offset[6] is at 4 + 6*4 = 28
    // Offset[5] is at 4 + 5*4 = 24
    file.seekg(24, std::ios::beg);
    int start6_offset = 0;
    file.read((char*)&start6_offset, 4);
    
    file.seekg(28, std::ios::beg);
    int end6_offset = 0;
    file.read((char*)&end6_offset, 4);
    
    std::cout << "Img 6 Start: " << start6_offset << " End: " << end6_offset << std::endl;
    
    file.seekg(start6_offset, std::ios::beg);
    file.read((char*)&x, 4);
    file.read((char*)&y, 4);
    file.read((char*)&black, 4);
    std::cout << "Img 6 Subheader: x=" << x << " y=" << y << " black=" << black << std::endl;
    
    std::cout << "First 16 bytes of Img 6 Data:" << std::endl;
    file.read((char*)buf, 16);
    for(int i=0; i<16; ++i) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)buf[i] << " ";
    }
    std::cout << std::endl;

    return 0;
}
