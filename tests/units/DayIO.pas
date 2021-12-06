UNIT DayIO;
(* This unit provides a facility to input and output day of the week
 * names.  The names are Sun, Mon, Tue, Wed, Thu, Fri, and Sat.  Such
 * names may be read in using ReadDay, written using WriteDay.  ReadDay
 * reads in the name, and returns it as a value of the enumerated type
 * DayType.  The input value is case-sensitive, and must be entered
 * exactly as given in the list above.  WriteDay takes a DayType value
 * and prints it, using one of the string above.  There is also a
 * function MapToDay which accepts a string containing the name of a
 * day and maps it to a DayType value.
 *)

INTERFACE
    TYPE
        { Type to represent a day of the week, or an error. }
        DayType = (Sun, Mon, Tue, Wed, Thu, Fri, Sat, BadDay);

    { Convert a string to a day.  If it is not a legal day, the result is
      BadDay.
        Precondition: None:
        Postcondition: If S is one of the strings Sun Mon Tue Wed Thu Fri, or
            Sat, MapToDay returns the corresponding day.  Otherwise, it
            returns BadDay. }
    FUNCTION MapToDay(S: String): DayType;

    { Read a day from the file.  The day must be the next item on the same
      line.  The procedure skips leading blanks, and reads the next
      non-blank item on the line, and returns the day read.  If there was
      no item on the line, or the item was not a legal day, it returns
      BadDay.
        Precondition: InFile is open for reading.
        Postcondition: The file has been read until the first non-blank
            character is seen, then through the first blank character, but
            not past the end of the current line.  If the sequence of
            non-blank characters read matches one of the day strings
            Sun Mon Tue Wed Thu Fri or Sat, the corresponding day of the
            week is returned in Day.  If not, or if no non-blank characters
            were read, the item BadDay is returned. }
    PROCEDURE ReadDay
            (VAR InFile: TEXT;                (* Input file read from. *)
             VAR Day: DayType);        (* Returned day of the week value. *)

    { Write a day to the file.
        Precondition: OutFile is open for writing.
        Postcondition: The string of characters Sun Mon Tue Wed Thu Fri or Sat
        corresponding the value is Day is written to OutFile. }
    PROCEDURE WriteDay
            (VAR OutFile: TEXT;                (* Input file written to. *)
                  Day: DayType);                (* Day to write. *)

IMPLEMENTATION
    CONST
        { Size of day strings. }
        DaySize = 3;
    TYPE
        { Type of a day. }
        DayStrType = STRING[DaySize];
    VAR
        { Map from enumerated day type to characters. }
        DayMap: ARRAY[DayType] of DayStrType;

    { Convert a string to a day.  If it is not a legal day, the result is
      BadDay. }
    FUNCTION MapToDay(S: String): DayType;
        VAR
            Day: DayType;                (* Scanner. *)
            Found: boolean;                (* Tell if a match was found. *)
        BEGIN
            Found := FALSE;
            Day := Sun;
            WHILE (Day < BadDay) AND NOT Found DO
                BEGIN
                    IF DayMap[Day] = S THEN
                        Found := TRUE
                    ELSE
                        Day := succ(Day)
                END;

            MapToDay := Day
        END;

    { Read one character, but do not read past the end of line.  Just
      return a space.
          Pre: InFile is open for reading.
         Post: If InFile was at eoln, Ch is set to ' ', and InFile is
              unchanged.  Otherwise, one character is read from InFile to Ch. }
    PROCEDURE ReadOnLine(VAR InFile: TEXT; VAR Ch: Char);
        BEGIN
            IF eoln(InFile) THEN
                Ch := ' '
            ELSE
                read(InFile, Ch)
        END;

    { Read a day from the file.  The day must be the next item on the same
      line.  The procedure skips leading blanks, and reads the next
      non-blank item on the line, and returns the day read.  If there was
      no item on the line, or the item was not a legal day, it returns
      BadDay. }
    PROCEDURE ReadDay
            (VAR InFile: TEXT;                (* Input file read from. *)
             VAR Day: DayType);                (* Returned day of the week value. *)
        VAR
            Ch: char;                        (* Input character. *)
            DayStr: DayStrType;                (* Input string of chars. *)
        BEGIN
            (* Skip blanks. *)
            Ch := ' ';
            WHILE (Ch = ' ') AND NOT eoln(InFile) DO
                BEGIN
                    read(InFile, Ch)
                END;

            (* See if we found a non-blank character. *)
            IF Ch = ' ' THEN
                (* The skip loop must have ended at eoln. *)
                Day := BadDay
            ELSE
                BEGIN
                    (* Read the characters. *)
                    DayStr := '';
                    WHILE (Ch <> ' ') AND (Length(DayStr) < DaySize) DO
                        BEGIN
                            DayStr := DayStr + Ch;
                            ReadOnLine(InFile, Ch)
                        END;

                    (* Match must be exact. *)
                    IF Ch <> ' ' THEN
                        (* Something else out there.  Not good. *)
                        Day := BadDay
                    ELSE
                        BEGIN
                             (* Discard any remaining characters. *)
                             WHILE (Ch <> ' ') AND NOT eoln(InFile) DO
                                 read(InFile, Ch);

                             (* Map the string to the enum. *)
                             Day := MapToDay(DayStr)
                        END
                END
        END;

    { Write a day to the file. }
    PROCEDURE WriteDay
            (VAR OutFile: TEXT;                (* Input file written to. *)
                  Day: DayType);                (* Day to write. *)
        BEGIN
            write(OutFile, DayMap[Day])
        END;

    BEGIN
        (* Initialize the DayMap.  This is an easy way to convert
           DayType values to strings.  It is used internally by the
           unit. *)
        DayMap[Sun] := 'Sun';
        DayMap[Mon] := 'Mon';
        DayMap[Tue] := 'Tue';
        DayMap[Wed] := 'Wed';
        DayMap[Thu] := 'Thu';
        DayMap[Fri] := 'Fri';
        DayMap[Sat] := 'Sat';
        DayMap[BadDay] := '***'
    END.

