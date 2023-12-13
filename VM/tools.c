#include "tools.h"
#include <string.h>

uint32 calcHash(char *str, uint32 range) {
    uint32 res = 0;
    while (*str != '\0') res = ((((uint64)res) << 8) + *str) % range, str++;
    return res;
}

Pair *makePair(const char *key, void *value) {
    Pair *res = (Pair *)malloc(sizeof(Pair));
    res->keyLength = strlen(key) + 1;
    res->key = malloc(sizeof(res->keyLength));
    memcpy(res->key, key, res->keyLength);
    res->value = value;
    return res;
}

void PairList_init(PairListElement *start, PairListElement *end) {
    start->next = end;
    end->prev = start;
    start->content = end->content = NULL;
    start->prev = end->next = NULL;
}

void PairList_insert(PairListElement *pos, PairListElement *ele) {
    ele->prev = pos->prev;
    ele->next = pos;
    pos->prev = ele;
}

void PairList_remove(PairListElement *ele) {
    ele->prev->next = ele->next;
    ele->next->prev = ele->prev;
    ele->next = ele->prev = NULL;
}

void List_init(ListElement *start, ListElement *end) {
    start->next = end;
    end->prev = start;
    start->content = end->content = NULL;
    start->prev = end->next = NULL;
}

void List_insert(ListElement *pos, ListElement *ele) {
    ele->prev = pos->prev;
    ele->next = pos;
    pos->prev = ele;
}

void List_remove(ListElement *ele) {
    ele->prev->next = ele->next;
    ele->next->prev = ele->prev;
    ele->next = ele->prev = NULL;
}

void HashMap_init(HashMap *map, uint32 range) {
    map->hashRange = range;
    map->listStart = mallocArray(ListElement, range);
    map->listEnd = mallocArray(ListElement, range);
    for (int i = 0; i < range; i++)
        List_init(&map->listStart[i], &map->listEnd[i]);
}

void HashMap_insert(HashMap *map, Pair *pir) {
    uint32 hash = calcHash(pir->key, map->hashRange);
    for (PairListElement *ele = map->listStart[hash]->next; ele != map->listEnd[hash]; ele = ele->next) {
        // the key exists
        if (ele->content->keyLength == pir->keyLength && strcmp(ele->content->key, pir->key)) {
            ele->content->value = pir->value;
            free(pir->key), free(pir);
            return ;
        }
    }
    // the key does not exist -> create a new pair
    ListElement *ele = mallocObj(ListElement);
    ele->content = pir;
}

Pair *HashMap_find(HashMap *map, const char *key) {
    uint32 hash = calcHash(key, map->hashRange);
    size_t len = strlen(key);
    for (PairListElement *ele = map->listStart[hash]->next; ele != map->listEnd[hash]; ele = ele->next) {
        // the key exists
        if (ele->content->keyLength == len && strcmp(ele->content->key, key)) return ele->content;
    }
    return NULL;
}

void HashMap_erase(HashMap *map, const char *key) {
    uint32 hash = calcHash(key, map->hashRange);
    size_t len = strlen(key);
    for (PairListElement *ele = map->listStart[hash]->next; ele != map->listEnd[hash]; ele = ele->next) {
        // the key exists
        if (ele->content->keyLength == len && strcmp(ele->content->key, key)) {
            List_remove(ele);
            free(ele->content->key), free(ele->content), free(ele);
            return ;
        }
    }
}

void Trie_init(TrieNode *root) {
    memset(root->child, NULL, sizeof(root->child));
    root->content = NULL;
}

void Trie_insert(TrieNode *root, const char *str, void *val) {
    TrieNode *cur = root;
    size_t len = strlen(str);
    for (int i = 0; i < len; i++) {
        if (cur->child[str[i]] == NULL) {
            cur->child[str[i]] = mallocObj(TrieNode);
            memset(cur->child[str[i]]->child, NULL, sizeof(root->child));
            cur->child[str[i]]->content = NULL;
        }
        cur = cur->child[str[i]];
    }
    cur->content = val;
}

void *Trie_get(TrieNode *root, const char *str) {
    TrieNode *cur = root;
    size_t len = strlen(str);
    for (int i = 0; i < len; i++) {
        if (cur->child[str[i]] == NULL) return NULL;
        cur = cur->child[str[i]];
    }
    return cur->content;
}