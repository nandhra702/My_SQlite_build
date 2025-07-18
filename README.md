HOW A DATABASE WORKS





-	Every SQL program I have seen works in the form of a loop. A prompt is printed, a value is taken in, its processed, stored and then the prompt is printed again. So, we start off by creating an infinite loop, that keeps on running till we exit. So, we create structs, that are named as input buffers. The logic of the loop is that inside a while loop, we print the prompt, read the input into the buffer. If the input is a string as same as .exit(), then we will exit the while loop, or else we continue with further jobs.
-	So initially we create an input buffer. We create a struct and store the properties of that buffer. What to read, where to read and all. So, inside the loop, we first initialize a buffer struct, assign it zeros and NULL addresses, and then we read the information from std. input stream into the buffer. We read the line using the standard getline () function that automatically allocates memory on need. This function returns the address of where the input is stored and what’s its size and that’s what we utilize in the getline () function.
-	Now, the SQLITE frontend, consists of 3 main things. These are the interface, which we have just seen. Then comes the compiler. This compiler, generates bytecode, that’s sent to a virtual machine for further processing. The compiler has 3 parts: Tokenizer, Parser and code generator. In this section, we are gonna create this compiler.
-	In SQLite, there are some commands, that have ‘.’ At the very start, for example “.exit”, “.tables” etc. Such commands, that are unique only to SQlite are called meta-commands. So, we have a separate check for them so that we handle them separately.
