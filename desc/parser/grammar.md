# The YMake (`ymk`) Language Specification

**Version:** 0.1.0 (Draft)
**Status:** Experimental
**Author:** DeltaY

## 1. Lexical Structure (The Lexer)

The source file is encoded in UTF-8. The tokenizer reads the stream and produces a sequence of atomic **Tokens**.

### 1.1 Comments

Comments are single-line C-style. They begin with `#` and continue to the end of the line.

```python
# This is a comment.
std: c++20 # This is also a comment.

```

### 1.2 Identifiers (The "Smart" Identifier)

YMake uses a permissive identifier rule to support flags and versions without quotes.

* **Allowed Characters:** `a-z`, `A-Z`, `0-9`, `_` (underscore), `+` (plus), `-` (minus).
* **Regex:** `[a-zA-Z0-9_+-]+`
* **Examples:**
* `project`
* `c++20`
* `-Wall`
* `user32`
* `debug`

### 1.3 Strings

Used for file paths, shell commands, or values containing special characters (spaces, wildcards).

* **Delimiter:** Double quotes `"`
* **Escape Sequences:** Standard C-style (`\"`, `\\`, `\n`).
* **Examples:**
* `"src/**/*.cpp"`
* `"C:/Program Files/..."`

### 1.4 Punctuation

The following symbols are reserved for structure:

| Symbol | Name | Usage |
| --- | --- | --- |
| `:` | Colon | Separates Keys and Values. |
| `{` | Left Brace | Starts a Block (Scope). |
| `}` | Right Brace | Ends a Block (Scope). |
| `[` | Left Bracket | Starts a List. |
| `]` | Right Bracket | Ends a List. |
| `,` | Comma | Separates items in a List. |

---

## 2. Grammar (The Parser)

The grammar is defined in Extended Backus-Naur Form (EBNF).

```ebnf
(* The Root Node *)
File        ::= { Statement } EOF

(* Statements *)
Statement   ::= Property
              | Block

(* 1. Properties (Key-Value Pairs) *)
(* Example: std: c++20 *)
Property    ::= Identifier ':' Value

(* 2. Blocks (Scoped Configuration) *)
(* Example: project: MyGame { ... } *)
Block       ::= Identifier ':' Identifier '{' { Statement } '}'

(* 3. Values *)
Value       ::= Identifier      (* simple keys, enums, flags *)
              | String          (* paths, commands *)
              | Number          (* integers *)
              | List            (* arrays *)

(* 4. Lists *)
(* Example: [ "A", "B", ] *)
List        ::= '[' [ ListItems ] ']'
ListItems   ::= Value { ',' Value } [ ',' ]

```

---

## 3. Semantic Rules

### 3.1 Data Types

The parser distinguishes between two core data types:

1. **Identifier:** Represents logical constants, enums, or simple flags (`shared`, `exe`, `c++20`).
2. **String:** Represents raw text content (`"src/main.cpp"`).

### 3.2 Scoping & Inheritance

The configuration model is hierarchical.

1. **Global Scope:** Properties defined at the root apply to **all** projects defined in the file.
2. **Project Scope:** A `project` block inherits the global scope.
3. **Specialized Scope:** Inner blocks (`conf`, `platform`) apply only when that specific condition is met.

### 3.3 Merge Strategy

When a property is defined in multiple scopes, the following rules apply during resolution:

* **Single Values (e.g., `std`):** The most specific scope **overwrites** the value.
* *Example:* If Global has `std: c++17` and Project has `std: c++20`, the result is `c++20`.


* **Lists (e.g., `defines`, `flags`):** The specific scope **appends** to the parent list.
* *Example:* If Global has `defines: [A]` and Project has `defines: [B]`, the result is `[A, B]`.



---

## 4. Canonical Example

```javascript
# Global Defaults
workspace: DoomWorkspace
c_std: c11
cpp_std: c++20

# Configuration Scope
conf: debug {
    defines: [ _DEBUG, YMAKE_DEBUG ]
    flags: [ "-g", "-O0" ]
}

# Project Definition
project: DoomEngine {
    kind: shared
    language: cpp
    
    src: [ "src/**/*.cpp" ]
    inc: [ "include" ]

    # Platform Scope
    platform: win {
        links: [ user32, gdi32 ]
        defines: [ YM_PLATFORM_WIN ]
    }
}

```