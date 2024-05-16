#include "tools.h"
#include <string.h>

const int8 argCnt[] = {
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
    1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 
    1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 
    1, 2, 1, 1, 1, 1, 0, 0, 
    9,
};

uint32 calcHash(const char *str, uint32 range) {
    uint32 res = 0;
    while (*str != '\0') res = ((((uint64)res) << 8) + *str) % range, str++;
    return res;
}

Pair *makePair(char *key, void *value) {
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
    ele->prev->next = ele;
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
    pos->prev->next = ele;
    ele->next = pos;
    pos->prev = ele;
}

void List_remove(ListElement *ele) {
    ele->prev->next = ele->next;
    ele->next->prev = ele->prev;
    ele->next = ele->prev = NULL;
}

void List_clear(ListElement *start, ListElement *end) {
    for (ListElement *ele = start->next, *nxt; ele != end; ele = nxt) {
        nxt = ele->next;
        List_remove(ele), free(ele);
    }
}

void HashMap_init(HashMap *map, uint32 range) {
    map->hashRange = range;
    map->listStart = mallocArray(PairListElement *, range);
    map->listEnd = mallocArray(PairListElement *, range);
    for (int i = 0; i < range; i++)
		map->listStart[i] = mallocObj(PairListElement),
		map->listEnd[i] = mallocObj(PairListElement),
        PairList_init(map->listStart[i], map->listEnd[i]);
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
            PairList_remove(ele);
            free(ele->content->key), free(ele->content), free(ele);
            return ;
        }
    }
}

void Trie_init(TrieNode *root) {
    memset(root->child, 0, sizeof(root->child));
    root->content = NULL;
}

void Trie_insert(TrieNode *root, const char *str, void *val) {
    TrieNode *cur = root;
    size_t len = strlen(str);
    for (int i = 0; i < len; i++) {
        if (cur->child[str[i]] == NULL) {
            cur->child[str[i]] = mallocObj(TrieNode);
            memset(cur->child[str[i]]->child, 0, sizeof(root->child));
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

void DArray_init(DArray *arr, uint64 elemSize) {
    arr->cap = arr->len = 0;
    arr->data = NULL;
    arr->elemSize = elemSize;
}

void DArray_clear(DArray *arr) {
    free(arr->data), arr->data = NULL;
    DArray_init(arr, arr->elemSize);
}

void DArray_push(DArray *arr, void *data) {
    if (arr->data == NULL) {
        arr->cap = 1;
        arr->data = malloc(arr->elemSize);
    } else if (arr->len == arr->cap) {
        arr->cap *= 2;
        arr->data = realloc(arr->data, arr->cap * arr->elemSize);
    }
    memcpy(arr->data + arr->len * arr->elemSize, data, arr->elemSize);
    arr->len++;
}

void DArray_merge(DArray *src, DArray *dst) {
    for (int i = 0; i < src->len; i++) DArray_push(dst, DArray_get(src, i));
}

void DArray_pop(DArray *arr) {
    if (arr->len == 0) return;
    arr->len--;
    if ((arr->len << 2) == arr->cap && arr->cap > 1) {
        arr->cap >>= 1;
        arr->data = realloc(arr->data, arr->cap * arr->elemSize);
    }
}

void *DArray_get(DArray *arr, uint64 index) {
    if (index >= arr->len) return NULL;
    return arr->data + index * arr->elemSize;
}

static char strBuf[1048576];
static const uint32 maxStrBufSize = 1048576;
static uint32 strBufSize;
char *readString(FILE *filePtr) {
    strBufSize = 0;
    char *res = NULL;
    uint32 resSize = 0;
    char ch;
    while (readData(filePtr, &ch, char), ch != '\0') {
        if (strBufSize == maxStrBufSize) {
            // update the size of res and copy the data in strBuf into res
            char *newRes = mallocArray(char, resSize + maxStrBufSize);
            if (res != NULL) {
                memcpy(newRes, res, resSize * sizeof(char));
                free(res);
            }
            memcpy(newRes + resSize, strBuf, sizeof(char) * maxStrBufSize);
            res = newRes;
            strBufSize = 0;
        }
        strBuf[strBufSize++] = ch;
    }
    char *newRes = mallocArray(char, resSize + strBufSize + 1);
    if (res != NULL) {
        memcpy(newRes, res, sizeof(resSize));
        free(res);
    }
    res = newRes;
    if (strBufSize > 0) memcpy(res + resSize, strBuf, sizeof(char) * strBufSize);
    resSize += strBufSize;
    res[resSize] = '\0';

    return res;
}

void writeString(FILE *filePtr, const char *str) {
    int len = strlen(str);
    fwrite(str, sizeof(char), len + 1, filePtr);
}

void ignoreString(FILE *filePtr) {
    char ch;
    while (readData(filePtr, &ch, uint8), ch != '\0') ;
}
