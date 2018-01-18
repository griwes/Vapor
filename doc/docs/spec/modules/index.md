# Modules

Modules provide a way to segregate code into separate units to separate it logically, and also physically - into individual
files. In Vapor, modules are tightly coupled to file paths, but the language still allows for defining multiple modules in
the same file, provided they are submodules of the module implied by the file name.

## Syntax

The modules support encompasses the following keywords:

 * `module`, used to introduce a module definition
 * `import`, found in import declarations and expressions, to actually bring definitions from a different module into the
current one
 * `export`, to make a name defined within a module accessible by whoever imports it

A module definition is a special kind of a statement, that creates a new module, with a given fully qualified name. That name
has to match the current filename (unless its name is `main` and the input file is standard input).

```
module-definition ::= `module` id-expression `{` declaration-statement* `}`
declaration-statement ::= `export`? (declaration | function-definition)
id-expression ::= identifier (`.` identifier)*
```

An import expression is an expression, whose result is an object representing the imported module, and which provides access
to all entities exported from that module. An import statement is a syntactic form that declares a variable, that can be
accessed with the same qualified expression as the module name, into the current scope; the value of the qualified expression
is the result of an implied import expression.

```
import-expression ::= `import` id-expression
import-statement ::= import-expression `;`
```

Import statements and module definitions are the only statements that can appear as top level statements in a Vapor source
file. All `import` statements must appear before any `module` statement. A file that defines no module is ill-formed.

## Semantics

This section assumes that the following files exist somewhere in the compiler's module search path, and that they have already
been compiled into appropriate module interface files. Function definitions are omitted for brevity.

**Drafting note:** make the functions fully defined once tuples are a thing (including empty tuples for "void").

```
// file: foo.vpr
module foo
{
    export int get_value() { /* ... */ }
    int get_other() { /* ... */ }
}
```

```
// file: bar/module.vpr
module bar
{
    export let int32 = sized_int(32);

    export let fizz = import bar.fizz; // X
}

module bar.baz
{
    export let int64 = sized_int(64);
}
```

```
// file: bar/fizz.vpr
module bar.fizz
{
    export let int16 = sized_int(16);
}
```

These files define a total of four modules: two top level modules, `foo` and `bar`, and two submodules of `bar`: `bar.baz`
and `bar.fizz`.

`foo` is a single file module. This means it can be defined in a file called `foo.vpr`, but it could also be defined
similarly to `bar`: inside a file `foo/module.vpr`.

`bar` is a multiple file module. This means that it is required that the exists a directory named `bar`, and within it a
file called `module.vpr`, which is used as the primary definition of the module, and any number of files and/or directories,
that define submodules of `bar`. Since there exists a directory called `bar`, they may not exist a file called `bar.vpr`
in the compiler's module search path.

`bar/module.vpr` defines the primary module `bar`, but also defines a submodule `bar.baz`. This is allowed only if a file
specific to `bar.baz`, i.e. `bar/baz.vpr` (or a directory, i.e. `bar/baz/`) doesn't exist. If a file specific to a submodule
name exists, the submodule shall be defined there.

Line marked `X` reexports the submodule as a direct member of the module `bar`. It will be visible when importing module
`bar`, without the need to import that submodule manually.

Now consider also this file, that is being compiled by the implementation:

```
import foo; // A
import bar; // B

module main
{
    let baz = import bar.baz; // C

    function entry(arg : bar.int32) -> bar.int32
    {
        let a = foo.get_value(); // D
        let b = foo.get_other(); // E
    }
}
```

 1. Line A imports the module foo. The compiler looks for a module definition for this module in either foo.vpr directly
 within one of the module search directories, or for module.vpr in directory foo within one of the module search directories.
 The program is ill-formed due to environment reasons when this search finds more than a single definition across all
 module search directories. This line happens to find foo.vpr. It's an import statement, which means it creates an object
 in the global scope, with the name `foo`, whose value is a module object of `foo`, containing all members of the original
 module.

 2. Line B imports the module bar. This line happens to find `bar/module.vpr`. This file also defines a submodule `bar.baz`,
 which will also be imported without the need to export it manually, like `bar.fizz`, on line X.

 3. Line C uses an import expression to import bar.baz. The value of that expression is exactly identical to `bar.baz`.
 Note that the id-expression in an import expression is not resolved using the current scope. Any reference using `baz.`
 from this line until the end of the scope refers to members of `bar.baz`.

 4. Line D attempts to invoke a function from module `foo`. This call succeeds, since the function is exported from `foo`.

 5. Line E attempts to invoke another function from module `foo`. This call fails, because the function is not exported
 from `foo`.

## Special modules

### Master module

The master module, or "global" module, is the module where builtin types and symbols reside. It is special, because it is
defined prior to analyzing the file being compiled, and because the names in this module cannot be overwritten by other
modules. Names currently defined within the master module:

 * `int`
 * `bool`
 * `type`
 * `sized_int`

### Main module

The `main` module is used to identify where the entry point of the program is defined. The `main` module should
normally be defined in a file called `main.vpr`, but it is the single module that can be consumed from standard input.
