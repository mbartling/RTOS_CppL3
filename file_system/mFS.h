#ifndef __MFS_H__
#define __MFS_H__
#include <stdint.h>

typedef __packed struct LUT_Entry {
  int A;
  int B;
  uint32_t firstCluster;
  uint32_t LBid;

} LUT_Entry_t;

typedef struct Fid{
  LUT_Entry_t entry;
  uint32_t file_size;
  uint32_t currentCluster;
  uint8_t* cursor;
} Fid_t;


void format_FS(void);

void myFS_init(void);
int create_file(char* fname);

Fid_t open(char* fname);
void mFS_initialize(void);
void write(Fid_t* fid, char c);
void close(Fid_t* fid);
// void redirect(bool);

#endif
