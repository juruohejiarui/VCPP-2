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

#define mallocObj(type) ((type *)malloc(sizeof(type)))
#define mallocArray(type, size) ((type *)malloc(sizeof(type) * (size)))

uint32 calcHash(char *str, uint32 range);

typedef struct tmpPair {
    size_t keyLength;
    const char *key;
    void *value;
} Pair;
/// @brief allocate a pair
/// @param key the key of this pair
/// @param value the value of this pair
/// @return the pair
Pair *makePair(const char *key, void *value);

typedef struct tmpPairListElement {
    Pair *content;
    struct tmpPairListElement *prev, *next;
} PairListElement;

/// @brief Initialize a list using the pointers START and END
/// @param start 
/// @param end 
void PairList_init(PairListElement *start, PairListElement *end);
/// @brief Insert the ELE in front of POS
/// @param pos 
/// @param ele 
void PairList_insert(PairListElement *pos, PairListElement *ele);
/// @brief Remove the ELE from the list that ELE currently belongs to (this action will not free the memory of ELE)
/// @param ele 
void PairList_remove(PairListElement *ele);

typedef struct tmpListElement {
    void *content;
    struct tmpListElement *prev, *next;
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
    PairListElement **listStart, **listEnd;
} HashMap;

/// @brief Initialize MAP using RANGE
/// @param map
/// @param range 
void HashMap_init(HashMap *map, uint32 range);
/// @brief Insert PIR into MAP, if the key exists, the key in PIR and PIR itself will be free
/// @param map 
/// @param pir 
void HashMap_insert(HashMap *map, Pair *pir);
/// @brief Get the pair whose key is STR from MAP, if the key does not exist, then return NULL
/// @param map 
/// @param key 
/// @return 
Pair *HashMap_find(HashMap *map, const char *key);
/// @brief Erase the pair whose key is STR, and free the memory of the key, the pair itself (will not free the value) and the element
/// @param map 
/// @param key
void HashMap_erase(HashMap *map, const char *key);

typedef struct tmpTrieNode {
    void *content;
    struct tmpTrieNode *child[128];
} TrieNode;

/// @brief Initialize the Trie
/// @param root 
void Trie_init(TrieNode *root);
/// @brief Insert STR and the VAL into a trie whose root is ROOT
/// @param root 
/// @param str 
/// @param value 
void Trie_insert(TrieNode *root, const char *str, void *val);
/// @brief get the value where the str is in the trie whose root is ROOT, return NULL if STR does not exist.
/// @param root 
/// @param str 
/// @return 
void *Trie_get(TrieNode *root, const char *str);

uint8 readuint8(FILE *file);

#endif