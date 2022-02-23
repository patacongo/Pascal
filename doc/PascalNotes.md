# Pascal Notes

## General Information

### TYPES

**Basic Types**.  All basic types I`NTEGER`, `BOOLEAN`, `CHAR`, `REAL`, `SCALAR`, `SUBRANGE`, `RECORD` and `SET` implemented along with some types from more contemporary Pascal types like `STRING` and short `STRING` types.

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

`SET` is limit to 64-bits adjacent elements

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
- `CASE` statement expects integer expression for the switch variable
- `ELSE` in `CASE` statement

## Standard Procedures and Functions

**Caveat**. Implements some functions/procedures that may not be standard, but are in common usage.
**Limitation**.  No `PROCEDURE` call with `PROCEDURE`s or `FUNCTION`s as parameters

### System Procedures/Functions:

- `Exit(exitCode : integer)`
- `Halt`
- `New(<pointer-variable>)`
- `Dispose(<pointer-variable>)`
- `GetEnv(name: string) : string`

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

Borland style string operators

- `Length( s : string)` – Return the length of a string.
- `Copy(s : string, from, howmuch: integer) : string` - Get a substring from a string.
- `Pos(substr, s : string) : integer` - Get the position of a substring within a string. If the substring is not found, it returns 0.
- `Str(numvar : integer, VAR strvar : string)` – Converts a numeric value into a string.  `numvar` may include a fieldwidth and, for the case of real values, a precision.
- `concat(s1,s2,...,sn : string) : string` – Concatenate one or more strings.
- `insert(source : string, VAR target : string; index : integer)` - Insert a string inside another string from at the indexth character.
- `delete(VAR s : string; i, n: integer)` - Deletes `n` characters from string `s` starting from index `i`.
- `fillchar(VAR s : string; count : integer; value : shortword)` -  Fill string s with character value until `s` is `count`-1 characters long
- `Val(str : string; VAR numvar : integer; VAR code : integer)` – Convert a string to a numeric value.  `strvar` is a string variable to be converted, numvar is any numeric variable either `Integer`, `Longinteger`, `ShortInteger`, or `Real`, and if the conversion isn't successful, then the parameter `code` contains the index of the character in `S` which prevented the conversion.

#### File I/O
- `Append` - Opens an existing file for appending data to end of file
- `AssignFile` - Assign a name to a file (Assign is an alias)
- `CloseFile` - Close opened file (Close is an alias)
- `EOF` - Check for end of file
- `EOLN` – Check for end of line
- `FilePos` - Get position in file
- `FileSize` - Get size of file
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
- `Flush` - Write file buffers to disk
- `Get(f : file-type)` - The get procedure reads the next component from a file into the file's buffer variable. NOTE: This procedure is seldom used since it is usually easier to use the read or readln procedures.IOResult - Return result of last file IO operation
- `Put(f : file-type)` – The put procedure writes the contents of a file's buffer variable to the file and empties the file's buffer variable leaving it undefined.
- `Rename` – Rename a filename
- `SetTextBuf`  - Sets size of file buffer

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

##Runtime

### P-Machine

The back-end is modular and can be change.  However, most of the development so far has been done with a P-Code virtual machine.  This is not the standard P-code but a unique machine model  developed for this project.  It currently implements only a 16-bit stack machine with a Harvard architecture:  Separate address and data.  Addressing is 16-bit allowing 64Kb for code and 64Kb for data, 128Kb total, for each thread.

### Run-time String Memory

Pascal runtime memory is divided into four regions:  String stack, RO data, the Pascal run-time stack, and a heap region.  The size of the string stack is set with the -t option to prun and 1024 is the default size used by the testone.sh script.  String allocations are large, the default size is 80 bytes*, and the string stack cleanup is lazy; perhaps only when a procedure/function returns.  As a result, programs with functions that do a lot of string  operations may need a start larger than the 1024 default.  All of the test files here work OK with a string stack of 1024.

This problem is largely alleviated by using short strings that do not require such large string stack allocations.

* The actual size of the string allocations is controlled by a command option to prun, -a.

## BUGS, ISSUES, and QUIRKS

### BUGS / MISSING FUNCTIONALITY

-See unimplemented standard procedures and functions..
- In `FUNC`/`PROC` calls, if `simpleExpressio`n fails to find a parameter (eg., proc (,,), no error is detected.
- There are cases where the string stack is not being managed correctly (e.g., `usesSection()`).
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

- All RO strings should be represented as short strings when converted to Pascal strings
- Should check for duplicate strings in the RO string area.

## Components
### Compiler
### Optimizer
### Linker
### Lister
### Run-Time
### Debugger

## Register Model / Native Code Translation

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

**Name Collision Issue**.  All symbols are kept on a symbol table stack.  This means that all naming must be unique.  For example, you could not have a type with the same name as a variable.  You could not do this:

    FUNCTION Map24to12(HourType: HourType): INTEGER;

Most of these are kinds of name occlusion are reasonable (if not 100% correct), but there are a couple of cases that are more problematic.  For example, record field name is treated like any other named thing by the compiler.  So you cannot have a RECORD field with the same name as a variable or type as in the following.

    IF ptr^.stuff = stuff THEN ...

Use of records with fields like `ptr^.word` would also collide with the `word` type.  Some of these worse offenders need to be fixed.

**Error Handling**.  Error handling is not user friendly at the moment:

- Output is only line number, an error number, and a token ID.
- One error typically generates numerous, additional, bogus errors if the compiler continues to execute.
- There are probably other bad behaviors that might happen(?).  Maybe crashes or hangs.  The error conditions all need to be analyzed and tested.

**64-Bit Integer Types**.  I believe that the `INT64` type is needed only for long file positions in procedures like `SEEK` and functions like `FILEPOS` and `FILESIZE`.  Currently, `INT64` is  simply aliased inside the compiler to `LONGINTEGER`, thus limiting the size of files that can be handled.
