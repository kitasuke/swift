//===--- SyntaxParsingCache.cpp - Incremental syntax parsing lookup--------===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2018 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//

#include "swift/Parse/SyntaxParsingCache.h"

using namespace swift;
using namespace swift::syntax;

bool SyntaxParsingCache::nodeCanBeReused(const Syntax &Node, size_t Position,
                                         SyntaxKind Kind) const {
  if (Node.getAbsolutePosition().getOffset() != Position)
    return false;
  if (Node.getKind() != Kind)
    return false;

  // Check if this node has been edited. If it has, we cannot reuse it.
  auto NodeStart = Node.getAbsolutePosition().getOffset();
  auto NodeEnd = NodeStart + Node.getTextLength();
  for (auto Edit : Edits) {
    if (Edit.intersectsOrTouchesRange(NodeStart, NodeEnd)) {
      return false;
    }
  }

  // FIXME: Node can also not be reused if an edit has been made in the next
  // token's leading trivia

  return true;
}

llvm::Optional<Syntax> SyntaxParsingCache::lookUpFrom(const Syntax &Node,
                                                      size_t Position,
                                                      SyntaxKind Kind) {
  if (nodeCanBeReused(Node, Position, Kind)) {
    return Node;
  }

  for (size_t I = 0, E = Node.getNumChildren(); I < E; ++I) {
    llvm::Optional<Syntax> Child = Node.getChild(I);
    if (!Child.hasValue()) {
      continue;
    }
    auto ChildStart = Child->getAbsolutePosition().getOffset();
    auto ChildEnd = ChildStart + Child->getTextLength();
    if (ChildStart <= Position && Position < ChildEnd) {
      return lookUpFrom(Child.getValue(), Position, Kind);
    }
  }
  return llvm::None;
}

llvm::Optional<Syntax> SyntaxParsingCache::lookUp(size_t NewPosition,
                                                  SyntaxKind Kind) {
  // Undo the edits in reverse order
  size_t OldPosition = NewPosition;
  for (auto I = Edits.rbegin(), E = Edits.rend(); I != E; ++I) {
    auto Edit = *I;
    if (Edit.End < OldPosition) {
      OldPosition =
          OldPosition - Edit.ReplacementLength + Edit.originalLength();
    }
  }

  auto Node = lookUpFrom(OldSyntaxTree, OldPosition, Kind);
  if (Node.hasValue()) {
    if (RecordReuseInformation) {
      auto LeadingTrivia =
          Node->getAbsolutePosition().getOffset() -
          Node->getAbsolutePositionWithLeadingTrivia().getOffset();
      auto Start = NewPosition - LeadingTrivia;
      auto End = Start + Node->getTextLength();
      ReusedRanges.push_back({Start, End});
    }
  }
  return Node;
}
