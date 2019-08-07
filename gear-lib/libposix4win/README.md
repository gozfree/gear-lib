## libposix4win
This is a simple libposix4win library.

open cmd.exe and run:

> nmake /f Makefile.nmake

```
Microsoft (R) Program Maintenance Utility Version 10.00.30319.01
Copyright (C) Microsoft Corporation.  All rights reserved.

        cl  /Ox /W3 /c libposix4win.c
Microsoft (R) 32-bit C/C++ Optimizing Compiler Version 16.00.30319.01 for 80x86
Copyright (C) Microsoft Corporation.  All rights reserved.

libposix4win.c
libposix4win.c(182) : warning C4996: 'wcscpy': This function or variable may be
unsafe. Consider using wcscpy_s instead. To disable deprecation, use _CRT_SECUR
_NO_WARNINGS. See online help for details.
libposix4win.c(190) : warning C4996: 'wcscpy': This function or variable may be
unsafe. Consider using wcscpy_s instead. To disable deprecation, use _CRT_SECUR
_NO_WARNINGS. See online help for details.
        lib libposix4win.obj /o:libposix4win.lib
Microsoft (R) Library Manager Version 10.00.30319.01
Copyright (C) Microsoft Corporation.  All rights reserved.

LINK : warning LNK4044: unrecognized option '/o:libposix4win.lib'; ignored
        link /Dll libposix4win.obj /o:libposix4win.dll
Microsoft (R) Incremental Linker Version 10.00.30319.01
Copyright (C) Microsoft Corporation.  All rights reserved.

LINK : warning LNK4044: unrecognized option '/o:libposix4win.dll'; ignored
        cl  /Ox /W3 /c test_libposix4win.c
Microsoft (R) 32-bit C/C++ Optimizing Compiler Version 16.00.30319.01 for 80x86
Copyright (C) Microsoft Corporation.  All rights reserved.

test_libposix4win.c
        cl libposix4win.lib test_libposix4win.obj /o test_libposix4win.exe
Microsoft (R) 32-bit C/C++ Optimizing Compiler Version 16.00.30319.01 for 80x86
Copyright (C) Microsoft Corporation.  All rights reserved.

cl : Command line warning D9035 : option 'o' has been deprecated and will be re
oved in a future release
Microsoft (R) Incremental Linker Version 10.00.30319.01
Copyright (C) Microsoft Corporation.  All rights reserved.

/out:test_libposix4win.exe
/out:test_libposix4win.exe
libposix4win.lib
test_libposix4win.obj
```
