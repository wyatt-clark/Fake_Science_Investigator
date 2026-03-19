// LZA.H
// Klase koje realizuju kodovanje i dekodovanje po Lempel-Zivovom algoritmu (LZA) iz 1977.
// i Bender-Wolfovoj modifikaciji (LZBWA) iz 1991. godine
// Predrag Radivojac, May 1999.

# ifndef __lza77
# define __lza77

# include <vector>
# include <string>
# include <algorithm>
# include <functional>


using namespace std;

typedef unsigned char byte;  // kod 'prenosenja' koda paziti na to da
typedef unsigned int uint4;  // uint4 bude 4 bajta, a char 8 bita

// maksimalna velicina niza za kompresiju; ostavljen je prostor ako nema kompresije
// da se niz moze prosiriti 9/8 puta (9/8 * exp2(31) < exp2(32))
const uint4 MaxDataLen = 1 << 30;

// maksimalna velicina prozora; 20 se moze i povecati, ali nema potrebe
const uint4 MaxWinLen = 1 << 20;

// maksimalna duzina match-a kod BW algoritma; BW kod nekog broja ne sme preci 32 bita;
// za broj predstavljen sa 16 cifara BW kod je duzine 31 bit
const uint4 MaxMatchLenBW = (1 << 16) - 1;

# ifdef _DEBUGGING
# include <iostream.h>
ostream& operator << (ostream& out, const vector <byte>& v)
{
  for (uint4 i = 0; i < v.size(); i++)
    cout << uint4(v[i]) << " ";
  return out;
}
# endif

//-------------------------------------------------------------------------------
// class LZA
//-------------------------------------------------------------------------------

class LZA
{
  protected:
    uint4 Window,    // velicina prozora
          LdWindow,  // gornje celo od log2(Window), tj. broj bita za match_dep
          max_window_len, // maksimalna velicina prozora
          max_data_len,   // maksimalna velicina ulaznog niza
          match_ind,  // indikator da li ima match-a (1) ili ne (0)
          match_dep,  // dubina match-a
          match_len,  // duzina match-a
          end,        // broj simbola preostalih za kodovanje (kod kodera)
                      // broj dekodovanih simbola (kod dekodera)
          dst_no,     // broj ucitanih bajtova iz niza Dst kod dekodera
          FreePos,    // broj neiskoriscenih pozicija u CompressedWord (koder)
                      // broj zauzetih pozicija u CompressedWord (dekoder)
          CompressedWord; // privremena rec za pakovanje i raspakivanje

    bool end_possible;  // ako je 'true' moze se zavrsiti dekodovanje

    vector <byte> Src;  // nekodovani (koder) ili dekodovani (dekoder) niz
    vector <byte> Dst;  // kodovani niz

  public:
    LZA () : max_data_len(MaxDataLen), max_window_len(MaxWinLen) {};
    ~LZA ()
    {
      if (Src.size() > 0) Src.erase(Src.begin(), Src.end());
      if (Dst.size() > 0) Dst.erase(Dst.begin(), Dst.end());
    };

    uint4 GetEncodedLen () const {return Dst.size();};
    vector <byte> GetEncoded () const {return Dst;};

    uint4 GetDecodedLen () const {return Src.size();};
    vector <byte> GetDecoded () const {return Src;};

    float GetRatio () const {return (float)Src.size() / Dst.size();};

    bool Encode (const vector <byte> & src, uint4 win = 8192);
	bool Decode (const vector <byte> & dst, uint4 win = 8192);

  protected:
    // funkcije koje koristi koder
    uint4 ceil_log2 (uint4);
    virtual uint4 find_best_match (void);
    bool initialize_encoder (vector <byte> const &, uint4);
    void pack (uint4, uint4);
    void read_source (uint4);
    virtual uint4 write_codeword (void);

    // funkcije koje koristi dekoder
    uint4 exp2 (uint4);
    virtual void extract_match (void);
    bool initialize_decoder (vector <byte> const &, uint4);
    void load_CompressedWord (void);
    virtual bool not_end_of_decoding (void);
    virtual uint4 read_codeword (void);
    uint4 unpack (uint4);
};

//-------------------------------------------------------------------------------
// class LZBWA
//-------------------------------------------------------------------------------

class LZBWA : public LZA
{
  private:
    uint4 max_match_len,  // maksimalna duzina match-a
          match2_len;     // velicina drugog najboljeg match-a
  public:
    LZBWA () : LZA (), max_match_len(MaxMatchLenBW) {};

    // funkcije koje koristi koder
    uint4 bin_length (uint4);
    uint4 bw_encode (uint4);
    uint4 bw_length (uint4);
    uint4 find_best_match (void);
    uint4 write_codeword (void);

    // funkcije koje koristi dekoder
    void extract_match (void);
    uint4 invert (uint4, uint4);
    bool not_end_of_decoding (void);
    uint4 read_codeword (void);
    uint4 read_bw_length (void);
    uint4 second_match_length (void);
};

# endif

