#ifndef __TOOLS_H__
#define __TOOLS_H__

#include <stdlib.h>
#include <stdio.h>

typedef unsigned long long uint64;
typedef unsigned int uint32;
typedef unsigned short uint16;
typedef unsigned char uint8;
typedef long long int64;
typedef int int32;
typedef short int16;
typedef char int8;
typedef float float32;
typedef double float64;

uint32 calcHash(const char *str);

typedef struct tmpPair {
    int keyLength;
    const char *key;
    void *value;
} Pair;
Pair *makePair(const char *key, void *value);

typedef struct tmpListElement {
    Pair *content;
    struct tmpListElement *previous, *next;
} ListElement;

/// @brief Initialize a list using the pointers START and END
/// @param start 
/// @param end 
void List_init(ListElement *start, ListElement *end);
/// @brief Insert the ELE in front of POS
/// @param pos 
/// @param ele 
void List_insert(ListElement *pos, ListElement *ele);
/// @brief Remove the ELE from the list that ELE currently belongs to (this action will not free the memory of ELE)
/// @param ele 
void List_remove(ListElement *ele);


typedef struct tmpHashMap {
    uint32 hashRange;
    ListElement *listStart, *listEnd;
} HashMap;

/// @brief Initialize MAP using RANGE
/// @param map
/// @param range 
void HashMap_init(HashMap *map, uint32 range);
/// @brief Insert PIR into MAP
/// @param map 
/// @param pir 
void HashMap_insert(HashMap *map, Pair *pir);
/// @brief Get the pair whose key is STR from MAP
/// @param map 
/// @param str 
/// @return 
Pair *HashMap_find(HashMap *map, const char *str);
/// @brief Erase the pair whose key is STR, and free the memory of that pair
/// @param map 
/// @param str 
void HashMap_erase(HashMap *map, const char *str);

typedef struct tmpTrie {
    void *content;
    tmpTrie *str[128];
} TrieNode;

void Trie_init(TrieNode *root);
void Trie_insert(TrieNode *root, const char *str, void *value);
void *Trie_get(TrieNode *root, const char *str);

#endif