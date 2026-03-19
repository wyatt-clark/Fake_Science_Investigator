# include "lza.h"
# include <stdio.h>
# include <curses.h>
# include <stdlib.h>
# include <math.h>
# include <ctype.h>
# include <string.h>
# include <fstream>
# include <iterator>
# include <iostream>

const int LEN = 100000;

//LZA x2;
LZBWA x2;

char *Source;
vector <byte> Src, Dst, Rec;

void input_file (char* p, char* seq, int& number);
void output_file (char* seq, char* p, int number);
void array_to_vector (const byte* src, vector <byte> & dst, const uint4& n);
void vector_to_array (const vector <byte> & src, byte* dst, uint4& n);

//--------------------------------------------------------------------------
// main ()
//--------------------------------------------------------------------------

//void main (void)
main (int argc, char *argv[])
{
  if (argc != 4)
  {
      cout << "Incorrect number of input arguments"  << endl;
      exit (0);
  }

  char in_file[200];
  strcpy(in_file, argv[1]);

  char out_file[200];
  strcpy(out_file, argv[3]);

  char win_str[200];
  strcpy(win_str, argv[2]);


  int Window = 0;
  int i = 0;
  while (win_str[i] != '\0')
  {
      Window = Window * 10 + int(win_str[i]) - 48;
      i++;
  }
      
  //int Window = 16384;

  uint4 size_uncmpr = 0;
  int file_length = LEN;

  Source = new char [3 * LEN];

  input_file(in_file, Source, file_length);

  array_to_vector((const unsigned char *) Source, Src, file_length);

  x2.Encode(Src, Window);
  
  //Dst = x2.GetEncoded();

 // cout << endl << "Length of a coded sequence: " << x2.GetEncodedLen();

  //x2.Decode(Dst, Window);
  //Rec = x2.GetDecoded();

  //cout << "Factor: " << (x2.GetRatio()) << endl;


  //vector_to_array(Rec, (unsigned char *) Source, size_uncmpr);
  


  // output the compression rate into the file
  FILE* file_handle;
  if((file_handle = fopen(out_file, "w+" )) == NULL)
  {
      cout << endl << endl << "Error in file opening" << endl;
      exit(0);
  }
  fprintf(file_handle, "%f", (double)file_length / x2.GetEncodedLen());
  fclose(file_handle);
 
  
  //output_file(Source, "output.dat", size_uncmpr);
  
  Src.erase(Src.begin(), Src.end()); Dst.erase(Dst.begin(), Dst.end()); Rec.erase(Rec.begin(), Rec.end());

  //getch();
 
  delete [] Source;
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------

void input_file (char* p, char* seq, int& number)
{
	ifstream inseq(p, ios::in | ios::binary);
	if (!inseq) {
		cout << "Cannot open file " << p << endl << endl;
		exit (0);
		}
	inseq.read ((char*)seq, number);
	number = inseq.gcount();
	//cout << "I read " << inseq.gcount() << " characters" << endl;
	inseq.close();
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------

void output_file (char* seq, char* p, int number)
{
	ofstream outseq(p, ios::out | ios::binary);
	if (!outseq) {
		cout << "Cannot open file " << p << endl << endl;
		exit(0);
		}
	outseq.write ((char*)seq, number);
	outseq.close();
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------

void array_to_vector (const byte* src, vector <byte> & dst, const uint4& n)
{
  if (dst.size() > 0) dst.erase(dst.begin(), dst.end());
  copy(src, src + n, back_inserter(dst));
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------

void vector_to_array (const vector <byte> & src, byte* dst, uint4& n)
{
  n = src.size();
  copy(src.begin(), src.end(), dst);
}

//--------------------------------------------------------------------------
// End of file
//--------------------------------------------------------------------------


