# GTempFileManagerLib

A small, single-header C++17 utility that creates temporary files and directories in the system's temporary directory and removes them automatically when it goes out of scope. Very simple and basic, hopefully useful.

No dependencies beyond the standard library.

> **Project status:** this library is at version 0.0.1, an early release. It
> is still being tested and will be further developed, so its API may change
> in future versions. Feedback and bug reports are welcome.
>
> This library has been tested only on Windows and compiled with MSVC 2022 so far

## Features

- **Automatic cleanup**: every file and directory created through the manager
  is removed when the manager is destroyed (RAII), directories together with
  their contents.
- **Readable names**: names are built from your own prefix, optionally
  followed by a timestamp, so temporary files are easy to recognize.
- **Never overwrites anything**: if a name is already taken, a numeric suffix
  (`_1`, `_2`, ...) is appended. The manager never overwrites, and never
  deletes, anything it did not create itself.
- **Clear ownership**: the manager can be moved but not copied, so each
  temporary resource is removed exactly once.
- **Header-only**: copy one file into your project and include it.

## Getting started

Copy `GTempFileManagerLib.h` into your project and include it:

```cpp
#include "GTempFileManagerLib.h"
```

Requires C++17 (`std::filesystem`). Everything is in the
`gtempfilemanagerlib` namespace.

## Example

```cpp
#include "GTempFileManagerLib.h"
#include <fstream>
#include <iostream>

int main()
{
    gtempfilemanagerlib::CGTempFileManager temp;

    // e.g. /tmp/export_1763380000.csv (Linux) or %TEMP%\export_1763380000.csv (Windows)
    std::filesystem::path csv = temp.CreateTempFile("export", ".csv");
    std::ofstream(csv) << "x,y\n1,2\n";

    // e.g. /tmp/work_1763380000
    std::filesystem::path work_dir = temp.CreateTempDir("work");
    std::ofstream(work_dir / "partial.dat") << "...";

    std::cout << "Temporary file: " << csv << '\n';

    // Both the file and the directory (with its contents) are removed
    // automatically when 'temp' goes out of scope.
}
```

## API overview

| Member                                          | Description                                                                                            |
| ----------------------------------------------- | ------------------------------------------------------------------------------------------------------ |
| `CreateTempFile(prefix, ext, timestamp = true)` | Creates an empty file named `prefix[_timestamp]ext` in the temp directory and returns its full path.   |
| `CreateTempDir(prefix, timestamp = true)`       | Creates a directory named `prefix[_timestamp]` in the temp directory and returns its full path.        |
| `~CGTempFileManager()`                          | Removes all files and directories created by the manager; directories are removed with their contents. |
| Move constructor / assignment                   | Transfer ownership of the tracked resources to another manager.                                        |

Both creation functions throw if the resource cannot be created:
`std::filesystem::filesystem_error` if the temp directory is not available or
a directory cannot be created, `std::runtime_error` if a file cannot be
created.

## Naming

Names are built as:

    <prefix>_<timestamp><extension>     (with timestamp, the default)
    <prefix><extension>                 (without timestamp)

The timestamp is the current time in seconds since the epoch. If the
resulting name already exists, a numeric suffix is added before the
extension:

    export_1763380000.csv
    export_1763380000_1.csv
    export_1763380000_2.csv

So, even without a timestamp, you may get `report_1.txt` instead of
`report.txt` if `report.txt` already exists in the temp directory.

## Notes

- **Cleanup errors are ignored**: if a file is still open or locked (common
  on Windows) when the manager is destroyed, it is left in place silently.
  Close your streams before the manager goes out of scope.
- **Ownership**: anything you put inside a directory created by the manager is
  removed along with it.
- **Not a security feature**: names are predictable, and the files are created
  with default permissions. For sensitive data or security-critical code, use
  your platform's secure temporary-file facilities instead.
- **Not thread-safe**: use one manager per thread, or protect a shared manager
  with a mutex.

## License

Released under the [Boost Software License 1.0](LICENSE).

SPDX: `BSL-1.0`
