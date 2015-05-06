
struct LUT_entry{
  int A;
  int B;
  uint32_t first_cluster;
  uint32_t last_cluster;


};

using std::vector;

void readLUTfile(int clusterNum, vector<LUT_entry>& resident_LUT){
  ifstream infile;
  resident_LUT.clear();
  std::string fname = std::to_string(clusterNum) + ".dat";
  infile.open(fname, ios::binary | ios::in);

  for(int i = 0; i < M_BLOCKS_PER_CLUSTER*M_BLOCK_SIZE/M_LUT_ENTRY_SIZE; i++){
    LUT_entry x;
    infile.read(&x.A, sizeof(int));
    infile.read(&x.B, sizeof(int));
    infile.read(&x.first_cluster, sizeof(uint32_t));
    infile.read(&x.last_cluster, sizeof(uint32_t));
    resident_LUT.push_back(LUT_entry);
  }
  infile.close();
}

void writeLUTfile(int clusterNum, vector<LUT_entry>& resident_LUT){
  ofstream outfile;
  // resident_LUT.clear();
  std::string fname = std::to_string(clusterNum) + ".dat";
  outfile.open(fname, ios::binary | ios::out);

  for(int i = 0; i < M_BLOCKS_PER_CLUSTER*M_BLOCK_SIZE/M_LUT_ENTRY_SIZE; i++){
    LUT_entry x = resident_LUT[i];
    outfile.write(&x.A, sizeof(int));
    outfile.write(&x.B, sizeof(int));
    outfile.write(&x.first_cluster, sizeof(uint32_t));
    outfile.write(&x.last_cluster, sizeof(uint32_t));
    //resident_LUT.push_back(LUT_entry);
  }
  outfile.close();
}

