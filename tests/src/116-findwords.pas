PROGRAM findwords(input, output);

CONST
  BS  = 8;
  TAB = 9;
  LF  = 10;
  VT  = 11;
  FF  = 12;
  CR  = 13;

{  this holds the line numbers for each word. Its double linked for
   ease of freeing memory later on }
TYPE
  listptr = ^list;
  list = RECORD
       line : integer;               { line number of occurrence     }
       nextline : listptr;           { link to next line number      }
       prevline : listptr            { link to previous line number  }
     END;

{ this holds the word with a link to a struct list holding line
   numbers. Double linking to simplify freeing of memory later on   }

  wordptr = ^words;
  words = record
      wordText : string;            { pointer to word                 }
      lines : listptr;              { pointer to list of line numbers }
      nextword : wordptr;           { pointer to next word in list    }
      prevword : wordptr;           { pointer to previous word in list}
    END;

VAR
  head, tail : wordptr;             { beginning and END of list       }
  fin : file of char;               { input file handle               }
  filename : string;                { name of input file              }
  thisisfirstword : BOOLEAN;        { to handle start of list words=0 }

{ customised exit routine to provide orderly shutdown }

PROCEDURE myexit(exitcode : INTEGER);
VAR
   word_ptr, tempw : wordptr;
   line_ptr, templ : listptr;
BEGIN
  { close input file }

  close(fin);

  { free any allocated memory }

  WRITELN('Deallocating memory:');
  word_ptr := head;
  WHILE word_ptr <> NIL DO
  BEGIN
     tempw := word_ptr;                { remember where we are        }
     line_ptr := word_ptr^.lines;      { go through line storage list }

     WHILE line_ptr <> NIL DO
     BEGIN
        templ := line_ptr;                 { remember where we are    }
        line_ptr := line_ptr^.nextline;    { point to next list       }
        dispose(templ)                     { free current list        }
     END;

     word_ptr := word_ptr^.nextword;       { point to next word node  }
     dispose(tempw)                        { free current word node   }
  END;

  { return to OS }

  halt
END;

{ check to see if word already in list, 1=found, 0=not present }

FUNCTION checkword(queryword : STRING) : BOOLEAN;
VAR ptr : wordptr;
BEGIN
  ptr := head;                      { start at first word in list      }

  WHILE ptr <> NIL DO
  BEGIN
    IF ptr^.wordText = queryword THEN { found the word?              }
       checkword := TRUE;           { yes, return found                }
    ptr := ptr^.nextword            { ELSE cycle to next word in list  }
  END;

  checkword := FALSE                 { word has not been found in list  }
END;

{ enter word and occurrence into list }

PROCEDURE makeword(addword : STRING; linenumber : INTEGER);
VAR
  newword, word_ptr : wordptr;
  newline, line_ptr : listptr;
BEGIN
  IF checkword(addword) = FALSE THEN
  BEGIN
    { insert word into list }

    NEW(newword);
    IF newword = NIL THEN
    BEGIN
       WRITELN('Error allocating word node for new word: ', addword);
       myexit(1)
    END;

    { add newnode to the list, update tail pointer }

    IF thisisfirstword = TRUE THEN
    BEGIN
      head := newword;
      tail := NIL;
      thisisfirstword := FALSE;
      head^.prevword := NIL
    END;

    newword^.nextword := NIL;       { node is signified as last in list }
    newword^.prevword := tail;      { link back to previous node in list }
    tail^.nextword := newword;      { tail updated to last node in list }
    tail := newword;

    { allocate storage for the word including END of string NULL }

    tail^.wordText := addword;

    { allocate a line storage for the new word }

    NEW(newline);
    IF newline = NIL THEN
    BEGIN
      WRITELN('Error allocating line memory for new word: ', addword);
      myexit(3)
    END;

    newline^.line     := linenumber;
    newline^.nextline := NIL;
    newline^.prevline := NIL;
    tail^.lines       := newline
  END
  ELSE
  BEGIN
    { word is in list, add on line number }

    NEW(newline);
    IF newline = NIL THEN
    BEGIN
       WRITELN('Error allocating line memory for existing word: ', addword);
       myexit(4)
    END;

    { cycle through list to get to the word }

    word_ptr := head;
    WHILE (word_ptr <> NIL) AND (word_ptr^.wordText <> addword) DO
    BEGIN
       word_ptr := word_ptr^.nextword;
    END;

    IF word_ptr = NIL THEN
       BEGIN
          writeln('ERROR - SHOULD NOT OCCUR ');
          myexit(5)
       END;

    { cycle through the line pointers }

    line_ptr := word_ptr^.lines;
    WHILE line_ptr^.nextline <> NIL DO
       line_ptr := line_ptr^.nextline;

    { add next line entry }

    line_ptr^.nextline := newline;
    newline^.line      := linenumber;
    newline^.nextline  := NIL;
    newline^.prevline  := line_ptr  { create back link to previous line number }
  END
  END;

{ read in file and scan for words }

PROCEDURE processfile;
VAR
  ch : CHAR;
  loop, in_word, linenumber : INTEGER;
  buffer : STRING;
BEGIN
  in_word := 0;        { not currently in a word }
  linenumber := 1;     { start at line number 1  }
  loop := 0;           { index character pointer for buffer[] }
  buffer := '';

  READ(fin, ch);
  WHILE not Eof(fin) DO
  BEGIN
    CASE ch OF
      CHR(CR) :
        BEGIN
          if in_word = 1 THEN
          BEGIN
            in_word := 0;
            makeword(buffer, linenumber);
            buffer := '';
          END;
          linenumber := linenumber + 1
        END;
      ' ', CHR(LF), CHR(TAB), CHR(VT), CHR(FF), ',' , '.'  :
        BEGIN
          IF in_word = 1 THEN
          BEGIN
            in_word := 0;
            makeword(buffer, linenumber);
            buffer := '';
          END
        END;
      ELSE
        BEGIN
          IF in_word = 0 THEN
          BEGIN
            in_word := 1;
            buffer := buffer + ch
          END
          ELSE
          BEGIN
            buffer := buffer + ch
          END
        END;
    END; { END of switch }
    READ(fin, ch)
  END  { END of WHILE }
END;

{ print out all words found and the line numbers }

PROCEDURE printlist;
VAR
  word_ptr : wordptr;
  line_ptr : listptr;
BEGIN
  WRITELN('Word list follows:');
  word_ptr := head;
  WHILE word_ptr <> NIL DO
  BEGIN
    WRITE(word_ptr^.wordText, ': ');
    line_ptr := word_ptr^.lines;
    WHILE line_ptr <> NIL DO
    BEGIN
      WRITE(line_ptr^.line, ' ');
      line_ptr := line_ptr^.nextline
    END;
    WRITELN;
    word_ptr := word_ptr^.nextword
  END
END;

PROCEDURE initvars;
BEGIN
   head := NIL;
   tail := NIL;
   thisisfirstword := TRUE
END;

BEGIN
  WRITE('Enter filename of text file: ');
  READLN(filename);
  ASSIGN(fin, filename);
  RESET(fin);
  {  IF  fin = NIL THEN
     BEGIN
       writeln('Unable to open ',filename,' for reading');
       myexit(1)
     END;
  }
  initvars;
  processfile;
  printlist;
  myexit(0)
END.
