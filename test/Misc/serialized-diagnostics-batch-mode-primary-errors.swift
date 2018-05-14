// Ensure that an error in a primary causes an error in the errorless primary.
//
// RUN: rm -f %t.*

// RUN: not %target-swift-frontend -typecheck  -primary-file %s  -serialize-diagnostics-path %t.main.dia -primary-file %S/../Inputs/empty.swift  -serialize-diagnostics-path %t.empty.dia  2> %t.stderr.txt
// RUN: c-index-test -read-diagnostics %t.main.dia 2> %t.main.txt
// RUN: c-index-test -read-diagnostics %t.empty.dia 2> %t.empty.txt

// RUN: %FileCheck -check-prefix=ERROR %s <%t.main.txt
// RUN: %FileCheck -check-prefix=NO-NONSPECIFIC-ERROR %s <%t.main.txt
// RUN: %FileCheck -check-prefix=NONSPECIFIC-ERROR %s <%t.empty.txt

// ERROR: error:
// NONSPECIFIC-ERROR: error: error(s) in other file(s) halted compilation
// NO-NONSPECIFIC-ERROR-NOT: error: error(s) in other file(s) halted compilation

func test(x: SomeType) {
  nonexistent()
}

