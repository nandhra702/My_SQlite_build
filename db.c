#include <stdbool.h>   
#include <stdio.h>     
#include <stdlib.h>    
#include <unistd.h>    
#include <string.h>    
#include <stdint.h>
#include "structs.h"
#include<sys/types.h>
#include<errno.h>
#include<fcntl.h>
#include<sys/stat.h>


// This function acts like a constructor for InputBuffer.
// It allocates memory and initializes all fields.
InputBuffer* new_input_buffer() {
  InputBuffer* input_buffer = malloc(sizeof(InputBuffer));
  input_buffer->buffer = NULL;          // Will be dynamically allocated by getline()
  input_buffer->buffer_length = 0;      
  input_buffer->input_length = 0;       

  return input_buffer;
}


void print_row(Row* row) {
  printf("(%d, %s, %s)\n", row->id, row->username, row->email);
}


void serialize_row(Row* source, void* destination) {
  memcpy(destination + ID_OFFSET, &(source->id), ID_SIZE);
  memcpy(destination + USERNAME_OFFSET, &(source->username), USERNAME_SIZE);
  memcpy(destination + EMAIL_OFFSET, &(source->email), EMAIL_SIZE);
}

void deserialize_row(void *source, Row* destination) {
  memcpy(&(destination->id), source + ID_OFFSET, ID_SIZE);
  memcpy(&(destination->username), source + USERNAME_OFFSET, USERNAME_SIZE);
  memcpy(&(destination->email), source + EMAIL_OFFSET, EMAIL_SIZE);
}

 
 //opens default database and initiates page caches to NULL

Pager* pager_open(const char* filename) {
  int fd = open(filename,
                O_RDWR | O_CREAT,  // Create file if it does not exist // if it does, open in Read/Write mode
                S_IWUSR | S_IRUSR   // User read permission     // User write permission () 
                );  //(THE 2ND ARGUMENT ONLY COMES TO THE PICTURE WHEN A NEW FILE IS CREATED)

  if (fd == -1) {
    printf("Unable to open file\n");
    exit(EXIT_FAILURE);
  }

  off_t file_length = lseek(fd, 0, SEEK_END); //RETURNS LENGTH IN BYTES

  //opens up the pager, assigns it memory, and sets each page to NULL.
  Pager* pager = malloc(sizeof(Pager));
  pager->file_descriptor = fd;
  pager->file_length = file_length;

  for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++) {
    pager->pages[i] = NULL;
  }

  return pager;
}

//RETURNS THE PAGE REQUESTED BY TABLE WHILE INSERTING ROWS


// Creates a new Table structure in memory and initializes it and does the same for Pager
/*db_open() in turn calls pager_open(), which opens the database file and keeps track of its size.
 It also initializes the page cache to all NULLs.*/

Table* db_open(const char* filename) {
  Pager* pager = pager_open(filename);
  uint32_t num_rows = pager->file_length / ROW_SIZE;

  // Allocate memory for the Table struct itself
  Table* table = malloc(sizeof(Table));
  table->pager = pager;
  table->num_rows = num_rows;

   return table;
 }


void* get_page(Pager* pager, uint32_t page_num) {
  if (page_num > TABLE_MAX_PAGES) {
    printf("Tried to fetch page number out of bounds. %d > %d\n", page_num,
           TABLE_MAX_PAGES);
    exit(EXIT_FAILURE);
  }

  if (pager->pages[page_num] == NULL) {
    // Cache miss. Allocate memory and load from file.
    void* page = malloc(PAGE_SIZE);
    uint32_t num_pages = pager->file_length / PAGE_SIZE;

    // We might save a partial page at the end of the file
    if (pager->file_length % PAGE_SIZE) {
      num_pages += 1;
    }

    if (page_num <= num_pages) {
      lseek(pager->file_descriptor, page_num * PAGE_SIZE, SEEK_SET);
      ssize_t bytes_read = read(pager->file_descriptor, page, PAGE_SIZE);
      if (bytes_read == -1) {
        printf("Error reading file: %d\n", errno);
        exit(EXIT_FAILURE);
      }
    }

    pager->pages[page_num] = page;
  }

  return pager->pages[page_num]; //pager->pages is an array of pointers. One pointer per page
}

//THIS FUNCTION HELPS US NAVIGATE TO THE EXACT SPOT WHERE WE HAVE TO INSERT THE ROW.

void* row_slot(Table* table, uint32_t row_num) {
  uint32_t page_num = row_num / ROWS_PER_PAGE;   // Find which drawer
  
  void* page = get_page(table->pager, page_num);
  uint32_t row_offset = row_num % ROWS_PER_PAGE; // How far into the drawer
  uint32_t byte_offset = row_offset * ROW_SIZE;  // Convert row index to byte position
  return page + byte_offset;                     // Return pointer to that spot
}


//TO GRACEFULLY EXIT FROM THE TABLE
void db_close(Table* table) {
  Pager* pager = table->pager;
  uint32_t num_full_pages = table->num_rows / ROWS_PER_PAGE;

  for (uint32_t i = 0; i < num_full_pages; i++) {
    if (pager->pages[i] == NULL) {
      continue;
    }
    pager_flush(pager, i, PAGE_SIZE);
    free(pager->pages[i]);
    pager->pages[i] = NULL;
  }

  //NEED TO UNDERSTAND THIS FUNCTION STILL

  // There may be a partial page to write to the end of the file
  // This should not be needed after we switch to a B-tree
  uint32_t num_additional_rows = table->num_rows % ROWS_PER_PAGE;
  if (num_additional_rows > 0) {
    uint32_t page_num = num_full_pages;
    if (pager->pages[page_num] != NULL) {
      pager_flush(pager, page_num, num_additional_rows * ROW_SIZE);
      free(pager->pages[page_num]);
      pager->pages[page_num] = NULL;
    }
  }

  int result = close(pager->file_descriptor);
  if (result == -1) {
    printf("Error closing db file.\n");
    exit(EXIT_FAILURE);
  }
  for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++) {
    void* page = pager->pages[i];
    if (page) {
      free(page);
      pager->pages[i] = NULL;
    }
  }
  free(pager);
  free(table);
}

//WORLD FAMOUS PAGE FLUSHER function

void pager_flush(Pager* pager, uint32_t page_num, uint32_t size) {
  if (pager->pages[page_num] == NULL) {
    printf("Tried to flush null page\n");
    exit(EXIT_FAILURE);
  }

  off_t offset = lseek(pager->file_descriptor, page_num * PAGE_SIZE, SEEK_SET);

  if (offset == -1) {
    printf("Error seeking: %d\n", errno);
    exit(EXIT_FAILURE);
  }

  ssize_t bytes_written =
      write(pager->file_descriptor, pager->pages[page_num], size);

  if (bytes_written == -1) {
    printf("Error writing: %d\n", errno);
    exit(EXIT_FAILURE);
  }
}



// Frees all memory used by the table, including each page
void free_table(Table* table) {
  // Loop through all pages and free each one if it's been allocated
  for (int i = 0; table->pager->pages[i]; i++) {
    free(table->pager->pages[i]);  // Free the memory for this page
  }

  // Finally, free the memory for the Table struct itself
  free(table);
}


void print_prompt() { 
  printf("ghost_protocol > "); 
}

// Reads a full line of input from stdin into the input buffer.
// Uses getline() which handles memory allocation if needed.
void read_input(InputBuffer* input_buffer) {
  // getline allocates (or reallocates) buffer as needed and fills it with user input
  //we input the location where the buffer is (the ptr which it'll return) and also the size of input (which it'll return)
  ssize_t bytes_read =
      getline(&(input_buffer->buffer), &(input_buffer->buffer_length), stdin);

  if (bytes_read <= 0) {
    printf("Error reading input\n");
    exit(EXIT_FAILURE);
  }

  // Ignore the trailing newline character by replacing it with null terminator
  input_buffer->input_length = bytes_read - 1;
  input_buffer->buffer[bytes_read - 1] = 0;  // Convert '\n' to '\0' to treat it as a string
}

// Frees memory allocated for buffer and the InputBuffer struct itself.
// Prevents memory leaks when we exit the program.
void close_input_buffer(InputBuffer* input_buffer) {
  free(input_buffer->buffer);  // Free the actual string buffer
  free(input_buffer);          // Free the struct itself
}

MetaCommandResult do_meta_command(InputBuffer* input_buffer, Table* table) {
  if (strcmp(input_buffer->buffer, ".exit") == 0) {
    db_close(table);
    exit(EXIT_SUCCESS);
  } else {
    return META_COMMAND_UNRECOGNIZED_COMMAND;
  }
}

PrepareResult prepare_statement(InputBuffer* input_buffer,Statement* statement) {
  if (strncmp(input_buffer->buffer, "insert", 6) == 0) {

    statement->type = STATEMENT_INSERT;
    char* keyword = strtok(input_buffer->buffer, " ");
    char* id_string = strtok(NULL, " ");
    char* username = strtok(NULL, " ");
    char* email = strtok(NULL, " ");

 if (id_string == NULL || username == NULL || email == NULL) {
      return PREPARE_SYNTAX_ERROR;
    }


  int id = atoi(id_string);
  if (id < 0) {
     return PREPARE_NEGATIVE_ID;
  }
  if (strlen(username) > COLUMN_USERNAME_SIZE) {
     return PREPARE_STRING_TOO_LONG;
  }
  if (strlen(email) > COLUMN_EMAIL_SIZE) {
     return PREPARE_STRING_TOO_LONG;
  }

  statement->row_to_insert.id = id;
  strcpy(statement->row_to_insert.username, username);
  strcpy(statement->row_to_insert.email, email);
    return PREPARE_SUCCESS;
  }


  if (strcmp(input_buffer->buffer, "select") == 0) {
    statement->type = STATEMENT_SELECT;
    return PREPARE_SUCCESS;
  }

  return PREPARE_UNRECOGNIZED_STATEMENT;
}


//////////////////////////////////// FUNCTIONS FOR EXECUTION OF COMMANDS /////////////////////////////////////

// Executes an INSERT statement
ExecuteResult execute_insert(Statement* statement, Table* table) {
  // Check if there's space left in the table
  if (table->num_rows >= TABLE_MAX_ROWS) {
    return EXECUTE_TABLE_FULL;  // Table is full, return an error result
  }

  // Get the row data from the statement
  Row* row_to_insert = &(statement->row_to_insert);

  // Serialize the row into a flat memory format
  // and store it at the correct memory location for the next row
  serialize_row(row_to_insert, row_slot(table, table->num_rows));

  // Increment the total row count after a successful insert
  table->num_rows += 1;

  return EXECUTE_SUCCESS;  // Indicate success
}


// Executes a SELECT statement (prints all rows)
ExecuteResult execute_select(Statement* statement, Table* table) {
  Row row;  // Temporary variable to hold each row while we print it

  // Loop through all inserted rows
  for (uint32_t i = 0; i < table->num_rows; i++) {
    // Read the serialized row from memory into the `row` variable
    deserialize_row(row_slot(table, i), &row);

    // Print the contents of the row
    print_row(&row);
  }

  return EXECUTE_SUCCESS;  // Indicate success
}


// Chooses which statement to execute: INSERT or SELECT
ExecuteResult execute_statement(Statement* statement, Table* table) {
  switch (statement->type) {
    case (STATEMENT_INSERT):
      // If it's an INSERT, call the insert executor
      return execute_insert(statement, table);
      
    case (STATEMENT_SELECT):
      // If it's a SELECT, call the select executor
      return execute_select(statement, table);
  }
}







                                                    //MAIN FUNCTION LOOP

int main(int argc, char* argv[]) {

  //Firstly initialize a table : 
    if (argc < 2) {
    printf("Must supply a database filename.\n");
    exit(EXIT_FAILURE);
  }

  char* filename = argv[1];
  Table* table = db_open(filename);



  // Create and initialize a new InputBuffer
  InputBuffer* input_buffer = new_input_buffer();

  // Infinite loop for continuous user interaction
  while (true) {
    print_prompt();             // Show prompt
    read_input(input_buffer);   // Read user input into buffer

  

   if (input_buffer->buffer[0] == '.') {        //means that this is a meta command thats special to SQLite
      switch (do_meta_command(input_buffer,table)) {
        case (META_COMMAND_SUCCESS):
          continue; //to skip the execution 
        case (META_COMMAND_UNRECOGNIZED_COMMAND):
          printf("Unrecognized command '%s'\n", input_buffer->buffer);
          continue; 
      }
    }

    Statement statement;  //this is a struct to store parsed SQL statements

    // The prepare statement function parses the input SQL string and fills out the statement struct accordingly
    switch (prepare_statement(input_buffer, &statement)) {
      case (PREPARE_SUCCESS):
        break;

      case (PREPARE_SYNTAX_ERROR):
        printf("Syntax error. Could not parse statement.\n");
        continue;
      
      case (PREPARE_NEGATIVE_ID):
      	printf("ID must be positive.\n");
        continue;
      case (PREPARE_STRING_TOO_LONG):
      	printf("String is too long.\n");
      	continue;

      case (PREPARE_UNRECOGNIZED_STATEMENT):
        printf("Unrecognized keyword at start of '%s'.\n",
               input_buffer->buffer);
        continue; 
    }


    switch (execute_statement(&statement, table)) {
      case (EXECUTE_SUCCESS):
        printf("Executed.\n");
        break;
      case (EXECUTE_TABLE_FULL):
        printf("Error: Table full.\n");
        break;
    }
    //printf("Executed.\n");
 }
}
