#include <stdbool.h>   
#include <stdio.h>     
#include <stdlib.h>    
#include <unistd.h>    
#include <string.h>    
#include <stdint.h>
#include "structs.h"


// This function acts like a constructor for InputBuffer.
// It allocates memory and initializes all fields.
InputBuffer* new_input_buffer() {
  InputBuffer* input_buffer = malloc(sizeof(InputBuffer));
  input_buffer->buffer = NULL;          // Will be dynamically allocated by getline()
  input_buffer->buffer_length = 0;      
  input_buffer->input_length = 0;       

  return input_buffer;
}

//THIS FUNCTION HELPS US NAVIGATE TO THE EXACT SPOT WHERE WE HAVE TO INSERT THE ROW.

void* row_slot(Table* table, uint32_t row_num) {
  uint32_t page_num = row_num / ROWS_PER_PAGE;   // Find which drawer
  void* page = table->pages[page_num];           // Get pointer to that drawer
  if (page == NULL) {
    // If drawer is empty, create it
    page = table->pages[page_num] = malloc(PAGE_SIZE);
  }
  uint32_t row_offset = row_num % ROWS_PER_PAGE; // How far into the drawer
  uint32_t byte_offset = row_offset * ROW_SIZE;  // Convert row index to byte position
  return page + byte_offset;                     // Return pointer to that spot
}


void print_row(Row* row) {
  printf("(%d, %s, %s)\n", row->id,     row->username, row->email);
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


// Creates a new Table structure in memory and initializes it
Table* new_table() {
  // Allocate memory for the Table struct itself
  Table* table = (Table*)malloc(sizeof(Table));

  // Initialize the number of rows to 0 (empty table)
  table->num_rows = 0;

  // Initialize all page pointers to NULL (no pages allocated yet)
  for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++) {
     table->pages[i] = NULL;
  }

  // Return the newly created table
  return table;
}


// Frees all memory used by the table, including each page
void free_table(Table* table) {
  // Loop through all pages and free each one if it's been allocated
  for (int i = 0; table->pages[i]; i++) {
    free(table->pages[i]);  // Free the memory for this page
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

MetaCommandResult do_meta_command(InputBuffer* input_buffer) {
  if (strcmp(input_buffer->buffer, ".exit") == 0) {
    close_input_buffer(input_buffer);
    exit(EXIT_SUCCESS);
  } else {
    return META_COMMAND_UNRECOGNIZED_COMMAND;
  }
}

PrepareResult prepare_statement(InputBuffer* input_buffer,Statement* statement) {

  if (strncmp(input_buffer->buffer, "insert", 6) == 0) {
    statement->type = STATEMENT_INSERT;
    int args_assigned = sscanf(
        input_buffer->buffer, "insert %d %s %s", &(statement->row_to_insert.id),
        statement->row_to_insert.username, statement->row_to_insert.email);
    if (args_assigned < 3) {
      return PREPARE_SYNTAX_ERROR;
    }
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
  Table* table = new_table();

  // Create and initialize a new InputBuffer
  InputBuffer* input_buffer = new_input_buffer();

  // Infinite loop for continuous user interaction
  while (true) {
    print_prompt();             // Show prompt
    read_input(input_buffer);   // Read user input into buffer

  

   if (input_buffer->buffer[0] == '.') {        //means that this is a meta command thats special to SQLite
      switch (do_meta_command(input_buffer)) {
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
    printf("Executed.\n");
 }
}
