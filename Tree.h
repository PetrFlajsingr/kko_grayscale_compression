//
// Created by petr on 3/23/21.
//

#ifndef HUFF_CODEC__TREE_H
#define HUFF_CODEC__TREE_H

#include "utils.h"
#include <experimental/memory>
#include <cassert>

namespace pf::kko {
/**
 * Binary tree node with parent pointer.
 * @tparam T stored type
 */
template<typename T>
class Node {
 public:
  using value_type = T;
  using reference = T &;
  using const_reference = const T &;
  using pointer = T *;
  using const_pointer = const T *;
  explicit Node(value_type initValue, std::experimental::observer_ptr<Node> parent = nullptr)
      : value(initValue), parent(parent) {}

  [[nodiscard]] reference operator*() { return value; }
  [[nodiscard]] const_reference operator*() const { return value; }

  [[nodiscard]] pointer operator->() { return &value; }
  [[nodiscard]] const_pointer operator->() const { return &value; }

  [[nodiscard]] const_reference getValue() const { return value; }
  [[nodiscard]] reference getValue() { return value; }
  void setValue(T newValue) { value = newValue; }

  [[nodiscard]] bool hasLeft() const { return left != nullptr; }
  [[nodiscard]] Node &getLeft() const { return *left; }
  void setLeft(std::unique_ptr<Node> &&newLeft) {
    newLeft->parent = std::experimental::make_observer(this);
    left = std::move(newLeft);
  }
  Node &makeLeft(value_type initValue) {
    left = std::make_unique<Node>(initValue, std::experimental::make_observer(this));
    return *left;
  }
  std::unique_ptr<Node> releaseLeft() { return std::move(left); }

  [[nodiscard]] Node &getRight() const { return *right; }
  void setRight(std::unique_ptr<Node> &&newRight) {
    newRight->parent = std::experimental::make_observer(this);
    right = std::move(newRight);
  }
  Node &makeRight(value_type initValue) {
    right = std::make_unique<Node>(initValue, std::experimental::make_observer(this));
    return *right;
  }
  std::unique_ptr<Node> releaseRight() { return std::move(right); }
  [[nodiscard]] bool hasRight() const { return right != nullptr; }
  [[nodiscard]] const std::experimental::observer_ptr<Node> &getParent() const { return parent; }

  [[nodiscard]] bool isLeaf() const { return left == nullptr && right == nullptr; }
  [[nodiscard]] bool isRoot() const { return parent == nullptr; }
  [[nodiscard]] bool isChild() const { return !isRoot(); }
  [[nodiscard]] bool isLeftChild() const { return parent->hasLeft() && &parent->getLeft() == this; }
  [[nodiscard]] bool isRightChild() const { return parent->hasRight() && &parent->getRight() == this; }
  [[nodiscard]] bool hasChildren() const { return hasRight() || hasLeft(); }

  bool operator<(const Node &rhs) const requires std::totally_ordered<T> { return value < rhs.value; }
  bool operator>(const Node &rhs) const requires std::totally_ordered<T> { return rhs < *this; }
  bool operator<=(const Node &rhs) const requires std::totally_ordered<T> { return !(rhs < *this); }
  bool operator>=(const Node &rhs) const requires std::totally_ordered<T> { return !(*this < rhs); }

  bool operator==(const Node &rhs) const requires std::equality_comparable<T> { return value == rhs.value; }
  bool operator!=(const Node &rhs) const requires std::equality_comparable<T> { return !(rhs == *this); }

 private:
  T value;
  std::unique_ptr<Node> left = nullptr;
  std::unique_ptr<Node> right = nullptr;
  std::experimental::observer_ptr<Node> parent;
};

template<typename T>
class Tree {
 public:
  Tree() = default;
  explicit Tree(typename Node<T>::value_type initValue) : root(std::make_unique<Node<T>>(initValue)) {}
  Node<T> &getRoot() const { return *root; }
  void setRoot(std::unique_ptr<Node<T>> &&node) { root = std::move(node); }
  std::experimental::observer_ptr<Node<T>> makeRoot(typename Node<T>::value_type initValue) {
    root = std::make_unique<Node<T>>(initValue);
    getRoot();
  }

 private:
  std::unique_ptr<Node<T>> root = nullptr;
};

template<typename T>
std::unique_ptr<Node<T>> makeUniqueNode(T &&value, std::experimental::observer_ptr<Node<T>> parent = nullptr) {
  if constexpr (std::same_as<std::decay_t<T>, Node<T>>) {
    return std::make_unique<Node<T>>(std::forward<T>(value));
  } else {
    return std::make_unique<Node<T>>(std::forward<T>(value), parent);
  }
}

template<typename T>
void depthFirst(const Node<T> &node, std::invocable<const Node<T> &> auto callable) {
  callable(node);
  if (node.hasLeft()) { depthFirst(node.getLeft(), callable); }
  if (node.hasRight()) { depthFirst(node.getRight(), callable); }
}

template<typename T>
std::size_t countNodes(const Node<T> &node) {
  auto result = std::size_t{};
  depthFirst(node, [&result](const auto &) { ++result; });
  return result;
}

template<typename T>
std::optional<std::observer_ptr<Node<T>>> findNode(Node<T> &node, std::predicate<Node<T>> auto pred) {
  if (pred(node)) { return node; }
  auto result = std::optional<std::observer_ptr<Node<T>>>{};
  if (node.hasLeft()) { result = findNode(node.getLeft(), pred); }
  if (!result.has_value() && node.hasRight()) { result = findNode(node.getRight(), pred); }
  return result;
}

template<typename T>
void swapNodes(std::observer_ptr<Node<T>> first, std::observer_ptr<Node<T>> second) {
  assert(first != nullptr);
  assert(second != nullptr);
  if (first->isRoot() || second->isRoot()) { return; }
  if (first->getParent() == second || second->getParent() == first) { return; }

  const auto isFirstLeftChild = first->isLeftChild();
  auto firstParent = first->getParent();
  auto firstUnique = isFirstLeftChild ? firstParent->releaseLeft() : firstParent->releaseRight();

  const auto isSecondLeftChild = second->isLeftChild();
  auto secondParent = second->getParent();
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
}// namespace pf::kko

#endif//HUFF_CODEC__TREE_H
