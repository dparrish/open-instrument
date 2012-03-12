/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#ifndef OPENINSTRUMENT_LIB_TRIE_H_
#define OPENINSTRUMENT_LIB_TRIE_H_

#include <list>
#include <string>
#include <vector>
#include "lib/common.h"
#include "lib/string.h"

namespace openinstrument {

template<typename T>
class Trie {
 public:
  typedef string key_type;
  typedef T mapped_type;
  typedef T value_type;
  typedef T & reference;
  typedef const T & const_reference;
  typedef T * pointer;
  typedef const T * const_pointer;
  typedef size_t size_type;


  class TrieNode {
   public:
    typedef list<TrieNode *> list_type;
    typedef typename list_type::iterator iterator;

    TrieNode(TrieNode *parent = NULL, unsigned char character = '\0')
      : parent(parent),
        character(character),
        value(NULL) {
    }

    ~TrieNode() {
      clear();
    }

    void clear() {
      BOOST_FOREACH(TrieNode *node, child_nodes)
        delete node;
      child_nodes.clear();
      if (value)
        delete value;
      value = NULL;
    }

    inline bool set_value(value_type *value) {
      bool ret = false;
      if (this->value) {
        delete this->value;
        ret = true;
      }
      this->value = value;
      return ret;
    }

    inline TrieNode *get_node(unsigned char c) {
      iterator i;
      for (i = child_nodes.begin(); i != child_nodes.end(); ++i) {
        if ((*i)->character == c)
          return (*i);
        if ((*i)->character > c) {
          TrieNode *node = new TrieNode(this, c);
          child_nodes.insert(i, node);
          return node;
        }
      }
      TrieNode *node = new TrieNode(this, c);
      child_nodes.push_back(node);
      return node;
    }

    TrieNode *parent;
    const unsigned char character;
    value_type *value;
    list_type child_nodes;

    friend class TrieIterator;
  };


  class TrieIterator {
   public:
    typedef T value_type;
    typedef ptrdiff_t difference_type;
    typedef T * pointer;
    typedef T reference;
    typedef std::forward_iterator_tag iterator_category;

    explicit TrieIterator(TrieNode *curr, string key = "")
      : curr_(curr),
        returned_value(false) {
      if (!curr)
        return;
      if (curr_->character)
        key_ = curr_->character;
      if (key.size())
        key_ = key;
      child_nodes_iterator = curr_->child_nodes.begin();
      // Find the first leaf if the current node isn't a leaf
      if (!curr->value)
        ++(*this);
    }

    void operator++() {
      while (curr_) {
        if (!returned_value && child_nodes_iterator == curr_->child_nodes.begin() && curr_->value) {
          returned_value = true;
          break;
        }
        if (child_nodes_iterator == curr_->child_nodes.end()) {
          // Move back up the tree
          curr_ = curr_->parent;
          if (!curr_ || !iterators.size()) {
            // Gone all the way back to the tree root, there's nothing left
            curr_ = NULL;
            iterators.clear();
            key_ = "";
            break;
          }
          child_nodes_iterator = iterators.back();
          ++child_nodes_iterator;
          iterators.pop_back();
          key_.resize(key_.size() - 1);
          continue;
        }
        TrieNode *nextnode = *child_nodes_iterator;
        if (nextnode) {
          curr_ = nextnode;
          iterators.push_back(child_nodes_iterator);
          child_nodes_iterator = curr_->child_nodes.begin();
          key_ += curr_->character;
          returned_value = false;
          continue;
        } else {
          ++child_nodes_iterator;
        }
      }
    }

    bool operator==(const TrieIterator &other) {
      if (curr_ == other.curr_ && key_ == other.key_)
        return true;
      return false;
    }

    bool operator!=(const TrieIterator &other) {
      if (curr_ == other.curr_ && key_ == other.key_)
        return false;
      return true;
    }

    const string &key() const {
      return key_;
    }

    T &operator*() {
      if (!curr_ || !curr_->value)
        throw out_of_range("No current value");
      return *curr_->value;
    }

    T *operator->() {
      if (!curr_)
        return NULL;
      return curr_->value;
    }

   private:
    void erase() {
      if (!curr_)
        return;
      curr_->set_value(NULL);
    }

    TrieNode *curr_;
    typename TrieNode::iterator child_nodes_iterator;
    list<typename TrieNode::iterator> iterators;
    string key_;
    bool returned_value;

    friend class Trie;
  };


  Trie()
    : size_(0),
      root_(NULL, 0) {
  }

  typedef TrieIterator iterator;
  typedef const iterator const_iterator;

  T &operator[](const key_type &key) {
    iterator it = find(key);
    if (it == end())
      throw out_of_range("Key not found in Trie");
    return *it;
  }

  void insert(const key_type &key, const_reference val) {
    list<unsigned char> keychars;
    BOOST_FOREACH(unsigned char c, key)
      keychars.push_back(c);
    TrieNode *p = &root_;
    while (keychars.size() && p) {
      if (keychars.size() == 1) {
        p = p->get_node(keychars.front());
        if (!p->set_value(new value_type(val)))
          ++size_;
        return;
      } else {
        p = p->get_node(keychars.front());
      }
      keychars.pop_front();
    }
    throw runtime_error("Trie::insert fell through the loop");
  }

  void erase(const key_type &key) {
    iterator it = find(key);
    if (it != end())
      erase(it);
  }

  void erase(iterator &it) {
    if (it == end())
      return;
    it.erase();
    --size_;
  }

  void clear() {
    root_.clear();
    size_ = 0;
  }

  iterator find(const key_type &key) {
    size_t key_index = 0;
    TrieNode *curr = &root_;
    while (curr) {
      if (key_index == key.size()) {
        // Last character
        if (!curr->value)
          return end();
        // key_index has already been incremented past the end of the string
        if (curr->character == key[key_index - 1])
          return TrieIterator(curr, key);
        return end();
      }
      typename TrieNode::iterator i;
      for (i = curr->child_nodes.begin(); i != curr->child_nodes.end(); ++i) {
        if ((*i)->character > key[key_index])
          return end();
        if ((*i)->character == key[key_index])
          break;
      }
      if (i == curr->child_nodes.end())
        return end();
      curr = (*i);
      key_index++;
    }
    return end();
  }

  // Return an iterator to the first item where the key starts with <prefix>.
  // The iterator will only iterate through the branch that contains the prefix so every returned item should match the
  // prefix.
  iterator find_prefix(const key_type &prefix) {
    size_t key_index = 0;
    TrieNode *curr = &root_;
    while (curr) {
      if (key_index == prefix.size()) {
        // key_index has already been incremented past the end of the string
        if (curr->character == prefix[key_index - 1])
          return TrieIterator(curr, prefix);
        return end();
      }
      typename TrieNode::iterator i;
      for (i = curr->child_nodes.begin(); i != curr->child_nodes.end(); ++i) {
        if ((*i)->character > prefix[key_index])
          return end();
        if ((*i)->character == prefix[key_index])
          break;
      }
      if (i == curr->child_nodes.end())
        return end();
      curr = (*i);
      key_index++;
    }
    return end();
  }

  iterator begin() {
    return TrieIterator(&root_);
  }

  iterator end() {
    return TrieIterator(NULL);
  }

  inline bool empty() const {
    return size_ == 0;
  }

  inline size_type size() const {
    return size_;
  }

 private:
  size_type size_;
  TrieNode root_;
};


}  // namespace openinstrument

#endif  // _OPENINSTRUMENT_LIB_PROTOBUF_H_
