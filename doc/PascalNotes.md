# Pascal Notes

## History

This compiler is the result of several years of work and is basically an homage to a Language that I loved in my youth.  Pascal was invented around 1970 by Niklaus Wirth https://en.wikipedia.org/wiki/Niklaus_Wirth it became widely popular until destroyed by Brian Kernighan https://www.cs.virginia.edu/~evans/cs655/readings/bwk-on-pascal.html.  Modern Pascal has resolved all of these issues, but it is no longer a language in wide use.  It was widely used not too long ago with Borland Turbo Pascal and Delphi.  There is a modest following for contemporary FreePascal/Lazarus and also GNU Pascal.

In the late 1970's, I was in graduate school and scraped enough money together to buy a TRS-80 Model I.  That little grey keyboard computer is what caused to to leave graduate school ABD and pursue a career in programming and, eventually, embedded systems.

Although I had multiple degrees, I was still not properly credentialed for that career and returned to graduate school to work on a master's degree in Computer Science.  That was in 1981 and that was when I was first exposed to real Pascal programming.  It was a core part of the culture in the CS department.

But I had some previous exposure.  I had subscribed to the *TRS-80 Newsletter* which published *BASIC* programs for the TRS-80 that you could type in yourself and save on cassette tape.  In 1979, *TRS-80 Newsletter* published the source for People's Pascal, http://www.trs-80.org/tiny-pascal/.  You could either buy the software with documentation or painfully enter it from the listing by hand.  I did the latter and got to know a little bit about the design of the tiny Pascal compiler.

[Alcor Pascal was also available in 1978 for CP/M and later the TRS-80, but that was out of my reach at $249.95.  http://www.trs-80.org/alcor-pascal/].

In the early/mid 1980's I upgraded my Trash-80 for a TRS-80 Model IV, my pride and joy at the time. I also purchased the Alcor C compiler on a 5.25 inch floppy.  One of my early programming projects was to write a work-alike tiny Pascal compiler in C.  It was really a pretty nice compiler, but very limited.  It was basically integer Pascal with peek and poke capabilities built on the TRS-80 ROM.

Some time later, perhaps in the 1990's or early 2000's, I decided to extend the compiler to support full Pascal with language definitions circa 1980.  I see the last changes in 2004, 2007, and 2008. While the language was much more complete, the compiler was also more unstable and remained unstable until very recently.

After that I was mostly preoccupied with another project, the NuttX RTOS, and the compiler sat gathering dust.  I did incorporate the Pascal P-Code VM into the RTOS for a while so that could run the Pascal code under the OS.  But the compiler was just too unstable to be useful.
 
In 2019, I granted the NuttX RTOS to *Apache Software Foundation*.  This freed up a lot of my time since I was no longer on the critical path for the RTOS.  So I devoted time again to this compiler.

I extended the language support to include most of the features of recent and contemporary Pascal compilers (while still avoiding the object-based Pascal that is in use today by *FreePascal* and *Lazarus*).  I also developed test suites that brought made the compiler much more stable and certainly usable.

Currently, I am bringing the compiler into the embedded environment.  I would like to see an all Pascal embedded system running on a P-Code VM with a Pascal JIT compiler.  Stay tuned.  Let's see how far I get.

## General Language Information

### TYPES

**Basic Types**.  All basic types `INTEGER`, `BOOLEAN`, `CHAR`, `REAL`, `SCALAR`, `SUBRANGE`, `RECORD` and `SET` implemented along with some types from more contemporary Pascal types like `STRING` and short `STRING` types.

**Integer Types**.  The standard Pascal `INTEGER` type is a signed, 16-bit integer value.  In addition to that standard `INTEGER` type, the following commonly support integer types are also available:

- **Word Type**.  An unsigned integer type, `WORD`, is also supported.  This is equivalent to `INTEGER` in all ways except for some arithmetic operators (multiplication and division) and for comparison operators.

- **Short Integer Types**.  Both `SHORTINTEGER` (signed) and `SHORTWORD` (unsigned) 8-bit integer types are supported.  The alias `SHORTINT` is also accepted.

- **Long Integer Types**.  Both `LONGINTEGER` (signed) and  `LONGWORD` (unsigned) 32-bit integer types are supported.  The alias `LONGINT` is also accepted.

**Explicit Type Cast**.  Explicit type casts are supported.  The form of the type cast is:  `type-name ‘(‘ expression ‘)’`.  Casting is limited to `REAL`/ordinal and ordinal/ordinal types and to pointer/pointer types.

### File Types

**Supported File Types**.  `FILE OF` and `TEXTFILE` (or `TEXT`)  types are supported.  The former are treated as binary files with fixed record lengths and the later as line-oriented files.

**Text File**.  `TEXTFILE` is not the same as a PACKED FILE OF CHAR.  The latter is is treated as a binary file but has some string-like behaviors for limited backward compatibility.  The traditional name `TEXT` also supported as an alias for `TEXTFILE`.

**Standard Files**.  `INPUT` and `OUTPUT` are text files.

**Text and Binary Files**.  Files of type `TEXTFILE` (`TEXT`) are text files.  All others of type `FILE OF <type>` are binary files.

**Limitations**.  No functions returning `FILE OF`.

### Variables

**Variable Initializers**.  Variable initializers are supported for most simple type. The compiler supports initializers of the form:

    VAR
      name : type = initial-value

### Sets

`SET` is limited to 64-bits adjacent elements. That is more that the traditional 59-bit limit but less than some contemporary Pascal compilers.  *FreePascal*, for example, supports small sets of up to 32 elements and large sets up to 256 elements.  *FreePascal* selects the set size, large or small, at compile time based on the declared size of the set.

### Records

- In `RECORD CASE`, there is no verification that the type of the tag is consistent with a case selector; the check is only if the case selector constants are of the same type as the tag type identifier.
Pointer Limitations.
- Can't pass pointers as `VAR` parameters
- No pointers to functions
- Supports pointers and pointers-to-pointers, but not pointers-to-pointers-to-pointers.

### Arrays

No `PACKED` types.  This is a bug and needs to be revisited in the future.

- No range checks on array indices

### Statements

- `WITH` statements cannot be nested.
- Cannot reference "up the chain" in `WITH` statements.  Eg. suppose `RECORD` "a" contains `RECORD` "b" which contains "c" and that `RECORD` "a" also contains "d" so that we could write a.b.c OR a.d.  Then the following should work but is not yet supported:

    WITH a DO
    BEGIN
       WITH b DO
       BEGIN
         c := ..
         d := ..  **SHOULD WORK!!!**

- `GOTO` only partially implemented -- no stack corrections for GOTOs between loops, blocks, etc.  Use is *DANGEROUS*.
- Expressions are not strongly typed across different `SCALAR` types (see exprEnum in expr.h)

## Standard Procedures and Functions

**Caveat**. Implements some functions/procedures that may not be standard, but are in common usage.
**Limitation**.  No `PROCEDURE` call with `PROCEDURE`s or `FUNCTION`s as parameters

### System Procedures/Functions:

- `procedure Exit(exitCode : integer)`
- `procedure Halt`
- `procedure Break` - `Break` jumps to the statement following the end of the current loop (`FOR`, `WHILE`, or `REPEAT` loop), exiting the loop without executing the code between the `BREAK` statement and the end of the loop.
- `procedure Continue` - `Continue` jumps to the statement at the end of the current loop (`FOR`, `WHILE`, or `REPEAT` loop), skipping to the next cycle through the loop without executing the code between the `CONTINUE` statement and the end of the loop.
- `procedure New(<pointer-variable>)`
- `procedure Dispose(<pointer-variable>)`
- `function GetEnv(name: string) : string`
- `function Spawn(PexFileName : string; StringBufferAlloc, HeapAlloc : integer; Wait, Debug : boolean) : boolean`

- `Addr(<any-variable>`.  Returns the address of the variable.  Similar to `@` except that it is not typed.
- `Card`
- `Chr`
- `Sizeof`
- `Pack` - The pack procedure assigns values to all the elements of a packed array by copying some or all of the elements of an unpacked array into the packed array. The elements of both arrays must have the same type. *Not Implemented*
- `Unpack` -  The unpack procedure assigns values to some or all of the elements of an unpacked array by copying all of the elements of a packed array into the unpacked array. The elements of both arrays must have the same type. *Not Implemented*

- `Abs`
- `Arctan`
- `Cos`
- `Exp`
- `Ln`
- `Round`
- `Sin`
- `Sqr`
- `Sqrt`
- `Succ`
- `Trunc`

- `Odd`
- `Ord`
- `Pred`

- `Exclude`
- `Include`

#### String Operations

Borland style string operators:

- `function Length( s : string)` – Return the length of a string.
- `function Copy(s : string, from, howmuch: integer) : string` - Get a substring from a string.
- `function Pos(substr, s : string) : integer` - Get the position of substring `substr` within stringi `s`. If the substring is not found, `Pos` will return 0.
- `procedure Str(numvar : integer, VAR strvar : string)` – Converts a numeric value into a string.  `numvar` may include a fieldwidth and, for the case of real values, a precision.
- `function Concat(s1, s2,... ,sn : string) : string` – Concatenate one or more strings.
- `procedure Insert(source : string, VAR target : string; index : integer)` - Insert a string inside another string from at the indexth character.
- `procedure Delete(VAR s : string; i, n: integer)` - Deletes `n` characters from string `s` starting from index `i`.
- `procedure fillchar(VAR s : string; count : integer; value : shortword)` -  Fill string s with character value until `s` is `count`-1 characters long
- `procedure Val(inString : string; VAR numvar : integer; VAR code : integer)` – Convert a string to a numeric value.  `strvar` is a string variable to be converted, numvar is any numeric variable either `Integer`, `Longinteger`, `ShortInteger`, or `Real`, and if the conversion isn't successful, then the parameter `code` contains the index of the character in `S` which prevented the conversion.

In addition to these built-in, *intrinsic* string operations.  Additional string support is provided throught the unit `StringUtils.pas`.  This additional support includes:

- `function UpperCase(ch : char) : char` - Convert the input `ch` character from lower- to upper-case.  if `ch` is not a lower case character, it is returned unaltered.
- `function LowerCase(ch : char) : char` - Convert the input `ch` character from upper- to lower-case.  if `ch` is not an upper lower case character, it is returned unaltered.
- `function CharPos(StrInput : STRING; CheckChar : char; StartPos : integer) : integer` - Return the position of the next occurence of `CheckChar` in `StrInput` after position `CheckPos`.  Position zero is returned if there is character meeting these conditions
- `function TokenizeString(inString, tokenDelimiters : string; VAR token : string; VAR StrPos : integer) : boolean` - Gets the next *token* from the string `inString` on each call.  A *token* is defined to be a substring of `inString` delimited by one of the characters from the string `tokenDelimiters`.  The integer `VAR` parameter `StrPos` must be provided so that `TokenizeString` can continue parsing the string `inString` from call-to-call.  `StrPos` simply holds the position in the string `inString` where `TokenizeString` will resume parsing on the next call.    This function returns true if the next token pas found or false if all tokens have been parsed.

Non-standard string operations.  These are used internally in the implementation of other string procedures/functions but are available to Pascal applications as well.  Use of non-standard prodecures/functions can make if more difficult to port the Pascal code to other platforms, however.

- `function CharAt(inString : string; charPos : integer) : char` - Returns the character at position `charPos` in `inString`.  NUL is returned if the character position is invalid.  The only precedent that I am aware of is Delphi that uses an array like syntax to peek at specific characters.  The Delphi syntax would be `inString[charPos]`.  The name `CharAt` derives from a similar JavaScript method.

#### File I/O
- `Append` - Opens an existing file for appending data to end of file
- `AssignFile` - Assign a name to a file (`Assign` is an alias)
- `CloseFile` - Close opened file (`Close` is an alias)
- `EOF` - Check for end of file
- `EOLN` – Check for end of line
- `function FilePos(f : TEXTFILE) : Int64` - Get position in file.  Argument may also be a binary file.
- `function FileSize(f : TEXTFILE) : Int64` - Get size of file.  Argument may also be a binary file.
- `procedure Flush(f : TEXTFILE)` - Write file buffers to disk.  Argument may also be a binary file.
- `Read` - Read from a text file
- `ReadLn` - Read from a text file and go to the next line
- `Reset` - Opens a file for reading
- `Rewrite` - Create a file for writing
- `Seek` - Change position in file
- `SeekEOF` - Set file position to end of file
- `SeekEOLn` - Set file position to end of line
- `Truncate` - Truncate the file at position
- `Write` - Write variable to a file
- `WriteLn` - Write variable to a text file and go to a new line

The following are not currently implemented:

- `BlockRead` - Read data from an untyped file into memory
- `BlockWrite` - Write data from memory to an untyped file
- `Erase` - Erase file from disk
- `Get(f : file-type)` - The get procedure reads the next component from a file into the file's buffer variable. NOTE: This procedure is seldom used since it is usually easier to use the read or readln procedures.IOResult - Return result of last file IO operation
- `Put(f : file-type)` – The put procedure writes the contents of a file's buffer variable to the file and empties the file's buffer variable leaving it undefined.
- `Rename` – Rename a filename
- `SetTextBuf`  - Sets size of file buffer

## Directory Operations

Variants of the basic directory operations, similar to those from Borland Turbo Pascal or Free Pascal are provided as intrinsic functions.

- `function GetDir : string` - Returns the current working directory.
- `function SetCurrentDir(DirPath : string) : boolean` - Set the current working directory.  Returns `true` if successful.
- `procedure ChDir(DirPath : string)` - Set the current working directory.  No failure indication is returned.
- `function CreateDir(DirPath : string) : boolean` - Create a new directory.  Returns `true` if the directory was create successfully.
- `procedure MkDir(DirPath : string)` - Create a new directory.  No failure indication is returned.
- `function RemoveDir(DirPath : string) : boolean` - Remove a new directory.  .  Returns `true` if the directory was successfully removed.
- `procedure RmDir(DirPath : string)` - Remove a new directory.  No failure indication is returned.

Other directory operations inspired by Free Pascal are provided in the unit `FileUtils.pas`.  Some primitive directory intrinics are provided in the run-time code to support this unit.  Those intrinsics are very close to the underlying functions from the standard C library, on which the Pascal run-time is built.  As a rule, these should not be used by Pascal application code for portability reasons.  These intrinsics include:

- `function OpenDir(DirPath : string; VAR dir: TDir) : boolean` - Open a directory for reading.  If the directory was successfully opened, *true* is returned and TDir is valid for use with `ReadDir`.
- `function ReadDir(VAR dir : TDir, VAR Result : TSearchRec) : boolean` - Read the next directory entry.  *true* is returned if the directory entry was read successfully and the content of the `Result` will be valid.  Only the `name` and `attr` field of the `TSearchRec` record are populated.  The most likely reason for a failure to read the directory entry is that the final directory entry has already been provided.
- `function FileInfo(FilePath : string; VAR Result : TSearchRec) : boolean` - This function will populate the remaining fields of the `TSearchRec` record, `size` and `time`.
- `function RewindDir(VAR dir : TDir) : boolean` - Reset the read position of the beginning of the directory.  *True* is returned if the directory read was successfully reset.
- `function CloseDir(VAR dir : TDir) : boolean` - Close the directory and release any resources.  *True* is returned if the directory was successfully closed.

Where the definition of `TDir` is implementation specific and should *not* have be accessed by application code.  And `TSearchRec` is a `record` type with several fields, some are internal to the implemenation but the following are available to your program:

- `name` - Name of the file found.
- `size` - The size of the file in bytes.
- `attr` - The file attribute character (as above).
- `time` - The time and data of the last modification.

The `attr` is a integer constant the identifies the type(s) of the file:

- `faAnyFile` - Find any file (this is a combination of the other flags).
- `faReadOnly` - The file is read-only.
- `faHidden` - The file is hidden. (Under a POSIX file system, this means that the filename starts with a dot).
- `faSysFile` - The file is a system file (Under a POSIX file system, this means that the file is a character, block or FIFO file).
- `faVolumeId` - Drive volume Label (Windows with FAT, not Fat32 of VFAT, file systems only).
- `faDirectory` - File is a directory.
- `faArchive` - The file needs to be archived (Windows only)

Using these intrinic functions, `FileUtils.pas` can then provide:

- `function  ExtractFilePath(FullPath : string ) : string` - `ExtractFilePath` returns the path part (including drive letter, if applicable) from `FullPath`.  The returned path consists of all characters before the last directory separator character ('/' or '\'), including the directory separator itself. In case there is only a drive letter, that will be returned.
- `function  ExtractFileName(FullPath : string ) : string` - `ExtractFileName` returns the filename part from `FullPath`.  The returned filename consists of all characters after the last directory separator character ('/' or '\') or drive letter.
- `function FindFirst(pathTemplate : string; attributes : ShortWord, VAR searchResult : TSearchRec ) : boolean` - `FindFirst` function searches for files matching a `pathTemplate` and `attributes`.  If successful, `FindFirst` returns *true* and the first match in `searchResult`.
- `function FindNext(VAR searchResults : TSearchRec) : boolean` - The `FindNext` function looks for the next matching file, as defined in the search criteria given by the preceding `FindFirst` call.
- `procedure FindClose(VAR searchResults : TSearchRec) - The `FindClose` function closes a successful FindFirst (and FindNext) file search. It frees up the resources used by the search in 'searchResults`.

The `pathTemplate` is full or relative path to the directory to seach. It is a *template* because it may contain wild cards that match many different files: `?` to match any one character and/or `*`  to match 0, 1 or more characters

The search attributes are the same as described above.  These can be concatenated:  You may set multiple `attributes` from one or more of these by simply `ORing them together.

## Units

## Extended Pascal Features

- `PACKED ARRAY[..] OF CHAR` is not a string.  But `PACKED ARRAY[] OF CHA`R does have some legacy behavior that allow them some limited behavior like `STRINGS`
- `SIZEOF` and `LENGTH` built-in are supported
- Several test cases found on the internet failed because they involved the use of non-standard Pascal types(`filename`, `list`) and undefined functions, and  procedures (`fsplit`, `lowercase`, etc.).  FreePascal examples, in particular, use a object-base Pacal variant which has compatible differences with this more traditional, procedural pascal.

I have seen initialization of file types like the following:

    input = 1;
    output = 2;

There are some files that have Turbo-Pascal style constructions.  For example, use `BEGIN` rather than `INITIALIZATION` to introduce the initializers in a Unit file.  I have done a good faith effort to support Turbo-Pascal-isms wherever possible.

## NON-standard Pascal extensions/differences

### Types

- Hexadecimal constants like %89ab
- INPUT and OUTPUT parameters in PROGRAM statement are pre-defined and optional.

### OPERATORS

- Binary shift operators -- << and >> like in C
- '#', "<>", and "><" are all equivalent

### Expressions

- Assumes sizeof(pointer) == sizeof(integer)

## Runtime

### P-Machine

The back-end is modular and can be change.  However, most of the development so far has been done with a P-Code virtual machine.  This is not the standard, UCSD P-code but a unique machine model  developed for this project.  It currently implements only a 16-bit stack machine with a Harvard architecture:  Separate address and data.  Addressing is 16-bit allowing 64Kb for code and 64Kb for data, 128Kb total, for each thread.

### Run-time String Memory

Pascal runtime memory is divided into four regions:  String stack, RO data, the Pascal run-time stack, and a heap region.  The size of the string stack is set with the -t option to prun and 1024 is the default size used by the testone.sh script.  String allocations are large, the default size is 80 bytes (note 1), and the string stack cleanup is lazy; perhaps only when a procedure/function returns and local string variables go out of scope.  As a result, programs with functions that do a lot of string  operations may need a start larger than the 1024 default.  All of the test files here work OK with a string stack of 1024.

This problem is largely alleviated by using short strings that do not require such large string stack allocations.

In order manage the string stack, two special instructions are supported:  `PUSHS` which pushes the string stack pointer and `POPS` that recovers the string stack pointer.  The compiler generates these on entry and exit from each dynamic nesting level.  In addition, the compiler generates numerous `PUSHS`/`POPS` pairs around each statement and the optimizer is tasked with remove the unnecessary pairs (Note 2).

Note 1: The actual size of the string allocations is controlled by a configuration setting when the compiler is built, but is fixed at runtime.
Note 2: The algorithm in the optimizer is lame at the moment.  It tends to leave unnecesary (but harmless) stack operations in the code.  I have some concerns that it might also release string stack space while it is still in use.  I haven't seen this, but I suspect it is possible.  Eventually, the optimizer should track usage of all string memory and free it the stack space when the variables are no longer used.

## BUGS, ISSUES, and QUIRKS

### BUGS / MISSING FUNCTIONALITY

-See unimplemented standard procedures and functions..
- Need forward references for procedures.  Necessary to support co-routines.

### PLANNED IMPROVEMENTS

- In tokenization, verify that the compiler string stack does not overflow when character added.

### Compile-Time Options

- Option to turn on listing
- Option to interleave assembly language with listing.  Source line numbers are already provided in the listing.
- Option to select symbol table size (or let it grow dynamically)
- List file should only be produced if option provided.

### Debugger

Provide instrumentation to use the line number data in the object files.  In debugger, display source line.  Allow stepping from source line to source line.

### Strings

- Should check for duplicate strings in the RO string area.

## Components

### Compiler

The Pascal compiler is a single pass compiler that targets a 16-bit P-Code stack machine.  It accepts a as Pascal source file as input and generates an unoptimized, P-Code object file with the extension `.o1`.  This code is not suitable for execution:  The code is non-optimal and contains unresolved label references.  It is not in a format that can be loaded for execution.  The optimizer and the linker will address these issues as described below.

    USAGE:
      pascal [options] <program-filename>
    [options]
      -I<include-path>
        Search in <include-path> for additional Unit files
        needed byte program file.
        A maximum of 8 pathes may be specified
        (default is current directory)

### Optimizer

The optimizer accepts the `.o1` files generated by the compiler, improves these, and generates an optimized object file with the extension`.o`.  Simple *peephole* optimization is performed.  In addition, relocation information information is generated and label references are resolved.  The optimizer accepts only the full path to the unoptimized `.o1` as its argument.

### Linker

The linker combines multiple files -- Pascal `PROGRAM` and `UNIT` files -- into a single executable file with the extension `.pex`.  The linker must be used even if the entire program resides within a single file; linking is required in order to generate the executable format.

    USAGE:
      plink <in-file-name> [<in-file-name>] <out-file-name>

Up to eight input file names may be provided.

### Lister

The lister accepts and object file -- `.o1`, `.o`, or `.pex` -- and provides and pretty disassembled listing with file names and line number.  All contents of the POFF file can be shown:

    USAGE:
      plist [options] <poff-filename>
    options:
      -a --all              Equivalent to: -h -S -s -r -d
      -h --file-header      Display the POFF file header
      -l --section-headers  Display line number information
      -S --section-headers  Display the sections' header
      -s --symbols          Display the symbol table
      -r --relocs           Display the relocations
      -d --disassemble      Display disassembled text
      -H --help             Display this information

### Run-Time

The P-Code run-time virtual machine can be used execute and/or debugt the `.pex` file:

    USAGE:
      prun [OPTIONS] <program-filename>
    OPTIONS:
      -a <string-buffer-size>
      --alloc <string-buffer-size>
        Size of the string buffer to be allocated whenever a
        'string' variable is created (default: 80)
      -s <stack-size>
      --stack <stack-size>
        Memory in bytes to allocate for the pascal program
        stack in bytes (minimum is 1024; default is 4096 bytes)
      -t <string-storage-size>
      --string <string-storage-size>
        Memory in bytes to allocate for the pascal program
        string storage in bytes (default is 0 bytes)
      -n <heap-size>
      --new <heap-size>
        Memory in bytes to allocate for the pascal program
        head use for new() (default is 0 bytes)
      -d
      --debug
        Enable PCode program debugger
      -h
      --help
        Shows this message

### Debugger

**Starting the Debugger**. The debugger is built in the Pascal run-time progrem, `prun`, and is started by simply adding the command line option `--debug`.

**P-Code Debugger**.  The debugger is a P-code level debugger.  This is very useful for the compiler tool developer but not ideal for the Pascal application developer.  The debugger supports this built-in help text:

    Commands:
      RE[set]   - Reset
      RU[n]     - Run
      S[tep]    - Single Step (Into)
      N[ext]    - Single Step (Over)
      G[o]      - Go
      BS xxxx   - Set Breakpoint
      BC n      - Clear Breakpoint
      WS xxxx   - [Re]set Watchpoint
      WF xxxx   - Level 0 Frame Watchpoint
      WC        - Clear Watchpoint
      DP        - Display Program Status
      DT        - Display Program Trace
      DS [xxxx] - Display Stack
      DI [xxxx] - Display Instructions
      DB        - Display Breakpoints
      H or ?    - Shows this list
      Q[uit]    - Quit

**Pascal Source Debugger**.  There is no source-level debugger at present, although all of the components to support such a debugger are present:  The executable format, *POFF* holds the file and line number information that can map any assembly-level instruction address to to a specific line in a specific Pascal source file.  Thetr is also library of *POFF* access helper functionis, `libpoff`, that makes access to the file and line number information very simple.  So a project to develop such a debugger is completely feasible and, in fact, not such a difficult job.

### Flat Address Space Issues

In a real application, you would probably need to have multiple Pascal programs running cocurrently.  This is not a problem with desktop systems like Linux or Windows where each program execution is encapsulated within its own address space.  But there could be issues in a flat address space such as you would have with an RTOS or on some custom bare-metal platform.

In particular, in these flat address space environments, there will be on a single instance of each global variable that is shared between all instances of the program that are running.  Contrast this to the destktop OS environment where there will be a unique, private copy of each global variable in each process.  This is not a problem for Pascal per se, since it is an interpreted P-Code solution and each instance has its one simulated environment.  However, it can be a problem for the run-time code and, perhaps, tools in that environment.

The run-time code has protections for this case:  The run-time uses no global variables and keeps all state information on the target machine stack (which is not the same as the P-Code stack).  So it should be possible to safely start many Pascal P-Code programs that all run concurrently (although there is not much that they can do to interact with no well-defined Pascal IPCs, Inter-Process Communications).

The tools do use global variables, however.  So in a flat address space only one instance of the compiler, optimizer, linker, or lister can run at a time (the debugger is part of the protected run-time so there can be multiple, concurrent debug sessions).  This is not thought to be an issue in most cases but could become issue if, for example, the tools are used for JIT (Just-In-Time) compilations in a multi-threaded environment.

## Register Model / Native Code Translation / 32-Machine

- Translation to 32-bit register model.  Support for this model was removed by commit 94a03ca1f2d138b5189924527331fedba2248caa only because I did not  have bandwidth to support it.  That would still be a good starting point.
- Native code translator

### Pascal Object File Format (POFF)

## ISSUES

**Issues with Default Files**.  There is an issue with using file types as the first argument of most standard file I/O procedures and functions.  This is a one-pass compiler and before we consume the input, we must be certain that that first argument this is going to resolve into a file type.

There is a particular problem if the first argument is a `READ` (or an `ARRAY of RECORDS`).  A `RECORD` could contain a file field that is selected further on.  All we know at the point of encountering the `RECORD` is that it is a `RECORD`; we cannot know the type of the `RECORD` field that is selected until we parse a little more.  So the logic will make bad decisions with `RECORD`s (and worse, with `ARRAYs` of `RECORD`s).

An option disables all support for files in `RECORD`s and at least makes the behavior consistent by disallowing `RECORD`s containing files. Perhaps, in the future, we will add some kind of parse ahead logic to handle this case.

**Variable Length Strings in Fixed Record Length Files**. There is a problem with binary files with fixed length records.  The problem involves files of records containing variable length strings.  Variable length strings consistent of a small fixed-size container structure and refers to a variable length string in separate, string memory.  The string itself is not part of the record.

When such records are written to a file, only the container is written.  This is clearly wrong, but what is the right solution?  How should variable length strings be represented when written to a file with fixed record lengths?

A work-around is to use `PACKED ARRAY[] OF CHAR` which lies entirely within the `RECORD`.

**Type Identifiers in Constant Definitions**.  Types in constant definitions are not accepted.  This, for example, does not work:

    names : array [color] of String[7]
              = ('red', 'blue', 'yellow', 'green', 'white', 'black', 'orange');

**Name Collision Issue**.  All symbols are kept in a symbol table.  This means that all naming must be unique.  For example, you could not have a type with the same name as a variable.  You could not do this:

    FUNCTION Map24to12(HourType: HourType): INTEGER;

Most of these are kinds of name occlusion are reasonable (if not 100% correct), but there are a couple of cases that are more problematic.  I have not seen any authoritative discussion about what duplicate names may coexist for different types.

A rather imperfect solution is in place for record field names with very often collide with other naming.  Basically record field names are ignored in two cases.  First if the record field name follows a period, as with:

    IF ptr^.stuff = stuff THEN ...

Where the second `stuff` is a variable with the same name as the record field.  Use of records with fields like `ptr^.word`, as another example, might otherwise collide with the `word` type.

This solution is *imperfect* in that the presence of the period followed by a record field name is not a guarantee of correct syntax.  The second case is when the field name is encountered within a `WITH *record*` block.  Again, this is imperfect because the field can then, again, collide with other naming within the `WITH *record*` block.

Precedence is given to the interpretation of the identifier as a field name in both of these cases.  Record field names are completely ignored in other contexts.

**Error Handling**.  Error handling is not user friendly at the moment:

- Output is only line number, an error number, and a token ID.  That information is written to a file with the extenion `.err`.  I am think that a separate error printing program that uses this error file might be the preferred solution because it will keep the size of the compiler as small as possible.
- One error typically generates numerous, additional, bogus errors if the compiler continues to execute.  What is probably needed here is logic to (1) skip to the end of the statement or perhaps to the end of the line when an error is detected and (2) re-prime the tokenizer to start fresh on the next link.
- There are probably other bad behaviors that might happen(?).  Maybe crashes or hangs.  The error conditions all need to be analyzed and tested.

**64-Bit Integer Types**.  I believe that the `INT64` type is needed only for long file positions in procedures like `SEEK` and functions like `FILEPOS` and `FILESIZE`.  Currently, `INT64` is  simply aliased inside the compiler to `LONGINTEGER`, thus limiting the size of files that can be handled.

**NULL Statements**.  In some cases, the compiler allows a *NULL Statement*, i.e., an extra semi-colon in contexts where one is not needed.  In other cases, it does not permit these NULL statements.  Need to make this consistent:  Either be consistently strict or consistently permissive.
