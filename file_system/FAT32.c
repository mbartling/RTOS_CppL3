#include "FAT32.h"

uint32_t BPB_FSInfo;
uint32_t BPB_TotSec32;
uint16_t BPB_RsvdSecCnt;
uint32_t BPB_RootClus;
uint8_t  BPB_SecPerClus;
uint32_t BPB_FATSz32;

uint32_t CountOfClusters;
uint32_t FirstDataSector;

void FAT_Init(uint8_t* MBRSector){
  
  BPB_SecPerClus = (uint8_t)  MBRSector[BPB_SecPerClus_OFFSET];
  BPB_RsvdSecCnt = (uint16_t) MBRSector[BPB_RsvdSecCnt_OFFSET];
  BPB_TotSec32   = (uint32_t) MBRSector[BPB_TotSec32_OFFSET];
  BPB_FATSz32    = (uint32_t) MBRSector[BPB_FATSz32_OFFSET];
  BPB_RootClus   = (uint32_t) MBRSector[BPB_RootClus_OFFSET];

  FirstDataSector = BPB_RsvdSecCnt + (BPB_NumFATs*BPB_FATSz32) + RootDirSectors;
}

/** Automatically handles reading FAT table entries for a cluster number
  Will load a block into resident memory if need be
  */

uint32_t ReadFATEntryForCluster(uint32_t N, uint8_t* SecBuff){
  uint32_t ThisFATSecNum = BPB_RsvdSecCnt + N*4/BPB_BytsPerSec;
  /*Check to see if need to load a different sector of FAT Table*/

  uint32_t ThisFATEntOffset = (N*4) % BPB_BytsPerSec;
  uint32_t FAT32ClusEntryVal = (*((uint32_t *) &SecBuff[ThisFATEntOffset])) & 0x0FFFFFFF;
  return FAT32ClusEntryVal;
}

void WriteFATEntryForCluster(uint32_t N, uint32_t FAT32ClusEntryVal, uint8_t* SecBuff){
  uint32_t ThisFATSecNum = BPB_RsvdSecCnt + N*4/BPB_BytsPerSec;
  /*Check to see if need to load a different sector of FAT Table*/

  uint32_t ThisFATEntOffset = (N*4) % BPB_BytsPerSec;
  FAT32ClusEntryVal = FAT32ClusEntryVal & 0x0FFFFFFF;
  (*((uint32_t *) &SecBuff[ThisFATEntOffset])) = (*((uint32_t *) &SecBuff[ThisFATEntOffset])) & 0xF0000000;
  (*((uint32_t *) &SecBuff[ThisFATEntOffset])) = (*((uint32_t *) &SecBuff[ThisFATEntOffset])) | FAT32ClusEntryVal;
  return;
}

int isDirFree(DIR_Entry* entry){
  return (entry->DIR_Name[0] == 0xE5 || entry->DIR_Name[0] == 0x00) ? 1 /*is Free*/ : 0 /*Is not free*/;
}

//uint32_t getDirSize(DIR_Entry* entry){
//  uint32_t clusterNum = entry->DIR_FstClus;
//  uint32_t fSize = 0;
//  
//  //fSize += BPB_BytsPerSec*BPB_SecPerClus;
//  while(clusterNum != EOC){
//    clusterNum = ReadFATEntryForCluster(clusterNum);
//    fSize += BPB_BytsPerSec*BPB_SecPerClus;
//  }
//  return fSize;
//}

void readDirEntry(DIR_Entry* entry, uint32_t entryNum, uint8_t* sector){
  
  uint8_t* entry_offset = &sector[entryNum*DIR_Entry_Size];
  FAT_nameFrom(entry->DIR_Name, (char *)entry_offset);
  entry->DIR_FstClus = (*((uint16_t *)entry_offset[DIR_FstClusHI_OFFSET])) << 16;
  entry->DIR_FstClus |= (*((uint16_t *)entry_offset[DIR_FstClusLO_OFFSET]));
  entry->DIR_FileSize= (*((uint32_t *)entry_offset[DIR_FileSize_OFFSET]));

}
void writeDirEntry(DIR_Entry* entry, uint32_t entryNum, uint8_t* sector){
  
  uint8_t* entry_offset = &sector[entryNum*DIR_Entry_Size];
  FAT_nameTo((char *)entry_offset, entry->DIR_Name);
  (*((uint16_t *)entry_offset[DIR_FstClusHI_OFFSET])) = (uint16_t)(entry->DIR_FstClus >> 16);
  (*((uint16_t *)entry_offset[DIR_FstClusLO_OFFSET])) = (uint16_t) entry->DIR_FstClus & 0xFFFF;
  (*((uint32_t *)entry_offset[DIR_FileSize_OFFSET])) = entry->DIR_FileSize;

}
/**
 Because BPB_SecPerClus is restricted to powers of 2 (1,2,4,8,16,32….), this means that
division and multiplication by BPB_SecPerClus can actually be performed via SHIFT operations on
2s complement architectures that are usually faster instructions than MULT and DIV instructions. On
current Intel X86 processors, this is largely irrelevant though because the MULT and DIV machine
instructions are heavily optimized for multiplication and division by powers of 2
*/
uint32_t GetFirstSectorOfCluster(uint32_t N){
  return (N-2)*BPB_SecPerClus + FirstDataSector;
}

/**
There is considerable confusion over exactly how this works, which leads to many “off by 1”, “off by
2”, “off by 10”, and “massively off” errors. It is really quite simple how this works. The FAT type—
one of FAT12, FAT16, or FAT32—is determined by the count of clusters on the volume and nothing
else.
*/


/**
 *  Make sure output buffer is at least 2 bytes longer than input buffer
 */
void FAT_nameFrom(char* outname, char* FATfilename){
  char* buff = outname;
  char* inName = FATfilename;
  int i = 0;
  while(*inName != ' '){
    if(i == 8) break;
    *buff = *inName;
    buff++;
    inName++;
    i++;
  }
  *buff = '.';
  i = 8;
  buff++;
  //inName = &FATfilename[i];
  while(FATfilename[i] != ' ' && i < 11){
    *buff = FATfilename[i];
    buff++;
    i++;
  }
  *buff = '\0';
  return;
}
void FAT_nameTo(char* outname, char* filename){
  //Note stringlen + ext must be <= 11bytes
  // No checks being done for it
  char* ptr = filename;
  char* buff = outname;
  int i = 0;
  //int j = 0;
  memset(buff, 0x20, 11);
  while(*ptr != '.'){
    buff[i] = *ptr;
    i++; ptr++;
  }
  ptr++;
  i = 8;
  while(i < 11){
    if(*ptr == '\0') break;
    buff[i] = *ptr;
    i++;
    ptr++;
  }

  return;
}
