UNIT FileUtils;

INTERFACE
  USES
    Machine in 'Machine.pas'

  CONST
    RegularFile = 'R';
    SystemFile  = 'S';
    Directory   = 'D';

  TYPE
    TSearchRec = RECORD
      name  : STRING[NameSize]; (* Name of the file found *)
      attr  : CHAR;             (* The file attribute character *)
      time  : LONGWORD;         (* Time/date of last modification *)
      size  : INT64;            (* The size of the file found in bytes *)
    END;

    TDir = TMachinePointer

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
