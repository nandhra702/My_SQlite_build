#include <stdbool.h>   
#include <stdio.h>     
#include <stdlib.h>    
#include <unistd.h>    
#include <string.h>    




// This structure holds everything we need to read user input
typedef struct {
  char* buffer; // - buffer: pointer to the actual text line entered by the user
  size_t buffer_length;  // - buffer_length: size of memory allocated for buffer
  ssize_t input_length;  // - input_length: actual number of characters entered (excluding newline)
} InputBuffer;


typedef enum {
  META_COMMAND_SUCCESS,
  META_COMMAND_UNRECOGNIZED_COMMAND
} MetaCommandResult;

typedef enum { PREPARE_SUCCESS, PREPARE_UNRECOGNIZED_STATEMENT } PrepareResult;

typedef enum { STATEMENT_INSERT, STATEMENT_SELECT } StatementType;

typedef struct {
  StatementType type;
} Statement;



// This function acts like a constructor for InputBuffer.
// It allocates memory and initializes all fields.
InputBuffer* new_input_buffer() {
  InputBuffer* input_buffer = malloc(sizeof(InputBuffer));
  input_buffer->buffer = NULL;          // Will be dynamically allocated by getline()
  input_buffer->buffer_length = 0;      
  input_buffer->input_length = 0;       

  return input_buffer;
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

PrepareResult prepare_statement(InputBuffer* input_buffer,
                                Statement* statement) {
  if (strncmp(input_buffer->buffer, "insert", 6) == 0) {
    statement->type = STATEMENT_INSERT;
    return PREPARE_SUCCESS;
  }
  if (strcmp(input_buffer->buffer, "select") == 0) {
    statement->type = STATEMENT_SELECT;
    return PREPARE_SUCCESS;
  }

  return PREPARE_UNRECOGNIZED_STATEMENT;
}

void execute_statement(Statement* statement) {
  switch (statement->type) {
    case (STATEMENT_INSERT):
      printf("This is where we would do an insert.\n");
      break;
    case (STATEMENT_SELECT):
      printf("This is where we would do a select.\n");
      break;
  }
}





                                                    //MAIN FUNCTION LOOP

int main(int argc, char* argv[]) {
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
      case (PREPARE_UNRECOGNIZED_STATEMENT):
        printf("Unrecognized keyword at start of '%s'.\n",
               input_buffer->buffer);
        continue; 
    }

    execute_statement(&statement);
    printf("Executed.\n");
 }
}
