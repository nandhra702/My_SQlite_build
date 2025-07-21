#include <stdbool.h>   
#include <stdio.h>     
#include <stdlib.h>    
#include <unistd.h>    
#include <string.h>    
#include <stdint.h>



#define COLUMN_USERNAME_SIZE 32
#define COLUMN_EMAIL_SIZE 255

typedef struct {
  uint32_t id;
  char username[COLUMN_USERNAME_SIZE];
  char email[COLUMN_EMAIL_SIZE];
} Row;

typedef enum { STATEMENT_INSERT, STATEMENT_SELECT } StatementType;

typedef enum { EXECUTE_SUCCESS, EXECUTE_TABLE_FULL } ExecuteResult;

typedef enum {PREPARE_SUCCESS, PREPARE_SYNTAX_ERROR, PREPARE_UNRECOGNIZED_STATEMENT } PrepareResult;

typedef enum {
  META_COMMAND_SUCCESS,
  META_COMMAND_UNRECOGNIZED_COMMAND
} MetaCommandResult;


typedef struct {
  StatementType type;
  Row row_to_insert; //only used by insert statement
} Statement;



#define size_of_attribute(Struct, Attribute) sizeof(((Struct*)0)->Attribute)

const uint32_t ID_SIZE = size_of_attribute(Row, id);
const uint32_t USERNAME_SIZE = size_of_attribute(Row, username);
const uint32_t EMAIL_SIZE = size_of_attribute(Row, email);
const uint32_t ID_OFFSET = 0;
const uint32_t USERNAME_OFFSET = ID_OFFSET + ID_SIZE;
const uint32_t EMAIL_OFFSET = USERNAME_OFFSET + USERNAME_SIZE;
const uint32_t ROW_SIZE = ID_SIZE + USERNAME_SIZE + EMAIL_SIZE;

const uint32_t PAGE_SIZE = 4096; // each page is 4KB
#define TABLE_MAX_PAGES 100      // max 100 pages
const uint32_t ROWS_PER_PAGE = PAGE_SIZE / ROW_SIZE; // how many rows can fit in 1 page
const uint32_t TABLE_MAX_ROWS = ROWS_PER_PAGE * TABLE_MAX_PAGES; // max total rows = rows per page * number of pages

typedef struct {
  uint32_t num_rows;               // keeps track of how many rows are currently stored
  void* pages[TABLE_MAX_PAGES];    // array of page pointers, each one is 4KB
} Table;



// This structure holds everything we need to read user input
typedef struct {
  char* buffer; // - buffer: pointer to the actual text line entered by the user
  size_t buffer_length;  // - buffer_length: size of memory allocated for buffer
  ssize_t input_length;  // - input_length: actual number of characters entered (excluding newline)
  Row row_to_insert;  // only used by insert statement
} InputBuffer;










