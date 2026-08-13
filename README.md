# Custom LLVM IR Code Generation Backend

A lightweight, high-performance C++ code generation layer built directly on the **LLVM API**. This project provides a context-aware Abstract Syntax Tree (AST) framework capable of converting programmatic AST constructs into valid, SSA-compliant **LLVM Intermediate Representation (IR)** with complex control flow graphs (CFGs).

By decoupling IR generation from frontend parsing, this backend allows for direct programmatic AST construction, making it ideal for verifying LLVM control flow, loop target stacks, and block termination logic before attaching a lexer or parser.

---

## Key Architectural Features

* **SSA-Compliant Control Flow Graphs (CFG):** Programmatically constructs clean basic blocks (`entry`, `body`, `condition`, `increment`, `then`, `else`, `merge`, `exit`) for complex branching and iterative structures.
* **LIFO Loop Target Resolution:** Implements a context-aware target stack (`exitBlocks` and `continueBlocks`) to dynamically map `break` and `continue` semantics across arbitrarily deep loop nesting levels without modifying standard `virtual Node::codegen` signatures.
* **Defensive Basic Block Termination:** Employs active basic block inspection (`getTerminator()`) across conditional branches and loop bodies to prevent unterminated basic blocks and eliminate LLVM double-terminator assertions.
* **Control Flow Coverage:** Native support for `if`/`else` conditionals, `while`, `do-while`, and `for` loops, binary/comparison operations, and loop jump instructions.
* **Frontend-Agnostic Backend:** Designed as an independent IR generation bed. You can manually construct AST node hierarchies in C++ to verify backend codegen without needing a frontend lexer or parser.

---

## Supported AST Constructs

| AST Node | Target LLVM IR Mechanism |
| :--- | :--- |
| **`IfNode`** | Conditional branching (`br i1 ...`), basic block splitting, and merge block joining. |
| **`whileNode`** / **`doWhileLoopNode`** | Iterative loop CFG with conditional back-edges and exit branches. |
| **`forLoopNode`** | 4-block loop topology (`body` $\rightarrow$ `increment` $\rightarrow$ `condition` $\rightarrow$ `exit`). |
| **`breakNode`** | Unconditional jump (`br`) targeting the active loop's `exitBlock` via stack peeking. |
| **`continueNode`** | Unconditional jump (`br`) targeting the active loop's `continueBlock` (`condition` or `increment`). |

---

## Prerequisites

* **C++ Compiler:** Modern C++ compiler supporting C++20 or later (GCC, Clang, or MSVC).
* **LLVM Toolkit:** LLVM 15+ (headers and libraries).
* **Build System:** CMake 3.20+ or Ninja.

---

## Build Instructions

### 1. Clone the Repository
```bash
git clone [https://github.com/your-username/llvm-ir-codegen-backend.git](https://github.com/your-username/llvm-ir-codegen-backend.git)
cd llvm-ir-codegen-backend
