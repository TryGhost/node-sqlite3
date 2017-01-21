
#include "import.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <regex>
#include <vector>
#include <sstream>
#include <string>

enum ColType { CT_NONE, CT_INT, CT_REAL, CT_TEXT };

// Number of rows to examine when determining column types
// Probably conservative by an order of magnitude; tune with real corpus of CSVs.
const int METASCAN_ROWS = 1024;

const char *colTypeName(ColType ct) {
  switch (ct) {
    case CT_NONE: return "NONE";
    case CT_INT:  return "integer";
    case CT_REAL: return "real";
    case CT_TEXT: return "text";
    default:  return "UNKNOWN";
  }
}

/*
** Render output like fprintf().  Except, if the output is going to the
** console and if this is running on a Windows machine, translate the
** output from UTF-8 into MBCS.
*/
#if defined(_WIN32) || defined(WIN32)
void utf8_printf(FILE *out, const char *zFormat, ...){
  va_list ap;
  va_start(ap, zFormat);
  if( stdout_is_console && (out==stdout || out==stderr) ){
    char *z1 = sqlite3_vmprintf(zFormat, ap);
    char *z2 = sqlite3_win32_utf8_to_mbcs_v2(z1, 0);
    sqlite3_free(z1);
    fputs(z2, out);
    sqlite3_free(z2);
  }else{
    vfprintf(out, zFormat, ap);
  }
  va_end(ap);
}
#elif !defined(utf8_printf)
# define utf8_printf fprintf
#endif

/*
** Render output like fprintf().  This should not be used on anything that
** includes string formatting (e.g. "%s").
*/
#if !defined(raw_printf)
# define raw_printf fprintf
#endif

/*
** Compute a string length that is limited to what can be stored in
** lower 30 bits of a 32-bit signed integer.
*/
static int strlen30(const char *z){
  const char *z2 = z;
  while( *z2 ){ z2++; }
  return 0x3fffffff & (int)(z2 - z);
}

/*
** True if an interrupt (Control-C) has been received.
*/
static volatile int seenInterrupt = 0;

/*
** subset of ShellState we need for csv import code
*/
/*
** State information about the database connection is contained in an
** instance of the following structure.
*/
typedef struct ShellState ShellState;
struct ShellState {
  int mode;              /* An output mode setting */
  char colSeparator[20]; /* Column separator character for several modes */
  char rowSeparator[20]; /* Row separator character for MODE_Ascii */
};

/*
** These are the allowed modes.
*/
#define MODE_Line     0  /* One column per line.  Blank line between records */
#define MODE_Column   1  /* One record per line in neat columns */
#define MODE_List     2  /* One record per line with a separator */
#define MODE_Semi     3  /* Same as MODE_List but append ";" to each line */
#define MODE_Html     4  /* Generate an XHTML table */
#define MODE_Insert   5  /* Generate SQL "insert" statements */
#define MODE_Tcl      6  /* Generate ANSI-C or TCL quoted elements */
#define MODE_Csv      7  /* Quote strings, numbers are plain */
#define MODE_Explain  8  /* Like MODE_Column, but do not truncate data */
#define MODE_Ascii    9  /* Use ASCII unit and record separators (0x1F/0x1E) */
#define MODE_Pretty  10  /* Pretty-print schemas */

/*
** These are the column/row/line separators used by the various
** import/export modes.
*/
#define SEP_Column    "|"
#define SEP_Row       "\n"
#define SEP_Tab       "\t"
#define SEP_Space     " "
#define SEP_Comma     ","
#define SEP_CrLf      "\r\n"
#define SEP_Unit      "\x1F"
#define SEP_Record    "\x1E"

/*
** An object used to read a CSV and other files for import.
*/
typedef struct ImportCtx ImportCtx;
struct ImportCtx {
  const char *zFile;  /* Name of the input file */
  FILE *in;           /* Read the CSV text from this input stream */
  char *z;            /* Accumulated text for a field */
  int n;              /* Number of bytes in z */
  int nAlloc;         /* Space allocated for z[] */
  int nLine;          /* Current line number */
  int cTerm;          /* Character that terminated the most recent field */
  int cColSep;        /* The column separator character.  (Usually ",") */
  int cRowSep;        /* The row separator character.  (Usually "\n") */
};

/* Append a single byte to z[] */
static void import_append_char(ImportCtx *p, int c){
  if( p->n+1>=p->nAlloc ){
    p->nAlloc += p->nAlloc + 100;
    p->z = reinterpret_cast<char*>(sqlite3_realloc64(p->z, p->nAlloc));
    if( p->z==0 ){
      raw_printf(stderr, "out of memory\n");
      exit(1);
    }
  }
  p->z[p->n++] = (char)c;
}

/* Read a single field of CSV text.  Compatible with rfc4180 and extended
** with the option of having a separator other than ",".
**
**   +  Input comes from p->in.
**   +  Store results in p->z of length p->n.  Space to hold p->z comes
**      from sqlite3_malloc64().
**   +  Use p->cSep as the column separator.  The default is ",".
**   +  Use p->rSep as the row separator.  The default is "\n".
**   +  Keep track of the line number in p->nLine.
**   +  Store the character that terminates the field in p->cTerm.  Store
**      EOF on end-of-file.
**   +  Report syntax errors on stderr
*/
static char *csv_read_one_field(ImportCtx *p){
  int c;
  int cSep = p->cColSep;
  int rSep = p->cRowSep;
  p->n = 0;
  c = fgetc(p->in);
  if( c==EOF || seenInterrupt ){
    p->cTerm = EOF;
    return 0;
  }
  if( c=='"' ){
    int pc, ppc;
    int startLine = p->nLine;
    int cQuote = c;
    pc = ppc = 0;
    while( 1 ){
      c = fgetc(p->in);
      if( c==rSep ) p->nLine++;
      if( c==cQuote ){
        if( pc==cQuote ){
          pc = 0;
          continue;
        }
      }
      if( (c==cSep && pc==cQuote)
       || (c==rSep && pc==cQuote)
       || (c==rSep && pc=='\r' && ppc==cQuote)
       || (c==EOF && pc==cQuote)
      ){
        do{ p->n--; }while( p->z[p->n]!=cQuote );
        p->cTerm = c;
        break;
      }
      if( pc==cQuote && c!='\r' ){
        utf8_printf(stderr, "%s:%d: unescaped %c character\n",
                p->zFile, p->nLine, cQuote);
      }
      if( c==EOF ){
        utf8_printf(stderr, "%s:%d: unterminated %c-quoted field\n",
                p->zFile, startLine, cQuote);
        p->cTerm = c;
        break;
      }
      import_append_char(p, c);
      ppc = pc;
      pc = c;
    }
  }else{
    while( c!=EOF && c!=cSep && c!=rSep ){
      import_append_char(p, c);
      c = fgetc(p->in);
    }
    if( c==rSep ){
      p->nLine++;
      if( p->n>0 && p->z[p->n-1]=='\r' ) p->n--;
    }
    p->cTerm = c;
  }
  if( p->z ) p->z[p->n] = 0;
  return p->z;
}

std::regex intRE("^(\\+|-)?\\$?[[:digit:],]+$");
std::regex realRE("(^\\+|-)?\\$?[[:digit:],]*\\.?[[:digit:]]+([eE][-+]?[[:digit:]]+)?$");
/**
 * Given the current guess for a column type and cell value string cs
 * make a conservative guess at column type.
 * We use the order none <: int <: real <: text, and a guess will only become more general.
 * TODO: support various date formats
 */
ColType guess_column_type(ColType cg, const char *s) {
  if (cg == CT_TEXT) {
    return cg;
  }
  if ((s==NULL) || strlen30(s)==0) {
    return cg;
  }
  if ((cg == CT_NONE) || (cg == CT_INT)) {
    if (regex_match(s, intRE)) {
      return CT_INT;
    }
  }
  if ((cg == CT_NONE) || (cg == CT_INT) || (cg == CT_REAL)) {
    if (regex_match(s, realRE)) {
      return CT_REAL;
    }
  }
  return CT_TEXT;
}

/*
 * perform initial scan of file using RegEx's to determine column types.
 *
 * Assumes header row has already been read
 *
 * Optimistically only looks at the first METASCAN_ROWS rows to determine
 * column types
 */
int metascan(std::vector<ColType> &colTypes, ImportCtx &ctx, int nCol) {
  int i;
  for (int row = 0; row < METASCAN_ROWS; row++) {
    int startLine = ctx.nLine;
    for (i = 0; i < nCol; i++) {
      char *z = csv_read_one_field(&ctx);
      if (colTypes[i] != CT_TEXT) {
        colTypes[i]=guess_column_type(colTypes[i], z);
      }
      /*
      ** Did we reach end-of-file before finding any columns?
      ** If so, stop instead of NULL filling the remaining columns.
      */
      if( z==0 && i==0 ) break;
      if( i<nCol-1 && ctx.cTerm!=ctx.cColSep ){
        utf8_printf(stderr, "%s:%d: metascan: expected %d columns but found %d\n",
                        ctx.zFile, startLine, nCol, i+1);
      }
    }
    // keep reading until we hit a line separator (or EOF)
    if( ctx.cTerm==ctx.cColSep ){
      do {
        csv_read_one_field(&ctx);
      } while( ctx.cTerm==ctx.cColSep );
      utf8_printf(stderr, "%s:%d: metascan: expected %d columns but found %d - "
                      "extras ignored\n",
                      ctx.zFile, startLine, nCol, i);
    }
    if (ctx.cTerm==EOF)
      break;
  }
  return 0;
}

ImportResult *sqlite_import(
  sqlite3 *db,
  const char *zFile,      // CSV file to import
  const char *zTable,     // sqlite destination table name
  ImportOptions &options  // import options
) {
  struct ShellState ss;
  struct ShellState *p = &ss;  /* TODO: replace */
  int rc = 0;
  sqlite3_stmt *pStmt = NULL; /* A statement */
  int nByte;                  /* Number of bytes in an SQL string */
  int i, j;                   /* Loop counters */
  int needCommit;             /* True to COMMIT or ROLLBACK at end */
  int nSep;                   /* Number of bytes in p->colSeparator[] */
  char *zSql;                 /* An SQL statement */
  ImportCtx sCtx;             /* Reader context */

  p->mode = MODE_Csv;
  sqlite3_snprintf(sizeof(p->colSeparator), p->colSeparator, SEP_Comma);
  sqlite3_snprintf(sizeof(p->rowSeparator), p->rowSeparator, SEP_CrLf);
  seenInterrupt = 0;
  memset(&sCtx, 0, sizeof(sCtx));
  nSep = strlen30(p->colSeparator);
  if( nSep==0 ){
    raw_printf(stderr,
               "Error: non-null column separator required for import\n");
    return NULL;
  }
  if( nSep>1 ){
    raw_printf(stderr, "Error: multi-character column separators not allowed"
                    " for import\n");
    return NULL;
  }
  nSep = strlen30(p->rowSeparator);
  if( nSep==0 ){
    raw_printf(stderr, "Error: non-null row separator required for import\n");
    return NULL;
  }
  if( nSep==2 && p->mode==MODE_Csv && strcmp(p->rowSeparator, SEP_CrLf)==0 ){
    /* When importing CSV (only), if the row separator is set to the
    ** default output row separator, change it to the default input
    ** row separator.  This avoids having to maintain different input
    ** and output row separators. */
    sqlite3_snprintf(sizeof(p->rowSeparator), p->rowSeparator, SEP_Row);
    nSep = strlen30(p->rowSeparator);
  }
  if( nSep>1 ){
    raw_printf(stderr, "Error: multi-character row separators not allowed"
                    " for import\n");
    return NULL;
  }
  sCtx.zFile = zFile;
  sCtx.nLine = 1;
  sCtx.in = fopen(sCtx.zFile, "rb");
  if( sCtx.in==0 ){
    utf8_printf(stderr, "Error: cannot open \"%s\"\n", zFile);
    return NULL;
  }
  sCtx.cColSep = p->colSeparator[0];
  sCtx.cRowSep = p->rowSeparator[0];
  zSql = sqlite3_mprintf("SELECT * FROM %s", zTable);
  if( zSql==0 ){
    raw_printf(stderr, "Error: out of memory\n");
    fclose(sCtx.in);
    return NULL;
  }

  nByte = strlen30(zSql);
  std::vector<std::string> colNames;
  rc = sqlite3_prepare_v2(db, zSql, -1, &pStmt, 0);
  if (!(rc && sqlite3_strglob("no such table: *", sqlite3_errmsg(db))==0)) {
    utf8_printf(stderr,"Error: %d: %s\n", rc, sqlite3_errmsg(db));
    sqlite3_free(zSql);
    fclose(sCtx.in);
    return NULL;
  }
  sqlite3_free(zSql);

  import_append_char(&sCtx, 0);    /* To ensure sCtx.z is allocated */
  while (csv_read_one_field(&sCtx)) {
    colNames.push_back(std::string(sCtx.z));
    if( sCtx.cTerm!=sCtx.cColSep ) break;
  }
  int nCol = colNames.size();
  if (nCol==0) {
    sqlite3_free(sCtx.z);
    fclose(sCtx.in);
    utf8_printf(stderr,"%s: empty file\n", sCtx.zFile);
    return NULL;
  }
  if (options.columnIds.size() > 0) {
    // column ids provided via options -- let's use those
    // TODO: validate that columnNames.size() == columnIds.size()
    colNames = options.columnIds;
  }

  int content_offset = ftell(sCtx.in);
  std::vector<ColType> colTypes(nCol, CT_NONE);
  if (metascan(colTypes,sCtx,nCol)!=0) {
    raw_printf(stderr, "error performing metascan\n");
    fclose(sCtx.in);
    return NULL;
  }
  std::vector<std::string> colTypeNames;
  printf("metascan complete -- column types: ");
  for (std::vector<ColType>::const_iterator it = colTypes.begin();
    it != colTypes.end();
    ++it) {
    std::string typeName(colTypeName(*it));
    printf("%s ", typeName.c_str());
    colTypeNames.push_back(typeName);
  }
  printf("\n");

  std::stringstream ssCreate;
  ssCreate << "CREATE TABLE " << zTable;
  char cSep = '(';
  std::vector<std::string>::const_iterator nmit = colNames.begin();
  std::vector<ColType>::const_iterator tyit = colTypes.begin();
  for ( ; nmit != colNames.end() && tyit != colTypes.end(); ++nmit, ++tyit) {
    ssCreate << cSep << "\n \"" << *nmit << "\" " << colTypeName(*tyit);
    cSep = ',';
  }
  ssCreate << "\n)";
  raw_printf(stderr, "%s\n", ssCreate.str().c_str());
  rc = sqlite3_exec(db, ssCreate.str().c_str(), 0, 0, 0);
  if( rc ){
    utf8_printf(stderr, "CREATE TABLE %s(...) failed: %s\n", zTable,
            sqlite3_errmsg(db));
    sqlite3_free(sCtx.z);
    fclose(sCtx.in);
    return NULL;
  }

  // rewind to content_offset:
  if (fseek(sCtx.in, content_offset, SEEK_SET)!=0) {
    raw_printf(stderr,"error rewinding file\n");
    fclose(sCtx.in);
    return NULL;
  }

  zSql = reinterpret_cast<char*>(sqlite3_malloc64( nByte*2 + 20 + nCol*2 ));
  if( zSql==0 ){
    raw_printf(stderr, "Error: out of memory\n");
    fclose(sCtx.in);
    return NULL;
  }
  sqlite3_snprintf(nByte+20, zSql, "INSERT INTO \"%w\" VALUES(?", zTable);
  j = strlen30(zSql);
  for(i=1; i<nCol; i++){
    zSql[j++] = ',';
    zSql[j++] = '?';
  }
  zSql[j++] = ')';
  zSql[j] = 0;
  rc = sqlite3_prepare_v2(db, zSql, -1, &pStmt, 0);
  sqlite3_free(zSql);
  if( rc ){
    utf8_printf(stderr, "Error: %s\n", sqlite3_errmsg(db));
    if (pStmt) sqlite3_finalize(pStmt);
    fclose(sCtx.in);
    return NULL;
  }
  needCommit = sqlite3_get_autocommit(db);
  if( needCommit ) sqlite3_exec(db, "BEGIN", 0, 0, 0);
  do{
    int startLine = sCtx.nLine;
    for(i=0; i<nCol; i++){
      char *z = csv_read_one_field(&sCtx);
      /*
      ** Did we reach end-of-file before finding any columns?
      ** If so, stop instead of NULL filling the remaining columns.
      */
      if( z==0 && i==0 ) break;
      sqlite3_bind_text(pStmt, i+1, z, -1, SQLITE_TRANSIENT);
      if( i<nCol-1 && sCtx.cTerm!=sCtx.cColSep ){
        utf8_printf(stderr, "%s:%d: expected %d columns but found %d - "
                        "filling the rest with NULL\n",
                        sCtx.zFile, startLine, nCol, i+1);
        i += 2;
        while( i<=nCol ){ sqlite3_bind_null(pStmt, i); i++; }
      }
    }
    if( sCtx.cTerm==sCtx.cColSep ){
      do{
        csv_read_one_field(&sCtx);
        i++;
      }while( sCtx.cTerm==sCtx.cColSep );
      utf8_printf(stderr, "%s:%d: expected %d columns but found %d - "
                      "extras ignored\n",
                      sCtx.zFile, startLine, nCol, i);
    }
    if( i>=nCol ){
      sqlite3_step(pStmt);
      rc = sqlite3_reset(pStmt);
      if( rc!=SQLITE_OK ){
        utf8_printf(stderr, "%s:%d: INSERT failed: %s\n", sCtx.zFile,
                    startLine, sqlite3_errmsg(db));
      }
    }
  }while( sCtx.cTerm!=EOF );

  fclose(sCtx.in);
  sqlite3_free(sCtx.z);
  sqlite3_finalize(pStmt);
  if( needCommit ) sqlite3_exec(db, "COMMIT", 0, 0, 0);

  ImportResult *ires = new ImportResult(zTable, colNames, colTypeNames);

  return ires;
}
