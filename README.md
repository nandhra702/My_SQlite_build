HOW A DATABASE WORKS
The full thing
Sukhraj Singh Nandhra


 Introduction and Setting up the REPL : read execute print loop
-	Every SQL program I have seen works in the form of a loop. A prompt is printed, a value is taken in, its processed, stored and then the prompt is printed again. So, we start off by creating an infinite loop, that keeps on running till we exit. So, we create structs, that are named as input buffers. The logic of the loop is that inside a while loop, we print the prompt, read the input into the buffer. If the input is a string as same as .exit(), then we will exit the while loop, or else we continue with further jobs.
-	So initially we create an input buffer. We create a struct and store the properties of that buffer. What to read, where to read and all. So, inside the loop, we first initialize a buffer struct, assign it zeros and NULL addresses, and then we read the information from std. input stream into the buffer. We read the line using the standard getline () function that automatically allocates memory on need. This function returns the address of where the input is stored and what’s its size and that’s what we utilize in the getline () function.

World's Simplest SQL Compiler and intro to Virtual Machine
-	Now, the SQLITE frontend, consists of 3 main things. These are the interface, which we have just seen. Then comes the compiler. This compiler, generates bytecode, that’s sent to a virtual machine for further processing. The compiler has 3 parts: Tokenizer, Parser and code generator. In this section, we are gonna create this compiler.
-	In SQLite, there are some commands, that have ‘.’ At the very start, for example “. exit”, “. tables” etc. Such commands, that are unique only to SQLite are called meta-commands. So, we have a separate check for them so that we handle them separately. So we create another struct by the name of statement that stores the type of statement, which inturn is defined by an enum which has statement types defined. For example select statement, create statement etc.
-	So, we start off. Firstly,  we check If the function is a Meta command, and if it is, is it a recogonized meta command. Based on that we use return values from the meta_command enum which defines 2 states, success or command not found. Then if its not a meta command, we call the prepare_statement function. This checks If the first word of the commands are insert or select etc. Here we set the statement type and return is given by special enum. After preparing the statement, we execute it using switch statements.
An In-Memory, Append-Only, Single-Table Database
-	Okay, so we start with a simple database, where we store userID, name and email. And this table will reside in memory only. So firstly, we will have to parse our statement in the prepare statement function. We define another struct to hold the information we are passing in. Now, in databases, tables are not stored as tables only. They are stored in the form of a data structure that makes insertion, deletion and lookups faster. SQLite uses B-trees.
-	Instead of just dumping rows in a list, you're organizing memory like this: Memory is split into pages (just blocks of memory). Each page holds multiple rows (if they can fit). Pages are created only when needed. Pages are kept in a fixed-size array — like a bookshelf with a fixed number of slots, each holding a memory page.
-	So what we do is, we squish the input into a chunk of memory and specify offsets for incoming data, so that its easier to enter data. For example, location of email is 4+32=36th byte onwards. This makes inserting and reading rows very fast.
-	Fine, we next build a container that’ll hold our rows together (when we create a table struct) and create the page sizes, maximum number of pages, and compute the number of rows a page will store etc. So when we insert a new row :
num_rows to find the correct page number and row offset within the page:

uint32_t row_num = table->num_rows;
uint32_t page_num = row_num / ROWS_PER_PAGE;
uint32_t row_offset = row_num % ROWS_PER_PAGE;
Now : 
If pages[page_num] is NULL, you allocate memory:
table->pages[page_num] = malloc(PAGE_SIZE);

We calculate where exactly should the row go inside that table
void* page = table->pages[page_num];
uint32_t byte_offset = row_offset * ROW_SIZE;

 
