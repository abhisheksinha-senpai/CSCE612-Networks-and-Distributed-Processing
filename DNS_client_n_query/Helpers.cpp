#include "pch.h"
#include "DNS_query.h"

void makeDNSQuestion(char* buff, char* host)
{
    char* pch = strtok(host, ".");
    int pos = 0;
    while (pch != NULL)
    {
        int size = ((int)strlen(pch));
        buff[pos++] = size;
        memcpy(buff + pos, pch, size);
        pos += size;
        pch = strtok(NULL, ".");
    }
    buff[pos] = 0;
}

unsigned char *readRRwithJumps(unsigned char* buff, unsigned char* reader, unsigned char* headerpos, int size)
{
    if (*reader == 0)
        return reader + 1;
    
    if (*reader >= 0xC0)
    {
        if ((reader + 1) == NULL)
        {
            printf("++\tinvalid record: truncated jump offset\n");
            WSACleanup();
            exit(EXIT_FAILURE);
        }

        int offset = ((*reader & 0x3F) << 8) +*(reader+1);
        if ((buff + offset - headerpos) < sizeof(FixedDNSheader))
        {
            printf("++\tinvalid record: jump into fixed header\n");
            WSACleanup();
            exit(EXIT_FAILURE);
        }

        unsigned char* nextPos = buff + offset;
        if (nextPos >= (buff + size))
        {
            printf("++\tinvalid record: jump beyond packet boundary\n");
            WSACleanup();
            exit(EXIT_FAILURE);
        }
        if (*nextPos >= 0xC0)
        {
            printf("++\tinvalid record: jump loop\n");
            WSACleanup();
            exit(EXIT_FAILURE);
        }

        if (offset > size)
        {
            printf("++\tinvalid record: jump beyond packet boundary\n");
            WSACleanup();
            exit(EXIT_FAILURE);
        }
        unsigned char* result = readRRwithJumps(buff, buff + offset, headerpos, size);
 
        return reader+2;
    }
    else
    {
        int length = *reader;
        if(length == 0)
            return NULL;

        reader = reader + 1;

        char temp[MAX_DNS_SIZE];
        memcpy(temp, reader, length+1);

        if (reader + length - buff > size)
        {
            temp[reader + length - buff - size] = 0;
            printf("%s\n", temp);
            printf(" ++\tinvalid record: truncated name\n");
            WSACleanup();
            exit(EXIT_FAILURE);
        }
        temp[length] = '\0';
        printf("%s", temp);
        reader = reader + length;
        if (*reader != 0)
            printf(".");
        else
            printf(" ");
        reader = readRRwithJumps(buff, reader, headerpos, size);
        return reader;
    }
}

unsigned char* printResult(unsigned char* reader)
{
    int dot = 0;
    while (true)
    {
        int size = (reader[0]);
        if (size == 0)
        {
            reader = reader + 1;
            break;
        }

        if (dot == 1)
            printf(".");
        reader = reader + 1;
        char temp[MAX_DNS_SIZE];
        memcpy(temp, reader, size);
        temp[size] = '\0';
        printf("%s", temp);
        reader = reader + size;
        dot = 1;
    }
    return reader;
}

char* reverseIP(char* host)
{
    char* final = (char*)malloc(30);
    final[0] = '\0';
    char part1[5], part2[5], part3[5], part4[5];
    char suffix[] = "in-addr.arpa";

    char* pch = strtok(host, ".");
    memcpy(part1, pch, strlen(pch));
    part1[strlen(pch)] = '.';
    part1[strlen(pch) + 1] = '\0';

    pch = strtok(NULL, ".");
    memcpy(part2, pch, strlen(pch));
    part2[strlen(pch)] = '.';
    part2[strlen(pch) + 1] = '\0';

    pch = strtok(NULL, ".");
    memcpy(part3, pch, strlen(pch));
    part3[strlen(pch)] = '.';
    part3[strlen(pch) + 1] = '\0';

    pch = strtok(NULL, ".");
    memcpy(part4, pch, strlen(pch));
    part4[strlen(pch)] = '.';
    part4[strlen(pch) + 1] = '\0';

    strcat(final, part4);
    strcat(final, part3);
    strcat(final, part2);
    strcat(final, part1);
    strcat(final, suffix);

    return final;
}