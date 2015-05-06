#include "mFS_params.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

uint8_t resident_mem[M_NUM_RESIDENT_CLUSTERS][M_BLOCKS_PER_CLUSTER*M_BLOCK_SIZE];

typedef struct __packed LUT_Entry {
  int A;
  int B;
  uint32_t firstCluster;
  uint32_t LBid;

} LUT_Entry_t;

typedef struct Fid{
  LUT_Entry_t entry;
  uint32_t file_size;
} Fid_t;

uint64_t uitilization;

inline uint32_t getLBAfromEffinCID(uint32_t effinCID){
  return (M_LUT_SIZE_CLUSTERS + effinCID)*M_BLOCKS_PER_CLUSTER -1;
}

inline uint32_t get_effin_clusterID(uint32_t k, LUT_Entry_t* entry){
  return ((entry->A*k + entry->B) % M_P) % M_N);
}

/**
 * @brief Clears the Largest BLOCK number of each LUT entry
 * @details [long description]
 */
 void format_FS(void)
 {
  LUT_Entry_t* entry;
  for(int i = 0; i < M_LUT_SIZE_CLUSTERS; i++ ){
    eDisk_Read(0, (BYTE *)&resident_mem[0][0], 0, M_BLOCKS_PER_CLUSTER);
    entry = (LUT_Entry_t *)&resident_mem[0][0];
    for(int j = 0; j < M_BLOCKS_PER_CLUSTER*M_BLOCK_SIZE/sizeof(LUT_Entry) ; j++){
      entry[j].LBid = 0;
      entry[j].firstCluster = 0;

    }
    eDisk_Write(0, (BYTE *)&resident_mem[0][0], 0, M_BLOCKS_PER_CLUSTER);
  }
  //create_root();    
}


/**
 * @brief Create a new File
 * @details Does not check if file exists. Grabs a new unallocated LUT entry
 * 
 * @param fname FileName
 * @return fid
 */
int create_file(char* fname){
  //Do not check if file exists, just create a new one;
  LUT_Entry_t* entry;
  for(int i = 0; i < M_LUT_SIZE_CLUSTERS; i++ )
  {
    eDisk_Read(0, (BYTE *)&resident_mem[0][0], i*M_BLOCKS_PER_CLUSTER, M_BLOCKS_PER_CLUSTER);
    entry = (LUT_Entry_t *)&resident_mem[0][0];
    for(int j = 0; j < M_BLOCKS_PER_CLUSTER*M_BLOCK_SIZE/sizeof(LUT_Entry) ; j++)
    {

      if(entry[j].LBid == 0)
      {
        entry[j].A = rand();
        entry[j].B = rand();
        uint32_t bID = get_effin_clusterID(1, &entry[j]);

        //TODO, CHECK IF cluster ALREADY ALLOCATED

        entry[j].firstCluster = 1;
        entry[j].LBid = 1;
        uint32_t LBA = getLBAfromEffinCID(bId);

        eDisk_Read(0, (BYTE *)&resident_mem[1][0], LBA, 1);
        eDisk_Write(0, (BYTE *)&resident_mem[0][0], i*M_BLOCKS_PER_CLUSTER, M_BLOCKS_PER_CLUSTER);

        (*(uint32_t *)&resident_mem[1][0]) = (i+1)*(j+1); //File ID
        (*(uint32_t *)&resident_mem[1][4]) = 0; //Size of file default 0
        strcpy(&resident_mem[1][8] , fname);
        eDisk_Write(0, (BYTE *)&resident_mem[1][0], LBA, 1);
        return (i+1)*(j+1);
      }
    }

    eDisk_Write(0, (BYTE *)&resident_mem[0][0], i*M_BLOCKS_PER_CLUSTER, M_BLOCKS_PER_CLUSTER);
  }    
}

Fid_t open(char* fname){
  LUT_Entry_t* entry;
  for(int i = 0; i < M_LUT_SIZE_CLUSTERS; i++ )
  {
    eDisk_Read(0, (BYTE *)&resident_mem[0][0], i*M_BLOCKS_PER_CLUSTER, M_BLOCKS_PER_CLUSTER);
    entry = (LUT_Entry_t *)&resident_mem[0][0];
    for(int j = 0; j < M_BLOCKS_PER_CLUSTER*M_BLOCK_SIZE/sizeof(LUT_Entry) ; j++)
    {
      eDisk_ReadBlock((BYTE *)&resident_mem[1][0], 0);
      uint32_t bID = get_effin_clusterID(entry[j].firstCluster, &entry[j]);
      uint32_t LBA = getLBAfromEffinCID(bId);

      if(strcmp(&resident_mem[1][8] , fname) == 0){
        bID = get_effin_clusterID(entry[j].LBid, &entry[j]);
        LBA = getLBAfromEffinCID(bId);
        eDisk_Read(0, (BYTE *)&resident_mem[1][0], LBA, M_BLOCKS_PER_CLUSTER);
        Fid_t fileId;
        fileId.file_size = (*(uint32_t *)&resident_mem[1][4]);
        memcpy(&fileId.entry, &entry[j], sizeof(LUT_Entry_t));
        return fileId;
      }//End if
    }
  }    
}
}
/**
 * @brief Create a Root folder
 * @details [long description]
 */

void create_root(void){
  LUT_Entry_t* entry;
  eDisk_ReadBlock((BYTE *)&resident_mem[0][0], 0);
  entry = (LUT_Entry_t *)&resident_mem[0][0];
  entry->A = rand();
  entry->B = rand();
  uint32_t bID = get_effin_clusterID(1, entry);

  entry->firstCluster = 1;
  entry->LBid = 1;

  eDisk_WriteBlock((BYTE *)&resident_mem[0][0], 0);
  uint32_t LBA = getLBAfromEffinCID(bId);
  eDisk_Read(0, (BYTE *)&resident_mem[1][0], LBA, 2);

  (*(uint32_t *)&resident_mem[1][0]) = 0; //ROOT
  (*(uint32_t *)&resident_mem[1][4]) = M_BLOCKS_PER_CLUSTER*M_BLOCK_SIZE; //Size of directory is 4KB
  resident_mem[1][8] = '/';
  resident_mem[1][9] = '\0';
  (*(uint32_t *)&resident_mem[1][M_BLOCK_SIZE]) = 0; //First Directory entry is 0 
  eDisk_Write(0, (BYTE *)&resident_mem[1][0], LBA, 2);
}

/*
void add_file_to_root(int fid){
  LUT_Entry_t entry;

  eDisk_Read(0, (BYTE *)&resident_mem[0][0], 0, 1);
  memcpy(&entry, &resident_mem[0][0], sizeof(LUT_Entry_t));
  eDisk_Read(0, (BYTE *)&resident_mem[0][0], getLBAfromEffinCID(get_effin_clusterID(entry.firstCluster, &entry)), M_BLOCKS_PER_CLUSTER);
  uint32_t currentCluster = entry.firstCluster;
  int i = M_BLOCK_SIZE;
  int* dirEntry = (int *)&resident_mem[0][M_BLOCK_SIZE];
  while(*dirEntry != 0 && *dirEntry != 0xFFFFFFFF){
    dirEntry++;
    i+= sizeof(int);
    if(i == M_BLOCK_SIZE*M_BLOCKS_PER_CLUSTER){
      eDisk_Read(0, (BYTE *)&resident_mem[0][0], 0, M_BLOCKS_PER_CLUSTER);
      dirEntry = (int *)&resident_mem[0][0];
      i = 0;
    }
  }
  *dirEntry = fid;

}
*/
void initialize(void){
  LUT_Entry_t* entry;
  uitilization = 0;
  for(int i = 0; i < M_LUT_SIZE_CLUSTERS; i++ ){
    eDisk_Read(0, (BYTE *)&resident_mem[0][0], i*M_BLOCKS_PER_CLUSTER, M_BLOCKS_PER_CLUSTER);
    entry = (LUT_Entry_t *)&resident_mem[0][0];
    for(int j = 0; j < M_BLOCKS_PER_CLUSTER*M_BLOCK_SIZE/sizeof(LUT_Entry) ; j++){
      if(entry[j].firstCluster != 0){
        eDisk_ReadBlock((BYTE *)&resident_mem[1][0], getLBAfromEffinCID(get_effin_clusterID(entry.firstCluster, &entry)) );
        uint32_t fSize = (*(uint32_t *)&resident_mem[1][4]);
        uitilization += fSize;
      }
    }
  }
}