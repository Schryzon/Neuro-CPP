<p align="center">
  <img src="img/Neuro-Banner-1MB.png" alt="description" style="width:60%;">
</p>

<table>
<tr>
<td><img src="img/Neuro-Erased.png" width="40"></td>
<td><h1>Neuro-CPP (No <vector> library)</h1></td>
</tr>
</table>

A TUI (Terminal User Interface) Gemini-based AI chatbot project made in C++.

This project is created as a requirement to complete the University of Mataram's Algorithms and Programming 2025 practicum.

This version is created without the use of `<vector>` standard library to make it stand out even more!

Licensed under the **MIT License** *because we love open-source!*

## Libraries Used
| Category        | Library         | Description and/or Necessities                 |
|-----------------|-----------------|------------------------------------------------|
| **STL**         |                 | Built-in C++ standard library components       |
| (Standard)      | `<iostream>`    | Input/output stream (STDIN & STDOUT)           |
|                 | `<sstream>`     | String streams for receiving HTTP responses    |
|                 | `<conio.h>`     | Keyboard-based menu navigations                |
|                 | `<iomanip>`     | To adjust UI elements to our liking            |
| **Third-party** |                 | Custom-made libraries installed through `vcpkg`|
|    (Custom)     | `cURLpp`        | C++ wrapper for `libcurl` for HTTP requests    |
|                 | `nlohmann-json` | JSON parsing and serialization library         |
|                 | `dotenv`        | Secret environment variables                   |

## Required `.env` Contents
1. `GEMINI_API_KEY` (API key for the Neuro chatbot)
2. `TITLE_KEY` (Optional, for the title maker - we included this to not get rate-limited)
3. `DEFAULT_PERSONALITY` (Default personality for Neuro)
4. `TITLE_PERSONALITY` (System instructions for the title maker)
5. `AI_BASE_URL` (Base URL for HTTP request to Gemini AI's API, currently using 2.0 flash model)

## Compile and Build Instructions
To build Neuro-CPP on Windows (with vcpkg), run:

```bash
mkdir build
cd build
cmake .. ^
    -DCMAKE_TOOLCHAIN_FILE="<PATH_TO_VCPKG>\scripts\buildsystems\vcpkg.cmake" ^
    -DCURL_INCLUDE_DIR="<PATH_TO_VCPKG>\installed\x64-windows\include" ^
    -DCURL_LIBRARY="<PATH_TO_VCPKG>\installed\x64-windows\lib\libcurl.lib" ^
    -DCMAKE_POLICY_VERSION_MINIMUM="<MINIMUM_ALLOWED_CMAKE_VERSION>"
```
Then, after doing some coding, perform

```bash
cmake --build . --config Release
```
to compile the program. It shall reside within `build/Release/` along with the installed libraries.

*Do note that `dotenv.h` might not be able to read your .env inside said directory.*

You may configure the following files in your `.vscode/` directory to create a debugger:
1. `launch.json`
2. `tasks.json`

## Contributors (Group 01)
- I Nyoman Widiyasa Jayananda ([Schryzon](https://github.com/Schryzon)) - F1D02410053
- I Kadek Mahesa Permana Putra ([Vuxyn](https://github.com/Vuxyn)) - F1D02410052
- Samara Wardasadiya ([samarawards](https://github.com/samarawards)) - F1D02410023

## Current Status
- ✅ | Approved by assistant YNK and Senior Kholilluloh.
- ✅ | Done, ready for improvement!