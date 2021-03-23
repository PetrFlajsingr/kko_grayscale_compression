//
// Created by petr on 3/23/21.
//

#ifndef HUFF_CODEC__TREE_H
#define HUFF_CODEC__TREE_H

#include <experimental/memory>

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

  [[nodiscard]] std::experimental::observer_ptr<Node> getLeft() const { return left; }
  void setLeft(std::unique_ptr<Node> &&newLeft) {
    newLeft->parent = this;
    left = std::move(newLeft);
  }
  std::experimental::observer_ptr<Node> makeLeft(value_type initValue) {
    left = std::make_unique<Node>(initValue, std::experimental::make_observer(this));
    return std::experimental::make_observer(left.get());
  }
  std::unique_ptr<Node> releaseLeft() {
    return std::move(left);
  }

  [[nodiscard]] std::experimental::observer_ptr<Node> getRight() const { return right; }
  void setRight(std::unique_ptr<Node> &&newRight) {
    newRight->parent = this;
    right = std::move(newRight);
  }
  std::experimental::observer_ptr<Node> makeRight(value_type initValue) {
    right = std::make_unique<Node>(initValue, std::experimental::make_observer(this));
    return std::experimental::make_observer(right.get());
  }
  std::unique_ptr<Node> releaseRight() {
    return std::move(right);
  }

  [[nodiscard]] const std::experimental::observer_ptr<Node> &getParent() const { return parent; }

 private:
  T value;
  std::unique_ptr<Node> left = nullptr;
  std::unique_ptr<Node> right = nullptr;
  std::experimental::observer_ptr<Node> parent;
};

#endif//HUFF_CODEC__TREE_H
