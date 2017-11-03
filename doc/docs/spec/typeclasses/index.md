# Typeclasses

Typeclasses are a feature that comes from Haskell as a means of achieving overloading of sorts. In Vapor this is different,
as the main goal to achieve with typeclasses is allowing users to customize behavior, but also in more ways than once. This
document tries to specify how they look like and how they behave.

## Syntax

The typeclass support encompasses the following keywords:

 * `typeclass`, used as a kind for the declaration of typeclasses.
 * `implicit`, which signifies that a given typeclass is - by design - to be inferred for all types, if no instances are
   defined. **Drafting node: need an opt-out mechanism.**
 * `instance`, used to declare an instance of a typeclass.
 * `default`, which marks the instance that is used when no instance is explicitly specified.
 * `with`, which will ultimately be used properly for introducing all kinds of templates, but right now its usage is
   confined to defining typeclasses, as templates are not a feature to fully land in the minimal viable product.

Similarly to structures, typeclasses can be defined in the named an unnamed forms; the named one is translated to a declaration
of an entity initialized with the unnamed form of the definition. The grammar is as follows (this duplicates some terms that
will be defined elsewhere, but not all of them, since quoting the entire grammar of expressions here would be excessive):

```
named-typeclass-declaration ::= template-introducer `implicit`? `typeclass` identifier
    `{` typeclass-function* `}` `;`
typeclass-literal ::= template-introducer `implicit`? `typeclass` `{` function-declaration* `}`

template-introducer ::= `with` `(` parameter (`,` parameter)* `)`
parameter ::= identifier `:` expression

typeclass-function ::= function-declaration | function-definition
function-declaration ::= function-signature `;`
function-definition ::= function-signature block
function-signature ::= `function` identifier `(` (parameter (`,` parameter)*)? `)` (`->` trailing-type)?
```

A definition of a default instance is itself a declaration and cannot be used as an expression. Other definitions of
typeclass instances are expressions and can be used as such.


```
default-instance-definition ::= `default` `instance` identifier `(` expression `)`
    `{` function-definition* `}` `;`
instance-literal ::= `instance` identifier `(` expression `)` `{` function-definition* `}`
```

## Semantics

This section uses names from the following piece of code to define semantics and requirements:

```
let tc = with (T : type) typeclass {        // 1
    function fndecl() -> int;               // 2

    function fndef(arg : int) -> int {      // 3
        return arg;
    }
};

default instance tc(int) {                  // 4
    function fndecl() -> int {              // 5
        return 0;
    }
};

let inst = instance tc(int) {               // 6
    function fndecl() -> int {              // 7
        return 1;
    }

    function fndef(inst_arg : int) -> int { // 8
        return arg - 1;
    }
};
```

### Basics of typeclasses and instances

 1. `tc` is a name of a typeclass. The type of `tc` is `typeclass`, which is a builtin type. All typeclasses are of type
    `typeclass`.
 2. `inst` is a name of a typeclass instance. The type of `inst` is `tc`.
 3. The type of the unnamed default instance is also `tc`.
 4. Values of type that is a typeclass can be used as a type. **Drafting note: this is somewhat unimportant for the time
    being, at least until Vapor gets templates and/or dependent types, but shall be implemented from the start. In essence,
    this allows to pass specific typeclass instances by-value by wrapping a value in them, but the exact specification of
    that doesn't matter because it won't be useful until it can be inferred or passed in in any way.**
 5. `fndecl` declared on line (2) is a declaration of a typeclass function. This function _must_ be defined by an instance
    for the instance to be valid.
 6. `fndecl`s defined on lines (5) and (7) are implementations of the function declared on line (2).
 7. `fndef` defined on line (3) is both a declaration of a typeclass function, and a default definition of such function.
    If any instance of `tc` doesn't override this implementation, it is going to be inherited from the typeclass definition.
    In other words, the default instance defined on line (4) uses this definition for its `fndef`.
 8. `fndef` defined on line (8) is a definition of the typeclass function declared on line (3) that overrides the default
    implementation. The instance `inst` uses this definition, and not the default one, as the definition of the typeclass
    function declared on line (3).

### Access to typeclass and instance members

A typeclass can be invoked with an argument to access the default instance for the given argument:

```
let default_instance = tc(int);
```

`default_instance` is of type `tc` and is equivalent to the default instance of `tc`.

When a call is made through a typeclass instance, the definition of a function belonging to that instance is looked up.

```
tc(int).fndecl()                    // returns 0, because the definition (5) is used
default_instance.fndecl()           // equivalent to the above
inst.fndecl()                       // returns 1, uses definition (7)
tc(int).fndef(123)                  // returns 123, per definition (3)
inst.fndef(123)                     // returns 122, per definition (8)
```

## Operators

Every operator in the language is mapped to a specially recognized by the core language typeclass. Those typeclasses, listed
below, are looked up when an operator is used, and the operator use is transformed into a call, according to the translation
described below.

**Drafting note: list the operator-typeclass mappings here. This is the library portion, so probably should wait with this
until the rest of the design is verified in an implementation.**
