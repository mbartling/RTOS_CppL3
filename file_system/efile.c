#include "efile.h"
#include "edisk.h"
#include "FAT32.h"
#include <stdio.h>
volatile int OUTPUT_redirected = 0;
#define NUMRESIDENTSECTORS 4

uint8_t ResidentSectors[512*NUMRESIDENTSECTORS];
uint8_t FAT_Table_Sector[512];

void printFATStats(void){
	printf("FAT32 Bytes per Sector: %u", BPB_BytsPerSec);
  printf("FAT32 Total Sectors: %u", BPB_TotSec32);
  printf("FAT32 Reserved Sector Count: %u", BPB_RsvdSecCnt);
  printf("FAT32 Root Cluster Num: %u", BPB_RootClus);
  printf("FAT32 Sectors per Cluster: %u", BPB_SecPerClus);
  printf("FAT32 Fat Size: %u", BPB_FATSz32);
  printf("FAT32 First Data Sector: %u", FirstDataSector);
}
//---------- eFile_Init-----------------
// Activate the file system, without formating
// Input: none
// Output: 0 if successful and 1 on failure (already initialized)
// since this program initializes the disk, it must run with 
//    the disk periodic task operating
int eFile_Init(void){
  DSTATUS result;
  
	result = eDisk_Init(0);  // initialize disk
  if(result) printf("eDisk_Init %d\n\r",result);
  eDisk_ReadBlock((BYTE *)ResidentSectors,0);
  FAT_Init(ResidentSectors);
  eDisk_ReadBlock((BYTE *)FAT_Table_Sector, FAT_Begin_LBA);
  printFATStats();
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

}  // create new file, make it empty 


//---------- eFile_WOpen-----------------
// Open the file, read into RAM last block
// Input: file name is a single ASCII letter
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_WOpen(char name[]){

}      // open a file for writing 

//---------- eFile_Write-----------------
// save at end of the open file
// Input: data to be saved
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_Write( char data){

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

