#ifndef __FAT32_H__
#define __FAT32_H__
#include <string.h>
#include <stdint.h>
/**
What are the two reserved clusters at the start of the FAT for? The first reserved cluster, FAT[0],
contains the BPB_Media byte value in its low 8 bits, and all other bits are set to 1. For example, if the
BPB_Media value is 0xF8, for FAT12 FAT[0] = 0x0FF8, for FAT16 FAT[0] = 0xFFF8, and for
FAT32 FAT[0] = 0x0FFFFFF8. The second reserved cluster, FAT[1], is set by FORMAT to the EOC
mark. On FAT12 volumes, it is not used and is simply always contains an EOC mark. For FAT16 and
FAT32, the file system driver may use the high two bits of the FAT[1] entry for dirty volume flags (all
other bits, are always left set to 1). Note that the bit location is different for FAT16 and FAT32,
because they are the high 2 bits of the entry.
*/

#define EOC 0xFFFFFFFF
#define FREE_CLUSTER 0x00000000
// #define BPB_BytsPerSec 512
#define BPB_BytsPerSec_OFFSET 11
#define BPB_SecPerClus_OFFSET 13
#define BPB_RsvdSecCnt_OFFSET 14

#define BPB_NumFATs 2
#define BPB_RootEntCnt 0
#define BPB_TotSec16 0
#define BPB_FATSz16 0
#define BPB_TotSec32_OFFSET 32
#define BPB_FATSz32_OFFSET 36
#define BPB_RootClus_OFFSET 44
#define BPB_FSInfo_OFFSET 48

#define Partition1_LBA_Begin_OFFSET 454

#define RootDirSectors 0 /*0 For FAT 32*/

#define isEOF(x) ( (uint32_t) x >= 0x0FFFFFF8 )

extern uint16_t BPB_BytsPerSec;
extern uint32_t BPB_FSInfo;
extern uint32_t BPB_TotSec32;
extern uint16_t BPB_RsvdSecCnt;
extern uint32_t BPB_RootClus;
extern uint8_t  BPB_SecPerClus;
extern uint32_t BPB_FATSz32;
extern uint32_t Partition1_LBA_Begin;
extern uint32_t CountOfClusters;
extern uint32_t FAT_Begin_LBA;
extern uint32_t Cluster_Begin_LBA;
/**
 This sector number is relative to the first sector of the volume that contains the BPB (the
sector that contains the BPB is sector number 0). This does not necessarily map directly onto the
drive, because sector 0 of the volume is not necessarily sector 0 of the drive due to partitioning
*/

#define DIR_FstClusHI_OFFSET 20
#define DIR_FstClusLO_OFFSET 26
#define DIR_FileSize_OFFSET 28
#define DIR_Name_OFFSET 0
#define DIR_Entry_Size 32 /*Bytes*/
#define DIR_UNUSED 0xE5
extern uint32_t FirstDataSector;

typedef struct _DIR_Entry{
  char DIR_Name[11];
  uint8_t DIR_Attr; //Unused
  uint32_t DIR_FstClus;
  uint32_t DIR_FileSize;
  uint32_t currentSector;
  uint8_t* sectorBasePtr;
  uint8_t* cursorPtr;
} DIR_Entry;



void FAT_Init(uint8_t* MBRSector);
uint32_t ReadFATEntryForCluster(uint32_t N, uint8_t* SecBuff);
void WriteFATEntryForCluster(uint32_t N, uint32_t FAT32ClusEntryVal, uint8_t* SecBuff);
int isDirFree(DIR_Entry* entry);
uint32_t getDirSize(DIR_Entry* entry);
uint32_t GetFirstSectorOfCluster(uint32_t N);
void FAT_nameFrom(char* outname, char* FATfilename);
void FAT_nameTo(char* outname, char* filename);
void readDirEntryFromCursor(DIR_Entry* entry, uint8_t* cursor);

void writeDirEntry(DIR_Entry* entry, uint32_t entryNum, uint8_t* sector);
#endif /*__FAT32_H__*/
