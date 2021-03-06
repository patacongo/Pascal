PROGRAM DirOperations;

VAR
  currDir, newDir, prevDir : STRING;
  result : BOOLEAN;

BEGIN
  currDir := GetDir;
  WriteLn('Current Working Directory: ', currDir);

  newDir := currDir + '/newdir';
  WriteLn('Create and change to ', newdir);

  result  := CreateDir(newDir);
  IF result = false THEN
    WriteLn('FAILED to create directory ', newDir);

  prevDir := currDir;
  result  := SetCurrentDir(newDir);
  IF result = false THEN
    WriteLn('FAILED to set current working directory to ', newDir);

  currDir := GetDir;
  WriteLn('Current Working Directory: ', currDir);

  WriteLn('Go back to ', prevDir);
  result  := SetCurrentDir(prevDir);
  IF result = false THEN
    WriteLn('FAILED to set current working directory to ', prevDir);

  currDir := GetDir;
  WriteLn('Current Working Directory: ', currDir);

  WriteLn('Remove ', newDir);
  result  := RemoveDir(newDir);
  IF result = false THEN
    WriteLn('FAILED to remove directory ', newDir);

  currDir := GetDir;
  WriteLn('Current Working Directory: ', currDir);

  result  := SetCurrentDir(newDir);
  IF result = true THEN
    WriteLn('FAILED: Can still set working directory to ', newDir)
  ELSE
    WriteLn('OK -- Cannot to set directory to removed ', newDir);

  currDir := GetDir;
  WriteLn('Current Working Directory: ', currDir)

END.
