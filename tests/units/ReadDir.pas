PROGRAM TestReadDir;

USES
  FileUtils IN 'FileUtils.pas'

VAR
  SearchResult : TSearchRec;
  DirEntry : TDir;
  DirPath : STRING[40];
  FilePath : STRING[256];
  Result : BOOLEAN

BEGIN
  DirPath := 'src/';
  Result := OpenDir(DirPath, DirEntry);
  IF NOT Result THEN
    WriteLn('ERROR: Failed to open ', DirPath)
  ELSE
    WHILE ReadDir(DirEntry, SearchResult) DO
    BEGIN
      WriteLn('- Name: ', SearchResult.name);
      WriteLn('  Attr: ', SearchResult.attr);

      FilePath := DirPath + SearchResult.name;
      IF FileInfo(FilePath, SearchResult) THEN
      BEGIN
        WriteLn('  Time: ', SearchResult.time);
        WriteLn('  size: ', SearchResult.size)
      END
      ELSE
        WriteLn('ERROR:  FileInfo failed ', FilePath)
    END;

  Result := CloseDir(DirEntry);
  IF NOT Result THEN
    WriteLn('ERROR:  Failed to close ', DirPath)
END.
