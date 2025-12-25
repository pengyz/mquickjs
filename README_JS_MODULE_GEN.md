# JS Module to C Structure Generator

## Overview

The `mqjs_js_module_gen` tool allows you to convert arbitrary JavaScript files into static C structures that can be embedded directly into your MQuickJS application. This is particularly useful for:

- Reducing memory usage in embedded systems
- Improving startup time by avoiding runtime parsing
- Storing JS code in ROM instead of RAM
- Creating self-contained applications

## Usage

### Using Make

```bash
# Build the tool
make mqjs_js_module_gen

# Convert a JS file to C header
./mqjs_js_module_gen <module_name> <js_file> > <output_header_file>

# Example:
./mqjs_js_module_gen my_module my_module.js > my_module_gen.h
```

### Using CMake

```bash
# Build the tool
cd build && cmake .. && make mqjs_js_module_gen

# Convert a JS file to C header
./tools/mqjs_js_module_gen <module_name> <js_file> > <output_header_file>

# Example:
./build/tools/mqjs_js_module_gen my_module my_module.js > my_module_gen.h
```

## How It Works

The tool:

1. Reads your JavaScript file
2. Extracts all identifiers and string literals as atoms
3. Creates a static C structure containing these atoms
4. Generates a `JSSTDLibraryDef` structure that can be used with MQuickJS contexts

## Example

Given a JavaScript file `math_utils.js`:

```javascript
function add(a, b) {
    return a + b;
}

function multiply(a, b) {
    return a * b;
}

var constants = {
    PI: 3.14159,
    E: 2.71828
};
```

You can convert it to a C header:

```bash
./mqjs_js_module_gen math_utils math_utils.js > math_utils_gen.h
```

Then use it in your C code:

```c
#include "math_utils_gen.h"  // Contains the js_math_utils structure

// Create a context with the precompiled module
JSContext *ctx = JS_NewContext(mem_buffer, mem_size, &js_math_utils);

// The functions and variables are now available in the context
```

## Memory Benefits

- No need to store JavaScript source code in RAM
- Atoms are stored in ROM and referenced directly
- Faster initialization since parsing is not needed
- Reduced garbage collection pressure

## Limitations

- The generated module only contains atoms (identifiers, strings)
- Actual JS code still needs to be loaded separately unless also precompiled
- Currently only supports basic tokenization of identifiers

## Integration with Applications

To use the generated modules in your application:

1. Convert your JS files to headers using the tool
2. Include the headers in your C source files
3. Use the generated `JSSTDLibraryDef` structures when creating contexts
4. Optionally load the actual JS code using the pre-populated atom table