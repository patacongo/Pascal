UNIT FileUtils;

INTERFACE
  USES
    Machine in 'Machine.pas'

  CONST
    faSysFile     = 1 << 0;  (* The file is a system file *)
    faDirectory   = 1 << 1;  (* File is a directory. *)
    faArchive     = 1 << 2;  (* The file needs to be archived (info only). *)
    faReadOnly    = 1 << 3;  (* The file is read-only (info only). *)
    faVolumeId    = 1 << 4;  (* Drive volume Label *)
    faHidden      = 1 << 5;  (* The file is hidden. *)
    faAnyFile     = faSysFile OR faDirectory OR faVolumeId OR faHidden

  TYPE
    TDir = TMachinePointer;
    TSearchRec = RECORD
      name  : STRING[NameSize]; (* Name of the file found *)
      attr  : ShortWord;        (* The file attribute bitset *)
      time  : LongWord;         (* Time/date of last modification *)
      size  : INT64;            (* The size of the file found in bytes *)

      match : STRING[NameSize]; (* Match file name template *)
      dir   : TDir;             (* Used internally *)
      sattr : ShortWord         (* Used internally *)
    END

  FUNCTION  ExtractFilePath(FullPath : string ) : string;
  FUNCTION  ExtractFileName(FullPath : string ) : string;
  FUNCTION  MatchFileName(FileName, MatchString : string ) : boolean;

  FUNCTION  FindNext(VAR SearchResult : TSearchRec) : BOOLEAN;
  FUNCTION  FindFirst(PathTemplate : string; attributes : ShortWord;
                      VAR SearchResult : TSearchRec ) : BOOLEAN;
  PROCEDURE FindClose(VAR SearchResult : TSearchRec);

IMPLEMENTATION

  { Return the position of the last delimiter in the string.  Zero is
    returned if no delimiter is found. }

  FUNCTION LastDelimiter(FullPath : string) : integer;
  VAR
    Position : integer;

  BEGIN
    (* Loop to find position of last delimiter *)

    LastDelimiter  := 0;
    Position       := 0;

    REPEAT
      (* Get the position of the first/next delimiter                      *)

      Position := POS(FullPath, DirSeparator, Position + 1);

      (* At least one delimiter must be present to return a  non-zero      *)
      (* value.                                                            *)

      IF Position  <> 0 THEN
        LastDelimiter  := Position;

    UNTIL Position = 0;
  END;

  { Extract the file path up to (but not including) the file delimiter. }

  FUNCTION ExtractFilePath(FullPath : string) : string;
  VAR
    Position : integer;

  BEGIN
    (* Get the position of the last delimiter                              *)

    Position := LastDelimiter(FullPath);

    (* A zero position means that there is no delimiter in the path and we *)
    (* return an empty string.  Otherwise The directory path is the        *)
    (* substring from the beginning of the string up to and including the  *)
    (* the delimiter.                                                       *)

    IF Position = 0 THEN
      ExtractFilePath := ''
    ELSE
      ExtractFilePath := Copy(FullPath, 1, Position);
  END;

  FUNCTION ExtractFileName(FullPath : string ) : string;
  VAR
    Position   : integer;
    NameLength : integer

  BEGIN
    (* Get the position of the last delimiter                              *)

    Position := LastDelimiter(FullPath);

    (* A zero position means that there is no delimiter in the path and we *)
    (* return the entire path.  Otherwise The file name is the substring   *)
    (* from just AFTER the delimiter position, through the end of the      *)
    (* string.                                                             *)

    IF Position = 0 THEN
      ExtractFileName := FullPath
    ELSE
    BEGIN
      NameLength      := Length(FullPath) - Position;
      ExtractFileName := Copy(FullPath, Position + 1, NameLength);
    END
  END;

  { Return true if a 'FileName 'matches a 'MatchString'.  The `MatchString`
    is filename string that may contain wild cards that match many different
    files: `?` to match any one character and/or `*`  to match 0, 1 or more
    characters. }

  FUNCTION MatchFileName(FileName, MatchString : string ) : boolean;
  VAR
    NamePosition     : integer;
    TmpNamePosition  : integer;
    NameLength       : integer;
    MatchPosition    : integer;
    MatchLength      : integer;
    NoMatchHere      : boolean;
    NameChar         : char;
    MatchChar        : char;

  BEGIN
    (* Get the position of the last delimiter                              *)
    (* Search every character position in the in the FileName or until a   *)
    (* we get a match.                                                     *)

    NameLength    := Length(FileName);
    MatchLength   := Length(MatchString);
    MatchFileName := false;
    NoMatchHere   := false;

    NamePosition  := 1;
    WHILE (NamePosition <= NameLength) AND NOT MatchFileName DO
    BEGIN
      MatchPosition := 1;
      TmpNamePosition   := NamePosition;

      WHILE (MatchPosition   <= MatchLength) AND
            (TmpNamePosition <= NameLength) AND
            NOT (MatchFileName OR NoMatchHere) DO
      BEGIN
        MatchChar := CharAt(MatchString, MatchPosition);
        NameChar  := CharAt(FileName, TmpNamePosition);

        CASE MatchChar OF
        '?' : (* Matches a single character *)
          BEGIN
            (* And single character matches.  But up positions.            *)

            MatchPosition   := MatchPosition + 1;
            TmpNamePosition := TmpNamePosition + 1;

            (* Check if the whole file name matches.                       *)

            MatchFileName   := (MatchPosition > MatchLength) AND
                               (TmpNamePosition > NameLength)
          END;

        '*' :
          BEGIN
            (* Get the next character after the '*'.  Additional '*' or    *)
            (* '?' characters after the '*' are ignored.                   *)

            REPEAT
              MatchPosition := MatchPosition + 1;
              MatchChar     := CharAt(MatchString, MatchPosition)
            UNTIL (MatchPosition <= MatchLength) AND (MatchChar <> '*') AND
                  (MatchChar <> '?');

            (* Check for a match to the end of the line.                   *)

            IF (MatchPosition > MatchLength) THEN
              MatchFileName := true
            ELSE
            BEGIN
              NameChar          := CharAt(FileName, TmpNamePosition);
              WHILE (NameChar <> MatchChar) OR
                    (TmpNamePosition > NameLength) DO
              BEGIN
                TmpNamePosition := TmpNamePosition + 1;
                NameChar        := CharAt(FileName, TmpNamePosition);
              END;

              IF TmpNamePosition > NameLength THEN
                NoMatchHere     := true
              ELSE
              BEGIN
                (* We have a matching after skipping 0 or more characters. *)

                MatchPosition   := MatchPosition + 1;
                TmpNamePosition := TmpNamePosition + 1;

                (* Check if the whole file name matches.                   *)

                MatchFileName   := (MatchPosition > MatchLength) AND
                                   (TmpNamePosition > NameLength)
              END
            END
          END;
        ELSE
          IF (MatchChar <> NameChar) THEN
             NoMatchHere     := true
          ELSE
          BEGIN
            (* We have a matching charactes.                               *)

            MatchPosition   := MatchPosition + 1;
            TmpNamePosition := TmpNamePosition + 1;

            (* Check if the whole file name matches.                       *)

            MatchFileName   := (MatchPosition > MatchLength) AND
                               (TmpNamePosition > NameLength)
          END
        END;

        NamePosition := NamePosition + 1
      END
    END
  END;

  FUNCTION FindNext(VAR SearchResult : TSearchRec) : boolean;
  VAR
    Success   : boolean;
    Found     : boolean;
    FirstChar : char

  BEGIN
    (* Read from the directory until a file is found or until we have read
       all of the directory entries. *)

    Found := false;
    REPEAT
      Success := ReadDir(SearchResult.dir, SearchResult);

      (* Visible, regular files always accepted, directories, system files,
         and hidden files are only reported if so requested. *)

       FirstChar := CharAt(SearchResult.name, 1);
       IF Success AND (Length(FirstChar) > 0) THEN
       BEGIN
         IF (FirstChar = '.') AND ((SearchResult.sattr & faHidden) <> 0) THEN
           Found := true
         ELSE IF (SearchResult.attr & SearchResult.sattr & faSysFile) <> 0 THEN
           Found := true
         ELSE IF (SearchResult.attr & SearchResult.sattr & faDirectory) <> 0 THEN
           Found := true
         ELSE IF (SearchResult.attr & SearchResult.sattr & faVolumeId) <> 0 THEN
           Found := true;
       END;

       IF Found AND NOT MatchFileName(SearchResult.name, SearchResult.match) THEN
         Found := false
    UNTIL Found OR NOT Success;

    FindNext := Found
  END;

  FUNCTION FindFirst(PathTemplate : string; attributes : ShortWord;
                     VAR SearchResult : TSearchRec ) : BOOLEAN;
  VAR
    DirPath : string;
    Success : boolean

  BEGIN
    (* Open the directory *)

    DirPath := ExtractFilePath(PathTemplate);
    Success := OpenDir(DirPath, SearchResult.dir);
    IF Success THEN
    BEGIN
      SearchResult.sattr := attributes;
      SearchResult.match := ExtractFileName(PathTemplate);
      FindFirst          := FindNext(SearchResult)
    END
    ELSE
      FindFirst          := false
  END;

  PROCEDURE FindClose(VAR SearchResult : TSearchRec);
  VAR
    Success : boolean

  BEGIN
    (* Open the directory *)

    Success := CloseDir(SearchResult.dir)
  END;
END.
