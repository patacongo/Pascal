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

      dir   : TDir;             (* Used internally *)
      sattr : ShortWord         (* Used internally *)
    END

  FUNCTION  ExtractFilePath (fullPath : string ) : string;
  FUNCTION  FindFirst(pathTemplate : string; attributes : ShortWord;
                      VAR searchResult : TSearchRec ) : BOOLEAN;
  FUNCTION  FindNext(VAR searchResult : TSearchRec) : BOOLEAN;
  PROCEDURE FindClose(VAR searchResult : TSearchRec);

IMPLEMENTATION

  FUNCTION ExtractFilePath (fullPath : string ) : string;
  VAR
    lastPosition : integer;
    nextPosition : integer

  BEGIN
    (* Loop to find position of last delimiter *)

    nextPosition   := 0;
    REPEAT
      lastPosition := nextPosition;
      nextPosition := POS(fullPath, DirSeparator, lastPosition + 1);
    UNTIL nextPosition = 0;

    { The directory path is then the substring from the beginning the
      string to the position just before the delimiter }

    ExtractFilePath := Copy(fullPath, 1, lastPosition - 1);
  END;

  FUNCTION FindFirst(pathTemplate : string; attributes : ShortWord;
                     VAR searchResult : TSearchRec ) : BOOLEAN;
  VAR
    dirPath : string;
    success : boolean

  BEGIN
    (* Open the directory *)

    dirPath := ExtractFilePath(pathTemplate);
    success := OpenDir(dirPath, searchResult.dir);
    IF success THEN
    BEGIN
      searchResult.sattr := attributes;
      FindFirst := FindNext(searchResult)
    END
    ELSE
      FindFirst := false
  END;

  FUNCTION FindNext(VAR searchResult : TSearchRec) : BOOLEAN;
  VAR
    firstChar : string;
    success : boolean;
    found : boolean

  BEGIN
    (* Read from the directory until a file is found or until we have read
       all of the directory entries. *)

    found := false;
    REPEAT
      success := ReadDir(searchResult.dir, searchResult);

      (* Visible, regular files always accepted, directories, system files,
         and hidden files are only reported if so requested. *)

       firstChar := Copy(searchResult.name, 1, 1);
       IF success AND Length(firstChar) > 0 THEN
       BEGIN
         IF firstChar = '.' AND (searchResult.sattr & faHidden) <> 0 THEN
           found := true
         ELSE IF (searchResult.attr & searchResult.sattr & faSysFile) <> 0 THEN
           found := true
         ELSE IF (searchResult.attr & searchResult.sattr & faDirectory) <> 0 THEN
           found := true
         ELSE IF (searchResult.attr & searchResult.sattr & faVolumeId) <> 0 THEN
           found := true;
       END
    WHILE NOT found AND success

    FindNext := found
  END;

  PROCEDURE FindClose(VAR searchResult : TSearchRec);
  VAR
    success

  BEGIN
    (* Open the directory *)

    success := CloseDir(searchResult.dir)
  END
END.
