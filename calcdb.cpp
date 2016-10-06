#include <iostream>
//#include <algo.h>
#include <algorithm>
#include <string>
#include <sstream>
#include "calcdb.h"
#include "eclat.h"

using namespace std;

int *Dbase_Ctrl_Blk::FreqIdx=NULL;
int *Dbase_Ctrl_Blk::FreqMap=NULL;
int Dbase_Ctrl_Blk::MaxTransSz=0;
int Dbase_Ctrl_Blk::TransSz=0;
int *Dbase_Ctrl_Blk::TransAry=NULL;
int Dbase_Ctrl_Blk::Tid=0;
int Dbase_Ctrl_Blk::Cid=0;
int Dbase_Ctrl_Blk::NumF1=0;
int Dbase_Ctrl_Blk::Cidsum=0;
int *Dbase_Ctrl_Blk::PvtTransAry=NULL;
int *Dbase_Ctrl_Blk::class_sz=NULL;
int *Dbase_Ctrl_Blk::F2sum=NULL;
vector<Eqnode *> Dbase_Ctrl_Blk::ParentClass;
bool Dbase_Ctrl_Blk::binary_input=false;
bool Dbase_Ctrl_Blk::nooffset=false;

Dbase_Ctrl_Blk::Dbase_Ctrl_Blk(const char *infile, const int buf_sz)
{
   if (binary_input){
      fd.open(infile, ios::binary);
      if (!fd.is_open()){
         cerr << "cannot open infile" << infile << endl;
         exit(1);
      }
   }
   else{
      fd.open(infile);
      if (!fd.is_open()){
         cerr << "cannot open infile" << infile << endl;
         exit(1);
      }
   }   
   
   buf_size = buf_sz;
   buf = new int [buf_sz];
   cur_buf_pos = 0;
   cur_blk_size = 0;
   readall = 0;
   fd.seekg(0,ios::end);
   endpos = fd.tellg();
   fd.seekg(0,ios::beg);
   
   if (nooffset){
      Tid = Cid = -1;
   }
}
   
Dbase_Ctrl_Blk::~Dbase_Ctrl_Blk()
{
   delete [] buf;
   fd.close();
}



void Dbase_Ctrl_Blk::reset()
{
   readall=0;

   fd.clear();
   fd.seekg(0,ios::beg);
   
   cur_buf_pos = 0;
   if (nooffset){
      Tid = Cid = 0;
   }
}

void Dbase_Ctrl_Blk::get_first_blk()
{
   reset();
   if (binary_input){
      fd.read((char *)buf, (buf_size*ITSZ));
      cur_blk_size = fd.gcount()/ITSZ; 
      if (cur_blk_size < 0){
         cerr << "problem in get_first_blk" << cur_blk_size << endl;
      }
      if (cur_blk_size < buf_size){
         fd.clear();
         fd.seekg(0,ios::end);
      }
   }
}

int Dbase_Ctrl_Blk::get_next_trans ()
{
   static char first=1;  
   static const int linesz = 16834;
   static const int wordsz = 1024;
   static char line[linesz];
   static char word[wordsz];

   if (binary_input){
      if (first){
         first = 0;
         get_first_blk();
      }
      
      if (cur_buf_pos+TRANSOFF >= cur_blk_size ||
          cur_buf_pos+buf[cur_buf_pos+TRANSOFF-1]+TRANSOFF > cur_blk_size){
         //fd.seekg(0,ios::cur);
         //if ((int)fd.tellg() == endpos) readall = 1;      
         if (fd.eof()) readall = 1;
         if (!readall){
            // Need to get more items from file
            get_next_trans_ext();
         }      
      }
      
      if (eof()){
         first = 1;
         return 0;
      }                     
      
      if (!readall){
         Cid = buf[cur_buf_pos];
         Tid = buf[cur_buf_pos+TRANSOFF-2];
         TransSz = buf[cur_buf_pos+TRANSOFF-1];
         TransAry = buf + cur_buf_pos + TRANSOFF;
         cur_buf_pos += TransSz + TRANSOFF;
      }
      return 1;
   }
   else{
      //fd.seekg(0,ios::cur);
      //if (fd.tellg() == endpos) readall = 1;

//       if ((int)fd.tellg() == endpos-1){
//          readall = 1;
//          first = 1;
//          return 0;
//       }
//       else{
      fd.getline(line, linesz);
      int line_len = fd.gcount();//how many chars read
      if (fd.eof()){
         first = 0;
         reset();
         return 0;
      }
      else{
         
         //istringstream istrs ((char *)line.c_str());
         parseline pl(line, line_len);
         //cout << "LINE " << line.c_str() << endl;
         if (nooffset){
            Cid++;
            Tid++;
         }
         else{
            pl.next(word);
            Cid = atoi(word);
            pl.next(word);
            Tid = atoi(word);
            pl.next(word);
            TransSz = atoi(word);
            //cout << " CAME HERE\n";
         }
         
         //cout << "CID " << Cid << " " << Tid << " " << TransSz << endl;
         TransSz = 0;
         while (pl.next(word)){
            buf[TransSz] = atoi(word);
            TransSz++;
         }
         
         TransAry = buf;
         cur_buf_pos = 0;
         
         //cout << "ENDPOS " << fd.tellg() << " " << endpos << endl;

         return 1;
      }
   }
}

void Dbase_Ctrl_Blk::get_next_trans_ext()
{
   // Need to get more items from file
   int res = cur_blk_size - cur_buf_pos;
   if (res > 0)
   {
      // First copy partial transaction to beginning of buffer
      for (int i=0; i < res; ++i)
         buf[i] = buf[cur_buf_pos+i]; 
      cur_blk_size = res;
   }
   else
   {
      // No partial transaction in buffer
      cur_blk_size = 0;
   }

   fd.read((char *)(buf + cur_blk_size),
           ((buf_size - cur_blk_size)*ITSZ));
   
   res = fd.gcount();
   if (res < 0){
      cerr << "in get_next_trans_ext" << res << endl;
   }
   
   //if (res < (buf_size - cur_blk_size)){
   //   fd.clear();
   //   fd.seekg(0,ios::end);
   //}

   cur_blk_size += res/ITSZ;
   cur_buf_pos = 0;
}


void Dbase_Ctrl_Blk::get_valid_trans()
{
   int i,j;
   const int invalid=-1; //-1 does not appear in original trans
   
   if (PvtTransAry == NULL)
      PvtTransAry = new int [MaxTransSz];
   
   //copy valid items to PvtTransAry
   for (i=0,j=0; i < TransSz; ++i){
      if (FreqMap[TransAry[i]] != invalid){
         PvtTransAry[j] = FreqMap[TransAry[i]];
         ++j;
      }
   }
   TransAry = PvtTransAry;
   TransSz = j;

   //cout << "NEW " << endl;
   //for (i=0; i < TransSz; ++i)
   //   cout << TransAry[i] << " ";
   //cout << endl;
}

void Dbase_Ctrl_Blk::print_trans(){
  cout << Cid << " " << Tid << " " << TransSz;
  for (int i=0; i < TransSz; ++i)
    cout << " " << TransAry[i];
  cout << endl;
}


void Dbase_Ctrl_Blk::alloc_ParentClass(vector<int> &itcnt)
{
   //allocate space for Idlists
   ParentClass.resize(NumF1);
   for (int i=0; i < NumF1; ++i){
      ParentClass[i] = new Eqnode(i, itcnt[FreqIdx[i]]);   
      if (diff_type == diffin) ParentClass[i]->hval = Cidsum;
   }
}

bool Dbase_Ctrl_Blk::incr_cmp(Eqnode *n1, Eqnode *n2)
{
   if (class_sz[n1->val] < class_sz[n2->val]) return true;
   else if (class_sz[n1->val] == class_sz[n2->val]){
      if (F2sum[n1->val] < F2sum[n2->val]) return true;
      else if (F2sum[n1->val] == F2sum[n2->val]){
         if (n1->sup < n2->sup) return true;
         else return false;
      }
      else return false;
   }
   else return false;
   //if (F2sum[n1->val] < F2sum[n2->val]) return true;
   //else return false;
}

bool Dbase_Ctrl_Blk::decr_cmp(Eqnode *n1, Eqnode *n2)
{
   return !incr_cmp(n1,n2);
}

void Dbase_Ctrl_Blk::sort_ParentClass()
{
   if (sort_type == incr) 
      sort(ParentClass.begin(), ParentClass.end(), incr_cmp);
   else if (sort_type == incr_noclass) 
      sort(ParentClass.begin(), ParentClass.end(), Eqnode::incr_cmp);
   else if (sort_type == decr) 
      sort(ParentClass.begin(), ParentClass.end(), decr_cmp);
}


//assumes that get_valid_trans has been called before
void Dbase_Ctrl_Blk::make_vertical(){
   int i, j;
   
   //convert current transaction into vertical format
   if (diff_type == diffin){
      //create a diffset
      for (i=0, j=0; i < TransSz; ++j){
         if (j == TransAry[i]){
            ++i; 
         }
         else{
            ParentClass[j]->tidset.push_back(Cid);
            ParentClass[j]->hval -= Cid; //used for closed sets
         }
      }
      for (; j < NumF1; ++j){
         ParentClass[j]->tidset.push_back(Cid);
         ParentClass[j]->hval -= Cid; //used for closed sets
      }
      
   }
   else{
      //create a tidset
      for (i=0; i < TransSz; ++i){
         //cout << "push " << TransAry[i] << " " << Cid << endl;
         ParentClass[TransAry[i]]->tidset.push_back(Cid);
         ParentClass[TransAry[i]]->hval += Cid; //used for closed sets
      }
   }
}


void Dbase_Ctrl_Blk::print_vertical(){
  int i;
  for (i=0; i < NumF1; ++i){
    cout << *ParentClass[i];
  }
}
