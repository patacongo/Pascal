UNIT FileUtils;

INTERFACE
  CONST
    RegularFile = 'R';
    SystemFile  = 'S';
    Directory   = 'D';

  TYPE
    TSearchRec = RECORD
      name      : STRING;  (* Name of the file found *)
      attribute : CHAR;    (* The file attribute character *)
      size      : INT64    (* The size of the file in bytes *)
    END;

  FUNCTION FindFirst(fileTemplate, attributes : string; VAR searchResult : TSearchRec ) : BOOLEAN;
  FUNCTION FindNext(VAR searchResults : TSearchRec) : BOOLEAN;
  PROCEDURE FindClose(VAR searchResults : TSearchRec);

IMPLEMENTATION

  FUNCTION FindFirst(fileTemplate, attributes : string; VAR searchResult : TSearchRec ) : BOOLEAN;
  BEGIN
    FindFirst := FALSE
  END;

  FUNCTION FindNext(VAR searchResults : TSearchRec) : BOOLEAN;
  BEGIN
    FindNext := FALSE
  END;

  PROCEDURE FindClose(VAR searchResults : TSearchRec);
  BEGIN
  END;

END.
