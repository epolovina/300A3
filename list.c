#include "list.h"

#include <stdio.h>
#include <stdlib.h>

static Node nodeArray[LIST_MAX_NUM_NODES] = {{NULL}};
static List listArray[LIST_MAX_NUM_HEADS] = {{NULL}};

static int nodeArrayIndex = 0;
static int listArrayIndex = 0;
static bool didWeInitArray = false;  // flag so we do not re-initialize array

static Node* releasedNodes = NULL;
static List* releasedHeads = NULL;

enum type { LIST, NODE };
static void stackPush(enum type type, void* item);
static void* stackPop(enum type type);
static void* getAvailableNode();
static void* getAvailableList();

List* List_create() {
  // Allocate Lists/Nodes then never go here again
  if (!didWeInitArray) {
    // Initialize list
    List* list = NULL;
    Node* node = NULL;

    for (int i = 0; i < LIST_MAX_NUM_HEADS; i++) {
      list = &listArray[i];
      list->size = 0;
      list->head = NULL;
      list->current = NULL;
      list->afterTail = false;
      list->beforeHead = false;
      list->tail = NULL;
    }

    // Initialize Nodes
    for (int i = 0; i < LIST_MAX_NUM_NODES; i++) {
      node = &nodeArray[i];
      node->data = 0;
      node->next = NULL;
      node->previous = NULL;
    }

    didWeInitArray = true;
  }

  if (didWeInitArray) {
    List* freeListNode = getAvailableList();
    if (freeListNode == NULL) {
      return NULL;
    }
    freeListNode->afterTail = false;
    freeListNode->beforeHead = false;
    freeListNode->current = NULL;
    freeListNode->head = NULL;
    freeListNode->tail = NULL;
    freeListNode->size = 0;
    return freeListNode;
  }
  return NULL;
}

// Input the address of the node or list stack based on the type
static void stackPush(enum type type, void* item) {
  if (type == NODE) {
    Node* removed = item;
    if (releasedNodes == NULL) {
      removed->next = NULL;
      removed->previous = NULL;
      releasedNodes = removed;  // add freed node to pool
    } else {
      removed->next = releasedNodes;
      removed->previous = NULL;
      releasedNodes = removed;  // add freed node to pool
    }
  } else if (type == LIST) {
    List* removed = item;
    if (releasedHeads == NULL) {
      releasedHeads = removed;  // add freed list head to pool
    } else {
      removed->next = releasedHeads;
      releasedHeads = removed;  // add freed list head to front of pool
    }
  }
  return;
}

// Return the address of the free node or list stack based on the type
static void* stackPop(enum type type) {
  if (type == NODE) {
    if (releasedNodes == NULL) {
      return NULL;
    } else {
      Node* returnNode = releasedNodes;
      releasedNodes = releasedNodes->next;
      return returnNode;
    }
  } else {
    if (releasedHeads == NULL) {
      return NULL;
    } else {
      List* returnHead = releasedHeads;
      releasedHeads = releasedHeads->next;
      return returnHead;
    }
  }
  return NULL;
}

// Fetch the Node from either stack or array
static void* getAvailableNode() {
  if (releasedNodes == NULL) {
    if (nodeArrayIndex + 1 > LIST_MAX_NUM_NODES) {
      return NULL;
    }
    return &nodeArray[nodeArrayIndex++];
  } else {
    return stackPop(NODE);
  }
  return NULL;
}

// Fetch the List from either stack or array
static void* getAvailableList() {
  if (releasedHeads == NULL) {
    if (listArrayIndex + 1 > LIST_MAX_NUM_HEADS) {
      return NULL;
    }
    return &listArray[listArrayIndex++];
  } else {
    return stackPop(LIST);
  }
  return NULL;
}

int List_count(List* pList) {
  if (pList == NULL) {
    return -1;
  }
  // return 0 if the size is -1 (nothing in the list)
  return pList->size <= -1 ? 0 : pList->size;
}

void* List_first(List* pList) {
  if (pList->size <= 0) {
    pList->current = NULL;
    return NULL;
  }
  Node* head = pList->head;
  pList->beforeHead = false;
  pList->current = head;
  return head->data;
}

void* List_last(List* pList) {
  if (pList->size <= 0 || pList->head == NULL) {
    pList->current = NULL;
    return NULL;
  }
  pList->current = pList->tail;
  pList->afterTail = false;
  Node* curr = pList->current;
  return curr->data;
}

void* List_next(List* pList) {
  if (pList->size <= 0) {
    return NULL;
  }

  // if current is beyond tail return null
  if (pList->afterTail) {
    return NULL;
  }

  // if current beyond head return head
  if (pList->beforeHead) {
    pList->current = pList->head;
    pList->beforeHead = false;
    Node* curr = pList->current;
    curr->previous = NULL;
    return curr->data;
  }

  // Check if the next item is null
  // set flags whether it is out of bounds
  Node* curr = pList->current;
  if (curr->next == NULL) {
    pList->current = NULL;
    pList->afterTail = true;
    pList->beforeHead = false;
    return NULL;
  }

  pList->current = curr->next;  // increment next pointer
  Node* node = pList->current;
  node->previous = curr;  // set new current previous to old current
  return node->data;      // return data
}

void* List_prev(List* pList) {
  if (pList->size <= 0) {
    return NULL;
  }

  // Check if current is before the head
  if (pList->beforeHead) {
    return NULL;
  }

  // Check if current is after the last element
  // if so, make curr the las element
  if (pList->afterTail) {
    pList->afterTail = false;
    pList->current = pList->tail;
    Node* curr = pList->current;
    return curr->data;
  }

  // set flags if previos is null
  Node* node = pList->current;  // make a copy of curr
  if (node->previous == NULL) {
    pList->beforeHead = true;
    pList->current = node->previous;
    return NULL;
  }

  pList->current = node->previous;  // set current to previous
  Node* curr = pList->current;
  return curr->data;
}

void* List_curr(List* pList) {
  // check if current is null
  if (pList->current == NULL || pList->size <= 0) {
    return NULL;
  }

  // return current data from meta data
  Node* curr = pList->current;
  return curr->data;
}

int List_add(List* pList, void* pItem) {
  // Fetch node from stack or array
  if (pList->size < 0) {
    return -1;
  }

  Node* node = getAvailableNode();
  if (node == NULL) {
    return -1;
  }

  // Initialize node
  node->data = pItem;
  node->next = NULL;
  node->previous = NULL;

  // If head is null, make item the head
  if (pList->head == NULL) {
    pList->head = node;
    Node* head = pList->head;
    head->previous = NULL;
    pList->tail = node;
    pList->current = node;
    pList->size++;
    return 0;
  }

  // get the item after the current
  Node* curr = pList->current;
  Node* after = curr->next;

  // set current next to the new node
  // set the nodes previous to current and the next to after
  curr->next = node;
  node->previous = curr;
  node->next = after;

  // check if the node is at the end of the list
  if (node->next == NULL) {
    pList->tail = node;
  }

  // set current and other flags
  pList->current = node;
  pList->beforeHead = false;
  pList->afterTail = false;
  pList->size++;

  return 0;
}

int List_insert(List* pList, void* pItem) {
  if (pList->size < 0) {
    return -1;
  }
  Node* node = getAvailableNode();

  // Init node
  node->data = pItem;
  node->next = NULL;
  node->previous = NULL;

  if (pList->head == NULL) {
    pList->head = node;
    Node* head = pList->head;
    head->previous = NULL;
    pList->tail = node;
    pList->current = node;
    pList->size++;
    return 0;
  }

  Node* curr = pList->current;
  node->next = curr;                // move next to current
  node->previous = curr->previous;  // move previous to curr prev

  if (curr->previous == NULL) {
    pList->head = node;
  } else {
    curr->previous->next = node;  // update prev next to new node
    curr->previous = node;
  }
  node->next = pList->current;
  pList->current = node;
  pList->beforeHead = false;
  pList->afterTail = false;
  pList->size++;

  return 0;
}

int List_append(List* pList, void* pItem) {
  if (pList->size < 0) {
    return -1;
  }
  Node* node = getAvailableNode();
  if (node == NULL) {
    return -1;
  }
  node->data = pItem;
  node->next = NULL;
  node->previous = NULL;

  if (pList->head == NULL) {
    pList->head = node;
    Node* head = pList->head;
    head->previous = NULL;
    pList->tail = node;
    pList->current = node;
    pList->size++;
    return 0;
  }

  Node* tail = pList->tail;
  pList->tail = node;
  pList->beforeHead = false;
  pList->afterTail = false;
  pList->current = node;
  tail->next = node;
  node->previous = tail;

  pList->tail = node;
  pList->size++;

  return 0;
}

int List_prepend(List* pList, void* pItem) {
  if (pList->size < 0) {
    return -1;
  }
  Node* node = getAvailableNode();

  node->data = pItem;
  node->next = NULL;

  if (pList->head == NULL) {
    pList->head = node;
    Node* head = pList->head;
    head->previous = NULL;
    pList->tail = node;
    pList->current = node;
    pList->size++;
    return 0;
  }

  node->next = pList->head;
  pList->head = node;
  pList->current = node;
  pList->beforeHead = false;
  pList->afterTail = false;
  pList->size++;

  return 0;
}

void* List_remove(List* pList) {
  Node* curr;
  Node* newCurrent;
  Node* removed;

  if (pList == NULL) {
    return NULL;
  }
  if (pList->current == NULL) {
    return NULL;
  }

  if (pList->current == pList->head) {
    curr = pList->current;
    removed = pList->current;
    curr = curr->next;
    if (curr != NULL) {
      curr->previous = NULL;
    }
    pList->head = curr == NULL ? NULL : curr;
    pList->current = curr == NULL ? NULL : curr;

    pList->size--;
    removed->next = NULL;
    removed->previous = NULL;
    stackPush(NODE, removed);  // push freed node into stack pool
    return removed->data;

  } else if (pList->current == pList->tail) {
    curr = pList->current;
    removed = pList->current;
    curr = curr->previous;
    curr->next = NULL;
    removed->next = NULL;
    removed->previous = NULL;
    pList->tail = curr;
    pList->current = curr;
    pList->size--;
    stackPush(NODE, removed);  // push freed node into stack pool
    return removed->data;
  }

  curr = pList->current;
  removed = pList->current;
  newCurrent = curr->next;
  newCurrent->previous = newCurrent->previous->previous;
  newCurrent->previous->next = newCurrent;
  pList->current = newCurrent;
  pList->size--;
  stackPush(NODE, removed);  // push freed node into stack pool
  return removed->data;
}

void List_concat(List* pList1, List* pList2) {
  if (pList1 == NULL || pList2->head == NULL) {
    return;
  }

  if (pList1->head == NULL) {
    pList1->head = pList2->head;
    pList1->size = pList2->size;
    pList1->tail = pList2->tail;
    pList1->afterTail = true;

    pList2->afterTail = false;
    pList2->beforeHead = false;
    pList2->current = NULL;
    pList2->tail = NULL;
    pList2->head = NULL;
    pList2->size = -1;
    pList2->next = NULL;
    stackPush(LIST, pList2);
    return;
  }

  Node* l1Tail = pList1->tail;
  Node* l2Head = pList2->head;

  // set tail of list1 to head of list2
  l1Tail->next = pList2->head;
  l2Head->previous = l1Tail;

  // set tail of list2 to tail of list1
  pList1->tail = pList2->tail;
  pList1->size += pList2->size;  // add the sizes of the 2 lists

  pList2->afterTail = false;
  pList2->beforeHead = false;
  pList2->current = NULL;
  pList2->tail = NULL;
  pList2->head = NULL;
  pList2->size = -1;
  pList2->next = NULL;
  stackPush(LIST, pList2);
}

void List_free(List* pList, FREE_FN pItemFreeFn) {
  // Edge case for when user frees a uninitialized list
  if (pList == NULL || pList->size < 0) {
    return;
  }

  // start freeing from the head then move next
  pList->current = pList->head;

  if (pList->head == NULL) {
    if (releasedHeads == pList) {
      return;
    }

    // reset list metadata
    pList->afterTail = false;
    pList->beforeHead = false;
    pList->current = NULL;
    pList->tail = NULL;
    pList->head = NULL;
    pList->size = -1;
    pList->next = NULL;
    stackPush(LIST, pList);
    return;
  }

  // Iterate from the head and remove each node from the head
  while (pList->head != NULL) {
    pItemFreeFn(pList->current);
    List_remove(pList);
  }

  // reset list metadata
  pList->afterTail = false;
  pList->beforeHead = false;
  pList->current = NULL;
  pList->tail = NULL;
  pList->head = NULL;
  pList->size = -1;
  pList->next = NULL;
  stackPush(LIST, pList);
  return;
}

void* List_trim(List* pList) {
  if (pList->head == NULL) {
    return NULL;
  }

  // move current to tail and call remove to remove current
  Node* tail = pList->tail;
  pList->current = tail;
  List_remove(pList);

  return tail->data;
}

void* List_search(List* pList, COMPARATOR_FN pComparator,
                  void* pComparisonArg) {
  // Stare searching from wherever current is located
  Node* curr = pList->current;

  if (curr == NULL) {
    if (pList->beforeHead) {
      pList->beforeHead = false;
      pList->current = pList->head;
      curr = pList->current;
    } else if (pList->afterTail) {
      pList->afterTail = false;
      pList->current = pList->tail;
      curr = pList->current;
    }
  }

  // iterate through list and user comparator to find a match
  while (curr != NULL) {
    bool match = pComparator(curr->data, pComparisonArg);
    if (match == true) {
      pList->current = curr;
      return curr->data;
    }
    curr = curr->next;
  }
  return NULL;
}
