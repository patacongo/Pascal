PROGRAM TestReadDir;

USES
  FileUtils IN 'FileUtils.pas'

VAR
  SearchResult : TSearchRec;
  DirEntry : TDir;
  DirPath : STRING[40];
  Result : BOOLEAN

  PROCEDURE ShowFileInfo;
  VAR
    FilePath : STRING[256];

  BEGIN
    FilePath := DirPath + SearchResult.name;
    IF FileInfo(FilePath, SearchResult) THEN
    BEGIN
      WriteLn('  Time: ', SearchResult.time);
      WriteLn('  size: ', SearchResult.size)
    END
    ELSE
      WriteLn('ERROR:  FileInfo failed ', FilePath)
  END;

  FUNCTION ShowDirEntry : boolean;
  BEGIN
    ShowDirEntry := ReadDir(DirEntry, SearchResult);
    IF ShowDirEntry THEN
    BEGIN
      WriteLn('- Name: ', SearchResult.name);
      WriteLn('  Attr: ', SearchResult.attr)
    END
  END;

BEGIN
  DirPath := 'src/';
  Result := OpenDir(DirPath, DirEntry);
  IF NOT Result THEN
    WriteLn('ERROR: Failed to open ', DirPath)
  ELSE
    WHILE ShowDirEntry DO
      ShowFileInfo;

  Result := CloseDir(DirEntry);
  IF NOT Result THEN
    WriteLn('ERROR:  Failed to close ', DirPath)
END.
