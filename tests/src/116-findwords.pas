program findwords(input, output);

const
    { TRUE = 1;
      FALSE = 0; }
      BS = 8;
      TAB = 9;
      LF = 10;
      VT = 11;
      FF = 12;
      CR = 13;

{  this holds the line numbers for each word. Its double linked for
   ease of freeing memory later on }
type
     listptr = ^list;
     list = record
              line : integer;       { line number of occurrence     }
              nextline : listptr;   { link to next line number      }
              prevline : listptr    { link to previous line number  }
     end;

{ this holds the word with a link to a struct list holding line
   numbers. Double linking to simplify freeing of memory later on   }
    wordptr = ^words;
    words = record
              word : string;        { pointer to word                 }
              lines : listptr;      { pointer to list of line numbers }
              nextword : wordptr;   { pointer to next word in list    }
              prevword : wordptr;   { pointer to previous word in list}
     end;

var
   head, tail : wordptr;            { beginning and end of list       }
   fin : file of char;              { input file handle               }
   filename : string;               { name of input file              }
   thisisfirstword : boolean;       { to handle start of list words=0 }

{ customised exit routine to provide orderly shutdown }
procedure myexit(exitcode : integer);
var
   word_ptr, tempw : wordptr;
    line_ptr, templ : listptr;
begin
 { close input file }
 close(fin);

    { free any allocated memory }
   writeln('Deallocating memory:');
   word_ptr := head;
   while word_ptr <> nil do
   begin
      tempw := word_ptr;               { remember where we are        }
      line_ptr := word_ptr^.lines;     { go through line storage list }
      while line_ptr <> nil do
      begin
         templ := line_ptr;                { remember where we are    }
         line_ptr := line_ptr^.nextline;   { point to next list       }
         dispose(templ)                    { free current list        }
      end;
      word_ptr := word_ptr^.nextword;      { point to next word node  }
      dispose(tempw)                       { free current word node   }
   end;

   { return to OS }
   halt
end;

{ check to see if word already in list, 1=found, 0=not present }
function checkword(queryword : string) : boolean;
var ptr : wordptr;
begin
   ptr := head;                     { start at first word in list      }
   while ptr <> nil do
   begin
      if ptr^.word = queryword then { found the word?                  }
         checkword := TRUE;         { yes, return found                }
      ptr := ptr^.nextword          { else cycle to next word in list  }
   end;
   checkword := FALSE               { word has not been found in list  }
end;

{ enter word and occurrence into list }
procedure makeword(addword : string; linenumber : integer);
var
    newword, word_ptr : wordptr;
    newline, line_ptr : listptr;
begin
    if checkword(addword) = FALSE then
    begin
       { insert word into list }
       new(newword);
       if newword = nil then
       begin
          writeln('Error allocating word node for new word: ', addword);
          myexit(1)
       end;
       { add newnode to the list, update tail pointer }
       if thisisfirstword = TRUE then
       begin
         head := newword;
         tail := nil;
         thisisfirstword := FALSE;
         head^.prevword := nil
       end;
       newword^.nextword := nil;    { node is signified as last in list }
       newword^.prevword := tail;   { link back to previous node in list }
       tail^.nextword := newword;   { tail updated to last node in list }
       tail := newword;
       { allocate storage for the word including end of string NULL }
       tail^.word := addword;

       { allocate a line storage for the new word }
       new(newline);
       if newline = nil then
       begin
          writeln('Error allocating line memory for new word: ', addword);
          myexit(3)
       end;
       newline^.line     := linenumber;
       newline^.nextline := nil;
       newline^.prevline := nil;
       tail^.lines       := newline
       end
    else
    begin
       { word is in list, add on line number }
       new(newline);
       if newline = nil then
       begin
          writeln('Error allocating line memory for existing word: ', addword);
          myexit(4)
       end;

       { cycle through list to get to the word }
       word_ptr := head;
       while (word_ptr <> nil) and (word_ptr^.word <> addword) do
       begin
          word_ptr := word_ptr^.nextword;
       end;

       if word_ptr = nil then
       begin
          writeln('ERROR - SHOULD NOT OCCUR ');
          myexit(5)
       end;

       { cycle through the line pointers }
       line_ptr := word_ptr^.lines;
       while line_ptr^.nextline <> nil do
          line_ptr := line_ptr^.nextline;

       { add next line entry }
       line_ptr^.nextline := newline;
       newline^.line      := linenumber;
       newline^.nextline  := nil;
       newline^.prevline  := line_ptr  { create back link to previous line number }
    end
 end;

 { read in file and scan for words }
 procedure processfile;
 var
    ch : char;
    loop, in_word, linenumber : integer;
    buffer : string;
 begin
    in_word := 0;       { not currently in a word }
    linenumber := 1;    { start at line number 1  }
    loop := 0;          { index character pointer for buffer[] }
    buffer := '';

    read(fin, ch);
    while not Eof(fin) do
    begin
        case ch of
          chr(CR) : begin
                     if in_word = 1 then begin
                       in_word := 0;
                       makeword(buffer, linenumber);
                       buffer := '';
                     end;
                     linenumber := linenumber + 1
                    end;
         ' ', chr(LF), chr(TAB), chr(VT), chr(FF), ',' , '.'  :
                    begin
                     if in_word = 1 then begin
                        in_word := 0;
                        makeword(buffer, linenumber);
                        buffer := '';
                     end
                    end;
         else
                    begin
                      if in_word = 0 then
                        begin
                          in_word := 1;
                          buffer := buffer + ch
                        end
                      else
                        begin
                          buffer := buffer + ch
                        end
                    end;
       end; { end of switch }
       read(fin, ch)
    end  { end of while }
 end;

 { print out all words found and the line numbers }
 procedure printlist;
 var
    word_ptr : wordptr;
    line_ptr : listptr;
 begin
    writeln('Word list follows:');
    word_ptr := head;
    while word_ptr <> nil do
    begin
       write(word_ptr^.word, ': ');
       line_ptr := word_ptr^.lines;
       while line_ptr <> nil  do
       begin
          write(line_ptr^.line, ' ');
          line_ptr := line_ptr^.nextline
       end;
       writeln;
       word_ptr := word_ptr^.nextword
    end
 end;

 procedure initvars;
 begin
    head := nil;
    tail := nil;
    thisisfirstword := TRUE
 end;

 begin
    write('Enter filename of text file: ');
    readln(filename);
    assign(fin, filename);
    reset(fin);
    {  if  fin = nil then
       begin
          writeln('Unable to open ',filename,' for reading');
          myexit(1)
       end;
    }
    initvars;
    processfile;
    printlist;
    myexit(0)
end.
