// LZA.CPP
// Klase koje realizuju kodovanje i dekodovanje po Lempel-Zivovom algoritmu (LZA) iz 1977.
// i Bender-Wolfovoj modifikaciji (LZBWA) iz 1991. godine
// Predrag Radivojac, May 1999.

# include "lza.h"

//-------------------------------------------------------------------------------
// Encode()
// vraca 0 ako nije izvrseno kodovanje; inace vraca 1
//-------------------------------------------------------------------------------

bool LZA :: Encode (const vector <byte> & src, uint4 win)
{
  uint4 shift;

  if (!initialize_encoder(src, win)) return 0;

  for (end = Src.size() - Window; end > 0; end -= shift) {
    match_ind = find_best_match();
    shift = write_codeword();
    }

  if (FreePos != 32)
    copy((byte*)&CompressedWord,
         ((byte*)&CompressedWord) + 4 - FreePos / 8,
         back_inserter(Dst));
  return 1;
}

//-------------------------------------------------------------------------------
// initialize_encoder ()
//-------------------------------------------------------------------------------

bool LZA :: initialize_encoder (vector <byte> const & src, uint4 win)
{
  if (src.size() >= max_data_len) return 0;

	FreePos = 32;
	CompressedWord = 0;

  Window = (win > max_window_len || win < 2) ? max_window_len : win;
  LdWindow = ceil_log2(Window);

  // Window nula umetnuto je na pocetak nekodovane sekvence
  Src = vector <byte> (Window, 0);
  Src.insert(Src.end(), src.begin(), src.end());
  if (Dst.size() > 0) Dst.erase(Dst.begin(), Dst.end());
  return 1;
}

//-------------------------------------------------------------------------------
// find_best_match ()
//-------------------------------------------------------------------------------

uint4 LZA :: find_best_match ()
{
  byte *l, *m, *n, *q = &(Src[Src.size() - end]);
  //vector <byte> :: iterator l, m, n, q = Src.end() - end;
  uint4 i, j, k = (end < Window) ? end : Window; // match_len <= k

  match_dep = match_len = 0;

  // postavi l na karakter u prozoru takav da je *l = *source_temp
  for (l = q - 1, j = 1; *q != *l && j <= Window; j++, l--);

  // match se trazi maksimalno Window pozicija iza q
  while (j <= Window) {
      m = l; n = q;
      for (i = 0; *m == *n && i < k; n++, m++, i++);
      if ((match_len < i)  && (2 * LdWindow + 1 < 9 * i)) {
        match_len = i;
        match_dep = j;
        }
      for (--l, ++j; *q != *l && j <= Window; j++, l--);
      }
  return (match_dep > 0) ? 1 : 0;
}

//-------------------------------------------------------------------------------
// write_codeword ()
// vraca vrednost pomeraja privremenog pokazivaca na nekomprimovani niz
//-------------------------------------------------------------------------------

uint4 LZA :: write_codeword (void)
{
  if (match_ind) {
    pack(1, 1);                      // [1, match_dep, match_len] format
    pack(match_dep - 1, LdWindow);
    pack(match_len - 1, LdWindow);
    }
  else
    pack(uint4(Src[Src.size() - end]) << 1, 9); // [0, character] format

  return (match_ind) ? match_len : 1;
}

//-------------------------------------------------------------------------------
// pack ()
// length bita reci x upise u prve slobodne bite reci C'Word; ako je length >=
// broja slobodnih pozicija (FreePos) potrebno je 4 bajta iz C'Word upisati u
// kodovani niz (Dst); u C'Word upisuje se zdesna na levo, prvo indikator matcha,
// a zatim ostalo; za length > 31 f-ja ne radi dobro
//-------------------------------------------------------------------------------

void LZA :: pack (uint4 x, uint4 length)
{
  if (FreePos > length) {
	  CompressedWord |= x << (32 - FreePos);
    FreePos -= length;
    }
  else if (FreePos == length) {
    CompressedWord |= x << (32 - FreePos);
    copy((byte*)&CompressedWord, (byte*)&CompressedWord + 4, back_inserter(Dst));
    CompressedWord = 0;
    FreePos = 32;
    }
  else {
    CompressedWord |= x << (32 - FreePos);
    copy((byte*)&CompressedWord, (byte*)&CompressedWord + 4, back_inserter(Dst));
    CompressedWord = x >> FreePos;
    FreePos = 32 - length + FreePos;
    }
}

//--------------------------------------------------------------------------
// ceil_log2 ()
// vraca gornje celo od logaritma baze dva argumenta; konstruktorom klase
// LZA obezbedjeno je da argument 'n' ne prekoraci svoj maksimum koji je
// 8 * sizeof(uint4) - 2
//--------------------------------------------------------------------------

uint4 LZA :: ceil_log2 (uint4 n)
{
  uint4 index, m = n;
  for (uint4 i = 0; i < 31; i++, m >>= 1)
    if (m & 1) index = i;
  return (n ^ (1 << index)) ? index + 1 : index;
}

//-------------------------------------------------------------------------------
// Decode()
// vraca 0 ako nije izvrseno dekodovanje; inace vraca 1
//-------------------------------------------------------------------------------

bool LZA :: Decode (const vector <byte> & dst, uint4 win)
{
  if (!initialize_decoder(dst, win)) return 0;

  while (not_end_of_decoding()) {
      read_codeword();
      extract_match();
      }

  Src.erase(Src.begin(), Src.begin() + Window);
  return 1;
}

//-------------------------------------------------------------------------------
// initialize_decoder ()
//-------------------------------------------------------------------------------

bool LZA :: initialize_decoder (vector <byte> const & dst, uint4 win)
{
  if (dst.size() >= max_data_len) return 0;

  // Src pocinje sa Window nula na koje se nadoda dekodovana sekvenca, a
  // nule se po zavrsetku dekodovanja izbrisu
  Src = vector <byte> (Window, 0);
  Dst = dst;

  Window = (win > max_window_len || win < 2) ? max_window_len : win;
  LdWindow = ceil_log2(Window);

  end_possible = false;
  dst_no = 0;
  end = Window;

	FreePos = 32;
  load_CompressedWord();  // mora biti ispod dst_no jer se dst_no menja tu

  match_ind = match_dep = match_len = 0;
  return 1;
}

//-------------------------------------------------------------------------------
// read_codeword ()
//-------------------------------------------------------------------------------

uint4 LZA :: read_codeword (void)
{
  match_ind = unpack(1);
  if (match_ind) {
    match_dep = unpack(LdWindow) + 1;
    match_len = unpack(LdWindow) + 1;
    }
  else
    match_dep = unpack(8);
  return (match_ind) ? match_len : 1;
}

//-------------------------------------------------------------------------------
// unpack ()
// iz reci C'Word izdvaja length bita; za length > 31 ne radi dobro
//-------------------------------------------------------------------------------

uint4 LZA :: unpack (uint4 length)
{
  uint4 mask = exp2(length) - 1, tmp;

  if (FreePos > length) {
    tmp = CompressedWord & mask;
	  CompressedWord >>= length;
    FreePos -= length;
    end_possible = true;
    }
  else if (FreePos == length) {
    tmp = CompressedWord & mask;
    end_possible = (dst_no == Dst.size()) ?  true : false;
    load_CompressedWord();
    FreePos = 32;
    }
  else {
    tmp = CompressedWord & mask;
    load_CompressedWord();
    mask = exp2(length - FreePos) - 1;
    tmp |= (CompressedWord & mask) << FreePos;
    CompressedWord >>= length - FreePos;
    FreePos = 32 - length + FreePos;
    end_possible = true;
    }
  return tmp;
}

//-------------------------------------------------------------------------------
// extract_match ()
// ako ima match-a ubaci match_len simbola u dekodovani niz, inace samo 1
//-------------------------------------------------------------------------------

void LZA :: extract_match (void)
{
  if (match_ind)
    for (uint4 i = 0; i < match_len; i++) {
      Src.push_back(Src[end - match_dep]);
      end++;
      }
  else {
    Src.push_back((byte)match_dep);
    end++;
    }
}

//-------------------------------------------------------------------------------
// load_CompressedWord ()
//-------------------------------------------------------------------------------

void LZA :: load_CompressedWord (void)
{
  if (Dst.size() > dst_no + 4) {
    copy(Dst.begin() + dst_no,
         Dst.begin() + dst_no + 4,
         (byte*)&CompressedWord);
    dst_no += 4;
    }
  else {
    copy(Dst.begin() + dst_no, Dst.end(), (byte*)&CompressedWord);
    dst_no = Dst.size();
    }
}

//-------------------------------------------------------------------------------
// not_end_of_decoding ()
//-------------------------------------------------------------------------------

bool LZA :: not_end_of_decoding (void)
{
  if (dst_no == Dst.size() && end_possible &&        // ako je u pitanju
      (Dst.size() % 4 + FreePos / 8) % 4 == 0) {     // poslednji bajt
    if (FreePos - FreePos / 8 * 8 >= 2 * LdWindow + 1) // ostatak upotrebljiv
      return (CompressedWord & 0x1) ? true : false;  // ako ima nesto u ostatku
    else
      return false;
    }
  return true;
}

//-------------------------------------------------------------------------------
// exp2 ()
//-------------------------------------------------------------------------------

uint4 LZA :: exp2 (uint4 i)
{
 return 1l << i;
}

//---------------------------------Bender-Wolf-----------------------------------

//-------------------------------------------------------------------------------
// find_best_match ()
//-------------------------------------------------------------------------------

uint4 LZBWA :: find_best_match ()
{
  byte *l, *m, *n, *q = &(Src[Src.size() - end]);
  //vector <byte> :: iterator l, m, n, q = Src.end() - end;
  uint4 i, j, k = (end < max_match_len) ? end : max_match_len;

  match_dep = match_len = match2_len = 0;

  // postavi l na karakter u prozoru takav da je *l = *source_temp
  for (l = q - 1, j = 1; *q != *l && j <= Window; j++, l--);

  // match se trazi maksimalno Window pozicija iza q
  while (j <= Window) {
      m = l; n = q;
      for (i = 0; *m == *n && i < k; n++, m++, i++);
      if (match_len < i) {
        match2_len = match_len;
        match_len = i;
        match_dep = j;
        }
      for (--l, ++j; *q != *l && j <= Window; j++, l--);
      }

  return (match_dep > 0) ? 1 : 0;
}

//-------------------------------------------------------------------------------
// write_codeword ()
// vraca vrednost pomeraja privremenog pokazivaca na nekomprimovani niz; Bender-
// Wolfov kod duzine match-a ne sme biti duzi od 32 bita zbog pack() -> duzina
// mecha mora biti manja od exp2(16) - 1
//-------------------------------------------------------------------------------

uint4 LZBWA :: write_codeword (void)
{
  if (match_ind) {
    pack(1, 1);               // [1, match_dep, match_len - match2_len] format
    pack(match_dep - 1, LdWindow);
    pack(bw_encode(match_len - match2_len), bw_length(match_len - match2_len));
    }
  else
    pack(uint4(Src[Src.size() - end]) << 1, 9);       // [0, character] format

  return (match_ind) ? match_len : 1;
}

//-------------------------------------------------------------------------------
// bw_encode ()
// formira Bender-Wolfov kod za broj x
//-------------------------------------------------------------------------------

uint4 LZBWA :: bw_encode (uint4 x)
{
  uint4 len = bin_length(x), y = 0;
  for (uint4 i = 0; i < len; i++) {
    y <<= 1;
    y |= x & 0x1;
    x >>= 1;
    }
  return y << (len - 1);
}

//-------------------------------------------------------------------------------
// bin_length ()
// odredjuje broj bita potrebnih za binarnu reprezentaciju reci x
//-------------------------------------------------------------------------------

uint4 LZBWA :: bin_length (uint4 x)
{
  uint4 i = 1;
  for (; x > 1; i++) x >>= 1;
  return i;
}

//-------------------------------------------------------------------------------
// bw_length ()
// odredjuje broj bita Bender-Wolfovog koda kojim se predstavlja rec x
//-------------------------------------------------------------------------------

uint4 LZBWA :: bw_length (uint4 x)
{
  return 2 * bin_length(x) - 1;
}

//-------------------------------------------------------------------------------
// read_codeword ()
// ucitava indikator match-a i u zavisnosti od njega ostale elemente kodne reci
//-------------------------------------------------------------------------------

uint4 LZBWA :: read_codeword (void)
{
  match_ind = unpack(1);
  if (match_ind) {
    match_dep = unpack(LdWindow) + 1;
    uint4 len = read_bw_length();
    match_len = invert(unpack(len), len);
    }
  else
    match_dep = unpack(8);
  return (match_ind) ? match_len : 1;
}

//-------------------------------------------------------------------------------
// read_bw_length ()
// octava broj nula koje idu kao zapis BW koda; broj znacajnih bita je za 1 veci
//-------------------------------------------------------------------------------

uint4 LZBWA :: read_bw_length (void)
{
  uint4 i, fp = FreePos, x;

  for (i = 0; i < fp && !(CompressedWord & 0x1); i++) {
    CompressedWord >>= 1;
    FreePos--;
    }
  x = i;
  if (FreePos == 0) {
    load_CompressedWord();
    FreePos = fp = 32;
    for (i = 0; i < fp && !(CompressedWord & 0x1); i++) {
      CompressedWord >>= 1;
      FreePos--;
      }
    x += i;
    }
  return x + 1;
}

//-------------------------------------------------------------------------------
// extract_match ()
// ako ima match-a ubaci match_len simbola u dekodovani niz, inace samo 1
//-------------------------------------------------------------------------------

void LZBWA :: extract_match (void)
{
  if (match_ind) {
    match_len += second_match_length();
    for (uint4 i = 0; i < match_len; i++) {
      Src.push_back(Src[end - match_dep]);
      end++;
      }
    }
  else {
    Src.push_back((byte)match_dep);
    end++;
    }
}

//-------------------------------------------------------------------------------
// second_match_length ()
// odredjuje velicinu drugog najboljeg match-a poredeci ga sa najboljim
//-------------------------------------------------------------------------------

uint4 LZBWA :: second_match_length (void)
{
  uint4 i, ii, l, m, n, k, match2_len = 0;

  for (l = end - match_dep, k = end - 1; Src[l] != Src[k] && k > l; k--);

  while (k > l) {
		  m = l; n = k;
      ii = 0; // broj cifara koje zbog bootstrap-ovanja prelaze 'end'
		  for (i = 0; Src[n] == Src[m] && i < max_match_len ; n++, m++, i++)
        if (n >= end - 1) {
          Src.push_back(Src[end + ii - match_dep]);
          ii++;
          }
      if (n >= end) Src.erase(Src.end() - ii, Src.end());

		  if (match2_len < i)
		    match2_len = i;

		  for (--k; Src[l] != Src[k] && k > l; k--);
		  }
  return match2_len;
}

//--------------------------------------------------------------------------
// invert ()
// obrce len LSB bita reci x i to salje na izlaz
//--------------------------------------------------------------------------

uint4 LZBWA :: invert (uint4 x, uint4 len)
{
  uint4 inv_x = 0;
  for (uint4 i = 0; i < 32; i++) {
    inv_x <<= 1;
    inv_x |= (x & 0x1);
    x >>= 1;
    }
  return inv_x >> (32 - len);
}

//-------------------------------------------------------------------------------
// not_end_of_decoding ()
//-------------------------------------------------------------------------------

bool LZBWA :: not_end_of_decoding (void)
{
  if (dst_no == Dst.size() && end_possible &&  // ako je u pitanju poslednji bajt
      (Dst.size() % 4 + FreePos / 8) % 4 == 0) {
    if (FreePos - FreePos / 8 * 8 >= LdWindow + 2)  // ostatak usable
      return (CompressedWord & 0x1) ? true : false; // ako ima nesto u ostatku
    else
      return false;
    }
  return true;
}

//-------------------------------------------------------------------------------
// end
//-------------------------------------------------------------------------------

