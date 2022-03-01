PROGRAM TestReadDir;

USES
  FileUtils IN 'FileUtils.pas'

VAR
  searchResult : TSearchRec;
  dirEntry : TDir;
  dirName : STRING[40];
  result : BOOLEAN

BEGIN
  dirName := 'src';
  result := OpenDir(dirName, dirEntry);
  IF NOT result THEN
    WriteLn('ERROR: Failed to open ', dirName)
  ELSE
    WHILE readDir(dirEntry, searchResult) DO
    BEGIN
      WriteLn('- Name: ', searchResult.name);
      WriteLn('  Attr: ', searchResult.attr);
      WriteLn('  Time: ', searchResult.time);
      WriteLn('  size: ', searchResult.size)
    END;

  result := CloseDir(dirEntry);
  IF NOT result THEN
    WriteLn('ERROR:  Failed to close ', dirName)
END.
