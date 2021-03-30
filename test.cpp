//
// Created by petr on 3/26/21.
//

#include "BinaryEncoder.h"
#include "Tree.h"
#include "utils.h"
#include <iostream>
using namespace pf::kko;

template<std::integral T>
void swapNodes(std::observer_ptr<Node<T>> first, std::observer_ptr<Node<T>> second) {
  if (first->isRoot() || second->isRoot()) { return; }
  if (first->getParent() == second || second->getParent() == first) { return; }

  const auto isFirstLeftChild = first->isLeftChild();
  auto &firstParent = first->getParent();
  auto firstUnique = isFirstLeftChild ? firstParent->releaseLeft() : firstParent->releaseRight();
  const auto isSecondLeftChild = second->isLeftChild();
  auto &secondParent = second->getParent();
  auto secondUnique = isSecondLeftChild ? secondParent->releaseLeft() : secondParent->releaseRight();

  if (isFirstLeftChild) {
    firstParent->setLeft(std::move(secondUnique));
  } else {
    firstParent->setRight(std::move(secondUnique));
  }
  if (isSecondLeftChild) {
    secondParent->setLeft(std::move(firstUnique));
  } else {
    secondParent->setRight(std::move(firstUnique));
  }
}

int main() {
  Tree<int> tree(0);
  auto &root = tree.getRoot();
  auto &left = root.makeLeft(1);
  left.makeLeft(11);
  left.makeRight(12);
  auto &right = root.makeRight(2);

  right.makeLeft(21);
  right.makeRight(22);

  swapNodes(std::make_observer(&left), std::make_observer(&right));
  return 0;
}