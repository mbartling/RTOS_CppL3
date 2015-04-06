#include "efile.h"
#include "edisk.h"
#include "FAT32.h"
#include <stdio.h>
volatile int OUTPUT_redirected = 0;
#define NUMRESIDENTSECTORS 4

//uint8_t fileSector[512];
uint8_t ResidentSectors[512*NUMRESIDENTSECTORS];
uint8_t FAT_Table_Sector[512];
uint32_t currentRootSector;
uint32_t currentFATSector;

void dummyTest(void);
void dummyTest2(void);
void dummyTest3(void);
void dummyTest4(void);
void printFATStats(void){
	printf("FAT32 Bytes per Sector: %u\n\r", BPB_BytsPerSec);
  printf("FAT32 Total Sectors: %u\n\r", BPB_TotSec32);
  printf("FAT32 Reserved Sector Count: %u\n\r", BPB_RsvdSecCnt);
  printf("FAT32 Root Cluster Num: %u\n\r", BPB_RootClus);
  printf("FAT32 Sectors per Cluster: %u\n\r", BPB_SecPerClus);
  printf("FAT32 Fat Size: %u\n\r", BPB_FATSz32);
  printf("FAT32 First Data Sector: %u\n\r", FirstDataSector);
}
uint16_t SectorSize;


//---------- eFile_Init-----------------
// Activate the file system, without formating
// Input: none
// Output: 0 if successful and 1 on failure (already initialized)
// since this program initializes the disk, it must run with 
//    the disk periodic task operating
int eFile_Init(void){
  int result; 
  while (1) {
    result = eDisk_Init(0);  // initialize disk
    if (result == 0){
      break;
    }
  } 
  if(result) printf("eDisk_Init %d\n\r",result);
  eDisk_ReadBlock((BYTE *)ResidentSectors,0);
  FAT_Init(ResidentSectors);
  eDisk_ReadBlock((BYTE *)FAT_Table_Sector, FAT_Begin_LBA);
  currentRootSector = 0;
  currentFATSector = 0;
  printFATStats();
 // dummyTest();
  //dummyTest2();
  //dummyTest3();
  dummyTest4();
} // initialize file system

//---------- eFile_Format-----------------
// Erase all files, create blank directory, initialize free space manager
// Input: none
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_Format(void){

} // erase disk, add format

//---------- eFile_Create-----------------
// Create a new, empty file with one allocated block
// Input: file name is an ASCII string up to seven characters 
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_Create( char name[]){
  uint8_t* rootSector = ResidentSectors;
  char nameFormated[11];
  FAT_nameTo(nameFormated, name);
  uint8_t* res = eFile_FindFileInRoot(rootSector, nameFormated);
  if (res != NULL) {
    printf("file already exist. We will be appending to the end of it.\n");
    return 0; 
  }else{
    //make a DIR_Entry variable and instatiate it 
    DIR_Entry newFileDIREntry;
	memcpy(newFileDIREntry.DIR_Name, (char *)name, 11);
    uint32_t entryNum;
    uint8_t* rootSector = ResidentSectors;
    //find the first empty directory entry 
    entryNum = (uint32_t) eFile_FindFirstUnusedDirEntry(rootSector);
    newFileDIREntry.DIR_FstClus =  currentRootSector/BPB_SecPerClus;
//      (entryNum) + (currentRootSector * BPB_SecPerClus); 
    newFileDIREntry.DIR_FileSize = 0;
    //newFileDIREntry.currentSector = (newFileDIREntry.DIR_FstClus);
    //newFileDIREntry->sectorBasePtr =  &(newFileDIREntry->DIR_FstClus);
//    newFileDIREntry->cursorPtr =  &(newFileDIREntry->DIR_FstClus);
    uint8_t sectorAddr =  (currentRootSector * BPB_BytsPerSec);
    writeDirEntry(&newFileDIREntry,(uint32_t) entryNum, &ResidentSectors[0] + sectorAddr);
    
   //update FAT 
    FAT_Table_Sector[currentRootSector/BPB_SecPerClus] = (uint8_t)-2;
    return 1;  
  }
}  // create new file, make it empty 


//---------- eFile_WOpen-----------------
// Open the file, read into RAM last block
// Input: file name is a single ASCII letter
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_WOpen(char name[]){
  
  //vars  
  uint8_t* rootSector = ResidentSectors;
  DIR_Entry entry; 
  char nameFormated[11];
  FAT_nameTo(nameFormated, name); 
  uint8_t* res = eFile_FindFileInRoot(rootSector, nameFormated);
  // if no entry was found in the RootDirecoty return 0 
  if (res == NULL) {
    return 0;  //no file found with that name
  }else { //dump the result in to the entry and find the last block own by the file
    readDirEntryFromCursor(&entry, res);
     
    //find the last cluster
    
    uint8_t FATIndex = (uint8_t) entry.DIR_FstClus;
    printf("%d\n", FATIndex); 
    while(FAT_Table_Sector[FATIndex] != (uint8_t)-2) {
      printf("%d\n",  FAT_Table_Sector[FATIndex]);
      FATIndex = FAT_Table_Sector[FATIndex];
    } 
    
    uint16_t lastBlock = (entry.DIR_FileSize)%SectorSize;
    uint8_t *fileSector = &ResidentSectors[512]; 
    eDisk_ReadBlock((BYTE *)fileSector, GetFirstSectorOfCluster(FAT_Table_Sector[FATIndex] + lastBlock));
 
    for (int i = 0; i < entry.DIR_FileSize; i++) {
      printf("%c", (char*)fileSector[i]);
    }
  } 
}      // open a file for writing 

//---------- eFile_Write-----------------
// save at end of the open file
// Input: data to be saved
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int writeCharNum = 0;
int eFile_Write(char data){
//  if (writeCharNum == 512) {
//    eDisk_WriteBlock (fileSector,  
//    DWORD sector        /* Start sector number (LBA) */
//  }
//  fileSector[writeCharNum] = data;
//


}  

//---------- eFile_Close-----------------
// Deactivate the file system
// Input: none
// Output: 0 if successful and 1 on failure (not currently open)
int eFile_Close(void){

} 


//---------- eFile_WClose-----------------
// close the file, left disk in a state power can be removed
// Input: none
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_WClose(void){

} // close the file for writing

//---------- eFile_ROpen-----------------
// Open the file, read first block into RAM 
// Input: file name is a single ASCII letter
// Output: 0 if successful and 1 on failure (e.g., trouble read to flash)
int eFile_ROpen( char name[]){
  //vars  
  uint8_t* rootSector = ResidentSectors;
  DIR_Entry entry; 
  char nameFormated[11];
  FAT_nameTo(nameFormated, name); 
  uint8_t* res = eFile_FindFileInRoot(rootSector, nameFormated);
  
  // if no entry was found in the RootDirecoty return 0 
  if (res == NULL) {
    return 0;  //no file found with that name
  }else { //dump the result in to the entry and find the first block
    readDirEntryFromCursor(&entry, res);
    uint8_t *fileSector = &ResidentSectors[512]; 
    eDisk_ReadBlock((BYTE *)fileSector, GetFirstSectorOfCluster(entry.DIR_FstClus));
//    for (int i = 0; i < entry.DIR_FileSize; i++) {
//      printf("%c", (char*)fileSector[i]);
//    }
    return 1;
  }
}      // open a file for reading 
   
//---------- eFile_ReadNext-----------------
// retreive data from open file
// Input: none
// Output: return by reference data
//         0 if successful and 1 on failure (e.g., end of file)
int eFile_ReadNext( char *pt){

  

}       // get next byte 
                              
//---------- eFile_RClose-----------------
// close the reading file
// Input: none
// Output: 0 if successful and 1 on failure (e.g., wasn't open)
int eFile_RClose(void){

} // close the file for writing

//---------- eFile_Directory-----------------
// Display the directory with filenames and sizes
// Input: pointer to a function that outputs ASCII characters to display
// Output: characters returned by reference
//         0 if successful and 1 on failure (e.g., trouble reading from flash)
int eFile_Directory(void(*fp)(unsigned char)){

}   

//---------- eFile_Delete-----------------
// delete this file
// Input: file name is a single ASCII letter
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_Delete( char name[]){

}  // remove this file 

//---------- eFile_RedirectToFile-----------------
// open a file for writing 
// Input: file name is a single ASCII letter
// stream printf data into file
// Output: 0 if successful and 1 on failure (e.g., trouble read/write to flash)
int eFile_RedirectToFile(char *name){
  eFile_WOpen(name);
  OUTPUT_redirected = 1;
}

//---------- eFile_EndRedirectToFile-----------------
// close the previously open file
// redirect printf data back to UART
// Output: 0 if successful and 1 on failure (e.g., wasn't open)
int eFile_EndRedirectToFile(void){
  OUTPUT_redirected = 0;
}


/*
 * Records that do not begin with 0x00 or 0xE5 are actual directory entries
 */
 /**
  * @brief Return a pointer to a directory entry if file exists, else return null
  * @details [long description]
  * 
  * @param rootSector pointer to 512 byte array for root directory sector management
  * @param FATfilename must be in format returned by FAT_nameTo
  * 
  * @return pointer to directory entry with filename, else returns null
  */
uint8_t* eFile_FindFileInRoot(uint8_t* rootSector, char* FATfilename){
  uint32_t i = 0;
  uint8_t* cursor;

  if(currentRootSector != 0){
    currentRootSector = 0;
    eDisk_ReadBlock((BYTE *)rootSector, GetFirstSectorOfCluster(BPB_RootClus));
  }
  cursor = rootSector;
  while(cursor[0] != 0x00){ //While not at end of directory
    if(i == 16){  //16 Directory Entries per sector
      currentRootSector++;
      i = 0;
      eDisk_ReadBlock((BYTE *)rootSector, GetFirstSectorOfCluster(BPB_RootClus) + currentRootSector);
    }
    if(cursor[0] != DIR_UNUSED){
      if(memcmp(cursor, FATfilename, 11) == 0){
        return cursor;
      }
    }

    cursor += DIR_Entry_Size; //Iterator++
    i++;
  }

  /*File not found*/
  if(currentRootSector != 0){
    currentRootSector = 0;
    eDisk_ReadBlock((BYTE *)rootSector, GetFirstSectorOfCluster(BPB_RootClus) + currentRootSector);
  }
  return NULL;

}


uint32_t eFile_FindFirstUnusedDirEntry(uint8_t* rootSector) {
  uint32_t i = 0;
  uint8_t* cursor;
  if(currentRootSector != 0){
    currentRootSector = 0;
    eDisk_ReadBlock((BYTE *)rootSector, GetFirstSectorOfCluster(BPB_RootClus));
  }
  cursor = rootSector;
  while(1) { //for now it look indefinitly untill it finds an unallocated directory entry (later we should put a bound on it)
    if(i == 16){  //16 Directory Entries per sector
      currentRootSector++; //this only would work if the directory is allocated contiguously in the memory)
      i = 0;
    }
    // if unallocated || deallocated
    if(cursor[0] == DIR_UNUSED || (cursor[0] == 0x00)){ 
      return i;
    }
    cursor += DIR_Entry_Size; //Iterator++
    i++;
  }

}

void printDirEntry(DIR_Entry* entry){
	int i = 0;
	printf("\n\rFileName: ");
	while(i < 11){
		putc(entry->DIR_Name[i], stdout);
		i++;
	}
	printf("\n\r");
  printf("Files First cluster: %u\n\r", entry->DIR_FstClus);
  printf("File Size: %u\n\r", entry->DIR_FileSize);

}

void dummyTest(){
  DIR_Entry entry;
  uint8_t* rootSector = ResidentSectors;
	uint8_t* fileSector = &ResidentSectors[512];
  FAT_nameTo(entry.DIR_Name, "TESTFILE.TXT");
  uint8_t* res = eFile_FindFileInRoot(rootSector, entry.DIR_Name);
  readDirEntryFromCursor(&entry, res);
  printDirEntry(&entry);
	eDisk_ReadBlock((BYTE *)fileSector, GetFirstSectorOfCluster(entry.DIR_FstClus));
  printf("Dumping Test File Contents\n\r");
  printf("%s", (char *)fileSector);
}

void dummyTest2(){
  DIR_Entry entry;
  eFile_Create("FILE2.TXT"); 
   
  uint8_t* rootSector = ResidentSectors;
  FAT_nameTo(entry.DIR_Name, "FILE2.TXT");
  uint8_t* res = eFile_FindFileInRoot(rootSector, entry.DIR_Name);
  readDirEntryFromCursor(&entry, res);
  printDirEntry(&entry);
//	eDisk_ReadBlock((BYTE *)fileSector, GetFirstSectorOfCluster(entry.DIR_FstClus));
//  printf("Dumping Test File Contents\n\r");
 // printf("%s", (char *)fileSector);
}

//eFile_ROpen
void dummyTest3(){
  DIR_Entry entry;
  uint8_t* rootSector = ResidentSectors;
  eFile_ROpen("TESTFILE.TXT");
}


//eFile_WOpen
void dummyTest4(){
  DIR_Entry entry;
  uint8_t* rootSector = ResidentSectors;
  eFile_Create("File2.TXT"); 
  eFile_WOpen("File2.TXT");
}

