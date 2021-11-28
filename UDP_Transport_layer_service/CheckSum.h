#pragma once
class CheckSum
{
	DWORD crc_table[256];
public:
	CheckSum();
	DWORD CRC32(unsigned char* buf, size_t len);
};

