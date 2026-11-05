# Third-party notices

This repository vendors four third-party libraries. They are kept in source
form under `langTextExtra/` and are **not** covered by the top-level MIT
`LICENSE`. Each retains its original copyright and license.

| Component | Location | License | License text |
| --- | --- | --- | --- |
| DuiLib | `langTextExtra/DuiLib/` | BSD-style / public-domain style notice used by the upstream DuiLib project | See the header comments in `langTextExtra/DuiLib/UIlib.h` and the individual source files. Upstream: <https://github.com/duilib/duilib> |
| libxlsxwriter | `langTextExtra/libxlsxwriter/` | FreeBSD (2-clause BSD) | `langTextExtra/libxlsxwriter/License.txt` (Copyright 2014-2024 John McNamara) |
| zlib 1.3.1 | `langTextExtra/zlib-1.3.1/` | zlib license | `langTextExtra/zlib-1.3.1/LICENSE` (Copyright Jean-loup Gailly and Mark Adler) |
| TinyXML | `langTextExtra/tinyxml/` | zlib-style license | License grant at the top of `langTextExtra/tinyxml/tinyxml.h` (Copyright 2000-2006 Lee Thomason) |

`libxlsxwriter` additionally bundles `third_party/minizip`; see
`langTextExtra/libxlsxwriter/third_party/minizip/README.txt`.

## Local modifications

Only two vendored build files were changed, and neither changes library
behaviour:

- `langTextExtra/libxlsxwriter/CMakeLists.txt`
  - removed the developer-specific absolute path `C:\Users\dnake\Desktop\...`;
  - prefer the vendored `zlibstatic` target instead of a system zlib;
  - raised `cmake_minimum_required` from `3.1/3.4` to `3.16`, because CMake 4
    rejects compatibility with versions older than 3.5.

No vendored header or implementation file was modified.

If you are redistributing a binary built from this repository, review the
license terms above and include the corresponding notices.
